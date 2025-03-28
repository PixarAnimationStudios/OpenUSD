//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/textFileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _Tokens,

    ((Extension, "test_target_format"))
    ((A_Id, "test_target_format_A"))
    ((A_Target, "A"))
    ((B_Id, "test_target_format_B"))
    ((B_Target, "B"))
);

class TestSdfTargetFileFormatBase
    : public SdfFileFormat
{
public:
    bool CanRead(const std::string& file) const override 
    { return _sdfFormat->CanRead(file); }

    bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override 
    { return _sdfFormat->Read(layer, resolvedPath, metadataOnly); }

    bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,
        const std::string& comment,
        const FileFormatArguments& args) const override
    { return _sdfFormat->WriteToFile(layer, filePath, comment, args); }

protected:
    TestSdfTargetFileFormatBase(const TfToken& formatId, const TfToken& target)
        : SdfFileFormat(formatId, TfToken(), target, _Tokens->Extension)
        , _sdfFormat(SdfFileFormat::FindByExtension("sdf"))
    {
    }

private:
    SdfFileFormatConstPtr _sdfFormat;
};

class TestSdfTargetFileFormat_A
    : public TestSdfTargetFileFormatBase
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfTargetFileFormat_A()
        : TestSdfTargetFileFormatBase(_Tokens->A_Id, _Tokens->A_Target)
    {
    }
};

class TestSdfTargetFileFormat_B
    : public TestSdfTargetFileFormatBase
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    TestSdfTargetFileFormat_B()
        : TestSdfTargetFileFormatBase(_Tokens->B_Id, _Tokens->B_Target)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_ABSTRACT_FILE_FORMAT(TestSdfTargetFileFormatBase, SdfFileFormat);
    SDF_DEFINE_FILE_FORMAT(TestSdfTargetFileFormat_A, TestSdfTargetFileFormatBase);
    SDF_DEFINE_FILE_FORMAT(TestSdfTargetFileFormat_B, TestSdfTargetFileFormatBase);
}

PXR_NAMESPACE_CLOSE_SCOPE
