//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"

#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/staticTokens.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define TEST_USD_PROCEDURAL_EXTERNAL_ASSETS_FILE_FORMAT_TOKENS \
    ((Id, "Test_UsdProceduralExternalAssetsFileFormatPlugin")) \
    ((Version, "1.0")) \
    ((Target, "usd")) \
    ((Extension, "test_usd_pea"))

TF_DECLARE_PUBLIC_TOKENS(Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens,
                         TEST_USD_PROCEDURAL_EXTERNAL_ASSETS_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens, 
    TEST_USD_PROCEDURAL_EXTERNAL_ASSETS_FILE_FORMAT_TOKENS);

/// \class Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
///
/// This is a contrived example of a file format that demonstrates the use
/// of external asset dependencies.
///
/// Layers of this file format will generate its contents by looking in the
/// directory that the layer file is in and finding all files that represent
/// a valid layer file format. A temporary stage is created with prims that 
/// reference each of these layer files and the final generated layer contains 
/// the flattened contents of this temporary stage.
/// 
/// What this gives us is a layer whose contents depend on the existence and 
/// contents of other layers but these dependent layers do not remain open and 
/// cannot be discovered through composition. Thus, this file format implements
/// GetExternalAssetDependencies in order to communicate which other assets
/// its layers depend on for both dependency analysis and determining when 
/// the layer should be reloaded.
class Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat : 
    public SdfFileFormat
{
public:

    // SdfFileFormat overrides.
    bool CanRead(const std::string &file) const override;
    bool Read(SdfLayer *layer,
              const std::string& resolvedPath,
              bool metadataOnly) const override;

    // We override Write methods so SdfLayer::ExportToString() etc, work. 
    // Writing this layer will write out the generated layer contents.
    bool WriteToString(const SdfLayer& layer,
                       std::string* str,
                       const std::string& comment=std::string()) const override;
    bool WriteToStream(const SdfSpecHandle &spec,
                       std::ostream& out,
                       size_t indent) const override;

    // SdfFileFormat override that returns all the file paths used to generate 
    // the given layer's contents.
    std::set<std::string> GetExternalAssetDependencies(
        const SdfLayer& layer) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    SdfLayerRefPtr _GenerateDynamicLayer(const std::string& layerPath) const;

    std::set<std::string> _GetIncludedLayerPaths(
        const std::string& layerPath) const;

    virtual ~Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat();
    Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat();
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat, 
                           SdfFileFormat);
}

Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
    ::Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat()
    : SdfFileFormat(
        Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens->Id,
        Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens->Version,
        Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens->Target,
        Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormatTokens->Extension)
{
}

Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
    ::~Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat()
{
}

bool
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat::CanRead(
    const std::string& filePath) const
{
    return true;
}

std::set<std::string> 
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
    ::_GetIncludedLayerPaths(const std::string& layerPath) const
{
    // Get the layer's directory and find all valid layer file paths in the
    // directory (not recursive).
    const std::string dir = TfGetPathName(layerPath);
    std::vector<std::string> layerPaths = TfListDir(dir);
    std::set<std::string> result;
    for (const std::string &path : layerPaths) {
        // If the file is not a layer file format or is of this procedural file
        // format, we skip it.
        SdfFileFormatConstPtr fileFormat = SdfFileFormat::FindByExtension(path);
        if (fileFormat && !fileFormat.PointsTo(*this)) {
            result.insert(path);
        }
    }
    return result;
}

SdfLayerRefPtr
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
    ::_GenerateDynamicLayer(const std::string& layerPath) const
{
    // Create a new anonymous layer.
    SdfLayerRefPtr tempLayer = SdfLayer::CreateAnonymous(".usd");
    SdfChangeBlock block;

    // Get all the layer paths we're going to include.
    // For each layer create a prim spec with a reference to that layer.
    for (const std::string &path : _GetIncludedLayerPaths(layerPath)) {
        const std::string name = TfStringReplace(TfGetBaseName(path), ".", "_");
        SdfPrimSpecHandle spec = SdfPrimSpec::New(
            SdfLayerHandle(tempLayer), TfToken(name), SdfSpecifierDef);
        spec->GetReferenceList().Add(SdfReference(path, SdfPath()));
    }

    // Open the generated layer in a stage and return the flattened layer from
    // the stage. This is so that the layers we opened will just have the 
    // contents of their default prims copied into our layer without having 
    // these layers be part of composition.
    UsdStageRefPtr stage = UsdStage::Open(tempLayer, TfNullPtr);
    return stage->Flatten();
}


bool
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat::Read(
    SdfLayer *layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    if (!TF_VERIFY(layer)) {
        return false;
    }

    // Generate the layer and transfer contents. The layer file itself does 
    // not contribute to this layer.
    SdfLayerRefPtr genLayer = _GenerateDynamicLayer(resolvedPath);
    if (genLayer) {
        layer->TransferContent(genLayer);
    }
    return true;
}

bool 
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    // Write the contents as an sdf text file.
    return SdfFileFormat::FindById(
        SdfTextFileFormatTokens->Id)->WriteToString(layer, str, comment);
}

bool
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // Write the contents as an sdf text file.
    return SdfFileFormat::FindById(
        SdfTextFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}

std::set<std::string> 
Test_UsdProceduralExternalAssetsFileFormatPlugin_FileFormat
    ::GetExternalAssetDependencies(
    const SdfLayer& layer) const
{
    // The external assets that the layer depends on are all the layers in its
    // director used to generate the layer contents.
    return _GetIncludedLayerPaths(layer.GetRealPath());
}

PXR_NAMESPACE_CLOSE_SCOPE



