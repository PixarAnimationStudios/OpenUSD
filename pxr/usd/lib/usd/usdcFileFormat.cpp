//
// Copyright 2016 Pixar
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
#include "pxr/usd/usd/usdcFileFormat.h"

#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/usdaFileFormat.h"

#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tracelite/trace.h"

#include "pxr/base/tf/registryManager.h"

#include "crateData.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


using std::string;

TF_DECLARE_WEAK_AND_REF_PTRS(Usd_CrateData);

TF_DEFINE_PUBLIC_TOKENS(UsdUsdcFileFormatTokens, USD_USDC_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdUsdcFileFormat, SdfFileFormat);
}

UsdUsdcFileFormat::UsdUsdcFileFormat()
    : SdfFileFormat(UsdUsdcFileFormatTokens->Id,
                    Usd_CrateData::GetSoftwareVersionToken(),
                    UsdUsdFileFormatTokens->Target,
                    UsdUsdcFileFormatTokens->Id)
{
}

UsdUsdcFileFormat::~UsdUsdcFileFormat()
{
}

SdfAbstractDataRefPtr
UsdUsdcFileFormat::InitData(const FileFormatArguments& args) const
{
    auto newData = new Usd_CrateData();

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    newData->CreateSpec(SdfAbstractDataSpecId(&SdfPath::AbsoluteRootPath()),
                        SdfSpecTypePseudoRoot);

    return TfCreateRefPtr(newData);
}

bool
UsdUsdcFileFormat::CanRead(const string& filePath) const
{
    return Usd_CrateData::CanRead(filePath);
}

bool
UsdUsdcFileFormat::Read(const SdfLayerBasePtr& layerBase,
                        const string& resolvedPath,
                        bool metadataOnly) const
{
    TRACE_FUNCTION();

    auto layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (!TF_VERIFY(layer))
        return false;

    SdfAbstractDataRefPtr data = InitData(layerBase->GetFileFormatArguments());
    auto crateData = TfDynamic_cast<Usd_CrateDataRefPtr>(data);

    if (!crateData || !crateData->Open(resolvedPath))
        return false;

    _SetLayerData(layer, data);
    return true;
}

bool
UsdUsdcFileFormat::WriteToFile(const SdfLayerBase* layerBase,
                               const std::string& filePath,
                               const std::string& comment,
                               const FileFormatArguments& args) const
{
    auto layer = dynamic_cast<const SdfLayer*>(layerBase);

    if (!TF_VERIFY(layer))
        return false;

    SdfAbstractDataConstPtr dataSource =
        _GetLayerData(SdfCreateNonConstHandle(layer));

    // XXX: WBN to avoid const-cast -- saving can't be non-mutating in general.
    if (auto const *constCrateData =
        dynamic_cast<Usd_CrateData const *>(get_pointer(dataSource))) {
        auto *crateData = const_cast<Usd_CrateData *>(constCrateData);
        return crateData->Save(filePath);
    }

    // Otherwise we're dealing with some arbitrary data object, just copy the
    // contents into the binary data.
    if (auto dataDest = 
        TfDynamic_cast<Usd_CrateDataRefPtr>(InitData(FileFormatArguments()))) {
        dataDest->CopyFrom(dataSource);
        return dataDest->Save(filePath);
    }
    return false;
}

bool 
UsdUsdcFileFormat::ReadFromString(const SdfLayerBasePtr& layerBase,
                                  const std::string& str) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        ReadFromString(layerBase, str);
}

bool 
UsdUsdcFileFormat::WriteToString(const SdfLayerBase* layerBase,
                                 std::string* str,
                                 const std::string& comment) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToString(layerBase, str, comment);
}

bool 
UsdUsdcFileFormat::WriteToStream(const SdfSpecHandle &spec,
                                 std::ostream& out,
                                 size_t indent) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToStream(spec, out, indent);
}

bool 
UsdUsdcFileFormat::_IsStreamingLayer(const SdfLayerBase& layer) const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

