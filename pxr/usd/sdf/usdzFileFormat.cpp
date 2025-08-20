//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/usd/sdf/usdzFileFormat.h"
#include "pxr/usd/sdf/usdzResolver.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/zipFile.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdfUsdzFileFormatTokens, SDF_USDZ_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfUsdzFileFormat, SdfFileFormat);
}

SdfUsdzFileFormat::SdfUsdzFileFormat()
    : SdfFileFormat(SdfUsdzFileFormatTokens->Id,
                    SdfUsdzFileFormatTokens->Version,
                    SdfUsdzFileFormatTokens->Target,
                    SdfUsdzFileFormatTokens->Id)
{
}

SdfUsdzFileFormat::~SdfUsdzFileFormat()
{
}

bool 
SdfUsdzFileFormat::IsPackage() const
{
    return true;
}

namespace
{

std::string
_GetFirstFileInZipFile(const std::string& zipFilePath)
{
    const SdfZipFile zipFile = Sdf_UsdzResolverCache::GetInstance()
        .FindOrOpenZipFile(zipFilePath).second;
    if (!zipFile) {
        return std::string();
    }

    const SdfZipFile::Iterator firstFileIt = zipFile.begin();
    return (firstFileIt == zipFile.end()) ? std::string() : *firstFileIt;
}

} // end anonymous namespace

std::string 
SdfUsdzFileFormat::GetPackageRootLayerPath(
    const std::string& resolvedPath) const
{
    TRACE_FUNCTION();
    return _GetFirstFileInZipFile(resolvedPath);
}

SdfAbstractDataRefPtr
SdfUsdzFileFormat::InitData(const FileFormatArguments& args) const
{
    return SdfFileFormat::InitData(args);
}

bool
SdfUsdzFileFormat::CanRead(const std::string& filePath) const
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
SdfUsdzFileFormat::Read(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    return _ReadHelper</* Detached = */ false>(
        layer, resolvedPath, metadataOnly);
}

bool
SdfUsdzFileFormat::_ReadDetached(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    return _ReadHelper</* Detached = */ true>(
        layer, resolvedPath, metadataOnly);
}

template <bool Detached>
bool
SdfUsdzFileFormat::_ReadHelper(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    // Use a scoped cache here so we only open the .usdz asset once.
    //
    // If the call to Read below calls ArResolver::OpenAsset, it will
    // ultimately ask Sdf_UsdzResolver to open the .usdz asset. The
    // scoped cache will ensure that will pick up the asset opened
    // in _GetFirstFileInZipFile instead of asking the resolver to
    // open the asset again.
    ArResolverScopedCache scopedCache;

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
    return Detached ?
        packagedFileFormat->ReadDetached(
            layer, packageRelativePath, metadataOnly) :
        packagedFileFormat->Read(
            layer, packageRelativePath, metadataOnly);
}

bool
SdfUsdzFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    TF_CODING_ERROR("Writing usdz layers is not allowed via this API.");
    return false;
}

bool 
SdfUsdzFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        ReadFromString(layer, str);
}

bool 
SdfUsdzFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        WriteToString(layer, str, comment);
}

bool
SdfUsdzFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        WriteToStream(spec, out, indent);
}

PXR_NAMESPACE_CLOSE_SCOPE

