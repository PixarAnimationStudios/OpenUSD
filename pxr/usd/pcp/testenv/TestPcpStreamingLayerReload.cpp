//
// Copyright 2021 Pixar
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
