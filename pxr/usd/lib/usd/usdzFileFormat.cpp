//
// Copyright 2018 Pixar
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
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/usd/usdzFileFormat.h"
#include "pxr/usd/usd/usdzResolver.h"
#include "pxr/usd/usd/zipFile.h"

#include "pxr/usd/sdf/layer.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdUsdzFileFormatTokens, USD_USDZ_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdUsdzFileFormat, SdfFileFormat);
}

UsdUsdzFileFormat::UsdUsdzFileFormat()
    : SdfFileFormat(UsdUsdzFileFormatTokens->Id,
                    UsdUsdzFileFormatTokens->Version,
                    UsdUsdzFileFormatTokens->Target,
                    UsdUsdzFileFormatTokens->Id)
{
}

UsdUsdzFileFormat::~UsdUsdzFileFormat()
{
}

bool 
UsdUsdzFileFormat::IsPackage() const
{
    return true;
}

namespace
{

std::string
_GetFirstFileInZipFile(const std::string& zipFilePath)
{
    const UsdZipFile zipFile = Usd_UsdzResolverCache::GetInstance()
        .FindOrOpenZipFile(zipFilePath).second;
    if (!zipFile) {
        return std::string();
    }

    const UsdZipFile::Iterator firstFileIt = zipFile.begin();
    return (firstFileIt == zipFile.end()) ? std::string() : *firstFileIt;
}

} // end anonymous namespace

std::string 
UsdUsdzFileFormat::GetPackageRootLayerPath(
    const std::string& resolvedPath) const
{
    TRACE_FUNCTION();
    return _GetFirstFileInZipFile(resolvedPath);
}

SdfAbstractDataRefPtr
UsdUsdzFileFormat::InitData(const FileFormatArguments& args) const
{
    return SdfFileFormat::InitData(args);
}

bool
UsdUsdzFileFormat::CanRead(const std::string& filePath) const
{
    TRACE_FUNCTION();

    const std::string firstFile = _GetFirstFileInZipFile(filePath);
    if (firstFile.empty()) {
        return false;
    }

    const SdfFileFormatConstPtr packagedFileFormat = 
        SdfFileFormat::FindByExtension(firstFile);
    if (!packagedFileFormat) {
        return false;
    }

    const std::string packageRelativePath = 
        ArJoinPackageRelativePath(filePath, firstFile);
    return packagedFileFormat->CanRead(packageRelativePath);
}

bool
UsdUsdzFileFormat::Read(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    const std::string firstFile = _GetFirstFileInZipFile(resolvedPath);
    if (firstFile.empty()) {
        return false;
    }

    const SdfFileFormatConstPtr packagedFileFormat = 
        SdfFileFormat::FindByExtension(firstFile);
    if (!packagedFileFormat) {
        return false;
    }

    const std::string packageRelativePath = 
        ArJoinPackageRelativePath(resolvedPath, firstFile);
    return packagedFileFormat->Read(layer, packageRelativePath, metadataOnly);
}

bool
UsdUsdzFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    TF_CODING_ERROR("Writing usdz layers is not allowed via this API.");
    return false;
}

bool 
UsdUsdzFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        ReadFromString(layer, str);
}

bool 
UsdUsdzFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToString(layer, str, comment);
}

bool
UsdUsdzFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->
        WriteToStream(spec, out, indent);
}

bool 
UsdUsdzFileFormat::_IsStreamingLayer(
    const SdfLayer& layer) const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

