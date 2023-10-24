//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hdsi/coordSysPrimSceneIndex.h"

#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hd/mapContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((coordSysPrimName, "__coordSys"))
    (xformDependency)
);

namespace
{

// Predicate whether to ignore a binding given the path of the
// targeted prim.
//
// We ignore non-prim paths for compatibility with UsdImagingDelegate
// which already adds coord sys hydra prims itself using a property path.
//
bool
_IgnoreBinding(const SdfPath &targetedPrimPath)
{
    return targetedPrimPath.IsEmpty() || !targetedPrimPath.IsPrimPath();
}

// Path for coord sys prim we need to create under a prim targeted
// by coord sys binding with given name.
//
// E.g. /PATH.__coordSys:FOO.
SdfPath
_PathForCoordSysPrim(const SdfPath &targetedPrimPath,
                     const TfToken &name)
{
    const TfToken propName(
        SdfPath::JoinIdentifier(
            TfTokenVector{_tokens->coordSysPrimName, name}));

    return targetedPrimPath.AppendProperty(propName);
}

// Data source for locator coordSys:FOO on a prim
// /PATH.__coordSys:FOO where /PATH is a path targeted by a
// coord sys binding and FOO is the name of the binding.
//
class _CoordSysPrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordSysPrimDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector result = {
            HdCoordSysSchema::GetSchemaToken(),
            HdXformSchema::GetSchemaToken(),
            HdDependenciesSchema::GetSchemaToken()
        };
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdCoordSysSchema::GetSchemaToken()) {
            return
                HdCoordSysSchema::Builder()
                    .SetName(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            _name))
                    .Build();
        }
        if (name == HdXformSchema::GetSchemaToken()) {
            HdContainerDataSourceHandle const primSource =
                _inputScene->GetPrim(_primPath).dataSource;
            if (!primSource) {
                return nullptr;
            }
            return primSource->Get(HdXformSchema::GetSchemaToken());
        }
        if (name == HdDependenciesSchema::GetSchemaToken()) {
            static HdLocatorDataSourceHandle const xformLocatorDs =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdXformSchema::GetDefaultLocator());
            return
                HdRetainedContainerDataSource::New(
                    _tokens->xformDependency,
                    HdDependencySchema::Builder()
                        .SetDependedOnPrimPath(
                            HdRetainedTypedSampledDataSource<SdfPath>::New(
                                _primPath))
                        .SetDependedOnDataSourceLocator(xformLocatorDs)
                        .SetAffectedDataSourceLocator(xformLocatorDs)
                        .Build());
        }
        return nullptr;
    }

private:
    _CoordSysPrimDataSource(const HdSceneIndexBaseRefPtr &inputScene,
                            const SdfPath &primPath,
                            const TfToken &name)
     : _inputScene(inputScene)
     , _primPath(primPath)
     , _name(name)
    {
    }

    HdSceneIndexBaseRefPtr const _inputScene;
    const SdfPath _primPath;
    const TfToken _name;
};

// Data source for locator coordSys.
//
// Re-writes paths of bindings to point to the coordSys prim that
// this scene index is adding.
class _CoordSysBindingDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordSysBindingDataSource);

private:
    _CoordSysBindingDataSource(const HdContainerDataSourceHandle &inputSource)
     : _inputSource(inputSource)
    {
    }

    TfTokenVector GetNames() override {
        return _inputSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        HdPathDataSourceHandle const ds =
            HdPathDataSource::Cast(_inputSource->Get(name));
        if (!ds) {
            return nullptr;
        }
        const SdfPath targetedPrimPath = ds->GetTypedValue(0.0f);
        if (_IgnoreBinding(targetedPrimPath)) {
            return ds;
        }
        return
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                _PathForCoordSysPrim(targetedPrimPath, name));
    }

    HdContainerDataSourceHandle const _inputSource;
};

// Prim data source rewriting coord sys bindings to point to
// coord sys prim this scene index is adding.
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override {
        return _inputSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        HdDataSourceBaseHandle const ds = _inputSource->Get(name);
        if (name == HdCoordSysBindingSchema::GetSchemaToken()) {
            HdContainerDataSourceHandle const containerDs =
                HdContainerDataSource::Cast(ds);
            if (!containerDs) {
                return nullptr;
            }
            return _CoordSysBindingDataSource::New(containerDs);
        }
        return ds;
    }

private:
    _PrimDataSource(const HdContainerDataSourceHandle &inputSource)
     : _inputSource(inputSource)
    {
    }

    HdContainerDataSourceHandle const _inputSource;
};

HdSceneIndexObserver::AddedPrimEntries
_ToAddedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::AddedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        entries.push_back({path, HdSprimTypeTokens->coordSys});
    }
    return entries;
}

HdSceneIndexObserver::RemovedPrimEntries
_ToRemovedPrimEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::RemovedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        entries.push_back({path});
    }
    return entries;
}

}

HdsiCoordSysPrimSceneIndex::HdsiCoordSysPrimSceneIndex(
    HdSceneIndexBaseRefPtr const &inputScene)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
{
    TRACE_FUNCTION();

    for (const SdfPath &primPath : HdSceneIndexPrimView(inputScene)) {
        _AddBindingsForPrim(primPath);
    }
}

HdContainerDataSourceHandle
HdsiCoordSysPrimSceneIndex::_GetCoordSysPrimSource(
    const SdfPath &primPath) const
{
    if (primPath.IsAbsoluteRootPath()) {
        return nullptr;
    }

    const std::string &primName = primPath.GetName();

    static const std::string &prefix = _tokens->coordSysPrimName.GetString();
    if (!TfStringStartsWith(primName, prefix)) {
        return nullptr;
    }

    const SdfPath parentPrimPath = primPath.GetParentPath();

    const auto it = _targetedPrimToNameToRefCount.find(parentPrimPath);
    if (it == _targetedPrimToNameToRefCount.end()) {
        return nullptr;
    }
    
    const TfToken coordSysName(SdfPath::StripNamespace(primName));
    const auto it2 = it->second.find(coordSysName);
    if (it2 == it->second.end()) {
        return nullptr;
    }
    
    return _CoordSysPrimDataSource::New(
        _GetInputSceneIndex(), parentPrimPath, coordSysName);
}

HdSceneIndexPrim
HdsiCoordSysPrimSceneIndex::GetPrim(const SdfPath &primPath) const
{
    if (HdContainerDataSourceHandle const coordSysPrimSource =
                _GetCoordSysPrimSource(primPath)) {
        return { HdSprimTypeTokens->coordSys, coordSysPrimSource };
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource = _PrimDataSource::New(prim.dataSource);
    }
    return prim;
}

SdfPathVector
HdsiCoordSysPrimSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    SdfPathVector result = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    const auto it = _targetedPrimToNameToRefCount.find(primPath);
    if (it == _targetedPrimToNameToRefCount.end()) {
        return result;
    }
    for (const auto &nameAndRefCount : it->second) {
        const TfToken &coordSysName = nameAndRefCount.first;
        result.push_back(_PathForCoordSysPrim(primPath, coordSysName));
    }

    return result;
}

void
HdsiCoordSysPrimSceneIndex::_AddBindingsForPrim(
    const SdfPath &primPath,
    SdfPathSet * const addedCoordSysPrims)
{
    HdCoordSysBindingSchema schema =
        HdCoordSysBindingSchema::GetFromParent(
            _GetInputSceneIndex()->GetPrim(primPath).dataSource);
    if (!schema) {
        return;
    }

    _Bindings bindings;
    for (const TfToken &name : schema.GetContainer()->GetNames()) {
        HdPathDataSourceHandle const ds = schema.GetCoordSysBinding(name);
        if (!ds) {
            continue;
        }
        const SdfPath targetedPrimPath = ds->GetTypedValue(0.0f);
        if (_IgnoreBinding(targetedPrimPath)) {
            continue;
        }

        size_t &refCount =
            _targetedPrimToNameToRefCount[targetedPrimPath][name];
        if (refCount == 0) {
            if (addedCoordSysPrims) {
                addedCoordSysPrims->insert(
                    _PathForCoordSysPrim(targetedPrimPath, name));
            }
        }
        refCount++;

        bindings.push_back({name, targetedPrimPath});
    }

    _primToBindings.emplace(primPath, std::move(bindings));
}

void
HdsiCoordSysPrimSceneIndex::_RemoveBindings(
    const _Bindings &bindings,
    SdfPathSet * const removedCoordSysPrims)
{
    for (const _Binding &binding : bindings) {
        auto primIt = _targetedPrimToNameToRefCount.find(binding.path);
        if (primIt == _targetedPrimToNameToRefCount.end()) {
            TF_CODING_ERROR(
                "No ref-counting entry for targeted prim "
                "when deleting binding.");
            continue;
        }
        auto refCountIt = primIt->second.find(binding.name);
        if (refCountIt == primIt->second.end()) {
            TF_CODING_ERROR(
                "No ref-counting entry for target prim and binding name "
                "when deleting binding.");
            continue;
        }

        size_t &refCount = refCountIt->second;
        if (refCount == 0) {
            TF_CODING_ERROR(
                "Zero ref count for target prim and binding name "
                "when deleting binding.");
            continue;
        }
        refCount--;
        if (refCount > 0) {
            continue;
        }
        if (removedCoordSysPrims) {
            removedCoordSysPrims->insert(
                _PathForCoordSysPrim(binding.path, binding.name));
        }
        primIt->second.erase(refCountIt);
        if (!primIt->second.empty()) {
            continue;
        }
        _targetedPrimToNameToRefCount.erase(primIt);
    }
}

void
HdsiCoordSysPrimSceneIndex::_RemoveBindingsForPrim(
    const SdfPath &primPath,
    SdfPathSet * const removedCoordSysPrims)
{
    const auto it = _primToBindings.find(primPath);
    if (it == _primToBindings.end()) {
        return;
    }

    _RemoveBindings(it->second, removedCoordSysPrims);

    _primToBindings.erase(it);
}

void
HdsiCoordSysPrimSceneIndex::_RemoveBindingsForSubtree(
    const SdfPath &primPath,
    SdfPathSet * const removedCoordSysPrims)
{
    auto it = _primToBindings.lower_bound(primPath);
    while (it != _primToBindings.end() && it->first.HasPrefix(primPath)) {
        _RemoveBindings(it->second, removedCoordSysPrims);
        it = _primToBindings.erase(it);
    }
}    

void
HdsiCoordSysPrimSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    SdfPathSet addedCoordSysPrims;
    SdfPathSet removedCoordSysPrims;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        _RemoveBindingsForPrim(
            entry.primPath,
            isObserved ? &removedCoordSysPrims : nullptr);
        _AddBindingsForPrim(
            entry.primPath,
            isObserved ? &addedCoordSysPrims : nullptr);
    }

    if (!isObserved) {
        return;
    }

    _SendPrimsAdded(entries);

    if (!addedCoordSysPrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedCoordSysPrims));
    }
    if (!removedCoordSysPrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedCoordSysPrims));
    }
}

void
HdsiCoordSysPrimSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    SdfPathSet addedCoordSysPrims;
    SdfPathSet removedCoordSysPrims;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Intersects(
                HdCoordSysBindingSchema::GetDefaultLocator())) {
            _RemoveBindingsForPrim(
                entry.primPath,
                isObserved ? &removedCoordSysPrims : nullptr);
            _AddBindingsForPrim(
                entry.primPath,
                isObserved ? &addedCoordSysPrims : nullptr);
        }
    }
    
    if (!isObserved) {
        return;
    }

    _SendPrimsDirtied(entries);

    if (!addedCoordSysPrims.empty()) {
        _SendPrimsAdded(_ToAddedPrimEntries(addedCoordSysPrims));
    }
    if (!removedCoordSysPrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedCoordSysPrims));
    }
}
   
void
HdsiCoordSysPrimSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    SdfPathSet removedCoordSysPrims;

    if (!_primToBindings.empty()) {
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            _RemoveBindingsForSubtree(
                entry.primPath,
                isObserved ? &removedCoordSysPrims : nullptr);
        }
    }

    if (!isObserved) {
        return;
    }

    _SendPrimsRemoved(entries);

    if (!removedCoordSysPrims.empty()) {
        _SendPrimsRemoved(_ToRemovedPrimEntries(removedCoordSysPrims));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
