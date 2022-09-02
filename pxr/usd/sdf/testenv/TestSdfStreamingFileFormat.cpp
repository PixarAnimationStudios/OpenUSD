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

#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormat.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class TestSdfStreamingData final
    : public SdfData
{
public:
    static SdfAbstractDataRefPtr New()
    {
        SdfAbstractDataRefPtr data = TfCreateRefPtr(new TestSdfStreamingData);
        data->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
        return data;
    }

    bool StreamsData() const override
    {
        return true;
    };

    bool IsDetached() const override
    {
        return false;
    }

private:
    TestSdfStreamingData()
    {
        CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    }
};

TF_DEFINE_PRIVATE_TOKENS(
    _Tokens,

    ((Extension, "test_streaming_format"))
    ((Id, "test_streaming_format"))
);

class TestSdfStreamingFileFormat
    : public SdfFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    SdfAbstractDataRefPtr
    InitData(const FileFormatArguments& args) const override
    {
        return TestSdfStreamingData::New();
    }

    bool CanRead(const std::string& file) const override
    { return true; }
    
    bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override
    {
        SdfAbstractDataRefPtr streamingData = TestSdfStreamingData::New();
        _SetLayerData(layer, streamingData);
        return true;
    }

    bool WriteToFile(
        const SdfLayer& layer,
        const std::string& filePath,   
        const std::string& comment,
        const FileFormatArguments& args) const override
    {
        return static_cast<bool>(ArGetResolver().OpenAssetForWrite(
            ArResolvedPath(filePath), ArResolver::WriteMode::Replace));
    }

protected:
    SdfAbstractDataRefPtr
    _InitDetachedData(const FileFormatArguments& args) const override
    {
        return SdfFileFormat::InitData(args);
    }

    bool
    _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override
    {
        return _ReadAndCopyLayerDataToMemory(layer, resolvedPath, metadataOnly);
    }

private:
    TestSdfStreamingFileFormat()
        : SdfFileFormat(_Tokens->Id, TfToken(), TfToken(), _Tokens->Extension)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestSdfStreamingFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE
