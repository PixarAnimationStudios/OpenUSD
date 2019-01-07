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
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/inheritedCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingPrimAdapter>();
}

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_SHARED_XFORM_CACHE, 1, 
                      "Enable a shared cache for transforms.");
static bool _IsEnabledXformCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_SHARED_XFORM_CACHE) == 1;
    return _v;
}

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_BINDING_CACHE, 1, 
                      "Enable a cache for material bindings.");
static bool _IsEnabledBindingCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_BINDING_CACHE) == 1;
    return _v;
}

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_VIS_CACHE, 1, 
                      "Enable a cache for visibility.");
static bool _IsEnabledVisCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_VIS_CACHE) == 1;
    return _v;
}

UsdImagingPrimAdapter::~UsdImagingPrimAdapter() 
{
}

/*static*/
bool
UsdImagingPrimAdapter::ShouldCullSubtree(UsdPrim const& prim)
{
    // Skip population of non-imageable prims during population traversal
    // (although they can still be populated by reference).
    return (!prim.IsA<UsdGeomImageable>() && !prim.GetTypeName().IsEmpty());
}

/*virtual*/
bool
UsdImagingPrimAdapter::ShouldCullChildren() const
{
    return false;
}

/*virtual*/
bool
UsdImagingPrimAdapter::IsInstancerAdapter() const
{
    return false;
}

/*virtual*/
bool
UsdImagingPrimAdapter::CanPopulateMaster() const
{
    return false;
}

/*virtual*/
HdDirtyBits 
UsdImagingPrimAdapter::ProcessPrimChange(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         TfTokenVector const& changedFields)
{
    // By default, resync the prim if there are any changes to non-plugin
    // fields and ignore changes to built-in fields. Schemas typically register
    // their own plugin metadata fields instead of relying on built-in fields.
    const SdfSchema& schema = SdfSchema::GetInstance();
    for (const TfToken& field : changedFields) {
        const SdfSchema::FieldDefinition* fieldDef = 
            schema.GetFieldDefinition(field);
        if (fieldDef && fieldDef->IsPlugin()) {
            return HdChangeTracker::AllDirty;
        }
    }

    return HdChangeTracker::Clean;
}

/*virtual*/
void
UsdImagingPrimAdapter::ProcessPrimResync(SdfPath const& usdPath, 
                                         UsdImagingIndexProxy* index) 
{
    // In the simple case, the usdPath and cachePath are the same, so here we
    // remove the adapter dependency and the prim and repopulate as the default
    // behavior.
    _RemovePrim(/*cachePath*/usdPath, index);
    index->RemovePrimInfo(/*usdPrimPath*/usdPath);

    if (_GetPrim(usdPath)) {
        // The prim still exists, so repopulate it.
        index->Repopulate(/*cachePath*/usdPath);
    }
}

/*virtual*/
void
UsdImagingPrimAdapter::ProcessPrimRemoval(SdfPath const& primPath,
                                          UsdImagingIndexProxy* index)
{
    // In the simple case, the usdPath and cachePath are the same, so here we
    // remove the adapter dependency and the prim. We don't repopulate.
    _RemovePrim(/*cachePath*/primPath, index);
    index->RemovePrimInfo(/*usdPrimPath*/primPath);
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkRefineLevelDirty(UsdPrim const& prim,
                                            SdfPath const& usdPath,
                                            UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkReprDirty(UsdPrim const& prim,
                                     SdfPath const& usdPath,
                                     UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkCullStyleDirty(UsdPrim const& prim,
                                          SdfPath const& usdPath,
                                          UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkTransformDirty(UsdPrim const& prim,
                                          SdfPath const& usdPath,
                                          UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                           SdfPath const& usdPath,
                                           UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                         SdfPath const& usdPath,
                                         UsdImagingIndexProxy* index)
{
}

/*virtual*/
SdfPath
UsdImagingPrimAdapter::GetInstancer(SdfPath const &cachePath)
{
    return SdfPath();
}

/*virtual*/
size_t
UsdImagingPrimAdapter::SampleInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time,
    const std::vector<float> &,
    size_t maxSampleCount,
    float *times,
    GfMatrix4d *samples)
{
    return 0;
}

size_t
UsdImagingPrimAdapter::SamplePrimvar(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time, const std::vector<float>& configuredSampleTimes,
    size_t maxNumSamples, float *times, VtValue *samples)
{
    HD_TRACE_FUNCTION();

    // Try as USD primvar.
    UsdGeomPrimvarsAPI primvars(usdPrim);
    UsdGeomPrimvar pv = primvars.FindPrimvarWithInheritance(key);

    if (pv && pv.HasValue()) {
        if (pv.ValueMightBeTimeVarying()) {
            size_t numSamples = std::min(maxNumSamples,
                                         configuredSampleTimes.size());
            for (size_t i=0; i < numSamples; ++i) {
                UsdTimeCode sceneTime =
                    _delegate->GetTimeWithOffset(configuredSampleTimes[i]);
                times[i] = configuredSampleTimes[i];
                pv.Get(&samples[i], sceneTime);
            }
            return numSamples;
        } else {
            // Return a single sample for non-varying primvars
            times[0] = 0;
            pv.Get(samples, time);
            return 1;
        }
    }

    // Try as USD attribute.  This handles cases like "points" that
    // are considered primvars by Hydra but non-primvar attributes by USD.
    if (UsdAttribute attr = usdPrim.GetAttribute(key)) {
        if (attr.ValueMightBeTimeVarying()) {
            size_t numSamples = std::min(maxNumSamples,
                                         configuredSampleTimes.size());
            for (size_t i=0; i < numSamples; ++i) {
                UsdTimeCode sceneTime =
                    _delegate->GetTimeWithOffset(configuredSampleTimes[i]);
                times[i] = configuredSampleTimes[i];
                attr.Get(&samples[i], sceneTime);
            }
            return numSamples;
        } else {
            // Return a single sample for non-varying primvars
            times[0] = 0;
            attr.Get(samples, time);
            return 1;
        }
    }

    // Fallback for adapters that do not read primvars from USD, but
    // instead synthesize them -- ex: Cube, Cylinder, Capsule.
    if (maxNumSamples > 0) {
        times[0] = 0;
        if (_GetValueCache()->ExtractPrimvar(cachePath, key, &samples[0])) {
            return samples[0].IsEmpty() ? 0 : 1;
        }
    }

    return 0;
}

/*virtual*/
SdfPath 
UsdImagingPrimAdapter::GetPathForInstanceIndex(
    SdfPath const &protoPath,
    int instanceIndex,
    int *instanceCount,
    int *absoluteInstanceIndex,
    SdfPath *resolvedPrimPath,
    SdfPathVector *instanceContext)
{
    if (absoluteInstanceIndex) {
        *absoluteInstanceIndex = UsdImagingDelegate::ALL_INSTANCES;
    }
    return SdfPath();
}

/*virtual*/
SdfPath
UsdImagingPrimAdapter::GetPathForInstanceIndex(
    SdfPath const &instancerPath, SdfPath const &protoPath,
    int instanceIndex, int *instanceCount,
    int *absoluteInstanceIndex, SdfPath *resolvedPrimPath,
    SdfPathVector *instanceContext)
{
    if (absoluteInstanceIndex) {
        *absoluteInstanceIndex = UsdImagingDelegate::ALL_INSTANCES;
    }
    return SdfPath();
}

/*virtual*/
bool
UsdImagingPrimAdapter::PopulateSelection(HdSelection::HighlightMode const& mode,
                                         SdfPath const &usdPath,
                                         VtIntArray const &instanceIndices,
                                         HdSelectionSharedPtr const &result)
{
    const SdfPath indexPath = _delegate->GetPathForIndex(usdPath);

    // insert itself into the selection map.
    // XXX: should check the existence of the path
    if (instanceIndices.size() == 0) {
        result->AddRprim(mode, indexPath);
    } else {
        result->AddInstance(mode, indexPath, instanceIndices);
    }

    TF_DEBUG(USDIMAGING_SELECTION).Msg("PopulateSelection: (prim) %s\n",
                                       indexPath.GetText());

    return true;
}

HdTextureResource::ID
UsdImagingPrimAdapter::GetTextureResourceID(UsdPrim const& usdPrim,
                                            SdfPath const &id,
                                            UsdTimeCode time,
                                            size_t salt) const
{
    return HdTextureResource::ID(-1);
}

HdTextureResourceSharedPtr
UsdImagingPrimAdapter::GetTextureResource(UsdPrim const& usdPrim,
                                          SdfPath const &id,
                                          UsdTimeCode time) const
{
    return nullptr;
}

HdVolumeFieldDescriptorVector
UsdImagingPrimAdapter::GetVolumeFieldDescriptors(UsdPrim const& usdPrim,
	                                         SdfPath const &id,
                                                 UsdTimeCode time) const
{
    return HdVolumeFieldDescriptorVector();
}

void
UsdImagingPrimAdapter::SetDelegate(UsdImagingDelegate* delegate)
{
    _delegate = delegate;
}

bool
UsdImagingPrimAdapter::IsChildPath(SdfPath const& path) const
{
    return _delegate->_IsChildPath(path);
}

UsdImagingValueCache* 
UsdImagingPrimAdapter::_GetValueCache() const
{
    return &_delegate->_valueCache; 
}

GfMatrix4d 
UsdImagingPrimAdapter::GetRootTransform() const
{
    return _delegate->GetRootTransform();
}

UsdPrim
UsdImagingPrimAdapter::_GetPrim(SdfPath const& usdPath) const
{
    // Intentionally not calling _delegate->_GetPrim here because it strictly
    // requires the prim to exist.
    return _delegate->_stage->GetPrimAtPath(usdPath);
}

const UsdImagingPrimAdapterSharedPtr& 
UsdImagingPrimAdapter::_GetPrimAdapter(UsdPrim const& prim,
                                       bool ignoreInstancing) const
{
    return _delegate->_AdapterLookup(prim, ignoreInstancing);
}

SdfPath
UsdImagingPrimAdapter::_GetPrimPathFromInstancerChain(
                                            SdfPathVector const& instancerChain)
{
    // The instancer chain is stored more-to-less local.  For example:
    //
    // ProtoCube   <----+
    //   +-- cube       | (native instance)
    // ProtoA           |  <--+
    //   +-- ProtoCube--+     | (native instance)
    // PointInstancer         |
    //   +-- ProtoA ----------+
    //
    // paths = 
    //    /__Master__1/cube
    //    /__Master__2/ProtoCube
    //    /PointInstancer/ProtoA
    //
    // This function uses the path chain to recreate the instance path:
    //    /PointInstancer/ProtoA/ProtoCube/cube

    if (instancerChain.size() == 0) {
        return SdfPath();
    }

    SdfPath primPath = instancerChain[0];

    // Every path except the last path should be a path in master.  The idea is
    // to replace the master path with the instance path that comes next in the
    // chain, and continue until we're back at scene scope.
    for (size_t i = 1; i < instancerChain.size(); ++i)
    {
        UsdPrim prim = _GetPrim(primPath);
        TF_VERIFY(prim.IsInMaster());

        UsdPrim master = prim;
        while (!master.IsMaster()) {
            master = master.GetParent();
        }
        primPath = primPath.ReplacePrefix(master.GetPath(), instancerChain[i]);
    }

    return primPath;
}

UsdTimeCode
UsdImagingPrimAdapter::_GetTimeWithOffset(float offset) const
{
    return _delegate->GetTimeWithOffset(offset);
}

SdfPath 
UsdImagingPrimAdapter::_GetPathForIndex(const SdfPath &usdPath) const
{
    return _delegate->GetPathForIndex(usdPath);
}

SdfPathVector
UsdImagingPrimAdapter::_GetRprimSubtree(SdfPath const& indexPath) const
{
    return _delegate->GetRenderIndex().GetRprimSubtree(indexPath);
}

TfToken
UsdImagingPrimAdapter::_GetMaterialBindingPurpose() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        GetMaterialBindingPurpose();
}

TfToken
UsdImagingPrimAdapter::_GetMaterialNetworkSelector() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        GetMaterialNetworkSelector();
}

TfTokenVector 
UsdImagingPrimAdapter::_GetShaderSourceTypes() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
            GetShaderSourceTypes();
}

bool 
UsdImagingPrimAdapter::_IsInInvisedPaths(SdfPath const& usdPath) const
{
    return _delegate->IsInInvisedPaths(usdPath);
}

void 
UsdImagingPrimAdapter::_MergePrimvar(
    HdPrimvarDescriptorVector* vec,
    TfToken const& name,
    HdInterpolation interp,
    TfToken const& role) const
{
    HdPrimvarDescriptor primvar(name, interp, role);
    HdPrimvarDescriptorVector::iterator it =
        std::find(vec->begin(), vec->end(), primvar);
    if (it == vec->end())
        vec->push_back(primvar);
    else
        *it = primvar;
}

/* static */
HdInterpolation
UsdImagingPrimAdapter::_UsdToHdInterpolation(TfToken const& usdInterp)
{
    if (usdInterp == UsdGeomTokens->uniform) {
        return HdInterpolationUniform;
    } else if (usdInterp == UsdGeomTokens->vertex) {
        return HdInterpolationVertex;
    } else if (usdInterp == UsdGeomTokens->varying) {
        return HdInterpolationVarying;
    } else if (usdInterp == UsdGeomTokens->faceVarying) {
        return HdInterpolationFaceVarying;
    } else if (usdInterp == UsdGeomTokens->constant) {
        return HdInterpolationConstant;
    }
    TF_CODING_ERROR("Unknown USD interpolation %s; treating as constant",
                    usdInterp.GetText());
    return HdInterpolationConstant;
}

/* static */
TfToken
UsdImagingPrimAdapter::_UsdToHdRole(TfToken const& usdRole)
{
    if (usdRole == SdfValueRoleNames->Point) {
        return HdPrimvarRoleTokens->point;
    } else if (usdRole == SdfValueRoleNames->Normal) {
        return HdPrimvarRoleTokens->normal;
    } else if (usdRole == SdfValueRoleNames->Vector) {
        return HdPrimvarRoleTokens->vector;
    } else if (usdRole == SdfValueRoleNames->Color) {
        return HdPrimvarRoleTokens->color;
    } else if (usdRole == SdfValueRoleNames->TextureCoordinate) {
        return HdPrimvarRoleTokens->textureCoordinate;
    }
    // Empty token means no role specified
    return TfToken();
}


void 
UsdImagingPrimAdapter::_ComputeAndMergePrimvar(
    UsdPrim const& gprim,
    SdfPath const& cachePath,
    UsdGeomPrimvar const& primvar,
    UsdTimeCode time,
    UsdImagingValueCache* valueCache,
    HdInterpolation *interpOverride) const
{
    VtValue v;
    TfToken primvarName = primvar.GetPrimvarName();
    if (primvar.ComputeFlattened(&v, time)) {
        valueCache->GetPrimvar(cachePath, primvarName) = v;
        HdInterpolation interp = interpOverride ? *interpOverride
            : _UsdToHdInterpolation(primvar.GetInterpolation());
        TfToken role = _UsdToHdRole(primvar.GetAttr().GetRoleName());
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg("UsdImaging: found primvar (%s %s) %s, interp %s\n",
                 gprim.GetPath().GetText(),
                 cachePath.GetText(),
                 primvarName.GetText(),
                 TfEnum::GetName(interp).c_str());
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      primvarName, interp, role);
    } else {
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg( "\t\t No primvar on <%s> named %s\n",
                  gprim.GetPath().GetText(), primvarName.GetText());
    }
}

UsdImaging_CollectionCache&
UsdImagingPrimAdapter::_GetCollectionCache() const
{
    return _delegate->_collectionCache;
}

bool 
UsdImagingPrimAdapter::_IsVarying(UsdPrim prim,
                                  TfToken const& attrName, 
                                  HdDirtyBits dirtyFlag,
                                  TfToken const& perfToken,
                                  HdDirtyBits* dirtyFlags,
                                  bool isInherited,
                                  bool *exists) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Unset the bit initially.
    (*dirtyFlags) &= ~dirtyFlag;

    if (exists != nullptr) {
        *exists = false;
    }

    do {
        UsdAttribute attr = prim.GetAttribute(attrName);

        if (attr && exists != nullptr) {
            *exists = true;
        }
        if (attr.ValueMightBeTimeVarying()){
            (*dirtyFlags) |= dirtyFlag;
            HD_PERF_COUNTER_INCR(perfToken);
            return true;
        }
        prim = prim.GetParent();

    } while (isInherited && prim.GetPath() != SdfPath::AbsoluteRootPath());

    return false;
}

bool 
UsdImagingPrimAdapter::_IsRefined(SdfPath const& cachePath) const
{
    return _delegate->IsRefined(cachePath);
}

bool 
UsdImagingPrimAdapter::_IsTransformVarying(UsdPrim prim,
                                           HdDirtyBits dirtyFlag,
                                           TfToken const& perfToken,
                                           HdDirtyBits* dirtyFlags) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Unset the bit initially.
    (*dirtyFlags) &= ~dirtyFlag;

    UsdImaging_XformCache &xfCache = _delegate->_xformCache;

    do {
        bool mayXformVary = 
            xfCache.GetQuery(prim)->TransformMightBeTimeVarying();
        if (mayXformVary) {
            (*dirtyFlags) |= dirtyFlag;
            HD_PERF_COUNTER_INCR(perfToken);
            return true;
        }

        // If the xformable prim resets the transform stack, then
        // we don't have to check the variability of ancestor transforms.
        bool resetsXformStack = xfCache.GetQuery(prim)->GetResetXformStack();
        if (resetsXformStack) {
            break;
        }

        prim = prim.GetParent();

    } while (prim.GetPath() != SdfPath::AbsoluteRootPath());

    return false;
}

GfMatrix4d 
UsdImagingPrimAdapter::GetTransform(UsdPrim const& prim, UsdTimeCode time,
                                    bool ignoreRootTransform) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    
    UsdImaging_XformCache &xfCache = _delegate->_xformCache;
    GfMatrix4d ctm(1.0);

    if (_IsEnabledXformCache() && xfCache.GetTime() == time) {
        ctm = xfCache.GetValue(prim);
    } else {
        ctm = UsdImaging_XfStrategy::ComputeTransform(
            prim, xfCache.GetRootPath(), time, 
            _delegate->_rigidXformOverrides);
    }

    return ignoreRootTransform ? ctm : ctm * GetRootTransform();
}

bool
UsdImagingPrimAdapter::GetVisible(UsdPrim const& prim, UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();

    if (_delegate->IsInInvisedPaths(prim.GetPath())) return false;

    UsdImaging_VisCache &visCache = _delegate->_visCache;
    if (_IsEnabledVisCache() && visCache.GetTime() == time)
    {
        return visCache.GetValue(prim)
                    == UsdGeomTokens->inherited;
    } else {
        return UsdImaging_VisStrategy::ComputeVisibility(prim, time)
                    == UsdGeomTokens->inherited;
    }
}

SdfPath
UsdImagingPrimAdapter::GetMaterialId(UsdPrim const& prim) const
{
    HD_TRACE_FUNCTION();

    // No need to worry about time here, since relationships do not have time
    // samples.
    if (_IsEnabledBindingCache()) {
        return _delegate->_materialBindingCache.GetValue(prim);
    } else {
        return UsdImaging_MaterialStrategy::ComputeMaterialPath(prim, 
                &_delegate->_materialBindingImplData);
    }
}

TfToken
UsdImagingPrimAdapter::GetModelDrawMode(UsdPrim const& prim)
{
    return _delegate->_GetModelDrawMode(prim);
}

SdfPath
UsdImagingPrimAdapter::GetInstancerBinding(UsdPrim const& prim,
                        UsdImagingInstancerContext const* instancerContext)
{
    return instancerContext ? instancerContext->instancerId
                            : SdfPath();
    
}

SdfPathVector
UsdImagingPrimAdapter::GetDependPaths(SdfPath const &path) const
{
    return SdfPathVector();
}

/*virtual*/
VtIntArray
UsdImagingPrimAdapter::GetInstanceIndices(SdfPath const &instancerPath,
                                          SdfPath const &protoRprimPath)
{
    return VtIntArray();
}

/*virtual*/
GfMatrix4d
UsdImagingPrimAdapter::GetRelativeInstancerTransform(
    SdfPath const &instancerPath,
    SdfPath const &protoInstancerPath, UsdTimeCode time) const
{
    return GfMatrix4d(1);
}

PXR_NAMESPACE_CLOSE_SCOPE

