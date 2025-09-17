//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
