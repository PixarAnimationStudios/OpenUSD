//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/textFileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

#define TEST_SDF_NO_ASSET_FILE_FORMAT_TOKENS       \
    ((Extension, "testsdfnoasset")) \
    ((RootName, "rootName"))

TF_DECLARE_PUBLIC_TOKENS(TestSdfNoAssetFileFormat_Tokens, 
                         TEST_SDF_NO_ASSET_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    TestSdfNoAssetFileFormat_Tokens, 
    TEST_SDF_NO_ASSET_FILE_FORMAT_TOKENS);

/// Simple text file format that does not read any assets and instead
/// creates a layer with a single root prim spec whose name may be 
/// specified in the file format arguments
class TestSdfNoAssetFileFormat : public SdfTextFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfNoAssetFileFormat()
        :SdfTextFileFormat(TestSdfNoAssetFileFormat_Tokens->Extension)
    {
        // Do Nothing.
    }

    /// Override of Read. This ignores the resolve path completely to create
    /// the layer from file format args.
    bool Read(SdfLayer *layer,
              const std::string &resolvedPath,
              bool metadataOnly) const override
    {
        const FileFormatArguments &args = layer->GetFileFormatArguments();
        SdfAbstractDataRefPtr data = InitData(args);

        // Use the "rootName" arg to create a root prim spec with that name 
        // directly through the abstract data.
        if (const std::string *rootName = TfMapLookupPtr(
                args, TestSdfNoAssetFileFormat_Tokens->RootName)) {
            TfToken rootToken(*rootName);
            SdfPath rootPath = 
                SdfPath::AbsoluteRootPath().AppendChild(rootToken);

            data->CreateSpec(rootPath, SdfSpecTypePrim);
            data->Set(SdfPath::AbsoluteRootPath(), 
                      SdfChildrenKeys->PrimChildren, 
                      VtValue(TfTokenVector({rootToken})));
        }

        _SetLayerData(layer, data);
        return true;
    }

    virtual ~TestSdfNoAssetFileFormat() {};

protected:
    // Override to allow reading of anonymous layers since Read doesn't 
    // need an asset. This allows FindOrOpen and Reload to populate
    // anonymous layers with the dynamic layer content.
    bool _ShouldReadAnonymousLayers() const override
    {
        return true;
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestSdfNoAssetFileFormat, 
                           SdfTextFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE



