//
// Copyright 2020 benmalartre
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
#include "pxr/usd/usdAnimX/fileFormat.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"

#include <iostream>
#include <fstream>

#include "pxr/usd/usdAnimX/animx.h"
#include "pxr/usd/usdAnimX/fileFormat.h"
#include "pxr/usd/usdAnimX/data.h"
#include "pxr/usd/usdAnimX/reader.h"
#include "pxr/usd/usdAnimX/writer.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdAnimXFileFormatTokens, USD_ANIMX_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdAnimXFileFormat, SdfFileFormat);
}

UsdAnimXFileFormat::UsdAnimXFileFormat()
    : SdfFileFormat(
        UsdAnimXFileFormatTokens->Id,
        UsdAnimXFileFormatTokens->Version,
        UsdAnimXFileFormatTokens->Target,
        UsdAnimXFileFormatTokens->Extension)
{
}

UsdAnimXFileFormat::~UsdAnimXFileFormat()
{
}

SdfAbstractDataRefPtr
UsdAnimXFileFormat::InitData(
    const FileFormatArguments &args) const
{
    return UsdAnimXData::New();
}

SdfAbstractDataConstPtr 
UsdAnimXFileFormat::GetData(SdfLayer *layer) const
{
    return _GetLayerData(*layer);
}

static
bool
_CanRead(const std::shared_ptr<ArAsset>& asset,
             const std::string& cookie)
{
    TfErrorMark mark;

    char aLine[512];

    size_t numToRead = std::min(sizeof(aLine), cookie.length());
    if (asset->Read(aLine, numToRead, /* offset = */ 0) != numToRead) {
        return false;
    }

    aLine[numToRead] = '\0';

    // Don't allow errors to escape this function, since this function is
    // just trying to answer whether the asset can be read.
    return !mark.Clear() && TfStringStartsWith(aLine, cookie);
}

bool 
UsdAnimXFileFormat::Read(SdfLayer* layer, const std::string& resolvedPath,
        bool metadataOnly) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(resolvedPath);
    if (!asset) {
        return false;
    }

    // Quick check to see if the file has the magic cookie before spinning up
    // the parser.
    if (!_CanRead(asset, GetFileCookie())) {
        TF_RUNTIME_ERROR("<%s> is not a valid %s layer",
                         resolvedPath.c_str(),
                         GetFormatId().GetText());
        return false;
    }

    SdfAbstractDataRefPtr data = InitData(layer->GetFileFormatArguments());
    UsdAnimXDataRefPtr animXData = TfStatic_cast<UsdAnimXDataRefPtr>(data);
    UsdAnimXReader reader;
    if(reader.Read(resolvedPath)) {
        reader.PopulateDatas(animXData);
        std::string outputFile = 
            "E:/Projects/RnD/USD_BUILD/assets/animX/output.animx";
        UsdAnimXWriter writer;
        writer.Open(outputFile);
        writer.Write(data);
        writer.Close();
    }
    _SetLayerData(layer, data);
    return true;
}

bool
UsdAnimXFileFormat::CanRead(const std::string& filePath) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(filePath);
    return asset && _CanRead(asset, GetFileCookie());
}

bool
UsdAnimXFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    // Write.
    SdfAbstractDataConstPtr data = _GetLayerData(layer);
    UsdAnimXWriter writer;
    writer.Open(filePath);
    if(TF_VERIFY(data))
        writer.Write(data);
    writer.Close();
}

bool 
UsdAnimXFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    // XXX: For now, this is not implemented
    return false;
}

bool 
UsdAnimXFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    // XXX: For now, this is not implemented
    return false;
}

bool
UsdAnimXFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    // XXX: For now, this is not implemented
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

