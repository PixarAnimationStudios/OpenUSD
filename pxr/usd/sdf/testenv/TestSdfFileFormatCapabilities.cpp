//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/usdaFileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

#define TEST_SDF_UNWRITABLE_FILE_FORMAT_TOKENS       \
    ((Extension, "unwritable")) \
    ((VersionStr, "0.0.0")) \
    ((Target, "")) 

TF_DECLARE_PUBLIC_TOKENS(TestSdfUnwritableFormat_Tokens, 
                         TEST_SDF_UNWRITABLE_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    TestSdfUnwritableFormat_Tokens, 
    TEST_SDF_UNWRITABLE_FILE_FORMAT_TOKENS);

class TestSdfUnwritableFormat : public SdfUsdaFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfUnwritableFormat()
        :SdfUsdaFileFormat(
            TestSdfUnwritableFormat_Tokens->Extension,
            TestSdfUnwritableFormat_Tokens->VersionStr,
            TestSdfUnwritableFormat_Tokens->Target)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestSdfUnwritableFormat, 
                           SdfUsdaFileFormat);
}

#define TEST_SDF_UNREADABLE_FILE_FORMAT_TOKENS       \
    ((Extension, "unreadable")) \
    ((VersionStr, "0.0.0")) \
    ((Target, "test")) 

TF_DECLARE_PUBLIC_TOKENS(TestSdfUnreadableFormat_Tokens, 
                         TEST_SDF_UNREADABLE_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    TestSdfUnreadableFormat_Tokens, 
    TEST_SDF_UNREADABLE_FILE_FORMAT_TOKENS);

class TestSdfUnreadableFormat : public SdfUsdaFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfUnreadableFormat()
        :SdfUsdaFileFormat(
            TestSdfUnreadableFormat_Tokens->Extension,
            TestSdfUnreadableFormat_Tokens->VersionStr,
            TestSdfUnreadableFormat_Tokens->Target)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestSdfUnreadableFormat, 
                           SdfUsdaFileFormat);
}


#define TEST_SDF_UNEDITABLE_FILE_FORMAT_TOKENS       \
    ((Extension, "uneditable")) \
    ((VersionStr, "0.0.0")) \
    ((Target, "test")) 

TF_DECLARE_PUBLIC_TOKENS(TestSdfUneditableFormat_Tokens, 
                         TEST_SDF_UNEDITABLE_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    TestSdfUneditableFormat_Tokens, 
    TEST_SDF_UNEDITABLE_FILE_FORMAT_TOKENS);

class TestSdfUneditableFormat : public SdfUsdaFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfUneditableFormat()
        :SdfUsdaFileFormat(
            TestSdfUneditableFormat_Tokens->Extension,
            TestSdfUneditableFormat_Tokens->VersionStr,
            TestSdfUneditableFormat_Tokens->Target)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestSdfUneditableFormat, 
                           SdfUsdaFileFormat);
}


PXR_NAMESPACE_CLOSE_SCOPE



