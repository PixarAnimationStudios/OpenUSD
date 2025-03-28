//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/bin/usdBakeMtlx/bakeMaterialX.h"

#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMtlx/reader.h"
#include "pxr/usd/usdMtlx/utils.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"
#include "pxr/usd/usdShade/shader.h"

#include <MaterialXCore/Document.h>
#include <MaterialXCore/Generated.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXRenderGlsl/TextureBaker.h>


#include <fstream>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)
    ((mtlxSurface, "mtlx:surface"))
    (surface)
);


namespace {

/// Read a MaterialX document then convert it using UsdMtlxRead().
template <typename R>
static
UsdStageRefPtr
_ReadMtlxToStage(R&& reader, UsdStageRefPtr stage)
{
    try {
        auto doc = reader();
        if (!doc) {
            return TfNullPtr;
        }
        UsdMtlxRead(doc, stage);
        return stage;
    }
    catch (mx::ExceptionFoundCycle& x) {
        TF_RUNTIME_ERROR("MaterialX cycle found: %s", x.what());
        return TfNullPtr;
    }
    catch (mx::Exception& x) {
        TF_RUNTIME_ERROR("MaterialX read failed: %s", x.what());
        return TfNullPtr;
    }
}

} // anonymous namespace

UsdStageRefPtr 
UsdBakeMtlxReadDocToStage(std::string const &pathname, UsdStageRefPtr stage)
{
    return _ReadMtlxToStage(
        [&](){ return UsdMtlxReadDocument(pathname); },
        stage);
}


static
mx::FileSearchPath _GetMtlxSearchPaths()
{
    // The mx::TextureBaker adds the 'libraries' folder to the search paths
    // when registering them. However, these searchPaths may include that 
    // folder in the path, so we need to remove it here. 
    mx::FileSearchPath searchPaths;
    mx::FileSearchPath originalSearchPaths = HdMtlxSearchPaths();
    for (const mx::FilePath &path : originalSearchPaths) {

        // Remove the "libraries" part from the searchPath 
        if (path.getBaseName() == "libraries") {
            searchPaths.append(path.getParentPath());
        }
        else {
            searchPaths.append(path);
        }
    }
    return searchPaths;
}

static
UsdShadeShader _GetSurfaceSource(UsdShadeMaterial const &mtlxMaterial)
{
    UsdShadeOutput output = mtlxMaterial.GetOutput(_tokens->mtlxSurface);
    if (output) {
        UsdShadeAttributeVector valueAttrs =
            UsdShadeUtils::GetValueProducingAttributes(
                output, /*shaderOutputsOnly*/true);
        return UsdShadeShader(valueAttrs[0].GetPrim());
    }
    return UsdShadeShader();
}

static 
void _BakeMtlxDocument(
    mx::DocumentPtr const &mtlxDoc,
    mx::FileSearchPath const &searchPath,
    mx::DocumentPtr const &stdLibraries, 
    mx::FilePath const &bakeFilename,
    int textureWidth,
    int textureHeight,
    bool bakeHdr,
    bool bakeAverage)
{
    mx::Image::BaseType baseType = bakeHdr 
        ? mx::Image::BaseType::FLOAT 
        : mx::Image::BaseType::UINT8;

    // Construct a Texture Baker.
#if MATERIALX_MAJOR_VERSION <= 1 && MATERIALX_MINOR_VERSION <= 38 && \
    MATERIALX_BUILD_VERSION <= 6
    mx::TextureBakerPtr baker = mx::TextureBaker::create(
        textureWidth, textureHeight, baseType);
#else
    mx::TextureBakerPtr baker = mx::TextureBakerGlsl::create(
        textureWidth, textureHeight, baseType);
#endif
    baker->setupUnitSystem(stdLibraries);
    baker->setAverageImages(bakeAverage);

    // Bake all materials in the active document.
    try {
        baker->bakeAllMaterials(mtlxDoc, searchPath, bakeFilename);
    }
    catch (std::exception& e) {
        TF_RUNTIME_ERROR("Error in texture baking: %s", e.what());
    }
}


std::string
UsdBakeMtlxBakeMaterial(
    UsdShadeMaterial const& mtlxMaterial,
    std::string const& bakedMtlxDir,
    int textureWidth,
    int textureHeight,
    bool bakeHdr,
    bool bakeAverage)
{
    // Get the surface shader node
    const UsdShadeShader mtlxShader = _GetSurfaceSource(mtlxMaterial);
    SdfPath const &terminalPath = mtlxShader.GetPath();

    // Convert to HdMaterialNetwork
    HdMaterialNetworkMap networkMap;
    UsdImagingBuildHdMaterialNetworkFromTerminal(
        mtlxShader.GetPrim(), _tokens->surface, {_tokens->mtlx},
        {_tokens->mtlx}, &networkMap, UsdTimeCode());

    // Convert to HdMaterialNetwork2
    bool isVolume = false;
    const HdMaterialNetwork2 network2 =
        HdConvertToHdMaterialNetwork2(networkMap, &isVolume);
    if (isVolume) {
        // Not supported
        return std::string();
    }

    // Load Standard Libraries/setup SearchPaths
    // XXX This does not follow the pattern used elsewhere because of how
    // mx::TextureBaker is registering the searchPaths. This means that in
    // order for this baking to work the user cannot change the name of the
    // libraries folder. 
    mx::FilePathVec libraryFolders = { "libraries", };
    mx::FileSearchPath searchPath = _GetMtlxSearchPaths();
    mx::DocumentPtr stdLibraries = mx::createDocument();
    mx::loadLibraries(libraryFolders, searchPath, stdLibraries);

    // Get Terminal Node 
    auto const terminalNodeItr = network2.nodes.find(terminalPath);
    if (terminalNodeItr == network2.nodes.end()) {
        return std::string();
    }
    HdMaterialNode2 terminalNode = terminalNodeItr->second;

    // Create a MaterialX Document
    mx::DocumentPtr mtlxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(network2,
        terminalNode, terminalPath, mtlxMaterial.GetPath(), stdLibraries);

    // Bake the MaterialX material. The baked mtlx file and associated textures
    // will all be in the bakedMtlxDir.
    mx::FilePath bakedMtlxFilename(
        mtlxMaterial.GetPath().GetName() + "_baked.mtlx");
    mx::FilePath bakedPath = mx::FilePath(bakedMtlxDir) / bakedMtlxFilename;
    _BakeMtlxDocument(mtlxDoc, searchPath, stdLibraries, bakedPath, 
        textureWidth, textureHeight, bakeHdr, bakeAverage);
    return bakedPath;
}

PXR_NAMESPACE_CLOSE_SCOPE
