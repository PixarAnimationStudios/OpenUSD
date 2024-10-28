//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <fstream>
#include <iostream>

#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXGenShader/Util.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace mx = MaterialX;

// Helpers to determine the Material Tag
// Note that similar helpers live in HdStMaterialXFilter

static bool _IsDifferentFrom(mx::InputPtr const &mxInput, float value) {
  if (!mxInput) {
    return false;
  }

  if (mxInput->hasValue()) {
    const mx::ValuePtr val = mxInput->getValue();
    if (val->isA<float>()) {
      return val->asA<float>() != value;
    }
    return true;
  }

  return mxInput->hasNodeName() || mxInput->hasNodeGraphString() ||
         mxInput->hasOutputString() || mxInput->hasInterfaceName();
}

static TfToken const &
_GetUsdPreviewSurfaceMaterialTag(mx::NodePtr const &terminalNode) {
  // See https://openusd.org/release/spec_usdpreviewsurface.html
  // and implementation in MaterialX libraries/bxdf/usd_preview_surface.mtlx

  // Non-zero opacityThreshold (or connected) triggers masked mode:
  if (_IsDifferentFrom(terminalNode->getInput("opacityThreshold"), 0.0f)) {
    return HdStMaterialTagTokens->masked;
  }

  // Opacity less than 1.0 (or connected) triggers transparent mode:
  if (_IsDifferentFrom(terminalNode->getInput("opacity"), 1.0f)) {
    return HdStMaterialTagTokens->translucent;
  }

  return HdStMaterialTagTokens->defaultMaterialTag;
}

static TfToken _GetMaterialTag(mx::DocumentPtr const &mxDoc) {
  // Find renderable elements in the Mtlx Document.
  // Note this code also lives in HdSt_GenMaterialXShader()
  std::vector<mx::TypedElementPtr> renderableElements;
  mx::findRenderableElements(mxDoc, renderableElements);

  // Should have exactly one renderable element (material).
  if (renderableElements.size() != 1) {
    TF_CODING_ERROR("Generated MaterialX Document does not "
                    "have 1 material");
    return HdStMaterialTagTokens->defaultMaterialTag;
  }

  // Extract out the Surface Shader Node for the Material Node
  mx::TypedElementPtr renderableElem = renderableElements.at(0);
  mx::NodePtr node = renderableElem->asA<mx::Node>();
  if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
    // Use auto so can compile against MaterialX 1.38.0 or 1.38.1
    auto mxShaderNodes =
        mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
    if (!mxShaderNodes.empty()) {
      renderableElem = *mxShaderNodes.begin();
    }
  }

  // The custom code to handle masked mode prevents MaterialX from
  // correctly deducing transparency with mx::isTransparentSurface()
  node = renderableElem->asA<mx::Node>();
  if (node && node->getCategory() == "UsdPreviewSurface") {
    return _GetUsdPreviewSurfaceMaterialTag(node);
  }

  // XXX: Once other material tests are added (eg. glTf) similar helper
  // helper functions will need to be added to get the correct MaterialTag
  if (mx::isTransparentSurface(renderableElem)) {
    return HdStMaterialTagTokens->translucent;
  }
  return HdStMaterialTagTokens->defaultMaterialTag;
}

namespace {

VtValue _SanitiseValue(const VtValue &value) {
  if (value.IsHolding<SdfAssetPath>()) {
    const SdfAssetPath assetPath = value.UncheckedGet<SdfAssetPath>();

    // Special case for openvdb assets, which use pointer addresses for
    // keys, which will change on each test run.
    if (TfStringStartsWith(assetPath.GetResolvedPath(), "openvdb-stream://")) {
      static const SdfAssetPath sanitisedAssetPath(
          "openvdb-stream://<MEMORY_ADDRESS>",
          "openvdb-stream://<MEMORY_ADDRESS>");
      return VtValue(sanitisedAssetPath);
    }

    // "Un-resolve" asset paths, just because their resolved paths might
    // vary based on whether they were run locally or remotely.
    return VtValue(
        SdfAssetPath(assetPath.GetAssetPath(), assetPath.GetAssetPath()));
  }

  return value;
}

VtValue _GetValueForDS(const HdDataSourceBaseHandle &dataSource);

std::vector<VtValue>
_GetValueForVector(const HdVectorDataSourceHandle &dataSource) {
  std::vector<VtValue> ret;
  if (dataSource) {
    for (size_t i = 0; i < dataSource->GetNumElements(); ++i) {
      ret.push_back(_GetValueForDS(dataSource->GetElement(i)));
    }
  }
  return ret;
}

VtDictionary _GetDictForDS(const HdContainerDataSourceHandle &dataSource) {
  VtDictionary dict;
  if (dataSource) {
    for (const auto &name : dataSource->GetNames()) {
      HdDataSourceBaseHandle child = dataSource->Get(name);
      dict[name.GetString()] = _GetValueForDS(child);
    }
  }

  return dict;
}

VtValue _GetValueForDS(const HdDataSourceBaseHandle &dataSource) {
  if (auto container =
          std::dynamic_pointer_cast<HdContainerDataSource>(dataSource)) {
    return VtValue(_GetDictForDS(container));
  } else if (auto vector =
                 std::dynamic_pointer_cast<HdVectorDataSource>(dataSource)) {
    return VtValue(_GetValueForVector(vector));
  } else if (auto sampled =
                 std::dynamic_pointer_cast<HdSampledDataSource>(dataSource)) {
    VtValue value = sampled->GetValue(0.0f);
    return _SanitiseValue(value);
  }
  return VtValue("UNKNOWN TYPE");
}

VtDictionary _GetDictForPrim(const HdSceneIndexBaseRefPtr &sceneIndex,
                             const SdfPath &path) {
  VtDictionary children;
  for (const auto &childPrim : sceneIndex->GetChildPrimPaths(path)) {
    children[childPrim.GetName()] = _GetDictForPrim(sceneIndex, childPrim);
  }

  HdSceneIndexPrim prim = sceneIndex->GetPrim(path);
  VtDictionary attributes = _GetDictForDS(prim.dataSource);

  VtDictionary dict;
  dict["type"] = prim.primType.GetString();
  if (!children.empty()) {
    dict["children"] = std::move(children);
  }
  if (!attributes.empty()) {
    dict["attributes"] = std::move(attributes);
  }
  return dict;
}

HdSceneIndexBaseRefPtr
_HydraDumpForUsdStage(const UsdStagePtr &stage,
                      //    const SdfPath& sceneGraphPath,
                      const UsdTimeCode &frame)
//    ,
//    VtDictionary* out)
{
  Hd_UnitTestNullRenderDelegate renderDelegate;
  std::unique_ptr<HdRenderIndex> renderIndex(
      HdRenderIndex::New(&renderDelegate, HdDriverVector()));

  UsdImagingCreateSceneIndicesInfo info;
  info.stage = stage;
  const UsdImagingSceneIndices sceneIndices =
      UsdImagingCreateSceneIndices(info);
  UsdImagingStageSceneIndexRefPtr stageSceneIndex =
      sceneIndices.stageSceneIndex;
  stageSceneIndex->SetTime(frame);
  HdSceneIndexBaseRefPtr finalSceneIndex = sceneIndices.finalSceneIndex;
  renderIndex->InsertSceneIndex(finalSceneIndex, SdfPath("/"));

  // NOTE: this makes assumptions based on scene index emulation and will
  //       need to be updated when HdRenderIndex no longer uses the
  //       emulated legacy APIs.
  auto registeredSceneIndexNames =
      HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();

  if (registeredSceneIndexNames.size() != 1) {
    std::cerr << "expecting 1 registered scene index (via emulation) and found "
              << registeredSceneIndexNames.size() << " instead." << std::endl;
    return nullptr;
  }

  HdSceneIndexBaseRefPtr sceneIndex =
      HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
          registeredSceneIndexNames[0]);

  if (!sceneIndex) {
    std::cerr << "registered scene index is null." << std::endl;
    return nullptr;
  }

  return sceneIndex;
  /*
  _RecordingSceneIndexObserver observer;
  sceneIndex->AddObserver(HdSceneIndexObserverPtr(&observer));
  */

  //  VtDictionary dict = _GetDictForPrim(sceneIndex, sceneGraphPath);
  //  if (out) {
  //    std::swap(*out, dict);
  //  }
  //  return true;
}

}; // namespace

class TestStageGlobals : public UsdImagingDataSourceStageGlobals {
public:
  TestStageGlobals() : _time(0) {}
  ~TestStageGlobals() override = default;

  UsdTimeCode GetTime() const override { return UsdTimeCode(_time); }

  void FlagAsTimeVarying(const SdfPath &hydraPath,
                         const HdDataSourceLocator &locator) const override {
    _timeVarying[hydraPath].insert(locator);
  }

  void FlagAsAssetPathDependent(const SdfPath &usdPath) const override {
    _assetPathDependent.insert(usdPath);
  }

  HdDataSourceLocatorSet const &
  GetTimeVaryingLocators(SdfPath const &hydraPath) const {
    return _timeVarying[hydraPath];
  }

  std::set<SdfPath> const &GetAssetPathDependents() const {
    return _assetPathDependent;
  }

private:
  double _time;
  mutable std::map<SdfPath, HdDataSourceLocatorSet> _timeVarying;
  mutable std::set<SdfPath> _assetPathDependent;
};

void TestHdMtlx(const std::string &inputUsdFilename,
                const std::string &outputMtlxFilename) {
  UsdStageRefPtr stage = UsdStage::Open(inputUsdFilename);

  UsdTimeCode frame(0);

  Hd_UnitTestNullRenderDelegate renderDelegate;
  std::unique_ptr<HdRenderIndex> renderIndex(
      HdRenderIndex::New(&renderDelegate, HdDriverVector()));

  UsdImagingCreateSceneIndicesInfo info;
  info.stage = stage;
  const UsdImagingSceneIndices sceneIndices = UsdImagingCreateSceneIndices(info);
  UsdImagingStageSceneIndexRefPtr stageSceneIndex = sceneIndices.stageSceneIndex;
  stageSceneIndex->SetTime(frame);
  HdSceneIndexBaseRefPtr finalSceneIndex = sceneIndices.finalSceneIndex;
  renderIndex->InsertSceneIndex(finalSceneIndex, SdfPath("/"));

  // NOTE: this makes assumptions based on scene index emulation and will
  //       need to be updated when HdRenderIndex no longer uses the
  //       emulated legacy APIs.
  auto registeredSceneIndexNames = HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();

  if (registeredSceneIndexNames.size() != 1) {
    std::cerr << "expecting 1 registered scene index (via emulation) and found "
              << registeredSceneIndexNames.size() << " instead." << std::endl;
    return;
  }

  HdSceneIndexBaseRefPtr sceneIndex =
      HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
          registeredSceneIndexNames[0]);

  if (!sceneIndex) {
    std::cerr << "registered scene index is null." << std::endl;
    return;
  }

//
//  SdfPathVector children =  sceneIndex->GetChildPrimPaths( SdfPath("/"));
//
//  for (auto & chi : children)
//  {
//    printf("BBBB : '%s'\n", chi.GetText());
//  }




  auto mtlPath = SdfPath("/MaterialX/Materials/Default");


  auto bbb = stageSceneIndex->GetPrim(mtlPath);

  HdSceneIndexPrim prim = sceneIndex->GetPrim(mtlPath);


  auto mtlUsdPrim = stage->GetPrimAtPath(mtlPath);

  TestStageGlobals stageGlobals;

  auto imgPrim = UsdImagingDataSourceMaterialPrim::New(mtlUsdPrim.GetPath(),
                                                       mtlUsdPrim, stageGlobals);


  auto mtlDataSource = imgPrim->Get(TfToken("material"));


  HdContainerDataSourceHandle networkContainerParent = HdContainerDataSource::Cast(mtlDataSource);

 auto taco =   networkContainerParent->Get(TfToken("mtlx"));

   HdContainerDataSourceHandle networkContainer1 = HdContainerDataSource::Cast(taco);


  HdContainerDataSourceHandle networkContainer2 = HdContainerDataSource::Cast(imgPrim);


  auto yyyy = HdDataSourceMaterialNetworkInterface(
      mtlUsdPrim.GetPath(), networkContainer1, prim.dataSource);




 auto bbc =    yyyy.GetMaterialConfigValue(TfToken("mtlx:version"));


  const mx::DocumentPtr &stdLibraries = HdMtlxStdLibraries();

  TfToken terminalNodeName = TfToken("ND_standard_surface_surfaceshader");
  TfTokenVector terminalNodeConnectionNames = {};
  HdMtlxTexturePrimvarData *mxHdData = nullptr;


//  auto ver = hdNetwork.config.GetValueAtPath("mtlx:version");


  MaterialX::DocumentPtr doc =
      HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
          &yyyy, terminalNodeName, terminalNodeConnectionNames, stdLibraries,
          mxHdData);

  //
  //
  //  // Get Standard Libraries and SearchPaths (for mxDoc and mxShaderGen)
  //  const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
  //  const mx::FileSearchPath& searchPaths = HdMtlxSearchPaths();
  //
  //  // Read the mtlx file
  //  mx::DocumentPtr mxDoc = mx::createDocument();
  //  fprintf(stderr, "reading the mtlx file: \n - %s\n",
  //  mtlxFilename.asString().c_str());
  //
  //  try {
  //    mx::readFromXmlFile(mxDoc, mtlxFilename, searchPaths);
  //  } catch (const mx::Exception& e) {
  //    TF_WARN("mx::readFromXmlFile threw an exception: %s", e.what());
  //    return;
  //  }
  //
  //  mxDoc->importLibrary(stdLibraries);
  //
  //  // Validate the document.
  //  std::string message;
  //  if (!mxDoc->validate(&message)) {
  //    std::cerr << "*** Validation warnings for "
  //              << mtlxFilename.getBaseName() << " ***" << std::endl;
  //    std::cerr << message;
  //  }
  //
  //  mxHdInfo->materialTag = _GetMaterialTag(mxDoc);
  //
  //  // Generate the HdSt MaterialX Shader
  //  mx::ShaderPtr glslfx = HdSt_GenMaterialXShader(
  //      mxDoc, stdLibraries, searchPaths, *mxHdInfo, HgiTokens->OpenGL);
  //  std::cout << glslfx->getSourceCode(mx::Stage::PIXEL);
}

int main(int argc, char *argv[]) {
  HdSt_MxShaderGenInfo mxHdInfo;
  mx::FilePath mtlxFile = "standard_surface_default.mtlx";

  for (int i = 0; i < argc; ++i) {
    const std::string arg(argv[i]);

    if (arg == "--filename") {
      mtlxFile = mx::FilePath(argv[++i]);
    }
    //    if (arg == "--textureMap") {
    //      const std::string textureMap(argv[++i]);
    //      auto const& npos = textureMap.find(":");
    //      if (npos != std::string::npos) {
    //        const std::string mx = textureMap.substr(0, npos);
    //        const std::string hd = textureMap.substr(npos + 1);
    //        mxHdInfo.textureMap[mx] = hd;
    //      }
    //      else {
    //        std::cerr << "textureMap input not formatted correctly.\n";
    //        std::cerr << "--textureMap <mxTextureNodeName>:<HdInptName>\n";
    //        return EXIT_FAILURE;
    //      }
    //    }
    //    if (arg == "--primvarMap") {
    //      const std::string primvarMap(argv[++i]);
    //      auto const& npos = primvarMap.find(":");
    //      if (npos != std::string::npos) {
    //        const std::string name = primvarMap.substr(0, npos);
    //        const std::string type = primvarMap.substr(npos + 1);
    //        mxHdInfo.primvarMap[name] = type;
    //      }
    //      else {
    //        std::cerr << "primvarMap input not formatted correctly.\n";
    //        std::cerr << "--primvarMap <PrimvarName>:<PrimvarType>\n";
    //        return EXIT_FAILURE;
    //      }
    //    }
    //    if (arg == "--bindless") {
    //      mxHdInfo.bindlessTexturesEnabled = true;
    //    }
  }

  TfErrorMark mark;

  std::ofstream out1("hdMtlx_log.txt");

  out1 << "taco" << std::endl;

  std::string usdFilename = "test_scene.usda";
  std::string outMtlxFilename = "test_out.mtlx";

//  TestHdMtlx(usdFilename, outMtlxFilename);
  TestHdMtlx(mtlxFile, outMtlxFilename);

  TF_VERIFY(mark.IsClean());

  if (mark.IsClean()) {
    std::cout << "OK" << std::endl;
    return EXIT_SUCCESS;
  } else {
    std::cout << "FAILED" << std::endl;
    return EXIT_FAILURE;
  }
}
