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

#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/trace/trace.h"

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


// Gives niPrototypePath from UsdImagingUsdPrimInfoSchema.
SdfPath
_GetUsdPrototypePath(HdContainerDataSourceHandle const &primSource)
{
    UsdImagingUsdPrimInfoSchema schema =
        UsdImagingUsdPrimInfoSchema::GetFromParent(primSource);
    HdPathDataSourceHandle const pathDs = schema.GetNiPrototypePath();
    if (!pathDs) {
        return SdfPath();
    }
    return pathDs->GetTypedValue(0.0f);
}

// Gives the name of niPrototypePath from UsdImagingUsdPrimInfoSchema.
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
                .AppendChild(UsdImagingTokens->propagatedPrototypesScope)
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

    using _PathToInt = std::map<SdfPath, int>;
    using _PathToIntSharedPtr = std::shared_ptr<_PathToInt>;
    using _PathToPathToInt = std::map<SdfPath, _PathToIntSharedPtr>;

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

    enum class _RemovalLevel : unsigned char {
        None = 0,
        Instance = 1,
        Instancer = 2,
        BindingScope = 3,
        EnclosingPrototypeRoot = 4
    };

    // Given path of an instance and its info,
    // removes corresponding entry from _infoToInstance map.
    // The map is nested several levels deep and the function
    // will erase entries that have become empty. The return
    // value describes how deep this erasure was.
    _RemovalLevel _RemoveInstanceFromInfoToInstance(
        const SdfPath &primPath,
        const _InstanceInfo &info);

    // Reset given pointer to nullptr. But before that, send
    // prim dirtied for all instances. The data source locator
    // of the prim dirtied message will be instance.
    //
    // This is called when instances have been added or removed
    // to instancers to account for the fact that the id
    // of potentially every instance might have changed.
    void _DirtyInstancesAndResetPointer(
        _PathToIntSharedPtr * const instanceToIndex);

    // Get prim data source for the named USD instance.
    HdContainerDataSourceHandle _GetDataSourceForInstance(
        const SdfPath &primPath);

    // Get data source for instance data source locator for
    // an instance.
    HdContainerDataSourceHandle _GetInstanceSchemaDataSource(
        const SdfPath &primPath);

    // Given path of an instance and its info, get its index.
    // That is, the index into the
    // instancer's instancerTopology's instanceIndices that
    // corresponds to this instance.
    int _GetInstanceIndex(
        const _InstanceInfo &info,
        const SdfPath &instancePath);

    // Given instance info identifying an instancer, get
    // the instance to instance id map. That is, compute
    // it if necessary.
    _PathToIntSharedPtr _GetInstanceToIndex(
        const _InstanceInfo &info);

    // Given instance info identifying an instancer, compute
    // the instance to instance id map.
    _PathToIntSharedPtr _ComputeInstanceToIndex(
        const _InstanceInfo &info);

    // This observer wants to observe a *flattened* view of the scene.  This
    // way, it can access the composed transform values for natively instanced
    // prims.
    HdSceneIndexBaseRefPtr const _flattenedInputScene;
    HdRetainedSceneIndexRefPtr const _retainedSceneIndex;
    const SdfPath _fallbackPrototypeRoot;
    HdContainerDataSourceHandle const _fallbackInstancedBySource;
    _CurriedInstanceInfoToInstance _infoToInstance;
    _PathToInstanceInfo _instanceToInfo;

    // _instancerToInstanceToIndex is populated lazily (per
    // instancer). That is, it has an entry for each instancer,
    // but the entry might be a nullptr until a client
    // has queried an instance for its instance data source.
    // We also only send out dirty entries for instances if
    // the entry was populated.
    //
    // This laziness avoids an N^2 invalidation behavior during
    // population. That is: if we added the N-th instance, we
    // potentially need to send out a dirty notice for every
    // previous instance since its id might have been affected.
    _PathToPathToInt _instancerToInstanceToIndex;
};

_InstanceObserver::_InstanceObserver(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &prototypeRoot)
  : _flattenedInputScene(HdFlatteningSceneIndex::New(inputScene))
  , _retainedSceneIndex(HdRetainedSceneIndex::New())
  , _fallbackPrototypeRoot(
        prototypeRoot.IsEmpty()
        ? SdfPath::AbsoluteRootPath()
        : prototypeRoot)
  , _fallbackInstancedBySource(
        _ComputeFallbackInstancedByDataSource(prototypeRoot))
{
    _Populate();
    _flattenedInputScene->AddObserver(HdSceneIndexObserverPtr(this));
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
                UsdImagingUsdPrimInfoSchema::GetNiPrototypePathLocator()};

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
             : HdSceneIndexPrimView(_flattenedInputScene,
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
    return _GetInfo(_flattenedInputScene->GetPrim(primPath).dataSource);
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
                    _flattenedInputScene->GetPrim(primPath).dataSource) } } );
    }

    const SdfPath instancerPath =
        info.GetInstancerPath();

    std::shared_ptr<SdfPathSet> &instances =
        prototypeNameToInstances[info.prototypeName];
    if (instances) {
        static const HdDataSourceLocatorSet locators{
            HdInstancerTopologySchema::GetDefaultLocator().Append(
                HdInstancerTopologySchemaTokens->instanceIndices),
            HdPrimvarsSchema::GetDefaultLocator()};

        _retainedSceneIndex->DirtyPrims(
            { { instancerPath, locators } });
    } else {
        instances = std::make_shared<SdfPathSet>();

        const SdfPath prototypePath =
            instancerPath.AppendChild(info.prototypeName);

        _retainedSceneIndex->AddPrims(
            { { instancerPath,
                HdPrimTypeTokens->instancer,
                _InstancerPrimSource::New(
                    _flattenedInputScene,
                    info.enclosingPrototypeRoot,
                    prototypePath,
                    instances,
                    _fallbackInstancedBySource) } });
    }

    instances->insert(primPath);

    _instanceToInfo[primPath] = info;

    // Add (lazy) instance data source to instance.
    _retainedSceneIndex->AddPrims(
        { { primPath,
            TfToken(),
            _GetDataSourceForInstance(primPath) } });

    // Create entry for instancer if not already present.
    //
    // Dirty instances (if previous non-null entry existed)
    // since the indices of potentially every other instance realized
    // by this instancer might have changed.
    _DirtyInstancesAndResetPointer(&_instancerToInstanceToIndex[instancerPath]);
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
    const _InstanceInfo &info = it->second;

    const SdfPath instancerPath = info.GetInstancerPath();


    const _RemovalLevel level =
        _RemoveInstanceFromInfoToInstance(primPath, info);

    if (level > _RemovalLevel::None) {
        // Remove instance data source we added in _AddInstance.
        _retainedSceneIndex->RemovePrims(
            { { primPath } });
    }

    if (level == _RemovalLevel::Instance) {
        // Instancer's data have changed because we removed
        // one of its instances.
        static const HdDataSourceLocatorSet locators{
            HdInstancerTopologySchema::GetDefaultLocator().Append(
                HdInstancerTopologySchemaTokens->instanceIndices),
            HdPrimvarsSchema::GetDefaultLocator()};
        _retainedSceneIndex->DirtyPrims(
            { { instancerPath, locators } });

        // The indices of potentially every other instance realized
        // by this instancer might have changed.
        auto it2 = _instancerToInstanceToIndex.find(instancerPath);
        if (it2 != _instancerToInstanceToIndex.end()) {
            _DirtyInstancesAndResetPointer(&it2->second);
        }
    }

    if (level >= _RemovalLevel::Instancer) {
        // Last instance for this instancer disappeared.
        // Remove instancer.
        _retainedSceneIndex->RemovePrims(
            { { instancerPath } });
        // And corresponding entry from map caching
        // instance indices.
        _instancerToInstanceToIndex.erase(instancerPath);
    }

    if (level >= _RemovalLevel::BindingScope) {
        // The last instancer under the prim grouping instancers
        // by material binding, ... has disappeared.
        // Remove grouping prim.
        _retainedSceneIndex->RemovePrims(
            { { info.GetBindingPrimPath() } });
    }

    return _instanceToInfo.erase(it);
}

_InstanceObserver::_RemovalLevel
_InstanceObserver::_RemoveInstanceFromInfoToInstance(
    const SdfPath &primPath,
    const _InstanceInfo &info)
{
    auto it0 = _infoToInstance.find(info.enclosingPrototypeRoot);
    if (it0 == _infoToInstance.end()) {
        return _RemovalLevel::None;
    }

    {
        auto it1 = it0->second.find(info.bindingHash);
        if (it1 == it0->second.end()) {
            return _RemovalLevel::None;
        }

        {
            auto it2 = it1->second.find(info.prototypeName);
            if (it2 == it1->second.end()) {
                return _RemovalLevel::None;
            }

            it2->second->erase(primPath);

            if (!it2->second->empty()) {
                return _RemovalLevel::Instance;
            }

            it1->second.erase(it2);
        }
    
        if (!it1->second.empty()) {
            return _RemovalLevel::Instancer;
        }
        
        it0->second.erase(it1);
    }

    if (!it0->second.empty()) {
        return _RemovalLevel::BindingScope;
    }

    _infoToInstance.erase(it0);

    return _RemovalLevel::EnclosingPrototypeRoot;
}

void
_InstanceObserver::_DirtyInstancesAndResetPointer(
    _PathToIntSharedPtr * const instanceToIndex)
{
    if (!*instanceToIndex) {
        return;
    }

    _PathToIntSharedPtr original = *instanceToIndex;
    // Invalidate pointer before sending clients a prim dirty so
    // that a prim dirty handler wouldn't pick up the stale data.
    *instanceToIndex = nullptr;

    for (const auto &instanceAndIndex : *original) {
        static const HdDataSourceLocatorSet locators{
            HdInstanceSchema::GetDefaultLocator()};
        _retainedSceneIndex->DirtyPrims(
            { { instanceAndIndex.first, locators } });
    }

}

HdContainerDataSourceHandle
_InstanceObserver::_GetDataSourceForInstance(
    const SdfPath &primPath)
{
    // Note that the _InstanceObserver has a strong reference
    // to the retained scene index which in turn has a strong
    // reference to the data source returned here.
    // Thus, the data source should hold on to a weak rather
    // than a strong reference to avoid a cycle.
    //
    // Such a cycle can yield to two problems:
    // It can obviously create a memory leak. However, it can
    // also yield a crash because the _InstanceObserver can stay
    // alive and listen to prims removed messages as scene index
    // observer. The _InstanceObserver can react to such a
    // message by deleting a prim from the retained scene index and
    // thus breaking the cycle causing the _InstanceObserver to
    // be destroyed while being in the middle of the _PrimsRemoved
    // call.
    //
    _InstanceObserverPtr self(this);

    // PrimSource for instance
    return
        HdRetainedContainerDataSource::New(
            HdInstanceSchemaTokens->instance,
            HdLazyContainerDataSource::New(
                [ self, primPath ] () {
                    if (self) {
                        return self->_GetInstanceSchemaDataSource(primPath);
                    } else {
                        return HdContainerDataSourceHandle();
                    }}));
}

HdContainerDataSourceHandle
_InstanceObserver::_GetInstanceSchemaDataSource(
    const SdfPath &primPath)
{
    auto it = _instanceToInfo.find(primPath);
    if (it == _instanceToInfo.end()) {
        return nullptr;
    }

    const _InstanceInfo &info = it->second;

    // The instance aggregation scene index never generates an
    // instancer with more than one prototype.
    static HdIntDataSourceHandle const prototypeIndexDs =
        HdRetainedTypedSampledDataSource<int>::New(0);

    return
        HdInstanceSchema::Builder()
            .SetInstancer(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    info.GetInstancerPath()))
            .SetPrototypeIndex(prototypeIndexDs)
            .SetInstanceIndex(
                HdRetainedTypedSampledDataSource<int>::New(
                    _GetInstanceIndex(info, primPath)))
            .Build();
}

int
_InstanceObserver::_GetInstanceIndex(
    const _InstanceInfo &info,
    const SdfPath &instancePath)
{
    TRACE_FUNCTION();

    _PathToIntSharedPtr const instanceToIndex = _GetInstanceToIndex(info);
    if (!instanceToIndex) {
        return -1;
    }

    auto it = instanceToIndex->find(instancePath);
    if (it == instanceToIndex->end()) {
        return -1;
    }

    return it->second;
}

_InstanceObserver::_PathToIntSharedPtr
_InstanceObserver::_GetInstanceToIndex(
    const _InstanceInfo &info)
{
    TRACE_FUNCTION();

    auto it = _instancerToInstanceToIndex.find(info.GetInstancerPath());
    if (it == _instancerToInstanceToIndex.end()) {
        // Entry (albeit nullptr) should for instancer should
        // have been added by _AddInstance.
        return nullptr;
    }

    // Check whether we have cached the result already.
    _PathToIntSharedPtr result = std::atomic_load(&it->second);
    if (!result) {
        // Compute if necessary.
        result = _ComputeInstanceToIndex(info);
        std::atomic_store(&it->second, result);
    }
    return result;
}

_InstanceObserver::_PathToIntSharedPtr
_InstanceObserver::_ComputeInstanceToIndex(
    const _InstanceInfo &info)
{
    TRACE_FUNCTION();

    _PathToIntSharedPtr result = std::make_shared<std::map<SdfPath, int>>();

    auto it0 = _infoToInstance.find(info.enclosingPrototypeRoot);
    if (it0 == _infoToInstance.end()) {
        return result;
    }
        
    auto it1 = it0->second.find(info.bindingHash);
    if (it1 == it0->second.end()) {
        return result;
    }

    auto it2 = it1->second.find(info.prototypeName);
    if (it2 == it1->second.end()) {
        return result;
    }

    // Compute the indices.
    int i = 0;
    for (const SdfPath &instancePath : *(it2->second)) {
        (*result)[instancePath] = i;
        i++;
    }

    return result;
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

