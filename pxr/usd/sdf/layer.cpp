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
#include "pxr/usd/sdf/layerUtils.h"
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
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <tbb/queuing_rw_mutex.h>

#include <atomic>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using std::map;
using std::set;
using std::string;
using std::vector;

namespace ph = std::placeholders;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    SDF_LAYER_VALIDATE_AUTHORING, false,
    "If enabled, layers will validate new fields and specs being authored "
    "against their schema. If the field or spec is not defined in the schema "
    "a coding error will be issued and the authoring operation will fail.");

TF_DEFINE_ENV_SETTING(
    SDF_LAYER_INCLUDE_DETACHED, "",
    R"("Set the default include patterns for specifying detached layers. "
       "This can be set to a comma-delimited list of strings or "*" to "
       "include all layers.")");

TF_DEFINE_ENV_SETTING(
    SDF_LAYER_EXCLUDE_DETACHED, "",
    R"("Set the default exclude patterns for specifying detached layers. "
       "This can be set to a comma-delimited list of strings.")");

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfLayer>();
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

// Specifies detached layers
TF_MAKE_STATIC_DATA(SdfLayer::DetachedLayerRules, _detachedLayerRules)
{
    const std::vector<std::string> includePatterns = TfStringSplit(
        TfGetEnvSetting(SDF_LAYER_INCLUDE_DETACHED), ",");
    if (std::find(includePatterns.begin(), includePatterns.end(), "*") !=
        includePatterns.end()) {
        _detachedLayerRules->IncludeAll();
    }
    else if (!includePatterns.empty()) {
        _detachedLayerRules->Include(includePatterns);
    }

    const std::vector<std::string> excludePatterns = TfStringSplit(
        TfGetEnvSetting(SDF_LAYER_EXCLUDE_DETACHED), ",");
    if (!excludePatterns.empty()) {
        _detachedLayerRules->Exclude(excludePatterns);
    }
}

// A registry for loaded layers.
static TfStaticData<Sdf_LayerRegistry> _layerRegistry;

// Global mutex protecting _layerRegistry.
static tbb::queuing_rw_mutex &
_GetLayerRegistryMutex() {
    static tbb::queuing_rw_mutex mutex;
    return mutex;
}

static SdfAbstractDataRefPtr
_CreateDataForFileFormat(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier,
    const SdfLayer::FileFormatArguments& args)
{
    return SdfLayer::IsIncludedByDetachedLayerRules(identifier) ?
        fileFormat->InitDetachedData(args) : fileFormat->InitData(args);
}

SdfLayer::SdfLayer(
    const SdfFileFormatConstPtr &fileFormat,
    const string &identifier,
    const string &realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments &args,
    bool validateAuthoring)
    : _self(this)
    , _fileFormat(fileFormat)
    , _fileFormatArgs(args)
    , _schema(fileFormat->GetSchema())
    , _idRegistry(SdfLayerHandle(this))
    , _data(_CreateDataForFileFormat(fileFormat, identifier, args))
    , _stateDelegate(SdfSimpleLayerStateDelegate::New())
    , _lastDirtyState(false)
    , _assetInfo(new Sdf_AssetInfo)
    , _mutedLayersRevisionCache(0)
    , _isMutedCache(false)
    , _permissionToEdit(true)
    , _permissionToSave(true)
    , _validateAuthoring(
        validateAuthoring || TfGetEnvSetting<bool>(SDF_LAYER_VALIDATE_AUTHORING))
    , _hints{/*.mightHaveRelocates =*/ false}
{
    TF_DEBUG(SDF_LAYER).Msg("SdfLayer::SdfLayer('%s', '%s')\n",
        identifier.c_str(), realPath.c_str());

    // If the identifier has the anonymous layer identifier prefix, it is a
    // template into which the layer address must be inserted. This ensures
    // that anonymous layers have unique identifiers, and can be referenced by
    // Sd object reprs.
    string layerIdentifier = Sdf_IsAnonLayerIdentifier(identifier) ?
        Sdf_ComputeAnonLayerIdentifier(identifier, this) : identifier;

    // Indicate that this layer's initialization is not yet complete before we
    // publish this object (i.e. add it to the registry in
    // _InitializeFromIdentifier).  This ensures that other threads looking for
    // this layer will block until it is fully initialized.
    _initializationComplete = false;

    // Initialize layer asset information.
    _InitializeFromIdentifier(
        layerIdentifier, realPath, std::string(), assetInfo);

    // A new layer is not dirty.
    _MarkCurrentStateAsClean();
}

SdfLayer::~SdfLayer()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

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
    _layerRegistry->Erase(_self);
}

const SdfFileFormatConstPtr&
SdfLayer::GetFileFormat() const
{
    return _fileFormat;
}

const SdfLayer::FileFormatArguments& 
SdfLayer::GetFileFormatArguments() const
{
    return _fileFormatArgs;
}

SdfLayerRefPtr
SdfLayer::_CreateNewWithFormat(
    const SdfFileFormatConstPtr &fileFormat,
    const string& identifier,
    const string& realPath,
    const ArAssetInfo& assetInfo,
    const FileFormatArguments& args)
{
    // This method should be called with the layerRegistryMutex already held.

    // Create and return a new layer with _initializationComplete set false.
    return fileFormat->NewLayer(
        fileFormat, identifier, realPath, assetInfo, args);
}

void
SdfLayer::_FinishInitialization(bool success)
{
    _initializationWasSuccessful = success;
    _initializationComplete = true; // unblock waiters.
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

    // Wait until _initializationComplete is set to true.  If the layer is still
    // being initialized, this will be false, blocking progress until
    // initialization completes.
    while (!_initializationComplete) {
        // XXX: Disabled for now due to intermittent crashes.
        //_initDispatcher.Wait();
        std::this_thread::yield();
    }

    // For various reasons, initialization may have failed.
    // For example, the sdf text format parser may have hit a syntax error,
    // or transferring content from a source layer may have failed.
    // In this case _initializationWasSuccessful will be set to false.
    // The callers of this method are responsible for checking the result
    // and dropping any references they hold.  As a convenience to them,
    // we return the value here.
    return _initializationWasSuccessful.get();
}

static bool
_ModificationTimesEqual(const VtValue& v1, const VtValue& v2)
{
    if (!v1.IsHolding<ArTimestamp>() || !v2.IsHolding<ArTimestamp>()) {
        return false;
    }

    const ArTimestamp& t1 = v1.UncheckedGet<ArTimestamp>();
    const ArTimestamp& t2 = v2.UncheckedGet<ArTimestamp>();
    return t1.IsValid() && t2.IsValid() && t1 == t2;
}

static bool
_ModificationTimesEqual(const VtDictionary& t1, const VtDictionary& t2)
{
    if (t1.size() != t2.size()) {
        return false;
    }

    for (const auto& e1 : t1) {
        const auto t2Iter = t2.find(e1.first);
        if (t2Iter == t2.end() || 
            !_ModificationTimesEqual(e1.second, t2Iter->second)) {
            return false;
        }
    }
    
    return true;
}

SdfLayerRefPtr
SdfLayer::CreateAnonymous(
    const string& tag, const FileFormatArguments& args)
{
    SdfFileFormatConstPtr fileFormat;
    string suffix = TfStringGetSuffix(tag);
    if (!suffix.empty()) {
        fileFormat = SdfFileFormat::FindByExtension(suffix, args);
    }

    if (!fileFormat) {
        fileFormat = SdfFileFormat::FindById(SdfTextFileFormatTokens->Id);
    }

    if (!fileFormat) {
        TF_CODING_ERROR("Cannot determine file format for anonymous SdfLayer");
        return SdfLayerRefPtr();
    }

    return _CreateAnonymousWithFormat(fileFormat, tag, args);
}

SdfLayerRefPtr
SdfLayer::CreateAnonymous(
    const string &tag, const SdfFileFormatConstPtr &format,
    const FileFormatArguments &args)
{
    if (!format) {
        TF_CODING_ERROR("Invalid file format for anonymous SdfLayer");
        return SdfLayerRefPtr();
    }

    return _CreateAnonymousWithFormat(format, tag, args);
}

SdfLayerRefPtr
SdfLayer::_CreateAnonymousWithFormat(
    const SdfFileFormatConstPtr &fileFormat, const std::string& tag,
    const FileFormatArguments &args)
{
    if (fileFormat->IsPackage()) {
        TF_CODING_ERROR("Cannot create anonymous layer: creating package %s "
                        "layer is not allowed through this API.",
                        fileFormat->GetFormatId().GetText());
        return SdfLayerRefPtr();
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();
    tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

    SdfLayerRefPtr layer =
        _CreateNewWithFormat(
            fileFormat, Sdf_GetAnonLayerIdentifierTemplate(tag), 
            string(), ArAssetInfo(), args);

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
    const FileFormatArguments &args)
{
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::CreateNew('%s', '%s')\n",
        identifier.c_str(), TfStringify(args).c_str());

    return _CreateNew(TfNullPtr, identifier, args);
}

SdfLayerRefPtr
SdfLayer::CreateNew(
    const SdfFileFormatConstPtr& fileFormat,
    const string& identifier,
    const FileFormatArguments &args)
{
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::CreateNew('%s', '%s', '%s')\n",
        fileFormat->GetFormatId().GetText(), 
        identifier.c_str(), TfStringify(args).c_str());

    return _CreateNew(fileFormat, identifier, args);
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

    SdfLayer::FileFormatArguments::iterator targetIt = 
        args.find(SdfFileFormatTokens->TargetArg);
    if (targetIt != args.end()) {
        if (fileFormat->IsPrimaryFormatForExtensions()) {
            // If the file format plugin being used to open the indicated layer
            // is the primary plugin for layers of that type, it means the 
            // 'target' argument (if any) had no effect and can be stripped 
            // from the arguments.
            args.erase(targetIt);
        }
        else {
            // The target argument may have been a comma-delimited list of
            // targets to use. The canonical arguments should contain just
            // the target for the file format for this layer so that subsequent
            // lookups using the same target return the same layer. For example,
            // a layer opened with target="x" and target="x,y" should return
            // the same layer.
            targetIt->second = fileFormat->GetTarget().GetString();
        }
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

SdfLayerRefPtr
SdfLayer::_CreateNew(
    SdfFileFormatConstPtr fileFormat,
    const string& identifier,
    const FileFormatArguments &args,
    bool saveLayer)
{
    string whyNot;
    if (!Sdf_CanCreateNewLayerWithIdentifier(identifier, &whyNot)) {
        TF_CODING_ERROR("Cannot create new layer '%s': %s",
            identifier.c_str(),
            whyNot.c_str());
        return TfNullPtr;
    }

    ArResolver& resolver = ArGetResolver();

    ArAssetInfo assetInfo;
    string absIdentifier, localPath;
    {
        TfErrorMark m;
        absIdentifier = resolver.CreateIdentifierForNewAsset(identifier);

        // Resolve the identifier to the path where new assets should go.
        localPath = resolver.ResolveForNewAsset(absIdentifier);

        if (!m.IsClean()) {
            std::vector<std::string> errors;
            for (const TfError& error : m) {
                errors.push_back(error.GetCommentary());
            }
            whyNot = TfStringJoin(errors, ", ");
            m.Clear();
        }
    }

    if (localPath.empty()) {
        TF_CODING_ERROR(
            "Cannot create new layer '%s': %s",
            absIdentifier.c_str(), 
            (whyNot.empty() ? "failed to compute path for new layer" 
                : whyNot.c_str()));
        return TfNullPtr;
    }

    // If not explicitly supplied one, try to determine the fileFormat 
    // based on the local path suffix,
    if (!fileFormat) {
        fileFormat = SdfFileFormat::FindByExtension(localPath, args);
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

    // Canonicalize any file format arguments passed in.
    FileFormatArguments finalArgs(args);
    _CanonicalizeFileFormatArguments(absIdentifier, fileFormat, finalArgs);

    // If a file format target is included in the arguments, it must be
    // included in the identifier of the new layer. This ensures that
    // FindOrOpen will find these layers if given the same target.
    //
    // All other arguments are currently assumed to contribute to how
    // the file format creates the new layer but not to the identity
    // of the layer.
    auto targetArgIt = finalArgs.find(SdfFileFormatTokens->TargetArg);
    if (targetArgIt != finalArgs.end()) {
        absIdentifier = Sdf_CreateIdentifier(
            absIdentifier, FileFormatArguments{*targetArgIt});
    }

    // In case of failure below, we want to release the layer
    // registry mutex lock before destroying the layer.
    SdfLayerRefPtr layer;
    {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());

        // Check for existing layer with this identifier.
        if (_layerRegistry->Find(absIdentifier)) {
            TF_CODING_ERROR("A layer already exists with identifier '%s'",
                absIdentifier.c_str());
            return TfNullPtr;
        }

        layer = _CreateNewWithFormat(
            fileFormat, absIdentifier, localPath, ArAssetInfo(), finalArgs);

        if (!TF_VERIFY(layer)) {
            return TfNullPtr;
        }

        if (saveLayer) {
            // Stash away the existing layer hints.  The call to _Save below
            // will invalidate them but they should still be good.
            SdfLayerHints hints = layer->_hints;

            // XXX 2011-08-19 Newly created layers should not be
            // saved to disk automatically.
            //
            // Force the save here to ensure this new layer overwrites any
            // existing layer on disk.
            if (!layer->_Save(/* force = */ true)) {
                // Dropping the layer reference will destroy it, and
                // the destructor will remove it from the registry.
                return TfNullPtr;
            }

            layer->_hints = hints;
        }

        // Once we have saved the layer, initialization is complete.
        layer->_FinishInitialization(/* success = */ true);
    }

    return layer;
}

SdfLayerRefPtr
SdfLayer::New(
    const SdfFileFormatConstPtr& fileFormat,
    const string& identifier,
    const FileFormatArguments& args)
{
    return _CreateNew(fileFormat, identifier, args, /* saveLayer = */ false);
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

    // Path to the layer. If the layer is an anonymous layer, this
    // will be the anonymous layer identifier.
    string layerPath;

    // Resolved path for the layer. If the layer is an anonymous layer,
    // this will be empty.
    ArResolvedPath resolvedLayerPath;

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

    ArAssetInfo assetInfo;
    ArResolvedPath resolvedLayerPath;
    const bool isAnonymous = IsAnonymousLayerIdentifier(layerPath);
    if (!isAnonymous) {
        layerPath = ArGetResolver().CreateIdentifier(layerPath);
        resolvedLayerPath = Sdf_ResolvePath(
            layerPath, computeAssetInfo ? &assetInfo : nullptr);
    }

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

    info->fileFormat = SdfFileFormat::FindByExtension(
        resolvedLayerPath.empty() ? layerPath : resolvedLayerPath, layerArgs);
    info->fileFormatArgs.swap(_CanonicalizeFileFormatArguments(
        layerPath, info->fileFormat, layerArgs));

    info->isAnonymous = isAnonymous;
    info->layerPath.swap(layerPath);
    info->resolvedLayerPath = std::move(resolvedLayerPath);
    info->identifier = Sdf_CreateIdentifier(
        info->layerPath, info->fileFormatArgs);
    swap(info->assetInfo, assetInfo);
    return true;
}

template <class ScopedLock>
SdfLayerRefPtr
SdfLayer::_TryToFindLayer(const string &identifier,
                          const ArResolvedPath &resolvedPath,
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
    // such a layer by identifier in the registry and the format doesn't specify
    // that anonymous layers should still be read, we're done since we don't
    // have an asset to open.
    if (layerInfo.isAnonymous) {
        if (!layerInfo.fileFormat || 
            !layerInfo.fileFormat->ShouldReadAnonymousLayers()) {
            return TfNullPtr;
        }
    }
    else if (layerInfo.resolvedLayerPath.empty()) {
        return TfNullPtr;
    }

    // Isolate.
    return WorkWithScopedParallelism([&]() -> SdfLayerRefPtr {
        
        // Otherwise we create the layer and insert it into the registry.
        return _OpenLayerAndUnlockRegistry(lock, layerInfo,
                                           /* metadataOnly */ false);
    });
}

/* static */
SdfLayerRefPtr
SdfLayer::FindOrOpenRelativeToLayer(
    const SdfLayerHandle &anchor,
    const string &identifier,
    const FileFormatArguments &args)
{
    TRACE_FUNCTION();

    if (!anchor) {
        TF_CODING_ERROR("Anchor layer is invalid");
        return TfNullPtr;
    }

    // For consistency with FindOrOpen, we silently bail out if identifier
    // is empty here to avoid the coding error that is emitted in that case
    // in SdfComputeAssetPathRelativeToLayer.
    if (identifier.empty()) {
        return TfNullPtr;
    }

    return FindOrOpen(
        SdfComputeAssetPathRelativeToLayer(anchor, identifier), args);
}

/* static */
SdfLayerRefPtr
SdfLayer::OpenAsAnonymous(
    const std::string &layerPath,
    bool metadataOnly,
    const std::string &tag)
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

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
        // either success or failure in order to unblock others
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
    return _schema;
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

    const bool isAnonymous = IsAnonymous();

    SdfChangeBlock block;
    if (isAnonymous && GetFileFormat()->ShouldSkipAnonymousReload()) {
        // Different file formats have different policies for reloading
        // anonymous layers.  Some want to treat it as a noop, others want to
        // treat it as 'Clear'.
        //
        // XXX: in the future, I think we want FileFormat plugins to
        // have a Reload function.  The plugin can manage when it needs to
        // reload data appropriately.
        return _ReloadSkipped;
    }
    else if (IsMuted() || 
             (isAnonymous && !GetFileFormat()->ShouldReadAnonymousLayers())) {
        // Reloading a muted layer leaves it with the initialized contents.
        SdfAbstractDataRefPtr initialData = _CreateData();
        if (_data->Equals(initialData)) {
            return _ReloadSkipped;
        }
        _SetData(initialData);
    }
    else if (isAnonymous) {
        // Ask the current external asset dependency state.
        VtDictionary externalAssetTimestamps = 
            Sdf_ComputeExternalAssetModificationTimestamps(*this);

        // See if we can skip reloading.
        if (!force && !IsDirty()
            && (externalAssetTimestamps == _externalAssetModificationTimes)) {
            return _ReloadSkipped;
        }

        if (!_Read(identifier, ArResolvedPath(), /* metadataOnly = */ false)) {
            return _ReloadFailed;
        }

        _externalAssetModificationTimes = std::move(externalAssetTimestamps);
    } else {
        // The physical location of the file may have changed since
        // the last load, so re-resolve the identifier.
        const ArResolvedPath oldResolvedPath = GetResolvedPath();
        UpdateAssetInfo();
        const ArResolvedPath resolvedPath = GetResolvedPath();

        // If asset resolution in UpdateAssetInfo failed, we may end
        // up with an empty real path, and cannot reload the layer.
        if (resolvedPath.empty()) {
            TF_RUNTIME_ERROR(
                "Cannot determine resolved path for '%s', skipping reload.",
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
        VtValue timestamp = Sdf_ComputeLayerModificationTimestamp(*this);

        // Ask the current external asset dependency state.
        VtDictionary externalAssetTimestamps = 
            Sdf_ComputeExternalAssetModificationTimestamps(*this);

        // See if we can skip reloading.
        if (!force && !IsDirty()
            && (resolvedPath == oldResolvedPath)
            && (_ModificationTimesEqual(timestamp, _assetModificationTime))
            && (_ModificationTimesEqual(
                    externalAssetTimestamps, _externalAssetModificationTimes))){
            return _ReloadSkipped;
        }

        if (!_Read(GetIdentifier(), resolvedPath, /* metadataOnly = */ false)) {
            return _ReloadFailed;
        }

        _assetModificationTime.Swap(timestamp);
        _externalAssetModificationTimes = std::move(externalAssetTimestamps);

        if (resolvedPath != oldResolvedPath) {
            Sdf_ChangeManager::Get().DidChangeLayerResolvedPath(_self);
        }
    }

    _MarkCurrentStateAsClean();

    Sdf_ChangeManager::Get().DidReloadLayerContent(_self);

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
    const ArResolvedPath filePath = Sdf_ResolvePath(layerPath);
    if (filePath.empty()) {
        return false;
    }

    return _Read(layerPath, filePath, /* metadataOnly = */ false);
}

bool
SdfLayer::ImportFromString(const std::string &s)
{
    return GetFileFormat()->ReadFromString(this, s);
}

bool
SdfLayer::_Read(
    const string& identifier,
    const ArResolvedPath& resolvedPathIn,
    bool metadataOnly)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("SdfLayer::_Read");

    // This is in support of specialized file formats that piggyback
    // on anonymous layer functionality. If the layer is anonymous,
    // pass the layer identifier to the reader, otherwise, pass the
    // resolved path of the layer.
    std::string resolvedPath;
    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        std::string args;
        Sdf_SplitIdentifier(identifier, &resolvedPath, &args);
    }
    else {
        resolvedPath = resolvedPathIn;
    }

    TF_DESCRIBE_SCOPE("Loading layer '%s'", resolvedPath.c_str());
    TF_DEBUG(SDF_LAYER).Msg(
        "SdfLayer::_Read('%s', '%s', metadataOnly=%s)\n",
        identifier.c_str(), resolvedPathIn.GetPathString().c_str(),
        TfStringify(metadataOnly).c_str());

    SdfFileFormatConstPtr format = GetFileFormat();
    if (!format->SupportsReading()) {
        TF_CODING_ERROR("Cannot read layer @%s@: %s file format does not"
                        "support reading",
                        identifier.c_str(),
                        format->GetFormatId().GetText());
        return false;
    }
    return IsIncludedByDetachedLayerRules(identifier) ?
        format->ReadDetached(this, resolvedPath, metadataOnly) :
        format->Read(this, resolvedPath, metadataOnly);
}

/*static*/
SdfLayerHandle
SdfLayer::Find(const string &identifier,
               const FileFormatArguments &args)
{
    TRACE_FUNCTION();

    tbb::queuing_rw_mutex::scoped_lock lock;
    return _Find(identifier, args, lock, /* retryAsWriter = */ false);
}

template <class ScopedLock>
SdfLayerRefPtr
SdfLayer::_Find(const string &identifier,
                const FileFormatArguments &args,
                ScopedLock& lock,
                bool retryAsWriter)
{
    // Drop the GIL here, since python identity object management may be invoked
    // when we convert the weakptr to refptr in _TryToFindLayer().
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _FindOrOpenLayerInfo layerInfo;
    if (!_ComputeInfoToFindOrOpenLayer(identifier, args, &layerInfo)) {
        return TfNullPtr;
    }

    // First see if this layer is already present.
    lock.acquire(_GetLayerRegistryMutex(), /*write=*/false);
    if (SdfLayerRefPtr layer = _TryToFindLayer(
            layerInfo.identifier, layerInfo.resolvedLayerPath,
            lock, retryAsWriter)) {
        return layer->_WaitForInitializationAndCheckIfSuccessful() ?
            layer : TfNullPtr;
    }
    return TfNullPtr;
}

/* static */
SdfLayerHandle
SdfLayer::FindRelativeToLayer(
    const SdfLayerHandle &anchor,
    const string &identifier,
    const FileFormatArguments &args)
{
    TRACE_FUNCTION();

    if (!anchor) {
        TF_CODING_ERROR("Anchor layer is invalid");
        return TfNullPtr;
    }

    // For consistency with FindOrOpen, we silently bail out if identifier
    // is empty here to avoid the coding error that is emitted in that case
    // in SdfComputeAssetPathRelativeToLayer.
    if (identifier.empty()) {
        return TfNullPtr;
    }

    return Find(
        SdfComputeAssetPathRelativeToLayer(anchor, identifier), args);
}

std::set<double>
SdfLayer::ListAllTimeSamples() const
{
    return _data->ListAllTimeSamples();
}

std::set<double> 
SdfLayer::ListTimeSamplesForPath(const SdfPath& path) const
{
    return _data->ListTimeSamplesForPath(path);
}

bool 
SdfLayer::GetBracketingTimeSamples(double time, double* tLower, double* tUpper)
{
    return _data->GetBracketingTimeSamples(time, tLower, tUpper);
}

size_t 
SdfLayer::GetNumTimeSamplesForPath(const SdfPath& path) const
{
    return _data->GetNumTimeSamplesForPath(path);
}

bool 
SdfLayer::GetBracketingTimeSamplesForPath(const SdfPath& path, 
                                          double time,
                                          double* tLower, double* tUpper)
{
    return _data->GetBracketingTimeSamplesForPath(path, time, tLower, tUpper);
}

bool 
SdfLayer::QueryTimeSample(const SdfPath& path, double time, 
                          VtValue *value) const
{
    return _data->QueryTimeSample(path, time, value);
}

bool 
SdfLayer::QueryTimeSample(const SdfPath& path, double time, 
                          SdfAbstractDataValue *value) const
{
    return _data->QueryTimeSample(path, time, value);
}

static TfType
_GetExpectedTimeSampleValueType(
    const SdfLayer& layer, const SdfPath& path)
{
    const SdfSpecType specType = layer.GetSpecType(path);
    if (specType == SdfSpecTypeUnknown) {
        TF_CODING_ERROR("Cannot set time sample at <%s> since spec does "
                        "not exist", path.GetText());
        return TfType();
    }
    else if (specType != SdfSpecTypeAttribute &&
             specType != SdfSpecTypeRelationship) {
        TF_CODING_ERROR("Cannot set time sample at <%s> because spec "
                        "is not an attribute or relationship",
                        path.GetText());
        return TfType();
    }

    TfType valueType;
    TfToken valueTypeName;
    if (specType == SdfSpecTypeRelationship) {
        static const TfType pathType = TfType::Find<SdfPath>();
        valueType = pathType;
    }
    else if (layer.HasField(path, SdfFieldKeys->TypeName, &valueTypeName)) {
        valueType = layer.GetSchema().FindType(valueTypeName).GetType();
    }

    if (!valueType) {
        TF_CODING_ERROR("Cannot determine value type for <%s>",
                        path.GetText());
    }
    
    return valueType;
}

void 
SdfLayer::SetTimeSample(const SdfPath& path, double time, 
                        const VtValue & value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    // circumvent type checking if setting a block.
    if (value.IsHolding<SdfValueBlock>()) {
        _PrimSetTimeSample(path, time, value);
        return;
    }

    const TfType expectedType = _GetExpectedTimeSampleValueType(*this, path);
    if (!expectedType) {
        // Error already emitted, just bail.
        return;
    }
    
    if (value.GetType() == expectedType) {
        _PrimSetTimeSample(path, time, value);
    }
    else {
        const VtValue castValue = 
            VtValue::CastToTypeid(value, expectedType.GetTypeid());
        if (castValue.IsEmpty()) {
            TF_CODING_ERROR("Can't set time sample on <%s> to %s: "
                            "expected a value of type \"%s\"",
                            path.GetText(),
                            TfStringify(value).c_str(),
                            expectedType.GetTypeName().c_str());
            return;
        }

        _PrimSetTimeSample(path, time, castValue);
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
SdfLayer::SetTimeSample(const SdfPath& path, double time, 
                        const SdfAbstractDataConstValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (value.valueType == _GetSdfValueBlockType().GetTypeid()) {
        _PrimSetTimeSample(path, time, value);
        return;
    }

    const TfType expectedType = _GetExpectedTimeSampleValueType(*this, path);
    if (!expectedType) {
        // Error already emitted, just bail.
        return;
    }

    if (TfSafeTypeCompare(value.valueType, expectedType.GetTypeid())) {
        _PrimSetTimeSample(path, time, value);
    }
    else {
        VtValue tmpValue;
        value.GetValue(&tmpValue);

        const VtValue castValue = 
            VtValue::CastToTypeid(tmpValue, expectedType.GetTypeid());
        if (castValue.IsEmpty()) {
            TF_CODING_ERROR("Can't set time sample on <%s> to %s: "
                            "expected a value of type \"%s\"",
                            path.GetText(),
                            TfStringify(tmpValue).c_str(),
                            expectedType.GetTypeName().c_str());
            return;
        }

        _PrimSetTimeSample(path, time, castValue);
    }
}

void 
SdfLayer::EraseTimeSample(const SdfPath& path, double time)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set time sample on <%s>.  "
                        "Layer @%s@ is not editable.", 
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }
    if (!HasSpec(path)) {
        TF_CODING_ERROR("Cannot SetTimeSample at <%s> since spec does "
                        "not exist", path.GetText());
        return;
    }

    if (!QueryTimeSample(path, time)) {
        // No time sample to remove.
        return;
    }

    _PrimSetTimeSample(path, time, VtValue());
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
SdfLayer::_PrimSetTimeSample(const SdfPath& path, double time,
                             const T& value,
                             bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetTimeSample(path, time, value);
        return;
    }

    SdfChangeBlock block;

    // TODO(USD):optimization: Analyze the affected time interval.
    Sdf_ChangeManager::Get().DidChangeAttributeTimeSamples(_self, path);

    // XXX: Should modify SetTimeSample API to take an
    //      SdfAbstractDataConstValue instead of (or along with) VtValue.
    const VtValue& valueToSet = _GetVtValue(value);
    _data->SetTimeSample(path, time, valueToSet);
}

template void SdfLayer::_PrimSetTimeSample(
    const SdfPath&, double, 
    const VtValue&, bool);
template void SdfLayer::_PrimSetTimeSample(
    const SdfPath&, double, 
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

    // Compute layer asset information from the identifier.
    std::unique_ptr<Sdf_AssetInfo> newInfo(
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
    ArResolvedPath oldResolvedPath = _assetInfo->resolvedPath;
    _assetInfo.swap(newInfo);

    // Update layer state delegate.
    if (TF_VERIFY(_stateDelegate)) {
        _stateDelegate->_SetLayer(_self);
    }

    // Update the layer registry before sending notices.
    _layerRegistry->InsertOrUpdate(_self);

    // Only send a notice if the identifier has changed (this notice causes
    // mass invalidation. See http://bug/33217). If the old identifier was
    // empty, this is a newly constructed layer, so don't send the notice.
    if (!oldIdentifier.empty()) {
        SdfChangeBlock block;
        if (oldIdentifier != GetIdentifier()) {
            Sdf_ChangeManager::Get().DidChangeLayerIdentifier(
                _self, oldIdentifier);
        }
        if (oldResolvedPath != GetResolvedPath()) {
            Sdf_ChangeManager::Get().DidChangeLayerResolvedPath(_self);
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
        return _schema.GetFallback(key).Get<T>();
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
    // If there is an authored value for timeCodesPerSecond, return that.
    VtValue value;
    if (HasField(
            SdfPath::AbsoluteRootPath(),
            SdfFieldKeys->TimeCodesPerSecond,
            &value)) {
        return value.Get<double>();
    }

    // Otherwise return framesPerSecond as a dynamic fallback.  This allows
    // layers to lock framesPerSecond and timeCodesPerSecond together by
    // specifying only framesPerSecond.
    //
    // If neither field has an authored value, this will return 24, which is the
    // final fallback value for both fields.
    return GetFramesPerSecond();
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

VtDictionary
SdfLayer::GetExpressionVariables() const
{
    return _GetValue<VtDictionary>(SdfFieldKeys->ExpressionVariables);
}

void
SdfLayer::SetExpressionVariables(const VtDictionary& dict)
{
    _SetValue(SdfFieldKeys->ExpressionVariables, dict);
}

bool 
SdfLayer::HasExpressionVariables() const
{
    return HasField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->ExpressionVariables);
}

void 
SdfLayer::ClearExpressionVariables()
{
    EraseField(SdfPath::AbsoluteRootPath(), SdfFieldKeys->ExpressionVariables);
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
    return SdfSubLayerProxy(
        std::make_unique<Sdf_SubLayerListEditor>(_self), SdfListOpTypeOrdered);
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
    SdfPath const *absPath = &path;
    if (ARCH_UNLIKELY(!path.IsAbsolutePath() || path.ContainsTargetPath())) {
        *canonicalPath = path.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
        absPath = canonicalPath;
    }
    // Grab the object type stored in the SdfData hash table. If no type has
    // been set, this path doesn't point to a valid location.
    *specType = GetSpecType(*absPath);
    return *specType != SdfSpecTypeUnknown;
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

    if (ARCH_UNLIKELY(!canonicalPath.IsEmpty())) {
        return SdfHandle<Spec>(_idRegistry.Identify(canonicalPath));
    }
    return SdfHandle<Spec>(_idRegistry.Identify(path));
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

    if (ARCH_UNLIKELY(!canonicalPath.IsEmpty())) {
        return SdfSpecHandle(_idRegistry.Identify(canonicalPath));
    }
    return SdfSpecHandle(_idRegistry.Identify(path));
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
        Sdf_CanWriteLayerToPath(GetResolvedPath());
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
    if (!edits.Process(NULL,
                       std::bind(&_HasObjectAtPath, _self, ph::_1),
                       std::bind(&_CanEdit, _self, ph::_1, ph::_2),
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
    SdfNamespaceEditVector final;
    if (!edits.Process(&final,
                       std::bind(&_HasObjectAtPath, _self, ph::_1),
                       std::bind(&_CanEdit, _self, ph::_1, ph::_2),
                       NULL, !fixBackpointers)) {
        return false;
    }

    SdfChangeBlock block;
    for (const auto& edit : final) {
        _DoEdit(_self, edit);
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

    if (SdfPrimSpecHandle owner = 
        TfDynamic_cast<SdfPrimSpecHandle>(prop->GetOwner())) {

        owner->RemoveProperty(prop);
        _RemoveInertToRootmost(owner);

    } 
    else if (SdfAttributeSpecHandle attr = 
             TfDynamic_cast<SdfAttributeSpecHandle>(prop)) {
        Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::RemoveChild(
            _self, attr->GetPath().GetParentPath(), attr->GetNameToken());
    }
    else if (SdfRelationshipSpecHandle rel = 
             TfDynamic_cast<SdfRelationshipSpecHandle>(prop)) {
        Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::RemoveChild(
            _self, rel->GetPath().GetParentPath(), rel->GetNameToken());
    }
    //XXX: We may want to do something like 
    //     _RemoveInertToRootmost here, but that would currently 
    //     exacerbate bug 23878. Until we have  a solution for that bug,
    //     we won't automatically clean up our parents in this case.
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

    string oldLayerPath;
    SdfLayer::FileFormatArguments oldArguments;
    if (!TF_VERIFY(Sdf_SplitIdentifier(
            GetIdentifier(), &oldLayerPath, &oldArguments))) {
        return;
    }

    string newLayerPath;
    SdfLayer::FileFormatArguments newArguments;
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

    string whyNot;
    if (!Sdf_CanCreateNewLayerWithIdentifier(newLayerPath, &whyNot)) {
        TF_CODING_ERROR("Cannot change identifier to '%s': %s", 
            identifier.c_str(), whyNot.c_str());
        return;
    }

    // Create an identifier for the layer based on the desired identifier
    // that was passed in. Since this may identifier may point to an asset
    // that doesn't exist yet, use CreateIdentifierForNewAsset.
    newLayerPath = ArGetResolver().CreateIdentifierForNewAsset(newLayerPath);
    const string absIdentifier = 
        Sdf_CreateIdentifier(newLayerPath, newArguments);
    const ArResolvedPath oldResolvedPath = GetResolvedPath();

    // Hold open a change block to defer identifier-did-change
    // notification until the mutex is unlocked.
    SdfChangeBlock block;

    {
        tbb::queuing_rw_mutex::scoped_lock lock;

        // See if another layer with the same identifier exists in the registry.
        // If it doesn't, we will be updating the registry so we need to ensure
        // our lock is upgraded to a write lock by setting retryAsWriter = true.
        //
        // It is possible that the call to _Find returns the same layer we're 
        // modifying. For example, if a layer was originally opened using some
        // path and we're now trying to set its identifier to something that
        // resolves to that same path. In this case, we don't want to error
        // out.
        const bool retryAsWriter = true;
        SdfLayerRefPtr existingLayer = _Find(
            absIdentifier, FileFormatArguments(), lock, retryAsWriter);
        if (existingLayer) {
            if (get_pointer(existingLayer) != this) {
                TF_CODING_ERROR(
                    "Layer with identifier '%s' and resolved path '%s' exists.",
                    existingLayer->GetIdentifier().c_str(),
                    existingLayer->GetResolvedPath().GetPathString().c_str());
                return;
            }
        }

        // We should have acquired a write lock on the layer registry by this
        // point, so it's safe to call _InitializeFromIdentifier.
        _InitializeFromIdentifier(absIdentifier);
    }

    // If this layer has changed where it's stored, reset the modification
    // time. Note that the new identifier may not resolve to an existing
    // location, and we get an empty timestamp from the resolver. 
    // This is OK -- this means the layer hasn't been serialized to this 
    // new location yet.
    const ArResolvedPath newResolvedPath = GetResolvedPath();
    if (oldResolvedPath != newResolvedPath) {
        const ArTimestamp timestamp = ArGetResolver().GetModificationTimestamp(
            newLayerPath, newResolvedPath);
        _assetModificationTime =
            (timestamp.IsValid() || Sdf_ResolvePath(newLayerPath)) ?
            VtValue(timestamp) : VtValue();
    }
}

void
SdfLayer::UpdateAssetInfo()
{
    TRACE_FUNCTION();
    TF_DEBUG(SDF_LAYER).Msg("SdfLayer::UpdateAssetInfo()\n");

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
        std::unique_ptr<ArResolverContextBinder> binder;
        if (!GetAssetName().empty()) {
            binder.reset(new ArResolverContextBinder(
                    _assetInfo->resolverContext));
        }    

        TF_PY_ALLOW_THREADS_IN_SCOPE();
        tbb::queuing_rw_mutex::scoped_lock lock(_GetLayerRegistryMutex());
        _InitializeFromIdentifier(GetIdentifier());
    }
}

string
SdfLayer::GetDisplayName() const
{
    return GetDisplayNameFromIdentifier(GetIdentifier());
}

const ArResolvedPath&
SdfLayer::GetResolvedPath() const
{
    return _assetInfo->resolvedPath;
}

const string&
SdfLayer::GetRealPath() const
{
    return _assetInfo->resolvedPath.GetPathString();
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

SdfLayerHints
SdfLayer::GetHints() const
{
    // Hints are invalidated by any authoring operation but we don't want to
    // incur the cost of resetting the _hints object at authoring time.
    // Instead, we return a default SdfLayerHints here if the layer is dirty.
    return IsDirty() ? SdfLayerHints{} : _hints;
}

SdfDataRefPtr
SdfLayer::GetMetadata() const
{
    SdfDataRefPtr result = TfCreateRefPtr(new SdfData);
    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();

    // The metadata for this layer is the data at the absolute root path.
    // Here, we copy it into 'result'.
    //
    // XXX: This is copying more than just the metadata. This includes things
    //      like name children, etc. We should probably be filtering this to
    //      just fields tagged as metadata in the schema.
    result->CreateSpec(absRoot, SdfSpecTypePseudoRoot);
    const TfTokenVector tokenVec = ListFields(absRoot);
    for (auto const &token : tokenVec) {
        const VtValue &value = GetField(absRoot, token);
        result->Set(absRoot, token, value);
    }

    return result;
}

string
SdfLayer::ComputeAbsolutePath(const string& assetPath) const
{
    if (assetPath.empty()
        || Sdf_IsAnonLayerIdentifier(assetPath)) {
        return assetPath;
    }

    return SdfComputeAssetPathRelativeToLayer(
        SdfCreateNonConstHandle(this), assetPath);
}

SdfLayer::DetachedLayerRules&
SdfLayer::DetachedLayerRules::Include(
    const std::vector<std::string>& patterns)
{
    _include.insert(_include.end(), patterns.begin(), patterns.end());

    std::sort(_include.begin(), _include.end());
    _include.erase(
        std::unique(_include.begin(), _include.end()), _include.end());

    return *this;
}

SdfLayer::DetachedLayerRules&
SdfLayer::DetachedLayerRules::Exclude(
    const std::vector<std::string>& patterns)
{
    _exclude.insert(_exclude.end(), patterns.begin(), patterns.end());

    std::sort(_exclude.begin(), _exclude.end());
    _exclude.erase(
        std::unique(_exclude.begin(), _exclude.end()), _exclude.end());

    return *this;
}

bool
SdfLayer::DetachedLayerRules::IsIncluded(const std::string& identifier) const
{
    // Early out if nothing is included in the mask.
    if (!_includeAll && _include.empty()) {
        return false;
    }

    // Always exclude anonymous layer identifiers.
    if (Sdf_IsAnonLayerIdentifier(identifier)) {
        return false;
    }

    // Only match against the layer path portion of the identifier and
    // not the file format arguments.
    std::string layerPath, args;
    if (!Sdf_SplitIdentifier(identifier, &layerPath, &args)) {
        return false;
    }

    const bool included = 
        _includeAll || 
        std::find_if(_include.begin(), _include.end(),
            [&layerPath](const std::string& s) { 
                return TfStringContains(layerPath, s);
            }) != _include.end();

    if (!included) {
        return false;
    }

    const bool excluded =
        std::find_if(_exclude.begin(), _exclude.end(),
            [&layerPath](const std::string& s) {
                return TfStringContains(layerPath, s);
            }) != _exclude.end();

    return !excluded;
}

void
SdfLayer::SetDetachedLayerRules(const DetachedLayerRules& rules)
{
    const DetachedLayerRules oldRules = *_detachedLayerRules;
    *_detachedLayerRules = rules;

    ArResolverScopedCache resolverCache;
    SdfChangeBlock changes;

    for (const SdfLayerHandle& layer : GetLoadedLayers()) {
        const bool wasIncludedBefore = 
            oldRules.IsIncluded(layer->GetIdentifier());
        const bool isIncludedNow =
            rules.IsIncluded(layer->GetIdentifier());

        const bool layerIsDetached = layer->IsDetached();

        if (!wasIncludedBefore && isIncludedNow && !layerIsDetached) {
            layer->Reload(/* force = */ true);
        }
        if (wasIncludedBefore && !isIncludedNow && layerIsDetached) {
            layer->Reload(/* force = */ true);
        }
    }
}

const SdfLayer::DetachedLayerRules&
SdfLayer::GetDetachedLayerRules()
{
    return *_detachedLayerRules;
}

bool
SdfLayer::IsIncludedByDetachedLayerRules(const std::string& identifier)
{
    return _detachedLayerRules->IsIncluded(identifier);
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
                SdfAbstractDataRefPtr initializedData = layer->_CreateData();
                if (layer->_data->StreamsData()) {
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
                    SdfAbstractDataRefPtr mutedData = layer->_CreateData();
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
                    // If SdfAbstractData::StreamsData() is true, this re-takes 
                    // ownership of the mutedData object.  Otherwise, this 
                    // mutates the existing data container to match its 
                    // contents.
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

    const bool isStreamingLayer = _data->StreamsData();

    _SetData(_CreateData());

    if (isStreamingLayer) {
        _stateDelegate->_MarkCurrentStateAsDirty();
    }
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
    _stateDelegate->_SetLayer(_self);

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
        SdfNotice::LayerDirtinessChanged().Send(_self);
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

bool
SdfLayer::StreamsData() const
{
    return _GetData()->StreamsData();
}

bool
SdfLayer::IsDetached() const
{
    return _GetData()->IsDetached();
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
    bool isStreamingLayer = _data->StreamsData();
    SdfAbstractDataRefPtr newData;

    if (!notify || isStreamingLayer) {
        newData = _CreateData();
        newData->CopyFrom(layer->_data);
    }
    else {
        newData = layer->_data;
    }

    if (notify) {
        _SetData(newData, &(layer->GetSchema()));
    } else {
        _data = newData;
    }

    // Copy hints from other layer
    _hints = layer->_hints;

    // If this is a "streaming" layer, we must mark it dirty.
    if (isStreamingLayer) {
        _stateDelegate->_MarkCurrentStateAsDirty();
    }
}

static void
_GatherPrimCompositionDependencies(const SdfPrimSpecHandle &prim,
                                   set<string> *assetReferences)
{
    if (prim != prim->GetLayer()->GetPseudoRoot()) {
        // Prim references
        for (const SdfReference &ref:
             prim->GetReferenceList().GetAddedOrExplicitItems()) {
            assetReferences->insert(ref.GetAssetPath());
        }

        // Prim payloads
        for (const SdfPayload &payload:
             prim->GetPayloadList().GetAddedOrExplicitItems()) {
            assetReferences->insert(payload.GetAssetPath());
        }

        // Prim variants
        SdfVariantSetsProxy variantSetMap = prim->GetVariantSets();
        for (const auto &varSetIt: variantSetMap) {
            const SdfVariantSetSpecHandle &varSetSpec = varSetIt.second;
            const SdfVariantSpecHandleVector &variants =
                varSetSpec->GetVariantList();
            for(const SdfVariantSpecHandle &varSpec : variants) {
                _GatherPrimCompositionDependencies( 
                    varSpec->GetPrimSpec(), assetReferences );
            }
        }
    }

    // Recurse on nameChildren
    for (const SdfPrimSpecHandle &child : prim->GetNameChildren()) {
        _GatherPrimCompositionDependencies(child, assetReferences);
    }
}

set<string>
SdfLayer::GetExternalReferences() const
{
    return GetCompositionAssetDependencies();
}

bool
SdfLayer::UpdateExternalReference(
    const string &oldLayerPath,
    const string &newLayerPath)
{
    return UpdateCompositionAssetDependency(oldLayerPath, newLayerPath);
}

set<string>
SdfLayer::GetCompositionAssetDependencies() const
{
    SdfSubLayerProxy subLayers = GetSubLayerPaths();

    set<string> results(subLayers.begin(), subLayers.end());

    _GatherPrimCompositionDependencies(GetPseudoRoot(), &results);

    return results;
}

bool
SdfLayer::UpdateCompositionAssetDependency(
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

    _UpdatePrimCompositionDependencyPaths(
        GetPseudoRoot(), oldLayerPath, newLayerPath);

    return true;
}


std::set<std::string> 
SdfLayer::GetExternalAssetDependencies() const
{
    return _fileFormat->GetExternalAssetDependencies(*this);
}

// ModifyItemEdits() callback that updates a reference's or payload's
// asset path for SdfReferenceListEditor and SdfPayloadListEditor.
template <class RefOrPayloadType>
static boost::optional<RefOrPayloadType>
_UpdateRefOrPayloadPath(
    const string &oldLayerPath,
    const string &newLayerPath,
    const RefOrPayloadType &refOrPayload)
{
    if (refOrPayload.GetAssetPath() == oldLayerPath) {
        // Delete if new layer path is empty, otherwise rename.
        if (newLayerPath.empty()) {
            return boost::optional<RefOrPayloadType>();
        } else {
            RefOrPayloadType updatedRefOrPayload = refOrPayload;
            updatedRefOrPayload.SetAssetPath(newLayerPath);
            return updatedRefOrPayload;
        }
    }
    return refOrPayload;
}

void
SdfLayer::_UpdatePrimCompositionDependencyPaths(
    const SdfPrimSpecHandle &prim,
    const string &oldLayerPath,
    const string &newLayerPath)
{
    TF_AXIOM(!oldLayerPath.empty());
    
    // Prim references
    prim->GetReferenceList().ModifyItemEdits(std::bind(
        &_UpdateRefOrPayloadPath<SdfReference>, oldLayerPath, newLayerPath,
        ph::_1));

    // Prim payloads
    prim->GetPayloadList().ModifyItemEdits(std::bind(
        &_UpdateRefOrPayloadPath<SdfPayload>, oldLayerPath, newLayerPath, 
        ph::_1));

    // Prim variants
    SdfVariantSetsProxy variantSetMap = prim->GetVariantSets();
    for (const auto& setNameAndSpec : variantSetMap) {
        const SdfVariantSetSpecHandle &varSetSpec = setNameAndSpec.second;
        const SdfVariantSpecHandleVector &variants =
            varSetSpec->GetVariantList();
        for (const auto& variantSpec : variants) {
            _UpdatePrimCompositionDependencyPaths(
                variantSpec->GetPrimSpec(), oldLayerPath, newLayerPath);
        }
    }

    // Recurse on nameChildren
    for (const auto& primSpec : prim->GetNameChildren()) {
        _UpdatePrimCompositionDependencyPaths(
            primSpec, oldLayerPath, newLayerPath);
    }
}

/*static*/
void
SdfLayer::DumpLayerInfo()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();
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
    TF_PY_ALLOW_THREADS_IN_SCOPE();
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

    // The layer constructor sets _initializationComplete to false, which will
    // block any other threads trying to use the layer until we complete
    // initialization.  But now that the layer is in the registry, we release
    // the registry lock to avoid blocking progress of threads working with
    // other layers.
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

    if (!layer->IsMuted()) {
        // Run the file parser to read in the file contents.  We do this in a
        // dispatcher, so that other threads that "wait" to read this file can
        // actually participate in completing its loading (assuming the layer
        // _Read is internally task-parallel).
        bool readSuccess = false;
        // XXX: Disabled for now due to intermittent crashes.
        // WorkWithScopedParallelism([&]() {
        //     layer->_initDispatcher.Run([&readSuccess, &layer, &info,
        //                                 &readFilePath, metadataOnly]() {
            readSuccess = layer->_Read(
                info.identifier, info.resolvedLayerPath, metadataOnly);
        //     });
        //     layer->_initDispatcher.Wait();
        //});
        if (!readSuccess) {
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
        VtValue timestamp(ArGetResolver().GetModificationTimestamp(
            info.layerPath, info.resolvedLayerPath));
        layer->_assetModificationTime.Swap(timestamp);
    }

    // Store any external asset dependencies so we have an initial state to
    // compare during reload.
    layer->_externalAssetModificationTimes =
        Sdf_ComputeExternalAssetModificationTimestamps(*layer);

    layer->_MarkCurrentStateAsClean();

    // Layer initialization is complete.
    layer->_FinishInitialization(/* success = */ true);

    return layer;
}

bool
SdfLayer::HasSpec(const SdfPath& path) const
{
    return _data->HasSpec(path);
}

SdfSpecType
SdfLayer::GetSpecType(const SdfPath& path) const
{
    return _data->GetSpecType(path);
}

vector<TfToken>
SdfLayer::ListFields(const SdfPath& path) const
{
    return _ListFields(_schema, *get_pointer(_data), path);
}

vector<TfToken>
SdfLayer::_ListFields(SdfSchemaBase const &schema,
                      SdfAbstractData const &data, const SdfPath& path)
{
    // Invoke List() on the underlying data implementation but be sure to
    // include all required fields too.

    // Collect the list from the data implemenation.
    vector<TfToken> dataList = data.List(path);

    // Determine spec type.  If unknown, return early.
    SdfSpecType specType = data.GetSpecType(path);
    if (ARCH_UNLIKELY(specType == SdfSpecTypeUnknown)) {
        return dataList;
    }

    // Collect required fields.
    vector<TfToken> const &req = schema.GetRequiredFields(specType);

    // Union them together, but retain order of dataList, since it influences
    // the output ordering in some file writers.
    TfToken const *dataListBegin = dataList.data();
    TfToken const *dataListEnd = dataListBegin + dataList.size();
    bool mightAlloc = (dataList.size() + req.size()) > dataList.capacity();
    for (size_t reqIdx = 0, reqSz = req.size(); reqIdx != reqSz; ++reqIdx) {
        TfToken const &reqName = req[reqIdx];
        TfToken const *iter = std::find(dataListBegin, dataListEnd, reqName);
        if (iter == dataListEnd) {
            // If the required field name is not already present, append it.
            // Make sure we have capacity for all required fields so we do no
            // more than one additional allocation here.
            if (mightAlloc && dataList.size() == dataList.capacity()) {
                dataList.reserve(dataList.size() + (reqSz - reqIdx));
                dataListEnd =
                    dataList.data() + std::distance(dataListBegin, dataListEnd);
                dataListBegin = dataList.data();
                mightAlloc = false;
            }
            dataList.push_back(reqName);
        }
    }
    return dataList;
}

SdfSchema::FieldDefinition const *
SdfLayer::_GetRequiredFieldDef(const SdfPath &path,
                               const TfToken &fieldName,
                               SdfSpecType specType) const
{
    SdfSchemaBase const &schema = _schema;
    if (ARCH_UNLIKELY(schema.IsRequiredFieldName(fieldName))) {
        // Get the spec definition.
        if (specType == SdfSpecTypeUnknown) {
            specType = GetSpecType(path);
        }
        if (SdfSchema::SpecDefinition const *
            specDef = schema.GetSpecDefinition(specType)) {
            // If this field is required for this spec type, look up the
            // field definition.
            if (specDef->IsRequiredField(fieldName)) {
                return schema.GetFieldDefinition(fieldName);
            }
        }
    }
    return nullptr;
}

SdfSchema::FieldDefinition const *
SdfLayer::_GetRequiredFieldDef(const SdfSchemaBase &schema,
                               const TfToken &fieldName,
                               SdfSpecType specType)
{
    if (ARCH_UNLIKELY(schema.IsRequiredFieldName(fieldName))) {
        if (SdfSchema::SpecDefinition const *
            specDef = schema.GetSpecDefinition(specType)) {
            // If this field is required for this spec type, look up the
            // field definition.
            if (specDef->IsRequiredField(fieldName)) {
                return schema.GetFieldDefinition(fieldName);
            }
        }
    }
    return nullptr;
}

bool
SdfLayer::_HasField(const SdfSchemaBase &schema,
                    const SdfAbstractData &data,
                    const SdfPath& path,
                    const TfToken& fieldName,
                    VtValue *value)
{
    SdfSpecType specType;
    if (data.HasSpecAndField(path, fieldName, value, &specType)) {
        return true;
    }
    if (specType == SdfSpecTypeUnknown) {
        return false;
    }
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(schema, fieldName, specType)) {
        if (value)
            *value = def->GetFallbackValue();
        return true;
    }
    return false;
}

bool
SdfLayer::HasField(const SdfPath& path, const TfToken& fieldName,
                   VtValue *value) const
{
    SdfSpecType specType;
    if (_data->HasSpecAndField(path, fieldName, value, &specType)) {
        return true;
    }
    if (specType == SdfSpecTypeUnknown) {
        return false;
    }
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(path, fieldName, specType)) {
        if (value)
            *value = def->GetFallbackValue();
        return true;
    }
    return false;
}

bool
SdfLayer::HasField(const SdfPath& path, const TfToken& fieldName,
                   SdfAbstractDataValue *value) const
{
    SdfSpecType specType;
    if (_data->HasSpecAndField(path, fieldName, value, &specType)) {
        return true;
    }
    if (specType == SdfSpecTypeUnknown) {
        return false;
    }
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(path, fieldName, specType)) {
        if (value)
            return value->StoreValue(def->GetFallbackValue());
        return true;
    }
    return false;
}

bool
SdfLayer::HasFieldDictKey(const SdfPath& path,
                          const TfToken &fieldName,
                          const TfToken &keyPath,
                          VtValue *value) const
{
    if (_data->HasDictKey(path, fieldName, keyPath, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(path, fieldName)) {
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
SdfLayer::HasFieldDictKey(const SdfPath& path,
                          const TfToken &fieldName,
                          const TfToken &keyPath,
                          SdfAbstractDataValue *value) const
{
    if (_data->HasDictKey(path, fieldName, keyPath, value))
        return true;
    // Otherwise if this is a required field, and the data has a spec here,
    // return the fallback value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(path, fieldName)) {
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
SdfLayer::GetField(const SdfPath& path,
    const TfToken& fieldName) const
{
    VtValue result;
    HasField(path, fieldName, &result);
    return result;
}

VtValue
SdfLayer::_GetField(const SdfSchemaBase &schema,
                    const SdfAbstractData &data,
                    const SdfPath& path,
                    const TfToken& fieldName)
{
    VtValue result;
    _HasField(schema, data, path, fieldName, &result);
    return result;
}

VtValue
SdfLayer::GetFieldDictValueByKey(const SdfPath& path,
                                 const TfToken& fieldName,
                                 const TfToken &keyPath) const
{
    VtValue result;
    HasFieldDictKey(path, fieldName, keyPath, &result);
    return result;
}

static bool
_IsValidFieldForLayer(
    const SdfLayer& layer, const SdfPath& path, 
    const TfToken& fieldName)
{
    return layer.GetSchema().IsValidFieldForSpec(
        fieldName, layer.GetSpecType(path));
}

void
SdfLayer::SetField(const SdfPath& path, const TfToken& fieldName,
                   const VtValue& value)
{
    if (value.IsEmpty())
        return EraseField(path, fieldName);

    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot set %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (ARCH_UNLIKELY(_validateAuthoring) && 
        !_IsValidFieldForLayer(*this, path, fieldName)) {
        TF_ERROR(SdfAuthoringErrorUnrecognizedFields,
                 "Cannot set %s on <%s>. Field is not valid for layer @%s@.",
                 fieldName.GetText(), path.GetText(),
                 GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetField(path, fieldName);
    if (value != oldValue)
        _PrimSetField(path, fieldName, value, &oldValue);
}

void
SdfLayer::SetField(const SdfPath& path, const TfToken& fieldName,
                   const SdfAbstractDataConstValue& value)
{
    if (value.IsEqual(VtValue()))
        return EraseField(path, fieldName);

    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot set %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (ARCH_UNLIKELY(_validateAuthoring) && 
        !_IsValidFieldForLayer(*this, path, fieldName)) {
        TF_ERROR(SdfAuthoringErrorUnrecognizedFields,
                 "Cannot set %s on <%s>. Field is not valid for layer @%s@.",
                 fieldName.GetText(), path.GetText(),
                 GetIdentifier().c_str());
        return;
    }
    
    VtValue oldValue = GetField(path, fieldName);
    if (!value.IsEqual(oldValue))
        _PrimSetField(path, fieldName, value, &oldValue);
}

void
SdfLayer::SetFieldDictValueByKey(const SdfPath& path,
                                 const TfToken& fieldName,
                                 const TfToken& keyPath,
                                 const VtValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (ARCH_UNLIKELY(_validateAuthoring) && 
        !_IsValidFieldForLayer(*this, path, fieldName)) {
        TF_ERROR(SdfAuthoringErrorUnrecognizedFields,
                 "Cannot set %s:%s on <%s>. Field is not valid for layer @%s@.",
                 fieldName.GetText(), keyPath.GetText(),
                 path.GetText(), GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetFieldDictValueByKey(path, fieldName, keyPath);
    if (value == oldValue)
        return;

    _PrimSetFieldDictValueByKey(path, fieldName, keyPath, value, &oldValue);
}

void
SdfLayer::SetFieldDictValueByKey(const SdfPath& path,
                                 const TfToken& fieldName,
                                 const TfToken& keyPath,
                                 const SdfAbstractDataConstValue& value)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot set %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (ARCH_UNLIKELY(_validateAuthoring) && 
        !_IsValidFieldForLayer(*this, path, fieldName)) {
        TF_ERROR(SdfAuthoringErrorUnrecognizedFields,
                 "Cannot set %s:%s on <%s>. Field is not valid for layer @%s@.",
                 fieldName.GetText(), keyPath.GetText(),
                 path.GetText(), GetIdentifier().c_str());
        return;
    }

    VtValue oldValue = GetFieldDictValueByKey(path, fieldName, keyPath); 
    if (value.IsEqual(oldValue))
        return;

    _PrimSetFieldDictValueByKey(path, fieldName, keyPath, value, &oldValue);
}

void
SdfLayer::EraseField(const SdfPath& path, const TfToken& fieldName)
{
    if (ARCH_UNLIKELY(!PermissionToEdit())) {
        TF_CODING_ERROR("Cannot erase %s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (!_data->Has(path, fieldName)) {
        return;
    }

    // If this is a required field, only perform the set if the current value
    // differs from the fallback.  Required fields behave as if they're always
    // authored, so the effect of an "erase" is to set the value to the fallback
    // value.
    if (SdfSchema::FieldDefinition const *def =
        _GetRequiredFieldDef(path, fieldName)) {
        if (GetField(path, fieldName) == def->GetFallbackValue())
            return;
    }

    // XXX:
    // Note that with this implementation, erasing a field and undoing that
    // operation will not restore the underlying SdfData exactly to its
    // previous state. Specifically, this may cause the order of the fields
    // for the given spec to change. There are no semantics attached to this
    // ordering, so this should hopefully be OK.
    _PrimSetField(path, fieldName, VtValue());
}

void
SdfLayer::EraseFieldDictValueByKey(const SdfPath& path,
                                   const TfToken& fieldName,
                                   const TfToken &keyPath)
{
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("Cannot erase %s:%s on <%s>. Layer @%s@ is not editable.",
                        fieldName.GetText(), keyPath.GetText(),
                        path.GetText(), 
                        GetIdentifier().c_str());
        return;
    }

    if (!_data->HasDictKey(path, fieldName, keyPath,
                           static_cast<VtValue *>(NULL))) {
        return;
    }

    // XXX:
    // Note that with this implementation, erasing a field and undoing that
    // operation will not restore the underlying SdfData exactly to its
    // previous state. Specifically, this may cause the order of the fields
    // for the given spec to change. There are no semantics attached to this
    // ordering, so this should hopefully be OK.
    _PrimSetFieldDictValueByKey(path, fieldName, keyPath, VtValue());
}

SdfAbstractDataConstPtr 
SdfLayer::_GetData() const
{
    return _data;
}

SdfAbstractDataRefPtr
SdfLayer::_CreateData() const
{
    return _CreateDataForFileFormat(
        GetFileFormat(), GetIdentifier(), GetFileFormatArguments());
}

void
SdfLayer::_SwapData(SdfAbstractDataRefPtr &data)
{
    _data.swap(data);
}

void
SdfLayer::_AdoptData(const SdfAbstractDataRefPtr &newData)
{
    SdfChangeBlock block;
    _data = newData;
    Sdf_ChangeManager::Get().DidReplaceLayerContent(_self);
}

void
SdfLayer::_SetData(const SdfAbstractDataPtr &newData,
                   const SdfSchemaBase *newDataSchema)
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Setting layer data");

    // Guard against setting an empty SdfData, which is invalid.
    TF_VERIFY(!newData->IsEmpty() );

    // This code below performs a series of specific edits to mutate _data
    // to match newData.  This approach provides fine-grained change
    // notification, which allows more efficient invalidation in clients
    // of Sdf.  Do all this in a single changeblock.
    SdfChangeBlock block;

    // If we're transferring from one schema to a different schema, we will go
    // through the fine-grained update in order to do cross-schema field
    // validation.
    const bool differentSchema = newDataSchema && newDataSchema != &GetSchema();

    // If this layer streams its data on demand, we avoid the fine-grained
    // change code path (unless it's to a different schema) because that would
    // cause all of the data in the layer to be streamed in from disk.  So we
    // move the new data into place and notify the world that this layer may
    // have changed arbitrarily.
    if (!differentSchema && _data->StreamsData()) {
        _AdoptData(newData);
        return;
    }

    // Remove specs that no longer exist or whose required fields changed.
    {
        // Collect specs to delete, ordered by namespace.
        struct _SpecsToDelete : public SdfAbstractDataSpecVisitor {
            _SpecsToDelete(const SdfAbstractDataPtr& newData_)
                : newData(newData_) { }

            virtual bool VisitSpec(
                const SdfAbstractData& oldData, const SdfPath& path)
            {
                if (!newData->HasSpec(path) ||
                    (newData->GetSpecType(path) != oldData.GetSpecType(path))) {
                    paths.insert(path);
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
            const SdfPath &path = *i;

            std::vector<TfToken> fields = ListFields(path);

            SdfSpecType specType = _data->GetSpecType(path);
            const SdfSchema::SpecDefinition* specDefinition = 
                GetSchema().GetSpecDefinition(specType);

            TF_FOR_ALL(field, fields) {
                if (!specDefinition->IsRequiredField(*field))
                    _PrimSetField(path, *field, VtValue());
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
                const SdfAbstractData& newData, const SdfPath& path)
            {
                if (!oldData.HasSpec(path)) {
                    paths.insert(path);
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

        SdfPath unrecognizedSpecTypePaths[SdfNumSpecTypes];

        // Create specs top-down to provide optimal diffs.
        TF_FOR_ALL(i, specsToCreate.paths) {
            const SdfPath& path = *i;

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
                    (newData->GetAs<SdfSpecifier>(path, SdfFieldKeys->Specifier,
                                                  SdfSpecifierOver)
                        == SdfSpecifierOver)
                    && (newData->GetAs<TfToken>(path, SdfFieldKeys->TypeName,
                                                TfToken())
                        .IsEmpty());
            } else if (path.IsPropertyPath()) {
                // Properties are considered inert if they are custom.
                inert = !newData->GetAs<bool>(path, SdfFieldKeys->Custom,
                                              false);
            }

            SdfSpecType specType = newData->GetSpecType(path);

            // If this is a cross-schema _SetData call, check to see if the spec
            // type is known to this layer's schema.  If not, skip creating it
            // and record it to issue an error later.
            if (differentSchema && !GetSchema().GetSpecDefinition(specType)) {
                // Record the path where this spec type was first encountered.
                if (unrecognizedSpecTypePaths[specType].IsEmpty()) {
                    unrecognizedSpecTypePaths[specType] = path;
                }
            }
            else {
                _PrimCreateSpec(path, specType, inert);
            }
        }
        // If there were unrecognized specTypes, issue an error.
        if (differentSchema) {
            vector<string> specDescrs;
            for (int i = 0; i != SdfSpecTypeUnknown; ++i) {
                if (unrecognizedSpecTypePaths[i].IsEmpty()) {
                    continue;
                }
                specDescrs.push_back(
                    TfStringPrintf(
                        "'%s' first seen at <%s>",
                        TfStringify(static_cast<SdfSpecType>(i)).c_str(),
                        unrecognizedSpecTypePaths[i].GetAsString().c_str()));
            }
            if (!specDescrs.empty()) {
                TF_ERROR(SdfAuthoringErrorUnrecognizedSpecType,
                         "Omitted unrecognized spec types setting data on "
                         "@%s@: %s", GetIdentifier().c_str(),
                         TfStringJoin(specDescrs, "; ").c_str());
            }
        }
    }

    // Update spec fields.
    {
        struct _SpecUpdater : public SdfAbstractDataSpecVisitor {
            _SpecUpdater(SdfLayer* layer_,
                         const SdfSchemaBase &newDataSchema_)
                : layer(layer_)
                , newDataSchema(newDataSchema_) {}

            virtual bool VisitSpec(
                const SdfAbstractData& newData, const SdfPath& path)
            {
                const TfTokenVector oldFields = layer->ListFields(path);
                const TfTokenVector newFields =
                    _ListFields(newDataSchema, newData, path);

                const SdfSchemaBase &thisLayerSchema = layer->GetSchema();

                const bool differentSchema = &thisLayerSchema != &newDataSchema;

                // If this layer has a different schema from newDataSchema, then
                // it's possible there is no corresponding spec for the path, in
                // case the spec type is not supported.  Check for this, and
                // skip field processing if so.
                if (differentSchema && !layer->HasSpec(path)) {
                    return true;
                }

                // Remove empty fields.
                for (TfToken const &field: oldFields) {
                    // This is O(N^2) in number of fields in each spec, but
                    // we expect a small max N, around 10.
                    if (std::find(newFields.begin(), newFields.end(), field)
                        == newFields.end()) {
                        layer->_PrimSetField(path, field, VtValue());
                    }
                }

                // Set field values.
                for (TfToken const &field: newFields) {
                    VtValue newValue =
                        _GetField(newDataSchema, newData, path, field);
                    VtValue oldValue = layer->GetField(path, field);
                    if (oldValue != newValue) {
                        if (differentSchema && oldValue.IsEmpty() &&
                            !thisLayerSchema.IsValidFieldForSpec(
                                field, layer->GetSpecType(path))) {
                            // This field might not be valid for the target
                            // schema.  If that's the case record it (if it's
                            // not already recorded) and skip setting it.
                            unrecognizedFields.emplace(field, path);
                        }
                        else {
                            layer->_PrimSetField(path, field,
                                                 newValue, &oldValue);
                        }
                    }
                }
                return true;
            }

            virtual void Done(const SdfAbstractData&)
            {
                // Do nothing
            }

            SdfLayer* layer;
            const SdfSchemaBase &newDataSchema;
            std::map<TfToken, SdfPath> unrecognizedFields;
        };

        // If no newDataSchema is supplied, we assume the newData adheres to
        // this layer's schema.
        _SpecUpdater updater(
            this, newDataSchema ? *newDataSchema : GetSchema());
        newData->VisitSpecs(&updater);

        // If there were unrecognized fields, report an error.
        if (!updater.unrecognizedFields.empty()) {
            vector<string> fieldDescrs;
            fieldDescrs.reserve(updater.unrecognizedFields.size());
            for (std::pair<TfToken, SdfPath> const &tokenPath:
                     updater.unrecognizedFields) {
                fieldDescrs.push_back(
                    TfStringPrintf("'%s' first seen at <%s>",
                                   tokenPath.first.GetText(),
                                   tokenPath.second.GetAsString().c_str()));
            }
            TF_ERROR(SdfAuthoringErrorUnrecognizedFields,
                     "Omitted unrecognized fields setting data on @%s@: %s",
                     GetIdentifier().c_str(),
                     TfStringJoin(fieldDescrs, "; ").c_str());
        }
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
SdfLayer::_PrimSetField(const SdfPath& path, 
                        const TfToken& fieldName,
                        const T& value,
                        VtValue* oldValuePtr,
                        bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetField(path, fieldName, value, oldValuePtr);
        return;
    }

    VtValue oldValue = 
        oldValuePtr ? std::move(*oldValuePtr) : GetField(path, fieldName);
    const VtValue& newValue = _GetVtValue(value);

    // Send notification when leaving the change block.
    SdfChangeBlock block;

    Sdf_ChangeManager::Get().DidChangeField(
        _self, path, fieldName, std::move(oldValue), newValue);

    _data->Set(path, fieldName, value);
}

template void SdfLayer::_PrimSetField(
    const SdfPath&, const TfToken&, 
    const VtValue&, VtValue *, bool);
template void SdfLayer::_PrimSetField(
    const SdfPath&, const TfToken&, 
    const SdfAbstractDataConstValue&, VtValue *, bool);

template <class T>
void
SdfLayer::_PrimPushChild(const SdfPath& parentPath, 
                         const TfToken& fieldName,
                         const T& value,
                         bool useDelegate)
{
    if (!HasField(parentPath, fieldName)) {
        _PrimSetField(parentPath, fieldName,
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
    VtValue box = _data->Get(parentPath, fieldName);
    _data->Erase(parentPath, fieldName);
    std::vector<T> vec;
    if (box.IsHolding<std::vector<T>>()) {
        box.Swap(vec);
    } else {
        // If the value isn't a vector, we replace it with an empty one.
    }
    vec.push_back(value);
    box.Swap(vec);
    _data->Set(parentPath, fieldName, box);
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
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        std::vector<T> vec = GetFieldAs<std::vector<T> >(parentPath, fieldName);
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
    VtValue box = _data->Get(parentPath, fieldName);
    _data->Erase(parentPath, fieldName);
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
    _data->Set(parentPath, fieldName, box);
}

template void SdfLayer::_PrimPopChild<TfToken>(
    const SdfPath&, const TfToken&, bool);
template void SdfLayer::_PrimPopChild<SdfPath>(
    const SdfPath&, const TfToken&, bool);

template <class T>
void
SdfLayer::_PrimSetFieldDictValueByKey(const SdfPath& path,
                                      const TfToken& fieldName,
                                      const TfToken& keyPath,
                                      const T& value,
                                      VtValue *oldValuePtr,
                                      bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->SetFieldDictValueByKey(
            path, fieldName, keyPath, value, oldValuePtr);
        return;
    }

    // Send notification when leaving the change block.
    SdfChangeBlock block;

    // This can't only use oldValuePtr currently, since we need the entire
    // dictionary, not just they key being set.  If we augment change
    // notification to be as granular as dict-key-path, we could use it.
    VtValue oldValue = GetField(path, fieldName);

    _data->SetDictValueByKey(path, fieldName, keyPath, value);

    VtValue newValue = GetField(path, fieldName);

    Sdf_ChangeManager::Get().DidChangeField(
        _self, path, fieldName, std::move(oldValue), newValue);
}

template void SdfLayer::_PrimSetFieldDictValueByKey(
    const SdfPath&, const TfToken&, const TfToken &,
    const VtValue&, VtValue *, bool);
template void SdfLayer::_PrimSetFieldDictValueByKey(
    const SdfPath&, const TfToken&, const TfToken &,
    const SdfAbstractDataConstValue&, VtValue *, bool);

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

    if (!_data->HasSpec(oldPath)) {
        // Cannot move; nothing at source.
        return false;
    }
    if (_data->HasSpec(newPath)) {
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
    
    data->MoveSpec(oldSpecPath, newSpecPath);
    idReg->MoveIdentity(oldSpecPath, newSpecPath);
}

void
SdfLayer::_PrimMoveSpec(const SdfPath& oldPath, const SdfPath& newPath,
                        bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->MoveSpec(oldPath, newPath);
        return;
    }

    SdfChangeBlock block;

    Sdf_ChangeManager::Get().DidMoveSpec(_self, oldPath, newPath);

    Traverse(oldPath, std::bind(_MoveSpecInternal, _data,
                                &_idRegistry, ph::_1, oldPath, newPath));
}

static bool
_IsValidSpecForLayer(
    const SdfLayer& layer, SdfSpecType specType)
{
    const SdfSchemaBase::SpecDefinition* specDef = 
        layer.GetSchema().GetSpecDefinition(specType);
    return static_cast<bool>(specDef);
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

    if (_validateAuthoring && !_IsValidSpecForLayer(*this, specType)) {
        TF_ERROR(SdfAuthoringErrorUnrecognizedSpecType,
                 "Cannot create spec at <%s>. %s is not a valid spec type "
                 "for layer @%s@",
                 path.GetText(), TfEnum::GetName(specType).c_str(), 
                 GetIdentifier().c_str());
        return false;
    }

    if (_data->HasSpec(path)) {
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

    if (!HasSpec(path)) {
        return false;
    }

    if (_IsInertSubtree(path)) {
        // If the subtree is inert, enqueue notifications for each spec that's
        // about to be removed. _PrimDeleteSpec adds a notice for the spec
        // path it's given, but notices about inert specs don't imply anything
        // about descendants. So if we just sent out a notice for the subtree
        // root, clients would not be made aware of the removal of the other
        // specs in the subtree.
        SdfChangeBlock block;
        Traverse(path, 
            [this, &cm = Sdf_ChangeManager::Get()](const SdfPath& specPath) {
                cm.DidRemoveSpec(_self, specPath, /* inert = */ true);
            });

        _PrimDeleteSpec(path, /* inert = */ true);
    }
    else {
        _PrimDeleteSpec(path, /* inert = */ false);
    }

    return true;
}

template<typename ChildPolicy>
void
SdfLayer::_TraverseChildren(const SdfPath &path, const TraversalFunction &func)
{
    std::vector<typename ChildPolicy::FieldType> children =
        GetFieldAs<std::vector<typename ChildPolicy::FieldType> >(
            path, ChildPolicy::GetChildrenToken(path));

    TF_FOR_ALL(i, children) {
        Traverse(ChildPolicy::GetChildPath(path, *i), func);
    }
}

void
SdfLayer::Traverse(const SdfPath &path, const TraversalFunction &func)
{
    std::vector<TfToken> fields = ListFields(path);
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
    data->EraseSpec(path);
}

void
SdfLayer::_PrimDeleteSpec(const SdfPath &path, bool inert, bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->DeleteSpec(path, inert);
        return;
    }

    SdfChangeBlock block;

    Sdf_ChangeManager::Get().DidRemoveSpec(_self, path, inert);

    TraversalFunction eraseFunc = 
        std::bind(&_EraseSpecAtPath, boost::get_pointer(_data), ph::_1);
    Traverse(path, eraseFunc);
}

void
SdfLayer::_PrimCreateSpec(const SdfPath &path,
                          SdfSpecType specType, bool inert,
                          bool useDelegate)
{
    if (useDelegate && TF_VERIFY(_stateDelegate)) {
        _stateDelegate->CreateSpec(path, specType, inert);
        return;
    }

    SdfChangeBlock block;
    
    Sdf_ChangeManager::Get().DidAddSpec(_self, path, inert);

    _data->CreateSpec(path, specType);
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

    // Prims, variants, and properties don't affect the scene if they only
    // contain opinions about required fields.
    if (specType == SdfSpecTypePrim         ||
        specType == SdfSpecTypeVariant      ||
        specType == SdfSpecTypeVariantSet   ||
        specType == SdfSpecTypeAttribute    ||
        specType == SdfSpecTypeRelationship) {

        const SdfSchema::SpecDefinition* specDefinition = 
            GetSchema().GetSpecDefinition(specType);
        if (!TF_VERIFY(specDefinition)) {
            return false;
        }

        TF_FOR_ALL(field, fields) {
            // If specified, skip over children fields.  This is a special case
            // to allow _IsInertSubtree to process these children separately.
            if (ignoreChildren &&
                ((specType == SdfSpecTypePrim &&
                  (*field == SdfChildrenKeys->PrimChildren ||
                   *field == SdfChildrenKeys->PropertyChildren ||
                   *field == SdfChildrenKeys->VariantSetChildren))
                 ||
                 (specType == SdfSpecTypeVariantSet &&
                  *field == SdfChildrenKeys->VariantChildren))) {
                continue;
            }

            // If the field is required, ignore it.
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
SdfLayer::_IsInertSubtree(
    const SdfPath &path) const
{
    if (!_IsInert(path, true /*ignoreChildren*/, 
                  true /* requiredFieldOnlyPropertiesAreInert */)) {
        return false;
    }

    // Check for a variant set path first -- this is a variant selection path
    // whose selection is the empty string.
    if (path.IsPrimVariantSelectionPath() &&
        path.GetVariantSelection().second.empty()) {

        std::string vsetName = path.GetVariantSelection().first;
        SdfPath parentPath = path.GetParentPath();

        std::vector<TfToken> variants;
        if (HasField(path, SdfChildrenKeys->VariantChildren, &variants)) {
            for (const TfToken &variant: variants) {
                if (!_IsInertSubtree(
                        parentPath.AppendVariantSelection(
                            vsetName, variant.GetString()))) {
                    return false;
                }
            }
        }
    }
    else if (path.IsPrimOrPrimVariantSelectionPath()) {

        // Check for prim & variant set children.
        for (auto const &childrenField: {
                SdfChildrenKeys->PrimChildren,
                SdfChildrenKeys->VariantSetChildren }) {
            std::vector<TfToken> childNames;
            if (HasField(path, childrenField, &childNames)) {
                for (const TfToken& name : childNames) {
                    if (!_IsInertSubtree(path.AppendChild(name))) {
                        return false;
                    }
                }
            }
        }
        
        std::vector<TfToken> properties;
        if (HasField(path, SdfChildrenKeys->PropertyChildren, &properties)) {
            for (const TfToken& prop : properties) {
                const SdfPath propPath = path.AppendProperty(prop);
                if (!_IsInert(propPath,
                        /* ignoreChildren = */ false, 
                        /* requiredFieldOnlyPropertiesAreInert = */ true)) {
                    return false;
                }
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

    return GetFileFormat()->WriteToString(*this, result);
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

    if (!fileFormat->SupportsWriting()) {
        TF_CODING_ERROR("Cannot save layer @%s@: %s file format does not"
                        "support writing",
                        newFileName.c_str(),
                        fileFormat->GetFormatId().GetText());
        return false;
    }

    // If the output file format has a different schema, then transfer content
    // to an in-memory layer first just to validate schema compatibility.
    const bool differentSchema = &fileFormat->GetSchema() != &GetSchema();
    if (differentSchema) {
        SdfLayerRefPtr tmpLayer =
            CreateAnonymous("cross-schema-write-test", fileFormat, args);
        TfErrorMark m;
        tmpLayer->TransferContent(
            SdfLayerHandle(const_cast<SdfLayer *>(this)));
        if (!m.IsClean()) {
            TF_RUNTIME_ERROR("Failed attempting to write '%s' under a "
                             "different schema.  If this is intended, "
                             "TransferContent() to a temporary anonymous "
                             "layer with the desired schema and handle "
                             "the errors, then export that temporary layer",
                             newFileName.c_str());
            return false;
        }
    }    

    bool ok = fileFormat->WriteToFile(*this, newFileName, comment, args);

    // If we wrote to the backing file then we're now clean.
    if (ok && newFileName == GetRealPath()) {
       _MarkCurrentStateAsClean();
    }

    return ok;
}

bool 
SdfLayer::Export(const string& newFileName, const string& comment,
                 const FileFormatArguments& args) const
{
    return _WriteToFile(
        newFileName,
        comment,
        // If the layer's current format supports the extension, use it,
        // otherwise pass TfNullPtr, which instructs the callee to use the
        // primary format for the output's extension.
        GetFileFormat()->IsSupportedExtension(newFileName)
            ? GetFileFormat() : TfNullPtr,
        args);
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

    const ArResolvedPath path = GetResolvedPath();
    if (path.empty())
        return false;

    // Skip saving if the file exists and the layer is clean.
    if (!force && !IsDirty() && TfPathExists(path))
        return true;

    if (!_WriteToFile(path, std::string(), 
                      GetFileFormat(), GetFileFormatArguments()))
        return false;

    // Layer hints are invalidated by authoring so _hints must be reset now
    // that the layer has been marked as clean.  See GetHints().
    _hints = SdfLayerHints{};

    // Record modification timestamp.
    _assetModificationTime = Sdf_ComputeLayerModificationTimestamp(*this);

    SdfNotice::LayerDidSaveLayerToFile().Send(_self);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
