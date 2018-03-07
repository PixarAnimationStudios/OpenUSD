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
#include "pxr/usdImaging/usdImaging/inheritedCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"

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

/*virtual*/
bool
UsdImagingPrimAdapter::ShouldCullChildren(UsdPrim const&)
{
    // By default, always continue traversal.
    return false;
}

/*virtual*/
bool
UsdImagingPrimAdapter::IsInstancerAdapter()
{
    // By default, opt-out of nested-instancing adapter resolution.
    return false;
}

/*virtual*/
bool
UsdImagingPrimAdapter::IsPopulatedIndirectly()
{
    // By default, do not delay population.
    return false;
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
SdfPath
UsdImagingPrimAdapter::GetInstancer(SdfPath const &cachePath)
{
    return SdfPath();
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
UsdImagingPrimAdapter::PopulateSelection(HdxSelectionHighlightMode const& mode,
                                         SdfPath const &usdPath,
                                         VtIntArray const &instanceIndices,
                                         HdxSelectionSharedPtr const &result)
{
    const SdfPath indexPath = _delegate->GetPathForIndex(usdPath);

    // insert itself into the selection map.
    // XXX: should check the existence of the path
    result->AddInstance(mode, indexPath, instanceIndices);

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
UsdImagingPrimAdapter::_GetValueCache() 
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
UsdImagingPrimAdapter::_GetPrimAdapter(UsdPrim const& prim, bool ignoreInstancing)
{
    return _delegate->_AdapterLookup(prim, ignoreInstancing);
}

void 
UsdImagingPrimAdapter::_MergePrimvar(
                    UsdImagingValueCache::PrimvarInfo const& primvar, 
                    PrimvarInfoVector* vec) 
{
    PrimvarInfoVector::iterator it = std::find(vec->begin(), vec->end(), 
                                                primvar);
    if (it == vec->end())
        vec->push_back(primvar);
    else
        *it = primvar;
}

bool 
UsdImagingPrimAdapter::_IsVarying(UsdPrim prim,
                                  TfToken const& attrName, 
                                  HdDirtyBits dirtyFlag,
                                  TfToken const& perfToken,
                                  HdDirtyBits* dirtyFlags,
                                  bool isInherited)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Unset the bit initially.
    (*dirtyFlags) &= ~dirtyFlag;

    for (bool prime = true;prime ||
          (isInherited && prim.GetPath() != SdfPath::AbsoluteRootPath());
          prime = false) 
    {
        UsdAttribute attr = prim.GetAttribute(attrName);

        if (attr.ValueMightBeTimeVarying()){
            (*dirtyFlags) |= dirtyFlag;
            HD_PERF_COUNTER_INCR(perfToken);
            return true;
        } 
        prim = prim.GetParent();
    }

    return false;
}

bool 
UsdImagingPrimAdapter::_IsTransformVarying(UsdPrim prim,
                                           HdDirtyBits dirtyFlag,
                                           TfToken const& perfToken,
                                           HdDirtyBits* dirtyFlags)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Unset the bit initially.
    (*dirtyFlags) &= ~dirtyFlag;

    UsdImaging_XformCache &xfCache = _delegate->_xformCache;

    for (bool prime = true; 
         prime || (prim.GetPath() != SdfPath::AbsoluteRootPath());
         prime = false) 
    {
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
    }

    return false;
}

GfMatrix4d 
UsdImagingPrimAdapter::GetTransform(UsdPrim const& prim, UsdTimeCode time,
                                    bool ignoreRootTransform)
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
UsdImagingPrimAdapter::GetVisible(UsdPrim const& prim, UsdTimeCode time)
{
    HD_TRACE_FUNCTION();

    if (_delegate->IsInInvisedPaths(prim.GetPath())) return false;

    UsdImaging_VisCache &visCache = _delegate->_visCache;
    if (_IsEnabledVisCache() && visCache.GetTime() == time)
    {
        return visCache.GetValue(prim)
                    == UsdGeomTokens->inherited;
    } else {
        return UsdImaging_VisStrategy::ComputeVisibility(
                            prim, visCache.GetRootPath(), time) 
                    == UsdGeomTokens->inherited;
    }
}

SdfPath
UsdImagingPrimAdapter::GetMaterialId(UsdPrim const& prim)
{
    HD_TRACE_FUNCTION();

    // No need to worry about time here, since relationships do not have time
    // samples.

    // If the renderer supports full material network then this function
    // will try to return a binding that is compatible..
    bool useMaterialNetworks = _delegate->GetRenderIndex().
        GetRenderDelegate()->CanComputeMaterialNetworks();

    if (_IsEnabledBindingCache()) {
        if (useMaterialNetworks) {
            return _delegate->_materialNetworkBindingCache.GetValue(prim);
        } else {
            return _delegate->_materialBindingCache.GetValue(prim);
        }
    } else {
        if (useMaterialNetworks) {
            return UsdImaging_MaterialNetworkStrategy::ComputeMaterialPath(prim);
        } else {
            return UsdImaging_MaterialStrategy::ComputeMaterialPath(prim);
        }
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
    SdfPath const &protoInstancerPath, UsdTimeCode time)
{
    return GfMatrix4d(1);
}

PXR_NAMESPACE_CLOSE_SCOPE

