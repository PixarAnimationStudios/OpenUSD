//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/usdcFileFormat.h"

#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/usdFileFormat.h"
#include "pxr/usd/sdf/usdaFileFormat.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/registryManager.h"

#include "crateData.h"

#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE


using std::string;

TF_DECLARE_WEAK_AND_REF_PTRS(Sdf_CrateData);

TF_DEFINE_PUBLIC_TOKENS(SdfUsdcFileFormatTokens, SDF_USDC_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfUsdcFileFormat, SdfFileFormat);
}

SdfUsdcFileFormat::SdfUsdcFileFormat()
    : SdfFileFormat(SdfUsdcFileFormatTokens->Id,
                    Sdf_CrateData::GetSoftwareVersionToken(),
                    SdfUsdFileFormatTokens->Target,
                    SdfUsdcFileFormatTokens->Id)
{
}

SdfUsdcFileFormat::~SdfUsdcFileFormat()
{
}

SdfAbstractDataRefPtr
SdfUsdcFileFormat::InitData(const FileFormatArguments& args) const
{
    auto newData = new Sdf_CrateData(/* detached = */ false);

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    newData->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(newData);
}

SdfAbstractDataRefPtr
SdfUsdcFileFormat::_InitDetachedData(const FileFormatArguments& args) const
{
    auto newData = new Sdf_CrateData(/* detached = */ true);

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    newData->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);
    return TfCreateRefPtr(newData);
}

bool
SdfUsdcFileFormat::CanRead(const string& filePath) const
{
    return Sdf_CrateData::CanRead(filePath);
}

bool
SdfUsdcFileFormat::_CanReadFromAsset(const string& filePath,
                                     const std::shared_ptr<ArAsset>& asset) const
{
    return Sdf_CrateData::CanRead(filePath, asset);
}

bool
SdfUsdcFileFormat::Read(SdfLayer* layer,
                        const string& resolvedPath,
                        bool metadataOnly) const
{
    TRACE_FUNCTION();
    return _ReadHelper(layer, resolvedPath, metadataOnly,
                       /* detached = */ false);
}

bool
SdfUsdcFileFormat::_ReadDetached(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();
    return _ReadHelper(layer, resolvedPath, metadataOnly, 
                       /* detached = */ true);
}

bool
SdfUsdcFileFormat::_ReadFromAsset(SdfLayer* layer,
                                  const string& resolvedPath,
                                  const std::shared_ptr<ArAsset>& asset,
                                  bool metadataOnly,
                                  bool detached) const
{
    TRACE_FUNCTION();
    return _ReadHelper(layer, resolvedPath, metadataOnly, asset, detached);
}

template <class ...Args>
bool
SdfUsdcFileFormat::_ReadHelper(
    SdfLayer* layer, 
    const std::string& resolvedPath,
    bool metadataOnly,
    Args&&... args) const
{
    SdfAbstractDataRefPtr data = InitData(layer->GetFileFormatArguments());
    auto crateData = TfDynamic_cast<Sdf_CrateDataRefPtr>(data);

    if (!crateData || 
        !crateData->Open(resolvedPath, std::forward<Args>(args)...)) {
        return false;
    }

    _SetLayerData(layer, data);
    return true;
}

bool
SdfUsdcFileFormat::WriteToFile(const SdfLayer& layer,
                               const std::string& filePath,
                               const std::string& comment,
                               const FileFormatArguments& args) const
{
    SdfAbstractDataConstPtr dataSource = _GetLayerData(layer);

    // XXX: WBN to avoid const-cast -- saving can't be non-mutating in general.
    if (auto const *constCrateData =
        dynamic_cast<Sdf_CrateData const *>(get_pointer(dataSource))) {
        auto *crateData = const_cast<Sdf_CrateData *>(constCrateData);
        return crateData->Export(filePath);
    }

    // Otherwise we're dealing with some arbitrary data object, just copy the
    // contents into the binary data.
    if (auto dataDest = 
        TfDynamic_cast<Sdf_CrateDataRefPtr>(InitData(FileFormatArguments()))) {
        dataDest->CopyFrom(dataSource);
        return dataDest->Export(filePath);
    }
    return false;
}

bool
SdfUsdcFileFormat::SaveToFile(const SdfLayer& layer,
                              const std::string& filePath,
                              const std::string& comment,
                              const FileFormatArguments& args) const
{
    SdfAbstractDataConstPtr dataSource = _GetLayerData(layer);

    // XXX: WBN to avoid const-cast -- saving can't be non-mutating in general.
    if (auto const *constCrateData =
        dynamic_cast<Sdf_CrateData const *>(get_pointer(dataSource))) {
        auto *crateData = const_cast<Sdf_CrateData *>(constCrateData);
        return crateData->Save(filePath);
    }

    TF_CODING_ERROR("Called SdfUsdcFileFormat::SaveToFile with "
                    "non-Crate-backed layer @%s@",
                    layer.GetIdentifier().c_str());
    return false;
}

bool 
SdfUsdcFileFormat::ReadFromString(SdfLayer* layer,
                                  const std::string& str) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        ReadFromString(layer, str);
}

bool 
SdfUsdcFileFormat::WriteToString(const SdfLayer& layer,
                                 std::string* str,
                                 const std::string& comment) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        WriteToString(layer, str, comment);
}

bool 
SdfUsdcFileFormat::WriteToStream(const SdfSpecHandle &spec,
                                 std::ostream& out,
                                 size_t indent) const
{
    return SdfFileFormat::FindById(SdfUsdaFileFormatTokens->Id)->
        WriteToStream(spec, out, indent);
}

PXR_NAMESPACE_CLOSE_SCOPE
