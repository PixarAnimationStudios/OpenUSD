//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/niInstanceAggregationSceneIndex.h"

#include "pxr/usdImaging/usdImaging/sceneIndexPrimView.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImaging_NiInstanceAggregationSceneIndex_Impl {

GfMatrix4d
_GetPrimTransform(
    const HdSceneIndexBaseRefPtr &sceneIndex, const SdfPath &primPath)
{
    static const GfMatrix4d id(1.0);
    if (!sceneIndex) {
        return id;
    }
    HdContainerDataSourceHandle const primSource =
        sceneIndex->GetPrim(primPath).dataSource;
    HdMatrixDataSourceHandle const ds =
        HdXformSchema::GetFromParent(primSource).GetMatrix();
    if (!ds) {
        return id;
    }
    return ds->GetTypedValue(0.0f);
}

class _InstanceTransformPrimvarValueDataSource : public HdMatrixArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstanceTransformPrimvarValueDataSource);

    VtValue GetValue(const Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
            const Time startTime,
            const Time endTime,
            std::vector<Time> * const outSampleTimes) override
    {
        // TODO: Support motion blur
        return false;
    }

    VtArray<GfMatrix4d> GetTypedValue(const Time shutterOffset) override
    {
        VtArray<GfMatrix4d> result(_instances->size());
        
        int i = 0;
        for (const SdfPath &instance : *_instances) {
            result[i] = _GetPrimTransform(_inputSceneIndex, instance);
            i++;
        }
        return result;
    }

private:
    _InstanceTransformPrimvarValueDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
};

class _InstanceTransformPrimvarDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstanceTransformPrimvarDataSource);

    TfTokenVector GetNames() override
    {
        return { HdPrimvarSchemaTokens->primvarValue,
                 HdPrimvarSchemaTokens->interpolation,
                 HdPrimvarSchemaTokens->role };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdPrimvarSchemaTokens->interpolation) {
            static HdDataSourceBaseHandle const ds =
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrimvarSchemaTokens->instance);
            return ds;
        }
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _InstanceTransformPrimvarValueDataSource::New(
                _inputSceneIndex, _instances);
        }
        // Does the instanceTransform have a role?
        return nullptr;
    }

private:
    _InstanceTransformPrimvarDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
};

class _PrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    TfTokenVector GetNames() override {
        return { HdInstancerTokens->instanceTransform };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdInstancerTokens->instanceTransform) {
            return _InstanceTransformPrimvarDataSource::New(
                _inputSceneIndex, _instances);
        }
        return nullptr;
    }

private:
    _PrimvarsDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
};

// [0, 1, ..., n-1]
VtArray<int>
_Range(const int n)
{
    VtArray<int> result(n);
    for (int i = 0; i < n; i++) {
        result[i] = i;
    }
    return result;
}

class _InstanceIndicesDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstanceIndicesDataSource);

    size_t GetNumElements() override
    {
        return 1;
    }

    HdDataSourceBaseHandle GetElement(size_t) override
    {
        const int n = _instances->size();
        return HdRetainedTypedSampledDataSource<VtArray<int>>::New(_Range(n));
    }

private:
    _InstanceIndicesDataSource(
        std::shared_ptr<SdfPathSet> const &instances)
      : _instances(instances)
    {
    }

    std::shared_ptr<SdfPathSet> const _instances;
};

class _InstanceLocationsDataSource : public HdPathArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstanceLocationsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
            const Time startTime,
            const Time endTime,
            std::vector<Time> * const outSampleTimes) override
    {
        return false;
    }

    VtArray<SdfPath> GetTypedValue(const Time shutterOffset) override {
        return { _instances->begin(), _instances->end() };
    }

private:
    _InstanceLocationsDataSource(
        std::shared_ptr<SdfPathSet> const &instances)
      : _instances(instances)
    {
    }

    std::shared_ptr<SdfPathSet> const _instances;
};

class _InstancerTopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstancerTopologyDataSource);

    TfTokenVector GetNames() override {
        return { HdInstancerTopologySchemaTokens->instanceIndices,
                 HdInstancerTopologySchemaTokens->prototypes,
                 HdInstancerTopologySchemaTokens->instanceLocations};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdInstancerTopologySchemaTokens->instanceIndices) {
            return _InstanceIndicesDataSource::New(_instances);
        }
        if (name == HdInstancerTopologySchemaTokens->prototypes) {
            return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                {_prototypePath});
        }
        if (name == HdInstancerTopologySchemaTokens->instanceLocations) {
            return _InstanceLocationsDataSource::New(_instances);
        }
        return nullptr;
    }

private:
    _InstancerTopologyDataSource(
        const SdfPath &prototypePath,
        std::shared_ptr<SdfPathSet> const &instances)
      : _prototypePath(prototypePath)
      , _instances(instances)
    {
    }

    const SdfPath _prototypePath;
    std::shared_ptr<SdfPathSet> const _instances;
};

class _InstancerPrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstancerPrimSource);

    TfTokenVector GetNames() override {
        return { HdInstancedBySchemaTokens->instancedBy,
                 HdInstancerTopologySchemaTokens->instancerTopology,
                 HdPrimvarsSchemaTokens->primvars};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdInstancedBySchemaTokens->instancedBy) {
            if (HdInstancedBySchema schema = HdInstancedBySchema::GetFromParent(
                    _inputSceneIndex->GetPrim(_enclosingPrototypeRoot)
                        .dataSource)) {
                return schema.GetContainer();
            }
            return _fallbackInstancedByDataSource;
        }
        if (name == HdInstancerTopologySchemaTokens->instancerTopology) {
            return _InstancerTopologyDataSource::New(
                _prototypePath, _instances);
        }
        if (name == HdPrimvarsSchemaTokens->primvars) {
            return _PrimvarsDataSource::New(
                    _inputSceneIndex, _instances);
        }
        return nullptr;
    }

private:
    _InstancerPrimSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &enclosingPrototypeRoot,
        const SdfPath &prototypePath,
        std::shared_ptr<SdfPathSet> const &instances,
        HdContainerDataSourceHandle const &fallbackInstancedByDataSource)
      : _inputSceneIndex(inputSceneIndex)
      , _enclosingPrototypeRoot(enclosingPrototypeRoot)
      , _prototypePath(prototypePath)
      , _instances(instances)
      , _fallbackInstancedByDataSource(fallbackInstancedByDataSource)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    const SdfPath _enclosingPrototypeRoot;
    const SdfPath _prototypePath;
    std::shared_ptr<SdfPathSet> const _instances;
    HdContainerDataSourceHandle const _fallbackInstancedByDataSource;
};

HdContainerDataSourceHandle
_ComputeFallbackInstancedByDataSource(const SdfPath &prototypeRoot)
{
    if (prototypeRoot.IsEmpty()) {
        return nullptr;
    } else {
        using DataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

        return
            HdInstancedBySchema::Builder()
                .SetPaths(DataSource::New({ SdfPath::AbsoluteRootPath() }))
                .SetPrototypeRoots(DataSource::New({ prototypeRoot }))
                .Build();
    }
}

// We should implement a generic data source hash and use it here.
size_t
_ComputeHash(HdContainerDataSourceHandle const &container)
{
    std::vector<std::pair<TfToken, SdfPath>> bindings;
    TfTokenVector names = container->GetNames();
    bindings.reserve(names.size());
    for (const TfToken &name : names) {
        if (HdPathDataSourceHandle const ds =
                    HdPathDataSource::Cast(container->Get(name))) {
            bindings.emplace_back(name, ds->GetTypedValue(0.0f));
        }
    }

    return TfHash::Combine(bindings);
}

TfToken
_GetBindingHash(HdContainerDataSourceHandle const &primSource)
{
    HdContainerDataSourceHandle container =
        HdMaterialBindingSchema::GetFromParent(primSource).GetContainer();
    if (!container) {
        return TfToken("NoBindings");
    }

    return TfToken(TfStringPrintf("Binding%zx", _ComputeHash(container)));
}


// Gives result of data source at "usdPrototypePath" or empty path.
SdfPath
_GetUsdPrototypePath(HdContainerDataSourceHandle const &primSource)
{
    if (!primSource) {
        return SdfPath();
    }
    HdPathDataSourceHandle const pathDs =
        HdPathDataSource::Cast(
            primSource->Get(
                UsdImagingNativeInstancingTokens->usdPrototypePath));
    if (!pathDs) {
        return SdfPath();
    }
    return pathDs->GetTypedValue(0.0f);
}

// Gives the name of the USD prototype obtained from the data source
// at "usdPrototypePath" - or empty token.
TfToken
_GetUsdPrototypeName(HdContainerDataSourceHandle const &primSource)
{
    const SdfPath prototypePath = _GetUsdPrototypePath(primSource);
    if (prototypePath.IsEmpty()) {
        return TfToken();
    }
    return prototypePath.GetNameToken();
}

SdfPath
_GetPrototypeRoot(HdContainerDataSourceHandle const &primSource)
{
    HdInstancedBySchema schema = HdInstancedBySchema::GetFromParent(
        primSource);
    HdPathArrayDataSourceHandle const ds = schema.GetPrototypeRoots();
    if (!ds) {
        return SdfPath();
    }
    const VtArray<SdfPath> result = ds->GetTypedValue(0.0f);
    if (result.empty()) {
        return SdfPath();
    }
    return result[0];
}

// We should implement a generic function to deep copy a data source
// and use it here.
HdDataSourceBaseHandle
_MakeCopy(HdDataSourceBaseHandle const &ds)
{
    if (!ds) {
        return nullptr;
    }
    if (HdContainerDataSourceHandle const container =
            HdContainerDataSource::Cast(ds)) {
        TfTokenVector names = container->GetNames();
        std::vector<HdDataSourceBaseHandle> items;
        items.reserve(names.size());
        for (const TfToken &name : names) {
            items.push_back(_MakeCopy(container->Get(name)));
        }
        return HdRetainedContainerDataSource::New(
            names.size(), names.data(), items.data());
    }
    if (HdPathDataSourceHandle const pathDs =
           HdPathDataSource::Cast(ds)) {
        return HdRetainedTypedSampledDataSource<SdfPath>::New(
            pathDs->GetTypedValue(0.0f));
    }

    TF_CODING_ERROR("Unknown data source type");

    return nullptr;
}

HdContainerDataSourceHandle
_MakeBindingCopy(HdContainerDataSourceHandle const &primSource)
{
    HdMaterialBindingSchema schema = HdMaterialBindingSchema::GetFromParent(
        primSource);
    return HdRetainedContainerDataSource::New(
        HdMaterialBindingSchemaTokens->materialBinding,
        _MakeCopy(schema.GetContainer()));
}

struct _InstanceInfo {
    // The root of the prototype that the instance is in.
    SdfPath enclosingPrototypeRoot;
    // The hash of the relevant bindings of an instance (e.g. material
    // bindings).
    TfToken bindingHash;
    // The name of the Usd prototype this instance is instancing.
    TfToken prototypeName;

    bool IsInstance() const { return !prototypeName.IsEmpty(); }
    
    SdfPath GetBindingPrimPath() const {
        return
            enclosingPrototypeRoot
                .AppendChild(UsdImagingNativeInstancingTokens->prototypesScope)
                .AppendChild(bindingHash);
    }

    SdfPath GetInstancerPath() const {
        return
            GetBindingPrimPath()
                .AppendChild(prototypeName);
    }
};

class _InstanceObserver : public HdSceneIndexObserver
{   
public:
    _InstanceObserver(HdSceneIndexBaseRefPtr const &inputScene,
                      const SdfPath &prototypeRoot);

    HdRetainedSceneIndexRefPtr const &GetRetainedSceneIndex() const {
        return _retainedSceneIndex;
    }

    void
    PrimsAdded(const HdSceneIndexBase &sender,
               const AddedPrimEntries &entries) override;
    
    void
    PrimsDirtied(const HdSceneIndexBase &sender,
                 const DirtiedPrimEntries &entries) override;
    
    void
    PrimsRemoved(const HdSceneIndexBase &sender,
                 const RemovedPrimEntries &entries) override;

private:
    using _Map0 = std::map<TfToken, std::shared_ptr<SdfPathSet>>;
    using _Map1 = std::map<TfToken, _Map0>;
    using _Map2 = std::map<SdfPath, _Map1>;
    using _CurriedInstanceInfoToInstance = _Map2;

    using _PathToInstanceInfo = std::map<SdfPath, _InstanceInfo>;

    _InstanceInfo _GetInfo(const HdContainerDataSourceHandle &primSource);
    _InstanceInfo _GetInfo(const SdfPath &primPath);

    void _Populate();
    void _AddPrim(const SdfPath &primPath);
    void _AddInstance(const SdfPath &primPath,
                      const _InstanceInfo &info);
    void _RemovePrim(
        const SdfPath &primPath);
    _PathToInstanceInfo::iterator _RemoveInstance(
        const SdfPath &primPath,
        const _PathToInstanceInfo::iterator &it);
    void _RemoveInstanceFromInfoToInstance(
        const SdfPath &primPath,
        const _InstanceInfo &info);

    HdSceneIndexBaseRefPtr const _inputScene;
    HdRetainedSceneIndexRefPtr const _retainedSceneIndex;
    SdfPath _fallbackPrototypeRoot;
    HdContainerDataSourceHandle const _fallbackInstancedBySource;
    _CurriedInstanceInfoToInstance _infoToInstance;
    _PathToInstanceInfo _instanceToInfo;
};

_InstanceObserver::_InstanceObserver(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &prototypeRoot)
  : _inputScene(inputScene)
  , _retainedSceneIndex(HdRetainedSceneIndex::New())
  , _fallbackPrototypeRoot(
        prototypeRoot.IsEmpty()
        ? SdfPath::AbsoluteRootPath()
        : prototypeRoot)
  , _fallbackInstancedBySource(
        _ComputeFallbackInstancedByDataSource(prototypeRoot))
{
    _Populate();
    _inputScene->AddObserver(HdSceneIndexObserverPtr(this));
}

void
_InstanceObserver::PrimsAdded(const HdSceneIndexBase &sender,
                              const AddedPrimEntries &entries)
{
    for (const AddedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        _RemovePrim(path);
        _AddPrim(path);
    }
}

void
_InstanceObserver::PrimsDirtied(const HdSceneIndexBase &sender,
                                const DirtiedPrimEntries &entries)
{
    if (_instanceToInfo.empty()) {
        return;
    }

    for (const DirtiedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;

        {
            static const HdDataSourceLocatorSet srcLocators{
                HdInstancedBySchema::GetDefaultLocator().Append(
                    HdInstancedBySchemaTokens->prototypeRoots),
                HdMaterialBindingSchema::GetDefaultLocator(),
                HdDataSourceLocator(
                    UsdImagingNativeInstancingTokens->usdPrototypePath)};

            if (entry.dirtyLocators.Intersects(srcLocators)) {
                _RemovePrim(path);
                _AddPrim(path);
            }
        }

        {
            static const HdDataSourceLocatorSet srcLocators{
                HdXformSchema::GetDefaultLocator()};

            if (entry.dirtyLocators.Intersects(srcLocators)) {

                auto it = _instanceToInfo.find(path);
                if (it != _instanceToInfo.end()) {
                    static const HdDataSourceLocatorSet dstLocators{
                        HdPrimvarsSchema::GetDefaultLocator()
                            .Append(HdInstancerTokens->instanceTransform)
                            .Append(HdPrimvarSchemaTokens->primvarValue)};

                    _retainedSceneIndex->DirtyPrims(
                        { { it->second.GetInstancerPath(),
                            dstLocators } } );
                }
            }
        }

    }
}

void
_InstanceObserver::PrimsRemoved(const HdSceneIndexBase &sender,
                                const RemovedPrimEntries &entries)
{
    if (_instanceToInfo.empty()) {
        return;
    }

    for (const RemovedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        auto it = _instanceToInfo.lower_bound(path);
        while (it != _instanceToInfo.end() &&
               it->first.HasPrefix(path)) {
            it = _RemoveInstance(path, it);
        }
    }
}

void
_InstanceObserver::_Populate()
{
    for (const SdfPath &primPath
             : UsdImaging_SceneIndexPrimView(_inputScene,
                                             SdfPath::AbsoluteRootPath())) {
        _AddPrim(primPath);
    }
}

_InstanceInfo
_InstanceObserver::_GetInfo(const HdContainerDataSourceHandle &primSource)
{
    _InstanceInfo result;

    result.prototypeName = _GetUsdPrototypeName(primSource);
    if (result.prototypeName.IsEmpty()) {
        return result;
    }

    result.enclosingPrototypeRoot = _GetPrototypeRoot(primSource);
    if (result.enclosingPrototypeRoot.IsEmpty()) {
        result.enclosingPrototypeRoot = _fallbackPrototypeRoot;
    }
    result.bindingHash = _GetBindingHash(primSource);

    return result;
}

_InstanceInfo
_InstanceObserver::_GetInfo(const SdfPath &primPath)
{
    return _GetInfo(_inputScene->GetPrim(primPath).dataSource);
}

void
_InstanceObserver::_AddInstance(const SdfPath &primPath,
                                const _InstanceInfo &info)
{
    _Map1 &bindingHashToPrototypeNameToInstances =
        _infoToInstance[info.enclosingPrototypeRoot];

    _Map0 &prototypeNameToInstances =
        bindingHashToPrototypeNameToInstances[info.bindingHash];

    if (prototypeNameToInstances.empty()) {
        _retainedSceneIndex->AddPrims(
            { { info.GetBindingPrimPath(),
                TfToken(),
                _MakeBindingCopy(
                    _inputScene->GetPrim(primPath).dataSource) } } );
    }

    std::shared_ptr<SdfPathSet> &instances =
        prototypeNameToInstances[info.prototypeName];
    if (instances) {
        static const HdDataSourceLocatorSet locators{
            HdInstancerTopologySchema::GetDefaultLocator().Append(
                HdInstancerTopologySchemaTokens->instanceIndices),
            HdPrimvarsSchema::GetDefaultLocator()};

        _retainedSceneIndex->DirtyPrims(
            { { info.GetInstancerPath(), locators } });
    } else {
        instances = std::make_shared<SdfPathSet>();

        const SdfPath instancerPath =
            info.GetInstancerPath();
        const SdfPath prototypePath =
            instancerPath.AppendChild(info.prototypeName);

        _retainedSceneIndex->AddPrims(
            { { instancerPath,
                HdPrimTypeTokens->instancer,
                _InstancerPrimSource::New(
                    _inputScene,
                    info.enclosingPrototypeRoot,
                    prototypePath,
                    instances,
                    _fallbackInstancedBySource) } });
    }

    instances->insert(primPath);

    _instanceToInfo[primPath] = info;
}

void
_InstanceObserver::_AddPrim(const SdfPath &primPath)
{
    const _InstanceInfo info = _GetInfo(primPath);
    if (info.IsInstance()) {
        _AddInstance(primPath, info);
    }
}

void
_InstanceObserver::_RemovePrim(const SdfPath &primPath)
{
    auto it = _instanceToInfo.find(primPath);
    if (it != _instanceToInfo.end()) {
        _RemoveInstance(primPath, it);
    }
}

_InstanceObserver::_PathToInstanceInfo::iterator
_InstanceObserver::_RemoveInstance(const SdfPath &primPath,
                                   const _PathToInstanceInfo::iterator &it)
{
    static const HdDataSourceLocatorSet locators{
        HdInstancerTopologySchema::GetDefaultLocator().Append(
            HdInstancerTopologySchemaTokens->instanceIndices),
        HdPrimvarsSchema::GetDefaultLocator()};

    _retainedSceneIndex->DirtyPrims(
        { { it->second.GetInstancerPath(), locators } });

    _RemoveInstanceFromInfoToInstance(primPath, it->second);
    return _instanceToInfo.erase(it);
}

void
_InstanceObserver::_RemoveInstanceFromInfoToInstance(
    const SdfPath &primPath,
    const _InstanceInfo &info)
{
    auto it0 = _infoToInstance.find(info.enclosingPrototypeRoot);
    if (it0 == _infoToInstance.end()) {
        return;
    }

    {
        auto it1 = it0->second.find(info.bindingHash);
        if (it1 == it0->second.end()) {
            return;
        }

        {
            auto it2 = it1->second.find(info.prototypeName);
            if (it2 == it1->second.end()) {
                return;
            }

            it2->second->erase(primPath);

            if (!it2->second->empty()) {
                return;
            }
    
            _retainedSceneIndex->RemovePrims(
                { { info.GetInstancerPath() } });
            it1->second.erase(it2);
        }
    
        if (!it1->second.empty()) {
            return;
        }
        
        _retainedSceneIndex->RemovePrims(
            { { info.GetBindingPrimPath() } });
        it0->second.erase(it1);
    }

    if (!it0->second.empty()) {
        return;
    }

    _infoToInstance.erase(it0);
}

}

using namespace UsdImaging_NiInstanceAggregationSceneIndex_Impl;

UsdImaging_NiInstanceAggregationSceneIndex::
UsdImaging_NiInstanceAggregationSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &prototypeRoot)
  : _instanceObserver(
        std::make_unique<_InstanceObserver>(inputScene, prototypeRoot))
  , _retainedSceneIndexObserver(this)
{
    _instanceObserver->GetRetainedSceneIndex()->AddObserver(
        HdSceneIndexObserverPtr(&_retainedSceneIndexObserver));
}

UsdImaging_NiInstanceAggregationSceneIndex::
~UsdImaging_NiInstanceAggregationSceneIndex() = default;

std::vector<HdSceneIndexBaseRefPtr>
UsdImaging_NiInstanceAggregationSceneIndex::GetInputScenes() const
{
    return { _instanceObserver->GetRetainedSceneIndex() };
}

HdSceneIndexPrim
UsdImaging_NiInstanceAggregationSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    return
        _instanceObserver->GetRetainedSceneIndex()->GetPrim(primPath);
}

SdfPathVector
UsdImaging_NiInstanceAggregationSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return
        _instanceObserver->GetRetainedSceneIndex()->GetChildPrimPaths(primPath);
}

UsdImaging_NiInstanceAggregationSceneIndex::
_RetainedSceneIndexObserver::_RetainedSceneIndexObserver(
    UsdImaging_NiInstanceAggregationSceneIndex * const owner)
  : _owner(owner)
{
}

void
UsdImaging_NiInstanceAggregationSceneIndex::
_RetainedSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    _owner->_SendPrimsAdded(entries);
}

void
UsdImaging_NiInstanceAggregationSceneIndex::
_RetainedSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    _owner->_SendPrimsDirtied(entries);
}

void
UsdImaging_NiInstanceAggregationSceneIndex::
_RetainedSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    _owner->_SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE

