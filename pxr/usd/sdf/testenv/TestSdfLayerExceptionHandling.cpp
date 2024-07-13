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

#define TEST_SDF_EXCEPTION_HANDLING       \
    ((Extension, "testexception")) \
    ((RootName, "rootName"))

TF_DECLARE_PUBLIC_TOKENS(TestSdfExceptionHandling_Tokens, 
                         TEST_SDF_EXCEPTION_HANDLING);

TF_DEFINE_PUBLIC_TOKENS(
    TestSdfExceptionHandling_Tokens, 
    TEST_SDF_EXCEPTION_HANDLING);

/// Simple text file format that throws an exception when read
class TestSdfExceptionHandlingFileFormat : public SdfTextFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfExceptionHandlingFileFormat()
        :SdfTextFileFormat(TestSdfExceptionHandling_Tokens->Extension)
    {
        // Do Nothing.
    }

    /// Override of Read. This ignores the resolve path completely to create
    /// the layer from file format args.
    bool Read(SdfLayer *layer,
              const std::string &resolvedPath,
              bool metadataOnly) const override
    {
        throw std::bad_alloc();
        return true;
    }

    virtual ~TestSdfExceptionHandlingFileFormat() {};

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
    SDF_DEFINE_FILE_FORMAT(TestSdfExceptionHandlingFileFormat, 
                           SdfTextFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE
