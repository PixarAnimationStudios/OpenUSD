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
/// \file Sdf/layer.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/assetPathResolver.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/changeManager.h"
#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/childrenUtils.h"
#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layerRegistry.h"
#include "pxr/usd/sdf/layerStateDelegate.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/specType.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/subLayerListEditor.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stackTrace.h"

#include <tbb/queuing_rw_mutex.h>

#include <atomic>
#include <functional>
#include <fstream>
#include <set>
#include <vector>

using std::map;
using std::set;
using std::string;
using std::vector;

namespace ph = std::placeholders;

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< SdfLayer, TfType::Bases<SdfLayerBase> >();
}

// Muted Layers stores the paths of layers that should be muted.  The stored
// paths should be asset paths, when applicable, or identifiers if no asset
// path exists for the desired layers.
typedef set<string> _MutedLayers;
typedef std::map<std::string, SdfAbstractDataRefPtr> _MutedLayerDataMap;
static TfStaticData<_MutedLayers> _mutedLayers;
static TfStaticData<_MutedLayerDataMap> _mutedLayerData;
// Global mutex protecting _mutedLayers and _mutedLayerData
static TfStaticData<std::mutex> _mutedLayersMutex;
// This is a global revision number that tracks changes to _mutedLayers.  Since
// we seldom mute and unmute layers, this lets layers cache their muteness and
// do quick validity checks without taking a lock and looking themselves up in
// _mutedLayers.
static std::atomic_size_t _mutedLayersRevision { 1 };


// A registry for loaded layers.
static TfStaticData<Sdf_LayerRegistry> _layerRegistry;

// Global mutex protecting _layerRegistry.
static tbb::queuing_rw_mutex &
_GetLayerRegistryMutex() {
    static tbb::queuing_rw_mutex mutex;
    return mutex;
}

SdfLayer::SdfLayer(
    const SdfFileFormatConstPtr &fileFormat,
    const string &identifier,
    const string &realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments &args) :
    SdfLayerBase(fileFormat, args),
    _idRegistry(SdfLayerHandle(this)),
    _data(fileFormat->InitData(args)),
    _stateDelegate(SdfSimpleLayerStateDelegate::New()),
    _lastDirtyState(false),
    _assetInfo(new Sdf_AssetInfo),
    _mutedLayersRevisionCache(0),
    _isMutedCache(false),
    _permissionToEdit(true),
    _permissionToSave(true)
{
    const string realPathFinal =
        TfIsRelativePath(realPath) ? realPath : TfAbsPath(realPath);

    TF_DEBUG(SDF_LAYER).Msg("SdfLayer::SdfLayer('%s', '%s')\n",
        identifier.c_str(), realPathFinal.c_str());

    // If the identifier has the anonymous layer identifier prefix, it is a
    // template into which the layer address must be inserted. This ensures
    // that anonymous layers have unique identifiers, and can be referenced by
    // Sd object reprs.
    string layerIdentifier = Sdf_IsAnonLayerIdentifier(identifier) ?
        Sdf_ComputeAnonLayerIdentifier(identifier, this) : identifier;

    // Lock the initialization mutex before we publish this object
    // (i.e. add it to the registry in _InitializeFromIdentifier).
    // This ensures that other threads looking for this layer will
    // block until it is fully initialized.
    _initializationMutex.lock();

    // Initialize layer asset information.
    _InitializeFromIdentifier(
        layerIdentifier, realPathFinal, std::string(), assetInfo);

    // A new layer is not dirty.
    _MarkCurrentStateAsClean();
}

// CODE_COVERAGE_OFF
SdfLayer::~SdfLayer()
{
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::~SdfLayer('%s')\n", GetIdentifier().c_str());

    if (IsMuted()) {
        std::string mutedPath = _GetMutedPath();
        SdfAbstractDataRefPtr mutedData;
        {
            std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
            // Drop any in-memory edits we may have been holding for this layer.
            // To minimize time holding the lock, swap the data out and
            // erase the entry, then release the lock before proceeding
            // to drop the refcount.
            _MutedLayerDataMap::iterator i = _mutedLayerData->find(mutedPath);
            if (i != _mutedLayerData->end()) {
                std::swap(mutedData, i->second);
                _mutedLayerData->erase(i);
            }
        }
    }

    tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

    // Note that FindOrOpen may have already removed this layer from
    // the registry, so we count on this API not emitting errors in that
    // case.
    _layerRegistry->Erase(SdfCreateHandle(this));
}
// CODE_COVERAGE_ON

// ---
// SdfLayer static functions and data
// ---

SdfLayerRefPtr
SdfLayer::_CreateNewWithFormat(
    const SdfFileFormatConstPtr &fileFormat,
    const string& identifier,
    const string& realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments& args)
{
    const string realPathFinal =
        TfIsRelativePath(realPath) ? realPath : TfAbsPath(realPath);

    // This method should be called with the layerRegistryMutex already held.

    // Create and return a new layer with _initializationMutex locked.
    return fileFormat->NewLayer<SdfLayer>(
        fileFormat, identifier, realPathFinal, assetInfo, args);
}

void
SdfLayer::_FinishInitialization(bool success)
{
    _initializationWasSuccessful = success;
    _initializationMutex.unlock();
}

bool
SdfLayer::_WaitForInitializationAndCheckIfSuccessful()
{
    // Note: the caller is responsible for holding a reference to this
    // layer, to keep it from being destroyed out from under us while
    // blocked on the mutex.

    // Drop the GIL in case we might have it -- if the layer load happening in
    // another thread needs the GIL, we'd deadlock here.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Try to acquire and then release _initializationMutex.  If the
    // layer is still being initialized, this will be locked, blocking
    // progress until initialization completes and the mutex unlocks.
    _initializationMutex.lock();
    _initializationMutex.unlock();

    // For various reasons, initialization may have failed.
    // For example, the menva parser may have hit a syntax error,
    // or transferring content from a source layer may have failed.
    // In this case _initializationWasSuccessful will be set to false.
    // The callers of this method are responsible for checking the result
    // and dropping any references they hold.  As a convenience to them,
    // we return the value here.
    return _initializationWasSuccessful.get();
}

SdfLayerRefPtr
SdfLayer::CreateAnonymous(const string& tag)
{
    // XXX: 
    // It would be nice to use the _GetFileFormatForPath helper function 
    // from below, but that function expects a layer identifier and the 
    // tag is supposed to be just a helpful debugging aid; the fact that
    // one can specify an underlying layer file format by specifying an
    // extension was unintended.
    SdfFileFormatConstPtr fileFormat;
    const string suffix = TfStringGetSuffix(tag);
    if (!suffix.empty()) {
        fileFormat = SdfFileFormat::FindById(TfToken(suffix));
    }

    return CreateAnonymous(tag, fileFormat);
}

SdfLayerRefPtr
SdfLayer::CreateAnonymous(
    const string &tag, const SdfFileFormatConstPtr &format)
{
    SdfFileFormatConstPtr fmt = format;
    
    if (!fmt) {
        fmt = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
    }

    if (!fmt) {
        TF_CODING_ERROR("Cannot determine file format for anonymous SdfLayer");
        return SdfLayerRefPtr();
    }

    return _CreateAnonymousWithFormat(fmt, tag);
}

SdfLayerRefPtr
SdfLayer::_CreateAnonymousWithFormat(
    const SdfFileFormatConstPtr &fileFormat, const std::string& tag)
{
    if (fileFormat->IsPackage()) {
        TF_CODING_ERROR("Cannot create anonymous layer: creating package %s "
                        "layer is not allowed through this API.",
                        fileFormat->GetFormatId().GetText());
        return SdfLayerRefPtr();
    }

    tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

    SdfLayerRefPtr layer =
        _CreateNewWithFormat(
            fileFormat, Sdf_GetAnonLayerIdentifierTemplate(tag), string());

    // No layer initialization required, so initialization is complete.
    layer->_FinishInitialization(/* success = */ true);

    return layer;
}

bool
SdfLayer::IsAnonymous() const
{
    return Sdf_IsAnonLayerIdentifier(GetIdentifier());
}

bool
SdfLayer::IsAnonymousLayerIdentifier(const string& identifier)
{
    return Sdf_IsAnonLayerIdentifier(identifier);
}

string
SdfLayer::GetDisplayNameFromIdentifier(const string& identifier)
{
    return Sdf_GetLayerDisplayName(identifier);
}

SdfLayerRefPtr
SdfLayer::CreateNew(
    const string& identifier,
    const string& realPath,
    const FileFormatArguments &args)
{
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::CreateNew('%s', '%s', '%s')\n",
        identifier.c_str(), realPath.c_str(), TfStringify(args).c_str());

    return _CreateNew(TfNullPtr, identifier, realPath, ArAssetInfo(), args);
}

SdfLayerRefPtr
SdfLayer::CreateNew(
    const SdfFileFormatConstPtr& fileFormat,
    const string& identifier,
    const string& realPath,
    const FileFormatArguments &args)
{
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::CreateNew('%s', '%s', '%s', '%s')\n",
        fileFormat->GetFormatId().GetText(), 
        identifier.c_str(), realPath.c_str(), TfStringify(args).c_str());

    return _CreateNew(fileFormat, identifier, realPath, ArAssetInfo(), args);
}

static SdfFileFormatConstPtr
_GetFileFormatForPath(const std::string &filePath,
                      const SdfLayer::FileFormatArguments &args)
{
    // Determine which file extension to use.
    const string ext = Sdf_GetExtension(filePath);
    if (ext.empty()) {
        return TfNullPtr;
    }

    // Find a file format that can handle this extension and the
    // specified target (if any).
    const std::string* target = 
        TfMapLookupPtr(args, SdfFileFormatTokens->TargetArg);

    return SdfFileFormat::FindByExtension(
        ext, (target ? *target : std::string()));
}

SdfLayerRefPtr
SdfLayer::_CreateNew(
    SdfFileFormatConstPtr fileFormat,
    const string& identifier,
    const string& realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments &args)
{
    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        TF_CODING_ERROR("Cannot create a new layer with anonymous "
            "layer identifier '%s'.", identifier.c_str());
        return TfNullPtr;
    }

    string whyNot;
    if (!Sdf_CanCreateNewLayerWithIdentifier(identifier, &whyNot)) {
        TF_CODING_ERROR("Cannot create new layer '%s': %s",
            identifier.c_str(),
            whyNot.c_str());
        return TfNullPtr;
    }

    ArResolver& resolver = ArGetResolver();

    // When creating a new layer, assume that relative identifiers are
    // relative to the current working directory.
    const bool isRelativePath = resolver.IsRelativePath(identifier);
    const string absIdentifier = 
        isRelativePath ? TfAbsPath(identifier) : identifier;

    // Direct newly created layers to a local path.
    const string localPath = realPath.empty() ? 
        resolver.ComputeLocalPath(absIdentifier) : realPath;
    if (localPath.empty()) {
        TF_CODING_ERROR(
            "Failed to compute local path for new layer with "
            "identifier '%s'", absIdentifier.c_str());
        return TfNullPtr;
    }

    // If not explicitly supplied one, try to determine the fileFormat 
    // based on the local path suffix,
    if (!fileFormat) {
        fileFormat = _GetFileFormatForPath(localPath, args);
        // XXX: This should be a coding error, not a failed verify.
        if (!TF_VERIFY(fileFormat))
            return TfNullPtr;
    }

    // Restrict creating package layers via the Sdf API. These layers
    // are expected to be created via other libraries or external programs.
    if (Sdf_IsPackageOrPackagedLayer(fileFormat, identifier)) {
        TF_CODING_ERROR("Cannot create new layer '%s': creating %s %s "
                        "layer is not allowed through this API.",
                        identifier.c_str(), 
                        fileFormat->IsPackage() ? "package" : "packaged",
                        fileFormat->GetFormatId().GetText());
        return TfNullPtr;
    }

    // In case of failure below, we want to release the layer
    // registry mutex lock before destroying the layer.
    SdfLayerRefPtr layer;
    {
        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

        // Check for existing layer with this identifier.
        if (_layerRegistry->Find(absIdentifier)) {
            TF_CODING_ERROR("A layer already exists with identifier '%s'",
                absIdentifier.c_str());
            return TfNullPtr;
        }

        layer = _CreateNewWithFormat(
            fileFormat, absIdentifier, localPath, assetInfo, args);

        // XXX 2011-08-19 Newly created layers should not be
        // saved to disk automatically.
        //
        // Force the save here to ensure this new layer overwrites any
        // existing layer on disk.
        if (!TF_VERIFY(layer) || !layer->_Save(/* force = */ true)) {
            // Dropping the layer reference will destroy it, and
            // the destructor will remove it from the registry.
            return TfNullPtr;
        }

        // Once we have saved the layer, initialization is complete.
        layer->_FinishInitialization(/* success = */ true);
    }
    // Return loaded layer or special-cased in-memory layer.
    return layer;
}

// Creates a new empty layer with the given identifier for a given file
// format class. This is so that Python File Format classes can create
// layers (CreateNew(); doesn't work, because it already saves during
// construction of the layer. That is something specific (python generated)
// layer types may choose to not do.)

SdfLayerRefPtr
SdfLayer::New(
    const SdfFileFormatConstPtr& fileFormat,
    const string& identifier,
    const string& realPath,
    const FileFormatArguments& args)
{
    // No layer identifier or realPath policies can be applied at this point.
    // This method is called by the file format implementation to create new
    // layer objects. Policy is applied in CreateNew.

    if (!fileFormat) {
        TF_CODING_ERROR("Invalid file format");
        return TfNullPtr;
    }

    if (identifier.empty()) {
        TF_CODING_ERROR("Cannot construct a layer with an empty identifier.");
        return TfNullPtr;
    }

    if (Sdf_IsPackageOrPackagedLayer(fileFormat, identifier)) {
        TF_CODING_ERROR("Cannot construct new %s %s layer", 
                        fileFormat->IsPackage() ? "package" : "packaged",
                        fileFormat->GetFormatId().GetText());
        return TfNullPtr;
    }

    tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

    // When creating a new layer, assume that relative identifiers are
    // relative to the current working directory.
    const string absIdentifier = ArGetResolver().IsRelativePath(identifier) ?
        TfAbsPath(identifier) : identifier;

    SdfLayerRefPtr layer = _CreateNewWithFormat(
        fileFormat, absIdentifier, realPath, ArAssetInfo(), args);

    // No further initialization required.
    layer->_FinishInitialization(/* success = */ true);

    return layer;
}

/* static */
string
SdfLayer::ComputeRealPath(const string &layerPath)
{
    return Sdf_ComputeFilePath(layerPath);
}

static SdfLayer::FileFormatArguments&
_CanonicalizeFileFormatArguments(const std::string& filePath,
                                 const SdfFileFormatConstPtr& fileFormat,
                                 SdfLayer::FileFormatArguments& args)
{

    // Nothing to do if there isn't an associated file format.
    // This is expected by _ComputeInfoToFindOrOpenLayer and isn't an error.
    if (!fileFormat) {
        // XXX:
        // Sdf is unable to determine a file format for layers that are created 
        // without a file extension (which includes anonymous layers). The keys 
        // for these layers in the registry will never include a 'target' 
        // argument -- the API doesn't give you a way to do that. 
        // 
        // So, if a 'target' is specified here, we want to strip it out 
        // so Find and FindOrOpen will search the registry and find these
        // layers. If we didn't, we would search the registry for an 
        // identifier with the 'target' arg embedded, and we'd never find
        // it.
        //
        // This is a hack. I think the right thing is to either:
        //   a) Ensure that a layer's identifier always encodes its file format
        //   b) Do this target argument stripping in Find / FindOrOpen, find
        //      the layer, then verify that the layer's target is the one that
        //      was specified.
        //
        // These are larger changes that require updating some clients, so
        // I don't want to do this yet.
        if (Sdf_GetExtension(filePath).empty()) {
            args.erase(SdfFileFormatTokens->TargetArg);
        }
        return args;
    }

    // If the file format plugin being used to open the indicated layer
    // is the primary plugin for layers of that type, it means the 'target'
    // argument (if any) had no effect and can be stripped from the arguments.
    if (fileFormat->IsPrimaryFormatForExtensions()) {
        args.erase(SdfFileFormatTokens->TargetArg);
    }

    // If there aren't any more args to canonicalize, we can exit early.
    if (args.empty()) {
        return args;
    }

    // Strip out any arguments that match the file format's published
    // default arguments. A layer opened without any arguments should
    // be considered equivalent to a layer opened with only default
    // arguments specified.
    const SdfLayer::FileFormatArguments defaultArgs = 
        fileFormat->GetDefaultFileFormatArguments();
    TF_FOR_ALL(it, defaultArgs) {
        SdfLayer::FileFormatArguments::iterator argIt = args.find(it->first);
        if (argIt != args.end() && argIt->second == it->second) {
            args.erase(argIt);
        }
    }

    return args;
}

struct SdfLayer::_FindOrOpenLayerInfo
{
    // File format plugin for the layer. This may be NULL if
    // the file format could not be identified.
    SdfFileFormatConstPtr fileFormat;

    // Canonical file format arguments.
    SdfLayer::FileFormatArguments fileFormatArgs;

    // Whether this layer is anonymous.
    bool isAnonymous = false;

    // Path to the layer.
    string layerPath;

    // Resolved path for the layer. If the layer is an anonymous layer,
    // this will be the same as layerPath.
    string resolvedLayerPath;

    // Identifier for the layer, combining both the layer path and
    // file format arguments.
    string identifier;

    // Asset info from resolving the layer path.
    ArAssetInfo assetInfo;
};

bool
SdfLayer::_ComputeInfoToFindOrOpenLayer(
    const string& identifier,
    const SdfLayer::FileFormatArguments& args,
    _FindOrOpenLayerInfo* info,
    bool computeAssetInfo)
{
    TRACE_FUNCTION();

    if (identifier.empty()) {
        return false;
    }

    string layerPath;
    SdfLayer::FileFormatArguments layerArgs;
    if (!Sdf_SplitIdentifier(identifier, &layerPath, &layerArgs) ||
        layerPath.empty()) {
        return false;
    }

    bool isAnonymous = IsAnonymousLayerIdentifier(layerPath);

    // If we're trying to open an anonymous layer, do not try to compute the
    // real path for it.
    ArAssetInfo assetInfo;
    string resolvedLayerPath = isAnonymous ? layerPath :
        Sdf_ResolvePath(layerPath, computeAssetInfo ? &assetInfo : nullptr);

    // Merge explicitly-specified arguments over any arguments
    // embedded in the given identifier.
    if (layerArgs.empty()) {
        layerArgs = args;
    }
    else {
        TF_FOR_ALL(it, args) {
            layerArgs[it->first] = it->second;
        }
    }

    info->fileFormat = _GetFileFormatForPath(
        resolvedLayerPath.empty() ? layerPath : resolvedLayerPath, layerArgs);
    info->fileFormatArgs.swap(_CanonicalizeFileFormatArguments(
        layerPath, info->fileFormat, layerArgs));

    info->isAnonymous = isAnonymous;
    info->layerPath.swap(layerPath);
    info->resolvedLayerPath.swap(resolvedLayerPath);
    info->identifier = Sdf_CreateIdentifier(
        info->layerPath, info->fileFormatArgs);
    swap(info->assetInfo, assetInfo);
    return true;
}

template <class ScopedLock>
SdfLayerRefPtr
SdfLayer::_TryToFindLayer(const string &identifier,
                          const string &resolvedPath,
                          ScopedLock &lock,
                          bool retryAsWriter)
{
    SdfLayerRefPtr result;
    bool hasWriteLock = false;

  retry:
    if (SdfLayerHandle layer = _layerRegistry->Find(identifier, resolvedPath)) {
        // We found a layer in the registry -- try to acquire a TfRefPtr to this
        // layer.  Since we have the lock, we guarantee that the layer's
        // TfRefBase will not be destroyed until we unlock.
        result = TfCreateRefPtrFromProtectedWeakPtr(layer);
        if (result) {
            // We got an ownership stake in the layer, release the lock and
            // return it.
            lock.release();
            return result;
        }

        // We found a layer but we could not get an ownership stake in it -- it
        // is expiring.  Upgrade the lock to a write lock since we will have to
        // try to remove this expiring layer from the registry.  If our upgrade
        // is non-atomic, we must retry the steps above, since everything
        // might've changed in the meantime.
        if (!hasWriteLock && !lock.upgrade_to_writer()) {
            // We have the write lock, but we released it in the interim, so
            // repeat our steps above now that we have the write lock.
            hasWriteLock = true;
            goto retry;
        }

        if (layer) {
            // Layer is expiring and we have the write lock: erase it from the
            // registry.
            _layerRegistry->Erase(layer);  
        }
    } else if (!hasWriteLock && retryAsWriter && !lock.upgrade_to_writer()) {
        // Retry the find since we released the lock in upgrade_to_writer().
        hasWriteLock = true;
        goto retry;
    }
    
    if (!retryAsWriter)
        lock.release();
    
    return result;
}

/* static */
SdfLayerRefPtr
SdfLayer::FindOrOpen(const string &identifier,
                     const FileFormatArguments &args)
{
    TRACE_FUNCTION();
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::FindOrOpen('%s', '%s')\n",
        identifier.c_str(), TfStringify(args).c_str());

    // Drop the GIL, since if we hold it and another thread that has the
    // _layerRegistryMutex needs it (if its opening code invokes python, for
    // instance), we'll deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _FindOrOpenLayerInfo layerInfo;
    if (!_ComputeInfoToFindOrOpenLayer(identifier, args, &layerInfo,
                                       /* computeAssetInfo = */ true)) {
        return TfNullPtr;
    }

    // First see if this layer is already present.
    tbb::queuing_rw_mutex::scoped_lock
        lock(_GetLayerRegistryMutex(), /*write=*/false);
    if (SdfLayerRefPtr layer =
        _TryToFindLayer(layerInfo.identifier, layerInfo.resolvedLayerPath,
                        lock, /*retryAsWriter=*/true)) {
        // This could be written as a ternary, but we rely on return values 
        // being implicitly moved to avoid making an unnecessary copy of 
        // layer and the associated ref-count bump.
        if (layer->_WaitForInitializationAndCheckIfSuccessful()) {
            return layer;
        }
        return TfNullPtr;
    }
    // At this point _TryToFindLayer has upgraded lock to a writer.

    // Some layers, such as anonymous layers, have identifiers but don't have
    // resolved paths.  They aren't backed by assets on disk.  If we don't find
    // such a layer by identifier in the registry, we're done since we don't
    // have an asset to open.
    if (layerInfo.resolvedLayerPath.empty()) {
        return TfNullPtr;
    }

    // Otherwise we create the layer and insert it into the registry.
    return _OpenLayerAndUnlockRegistry(lock, layerInfo,
                                       /* metadataOnly */ false);
}

/* static */
SdfLayerRefPtr
SdfLayer::OpenAsAnonymous(
    const std::string &layerPath,
    bool metadataOnly,
    const std::string &tag)
{
    _FindOrOpenLayerInfo layerInfo;
    if (!_ComputeInfoToFindOrOpenLayer(layerPath, FileFormatArguments(), 
                                       &layerInfo)) {
        return TfNullPtr;
    }

    // XXX: Is this really a coding error? SdfLayer avoids issuing errors if
    //      given a non-existent file, for instance. Should we be following the
    //      same policy here?
    if (!layerInfo.fileFormat) {
        TF_CODING_ERROR("Cannot determine file format for @%s@", 
                        layerInfo.identifier.c_str());
        return TfNullPtr;
    }

    // Create a new anonymous layer.
    SdfLayerRefPtr layer;
    {
        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());
        layer = _CreateNewWithFormat(
                layerInfo.fileFormat, Sdf_GetAnonLayerIdentifierTemplate(tag),
                string());
        // From this point, we must call _FinishInitialization() on
        // either success or failure in order to unblock other
        // threads waiting for initialization to finish.
    }

    // Run the file parser to read in the file contents.
    if (!layer->_Read(layerInfo.identifier, layerInfo.resolvedLayerPath, 
                      metadataOnly)) {
        layer->_FinishInitialization(/* success = */ false);
        return TfNullPtr;
    }

    layer->_MarkCurrentStateAsClean();
    layer->_FinishInitialization(/* success = */ true);
    return layer;
}

const SdfSchemaBase& 
SdfLayer::GetSchema() const
{
    return SdfSchema::GetInstance();
}

SdfLayer::_ReloadResult
SdfLayer::_Reload(bool force)
{
    TRACE_FUNCTION();

    string identifier = GetIdentifier();
    if (identifier.empty()) {
        TF_CODING_ERROR("Can't reload a layer with no identifier");
        return _ReloadFailed;
    }

    SdfChangeBlock block;

    if (IsAnonymous() && GetFileFormat()->ShouldSkipAnonymousReload()) {
        // Different file formats have different policies for reloading
        // anonymous layers.  Some want to treat it as a noop, others want to
        // treat it as 'Clear'.
        //
        // XXX: in the future, I think we want FileFormat plugins to
        // have a Reload function.  The plugin can manage when it needs to
        // reload data appropriately.
        return _ReloadSkipped;
    }
    else if (IsMuted() || IsAnonymous()) {
        // Reloading a muted layer leaves it with the initialized contents.
        SdfAbstractDataRefPtr initialData = 
            GetFileFormat()->InitData(GetFileFormatArguments());
        if (_data->Equals(initialData)) {
            return _ReloadSkipped;
        }
        _SetData(initialData);
    } else {
        // The physical location of the file may have changed since
        // the last load, so re-resolve the identifier.
        string oldRealPath = GetRealPath();
        UpdateAssetInfo();
        string realPath = GetRealPath();

        // If path resolution in UpdateAssetInfo failed, we may end
        // up with an empty real path, and cannot reload the layer.
        if (realPath.empty()) {
            TF_RUNTIME_ERROR(
                "Cannot determine real path for '%s', skipping reload.",
                identifier.c_str());
            return _ReloadFailed;
        }

        // If this layer's modification timestamp is empty, this is a
        // new layer that has never been serialized. This could happen 
        // if a layer were created with SdfLayer::New, for instance. 
        // In such cases we can skip the reload since there's nowhere
        // to reload data from.
        //
        // This ensures we don't ask for the modification timestamp for
        // unserialized new layers below, which would result in errors.
        //
        // XXX 2014-09-02 Reset layer to initial data?
        if (_assetModificationTime.IsEmpty()) {
            return _ReloadSkipped;
        }

        // Get the layer's modification timestamp.
        VtValue timestamp = ArGetResolver().GetModificationTimestamp(
            GetIdentifier(), realPath);
        if (timestamp.IsEmpty()) {
            TF_CODING_ERROR(
                "Unable to get modification time for '%s (%s)'",
                GetIdentifier().c_str(), realPath.c_str());
            return _ReloadFailed;
        }

        // See if we can skip reloading.
        if (!force && !IsDirty()
            && (realPath == oldRealPath)
            && (timestamp == _assetModificationTime)) {
            return _ReloadSkipped;
        }

        if (!_Read(GetIdentifier(), realPath, /* metadataOnly = */ false)) {
            return _ReloadFailed;
        }

        _assetModificationTime.Swap(timestamp);

        if (realPath != oldRealPath) {
            Sdf_ChangeManager::Get().DidChangeLayerResolvedPath(
                SdfLayerHandle(this));
        }
    }

    _MarkCurrentStateAsClean();

    Sdf_ChangeManager::Get().DidReloadLayerContent(SdfLayerHandle(this));

    return _ReloadSucceeded;
}

bool
SdfLayer::Reload(bool force)
{
    return _Reload(force) == _ReloadSucceeded;
}

bool
SdfLayer::ReloadLayers(
    const set<SdfLayerHandle>& layers,
    bool force)
{
    TF_DESCRIBE_SCOPE("Reloading %zu layer(s)", layers.size());

    // Block re-composition until we've finished reloading layers.
    SdfChangeBlock block;
    bool status = true;
    TF_FOR_ALL(layer, layers) {
        if (*layer) {
            if ((*layer)->_Reload(force) == _ReloadFailed) {
                status = false;
                TF_WARN("Unable to re-read @%s@",
                        (*layer)->GetIdentifier().c_str());
            }
        }
    }

    return status;
}

bool 
SdfLayer::Import(const string &layerPath)
{
    string filePath = Sdf_ComputeFilePath(layerPath);
    if (filePath.empty())
        return false;

    return _Read(layerPath, filePath, /* metadataOnly = */ false);
}

bool
SdfLayer::ImportFromString(const std::string &s)
{
    return GetFileFormat()->ReadFromString(SdfLayerBasePtr(this), s);
}

bool
SdfLayer::_Read(
    const string& identifier,
    const string& resolvedPath,
    bool metadataOnly)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("SdfLayer::_Read");
    TF_DESCRIBE_SCOPE("Loading layer '%s'", resolvedPath.c_str());
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::_Read('%s', '%s', metadataOnly=%s)\n",
        identifier.c_str(), resolvedPath.c_str(),
        TfStringify(metadataOnly).c_str());

    SdfFileFormatConstPtr format = GetFileFormat();
    if (format->LayersAreFileBased()) {
        if (!ArGetResolver().FetchToLocalResolvedPath(
                identifier, resolvedPath)) {
            TF_DEBUG(SDF_LAYER).Msg(
                "SdfLayer::_Read - unable to fetch '%s' to "
                "local path '%s'\n",
                identifier.c_str(), resolvedPath.c_str());
            return false;
        }

        TF_DEBUG(SDF_LAYER).Msg(
            "SdfLayer::_Read - fetched '%s' to local path '%s'\n",
            identifier.c_str(), resolvedPath.c_str());
    }

    return format->Read(SdfLayerBasePtr(this), resolvedPath, metadataOnly);
}

/*static*/
SdfLayerHandle
SdfLayer::Find(const string &identifier,
               const FileFormatArguments &args)
{
    TRACE_FUNCTION();

    // We don't need to drop the GIL here, since _TryToFindLayer() doesn't
    // invoke any plugin code, and if we do wind up calling
    // _WaitForInitializationAndCheckIfSuccessful() then we'll drop the GIL in
    // there.

    _FindOrOpenLayerInfo layerInfo;
    if (!_ComputeInfoToFindOrOpenLayer(identifier, args, &layerInfo)) {
        return TfNullPtr;
    }

    // First see if this layer is already present.
    tbb::queuing_rw_mutex::scoped_lock
        lock(_GetLayerRegistryMutex(), /*write=*/false);
    if (SdfLayerRefPtr layer = _TryToFindLayer(
            layerInfo.identifier, layerInfo.resolvedLayerPath,
            lock, /*retryAsWriter=*/false)) {
        return layer->_WaitForInitializationAndCheckIfSuccessful() ?
            layer : TfNullPtr;
    }
    return TfNullPtr;
}

/* static */
SdfLayerHandle
SdfLayer::FindRelativeToLayer(
    const SdfLayerHandle &anchor,
    const string &layerPath,
    const FileFormatArguments &args)
{
    TRACE_FUNCTION();

    if (!anchor) {
        TF_CODING_ERROR("Anchor layer is invalid");
        return TfNullPtr;
    }

    return Find(anchor->ComputeAbsolutePath(layerPath), args);
}

std::set<double>
SdfLayer::ListAllTimeSamples() const
{
    return _data->ListAllTimeSamples();
}

std::set<double> 
SdfLayer::ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _data->ListTimeSamplesForPath(id);
}

bool 
SdfLayer::GetBracketingTimeSamples(double time, double* tLower, double* tUpper)
{
    return _data->GetBracketingTimeSamples(time, tLower, tUpper);
}

size_t 
SdfLayer::GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _data->GetNumTimeSamplesForPath(id);
}

bool 
SdfLayer::GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId& id, 
                                          double time,
                                          double* tLower, double* tUpper)
{
    return _data->GetBracketingTimeSamplesForPath(id, time, tLower, tUpper);
}

bool 
SdfLayer::QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                          VtValue *value) const
{
    return _data->QueryTimeSample(id, time, value);
}

bool 
SdfLayer::QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                          SdfAbstractDataValue *value) const
{
    return _data->QueryTimeSample(id, time, value);
}

static TfType
_GetExpectedTimeSampleValueType(
    const SdfLayer& layer, const SdfAbstractDataSpecId& id)
{
    const SdfSpecType specType = layer.GetSpecType(id);
    if (specType == SdfSpecTypeUnknown) {
        TF_CODING_ERROR("Cannot set time sample at <%s> since spec does "
                        "not exist", id.GetString().c_str());
        return TfType();
    }
    else if (specType != SdfSpecTypeAttribute &&
             specType != SdfSpecTypeRelationship) {
        TF_CODING_ERROR("Cannot set time sample at <%s> because spec "
                        "is not an attribute or relationship",
                        id.GetString().c_str());
        return TfType();
    }

    TfType valueType;
    TfToken valueTypeName;
    if (specType == SdfSpecTypeRelationship) {
        static const TfType pathType = TfType::Find<SdfPath>();
        valueType = pathType;
    }
    else if (layer.HasField(id, SdfFieldKeys->TypeName, &valueTypeName)) {
        valueType = layer.GetSchema().FindType(valueTypeName).GetType();
    }

    if (!valueType) {
        TF_CODING_ERROR("Cannot determine value type for <%s>",
                        id.GetString().c_str());
    }
    
    return valueType;
}

void 
SdfLayer::SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                        const VtValue & value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    // circumvent type checking if setting a block.
    if (value.IsHolding<SdfValueBlock>()) {
        _PrimSetTimeSample(id, time, value);
        return;
    }

    const TfType expectedType = _GetExpectedTimeSampleValueType(*this, id);
    if (!expectedType) {
        // Error already emitted, just bail.
        return;
    }
    
    if (value.GetType() == expectedType) {
        _PrimSetTimeSample(id, time, value);
    }
    else {
        const VtValue castValue = 
            VtValue::CastToTypeid(value, expectedType.GetTypeid());
        if (castValue.IsEmpty()) {
            TF_CODING_ERROR("Can't set time sample on <%s> to %s: "
                            "expected a value of type \"%s\"",
                            id.GetString().c_str(),
                            TfStringify(value).c_str(),
                            expectedType.GetTypeName().c_str());
            return;
        }

        _PrimSetTimeSample(id, time, castValue);
    }
}

// cache the value of typeid(SdfValueBlock)
namespace 
{
    const TfType& _GetSdfValueBlockType() 
    {
        static const TfType blockType = TfType::Find<SdfValueBlock>();
        return blockType;
    }
}


void 
SdfLayer::SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                        const SdfAbstractDataConstValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    if (value.valueType == _GetSdfValueBlockType().GetTypeid()) {
        _PrimSetTimeSample(id, time, value);
        return;
    }

    const TfType expectedType = _GetExpectedTimeSampleValueType(*this, id);
    if (!expectedType) {
        // Error already emitted, just bail.
        return;
    }

    if (TfSafeTypeCompare(value.valueType, expectedType.GetTypeid())) {
        _PrimSetTimeSample(id, time, value);
    }
    else {
        VtValue tmpValue;
        value.GetValue(&tmpValue);

        const VtValue castValue = 
            VtValue::CastToTypeid(tmpValue, expectedType.GetTypeid());
        if (castValue.IsEmpty()) {
            TF_CODING_ERROR("Can't set time sample on <%s> to %s: "
                            "expected a value of type \"%s\"",
                            id.GetString().c_str(),
                            TfStringify(tmpValue).c_str(),
                            expectedType.GetTypeName().c_str());
            return;
        }

        _PrimSetTimeSample(id, time, castValue);
    }
}

void 
SdfLayer::EraseTimeSample(const SdfAbstractDataSpecId& id, double time)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }
    if (!HasSpec(id)) {
        TF_CODING_ERROR("Cannot SetTimeSample at <%s> since spec does "
                        "not exist", id.GetString().c_str());
        return;
    }

    if (!QueryTimeSample(id, time)) {
        // No time sample to remove.
        return;
    }

    _PrimSetTimeSample(id, time, VtValue());
}

static 
const VtValue& _GetVtValue(const VtValue& v)
{ return v; }

static 
VtValue _GetVtValue(const SdfAbstractDataConstValue& v)
{
    VtValue value;
    TF_VERIFY(v.GetValue(&value));
    return value;
}

template <class T>
void 
SdfLayer::_PrimSetTimeSample(const SdfAbstractDataSpecId& id, double time,
                             const T& value,
                             bool useDelegate)
{
    SdfChangeBlock block;

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetTimeSample(id, time, value);
        return;
    }

    // TODO(USD):optimization: Analyze the affected time interval.
    Sdf_ChangeManager::Get()
        .DidChangeAttributeTimeSamples(SdfLayerHandle(this), 
                                       id.GetFullSpecPath());

    // XXX: Should modify SetTimeSample API to take an
    //      SdfAbstractDataConstValue instead of (or along with) VtValue.
    const VtValue& valueToSet = _GetVtValue(value);
    _data->SetTimeSample(id, time, valueToSet);
}

template void SdfLayer::_PrimSetTimeSample(
    const SdfAbstractDataSpecId&, double, 
    const VtValue&, bool);
template void SdfLayer::_PrimSetTimeSample(
    const SdfAbstractDataSpecId&, double, 
    const SdfAbstractDataConstValue&, bool);

// ---
// End of SdfLayer static functions
// ---

void
SdfLayer::_InitializeFromIdentifier(
    const string& identifier,
    const string& realPath,
    const string& fileVersion,
    const ArAssetInfo& assetInfo)
{
    TRACE_FUNCTION();

    SdfLayerHandle self(this);

    // Compute layer asset information from the identifier.
    boost::scoped_ptr<Sdf_AssetInfo> newInfo(
        Sdf_ComputeAssetInfoFromIdentifier(identifier, realPath, assetInfo,
            fileVersion));
    if (!newInfo)
        return;

    // If the newly computed asset info is identical to the existing asset
    // info, there is no need to update registries or send notices.
    if (*newInfo == *_assetInfo)
        return;

    // Swap the layer asset info with the newly computed information. This
    // must occur prior to updating the layer registry, as the new layer
    // information is used to recompute registry indices.
    string oldIdentifier = _assetInfo->identifier;
    string oldRealPath = _assetInfo->realPath;
    _assetInfo.swap(newInfo);

    // Update layer state delegate.
    if (TF_VERIFY(_stateDelegate)) {
        _stateDelegate->_SetLayer(self);
    }

    // Update the layer registry before sending notices.
    _layerRegistry->InsertOrUpdate(self);

    // Only send a notice if the identifier has changed (this notice causes
    // mass invalidation. See http://bug/33217). If the old identifier was
    // empty, this is a newly constructed layer, so don't send the notice.
    if (!oldIdentifier.empty()) {
        SdfChangeBlock block;
        if (oldIdentifier != GetIdentifier()) {
            Sdf_ChangeManager::Get().DidChangeLayerIdentifier(
                self, oldIdentifier);
        }
        if (oldRealPath != GetRealPath()) {
            Sdf_ChangeManager::Get().DidChangeLayerResolvedPath(self);
        }
    }
}

template <class T>
inline
void
SdfLayer::_SetValue(const TfToken& key, T value)
{
    SetField(SdfPath::AbsoluteRootPath(), key, VtValue(value));
}

template <class T>
inline
T
SdfLayer::_GetValue(const TfToken& key) const
{
    VtValue value;
    if (!HasField(SdfPath::AbsoluteRootPath(), key, &value)) {
        return GetSchema().GetFallback(key).Get<T>();
    }
    
    return value.Get<T>();
}

SdfAssetPath 
SdfLayer::GetColorConfiguration() const
{
    return _GetValue<SdfAssetPath>(SdfFieldKeys->ColorConfiguration);
}

void 
SdfLayer::SetColorConfiguration(const SdfAssetPath &colorConfiguration)
{
    _SetValue(SdfFieldKeys->ColorConfiguration, colorConfiguration);
}

bool 
SdfLayer::HasColorConfiguration() const
{
    return HasField(SdfPath::AbsoluteRootPath(), 
                    SdfFieldKeys->ColorConfiguration);
}
    
void 
SdfLayer::ClearColorConfiguration()
{
    EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->ColorConfiguration);
}

TfToken
SdfLayer::GetColorManagementSystem() const
{
    return _GetValue<TfToken>(SdfFieldKeys->ColorManagementSystem);
}

void 
SdfLayer::SetColorManagementSystem(const TfToken &cms)
{
    _SetValue(SdfFieldKeys->ColorManagementSystem, cms);
}

bool 
SdfLayer::HasColorManagementSystem() const
{
    return HasField(SdfPath::AbsoluteRootPath(), 
                    SdfFieldKeys->ColorManagementSystem);    
}

void 
SdfLayer::ClearColorManagementSystem() 
{
    EraseField(SdfPath::AbsoluteRootPath(), 
               SdfFieldKeys->ColorManagementSystem);
}

void
SdfLayer::SetComment(const string &newVal)
{
    _SetValue(SdfFieldKeys->Comment, newVal);
}

string
SdfLayer::GetComment() const
{
    return _GetValue<string>(SdfFieldKeys->Comment);
}

void
SdfLayer::SetDefaultPrim(const TfToken &name)
{
    _SetValue(SdfFieldKeys->DefaultPrim, name);
}

TfToken
SdfLayer::GetDefaultPrim() const
{
    return _GetValue<TfToken>(SdfFieldKeys->DefaultPrim);
}

void
SdfLayer::ClearDefaultPrim()
{
    EraseField(SdfPath::AbsoluteRootPath(),
               SdfFieldKeys->DefaultPrim);
}

bool
SdfLayer::HasDefaultPrim()
{
    return HasField(SdfPath::AbsoluteRootPath(),
                    SdfFieldKeys->DefaultPrim);
}

void
SdfLayer::SetDocumentation(const string &newVal)
{
    _SetValue(SdfFieldKeys->Documentation, newVal);
}

string
SdfLayer::GetDocumentation() const
{
    return _GetValue<string>(SdfFieldKeys->Documentation);
}

void 
SdfLayer::SetStartTimeCode( double newVal )
{
    _SetValue(SdfFieldKeys->StartTimeCode, newVal);
}

double
SdfLayer::GetStartTimeCode() const
{
    return _GetValue<double>(SdfFieldKeys->StartTimeCode);
}

bool
SdfLayer::HasStartTimeCode() const
{
    return HasField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->StartTimeCode);
}

void
SdfLayer::ClearStartTimeCode()
{
    EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->StartTimeCode);
}

void 
SdfLayer::SetEndTimeCode( double newVal )
{
    _SetValue(SdfFieldKeys->EndTimeCode, newVal);
}

double
SdfLayer::GetEndTimeCode() const
{
    return _GetValue<double>(SdfFieldKeys->EndTimeCode);
}

bool
SdfLayer::HasEndTimeCode() const
{
    return HasField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->EndTimeCode);
}

void
SdfLayer::ClearEndTimeCode()
{
    EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->EndTimeCode);
}

void 
SdfLayer::SetTimeCodesPerSecond( double newVal )
{
    _SetValue(SdfFieldKeys->TimeCodesPerSecond, newVal);
}

double
SdfLayer::GetTimeCodesPerSecond() const
{
    return _GetValue<double>(SdfFieldKeys->TimeCodesPerSecond);
}

bool
SdfLayer::HasTimeCodesPerSecond() const
{
    return HasField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->TimeCodesPerSecond);
}

void
SdfLayer::ClearTimeCodesPerSecond()
{
    return EraseField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->TimeCodesPerSecond);
}

void 
SdfLayer::SetFramesPerSecond( double newVal )
{
    _SetValue(SdfFieldKeys->FramesPerSecond, newVal);
}

double
SdfLayer::GetFramesPerSecond() const
{
    return _GetValue<double>(SdfFieldKeys->FramesPerSecond);
}

bool
SdfLayer::HasFramesPerSecond() const
{
    return HasField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->FramesPerSecond);
}

void
SdfLayer::ClearFramesPerSecond()
{
    return EraseField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->FramesPerSecond);
}

void 
SdfLayer::SetFramePrecision( int newVal )
{
    _SetValue(SdfFieldKeys->FramePrecision, newVal);
}

int
SdfLayer::GetFramePrecision() const
{
    return _GetValue<int>(SdfFieldKeys->FramePrecision);
}

bool
SdfLayer::HasFramePrecision() const
{
    return HasField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->FramePrecision);
}

void
SdfLayer::ClearFramePrecision()
{
    return EraseField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->FramePrecision);
}

string
SdfLayer::GetOwner() const
{
    return _GetValue<string>(SdfFieldKeys->Owner);
}

void
SdfLayer::SetOwner(const std::string& newVal)
{
    _SetValue(SdfFieldKeys->Owner, newVal);
}

bool
SdfLayer::HasOwner() const
{
    return HasField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->Owner);
}

void
SdfLayer::ClearOwner()
{
    return EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->Owner);
}

string
SdfLayer::GetSessionOwner() const
{
    return _GetValue<string>(SdfFieldKeys->SessionOwner);
}

void
SdfLayer::SetSessionOwner(const std::string& newVal)
{
    _SetValue(SdfFieldKeys->SessionOwner, newVal);
}

bool
SdfLayer::HasSessionOwner() const
{
    return HasField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->SessionOwner);
}

void
SdfLayer::ClearSessionOwner()
{
    return EraseField(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->SessionOwner);
}

bool
SdfLayer::GetHasOwnedSubLayers() const
{
    return _GetValue<bool>(SdfFieldKeys->HasOwnedSubLayers);
}

void
SdfLayer::SetHasOwnedSubLayers(bool newVal)
{
    _SetValue(SdfFieldKeys->HasOwnedSubLayers, newVal);
}

VtDictionary
SdfLayer::GetCustomLayerData() const
{
    return _GetValue<VtDictionary>(SdfFieldKeys->CustomLayerData);
}

void
SdfLayer::SetCustomLayerData(const VtDictionary& dict)
{
    _SetValue(SdfFieldKeys->CustomLayerData, dict);
}

bool 
SdfLayer::HasCustomLayerData() const
{
    return HasField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->CustomLayerData);
}

void 
SdfLayer::ClearCustomLayerData()
{
    EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->CustomLayerData);
}

SdfPrimSpecHandle
SdfLayer::GetPseudoRoot() const
{
    return SdfPrimSpecHandle(
        _idRegistry.Identify(SdfPath::AbsoluteRootPath()));
}

SdfLayer::RootPrimsView
SdfLayer::GetRootPrims() const
{
    return GetPseudoRoot()->GetNameChildren();
}

void
SdfLayer::SetRootPrims( const SdfPrimSpecHandleVector &newComps )
{
    return GetPseudoRoot()->SetNameChildren(newComps);
}

bool
SdfLayer::InsertRootPrim( const SdfPrimSpecHandle & prim, int index )
{
    return GetPseudoRoot()->InsertNameChild(prim, index);
}

void
SdfLayer::RemoveRootPrim(const SdfPrimSpecHandle & prim)
{
    GetPseudoRoot()->RemoveNameChild(prim);
}

SdfNameOrderProxy
SdfLayer::GetRootPrimOrder() const
{
    return GetPseudoRoot()->GetNameChildrenOrder();
}

void
SdfLayer::SetRootPrimOrder( const vector<TfToken>& names )
{
    GetPseudoRoot()->SetNameChildrenOrder(names);
}

void
SdfLayer::InsertInRootPrimOrder( const TfToken & name, int index )
{
    GetPseudoRoot()->InsertInNameChildrenOrder(name, index);
}

void
SdfLayer::RemoveFromRootPrimOrder( const TfToken & name )
{
    GetPseudoRoot()->RemoveFromNameChildrenOrder(name);
}

void
SdfLayer::RemoveFromRootPrimOrderByIndex(int index)
{
    GetPseudoRoot()->RemoveFromNameChildrenOrderByIndex(index);
}

void
SdfLayer::ApplyRootPrimOrder( vector<TfToken>* vec ) const
{
    GetPseudoRoot()->ApplyNameChildrenOrder(vec);
}

SdfSubLayerProxy
SdfLayer::GetSubLayerPaths() const
{
    boost::shared_ptr<Sdf_ListEditor<SdfSubLayerTypePolicy> > editor(
        new Sdf_SubLayerListEditor(SdfCreateNonConstHandle(this)));
    
    return SdfSubLayerProxy(editor, SdfListOpTypeOrdered);
}

void
SdfLayer::SetSubLayerPaths(const vector<string>& newPaths)
{
    GetSubLayerPaths() = newPaths;
}

size_t
SdfLayer::GetNumSubLayerPaths() const
{
    return GetSubLayerPaths().size();
}

void
SdfLayer::InsertSubLayerPath(const string& path, int index)
{
    SdfSubLayerProxy proxy = GetSubLayerPaths();

    if (index == -1) {
        index = static_cast<int>(proxy.size());
    }

    proxy.Insert(index, path);
}

void
SdfLayer::RemoveSubLayerPath(int index)
{
    GetSubLayerPaths().Erase(index);
}

SdfLayerOffsetVector
SdfLayer::GetSubLayerOffsets() const
{
    return GetFieldAs<SdfLayerOffsetVector>(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets);
}

SdfLayerOffset
SdfLayer::GetSubLayerOffset(int index) const
{
    SdfLayerOffsetVector offsets = GetSubLayerOffsets();
    if (index < 0 || static_cast<size_t>(index) >= offsets.size()) {
        TF_CODING_ERROR("Invalid sublayer index");
        return SdfLayerOffset();
    }
    return offsets[index];
}

void
SdfLayer::SetSubLayerOffset(const SdfLayerOffset& offset, int index)
{
    SdfLayerOffsetVector offsets = GetFieldAs<SdfLayerOffsetVector>(
        SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets);
    if (index < 0 || static_cast<size_t>(index) >= offsets.size()) {
        TF_CODING_ERROR("Invalid sublayer index");
        return;
    }
    
    offsets[index] = offset;
    
    SetField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->SubLayerOffsets,
        VtValue(offsets));
}

bool 
SdfLayer::_CanGetSpecAtPath(
    const SdfPath& path, 
    SdfPath* canonicalPath, SdfSpecType* specType) const
{
    if (path.IsEmpty()) {
        return false;
    }

    // We need to always call MakeAbsolutePath, even if relativePath is
    // already absolute, because we also need to absolutize target paths
    // within the path.
    const SdfPath &absPath =
        path.IsAbsolutePath() && !path.ContainsTargetPath() ? path :
        path.MakeAbsolutePath(SdfPath::AbsoluteRootPath());

    // Grab the object type stored in the SdfData hash table. If no type has
    // been set, this path doesn't point to a valid location.
    if (!HasSpec(absPath)) {
        return false;
    }

    *canonicalPath = absPath;
    *specType = GetSpecType(absPath);
    return true;
}

template <class Spec>
SdfHandle<Spec>
SdfLayer::_GetSpecAtPath(const SdfPath& path)
{
    SdfPath canonicalPath;
    SdfSpecType specType;
    if (!_CanGetSpecAtPath(path, &canonicalPath, &specType) ||
        !Sdf_SpecType::CanCast(specType, typeid(Spec))) {
        return TfNullPtr;
    }

    return SdfHandle<Spec>(_idRegistry.Identify(canonicalPath));
}

SdfSpecHandle
SdfLayer::GetObjectAtPath(const SdfPath &path)
{
    // This function is exactly the same as _GetSpecAtPath, but skips the
    // CanCast(...) check since all specs can be represented by SdfSpecHandles.
    // In addition, this avoids issues when dealing with things like
    // relationship target specs where an SdfSpecType value is defined, but
    // no C++ SdfSpec class exists. In that case, consumers should still be
    // able to get a generic SdfSpecHandle.
    SdfPath canonicalPath;
    SdfSpecType specType;
    if (!_CanGetSpecAtPath(path, &canonicalPath, &specType)) {
        return TfNullPtr;
    }

    return SdfSpecHandle(_idRegistry.Identify(canonicalPath));
}

SdfPrimSpecHandle
SdfLayer::GetPrimAtPath(const SdfPath& path)
{
    // Special-case attempts to look up the pseudo-root via this function.
    if (path == SdfPath::AbsoluteRootPath()) {
        return GetPseudoRoot();
    }

    return _GetSpecAtPath<SdfPrimSpec>(path);
}

SdfPropertySpecHandle
SdfLayer::GetPropertyAtPath(const SdfPath &path)
{
    return _GetSpecAtPath<SdfPropertySpec>(path);
}

SdfAttributeSpecHandle
SdfLayer::GetAttributeAtPath(const SdfPath &path)
{
    return _GetSpecAtPath<SdfAttributeSpec>(path);
}

SdfRelationshipSpecHandle
SdfLayer::GetRelationshipAtPath(const SdfPath &path)
{
    return _GetSpecAtPath<SdfRelationshipSpec>(path);
}

bool
SdfLayer::PermissionToEdit() const
{
    return _permissionToEdit && !IsMuted();
}

bool
SdfLayer::PermissionToSave() const
{
    return _permissionToSave &&
        !IsAnonymous() &&
        !IsMuted()     &&
        Sdf_CanWriteLayerToPath(GetRealPath());
}

void
SdfLayer::SetPermissionToEdit(bool allow)
{
    _permissionToEdit = allow;
}

void
SdfLayer::SetPermissionToSave(bool allow)
{
    _permissionToSave = allow;
}

static
bool
_HasObjectAtPath(const SdfLayerHandle& layer, const SdfPath& path)
{
    return layer->GetObjectAtPath(path);
}

static
bool
_CanEdit(
    const SdfLayerHandle& layer,
    const SdfNamespaceEdit& edit,
    std::string* detail)
{
    if (edit.currentPath.IsPrimPath()) {
        if (edit.newPath.IsEmpty()) {
            // Remove prim child.
            return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::
                    CanRemoveChildForBatchNamespaceEdit(
                        layer, edit.currentPath.GetParentPath(),
                        edit.currentPath.GetNameToken(),
                        detail);
        }
        else {
            // Insert prim child.
            return Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::
                    CanMoveChildForBatchNamespaceEdit(
                        layer, edit.newPath.GetParentPath(),
                        layer->GetPrimAtPath(edit.currentPath),
                        edit.newPath.GetNameToken(),
                        edit.index,
                        detail);
        }
    }
    else {
        if (edit.newPath.IsEmpty()) {
            if (edit.currentPath.IsRelationalAttributePath()) {
                // Remove relational attribute.
                return Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::
                        CanRemoveChildForBatchNamespaceEdit(
                            layer, edit.currentPath.GetParentPath(),
                            edit.currentPath.GetNameToken(),
                            detail);
            }
            else {
                // Remove prim property.
                return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::
                        CanRemoveChildForBatchNamespaceEdit(
                            layer, edit.currentPath.GetParentPath(),
                            edit.currentPath.GetNameToken(),
                            detail);
            }
        }
        else if (edit.newPath.IsRelationalAttributePath()) {
            if (SdfAttributeSpecHandle attr =
                    layer->GetAttributeAtPath(edit.currentPath)) {
                // Move a prim or relational attribute to be a relational
                // attribute.
                return Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::
                        CanMoveChildForBatchNamespaceEdit(
                            layer, edit.newPath.GetParentPath(),
                            attr,
                            edit.newPath.GetNameToken(),
                            edit.index,
                            detail);
            }
            else {
                // Trying to make a non-attribute into a relational attribute.
                if (detail) {
                    *detail = "Object is not an attribute";
                }
                return false;
            }
        }
        else {
            // Move a prim property or relational attribute to be a prim
            // property
            return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::
                    CanMoveChildForBatchNamespaceEdit(
                        layer, edit.newPath.GetParentPath(),
                        layer->GetPropertyAtPath(edit.currentPath),
                        edit.newPath.GetNameToken(),
                        edit.index,
                        detail);
        }
    }
}

static
void
_DoEdit(const SdfLayerHandle& layer, const SdfNamespaceEdit& edit)
{
    if (edit.currentPath.IsPrimPath()) {
        if (edit.newPath.IsEmpty()) {
            // Remove prim child.
            Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::
                RemoveChildForBatchNamespaceEdit(
                    layer, edit.currentPath.GetParentPath(),
                    edit.currentPath.GetNameToken());
        }
        else {
            // Insert prim child.
            Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::
                MoveChildForBatchNamespaceEdit(
                    layer, edit.newPath.GetParentPath(),
                    layer->GetPrimAtPath(edit.currentPath),
                    edit.newPath.GetNameToken(),
                    edit.index);
        }
    }
    else {
        if (edit.newPath.IsEmpty()) {
            if (edit.currentPath.IsRelationalAttributePath()) {
                // Remove relational attribute.
                Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::
                    RemoveChildForBatchNamespaceEdit(
                        layer, edit.currentPath.GetParentPath(),
                        edit.currentPath.GetNameToken());
            }
            else {
                // Remove prim property.
                Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::
                    RemoveChildForBatchNamespaceEdit(
                        layer, edit.currentPath.GetParentPath(),
                        edit.currentPath.GetNameToken());
            }
        }
        else {
            if (edit.newPath.IsRelationalAttributePath()) {
                // Move a prim or relational attribute to be a relational
                // attribute.
                Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::
                    MoveChildForBatchNamespaceEdit(
                        layer, edit.newPath.GetParentPath(),
                        layer->GetAttributeAtPath(edit.currentPath),
                        edit.newPath.GetNameToken(),
                        edit.index);
            }
            else {
                // Move a prim property or relational attribute to be a prim
                // property
                Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::
                    MoveChildForBatchNamespaceEdit(
                        layer, edit.newPath.GetParentPath(),
                        layer->GetPropertyAtPath(edit.currentPath),
                        edit.newPath.GetNameToken(),
                        edit.index);
            }
        }
    }
}

SdfNamespaceEditDetail::Result
SdfLayer::CanApply(
    const SdfBatchNamespaceEdit& edits,
    SdfNamespaceEditDetailVector* details) const
{
    SdfNamespaceEditDetail::Result result = SdfNamespaceEditDetail::Okay;

    static const bool fixBackpointers = true;
    SdfLayerHandle self = SdfCreateNonConstHandle(this);
    if (!edits.Process(NULL,
                       std::bind(&_HasObjectAtPath, self, ph::_1),
                       std::bind(&_CanEdit, self, ph::_1, ph::_2),
                       details, !fixBackpointers)) {
        result = CombineError(result);
    }

    return result;
}

bool
SdfLayer::Apply(const SdfBatchNamespaceEdit& edits)
{
    if (!PermissionToEdit()) {
        return false;
    }

    static const bool fixBackpointers = true;
    SdfLayerHandle self(this);
    SdfNamespaceEditVector final;
    if (!edits.Process(&final,
                       std::bind(&_HasObjectAtPath, self, ph::_1),
                       std::bind(&_CanEdit, self, ph::_1, ph::_2),
                       NULL, !fixBackpointers)) {
        return false;
    }

    SdfChangeBlock block;
    for (const auto& edit : final) {
        _DoEdit(self, edit);
    }

    return true;
}

void
SdfLayer::ScheduleRemoveIfInert(const SdfSpec& spec)
{
    Sdf_ChangeManager::Get().RemoveSpecIfInert(spec);
}

void
SdfLayer::_RemoveIfInert(const SdfSpec& spec)
{
    if (!spec.IsDormant()) {
        SdfSpecHandle specHandle(spec);
        if (SdfPrimSpecHandle prim =
            TfDynamic_cast<SdfPrimSpecHandle>(specHandle)) {
            // We only want to call RemovePrimIfInert if the prim itself is 
            // inert because RemovePrimIfInert first removes any inert children 
            // before checking if the prim is inert, but we don't want to touch 
            // the children. We only want to concern ourselves with the 
            // specified spec without modifying its children first.
            if (prim->IsInert()) {
                RemovePrimIfInert(prim);
            }
        }
        else if(SdfPropertySpecHandle property =
                TfDynamic_cast<SdfPropertySpecHandle>(specHandle)) {

            RemovePropertyIfHasOnlyRequiredFields(property);
        }
    }
}

void
SdfLayer::RemovePrimIfInert(SdfPrimSpecHandle prim)
{
    if (prim && _RemoveInertDFS(prim))
        _RemoveInertToRootmost(prim);
}

void
SdfLayer::RemovePropertyIfHasOnlyRequiredFields(SdfPropertySpecHandle prop)
{
    if (!(prop && prop->HasOnlyRequiredFields()))
        return;

    // XXX -- This doesn't deal with relational attributes;  bug 20145.
    if (SdfPrimSpecHandle owner = 
        TfDynamic_cast<SdfPrimSpecHandle>(prop->GetOwner())) {

        owner->RemoveProperty(prop);
        _RemoveInertToRootmost(owner);

    } else if (SdfRelationshipSpecHandle owner = 
               TfDynamic_cast<SdfRelationshipSpecHandle>(prop->GetOwner())) {

        if (SdfAttributeSpecHandle attr = 
            TfDynamic_cast<SdfAttributeSpecHandle>(prop)) {

            owner->RemoveAttributeForTargetPath(
                owner->GetTargetPathForAttribute(attr), attr);

            //XXX: We may want to do something like 
            //     _RemoveInertToRootmost here, but that would currently 
            //     exacerbate bug 23878. Until we have  a solution for that bug,
            //     we won't automatically clean up our parent (and his parent, 
            //     etc) when deleting a relational attribute.
        }
    }
}

void
SdfLayer::RemoveInertSceneDescription()
{
    SdfChangeBlock block;

    _RemoveInertDFS(GetPseudoRoot());
}

bool
SdfLayer::_RemoveInertDFS(SdfPrimSpecHandle prim)
{
    bool inert = prim->IsInert();

    if (!inert) {
        // Child prims
        SdfPrimSpecHandleVector removedChildren;
        TF_FOR_ALL(it, prim->GetNameChildren()) {
            SdfPrimSpecHandle child = *it;
            if (_RemoveInertDFS(child) &&
                !SdfIsDefiningSpecifier(child->GetSpecifier()))
                removedChildren.push_back(child);
        }
        TF_FOR_ALL(it, removedChildren) {
            prim->RemoveNameChild(*it);
        }
        // Child prims inside variants
        SdfVariantSetsProxy variantSetMap = prim->GetVariantSets();
        TF_FOR_ALL(varSetIt, variantSetMap) {
            const SdfVariantSetSpecHandle &varSetSpec = varSetIt->second;
            const SdfVariantSpecHandleVector &variants =
                varSetSpec->GetVariantList();
            TF_FOR_ALL(varIt, variants) {
                _RemoveInertDFS((*varIt)->GetPrimSpec());
            }
        }
    }

    return inert ? inert : prim->IsInert();
}

void
SdfLayer::_RemoveInertToRootmost(SdfPrimSpecHandle prim)
{
    while (prim &&
           !SdfIsDefiningSpecifier(prim->GetSpecifier()) &&
           prim->IsInert()) {
        SdfPrimSpecHandle parent = prim->GetRealNameParent();
        if (parent) {
            parent->RemoveNameChild(prim);
        }

        // Recurse.
        prim = parent;
    }
}

bool
SdfLayer::SplitIdentifier(
    const string& identifier,
    string* layerPath,
    FileFormatArguments* arguments)
{
    return Sdf_SplitIdentifier(identifier, layerPath, arguments);
}

std::string 
SdfLayer::CreateIdentifier(
    const string& layerPath,
    const FileFormatArguments& arguments)
{
    return Sdf_CreateIdentifier(layerPath, arguments);
}

const string&
SdfLayer::GetIdentifier() const
{
    return _assetInfo->identifier;
}

void
SdfLayer::SetIdentifier(const string &identifier)
{
    TRACE_FUNCTION();
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::SetIdentifier('%s')\n",
        identifier.c_str());

    string oldLayerPath, oldArguments;
    if (!TF_VERIFY(Sdf_SplitIdentifier(
            GetIdentifier(), &oldLayerPath, &oldArguments))) {
        return;
    }

    string newLayerPath, newArguments;
    if (!Sdf_SplitIdentifier(identifier, &newLayerPath, &newArguments)) {
        TF_CODING_ERROR("Invalid identifier '%s'", identifier.c_str());
        return;
    }
    
    if (oldArguments != newArguments) {
        TF_CODING_ERROR(
            "Identifier '%s' contains arguments that differ from the layer's "
            "current arguments ('%s').",
            identifier.c_str(), GetIdentifier().c_str());
        return;
    }

    // When changing a layer's identifier, assume that relative identifiers are
    // relative to the current working directory.
    const string absIdentifier = ArGetResolver().IsRelativePath(identifier) ?
        TfAbsPath(identifier) : identifier;

    const string oldRealPath = GetRealPath();

    // Hold open a change block to defer identifier-did-change
    // notification until the mutex is unlocked.
    SdfChangeBlock block;
    {
        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());
        _InitializeFromIdentifier(absIdentifier);
    }

    // If this layer has changed where it's stored, reset the modification
    // time. Note that the new identifier may not resolve to an existing
    // location, and we get an empty timestamp from the resolver. 
    // This is OK -- this means the layer hasn't been serialized to this 
    // new location yet.
    const string newRealPath = GetRealPath();
    if (oldRealPath != newRealPath) {
        _assetModificationTime = ArGetResolver().GetModificationTimestamp(
            GetIdentifier(), GetRealPath());
    }
}

void
SdfLayer::UpdateAssetInfo(const string &fileVersion)
{
    TRACE_FUNCTION();
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::UpdateAssetInfo('%s')\n",
        fileVersion.c_str());

    // Hold open a change block to defer identifier-did-change
    // notification until the mutex is unlocked.
    SdfChangeBlock block;
    {
        // If the layer has a resolve info with a non-empty asset name, this
        // means that the layer identifier is a search-path to a layer within
        // an asset, which last resolved to a pinnable location. Bind the
        // original context found in the resolve info within this block so the
        // layer's search path identifier can be properly re-resolved within
        // _InitializeFromIdentifier.
        boost::scoped_ptr<ArResolverContextBinder> binder;
        if (!GetAssetName().empty()) {
            binder.reset(new ArResolverContextBinder(
                    _assetInfo->resolverContext));
        }    

        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());
        _InitializeFromIdentifier(GetIdentifier(),
            /* realPath */ std::string(), fileVersion);
    }
}

string
SdfLayer::GetDisplayName() const
{
    return GetDisplayNameFromIdentifier(GetIdentifier());
}

const string&
SdfLayer::GetRealPath() const
{
    return _assetInfo->realPath;
}

string
SdfLayer::GetFileExtension() const
{
    string ext = Sdf_GetExtension(GetRealPath());

    if (ext.empty())
        ext = GetFileFormat()->GetPrimaryFileExtension();

    return ext;
}

const string&
SdfLayer::GetRepositoryPath() const
{
    return _assetInfo->assetInfo.repoPath;
}

const string&
SdfLayer::GetVersion() const
{
    return _assetInfo->assetInfo.version;
}

const VtValue&
SdfLayer::GetAssetInfo() const
{
    return _assetInfo->assetInfo.resolverInfo;
}

const string&
SdfLayer::GetAssetName() const 
{
    return _assetInfo->assetInfo.assetName;
}

SdfDataRefPtr
SdfLayer::GetMetadata() const
{
    SdfDataRefPtr result = TfCreateRefPtr(new SdfData);
    const SdfAbstractDataSpecId rootId(&SdfPath::AbsoluteRootPath());

    // The metadata for this layer is the data at the absolute root path.
    // Here, we copy it into 'result'.
    //
    // XXX: This is copying more than just the metadata. This includes things
    //      like name children, etc. We should probably be filtering this to
    //      just fields tagged as metadata in the schema.
    result->CreateSpec(rootId, SdfSpecTypePseudoRoot);
    const TfTokenVector tokenVec = _data->List(rootId);
    for (auto const &token : tokenVec) {
        const VtValue &value = GetField(rootId, token);
        result->Set(rootId, token, value);
    }

    return result;
}

string
SdfLayer::ComputeAbsolutePath(const string &relativePath)
{
    if (relativePath.empty()
        || Sdf_IsAnonLayerIdentifier(relativePath)){
        return relativePath;
    }

    // Make it relative to the repository path, if available, so that path
    // resolution will work for references.
    const string relativeToPath = GetRepositoryPath().empty() ?
        GetRealPath() : GetRepositoryPath();
    return ArGetResolver().AnchorRelativePath(relativeToPath, relativePath);
}

string
SdfLayer::_GetMutedPath() const
{
    return GetRepositoryPath().empty()
           ? GetIdentifier()
           : GetRepositoryPath();
}

set<string>
SdfLayer::GetMutedLayers()
{
    std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
    return *_mutedLayers;
}

void 
SdfLayer::SetMuted(bool muted)
{
    // XXX Racy...
    
    if (muted == IsMuted()) {
        return;
    }

    if (muted) {
        AddToMutedLayers(_GetMutedPath());
    }
    else {
        RemoveFromMutedLayers(_GetMutedPath());
    }
}

bool 
SdfLayer::IsMuted() const
{
    // Read the current muted revision number.  If it's up-to-date we return our
    // cache.  It's possible that this is racy, but the whole thing is racy
    // regardless.  Even with a pure locking implementation, say we found this
    // layer in the muted set -- by the time we return to the caller with
    // 'true', some other thread may have removed this layer from the muted set.

    size_t curRev = _mutedLayersRevision;
    if (ARCH_UNLIKELY(_mutedLayersRevisionCache != curRev)) {
        string mutedPath = _GetMutedPath();
        std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
        // Read again, since this is guaranteed to give us the current value
        // because we have the lock.  _mutedLayersRevision only changes with the
        // lock held.
        _mutedLayersRevisionCache = _mutedLayersRevision;
        _isMutedCache = _mutedLayers->count(mutedPath);
    }

    return _isMutedCache;
}

/*static*/
bool
SdfLayer::IsMuted(const string &path)
{
    std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
    return _mutedLayers->count(path);
}

/*static*/
void
SdfLayer::AddToMutedLayers(const string &path)
{
    bool didChange = false;
    {
        // Racy...
        std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
        ++_mutedLayersRevision;
        didChange = _mutedLayers->insert(path).second;
    }
    if (didChange) {
        if (SdfLayerHandle layer = Find(path)) {
            if (layer->IsDirty()) {
                SdfFileFormatConstPtr format = layer->GetFileFormat();
                SdfAbstractDataRefPtr initializedData = 
                    format->InitData(layer->GetFileFormatArguments());
                if (format->IsStreamingLayer(*layer.operator->())) {
                    // See the discussion in TransferContent()
                    // about streaming layers; the same concerns
                    // apply here.  We must swap out the actual data
                    // ownership and tell clients the entire data
                    // store has changed.
                    {
                        std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
                        TF_VERIFY((*_mutedLayerData).find(path) ==
                                  (*_mutedLayerData).end());
                        (*_mutedLayerData)[path] = layer->_data;
                    }
                    // _SetData() takes ownership of initializedData and sends
                    // change notification.
                    layer->_SetData(initializedData);
                } else {
                    // Copy the dirty layer data to an in-memory store
                    // that will be owned by _mutedLayerData.
                    SdfAbstractDataRefPtr mutedData = 
                        format->InitData(layer->GetFileFormatArguments());
                    mutedData->CopyFrom(layer->_data);
                    {
                        std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
                        TF_VERIFY((*_mutedLayerData).find(path) ==
                                  (*_mutedLayerData).end());
                        std::swap( (*_mutedLayerData)[path], mutedData );
                    }
                    // Mutate the layer's data to the initialized state.
                    // This enables efficient change processing downstream.
                    layer->_SetData(initializedData);
                }
                TF_VERIFY(layer->IsDirty());
            } else {
                // Reload as muted.
                layer->_Reload(/* force */ true);
            }
        }
        SdfNotice::LayerMutenessChanged(path, /* wasMuted = */ true).Send();
    }
}

/*static*/
void
SdfLayer::RemoveFromMutedLayers(const string &path)
{
    bool didChange = false;
    {
        // Racy...
        std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
        ++_mutedLayersRevision;
        didChange = _mutedLayers->erase(path);
    }
    if (didChange) {
        if (SdfLayerHandle layer = Find(path)) {
            if (layer->IsDirty()) {
                SdfAbstractDataRefPtr mutedData;
                {
                    std::lock_guard<std::mutex> lock(*_mutedLayersMutex);
                    _MutedLayerDataMap::iterator i =
                        _mutedLayerData->find(path);
                    if (TF_VERIFY(i != _mutedLayerData->end())) {
                        std::swap(mutedData, i->second);
                        _mutedLayerData->erase(i);
                    }
                }
                if (TF_VERIFY(mutedData)) {
                    // If IsStreamingLayer() is true, this re-takes ownership
                    // of the mutedData object.  Otherwise, this mutates
                    // the existing data container to match its contents.
                    layer->_SetData(mutedData);
                }
                TF_VERIFY(layer->IsDirty());
            } else {
                // Reload as unmuted.
                layer->_Reload(/* force */ true);
            }
        }
        SdfNotice::LayerMutenessChanged(path, /* wasMuted = */ false).Send();
    }
}

bool
SdfLayer::_ShouldNotify() const
{
    // Only notify if this layer has been successfully initialized.
    // (If initialization is not yet complete, do not notify.)
    return _initializationWasSuccessful.get_value_or(false);
}

void
SdfLayer::Clear()
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Clear: Permission denied.");
        return;
    }

    _SetData(GetFileFormat()->InitData(GetFileFormatArguments()));

    if (GetFileFormat()->IsStreamingLayer(*this))
        _stateDelegate->_MarkCurrentStateAsDirty();
}

bool
SdfLayer::IsDirty() const
{
    return (TF_VERIFY(_stateDelegate) ? _stateDelegate->IsDirty() : false);
}

bool
SdfLayer::_UpdateLastDirtinessState() const
{
    // Did not change since last call... 
    if (IsDirty() == _lastDirtyState)
        return false;

    // It did change, update last saved changed state...
    _lastDirtyState = IsDirty();

    return true;
}

SdfLayerStateDelegateBasePtr 
SdfLayer::GetStateDelegate() const
{
    return _stateDelegate;
}

void 
SdfLayer::SetStateDelegate(const SdfLayerStateDelegateBaseRefPtr& delegate)
{
    // A layer can never have an invalid state delegate, as it relies
    // on it to track dirtiness.
    if (!delegate) {
        TF_CODING_ERROR("Invalid layer state delegate");
        return;
    }

    _stateDelegate->_SetLayer(SdfLayerHandle());
    _stateDelegate = delegate;
    _stateDelegate->_SetLayer(SdfCreateHandle(this));

    if (_lastDirtyState) {
        _stateDelegate->_MarkCurrentStateAsDirty();
    }
    else {
        _stateDelegate->_MarkCurrentStateAsClean();
    }
}

void
SdfLayer::_MarkCurrentStateAsClean() const
{
    if (TF_VERIFY(_stateDelegate)) {
        _stateDelegate->_MarkCurrentStateAsClean();
    }

    if (_UpdateLastDirtinessState()) {
        SdfLayerHandle layer = SdfCreateNonConstHandle(this);
        SdfNotice::LayerDirtinessChanged().Send(layer);
    }
}

bool
SdfLayer::IsEmpty() const
{
    // XXX: What about documentation/frames?  I don't
    // think these get composed or exposed through composition, so I don't think
    // they matter for the sake of this query.
    return GetRootPrims().empty()  && 
        GetRootPrimOrder().empty() && 
        GetSubLayerPaths().empty();
}

void
SdfLayer::TransferContent(const SdfLayerHandle& layer)
{
    if (!PermissionToEdit()) {
        TF_RUNTIME_ERROR("TransferContent of '%s': Permission denied.",
                         GetDisplayName().c_str());
        return;
    }

    // Two concerns apply here:
    //
    // If we need to notify about the changes, we need to use the
    // _SetData() API to get incremental change notification;
    // otherwise we can just blindly copy the SdfAbstractData.
    //
    // If this is a streaming layer, _SetData will simply take
    // ownership of the data object passed to it. We don't want
    // multiple layers to be sharing the same data object, so we
    // have to make a copy of the data here.
    //

    bool notify = _ShouldNotify();
    bool isStreamingLayer = GetFileFormat()->IsStreamingLayer(*this);
    SdfAbstractDataRefPtr newData;

    if (!notify || isStreamingLayer) {
        newData = GetFileFormat()->InitData(GetFileFormatArguments());
        newData->CopyFrom(layer->_data);
    }
    else {
        newData = layer->_data;
    }

    if (notify) {
        _SetData(newData);
    } else {
        _data = newData;
    }

    // If this is a "streaming" layer, we must mark it dirty.
    if (isStreamingLayer) {
        _stateDelegate->_MarkCurrentStateAsDirty();
    }
}

static void
_GatherPrimAssetReferences(const SdfPrimSpecHandle &prim,
                       set<string> *assetReferences)
{
    if (prim != prim->GetLayer()->GetPseudoRoot()) {
        // Prim references
        for (const SdfReference &ref:
             prim->GetReferenceList().GetAddedOrExplicitItems()) {
            assetReferences->insert(ref.GetAssetPath());
        }

        // Prim payloads
        if (prim->HasPayload()) {
            SdfPayload payload = prim->GetPayload();
            assetReferences->insert(payload.GetAssetPath());
        }

        // Prim variants
        SdfVariantSetsProxy variantSetMap = prim->GetVariantSets();
        TF_FOR_ALL(varSetIt, variantSetMap) {
            const SdfVariantSetSpecHandle &varSetSpec = varSetIt->second;
            const SdfVariantSpecHandleVector &variants =
                varSetSpec->GetVariantList();
            TF_FOR_ALL(varIt, variants) {
                _GatherPrimAssetReferences( (*varIt)->GetPrimSpec(),
                                        assetReferences );
            }
        }
    }

    // Recurse on nameChildren
    TF_FOR_ALL(child, prim->GetNameChildren()) {
        _GatherPrimAssetReferences(*child, assetReferences);
    }
}

set<string>
SdfLayer::GetExternalReferences()
{
    SdfSubLayerProxy subLayers = GetSubLayerPaths();

    set<string> results(subLayers.begin(), subLayers.end());

    _GatherPrimAssetReferences(GetPseudoRoot(), &results);

    return results;
}

bool
SdfLayer::UpdateExternalReference(
    const string &oldLayerPath,
    const string &newLayerPath)
{
    if (oldLayerPath.empty())
        return false;

    // Search sublayers and rename if found...
    SdfSubLayerProxy subLayers = GetSubLayerPaths();
    size_t index = subLayers.Find(oldLayerPath);
    if (index != (size_t)-1) {
        RemoveSubLayerPath(index);

        // If new layer path given, do rename, otherwise it's a delete.
        if (!newLayerPath.empty()) {
            InsertSubLayerPath(newLayerPath, index);
        }

        return true; // sublayers are unique, do no more...
    }

    _UpdateReferencePaths(GetPseudoRoot(), oldLayerPath, newLayerPath);

    return true;
}

// SdfReferenceListEditor::ModifyItemEdits() callback that updates a reference's
// asset path.
//
static boost::optional<SdfReference>
_UpdateReferencePath(
    const string &oldLayerPath,
    const string &newLayerPath,
    const SdfReference &reference)
{
    if (reference.GetAssetPath() == oldLayerPath) {
        // Delete if new layer path is empty, otherwise rename.
        if (newLayerPath.empty()) {
            return boost::optional<SdfReference>();
        } else {
            SdfReference ref = reference;
            ref.SetAssetPath(newLayerPath);
            return ref;
        }
    }
    return reference;
}

void
SdfLayer::_UpdateReferencePaths(
    const SdfPrimSpecHandle &prim,
    const string &oldLayerPath,
    const string &newLayerPath)
{
    TF_AXIOM(!oldLayerPath.empty());
    
    // Prim references
    prim->GetReferenceList().ModifyItemEdits(std::bind(
        &_UpdateReferencePath, oldLayerPath, newLayerPath, ph::_1));

    // Prim payloads
    if (prim->HasPayload()) {
        SdfPayload payload = prim->GetPayload();
        if (payload.GetAssetPath() == oldLayerPath) {
            if (newLayerPath.empty()) {
                prim->ClearPayload();
            }
            else {
                payload.SetAssetPath(newLayerPath);
                prim->SetPayload(payload);
            }
        }
    }

    // Prim variants
    SdfVariantSetsProxy variantSetMap = prim->GetVariantSets();
    for (const auto& setNameAndSpec : variantSetMap) {
        const SdfVariantSetSpecHandle &varSetSpec = setNameAndSpec.second;
        const SdfVariantSpecHandleVector &variants =
            varSetSpec->GetVariantList();
        for (const auto& variantSpec : variants) {
            _UpdateReferencePaths(
                variantSpec->GetPrimSpec(), oldLayerPath, newLayerPath);
        }
    }

    // Recurse on nameChildren
    for (const auto& primSpec : prim->GetNameChildren()) {
        _UpdateReferencePaths(primSpec, oldLayerPath, newLayerPath);
    }
}

/*static*/
void
SdfLayer::DumpLayerInfo()
{
    tbb::queuing_rw_mutex::scoped_lock
        lock(_GetLayerRegistryMutex(), /*write=*/false);
    std::cerr << "Layer Registry Dump:" << std::endl
        << *_layerRegistry << std::endl;
}

bool
SdfLayer::WriteDataFile(const string &filename)
{
    std::ofstream file(filename.c_str());
    _data->WriteToStream(file);
    return file.good();
}

/*static*/
set<SdfLayerHandle>
SdfLayer::GetLoadedLayers()
{
    tbb::queuing_rw_mutex::scoped_lock
        lock(_GetLayerRegistryMutex(), /*write=*/false);
    return _layerRegistry->GetLayers();
}

/* static */
template <class Lock>
SdfLayerRefPtr
SdfLayer::_OpenLayerAndUnlockRegistry(
    Lock &lock,
    const _FindOrOpenLayerInfo& info,
    bool metadataOnly)
{
    TfAutoMallocTag2 tag("Sdf", "SdfLayer::_OpenLayerAndUnlockRegistry "
                         + info.identifier);

    TRACE_FUNCTION();

    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::_OpenLayerAndUnlockRegistry('%s', '%s', '%s', '%s', "
        "metadataOnly=%s)\n",
        info.identifier.c_str(), info.layerPath.c_str(),
        info.fileFormat ? 
            info.fileFormat->GetFormatId().GetText() :  "unknown file format",
        TfStringify(info.fileFormatArgs).c_str(),
        metadataOnly ? "True" : "False");

    // XXX: Is this really a coding error? SdfLayer avoids issuing errors if
    //      given a non-existent file, for instance. Should we be following the
    //      same policy here?
    if (!info.fileFormat) {
        TF_CODING_ERROR("Cannot determine file format for @%s@", 
                        info.identifier.c_str());
        lock.release();
        return TfNullPtr;
    }

    // Create a new layer of the appropriate format.
    SdfLayerRefPtr layer = _CreateNewWithFormat(
        info.fileFormat, info.identifier, info.resolvedLayerPath, 
        info.assetInfo, info.fileFormatArgs);

    // The layer constructor locks _initializationMutex, which will
    // block any other threads trying to use the layer until we complete
    // initialization here.  But now that the layer is in the registry,
    // we release the registry lock to avoid blocking progress of
    // threads working with other layers.
    TF_VERIFY(_layerRegistry->
              FindByIdentifier(layer->GetIdentifier()) == layer,
              "Could not find %s", layer->GetIdentifier().c_str());

    lock.release();

    // From this point on, we need to be sure to call
    // layer->_FinishInitialization() with either success or failure,
    // in order to unblock any other threads waiting for initialization
    // to finish.

    if (info.isAnonymous != layer->IsAnonymous()) {
        if (info.isAnonymous) {
            TF_CODING_ERROR("Opened anonymous layer ('%s' with format id '%s') "
                    "but resulting layer is not anonymous.",
                    info.identifier.c_str(),
                    info.fileFormat->GetFormatId().GetText());
        }
        else {
            TF_CODING_ERROR("Opened layer without anonymous prefix "
                    "('%s' with format id '%s') but resulting layer is "
                    "anonymous.",
                    info.identifier.c_str(),
                    info.fileFormat->GetFormatId().GetText());
        }
        layer->_FinishInitialization(/* success = */ false);
        return TfNullPtr;
    }

    // This is in support of specialized file formats that piggyback
    // on anonymous layer functionality. If the layer is anonymous,
    // pass the original assetPath to the reader, otherwise, pass the
    // resolved path of the layer.
    const string readFilePath = 
        info.isAnonymous ? info.layerPath : info.resolvedLayerPath;

    if (!layer->IsMuted()) {
        // Run the file parser to read in the file contents.
        if (!layer->_Read(info.identifier, readFilePath, metadataOnly)) {
            layer->_FinishInitialization(/* success = */ false);
            return TfNullPtr;
        }
     }

    // Grab the modification time even if layer is muted and not being
    // read. Since a muted layer may become unmuted later, there needs
    // to be a non-empty timestamp so it will not be misidentified as
    // a newly created non-serialized layer.
    if (!info.isAnonymous) {
        // Grab modification timestamp.
        VtValue timestamp = ArGetResolver().GetModificationTimestamp(
            info.identifier, readFilePath);
        if (timestamp.IsEmpty()) {
            TF_CODING_ERROR(
                "Unable to get modification timestamp for '%s (%s)'",
                info.identifier.c_str(), readFilePath.c_str());
            layer->_FinishInitialization(/* success = */ false);
            return TfNullPtr;
        }
        
        layer->_assetModificationTime.Swap(timestamp);
    }

    layer->_MarkCurrentStateAsClean();

    // Layer initialization is complete.
    layer->_FinishInitialization(/* success = */ true);

    return layer;
}

bool
SdfLayer::HasSpec(const SdfAbstractDataSpecId& id) const
{
    return _data->HasSpec(id);
}

SdfSpecType
SdfLayer::GetSpecType(const SdfAbstractDataSpecId& id) const
{
    return _data->GetSpecType(id);
}

vector<TfToken>
SdfLayer::ListFields(const SdfAbstractDataSpecId& id) const
{
    // XXX: Should add all required fields.
    return _data->List(id);
}

bool
SdfLayer::HasField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   VtValue *value) const
{
    if (_data->Has(id, fieldName, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(id, fieldName)) {
        if (value)
            *value = def->GetFallbackValue();
        return true;
    }
    return false;
}

bool
SdfLayer::HasField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   SdfAbstractDataValue *value) const
{
    if (_data->Has(id, fieldName, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(id, fieldName)) {
        if (value)
            return value->StoreValue(def->GetFallbackValue());
        return true;
    }
    return false;
}

bool
SdfLayer::HasFieldDictKey(const SdfAbstractDataSpecId& id,
                          const TfToken &fieldName,
                          const TfToken &keyPath,
                          VtValue *value) const
{
    if (_data->HasDictKey(id, fieldName, keyPath, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(id, fieldName)) {
        VtValue const &fallback = def->GetFallbackValue();
        if (fallback.IsHolding<VtDictionary>()) {
            VtDictionary const &dict = fallback.UncheckedGet<VtDictionary>();
            if (VtValue const *v = dict.GetValueAtPath(keyPath)) {
                if (value)
                    *value = *v;
                return true;
            }
        }
    }
    return false;
}

bool
SdfLayer::HasFieldDictKey(const SdfAbstractDataSpecId& id,
                          const TfToken &fieldName,
                          const TfToken &keyPath,
                          SdfAbstractDataValue *value) const
{
    if (_data->HasDictKey(id, fieldName, keyPath, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(id, fieldName)) {
        VtValue const &fallback = def->GetFallbackValue();
        if (fallback.IsHolding<VtDictionary>()) {
            VtDictionary const &dict = fallback.UncheckedGet<VtDictionary>();
            if (VtValue const *v = dict.GetValueAtPath(keyPath)) {
                if (value)
                    return value->StoreValue(*v);
                return true;
            }
        }
    }
    return false;
}

VtValue
SdfLayer::GetField(const SdfAbstractDataSpecId& id,
    const TfToken& fieldName) const
{
    VtValue result;
    HasField(id, fieldName, &result);
    return result;
}

VtValue
SdfLayer::GetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                 const TfToken& fieldName,
                                 const TfToken &keyPath) const
{
    VtValue result;
    HasFieldDictKey(id, fieldName, keyPath, &result);
    return result;
}

void
SdfLayer::SetField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   const VtValue& value)
{
    if (value.IsEmpty())
        return EraseField(id, fieldName);

    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot set %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetField(id, fieldName);
    if (value != oldValue)
        _PrimSetField(id, fieldName, value, &oldValue);
}

void
SdfLayer::SetField(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   const SdfAbstractDataConstValue& value)
{
    if (value.IsEqual(VtValue()))
        return EraseField(id, fieldName);

    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot set %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }
    
    VtValue oldValue = GetField(id, fieldName);
    if (!value.IsEqual(oldValue))
        _PrimSetField(id, fieldName, value, &oldValue);
}

void
SdfLayer::SetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                 const TfToken& fieldName,
                                 const TfToken &keyPath,
                                 const VtValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetFieldDictValueByKey(id, fieldName, keyPath);
    if (value == oldValue)
        return;

    _PrimSetFieldDictValueByKey(id, fieldName, keyPath, value, &oldValue);
}

void
SdfLayer::SetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                 const TfToken& fieldName,
                                 const TfToken &keyPath,
                                 const SdfAbstractDataConstValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetFieldDictValueByKey(id, fieldName, keyPath); 
    if (value.IsEqual(oldValue))
        return;

    _PrimSetFieldDictValueByKey(id, fieldName, keyPath, value, &oldValue);
}

void
SdfLayer::EraseField(const SdfAbstractDataSpecId& id, const TfToken& fieldName)
{
    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot erase %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    if (!_data->Has(id, fieldName)) {
        return;
    }

    // If this is a required field, only perform the set if the current value
    // differs from the fallback.  Required fields behave as if they're always
    // authored, so the effect of an "erase" is to set the value to the fallback
    // value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(id, fieldName)) {
        if (GetField(id, fieldName) == def->GetFallbackValue())
            return;
    }

    // XXX:
    // Note that with this implementation, erasing a field and undoing that
    // operation will not restore the underlying SdfData exactly to its
    // previous state. Specifically, this may cause the order of the fields
    // for the given spec to change. There are no semantics attached to this
    // ordering, so this should hopefully be OK.
    _PrimSetField(id, fieldName, VtValue());
}

void
SdfLayer::EraseFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken& fieldName,
                                   const TfToken &keyPath)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot erase %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        id.GetString().c_str(), 
                        GetIdentifier().c_str());
        return;
    }

    if (!_data->HasDictKey(id, fieldName, keyPath,
                           static_cast<VtValue *>(NULL))) {
        return;
    }

    // XXX:
    // Note that with this implementation, erasing a field and undoing that
    // operation will not restore the underlying SdfData exactly to its
    // previous state. Specifically, this may cause the order of the fields
    // for the given spec to change. There are no semantics attached to this
    // ordering, so this should hopefully be OK.
    _PrimSetFieldDictValueByKey(id, fieldName, keyPath, VtValue());
}

SdfAbstractDataConstPtr 
SdfLayer::_GetData() const
{
    return _data;
}

void
SdfLayer::_SwapData(SdfAbstractDataRefPtr &data)
{
    _data.swap(data);
}

void
SdfLayer::_SetData(const SdfAbstractDataPtr &newData)
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Setting layer data");

    // Guard against setting an empty SdfData, which is invalid.
    TF_VERIFY(!newData->IsEmpty() );

    // This code below performs a series of specific edits to mutate _data
    // to match newData.  This approach provides fine-grained change
    // notification, which allows more efficient invalidation in clients
    // of Sd.  Do all this in a single changeblock.
    SdfChangeBlock block;

    // If this layer streams its data on demand, we cannot perform 
    // fine-grained change notification because that would cause all of 
    // the data in the layer to be streamed in from disk.
    // So, all we can do is move the new data into place and
    // notify the world that this layer may have changed arbitrarily.
    if (GetFileFormat()->IsStreamingLayer(*this)) {
        _data = newData;
        Sdf_ChangeManager::Get()
            .DidReplaceLayerContent(SdfCreateHandle(this));
        return;
    }

    // Remove specs that no longer exist or whose required fields changed.
    {
        // Collect specs to delete, ordered by namespace.
        struct _SpecsToDelete : public SdfAbstractDataSpecVisitor {
            _SpecsToDelete(const SdfAbstractDataPtr& newData_)
                : newData(newData_) { }

            virtual bool VisitSpec(
                const SdfAbstractData& oldData, const SdfAbstractDataSpecId& id)
            {
                if (!newData->HasSpec(id) ||
                    (newData->GetSpecType(id) != oldData.GetSpecType(id))) {
                    paths.insert(id.GetFullSpecPath());
                }
                return true;
            }

            virtual void Done(const SdfAbstractData&)
            {
                // Do nothing
            }

            const SdfAbstractDataRefPtr newData;
            std::set<SdfPath> paths;
        };

        _SpecsToDelete specsToDelete(newData);
        _data->VisitSpecs(&specsToDelete);

        // Delete specs bottom-up to provide optimal diffs.
        // Erase fields first, to take advantage of the more efficient
        // update possible when removing inert specs.
        TF_REVERSE_FOR_ALL(i, specsToDelete.paths) {
            const SdfAbstractDataSpecId id(&*i);

            std::vector<TfToken> fields = _data->List(id);

            SdfSpecType specType = _data->GetSpecType(id);
            const SdfSchema::SpecDefinition* specDefinition = 
                GetSchema().GetSpecDefinition(specType);

            TF_FOR_ALL(field, fields) {
                if (!specDefinition->IsRequiredField(*field))
                    _PrimSetField(id, *field, VtValue());
            }
            _PrimDeleteSpec(*i, _IsInertSubtree(*i));
        }
    }

    // Create new specs.
    {
        // Collect specs to create, ordered by namespace.
        struct _SpecsToCreate : public SdfAbstractDataSpecVisitor {
            _SpecsToCreate(const SdfAbstractData& oldData_) 
                : oldData(oldData_) { }

            virtual bool VisitSpec(
                const SdfAbstractData& newData, const SdfAbstractDataSpecId& id)
            {
                if (!oldData.HasSpec(id)) {
                    paths.insert(id.GetFullSpecPath());
                }
                return true;
            }

            virtual void Done(const SdfAbstractData&)
            {
                // Do nothing
            }

            const SdfAbstractData& oldData;
            std::set<SdfPath> paths;
        };

        _SpecsToCreate specsToCreate(*boost::get_pointer(_data));
        newData->VisitSpecs(&specsToCreate);

        // Create specs top-down to provide optimal diffs.
        TF_FOR_ALL(i, specsToCreate.paths) {
            const SdfPath& path = *i;
            const SdfAbstractDataSpecId id(&path);

            // Determine if the spec is inert based on its fields.
            //
            // XXX We should consolidate this with the logic
            //     in the spec _New() methods.
            bool inert = false;
            if (path.IsPrimPath()) {
                // Prims are considered inert if they are an 'over' with
                // no typename. Make sure we specify the expected fallback
                // values in case newData does not explicitly store a value
                // for these fields.
                inert = 
                    (newData->GetAs<SdfSpecifier>(id, SdfFieldKeys->Specifier,
                                                  SdfSpecifierOver)
                        == SdfSpecifierOver)
                    && (newData->GetAs<TfToken>(id, SdfFieldKeys->TypeName,
                                                TfToken())
                        .IsEmpty());
            } else if (path.IsPropertyPath()) {
                // Properties are considered inert if they are custom.
                inert = !newData->GetAs<bool>(id, SdfFieldKeys->Custom,
                                              false);
            }

            SdfSpecType specType = newData->GetSpecType(id);

            _PrimCreateSpec(path, specType, inert);
        }
    }

    // Update spec fields.
    {
        struct _SpecUpdater : public SdfAbstractDataSpecVisitor {
            _SpecUpdater(SdfLayer* layer_) : layer(layer_) { }

            virtual bool VisitSpec(
                const SdfAbstractData& newData, const SdfAbstractDataSpecId& id)
            {
                const TfTokenVector oldFields = layer->_data->List(id);
                const TfTokenVector newFields = newData.List(id);

                // Remove empty fields.
                TF_FOR_ALL(field, oldFields) {
                    // This is O(N^2) in number of fields in each spec, but
                    // we expect a small max N, around 10.
                    if (std::find(newFields.begin(), newFields.end(), *field)
                        == newFields.end()) {
                        layer->_PrimSetField(id, *field, VtValue());
                    }
                }

                // Set field values.
                TF_FOR_ALL(field, newFields) {
                    VtValue newValue = newData.Get(id, *field);
                    VtValue oldValue = layer->GetField(id, *field);
                    if (oldValue != newValue) {
                        layer->_PrimSetField(id, *field, newValue, &oldValue);
                    }
                }

                return true;
            }

            virtual void Done(const SdfAbstractData&)
            {
                // Do nothing
            }

            SdfLayer* layer;
        };

        _SpecUpdater updater(this);
        newData->VisitSpecs(&updater);
    }

    // Verify that the result matches.
    // TODO Enable in debug builds.
    if (0) {
        TRACE_SCOPE("SdfLayer::_SetData - Verify result");
        TF_VERIFY(_data->Equals(newData));
    }
}

template <class T>
void
SdfLayer::_PrimSetField(const SdfAbstractDataSpecId& id, 
                        const TfToken& fieldName,
                        const T& value,
                        const VtValue *oldValuePtr,
                        bool useDelegate)
{
    // Send notification when leaving the change block.
    SdfChangeBlock block;

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetField(id, fieldName, value, oldValuePtr);
        return;
    }

    const VtValue& oldValue =
        oldValuePtr ? *oldValuePtr : GetField(id, fieldName);
    const VtValue& newValue = _GetVtValue(value);

    Sdf_ChangeManager::Get().DidChangeField(
        SdfLayerHandle(this),
        id.GetFullSpecPath(), fieldName, oldValue, newValue);

    _data->Set(id, fieldName, value);
}

template void SdfLayer::_PrimSetField(
    const SdfAbstractDataSpecId&, const TfToken&, 
    const VtValue&, const VtValue *, bool);
template void SdfLayer::_PrimSetField(
    const SdfAbstractDataSpecId&, const TfToken&, 
    const SdfAbstractDataConstValue&, const VtValue *, bool);

template <class T>
void
SdfLayer::_PrimPushChild(const SdfPath& parentPath, 
                         const TfToken& fieldName,
                         const T& value,
                         bool useDelegate)
{
    SdfAbstractDataSpecId id(&parentPath);

    if (!HasField(id, fieldName)) {
        _PrimSetField(id, fieldName,
            VtValue(std::vector<T>(1, value)));
        return;
    }

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->PushChild(parentPath, fieldName, value);
        return;
    }

    // A few efficiency notes:
    //
    // - We want to push the child onto the existing vector.  Since
    //   VtValue is copy-on-write, we avoid incurring a copy fault
    //   by retrieving the value from the data store and then
    //   erasing the field before modifying the vector.  Similarly,
    //   we swap the vector<T> out of the type-erased VtValue box,
    //   modify that, then swap it back in.
    //
    // - Do not record a field change entry with Sdf_ChangeManager.
    //   Doing so would require us to provide both the old & new
    //   values for the vector.  Note tha the the changelist protocol
    //   already has special affordances for spec add/remove events,
    //   and child fields are essentially an implementation detail.
    //
    VtValue box = _data->Get(id, fieldName);
    _data->Erase(id, fieldName);
    std::vector<T> vec;
    if (box.IsHolding<std::vector<T>>()) {
        box.Swap(vec);
    } else {
        // If the value isn't a vector, we replace it with an empty one.
    }
    vec.push_back(value);
    box.Swap(vec);
    _data->Set(id, fieldName, box);
}

template void SdfLayer::_PrimPushChild(
    const SdfPath&, const TfToken&, 
    const TfToken &, bool);
template void SdfLayer::_PrimPushChild(
    const SdfPath&, const TfToken&, 
    const SdfPath &, bool);

template <class T>
void
SdfLayer::_PrimPopChild(const SdfPath& parentPath, 
                        const TfToken& fieldName,
                        bool useDelegate)
{
    SdfAbstractDataSpecId id(&parentPath);

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        std::vector<T> vec = GetFieldAs<std::vector<T> >(id, fieldName);
        if (!vec.empty()) {
            T oldValue = vec.back();
            _stateDelegate->PopChild(parentPath, fieldName, oldValue);
        } else {
            TF_CODING_ERROR("SdfLayer::_PrimPopChild failed: field %s is "
                            "empty vector", fieldName.GetText());
        }
        return;
    }

    // See efficiency notes in _PrimPushChild().
    VtValue box = _data->Get(id, fieldName);
    _data->Erase(id, fieldName);
    if (!box.IsHolding<std::vector<T>>()) {
        TF_CODING_ERROR("SdfLayer::_PrimPopChild failed: field %s is "
                        "non-vector", fieldName.GetText());
        return;
    }
    std::vector<T> vec;
    box.Swap(vec);
    if (vec.empty()) {
        TF_CODING_ERROR("SdfLayer::_PrimPopChild failed: %s is empty",
                        fieldName.GetText());
        return;
    }
    vec.pop_back();
    box.Swap(vec);
    _data->Set(id, fieldName, box);
}

template void SdfLayer::_PrimPopChild<TfToken>(
    const SdfPath&, const TfToken&, bool);
template void SdfLayer::_PrimPopChild<SdfPath>(
    const SdfPath&, const TfToken&, bool);

template <class T>
void
SdfLayer::_PrimSetFieldDictValueByKey(const SdfAbstractDataSpecId& id,
                                      const TfToken& fieldName,
                                      const TfToken& keyPath,
                                      const T& value,
                                      const VtValue *oldValuePtr,
                                      bool useDelegate)
{
    // Send notification when leaving the change block.
    SdfChangeBlock block;

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetFieldDictValueByKey(
            id, fieldName, keyPath, value, oldValuePtr);
        return;
    }

    // This can't only use oldValuePtr currently, since we need the entire
    // dictionary, not just they key being set.  If we augment change
    // notification to be as granular as dict-key-path, we could use it.
    VtValue oldValue = GetField(id, fieldName);

    _data->SetDictValueByKey(id, fieldName, keyPath, value);

    VtValue newValue = GetField(id, fieldName);

    Sdf_ChangeManager::Get().DidChangeField(
        SdfLayerHandle(this), id.GetFullSpecPath(), fieldName,
        oldValue, newValue);
}

template void SdfLayer::_PrimSetFieldDictValueByKey(
    const SdfAbstractDataSpecId&, const TfToken&, const TfToken &,
    const VtValue&, const VtValue *, bool);
template void SdfLayer::_PrimSetFieldDictValueByKey(
    const SdfAbstractDataSpecId&, const TfToken&, const TfToken &,
    const SdfAbstractDataConstValue&, const VtValue *, bool);

bool
SdfLayer::_MoveSpec(const SdfPath &oldPath, const SdfPath &newPath)
{
    TRACE_FUNCTION();

    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot move <%s> to <%s>. Layer @%s@ is not editable.",
                        oldPath.GetText(), newPath.GetText(), 
                        GetIdentifier().c_str());
        return false;
    }

    if (oldPath.IsEmpty() || newPath.IsEmpty()) {
        TF_CODING_ERROR("Cannot move <%s> to <%s>. "
                        "Source and destination must be non-empty paths",
                        oldPath.GetText(), newPath.GetText());
        return false;
    }

    if (oldPath.HasPrefix(newPath) || newPath.HasPrefix(oldPath)) {
        TF_CODING_ERROR("Cannot move <%s> to <%s>. "
                        "Source and destination must not overlap",
                        oldPath.GetText(), newPath.GetText());
        return false;
    }

    if (!_data->HasSpec(SdfAbstractDataSpecId(&oldPath))) {
        // Cannot move; nothing at source.
        return false;
    }
    if (_data->HasSpec(SdfAbstractDataSpecId(&newPath))) {
        // Cannot move; destination exists.
        return false;
    }

    _PrimMoveSpec(oldPath, newPath);

    return true;
}

static void
_MoveSpecInternal(
    SdfAbstractDataRefPtr data, Sdf_IdentityRegistry* idReg,
    const SdfPath& oldSpecPath, 
    const SdfPath& oldRootPath, const SdfPath& newRootPath)
{
    const SdfPath newSpecPath = 
        oldSpecPath.ReplacePrefix(
            oldRootPath, newRootPath, /* fixTargets = */ false);
    
    data->MoveSpec(
        SdfAbstractDataSpecId(&oldSpecPath), 
        SdfAbstractDataSpecId(&newSpecPath));

    idReg->MoveIdentity(oldSpecPath, newSpecPath);
}

void
SdfLayer::_PrimMoveSpec(const SdfPath& oldPath, const SdfPath& newPath,
                        bool useDelegate)
{
    SdfChangeBlock block;

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->MoveSpec(oldPath, newPath);
        return;
    }

    Sdf_ChangeManager::Get().DidMoveSpec(SdfLayerHandle(this), oldPath, newPath);

    Traverse(oldPath, 
        std::bind(_MoveSpecInternal, _data, &_idRegistry, ph::_1, oldPath, newPath));
}

bool 
SdfLayer::_CreateSpec(const SdfPath& path, SdfSpecType specType, bool inert)
{
    if (specType == SdfSpecTypeUnknown) {
        return false;
    }

    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot create spec at <%s>. Layer @%s@ is not editable.",
                        path.GetText(), 
                        GetIdentifier().c_str());
        return false;
    }

    if (_data->HasSpec(SdfAbstractDataSpecId(&path))) {
        TF_CODING_ERROR(
            "Cannot create spec <%s> because it already exists in @%s@",
            path.GetText(), GetIdentifier().c_str());
        return false;
    }

    _PrimCreateSpec(path, specType, inert);

    return true;
}

bool
SdfLayer::_DeleteSpec(const SdfPath &path)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot delete <%s>. Layer @%s@ is not editable",
                        path.GetText(), 
                        GetIdentifier().c_str());
        return false;
    }

    bool inert = _IsInertSubtree(path);

    if (!HasSpec(path)) {
        return false;
    }
    
    _PrimDeleteSpec(path, inert);

    return true;
}

template<typename ChildPolicy>
void
SdfLayer::_TraverseChildren(const SdfPath &path, const TraversalFunction &func)
{
    std::vector<typename ChildPolicy::FieldType> children =
        GetFieldAs<std::vector<typename ChildPolicy::FieldType> >(
            SdfAbstractDataSpecId(&path), ChildPolicy::GetChildrenToken(path));

    TF_FOR_ALL(i, children) {
        Traverse(ChildPolicy::GetChildPath(path, *i), func);
    }
}

void
SdfLayer::Traverse(const SdfPath &path, const TraversalFunction &func)
{
    std::vector<TfToken> fields = _data->List(SdfAbstractDataSpecId(&path));
    TF_FOR_ALL(i, fields) {
        if (*i == SdfChildrenKeys->PrimChildren) {
            _TraverseChildren<Sdf_PrimChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->PropertyChildren) {
            _TraverseChildren<Sdf_PropertyChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->MapperChildren) {
            _TraverseChildren<Sdf_MapperChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->MapperArgChildren) {
            _TraverseChildren<Sdf_MapperArgChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->VariantChildren) {
            _TraverseChildren<Sdf_VariantChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->VariantSetChildren) {
            _TraverseChildren<Sdf_VariantSetChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->ConnectionChildren) {
            _TraverseChildren<Sdf_AttributeConnectionChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->RelationshipTargetChildren) {
            _TraverseChildren<Sdf_RelationshipTargetChildPolicy>(path, func);
        } else if (*i == SdfChildrenKeys->ExpressionChildren) {
            _TraverseChildren<Sdf_ExpressionChildPolicy>(path, func);
        }
    }

    func(path);
}

static void
_EraseSpecAtPath(SdfAbstractData* data, const SdfPath& path)
{
    data->EraseSpec(SdfAbstractDataSpecId(&path));
}

void
SdfLayer::_PrimDeleteSpec(const SdfPath &path, bool inert, bool useDelegate)
{
    SdfChangeBlock block;

    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->DeleteSpec(path, inert);
        return;
    }

    Sdf_ChangeManager::Get().DidRemoveSpec(SdfLayerHandle(this), path, inert);

    TraversalFunction eraseFunc = 
        std::bind(&_EraseSpecAtPath, boost::get_pointer(_data), ph::_1);
    Traverse(path, eraseFunc);
}

void
SdfLayer::_PrimCreateSpec(const SdfPath &path,
                          SdfSpecType specType, bool inert,
                          bool useDelegate)
{
    SdfChangeBlock block;
    
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->CreateSpec(path, specType, inert);
        return;
    }

    Sdf_ChangeManager::Get().DidAddSpec(SdfLayerHandle(this), path, inert);

    _data->CreateSpec(SdfAbstractDataSpecId(&path), specType);
}

bool
SdfLayer::_IsInert(const SdfPath &path, bool ignoreChildren,
                   bool requiredFieldOnlyPropertiesAreInert) const
{
    // If the spec has only the required SpecType field (stored
    // separately from other fields), then it doesn't affect the scene.
    const std::vector<TfToken> fields = ListFields(path);
    if (fields.empty()) {
        return true;
    }

    // If the spec is custom it affects the scene.
    if (GetFieldAs<bool>(path, SdfFieldKeys->Custom, false)) {
        return false;
    }

    // Special cases for determining whether a spec affects the scene.
    const SdfSpecType specType = GetSpecType(path);

    // Prims that are defs or with a specific typename always affect the scene
    // since they bring a prim into existence.
    if (specType == SdfSpecTypePrim) {
        const SdfSpecifier specifier = GetFieldAs<SdfSpecifier>(
            path, SdfFieldKeys->Specifier, SdfSpecifierOver);
        if (SdfIsDefiningSpecifier(specifier)) {
            return false;
        }
            
        const TfToken type = GetFieldAs<TfToken>(path, SdfFieldKeys->TypeName);
        if (!type.IsEmpty()) {
            return false;
        }
    }

    // If we're not considering required-field-only properties as inert, then 
    // properties should never be considered inert because they might exist to 
    // instantiate an on-demand property.
    if (!requiredFieldOnlyPropertiesAreInert &&
        (specType == SdfSpecTypeAttribute    ||
         specType == SdfSpecTypeRelationship)) {
        return false;
    }

    // Prims and properties don't affect the scene if they only contain
    // opinions about required fields.
    if (specType == SdfSpecTypePrim         ||
        specType == SdfSpecTypeAttribute    ||
        specType == SdfSpecTypeRelationship) {

        const SdfSchema::SpecDefinition* specDefinition = 
            GetSchema().GetSpecDefinition(specType);
        if (!TF_VERIFY(specDefinition)) {
            return false;
        }

        TF_FOR_ALL(field, fields) {
            // If specified, skip over prim name children and properties.
            // This is a special case to allow _IsInertSubtree to process
            // these children separately.
            if (specType == SdfSpecTypePrim && ignoreChildren) {
                if (*field == SdfChildrenKeys->PrimChildren ||
                    *field == SdfChildrenKeys->PropertyChildren) {
                    continue;
                }
            }

            if (specDefinition->IsRequiredField(*field)) {
                continue;
            }

            return false;
        }

        return true;
    }

    return false;
}

bool
SdfLayer::_IsInertSubtree(const SdfPath &path)
{
    if (!_IsInert(path, true /*ignoreChildren*/, 
                  true /* requiredFieldOnlyPropertiesAreInert */)) {
        return false;
    }

    if (path.IsPrimPath()) {
        std::vector<TfToken> prims = GetFieldAs<std::vector<TfToken> >(
            path, SdfChildrenKeys->PrimChildren);
        TF_FOR_ALL(i, prims) {
            if (!_IsInertSubtree(path.AppendChild(*i))) {
                return false;
            }
        }
        
        std::vector<TfToken> properties = GetFieldAs<std::vector<TfToken> >(
            path, SdfChildrenKeys->PropertyChildren);
        TF_FOR_ALL(i, properties) {
            if (!_IsInert(path.AppendProperty(*i), 
                          false /*ignoreChildren*/, 
                          true /* requiredFieldOnlyPropertiesAreInert */)) {

                return false;
            }
        }
    }
    return true;
}

bool
SdfLayer::ExportToString( std::string *result ) const
{
    TRACE_FUNCTION();

    TF_DESCRIBE_SCOPE("Writing layer @%s@", GetIdentifier().c_str());

    return GetFileFormat()->WriteToString(this, result);
}

bool 
SdfLayer::_WriteToFile(const string & newFileName, 
                       const string &comment, 
                       SdfFileFormatConstPtr fileFormat,
                       const FileFormatArguments& args) const
{
    TRACE_FUNCTION();

    TF_DESCRIBE_SCOPE("Writing layer @%s@", GetIdentifier().c_str());

    if (newFileName.empty())
        return false;
        
    if ((newFileName == GetRealPath()) && !PermissionToSave()) {
        TF_RUNTIME_ERROR("Cannot save layer @%s@, saving not allowed", 
                    newFileName.c_str());
        return false;
    }

    // If a file format was explicitly provided, use that regardless of the 
    // file extension, else discover the file format from the file extension.
    if (!fileFormat) {
        const string ext = Sdf_GetExtension(newFileName);
        if (!ext.empty()) 
            fileFormat = SdfFileFormat::FindByExtension(ext);

        if (!fileFormat) {
            // Some parts of the system generate temp files
            // with garbage extensions, furthermore we do not restrict
            // users from writing to arbitrary file names, so here we must fall
            // back to the current file format associated with the layer.
            fileFormat = GetFileFormat();
        }
    }

    // Disallow saving or exporting package layers via the Sdf API.
    if (Sdf_IsPackageOrPackagedLayer(fileFormat, newFileName)) {
        TF_CODING_ERROR("Cannot save layer @%s@: writing %s %s layer "
                        "is not allowed through this API.",
                        newFileName.c_str(), 
                        fileFormat->IsPackage() ? "package" : "packaged",
                        fileFormat->GetFormatId().GetText());
        return false;
    }

    if (!TF_VERIFY(fileFormat)) {
        TF_RUNTIME_ERROR("Unknown file format when attempting to write '%s'",
            newFileName.c_str());
        return false;
    }

    string layerDir = TfGetPathName(newFileName);
    if (!(layerDir.empty() || TfIsDir(layerDir) || TfMakeDirs(layerDir))) {
        TF_RUNTIME_ERROR(
            "Cannot create destination directory %s",
            layerDir.c_str());
        return false;
    }
    
    bool ok = fileFormat->WriteToFile(this, newFileName, comment, args);

    // If we wrote to the backing file then we're now clean.
    if (ok && newFileName == GetRealPath())
       _MarkCurrentStateAsClean();

    return ok;
}

bool 
SdfLayer::Export(const string& newFileName, const string& comment,
                 const FileFormatArguments& args) const
{
    return _WriteToFile(newFileName, comment, TfNullPtr, args);
}

bool
SdfLayer::Save(bool force) const
{
    return _Save(force);
}

bool
SdfLayer::_Save(bool force) const
{
    TRACE_FUNCTION();

    if (IsMuted()) {
        TF_CODING_ERROR("Cannot save muted layer @%s@",
                        GetIdentifier().c_str());
        return false;
    }

    if (IsAnonymous()) {
        TF_CODING_ERROR("Cannot save anonymous layer @%s@",
            GetIdentifier().c_str());
        return false;
    }

    string path(GetRealPath());
    if (path.empty())
        return false;

    // Skip saving if the file exists and the layer is clean.
    if (!force && !IsDirty() && TfPathExists(path))
        return true;

    if (!_WriteToFile(path, std::string(), 
                      GetFileFormat(), GetFileFormatArguments()))
        return false;

    // Record modification timestamp.
    VtValue timestamp = ArGetResolver().GetModificationTimestamp(
        GetIdentifier(), path);
    if (timestamp.IsEmpty()) {
        TF_CODING_ERROR(
            "Unable to get modification timestamp for '%s (%s)'",
            GetIdentifier().c_str(), path.c_str());
        return false;
    }
    _assetModificationTime.Swap(timestamp);

    SdfNotice::LayerDidSaveLayerToFile().Send(SdfCreateNonConstHandle(this));

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
