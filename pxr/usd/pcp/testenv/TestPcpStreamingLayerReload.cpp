//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/textFileFormat.h"


PXR_NAMESPACE_OPEN_SCOPE

// Our SdfData subclass overrides StreamsData() to return true for testing.
class Test_PcpStreamingData : public SdfData
{
    virtual ~Test_PcpStreamingData();
    virtual bool StreamsData() const {
        return true;
    }
};

Test_PcpStreamingData::~Test_PcpStreamingData()
{
}

#define TEST_PCP_STREAMING_LAYER_RELOAD_TOKENS       \
    ((Id, "testpcpstreaminglayerreload"))            \
    ((Version, "1.0"))                               \
    ((Target, "usd"))

TF_DECLARE_PUBLIC_TOKENS(Test_PcpStreamingLayerReload_FileFormatTokens,
                         TEST_PCP_STREAMING_LAYER_RELOAD_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(
    Test_PcpStreamingLayerReload_FileFormatTokens,
    TEST_PCP_STREAMING_LAYER_RELOAD_TOKENS);

/// \class Test_PcpStreamingLayerReload_FileFormat
///
class Test_PcpStreamingLayerReload_FileFormat : public SdfTextFileFormat
{
private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;
    Test_PcpStreamingLayerReload_FileFormat();
    virtual ~Test_PcpStreamingLayerReload_FileFormat();

    virtual SdfAbstractDataRefPtr
    InitData(const FileFormatArguments &args) const;

};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(
        Test_PcpStreamingLayerReload_FileFormat, SdfTextFileFormat);
}

Test_PcpStreamingLayerReload_FileFormat
::Test_PcpStreamingLayerReload_FileFormat()
    : SdfTextFileFormat(
        Test_PcpStreamingLayerReload_FileFormatTokens->Id,
        Test_PcpStreamingLayerReload_FileFormatTokens->Version,
        Test_PcpStreamingLayerReload_FileFormatTokens->Target)
{
}

Test_PcpStreamingLayerReload_FileFormat
::~Test_PcpStreamingLayerReload_FileFormat()
{
}

SdfAbstractDataRefPtr
Test_PcpStreamingLayerReload_FileFormat
::InitData(const FileFormatArguments &args) const {
    SdfData *metadata = new Test_PcpStreamingData;
    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    metadata->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(metadata);
}   

PXR_NAMESPACE_CLOSE_SCOPE
