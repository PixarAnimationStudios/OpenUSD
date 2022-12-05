//
// Copyright 2022 Pixar
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
