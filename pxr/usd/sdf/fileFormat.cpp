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
///
/// \file Sdf/fileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormatRegistry.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerHints.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

static TfStaticData<Sdf_FileFormatRegistry> _FileFormatRegistry;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfFileFormat>();
}

TF_DEFINE_PUBLIC_TOKENS(SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::string& extension,
    const SdfSchemaBase& schema)
    : SdfFileFormat(
        formatId, versionString, target, std::vector<std::string>{extension}, 
        schema)
{
}

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::string& extension)
    : SdfFileFormat(
        formatId, versionString, target, std::vector<std::string>{extension}, 
        SdfSchema::GetInstance())
{
}

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::vector<std::string> &extensions)
    : SdfFileFormat(
        formatId, versionString, target, extensions, SdfSchema::GetInstance())
{
}

SdfFileFormat::SdfFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target,
    const std::vector<std::string>& extensions,
    const SdfSchemaBase& schema)
    : _schema(schema)
    , _formatId(formatId)
    , _target(target)
    , _cookie("#" + formatId.GetString())
    , _versionString(versionString)
    , _extensions(extensions)

    // If a file format is marked as primary, then it must be the
    // primary format for all of the extensions it supports. So,
    // it's sufficient to just check the first extension in the list.
    , _isPrimaryFormat(
        _FileFormatRegistry
            ->GetPrimaryFormatForExtension(extensions[0]) == formatId)
{
    // Do Nothing.
}

SdfFileFormat::~SdfFileFormat()
{
    // Do Nothing.
}

SdfFileFormat::FileFormatArguments 
SdfFileFormat::GetDefaultFileFormatArguments() const
{
    return FileFormatArguments();
}

namespace
{

SdfAbstractDataRefPtr
_CreateData(const SdfFileFormat::FileFormatArguments& args)
{
    SdfData* metadata = new SdfData;

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    metadata->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);

    return TfCreateRefPtr(metadata);
}

} // end anonymous namespace

SdfAbstractDataRefPtr
SdfFileFormat::InitData(const FileFormatArguments& args) const
{
    return _CreateData(args);
}

SdfAbstractDataRefPtr
SdfFileFormat::InitDetachedData(const FileFormatArguments& args) const
{
    SdfAbstractDataRefPtr detachedData = _InitDetachedData(args);
    if (detachedData && !detachedData->IsDetached()) {
        TF_CODING_ERROR("File format did not return detached data object.");
        return _CreateData(args);
    }
    return detachedData;
}

SdfLayerRefPtr
SdfFileFormat::NewLayer(const SdfFileFormatConstPtr &fileFormat,
                        const std::string &identifier,
                        const std::string &realPath,
                        const ArAssetInfo& assetInfo,
                        const FileFormatArguments &args) const
{
    return TfCreateRefPtr(
        _InstantiateNewLayer(
            fileFormat, identifier, realPath, assetInfo, args));
}

bool
SdfFileFormat::ShouldSkipAnonymousReload() const
{
    return _ShouldSkipAnonymousReload();
}

bool 
SdfFileFormat::ShouldReadAnonymousLayers() const
{
    return _ShouldReadAnonymousLayers();
}

const SdfSchemaBase&
SdfFileFormat::GetSchema() const
{
    return _schema;
}

const TfToken&
SdfFileFormat::GetFormatId() const
{
    return _formatId;
}

const TfToken&
SdfFileFormat::GetTarget() const
{
    return _target;
}

const std::string&
SdfFileFormat::GetFileCookie() const
{
    return _cookie;
}

const TfToken&
SdfFileFormat::GetVersionString() const
{
    return _versionString;
}

bool 
SdfFileFormat::IsPrimaryFormatForExtensions() const
{
    return _isPrimaryFormat;
}

const std::vector<std::string>&
SdfFileFormat::GetFileExtensions() const {
    return _extensions;
}

const std::string&
SdfFileFormat::GetPrimaryFileExtension() const
{
    static std::string emptyString;
    return TF_VERIFY(!_extensions.empty()) ? _extensions[0] : emptyString;
}

bool
SdfFileFormat::IsSupportedExtension(
    const std::string& extension) const
{
    std::string ext = GetFileExtension(extension);

    return ext.empty() ?
        false : std::count(_extensions.begin(), _extensions.end(), ext);
}

bool 
SdfFileFormat::IsPackage() const
{
    return false;
}

std::string 
SdfFileFormat::GetPackageRootLayerPath(
    const std::string& resolvedPath) const
{
    return std::string();
}

bool
SdfFileFormat::WriteToFile(
    const SdfLayer&,
    const std::string&,
    const std::string&,
    const FileFormatArguments&) const
{
    return false;
}

bool
SdfFileFormat::ReadDetached(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    const bool readSuccess = _ReadDetached(layer, resolvedPath, metadataOnly);
    if (readSuccess && !_GetLayerData(*layer)->IsDetached()) {
        TF_CODING_ERROR(
            "File format did not return detached layer when reading layer %s.", 
            layer->GetIdentifier() == resolvedPath || resolvedPath.empty() ? 
            TfStringPrintf("@%s@", layer->GetIdentifier().c_str()).c_str() :
            TfStringPrintf("@%s@ (%s)",
                layer->GetIdentifier().c_str(), resolvedPath.c_str()).c_str());
        return false;
    }
    return readSuccess;
}

SdfAbstractDataRefPtr
SdfFileFormat::_InitDetachedData(
    const FileFormatArguments& args) const
{
    return _CreateData(args);
}

namespace
{

class _WarnedFormatTracker
{
public:
    bool NeedToWarn(const SdfFileFormat& format) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _formatIds.insert(format.GetFormatId()).second;
    };

private:
    mutable std::set<TfToken> _formatIds;
    mutable std::mutex _mutex;
};

} // end anonymous namespace

static TfStaticData<_WarnedFormatTracker> _WarnedFormats;

bool
SdfFileFormat::_ReadDetached(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly) const
{
    bool didCopyData = false;
    if (!_ReadAndCopyLayerDataToMemory(
            layer, resolvedPath, metadataOnly, &didCopyData)) {
        return false;
    }

    if (didCopyData && _WarnedFormats->NeedToWarn(*this)) {
        TF_WARN(
            "File format plugin '%s' did not produce a detached layer when "
            "requested. Layer data has been copied to produce a detached "
            "layer, which may impact performance. The file format should be "
            "updated to avoid this issue. This was first encountered when"
            "reading %s",
            GetFormatId().GetText(),
            layer->GetIdentifier() == resolvedPath || resolvedPath.empty() ? 
            TfStringPrintf("@%s@", layer->GetIdentifier().c_str()).c_str() :
            TfStringPrintf("@%s@ (%s)",
                layer->GetIdentifier().c_str(), resolvedPath.c_str()).c_str());
    }

    return true;
}

bool
SdfFileFormat::_ReadAndCopyLayerDataToMemory(
    SdfLayer* layer,
    const std::string& resolvedPath,
    bool metadataOnly,
    bool* didCopyData) const
{
    if (!Read(layer, resolvedPath, metadataOnly)) {
        return false;
    }

    SdfAbstractDataConstPtr layerData = _GetLayerData(*layer);
    if (layerData && !layerData->IsDetached()) {
        SdfAbstractDataRefPtr detachedData = TfCreateRefPtr(new SdfData);
        detachedData->CopyFrom(layerData);
        _SetLayerData(layer, detachedData);

        if (didCopyData) {
            *didCopyData = true;
        }
    }
    else {
        if (didCopyData) {
            *didCopyData = false;
        }
    }

    return true;
}

bool 
SdfFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    return false;
}

bool
SdfFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return false;
}

bool 
SdfFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    return false;
}

std::set<std::string> 
SdfFileFormat::GetExternalAssetDependencies(
    const SdfLayer& layer) const
{
    return std::set<std::string>();
}

// XXX:
// fileFormatRegistry.cpp: file format creation does not provide a
// straightforward way to pass parsed capabilities to format constructor.
// As a result, instance methods defer this check to the registry itself.
bool SdfFileFormat::SupportsReading() const
{
    return _FileFormatRegistry->FormatSupportsReading(
        GetPrimaryFileExtension(), GetTarget());
}

bool SdfFileFormat::SupportsWriting() const
{
    return _FileFormatRegistry->FormatSupportsWriting(
        GetPrimaryFileExtension(), GetTarget());
}

bool SdfFileFormat::SupportsEditing() const
{
    return _FileFormatRegistry->FormatSupportsEditing(
        GetPrimaryFileExtension(), GetTarget());
}

/* static */
std::string
SdfFileFormat::GetFileExtension(
    const std::string& s)
{
    if (s.empty()) {
        return s;
    }

    const std::string extension = Sdf_GetExtension(s);
    return extension.empty() ? s : extension;
}

/* static */
std::set<std::string>
SdfFileFormat::FindAllFileFormatExtensions()
{
    return _FileFormatRegistry->FindAllFileFormatExtensions();
}

/* static */
std::set<std::string>
SdfFileFormat::FindAllDerivedFileFormatExtensions(const TfType& baseType)
{
    return _FileFormatRegistry->FindAllDerivedFileFormatExtensions(baseType);
}

/* static */
bool SdfFileFormat::FormatSupportsReading(
    const std::string& extension,
    const std::string& target)
{
    return _FileFormatRegistry->FormatSupportsReading(extension, target);
}

/* static */
bool SdfFileFormat::FormatSupportsWriting(
    const std::string& extension,
    const std::string& target)
{
    return _FileFormatRegistry->FormatSupportsWriting(extension, target);
}

/* static */
bool SdfFileFormat::FormatSupportsEditing(
    const std::string& extension,
    const std::string& target)
{
    return _FileFormatRegistry->FormatSupportsEditing(extension, target);
}


/* static */
SdfFileFormatConstPtr
SdfFileFormat::FindById(
    const TfToken& formatId)
{
    return _FileFormatRegistry->FindById(formatId);
}

/* static */
SdfFileFormatConstPtr
SdfFileFormat::FindByExtension(
    const std::string& extension,
    const std::string& target)
{
    return _FileFormatRegistry->FindByExtension(extension, target);
}

/* static */
SdfFileFormatConstPtr
SdfFileFormat::FindByExtension(
    const std::string &path,
    const FileFormatArguments &args)
{
    // Find a file format that can handle this extension and the
    // specified target (if any).
    const std::string* targets = 
        TfMapLookupPtr(args, SdfFileFormatTokens->TargetArg);
    if (targets) {
        for (std::string& target : TfStringTokenize(*targets, ",")) {
            target = TfStringTrim(target);
            if (target.empty()) {
                continue;
            }

            if (const SdfFileFormatConstPtr format = 
                SdfFileFormat::FindByExtension(path, target)) {
                return format;
            }
        }
        return TfNullPtr;
    }

    return SdfFileFormat::FindByExtension(path);
}

bool
SdfFileFormat::_ShouldSkipAnonymousReload() const
{
    return true;
}

bool 
SdfFileFormat::_ShouldReadAnonymousLayers() const
{
    return false;
}

void
SdfFileFormat::_SetLayerData(
    SdfLayer* layer,
    SdfAbstractDataRefPtr& data)
{
    _SetLayerData(layer, data, SdfLayerHints{});
}

void
SdfFileFormat::_SetLayerData(
    SdfLayer* layer,
    SdfAbstractDataRefPtr& data,
    SdfLayerHints hints)
{
    // If layer initialization has not completed, then this
    // is being loaded as a new layer; otherwise we are loading
    // data into an existing layer.
    //
    // Note that this is an optional::bool and we are checking if it has
    // been set, not what its held value is.
    //
    const bool layerIsLoadingAsNew = !layer->_initializationWasSuccessful;
    if (layerIsLoadingAsNew) {
        layer->_SwapData(data);
    }
    else {
        // If we're reading into an existing layer (e.g. due to a Reload), we
        // want the layer to use whatever data implementation the file format
        // wants to set because that object may have special behaviors specific
        // to the format. If we detect the layer is currently using a different
        // implementation, call _AdoptData to have the layer take ownership of
        // the new data object and emit coarse invalidation. However, if we
        // detect the layer is already using the same implementation, we call
        // _SetData, which may perform finer-grained copies and change
        // notification.
        //
        // We consider data implementations to differ if the object types don't
        // match or if one streams data and the other doesn't, or if one is
        // detached and the other isn't. In the latter cases, we want the layer
        // to have the qualities the file format dictates, even if the
        // underlying data object type is the same.
        const SdfAbstractDataConstPtr oldData = _GetLayerData(*layer);
        const bool differentDataImpl = 
            data->StreamsData() != oldData->StreamsData() ||
            data->IsDetached() != oldData->IsDetached() ||
            !TfSafeTypeCompare(
                typeid(*get_pointer(data)), typeid(*get_pointer(oldData)));

        if (differentDataImpl) {
            layer->_AdoptData(data);
        }
        else {
            layer->_SetData(data);
        }
    }

    layer->_hints = hints;
}

SdfAbstractDataConstPtr
SdfFileFormat::_GetLayerData(const SdfLayer& layer)
{
    return layer._GetData();
}

/* virtual */
SdfLayer*
SdfFileFormat::_InstantiateNewLayer(
    const SdfFileFormatConstPtr &fileFormat,
    const std::string &identifier,
    const std::string &realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments &args) const
{
    return new SdfLayer(fileFormat, identifier, realPath, assetInfo, args);
}

// ------------------------------------------------------------

Sdf_FileFormatFactoryBase::~Sdf_FileFormatFactoryBase() = default;

PXR_NAMESPACE_CLOSE_SCOPE
