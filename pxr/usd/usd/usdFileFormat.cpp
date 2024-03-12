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
#include "pxr/usd/usd/usdFileFormat.h"

#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/usd/usdcFileFormat.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include "crateData.h"

PXR_NAMESPACE_OPEN_SCOPE


using std::string;

TF_DECLARE_WEAK_AND_REF_PTRS(Usd_CrateData);

TF_DEFINE_PUBLIC_TOKENS(UsdUsdFileFormatTokens, USD_USD_FILE_FORMAT_TOKENS);

TF_DEFINE_ENV_SETTING(
    USD_DEFAULT_FILE_FORMAT, "usdc",
    "Default file format for new .usd files; either 'usda' or 'usdc'.");

// ------------------------------------------------------------

static
SdfFileFormatConstPtr
_GetFileFormat(const TfToken& formatId)
{
    const SdfFileFormatConstPtr fileFormat = SdfFileFormat::FindById(formatId);
    TF_VERIFY(fileFormat);
    return fileFormat;
}

static
const UsdUsdcFileFormatConstPtr&
_GetUsdcFileFormat()
{
    static const auto usdcFormat = TfDynamic_cast<UsdUsdcFileFormatConstPtr>(
        _GetFileFormat(UsdUsdcFileFormatTokens->Id));
    return usdcFormat;
}

static
const UsdUsdaFileFormatConstPtr&
_GetUsdaFileFormat()
{
    static const auto usdaFormat = TfDynamic_cast<UsdUsdaFileFormatConstPtr>(
        _GetFileFormat(UsdUsdaFileFormatTokens->Id));
    return usdaFormat;
}

// A .usd file may actually be either a text .usda file or a binary crate 
// .usdc file. This function returns the appropriate file format for a
// given data object.
static
SdfFileFormatConstPtr
_GetUnderlyingFileFormat(const SdfAbstractDataConstPtr& data)
{
    // A .usd file can only be backed by one of these formats,
    // so check each one individually.
    if (TfDynamic_cast<const Usd_CrateDataConstPtr>(data)) {
        return _GetFileFormat(UsdUsdcFileFormatTokens->Id);
    }

    if (TfDynamic_cast<const SdfDataConstPtr>(data)) {
        return _GetFileFormat(UsdUsdaFileFormatTokens->Id);
    }
    
    return SdfFileFormatConstPtr();
}

// Returns the default underlying file format for a .usd file.
static
SdfFileFormatConstPtr
_GetDefaultFileFormat()
{
    TfToken defaultFormatId(TfGetEnvSetting(USD_DEFAULT_FILE_FORMAT));
    if (defaultFormatId != UsdUsdaFileFormatTokens->Id
        && defaultFormatId != UsdUsdcFileFormatTokens->Id) {
        TF_WARN("Default file format '%s' set in USD_DEFAULT_FILE_FORMAT "
                "must be either 'usda' or 'usdc'. Falling back to 'usdc'", 
                defaultFormatId.GetText());
        defaultFormatId = UsdUsdcFileFormatTokens->Id;
    }

    SdfFileFormatConstPtr defaultFormat = _GetFileFormat(defaultFormatId);
    TF_VERIFY(defaultFormat);
    return defaultFormat;
}

// Returns the 'format' argument token corresponding to the given
// file format.
static
TfToken
_GetFormatArgumentForFileFormat(const SdfFileFormatConstPtr& fileFormat)
{
    TfToken formatArg = fileFormat ? fileFormat->GetFormatId() : TfToken();
    TF_VERIFY(formatArg == UsdUsdaFileFormatTokens->Id ||
              formatArg == UsdUsdcFileFormatTokens->Id,
              "Unhandled file format '%s'",
              fileFormat ? formatArg.GetText() : "<null>");
    return formatArg;
}

// Returns the file format associated with the given arguments, or nullptr.
static
SdfFileFormatConstPtr
_GetFileFormatForArguments(const SdfFileFormat::FileFormatArguments& args)
{
    auto it = args.find(UsdUsdFileFormatTokens->FormatArg.GetString());
    if (it != args.end()) {
        const std::string& format = it->second;
        if (format == UsdUsdaFileFormatTokens->Id) {
            return _GetFileFormat(UsdUsdaFileFormatTokens->Id);
        }
        else if (format == UsdUsdcFileFormatTokens->Id) {
            return _GetFileFormat(UsdUsdcFileFormatTokens->Id);
        }
        TF_CODING_ERROR("'%s' argument was '%s', must be '%s' or '%s'. "
                        "Defaulting to '%s'.", 
                        UsdUsdFileFormatTokens->FormatArg.GetText(),
                        format.c_str(),
                        UsdUsdaFileFormatTokens->Id.GetText(),
                        UsdUsdcFileFormatTokens->Id.GetText(),
                        _GetFormatArgumentForFileFormat(
                            _GetDefaultFileFormat()).GetText());
    }
    return TfNullPtr;
}

// ------------------------------------------------------------

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdUsdFileFormat, SdfFileFormat);
}

UsdUsdFileFormat::UsdUsdFileFormat()
    : SdfFileFormat(UsdUsdFileFormatTokens->Id,
                    UsdUsdFileFormatTokens->Version,
                    UsdUsdFileFormatTokens->Target,
                    UsdUsdFileFormatTokens->Id)
{
}

UsdUsdFileFormat::~UsdUsdFileFormat()
{
}

SdfAbstractDataRefPtr
UsdUsdFileFormat::InitData(const FileFormatArguments& args) const
{
    SdfFileFormatConstPtr fileFormat = _GetFileFormatForArguments(args);
    if (!fileFormat) {
        fileFormat = _GetDefaultFileFormat();
    }
    
    return fileFormat->InitData(args);
}

SdfAbstractDataRefPtr
UsdUsdFileFormat::_InitDetachedData(const FileFormatArguments& args) const
{
    SdfFileFormatConstPtr fileFormat = _GetFileFormatForArguments(args);
    if (!fileFormat) {
        fileFormat = _GetDefaultFileFormat();
    }
    
    return fileFormat->InitDetachedData(args);
}

bool
UsdUsdFileFormat::CanRead(const string& filePath) const
{
    auto asset = ArGetResolver().OpenAsset(ArResolvedPath(filePath));
    return asset &&
        (_GetUsdcFileFormat()->_CanReadFromAsset(filePath, asset) ||
         _GetUsdaFileFormat()->_CanReadFromAsset(filePath, asset));
}

bool
UsdUsdFileFormat::Read(
    SdfLayer* layer,
    const string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    return _ReadHelper</* Detached = */ false>(
        layer, resolvedPath, metadataOnly);
}

bool
UsdUsdFileFormat::_ReadDetached(
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
UsdUsdFileFormat::_ReadHelper(
    SdfLayer* layer,
    const string& resolvedPath,
    bool metadataOnly) const
{
    // Fetch the asset from Ar.
    auto asset = ArGetResolver().OpenAsset(ArResolvedPath(resolvedPath));
    if (!asset) {
        return false;
    }

    const auto& usdcFileFormat = _GetUsdcFileFormat();
    const auto& usdaFileFormat = _GetUsdaFileFormat();

    // Network-friendly path -- just try to read the file and if we get one that
    // works we're good.
    //
    // Try binary usdc format first, since that's most common, then usda text.
    {
        TfErrorMark m;
        if (usdcFileFormat->_ReadFromAsset(
                layer, resolvedPath, asset, metadataOnly, Detached)) {
            return true;
        }
        m.Clear();

        if (usdaFileFormat->_ReadFromAsset(
                layer, resolvedPath, asset, metadataOnly)) {
            return true;
        }
        m.Clear();
    }

    // Failed to load.  Do the slower (for the network) version where we attempt
    // to determine the underlying format first, and then load using it. This
    // gives us better diagnostic messages.
    if (usdcFileFormat->_CanReadFromAsset(resolvedPath, asset)) {
        return usdcFileFormat->_ReadFromAsset(
            layer, resolvedPath, asset, metadataOnly, Detached);
    }

    if (usdaFileFormat->_CanReadFromAsset(resolvedPath, asset)) {
        return usdaFileFormat->_ReadFromAsset(
            layer, resolvedPath, asset, metadataOnly);
    }

    return false;
}

SdfFileFormatConstPtr 
UsdUsdFileFormat::_GetUnderlyingFileFormatForLayer(
    const SdfLayer& layer)
{
    auto underlyingFileFormat = _GetUnderlyingFileFormat(_GetLayerData(layer));
    return underlyingFileFormat ?
        underlyingFileFormat : _GetDefaultFileFormat();
}

/* static */
TfToken
UsdUsdFileFormat::GetUnderlyingFormatForLayer(const SdfLayer& layer)
{
    if (layer.GetFileFormat()->GetFormatId() != UsdUsdFileFormatTokens->Id)
        return TfToken();

    auto fileFormat = _GetUnderlyingFileFormatForLayer(layer);
    return _GetFormatArgumentForFileFormat(fileFormat);
}

bool
UsdUsdFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    // If a specific underlying file format is requested via the file format
    // arguments, just use that.
    SdfFileFormatConstPtr fileFormat = _GetFileFormatForArguments(args);

    // When exporting to a .usd layer (i.e., calling SdfLayer::Export), we use
    // the default underlying format for .usd. This ensures consistent behavior
    // -- creating a new .usd layer always uses the default format unless
    // otherwise specified.
    if (!fileFormat) {
        fileFormat = _GetDefaultFileFormat();
    }

    return fileFormat->WriteToFile(layer, filePath, comment, args);
}

bool
UsdUsdFileFormat::SaveToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    // If we are saving a .usd layer (i.e., calling SdfLayer::Save), we want to
    // maintain that layer's underlying format. For example, calling Save() on a
    // text .usd file should produce a text file and not convert it to binary.
    // 
    SdfFileFormatConstPtr fileFormat = _GetUnderlyingFileFormatForLayer(layer);

    if (!fileFormat) {
        fileFormat = _GetDefaultFileFormat();
    }

    return fileFormat->SaveToFile(layer, filePath, comment, args);
}

bool 
UsdUsdFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    return _GetUnderlyingFileFormatForLayer(*layer)
        ->ReadFromString(layer, str);
}

bool 
UsdUsdFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    return _GetUnderlyingFileFormatForLayer(layer)
        ->WriteToString(layer, str, comment);
}

bool
UsdUsdFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return _GetUnderlyingFileFormatForLayer(
        *get_pointer(spec->GetLayer()))->WriteToStream(
            spec, out, indent);
}

PXR_NAMESPACE_CLOSE_SCOPE

