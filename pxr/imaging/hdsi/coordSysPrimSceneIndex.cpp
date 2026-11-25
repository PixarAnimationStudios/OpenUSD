//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/coordSysPrimSceneIndex.h"

#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/imaging/hd/mapContainerDataSource.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/work/loops.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((coordSysPrimPrefix, "__coordSys_"))
    (xformDependency)
    ((coordSysBindingDependencyPrefix, "coordSysBinding_"))
    (coordSysBindingDependenciesDependency)
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
// E.g. </PATH/__coordSys_FOO>
SdfPath
_PathForCoordSysPrim(const SdfPath &targetedPrimPath,
                     const TfToken &name)
{
    const TfToken childPrimName(
        _tokens->coordSysPrimPrefix.GetString() + name.GetString());
    return targetedPrimPath.AppendChild(childPrimName);
}

// Name of entry in __dependencies for coord sys binding with given name.
TfToken
_DependencyNameForBinding(const TfToken &name)
{
    return TfToken(
        _tokens->coordSysBindingDependencyPrefix.GetString() +
        name.GetString());
}

void
_AddIfNecessary(const TfToken &name, TfTokenVector * const names)
{
    if (std::find(names->begin(), names->end(), name) == names->end()) {
        names->push_back(name);
    }
}

// Data source for locator coordSys:FOO on a prim
// </PATH/__coordSys_FOO> where </PATH> is a path targeted by a
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
    _CoordSysPrimDataSource(const HdSceneIndexBasePtr &inputScene,
                            const SdfPath &primPath,
                            const TfToken &name)
     : _inputScene(inputScene)
     , _primPath(primPath)
     , _name(name)
    {
    }

    HdSceneIndexBasePtr const _inputScene;
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

// Data source for locator __dependencies on a prim with coord sys bindings.
//
// For each coord sys binding, lists a dependency of the binding on
// the coordSys locator of the coord sys prim this scene index is adding.
// Also lists a dependency of these dependencies on the bindings
// themselves.
class _CoordSysBindingDependenciesDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_CoordSysBindingDependenciesDataSource);

private:
    _CoordSysBindingDependenciesDataSource(
        const HdContainerDataSourceHandle &inputSource)
     : _inputSource(inputSource)
    {
    }

    TfTokenVector GetNames() override {
        TfTokenVector result;
        for (const TfToken &name : _inputSource->GetNames()) {
            if (!_GetTargetedPrimPath(name).IsEmpty()) {
                result.push_back(_DependencyNameForBinding(name));
            }
        }
        result.push_back(_tokens->coordSysBindingDependenciesDependency);
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        static HdLocatorDataSourceHandle const bindingLocatorDs =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdCoordSysBindingSchema::GetDefaultLocator());

        // The dependencies we compute depend themselves on the
        // coordinate system bindings.
        if (name == _tokens->coordSysBindingDependenciesDependency) {
            static HdLocatorDataSourceHandle const dependenciesLocatorDs =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator());
            static HdDataSourceBaseHandle const dependencyDs =
                HdDependencySchema::Builder()
                    // The dependencies depend on the coord sys bindings
                    // on this same prim.
                    .SetDependedOnDataSourceLocator(bindingLocatorDs)
                    .SetAffectedDataSourceLocator(dependenciesLocatorDs)
                    .Build();
            return dependencyDs;
        }

        // For each coordinate system binding on this prim, we include
        // a dependency on the target prim.
        const std::string &prefix =
            _tokens->coordSysBindingDependencyPrefix.GetString();
        if (!TfStringStartsWith(name.GetString(), prefix)) {
            return nullptr;
        }
        const TfToken bindingName(name.GetString().substr(prefix.size()));
        const SdfPath targetedPrimPath = _GetTargetedPrimPath(bindingName);
        if (targetedPrimPath.IsEmpty()) {
            return nullptr;
        }
        static HdLocatorDataSourceHandle const coordSysLocatorDs =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdCoordSysSchema::GetDefaultLocator());
        return
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(
                    HdRetainedTypedSampledDataSource<SdfPath>::New(
                        _PathForCoordSysPrim(targetedPrimPath, bindingName)))
                .SetDependedOnDataSourceLocator(coordSysLocatorDs)
                .SetAffectedDataSourceLocator(bindingLocatorDs)
                .Build();
    }

    // Path of prim targeted by binding with given name or empty path
    // if binding is ignored.
    SdfPath _GetTargetedPrimPath(const TfToken &name) const {
        HdPathDataSourceHandle const ds =
            HdPathDataSource::Cast(_inputSource->Get(name));
        if (!ds) {
            return SdfPath();
        }
        const SdfPath targetedPrimPath = ds->GetTypedValue(0.0f);
        if (_IgnoreBinding(targetedPrimPath)) {
            return SdfPath();
        }
        return targetedPrimPath;
    }

    HdContainerDataSourceHandle const _inputSource;
};

// Prim data source rewriting coord sys bindings to point to
// coord sys prim this scene index is adding and adding dependencies
// of the bindings on these coord sys prims.
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override {
        TfTokenVector result = _inputSource->GetNames();
        if (std::find(result.begin(), result.end(),
                      HdCoordSysBindingSchema::GetSchemaToken())
                != result.end()) {
            _AddIfNecessary(HdDependenciesSchema::GetSchemaToken(), &result);
        }
        return result;
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
        if (name == HdDependenciesSchema::GetSchemaToken()) {
            HdContainerDataSourceHandle const bindingDs =
                HdContainerDataSource::Cast(
                    _inputSource->Get(
                        HdCoordSysBindingSchema::GetSchemaToken()));
            if (!bindingDs) {
                return ds;
            }
            return
                HdCreateOverlayContainerDataSource(
                    HdContainerDataSource::Cast(ds),
                    _CoordSysBindingDependenciesDataSource::New(bindingDs));
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
_AddedCoordSysEntries(const SdfPathSet &paths)
{
    HdSceneIndexObserver::AddedPrimEntries entries;
    entries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        entries.push_back({path, HdSprimTypeTokens->coordSys});
    }
    return entries;
}

HdSceneIndexObserver::RemovedPrimEntries
_RemovedCoordSysEntries(const SdfPathSet &paths)
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

    static const std::string &prefix = _tokens->coordSysPrimPrefix.GetString();
    if (!TfStringStartsWith(primName, prefix)) {
        return nullptr;
    }

    const SdfPath parentPrimPath = primPath.GetParentPath();

    const auto it = _targetedPrimToNameToSourcePrims.find(parentPrimPath);
    if (it == _targetedPrimToNameToSourcePrims.end()) {
        return nullptr;
    }
    
    const TfToken coordSysName(
        primName.substr( _tokens->coordSysPrimPrefix.size() ));
    const auto it2 = it->second.find(coordSysName);
    if (it2 == it->second.end()) {
        return nullptr;
    }

    // Require that the parent prim exists in order to generate
    // a child coordinate system prim.  This ensures that if we
    // subsequently remove the target prim of a coordinate system
    // binding, or any of its ancestors, that we no longer return
    // that coordinate system prim.
    if (!_GetInputSceneIndex()->GetPrim(parentPrimPath)) {
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

    const auto it = _targetedPrimToNameToSourcePrims.find(primPath);
    if (it == _targetedPrimToNameToSourcePrims.end()) {
        return result;
    }
    for (const auto &nameAndSourcePrims : it->second) {
        const TfToken &coordSysName = nameAndSourcePrims.first;
        result.push_back(_PathForCoordSysPrim(primPath, coordSysName));
    }

    return result;
}

void
HdsiCoordSysPrimSceneIndex::_AddBindingsForPrim(
    const SdfPath &primPath,
    SdfPathSet * const addedCoordSysPrims)
{
    _AddBindings(
        primPath, _ComputeBindingsForPrim(primPath), addedCoordSysPrims);
}

HdsiCoordSysPrimSceneIndex::_Bindings
HdsiCoordSysPrimSceneIndex::_ComputeBindingsForPrim(
    const SdfPath &primPath) const
{
    _Bindings bindings;

    HdCoordSysBindingSchema schema =
        HdCoordSysBindingSchema::GetFromParent(
            _GetInputSceneIndex()->GetPrim(primPath).dataSource);
    if (!schema) {
        return bindings;
    }

    for (const TfToken &name : schema.GetContainer()->GetNames()) {
        HdPathDataSourceHandle const ds = schema.GetCoordSysBinding(name);
        if (!ds) {
            continue;
        }
        const SdfPath targetedPrimPath = ds->GetTypedValue(0.0f);
        if (_IgnoreBinding(targetedPrimPath)) {
            continue;
        }
        bindings.push_back({name, targetedPrimPath});
    }

    return bindings;
}

void
HdsiCoordSysPrimSceneIndex::_AddBindings(
    const SdfPath &primPath,
    _Bindings &&bindings,
    SdfPathSet * const addedCoordSysPrims)
{
    if (bindings.empty()) {
        return;
    }

    for (const _Binding &binding : bindings) {
        SdfPathSet &sourcePrims =
            _targetedPrimToNameToSourcePrims[binding.path][binding.name];
        if (sourcePrims.empty()) {
            if (addedCoordSysPrims) {
                // XXX Should we be confirming that the binding.path
                // actually exists in the scene before generating
                // children below it?  Consider the case of coordinate
                // system bindings to non-existent prims.
                addedCoordSysPrims->insert(
                    _PathForCoordSysPrim(binding.path, binding.name));
            }
        }
        sourcePrims.insert(primPath);
    }

    _primToBindings.emplace(primPath, std::move(bindings));
}

void
HdsiCoordSysPrimSceneIndex::_RemoveBindings(
    const SdfPath &primPath,
    const _Bindings &bindings,
    SdfPathSet * const removedCoordSysPrims)
{
    for (const _Binding &binding : bindings) {
        auto primIt = _targetedPrimToNameToSourcePrims.find(binding.path);
        if (primIt == _targetedPrimToNameToSourcePrims.end()) {
            TF_CODING_ERROR(
                "No entry for targeted prim when deleting binding.");
            continue;
        }
        auto sourcePrimsIt = primIt->second.find(binding.name);
        if (sourcePrimsIt == primIt->second.end()) {
            TF_CODING_ERROR(
                "No entry for target prim and binding name "
                "when deleting binding.");
            continue;
        }

        SdfPathSet &sourcePrims = sourcePrimsIt->second;
        if (sourcePrims.erase(primPath) == 0) {
            TF_CODING_ERROR(
                "Prim not recorded as source for target prim and "
                "binding name when deleting binding.");
            continue;
        }
        if (!sourcePrims.empty()) {
            continue;
        }
        if (removedCoordSysPrims) {
            removedCoordSysPrims->insert(
                _PathForCoordSysPrim(binding.path, binding.name));
        }
        primIt->second.erase(sourcePrimsIt);
        if (!primIt->second.empty()) {
            continue;
        }

        // Tidy up disused SdfPathTable entries.  SdfPathTable implicitly
        // adds entries for ancestors, so walk up, removing empty entries.
        SdfPath path = binding.path;
        while (!path.IsEmpty()) {
            const auto it = _targetedPrimToNameToSourcePrims.find(path);
            if (it == _targetedPrimToNameToSourcePrims.end() ||
                !it->second.empty() || it.HasChild()) {
                break;
            }
            _targetedPrimToNameToSourcePrims.erase(it);
            path = path.GetParentPath();
        }
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

    _RemoveBindings(primPath, it->second, removedCoordSysPrims);

    _primToBindings.erase(it);
}

void
HdsiCoordSysPrimSceneIndex::_RemoveBindingsForSubtree(
    const SdfPath &primPath,
    SdfPathSet * const removedCoordSysPrims)
{
    auto it = _primToBindings.lower_bound(primPath);
    while (it != _primToBindings.end() && it->first.HasPrefix(primPath)) {
        _RemoveBindings(it->first, it->second, removedCoordSysPrims);
        it = _primToBindings.erase(it);
    }
}    

void
HdsiCoordSysPrimSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Processing coordinate systems");

    const bool isObserved = _IsObserved();

    SdfPathSet addedCoordSysPrims;
    SdfPathSet removedCoordSysPrims;

    // Querying the input scene for bindings is expensive, so compute the
    // bindings of the added prims in parallel.
    std::vector<_Bindings> bindings(entries.size());
    WorkParallelForN(entries.size(), [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            bindings[i] = _ComputeBindingsForPrim(entries[i].primPath);
        }
    }, /* grainSize = */ 16);

    // Update the binding data structures serially.
    for (size_t i = 0; i < entries.size(); ++i) {
        const SdfPath &primPath = entries[i].primPath;
        _RemoveBindingsForPrim(
            primPath,
            isObserved ? &removedCoordSysPrims : nullptr);
        _AddBindings(
            primPath,
            std::move(bindings[i]),
            isObserved ? &addedCoordSysPrims : nullptr);
    }

    if (!isObserved) {
        return;
    }

    // If we added prims that were previously discovered to be the target
    // of a coordinate system binding, add the generated coordSys prims.
    for (size_t i = 0; i < entries.size(); ++i) {
        const SdfPath &primPath = entries[i].primPath;
        const auto coordSysIt =
            _targetedPrimToNameToSourcePrims.find(primPath);
        if (coordSysIt != _targetedPrimToNameToSourcePrims.end()) {
            for (const auto& entry: coordSysIt->second) {
                addedCoordSysPrims.insert(
                    _PathForCoordSysPrim(primPath, entry.first));
            }
        }
    }

    // Skip removing coordinate systems that will be re-added.
    for (SdfPath const& addedPath: addedCoordSysPrims) {
        removedCoordSysPrims.erase(addedPath);
    }

    _SendPrimsAdded(entries);

    if (!addedCoordSysPrims.empty()) {
        _SendPrimsAdded(_AddedCoordSysEntries(addedCoordSysPrims));
    }
    if (!removedCoordSysPrims.empty()) {
        _SendPrimsRemoved(_RemovedCoordSysEntries(removedCoordSysPrims));
    }
}

void
HdsiCoordSysPrimSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();
    TF_DESCRIBE_SCOPE("Processing coordinate systems");

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

    // Skip removing coordinate systems that will be re-added.
    for (SdfPath const& addedPath: addedCoordSysPrims) {
        removedCoordSysPrims.erase(addedPath);
    }

    _SendPrimsDirtied(entries);

    if (!addedCoordSysPrims.empty()) {
        _SendPrimsAdded(_AddedCoordSysEntries(addedCoordSysPrims));
    }
    if (!removedCoordSysPrims.empty()) {
        _SendPrimsRemoved(_RemovedCoordSysEntries(removedCoordSysPrims));
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
        _SendPrimsRemoved(_RemovedCoordSysEntries(removedCoordSysPrims));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
