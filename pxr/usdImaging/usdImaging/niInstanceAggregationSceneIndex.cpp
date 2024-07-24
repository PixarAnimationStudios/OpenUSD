//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/niInstanceAggregationSceneIndex.h"

#include "pxr/usdImaging/usdImaging/niPrototypeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((propagatedPrototypesScope, "UsdNiPropagatedPrototypes")));
                         

namespace UsdImaging_NiInstanceAggregationSceneIndex_Impl {

// Gets primvars from prim at given path in scene index.
HdPrimvarsSchema
_GetPrimvarsSchema(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
{
    if (!sceneIndex) {
        return HdPrimvarsSchema(nullptr);
    }
    return 
        HdPrimvarsSchema::GetFromParent(
            sceneIndex->GetPrim(primPath).dataSource);
}
    
// Gets primvar from prim at given path with given name in scene index.
HdPrimvarSchema
_GetPrimvarSchema(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    const TfToken &primvarName)
{
    return
        _GetPrimvarsSchema(sceneIndex, primPath)
            .GetPrimvar(primvarName);
}

// Checks whether there is a primvar of the given name on the given prim
// in the scene index and its interpolation is constant.
bool
_IsConstantPrimvar(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    const TfToken &primvarName)
{
    HdTokenDataSourceHandle const interpolationSrc =
        _GetPrimvarSchema(sceneIndex, primPath, primvarName).GetInterpolation();
    if (!interpolationSrc) {
        return false;
    }
    const TfToken interpolation = interpolationSrc->GetTypedValue(0.0f);
    return interpolation == HdPrimvarSchemaTokens->constant;
}

// Gets names of all constant prims on prim at given path in given scene index.
TfTokenVector
_GetConstantPrimvarNames(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
{
    TfTokenVector result;

    HdPrimvarsSchema primvarsSchema = _GetPrimvarsSchema(sceneIndex, primPath);
    for (const TfToken &name : primvarsSchema.GetPrimvarNames()) {
        HdPrimvarSchema primvarSchema = primvarsSchema.GetPrimvar(name);
        if (HdTokenDataSourceHandle const interpolationSrc =
                    primvarSchema.GetInterpolation()) {
            const TfToken interpolation = interpolationSrc->GetTypedValue(0.0f);
            if (interpolation == HdPrimvarSchemaTokens->constant) {
                result.push_back(name);
            }
        }
    }

    return result;
}

// Gets value of primvar with given name of prim at given path in scene index.
VtValue
_GetPrimvarValue(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    const TfToken &primvarName)
{
    HdSampledDataSourceHandle const ds =
        _GetPrimvarSchema(sceneIndex, primPath, primvarName)
            .GetPrimvarValue();
    if (!ds) {
        return {};
    }
    return ds->GetValue(0.0f);
}

// Returns the first of the values that the primvar with given name of prim
// at given path in scene index has if the type matches the given type.
template<typename T>
T
_GetTypedPrimvarValue(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    const TfToken &primvarName)
{ 
    const VtValue value =
        _GetPrimvarValue(sceneIndex, primPath, primvarName);
    if (value.IsHolding<T>()) {
        return value.UncheckedGet<T>();
    }
    if (value.IsHolding<VtArray<T>>()) {
        const VtArray<T> array = value.UncheckedGet<VtArray<T>>();
        if (array.empty()) {
            return {};
        }
        return array[0];
    }
    return {};
}
    
GfMatrix4d
_GetPrimTransform(
    HdSceneIndexBaseRefPtr const &sceneIndex, const SdfPath &primPath)
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

// Data source for locator primvars:NAME:primvarValue for an instancer.
//
// Extracts the values of the primvar of given name authored on the native
// instances realized by the instancer.
//
template<typename T>
class _PrimvarValueDataSource : public HdTypedSampledDataSource<VtArray<T>>
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarValueDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * const outSampleTimes) override
    {
        // TODO: Support motion blur
        return false;
    }

    VtArray<T> GetTypedValue(
        const HdSampledDataSource::Time shutterOffset) override
    {
        VtArray<T> result(_instances->size());
        
        int i = 0;
        for (const SdfPath &instance : *_instances) {
            result[i] = _GetTypedPrimvarValue<T>(
                _inputSceneIndex, instance, _primvarName);
            i++;
        }
        return result;
    }

private:
    _PrimvarValueDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances,
        const TfToken &primvarName)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
      , _primvarName(primvarName)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
    TfToken const _primvarName;
};

// Implements Visitor for VtVisitValue.
class _PrimvarValueDataSourceFactory
{
public:
    template <class T>
    HdDataSourceBaseHandle operator()(const T &v) const
    {
        return _PrimvarValueDataSource<T>::New(
            _inputSceneIndex, _instances, _primvarName);
        
    }

    template <class T>
    HdDataSourceBaseHandle operator()(const VtArray<T> &array) const {
        return _PrimvarValueDataSource<T>::New(
            _inputSceneIndex, _instances, _primvarName);
    }

    HdDataSourceBaseHandle operator()(const VtValue &v) const {
        return nullptr;
    }

    _PrimvarValueDataSourceFactory(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances,
        const TfToken &primvarName)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
      , _primvarName(primvarName)
    {
    }

private:
    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
    TfToken const _primvarName;
};

// Examines type of primvar authored on the first instance to dispatch
// by type to create the correct primvar value data source for an
// instancer realizing the instances.
HdDataSourceBaseHandle
_MakePrimvarValueDataSource(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    std::shared_ptr<SdfPathSet> const &instances,
    const TfToken &primvarName)
{
    if (instances->empty()) {
        return nullptr;
    }
    const SdfPath &primPath = *(instances->begin());
    const VtValue value =
        _GetPrimvarValue(inputSceneIndex, primPath, primvarName);

    return VtVisitValue(
        value,
        _PrimvarValueDataSourceFactory(
            inputSceneIndex, instances, primvarName));
}

// Container data source for locator primvars:NAME for an instancer.
//
// primvarValue: obtained by taking the values of the primvar of given
// name authored on the native instances realized by this instancer
// to provide the primvar value.
// role: obtained by taking the role of the primvar authored on the
// first instance.
// interpolation: constant.
//
class _PrimvarDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);

    TfTokenVector GetNames() override
    {
        return { HdPrimvarSchemaTokens->primvarValue,
                 HdPrimvarSchemaTokens->interpolation,
                 HdPrimvarSchemaTokens->role };
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _MakePrimvarValueDataSource(
                _inputSceneIndex, _instances, _primvarName);
        }
        if (name == HdPrimvarSchemaTokens->interpolation) {
            static HdDataSourceBaseHandle const ds =
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdPrimvarSchemaTokens->instance);
            return ds;
        }
        if (name == HdPrimvarSchemaTokens->role) {
            if (_instances->empty()) {
                return nullptr;
            }
            const SdfPath &primPath = *(_instances->begin());
            return 
                _GetPrimvarSchema(_inputSceneIndex, primPath, name)
                    .GetRole();
        }
        return nullptr;
    }

private:
    _PrimvarDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        std::shared_ptr<SdfPathSet> const &instances,
        const TfToken &primvarName)
      : _inputSceneIndex(inputSceneIndex)
      , _instances(instances)
      , _primvarName(primvarName)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    std::shared_ptr<SdfPathSet> const _instances;
    const TfToken _primvarName;
};

// Data source for locator primvars:hydra:instanceTransforms:primvarValue for an
// instancer.
//
// Extracts the transforms of the native instances realized by the instancer.
//
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
            // If this is for a native instance within a Usd point instancer's
            // prototype, this transform will include the prototype's
            // root transform.
            //
            // The instancer for this native instance has no transform and thus
            // does not include the prototype's root transform.
            //
            // Thus, prototype's root transform will be applied exactly once.
            result[i] =
                _GetPrimTransform(_inputSceneIndex, instance);
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

// Data source for locator primvars:hydra:instanceTransforms for an instancer.
//
// primvarValue: transforms of native instances realized by the instancer.
// interpolation: instance.
// role: null.
//
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

// Data source for locator primvars for an instancer.
//
// Uses above data sources for hydra:instanceTransforms and for constant
// primvars authored on the native instances realized by the instancer.
//
class _PrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    TfTokenVector GetNames() override {
        TfTokenVector result;
        if (!_instances->empty()) {
            result = _GetConstantPrimvarNames(
                _inputSceneIndex, *(_instances->begin()));
        }
        result.push_back(HdInstancerTokens->instanceTransforms);
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdInstancerTokens->instanceTransforms) {
            return _InstanceTransformPrimvarDataSource::New(
                _inputSceneIndex, _instances);
        }
        if (!_instances->empty()) {
            if (_IsConstantPrimvar(
                    _inputSceneIndex, *(_instances->begin()), name)) {
                return _PrimvarDataSource::New(
                    _inputSceneIndex, _instances, name);
            }
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

bool
_GetVisibility(HdSceneIndexBaseRefPtr const &sceneIndex,
               const SdfPath &primPath)
{
    if (!sceneIndex) {
        return true;
    }

    HdContainerDataSourceHandle const primDs =
        sceneIndex->GetPrim(primPath).dataSource;        
    HdBoolDataSourceHandle const ds =
        HdVisibilitySchema::GetFromParent(primDs).GetVisibility();
    if (!ds) {
        return true;
    }
    return ds->GetTypedValue(0.0f);
}

VtBoolArray
_ComputeMask(HdSceneIndexBaseRefPtr const &sceneIndex,
             std::shared_ptr<SdfPathSet> const &instances)
{
    VtBoolArray result(instances->size());

    int i = 0;
    for (const SdfPath &instance : *instances) {
        result[i] = _GetVisibility(sceneIndex, instance);
        i++;
    }
    return result;
}

class _InstancerTopologyDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstancerTopologyDataSource);

    TfTokenVector GetNames() override {
        return { HdInstancerTopologySchemaTokens->instanceIndices,
                 HdInstancerTopologySchemaTokens->prototypes,
                 HdInstancerTopologySchemaTokens->instanceLocations,
                 HdInstancerTopologySchemaTokens->mask };
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
        if (name == HdInstancerTopologySchemaTokens->mask) {
            return HdRetainedTypedSampledDataSource<VtBoolArray>::New(
                _ComputeMask(_inputSceneIndex, _instances));
        }
        return nullptr;
    }

private:
    _InstancerTopologyDataSource(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const SdfPath &prototypePath,
        std::shared_ptr<SdfPathSet> const &instances)
      : _inputSceneIndex(inputSceneIndex)
      , _prototypePath(prototypePath)
      , _instances(instances)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    const SdfPath _prototypePath;
    std::shared_ptr<SdfPathSet> const _instances;
};

class _InstancerPrimSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_InstancerPrimSource);

    TfTokenVector GetNames() override {
        return { HdInstancedBySchema::GetSchemaToken(),
                 HdInstancerTopologySchema::GetSchemaToken(),
                 HdPrimvarsSchema::GetSchemaToken()};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        if (name == HdInstancedBySchema::GetSchemaToken()) {
            // If this instancer is inside a point instanced prototype, use
            // the instancedBy schema from the prototype root so that this
            // instancer will be instanced by the the point instancer.
            if (HdInstancedBySchema schema = HdInstancedBySchema::GetFromParent(
                    _inputSceneIndex->GetPrim(_enclosingPrototypeRoot)
                        .dataSource)) {
                return schema.GetContainer();
            }
            if (_forNativePrototype) {
                // This instancer is itself within a native prototype which
                // in turn is instanced by an instancer. Use respective
                // instancedBy data source.
                return
                    UsdImaging_NiPrototypeSceneIndex::
                    GetInstancedByDataSource();
            }
            return nullptr;
        }
        if (name == HdInstancerTopologySchema::GetSchemaToken()) {
            return _InstancerTopologyDataSource::New(
                    _inputSceneIndex, _prototypePath, _instances);
        }
        if (name == HdPrimvarsSchema::GetSchemaToken()) {
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
        const bool forNativePrototype)
      : _inputSceneIndex(inputSceneIndex)
      , _enclosingPrototypeRoot(enclosingPrototypeRoot)
      , _prototypePath(prototypePath)
      , _instances(instances)
      , _forNativePrototype(forNativePrototype)
    {
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    const SdfPath _enclosingPrototypeRoot;
    const SdfPath _prototypePath;
    std::shared_ptr<SdfPathSet> const _instances;
    const bool _forNativePrototype;
};

// We can only group together native instances to be realized by the same
// instancer if each has the same set of constant primvars authored.
//
// We thus include the set of constant primvars (and their roles) in the
// hash used to group the native instances.
//
// This function is computing a hash based on the set of the names of the
// constant primvars and their roles.
std::string
_ComputeConstantPrimvarsRoleHash(HdPrimvarsSchema primvarsSchema)
{
    std::map<TfToken, TfToken> nameToRole;
    
    for (const TfToken &name : primvarsSchema.GetPrimvarNames()) {
        HdPrimvarSchema primvarSchema = primvarsSchema.GetPrimvar(name);
        if (HdTokenDataSourceHandle const interpolationSrc =
                    primvarSchema.GetInterpolation()) {
            if (interpolationSrc->GetTypedValue(0.0f) ==
                        HdPrimvarSchemaTokens->constant) {
                TfToken role;
                if (HdTokenDataSourceHandle const roleSrc =
                           primvarSchema.GetRole()) {
                    role = roleSrc->GetTypedValue(0.0f);
                }
                nameToRole[name] = role;
            }
        }
    }

    if (nameToRole.empty()) {
        return "NoPrimvars";
    }

    return TfStringPrintf("Primvars%zx", TfHash::Combine(nameToRole));
}

// We can only group together native instances if the same set of constant
// primvars is authored and if the data sources at the given names have the
// same data. We construct a key accordingly.
//
// instanceDataSourceNames typically includes material binding, purpose and
// model.
//
// Note that instanceDataSourceNames should not include the primvars, xform or
// visibility as these turn into instance interpolated primvars or
// the instancer topology's mask.
//
TfToken
_ComputeBindingHash(HdContainerDataSourceHandle const &primSource,
                    const TfTokenVector &instanceDataSourceNames)
{
    std::string result
        = _ComputeConstantPrimvarsRoleHash(
            HdPrimvarsSchema::GetFromParent(primSource));

    for (const TfToken &name : instanceDataSourceNames) {
        if (HdDataSourceBaseHandle const ds = primSource->Get(name)) {
            result +=
                TfStringPrintf("_%s%zx",
                               name.GetText(),
                               HdDataSourceHash(ds, 0.0f, 0.0f));
        }
    }

    return TfToken(result);
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

// Make a partial copy of the prim data source of a native
// instance using instanceDataSourceNames.
HdContainerDataSourceHandle
_MakeBindingCopy(HdContainerDataSourceHandle const &primSource,
                 const TfTokenVector &instanceDataSourceNames)
{
    TfTokenVector names;
    names.reserve(instanceDataSourceNames.size());
    std::vector<HdDataSourceBaseHandle> dataSources;
    dataSources.reserve(instanceDataSourceNames.size());

    for (const TfToken &name : instanceDataSourceNames) {
        if (HdDataSourceBaseHandle const ds = primSource->Get(name)) {
            names.push_back(name);
            dataSources.push_back(HdMakeStaticCopy(ds));
        }
    }

    return HdRetainedContainerDataSource::New(
        names.size(),
        names.data(),
        dataSources.data());
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

    // A path like /MyPiPrototype/UsdNiPropagtedPrototypes/Binding312...436
    // that serves for all instancers of instances with the same, e.g.,
    // material binding.
    SdfPath GetBindingPrimPath() const {
        return
            enclosingPrototypeRoot
                .AppendChild(_tokens->propagatedPrototypesScope)
                .AppendChild(bindingHash);
    }

    // A path like /MyPiPrototype/UsdNiPropagtedPrototypes/Binding312...436/__Prototype_1
    // that is the parent prim for the instancer for a particular USD
    // prototype.
    SdfPath GetPropagatedPrototypeBase() const {
        return
            GetBindingPrimPath()
                .AppendChild(prototypeName);
    }

    // A path like /MyPiPrototype/UsdNiPropagtedPrototypes/Binding312...436/__Prototype_1/UsdNiInstancer
    // that is where the instancer actually is.
    SdfPath GetInstancerPath() const {
        return
            GetPropagatedPrototypeBase()
                .AppendChild(
                    UsdImaging_NiPrototypeSceneIndexTokens->instancer);
    }

    // A path like /MyPiPrototype/UsdNiPropagtedPrototypes/Binding312...436/__Prototype_1/UsdNiInstancer/UsdNiPrototype
    // the path where the propagating scene index needs to insert a copy
    // of the USD prototype.
    SdfPath GetPrototypePath() const {
        return
            GetInstancerPath()
                .AppendChild(
                    UsdImaging_NiPrototypeSceneIndexTokens->prototype);
    }
};

class _InstanceObserver : public HdSceneIndexObserver
{
public:
    _InstanceObserver(HdSceneIndexBaseRefPtr const &inputScene,
                      const bool forNativePrototype,
                      const TfTokenVector &instanceDataSourceNames);

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

    void
    PrimsRenamed(const HdSceneIndexBase &sender,
                 const RenamedPrimEntries &entries) override;


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
    void _ResyncPrim(const SdfPath &primPath);
    void _DirtyInstancerForInstance(const SdfPath &instance,
                                    const HdDataSourceLocatorSet &locators);
    
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

    HdSceneIndexBaseRefPtr const _inputScene;
    HdRetainedSceneIndexRefPtr const _retainedSceneIndex;
    const bool _forNativePrototype;
    const TfTokenVector _instanceDataSourceNames;
    // If dirtied, we need to re-aggregate the native instance.
    const HdDataSourceLocatorSet _resyncLocators;
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

// Compute which dirtied data source locators force us
// re-aggregating the native instance.
static
HdDataSourceLocatorSet
_ComputeResyncLocators(const TfTokenVector &instanceDataSourceNames)
{
    HdDataSourceLocatorSet result;
    // The enclosing scope of the native instance might have changed.
    result.insert(
        HdInstancedBySchema::GetDefaultLocator().Append(
            HdInstancedBySchemaTokens->prototypeRoots));
    // A data source used to determine which instances can be
    // aggregated has changed.
    for (const TfToken &name : instanceDataSourceNames) {
        result.insert(HdDataSourceLocator(name));
    }
    return result;
}

_InstanceObserver::_InstanceObserver(
        HdSceneIndexBaseRefPtr const &inputScene,
        const bool forNativePrototype,
        const TfTokenVector &instanceDataSourceNames)
  : _inputScene(inputScene)
  , _retainedSceneIndex(HdRetainedSceneIndex::New())
  , _forNativePrototype(forNativePrototype)
  , _instanceDataSourceNames(instanceDataSourceNames)
  , _resyncLocators(_ComputeResyncLocators(instanceDataSourceNames))
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
        _ResyncPrim(path);
    }
}

// Given a set of data source locators, examine the ones relating to primvars.
// If only the values of the primvars change, record the corresponding data
// source locators. If more than the values change, set the needsResync flag
// to indicate that structural changes need to happen, that is the native
// instances need to be grouped differently.
void
_GetPrimvarValueLocatorsAndNeedsResyncFlag(
    const HdDataSourceLocatorSet &locators,
    HdDataSourceLocatorSet * const primvarValueLocators,
    bool * const needsResync)
{
    for (const HdDataSourceLocator &locator :
             locators.Intersection(HdPrimvarsSchema::GetDefaultLocator())) {
        if (locator.GetElementCount() >= 3 &&
            locator.GetElement(2) == HdPrimvarSchemaTokens->primvarValue) {

            primvarValueLocators->insert(locator);
        } else {
            *needsResync = true;
            return;
        }
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
        const HdDataSourceLocatorSet &locators = entry.dirtyLocators;

        if (locators.Intersects(_resyncLocators)) {
            _ResyncPrim(path);
            continue;
        }

        {
            static const HdDataSourceLocatorSet xformLocators{
                HdXformSchema::GetDefaultLocator()};

            if (locators.Intersects(xformLocators)) {
                static const HdDataSourceLocatorSet instanceTransformLocators{
                    HdPrimvarsSchema::GetDefaultLocator()
                        .Append(HdInstancerTokens->instanceTransforms)
                        .Append(HdPrimvarSchemaTokens->primvarValue)};
                _DirtyInstancerForInstance(path, instanceTransformLocators);
            }
        }

        {
            HdDataSourceLocatorSet primvarValueLocators;
            bool needsResync = false;
            _GetPrimvarValueLocatorsAndNeedsResyncFlag(
                locators, &primvarValueLocators, &needsResync);
            if (needsResync) {
                // The set of constant primvars might have changed
                // (e.g., because the interpolation of a primvar has
                // changed). We potentially need to put this instance
                // into a different group.
                _ResyncPrim(path);
            } else if (!primvarValueLocators.IsEmpty()) {
                // Only the primvar values have changed. Update instancer.
                _DirtyInstancerForInstance(path, primvarValueLocators);
            }
        }

        {
            if (locators.Intersects(HdVisibilitySchema::GetDefaultLocator())) {
                static const HdDataSourceLocatorSet maskLocators{
                    HdInstancerTopologySchema::GetDefaultLocator()
                        .Append(HdInstancerTopologySchemaTokens->mask) };
                _DirtyInstancerForInstance(path, maskLocators);
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
_InstanceObserver::PrimsRenamed(const HdSceneIndexBase &sender,
                                const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}




void
_InstanceObserver::_Populate()
{
    for (const SdfPath &primPath
             : HdSceneIndexPrimView(_inputScene,
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
        if (_forNativePrototype) {
            result.enclosingPrototypeRoot =
                UsdImaging_NiPrototypeSceneIndex::GetPrototypePath();
        } else {
            result.enclosingPrototypeRoot =
                SdfPath::AbsoluteRootPath();
        }
    }
    result.bindingHash = _ComputeBindingHash(
        primSource, _instanceDataSourceNames);

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
                    _inputScene->GetPrim(primPath).dataSource,
                    _instanceDataSourceNames) } } );
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

        _retainedSceneIndex->AddPrims(
            { // Add propagated prototype base prim
              { info.GetPropagatedPrototypeBase(),
                TfToken(),
                HdRetainedContainerDataSource::New() },
              // instancer which is child of base prim.
              { instancerPath,
                HdPrimTypeTokens->instancer,
                _InstancerPrimSource::New(
                    _inputScene,
                    info.enclosingPrototypeRoot,
                    info.GetPrototypePath(),
                    instances,
                    _forNativePrototype) } });
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

void
_InstanceObserver::_ResyncPrim(const SdfPath &primPath)
{
    _RemovePrim(primPath);
    _AddPrim(primPath);
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

void
_InstanceObserver::_DirtyInstancerForInstance(
    const SdfPath &instance,
    const HdDataSourceLocatorSet &locators)
{
    auto it = _instanceToInfo.find(instance);
    if (it == _instanceToInfo.end()) {
        return;
    }

    const SdfPath &instancer = it->second.GetInstancerPath();
    
    _retainedSceneIndex->DirtyPrims({{instancer, locators}});
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
            HdInstanceSchema::GetSchemaToken(),
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
        const bool forNativePrototype,
        const TfTokenVector &instanceDataSourceNames)
  : _instanceObserver(
        std::make_unique<_InstanceObserver>(
            inputScene, forNativePrototype, instanceDataSourceNames))
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

/* static */
TfToken
UsdImaging_NiInstanceAggregationSceneIndex::
GetPrototypeNameFromInstancerPath(const SdfPath &primPath)
{
    // Use the convention that the prototype base will be at a path such as
    // that instancers will have paths such as
    // /Foo/UsdNiPropatedPrototypes/Binding435..f52/__Prototype_1/UsdNiInstancer
    // find them.
    if (primPath.GetPathElementCount() < 4) {
        return TfToken();
    }

    if (primPath.GetNameToken() !=
                    UsdImaging_NiPrototypeSceneIndexTokens->instancer) {
        return TfToken();
    }

    // Get second last element, e.g., __Prototype_1
    return primPath.GetParentPath().GetNameToken();
}

/* static */
SdfPath
UsdImaging_NiInstanceAggregationSceneIndex::
GetBindingScopeFromInstancerPath(const SdfPath &primPath)
{
    return primPath.GetParentPath().GetParentPath();
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

void
UsdImaging_NiInstanceAggregationSceneIndex::
_RetainedSceneIndexObserver::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}



PXR_NAMESPACE_CLOSE_SCOPE

