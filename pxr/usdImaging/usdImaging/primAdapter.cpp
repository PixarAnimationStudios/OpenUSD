//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"
#include "pxr/usdImaging/usdImaging/resolvedAttributeCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usdRender/settingsBase.h"

#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include <vector>

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

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_PURPOSE_CACHE, 1, 
                      "Enable a cache for purpose.");
static bool _IsEnabledPurposeCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_PURPOSE_CACHE) == 1;
    return _v;
}

TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_POINT_INSTANCER_INDICES_CACHE, 1,
                      "Enable a cache for point instancer indices.");
static bool _IsEnabledPointInstancerIndicesCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_POINT_INSTANCER_INDICES_CACHE) == 1;
    return _v;
}

TfTokenVector
UsdImagingPrimAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    TF_WARN("Datasource support not yet added for adapter '%s'",
            TfType::GetCanonicalTypeName(typeid(*this)).c_str());
    return TfTokenVector();
}

TfToken
UsdImagingPrimAdapter::GetImagingSubprimType(UsdPrim const& prim,
    TfToken const& subprim)
{
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingPrimAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingPrimAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

UsdImagingPrimAdapter::PopulationMode
UsdImagingPrimAdapter::GetPopulationMode()
{
    return RepresentsSelf;
}

HdDataSourceLocatorSet
UsdImagingPrimAdapter::InvalidateImagingSubprimFromDescendent(
        UsdPrim const& prim,
        UsdPrim const& descendentPrim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    return InvalidateImagingSubprim(
        descendentPrim, subprim, properties, invalidationType);
}

// ----------------------------------------------------------------------------


/*static*/
bool
UsdImagingPrimAdapter::ShouldCullSubtree(UsdPrim const& prim)
{
    // Do not skip RenderSettings prims even though they are non-imageable
    if (prim.IsA<UsdRenderSettingsBase>() && !prim.GetTypeName().IsEmpty()) {
        return false;
    }
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
UsdImagingPrimAdapter::ShouldIgnoreNativeInstanceSubtrees() const
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
UsdImagingPrimAdapter::CanPopulateUsdInstance() const
{
    return false;
}

/*virtual*/
HdDirtyBits 
UsdImagingPrimAdapter::ProcessPrimChange(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         TfTokenVector const& changedFields)
{
    // By default, resync the prim if there are any changes to plugin
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
UsdImagingPrimAdapter::ProcessPrimResync(SdfPath const& cachePath, 
                                         UsdImagingIndexProxy* index) 
{
    _RemovePrim(cachePath, index);

    /// XXX(UsdImagingPaths): We use the cachePath directly as the
    // usdPath here, but should do the proper transformation.
    // Maybe we could check the primInfo before its removal.
    SdfPath const& usdPath = cachePath;
    if (_GetPrim(usdPath)) {
        // The prim still exists, so repopulate it.
        index->Repopulate(/*cachePath*/usdPath);
    }
}

/*virtual*/
void
UsdImagingPrimAdapter::_ResyncDependents(SdfPath const& usdPath,
                                         UsdImagingIndexProxy* index)
{
    auto const range = _delegate->_dependencyInfo.equal_range(usdPath);
    for (auto it = range.first; it != range.second; ++it) {
        SdfPath const& depCachePath = it->second;
        // If _ResyncDependents is called by the resync method of hydra prim
        // /Foo, there's a strong chance the hydra prim has a declared
        // dependency on USD prim /Foo.  (This is true pretty much except for
        // instancing cases that aren't expected to call this function).
        //
        // In order to avoid infinite loops, if the hydra dependency we get has
        // the same path as the passed in usdPath, skip resyncing it.
        if (depCachePath == usdPath) {
            continue;
        }

        TF_DEBUG(USDIMAGING_CHANGES)
            .Msg("<%s> Resyncing dependent %s\n",
                    usdPath.GetText(), depCachePath.GetText());

        UsdImagingDelegate::_HdPrimInfo *primInfo =
            _delegate->_GetHdPrimInfo(depCachePath);
        if (primInfo != nullptr &&
            TF_VERIFY(primInfo->adapter != nullptr)) {
            primInfo->adapter->ProcessPrimResync(depCachePath, index);
        }
    }
}

/*virtual*/
void
UsdImagingPrimAdapter::ProcessPrimRemoval(SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
    _RemovePrim(cachePath, index);
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkRefineLevelDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkReprDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkCollectionsDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkCullStyleDirty(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkRenderTagDirty(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkTransformDirty(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkLightParamsDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::MarkWindowPolicyDirty(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
}

/*virtual*/
void
UsdImagingPrimAdapter::InvokeComputation(SdfPath const& cachePath,
                                         HdExtComputationContext* context)
{
}

/*virtual*/
std::vector<VtArray<TfToken>>
UsdImagingPrimAdapter::GetInstanceCategories(UsdPrim const& prim)
{
    return std::vector<VtArray<TfToken>>();
}

/*virtual*/
PxOsdSubdivTags
UsdImagingPrimAdapter::GetSubdivTags(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time) const
{
    return PxOsdSubdivTags();
}

/*virtual*/
size_t
UsdImagingPrimAdapter::SampleInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time,
    size_t maxNumSamples,
    float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    return 0;
}

/*virtual*/
GfMatrix4d
UsdImagingPrimAdapter::GetInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time) const
{
    return GfMatrix4d(1.0); 
}

/*virtual*/
SdfPath
UsdImagingPrimAdapter::GetInstancerId(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath) const
{
    return SdfPath::EmptyPath();
}

/*virtual*/
SdfPathVector
UsdImagingPrimAdapter::GetInstancerPrototypes(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath) const
{
    return SdfPathVector();
}

/*virtual*/
size_t
UsdImagingPrimAdapter::SamplePrimvar(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time, 
    size_t maxNumSamples, 
    float *sampleTimes, 
    VtValue *sampleValues,
    VtIntArray *sampleIndices)
{
    HD_TRACE_FUNCTION();

    if (maxNumSamples == 0) {
        return 0;
    }

    // Try as USD primvar.
    // XXX Here we could use the cache.
    UsdGeomPrimvarsAPI primvars(usdPrim);
    UsdGeomPrimvar pv = primvars.FindPrimvarWithInheritance(key);

    GfInterval interval = _GetCurrentTimeSamplingInterval();
    std::vector<double> timeSamples;

    if (pv && pv.HasValue()) {
        if (pv.ValueMightBeTimeVarying()) {
            pv.GetTimeSamplesInInterval(interval, &timeSamples);

            // Add time samples at the boundary conditions
            timeSamples.push_back(interval.GetMin());
            timeSamples.push_back(interval.GetMax());

            // Sort here
            std::sort(timeSamples.begin(), timeSamples.end());
            timeSamples.erase(
                std::unique(timeSamples.begin(), 
                    timeSamples.end()), 
                    timeSamples.end());

            size_t numSamples = timeSamples.size();

            // XXX: We should add caching to the transform computation if this shows
            // up in profiling, but all of our current caches are cleared on time 
            // change so we'd need to write a new structure.
            size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);

            if (sampleIndices) {
                for (size_t i=0; i < numSamplesToEvaluate; ++i) {
                    sampleTimes[i] = timeSamples[i] - time.GetValue();
                    if (pv.Get(&sampleValues[i], timeSamples[i])) {
                        if (!pv.GetIndices(&sampleIndices[i], timeSamples[i])) {
                            sampleIndices[i].clear();
                        }
                    }
                }
            } else {
                for (size_t i=0; i < numSamplesToEvaluate; ++i) {
                    sampleTimes[i] = timeSamples[i] - time.GetValue();
                    pv.ComputeFlattened(&sampleValues[i], timeSamples[i]);
                }
            }
            return numSamples;
        } else {
            // Return a single sample for non-varying primvars
            sampleTimes[0] = 0.0f;
            if (sampleIndices) {
                if (pv.Get(sampleValues, time)) {
                    if (!pv.GetIndices(sampleIndices, time)) {
                        sampleIndices->clear();
                    }
                }
            } else {
                pv.ComputeFlattened(sampleValues, time);
            }
            return 1;
        }
    }

    // Try as USD attribute.  This handles cases like "points" that
    // are considered primvars by Hydra but non-primvar attributes by USD.
    if (UsdAttribute attr = usdPrim.GetAttribute(key)) {
        if (attr.ValueMightBeTimeVarying()) {
            attr.GetTimeSamplesInInterval(interval, &timeSamples);
    
            // Add time samples at the boudary conditions
            timeSamples.push_back(interval.GetMin());
            timeSamples.push_back(interval.GetMax());

            // Sort here
            std::sort(timeSamples.begin(), timeSamples.end());
            timeSamples.erase(
                std::unique(timeSamples.begin(), 
                    timeSamples.end()), 
                    timeSamples.end());

            size_t numSamples = timeSamples.size();

            // XXX: We should add caching to the transform computation if this 
            // shows up in profiling, but all of our current caches are cleared
            // on time change so we'd need to write a new structure.
            size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);
            for (size_t i=0; i < numSamplesToEvaluate; ++i) {
                sampleTimes[i] = timeSamples[i] - time.GetValue();
                attr.Get(&sampleValues[i], timeSamples[i]);
            }
            return numSamples;
        } else {
            // Return a single sample for non-varying primvars
            sampleTimes[0] = 0;
            attr.Get(sampleValues, time);
            return 1;
        }
    }

    // Fallback for adapters that do not read primvars from USD, but
    // instead synthesize them -- ex: Cube, Cylinder, Capsule.
    if (maxNumSamples > 0) {
        sampleTimes[0] = 0;
        sampleValues[0] = Get(usdPrim, cachePath, key, time, &sampleIndices[0]);
        return sampleValues[0].IsEmpty() ? 0 : 1;
    }

    return 0;
}

/*virtual*/
SdfPath 
UsdImagingPrimAdapter::GetScenePrimPath(
    SdfPath const& cachePath,
    int instanceIndex,
    HdInstancerContext *instancerCtx) const
{
    // Note: if we end up here, we're not instanced, since primInfo
    // holds the instance adapter for instanced gprims.
    return cachePath;
}

/*virtual*/
SdfPathVector
UsdImagingPrimAdapter::GetScenePrimPaths(SdfPath const& cachePath,
    std::vector<int> const& instanceIndices,
    std::vector<HdInstancerContext> *instancerCtxs) const
{
    // Note: if we end up here, we're not instanced, since primInfo
    // holds the instance adapter for instanced gprims.
    return SdfPathVector(instanceIndices.size(), cachePath);
}

/*virtual*/
bool
UsdImagingPrimAdapter::PopulateSelection(
    HdSelection::HighlightMode const& mode,
    SdfPath const &cachePath,
    UsdPrim const &usdPrim,
    int const hydraInstanceIndex,
    VtIntArray const &parentInstanceIndices,
    HdSelectionSharedPtr const &result) const
{
    // usdPrim (the original prim selection) might point to a parent node of
    // this hydra prim; but it's also possible for it to point to dependent
    // data sources like materials/coord systems/etc.  Only apply the highlight
    // if usdPrim is a parent of cachePath.
    // Note: this strategy won't work for native instanced prims, but we expect
    // those to be handled in the instance adapter PopulateSelection.
    if (!cachePath.HasPrefix(usdPrim.GetPath())) {
        return false;
    }

    const SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);

    // Insert gprim into the selection map.
    // If "hydraInstanceIndex" is set, just use that.
    // Otherwise, parentInstanceIndices either points to an arry of flat indices
    // to highlight, or (if it's empty) it indicates highlight all indices.
    if (hydraInstanceIndex != -1) {
        VtIntArray indices(1, hydraInstanceIndex);
        result->AddInstance(mode, indexPath, indices);
    } else if (parentInstanceIndices.size() == 0) {
        result->AddRprim(mode, indexPath);
    } else {
        result->AddInstance(mode, indexPath, parentInstanceIndices);
    }

    if (TfDebug::IsEnabled(USDIMAGING_SELECTION)) {
        std::stringstream ss;
        if (hydraInstanceIndex != -1) {
            ss << hydraInstanceIndex;
        } else {
            ss << parentInstanceIndices;
        }
        TF_DEBUG(USDIMAGING_SELECTION).Msg("PopulateSelection: (prim) %s %s\n",
            indexPath.GetText(), ss.str().c_str());
    }

    return true;
}

HdVolumeFieldDescriptorVector
UsdImagingPrimAdapter::GetVolumeFieldDescriptors(UsdPrim const& usdPrim,
	                                         SdfPath const &id,
                                                 UsdTimeCode time) const
{
    return HdVolumeFieldDescriptorVector();
}

static VtValue
_GetUsdPrimAttribute(
    const UsdPrim& prim,
    const TfToken& attrName,
    UsdTimeCode time)
{
    VtValue value;
    if (prim.HasAttribute(attrName)) {
        UsdAttribute attr = prim.GetAttribute(attrName);
        attr.Get(&value, time);
    }
    return value;
}

/*static*/
UsdAttribute
UsdImagingPrimAdapter::LookupLightParamAttribute(
    UsdPrim const& prim,
    TfToken const& paramName)
{
    // Fallback to USD attributes.
    static const std::unordered_map<TfToken, TfToken, TfHash> paramToAttrName({
        { HdLightTokens->angle, UsdLuxTokens->inputsAngle },
        { HdLightTokens->color, UsdLuxTokens->inputsColor },
        { HdLightTokens->colorTemperature, 
            UsdLuxTokens->inputsColorTemperature },
        { HdLightTokens->diffuse, UsdLuxTokens->inputsDiffuse },
        { HdLightTokens->enableColorTemperature, 
            UsdLuxTokens->inputsEnableColorTemperature },
        { HdLightTokens->exposure, UsdLuxTokens->inputsExposure },
        { HdLightTokens->height, UsdLuxTokens->inputsHeight },
        { HdLightTokens->intensity, UsdLuxTokens->inputsIntensity },
        { HdLightTokens->length, UsdLuxTokens->inputsLength },
        { HdLightTokens->normalize, UsdLuxTokens->inputsNormalize },
        { HdLightTokens->radius, UsdLuxTokens->inputsRadius },
        { HdLightTokens->specular, UsdLuxTokens->inputsSpecular },
        { HdLightTokens->textureFile, UsdLuxTokens->inputsTextureFile },
        { HdLightTokens->textureFormat, UsdLuxTokens->inputsTextureFormat },
        { HdLightTokens->width, UsdLuxTokens->inputsWidth },

        { HdLightTokens->shapingFocus, UsdLuxTokens->inputsShapingFocus },
        { HdLightTokens->shapingFocusTint, 
            UsdLuxTokens->inputsShapingFocusTint },
        { HdLightTokens->shapingConeAngle, 
            UsdLuxTokens->inputsShapingConeAngle },
        { HdLightTokens->shapingConeSoftness, 
            UsdLuxTokens->inputsShapingConeSoftness },
        { HdLightTokens->shapingIesFile, UsdLuxTokens->inputsShapingIesFile },
        { HdLightTokens->shapingIesAngleScale, 
            UsdLuxTokens->inputsShapingIesAngleScale },
        { HdLightTokens->shapingIesNormalize, 
            UsdLuxTokens->inputsShapingIesNormalize },
        { HdLightTokens->shadowEnable, UsdLuxTokens->inputsShadowEnable },
        { HdLightTokens->shadowColor, UsdLuxTokens->inputsShadowColor },
        { HdLightTokens->shadowDistance, UsdLuxTokens->inputsShadowDistance },
        { HdLightTokens->shadowFalloff, UsdLuxTokens->inputsShadowFalloff },
        { HdLightTokens->shadowFalloffGamma, 
            UsdLuxTokens->inputsShadowFalloffGamma }
    });

    const TfToken *attrName = TfMapLookupPtr(paramToAttrName, paramName);

    if (prim.HasAttribute(attrName ? *attrName : paramName)) {
        return prim.GetAttribute(attrName ? *attrName : paramName);
    }

    return UsdAttribute();
}

VtValue
UsdImagingPrimAdapter::GetLightParamValue(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& paramName,
    UsdTimeCode time) const
{
    UsdLuxLightAPI light = UsdLuxLightAPI(prim);
    UsdImaging_CollectionCache& collectionCache = _GetCollectionCache();
    if (!light) {
        // Its ok that this is not a light. Lets assume its a light filter.
        // Asking for the lightFilterType is the render delegates way of
        // determining the type of the light filter.
        if (paramName == HdTokens->lightFilterType) {
            // Use the schema type name from the prim type info which is the
            // official type of the prim.
            return VtValue(prim.GetPrimTypeInfo().GetSchemaTypeName());
        }
        if (paramName == HdTokens->lightFilterLink) {
            UsdLuxLightFilter lightFilter = UsdLuxLightFilter(prim);
            UsdCollectionAPI lightFilterLink =
                lightFilter.GetFilterLinkCollectionAPI();
            return VtValue(collectionCache.GetIdForCollection(
                lightFilterLink));
        }
        // fallback to usd attributes
        return _GetUsdPrimAttribute(prim, paramName, time);
    }

    if (paramName == HdTokens->lightLink) {
        UsdCollectionAPI lightLink = light.GetLightLinkCollectionAPI();
        return VtValue(collectionCache.GetIdForCollection(lightLink));
    } else if (paramName == HdTokens->filters) {
        SdfPathVector filterPaths;
        light.GetFiltersRel().GetForwardedTargets(&filterPaths);
        return VtValue(filterPaths);
    } else if (paramName == HdTokens->shadowLink) {
        UsdCollectionAPI shadowLink = light.GetShadowLinkCollectionAPI();
        return VtValue(collectionCache.GetIdForCollection(shadowLink));
    } else if (paramName == HdLightTokens->intensity) {
        // return 0.0 intensity if scene lights are not enabled
        if (!_GetSceneLightsEnabled()) {
            return VtValue(0.0f);
        }

        // return 0.0 intensity if the scene lights are not visible
        if (!GetVisible(prim, cachePath, time)) {
            return VtValue(0.0f);
        }
    } else if (paramName == HdTokens->isLight) {
        return VtValue(!!light);
    } else if (paramName == HdTokens->materialSyncMode) {
        VtValue val;
        light.GetMaterialSyncModeAttr().Get(&val, time);
        return val;
    }

    // Fallback to USD attributes.
    VtValue value;
    if (UsdAttribute attr = LookupLightParamAttribute(prim, paramName)) {
        attr.Get(&value, time);
    }

    return value;
}

void
UsdImagingPrimAdapter::SetDelegate(UsdImagingDelegate* delegate)
{
    _delegate = delegate;
}

bool
UsdImagingPrimAdapter::IsChildPath(SdfPath const& path) const
{
    return path.IsPropertyPath();
}

UsdImagingPrimvarDescCache* 
UsdImagingPrimAdapter::_GetPrimvarDescCache() const
{
    return &_delegate->_primvarDescCache; 
}

UsdImaging_NonlinearSampleCountCache* 
UsdImagingPrimAdapter::_GetNonlinearSampleCountCache() const
{
    return &_delegate->_nonlinearSampleCountCache;
}

UsdImaging_BlurScaleCache* 
UsdImagingPrimAdapter::_GetBlurScaleCache() const
{
    return &_delegate->_blurScaleCache;
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

const UsdImagingPrimAdapterSharedPtr& 
UsdImagingPrimAdapter::_GetAdapter(TfToken const& adapterKey) const
{
    return _delegate->_AdapterLookup(adapterKey);
}

SdfPath
UsdImagingPrimAdapter::_GetPrimPathFromInstancerChain(
                                     SdfPathVector const& instancerChain) const
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
    //    /__Prototype_1/cube
    //    /__Prototype_2/ProtoCube
    //    /PointInstancer/ProtoA
    //
    // This function uses the path chain to recreate the instance path:
    //    /PointInstancer/ProtoA/ProtoCube/cube

    if (instancerChain.size() == 0) {
        return SdfPath();
    }

    SdfPath primPath = instancerChain[0];

    // Every path except the last path should be a path in prototype.  The idea
    // is to replace the prototype path with the instance path that comes next
    // in the chain, and continue until we're back at scene scope.
    for (size_t i = 1; i < instancerChain.size(); ++i)
    {
        UsdPrim prim = _GetPrim(primPath);
        TF_VERIFY(prim.IsInPrototype());

        UsdPrim prototype = prim;
        while (!prototype.IsPrototype()) {
            prototype = prototype.GetParent();
        }
        primPath = primPath.ReplacePrefix(
            prototype.GetPath(), instancerChain[i]);
    }

    return primPath;
}

UsdTimeCode
UsdImagingPrimAdapter::_GetTimeWithOffset(float offset) const
{
    return _delegate->GetTimeWithOffset(offset);
}

SdfPath 
UsdImagingPrimAdapter::_ConvertCachePathToIndexPath(const SdfPath &usdPath) const
{
    return _delegate->ConvertCachePathToIndexPath(usdPath);
}

SdfPath 
UsdImagingPrimAdapter::_ConvertIndexPathToCachePath(const SdfPath &indexPath) const
{
    return _delegate->ConvertIndexPathToCachePath(indexPath);
}

TfToken
UsdImagingPrimAdapter::_GetMaterialBindingPurpose() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        GetMaterialBindingPurpose();
}

TfTokenVector
UsdImagingPrimAdapter::_GetMaterialRenderContexts() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        GetMaterialRenderContexts();
}

TfTokenVector
UsdImagingPrimAdapter::_GetRenderSettingsNamespaces() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        GetRenderSettingsNamespaces();
}

bool 
UsdImagingPrimAdapter::_GetSceneMaterialsEnabled() const
{
    return _delegate->_sceneMaterialsEnabled;
}

bool 
UsdImagingPrimAdapter::_GetSceneLightsEnabled() const
{
    return _delegate->_sceneLightsEnabled;
}

bool
UsdImagingPrimAdapter::_IsPrimvarFilteringNeeded() const
{
    return _delegate->GetRenderIndex().GetRenderDelegate()->
        IsPrimvarFilteringNeeded();
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
    TfToken const& role, 
    bool indexed) const
{
    HdPrimvarDescriptor primvar(name, interp, role, indexed);

    for (HdPrimvarDescriptorVector::iterator it = vec->begin();
        it != vec->end(); ++it) {
        if (it->name == name) {
            *it =  primvar;
            return;
        }
    }

    vec->push_back(primvar);
}

void
UsdImagingPrimAdapter::_RemovePrimvar(
    HdPrimvarDescriptorVector* vec,
    TfToken const& name) const
{
    for (HdPrimvarDescriptorVector::iterator it = vec->begin();
         it != vec->end(); ++it) {
        if (it->name == name) {
            vec->erase(it);
            return;
        }
    }
}

void 
UsdImagingPrimAdapter::_ComputeAndMergePrimvar(
    UsdPrim const& gprim,
    UsdGeomPrimvar const& primvar,
    UsdTimeCode time,
    HdPrimvarDescriptorVector* primvarDescs,
    HdInterpolation *interpOverride) const
{
    TRACE_FUNCTION();

    VtValue v;
    TfToken primvarName = primvar.GetPrimvarName();

    // Note: we call Get() here to check if the primvar exists.
    // We can't call HasValue(), since it won't take time-varying
    // blocks (from value clips) into account. Get() should be
    // fast as long as we don't touch the returned data.
    if (primvar.Get(&v, time)) {
        HdInterpolation interp = interpOverride ? *interpOverride
            : UsdImagingUsdToHdInterpolation(primvar.GetInterpolation());
        TfToken role = UsdImagingUsdToHdRole(primvar.GetAttr().GetRoleName());
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg("UsdImaging: found primvar (%s) %s, interp %s\n",
                 gprim.GetPath().GetText(),
                 primvarName.GetText(),
                 TfEnum::GetName(interp).c_str());
        _MergePrimvar(primvarDescs, primvarName, interp, role, 
                      primvar.IsIndexed());

    } else {
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg( "\t\t No primvar on <%s> named %s\n",
                  gprim.GetPath().GetText(), primvarName.GetText());
        _RemovePrimvar(primvarDescs, primvarName);
    }
}

namespace {

// Figure out what changed about the primvar and update the primvar descriptors
// if necessary
/*static*/
void
_ProcessPrimvarChange(bool primvarOnPrim,
                      TfToken const& primvarName,
                      HdPrimvarDescriptorVector* primvarDescs,
                      SdfPath const& cachePath/*debug*/)
{
    // Determine if primvar is in the value cache.
    HdPrimvarDescriptorVector::iterator primvarIt = primvarDescs->end();
    for (HdPrimvarDescriptorVector::iterator it = primvarDescs->begin();
         it != primvarDescs->end(); it++) {
        if (it->name == primvarName) {
            primvarIt = it;
            break;
        }
    }
    bool primvarInValueCache = primvarIt != primvarDescs->end();

    if (!primvarOnPrim && primvarInValueCache) {
        TF_DEBUG(USDIMAGING_CHANGES).Msg(
            "Removing primvar descriptor %s for cachePath %s.\n",
            primvarIt->name.GetText(), cachePath.GetText());

        // Remove the value cache entry.
        primvarDescs->erase(primvarIt);
    }
}

} // anonymous namespace

HdDirtyBits
UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName,
        TfToken const& primvarName,
        HdInterpolation const& primvarInterp,
        HdDirtyBits primvarDirtyBit
            /*= HdChangeTracker::DirtyPrimvar*/) const
{
    // Determine if primvar exists on the prim.
    bool primvarOnPrim = false;
    UsdAttribute attr = prim.GetAttribute(propertyName);
    if (attr && attr.HasValue()) {
        // The expectation is that this method is used for "built-in" attributes
        // that are treated as primvars.
        if (UsdGeomPrimvar::IsPrimvar(attr)) {
            TF_CODING_ERROR("Prefixed primvar (%s) with cache path %s should "
                "use _ProcessPrefixedPrimvarPropertyChange instead.\n",
                propertyName.GetText(), cachePath.GetText());
        
            return HdChangeTracker::AllDirty;
        }

        primvarOnPrim = true;
    }

    HdPrimvarDescriptorVector& primvarDescs =
        _GetPrimvarDescCache()->GetPrimvars(cachePath);

    _ProcessPrimvarChange(primvarOnPrim, primvarName, &primvarDescs, cachePath);

    return primvarDirtyBit;
}

HdDirtyBits
UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName,
        HdDirtyBits primvarDirtyBit/*= HdChangeTracker::DirtyPrimvar*/,
        bool inherited/*=true*/) const
{
    // Determine if primvar exists on the prim.
    bool primvarOnPrim = false;
    UsdAttribute attr;
    TfToken interpOnPrim;
    UsdGeomPrimvarsAPI api(prim);
    if (inherited) {
        UsdGeomPrimvar pv = api.FindPrimvarWithInheritance(propertyName);
        attr = pv;
        if (pv)
            interpOnPrim = pv.GetInterpolation();
    } else {
        UsdGeomPrimvar localPv = api.GetPrimvar(propertyName);
        attr = localPv;
        if (localPv)
            interpOnPrim = localPv.GetInterpolation();
    }
    if (attr && attr.HasValue()) {
        primvarOnPrim = true;
    }

    // Determine if primvar is in the value cache.
    TfToken primvarName = UsdGeomPrimvar::StripPrimvarsName(propertyName);
    HdPrimvarDescriptorVector& primvarDescs =
        _GetPrimvarDescCache()->GetPrimvars(cachePath);

    _ProcessPrimvarChange(primvarOnPrim, primvarName, &primvarDescs,
                          cachePath);

    return primvarDirtyBit;
}

UsdImaging_CollectionCache&
UsdImagingPrimAdapter::_GetCollectionCache() const
{
    return _delegate->_collectionCache;
}

UsdStageRefPtr
UsdImagingPrimAdapter::_GetStage() const
{
    return _delegate->_stage;
}

UsdImaging_CoordSysBindingStrategy::value_type
UsdImagingPrimAdapter::_GetCoordSysBindings(UsdPrim const& prim) const
{
    return _delegate->_coordSysBindingCache.GetValue(prim);
}

UsdImaging_InheritedPrimvarStrategy::value_type
UsdImagingPrimAdapter::_GetInheritedPrimvars(UsdPrim const& prim) const
{
    return _delegate->_inheritedPrimvarCache.GetValue(prim);
}

UsdGeomPrimvar
UsdImagingPrimAdapter::_GetInheritedPrimvar(UsdPrim const& prim,
                                             TfToken const& primvarName) const
{
    UsdImaging_InheritedPrimvarStrategy::value_type inheritedPrimvarRecord =
        _GetInheritedPrimvars(prim.GetParent());
    if (inheritedPrimvarRecord) {
        for (UsdGeomPrimvar const& pv : inheritedPrimvarRecord->primvars) {
            if (pv.GetPrimvarName() == primvarName) {
                return pv;
            }
        }
    }
    return UsdGeomPrimvar();
}

bool
UsdImagingPrimAdapter::_DoesDelegateSupportCoordSys() const
{
    return _delegate->_coordSysEnabled;
}

SdfPath
UsdImagingPrimAdapter::ResolveCachePath(
    const SdfPath& usdPath,
    const UsdImagingInstancerContext* /* unused */) const
{
    return usdPath;
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
UsdImagingPrimAdapter::_IsTransformVarying(UsdPrim prim,
                                           HdDirtyBits dirtyFlag,
                                           TfToken const& perfToken,
                                           HdDirtyBits* dirtyFlags) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

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
UsdImagingPrimAdapter::GetTransform(UsdPrim const& prim, 
                                    SdfPath const& cachePath,
                                    UsdTimeCode time,
                                    bool ignoreRootTransform) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    
    UsdImaging_XformCache &xfCache = _delegate->_xformCache;
    SdfPath const& xformRoot = xfCache.GetRootPath();
    GfMatrix4d ctm(1.0);

    // If the cachePath has the 'coordSys' namespace, it is a coordSys prim
    // which can point to prims outside the xformRoot. So if 'prim', the
    // coordSys target, is outside the xformRoot use the identity matrix.
    std::pair<std::string, bool> isCoordSys = SdfPath::StripPrefixNamespace(
        cachePath.GetName(), HdPrimTypeTokens->coordSys);
    if (isCoordSys.second && !prim.GetPath().HasPrefix(xformRoot)) {
        TF_WARN("Prim associated with '%s' has path <%s> which is not under "
                "the xformCache root (%s), using the identity matrix.", 
                cachePath.GetText(), prim.GetPath().GetText(), 
                xformRoot.GetText());
    }
    else if (_IsEnabledXformCache() && xfCache.GetTime() == time) {
        ctm = xfCache.GetValue(prim);
    } else {
        ctm = UsdImaging_XfStrategy::ComputeTransform(
            prim, xformRoot, time, _delegate->_rigidXformOverrides);
    }

    return ignoreRootTransform ? ctm : ctm * GetRootTransform();
}

static
size_t
_GatherAuthoredTransformTimeSamples(
    UsdPrim const& prim,
    GfInterval const interval,
    UsdImaging_XformCache const& xfCache,
    std::vector<double>* timeSamples) 
{
    UsdPrim p = prim;
    while (p && p.GetPath() != xfCache.GetRootPath()) {
        // XXX Add caching here.
        if (UsdGeomXformable xf = UsdGeomXformable(p)) {
            std::vector<double> localTimeSamples;
            xf.GetTimeSamplesInInterval(interval, &localTimeSamples);

            // Join timesamples 
            timeSamples->insert(
                timeSamples->end(), 
                localTimeSamples.begin(), 
                localTimeSamples.end());
        }
        p = p.GetParent();
    }

    // Sort here
    std::sort(timeSamples->begin(), timeSamples->end());
    timeSamples->erase(
        std::unique(timeSamples->begin(), 
            timeSamples->end()), 
            timeSamples->end());

    return timeSamples->size();
}

GfInterval
UsdImagingPrimAdapter::_GetCurrentTimeSamplingInterval()
{
    return _delegate->GetCurrentTimeSamplingInterval();
}

Usd_PrimFlagsConjunction
UsdImagingPrimAdapter::_GetDisplayPredicate() const
{
    return _delegate->_GetDisplayPredicate();
}

Usd_PrimFlagsConjunction
UsdImagingPrimAdapter::_GetDisplayPredicateForPrototypes() const
{
    return _delegate->_GetDisplayPredicateForPrototypes();
}

size_t
UsdImagingPrimAdapter::SampleTransform(
    UsdPrim const& prim, 
    SdfPath const& cachePath,
    UsdTimeCode time,
    size_t maxNumSamples, 
    float *sampleTimes, 
    GfMatrix4d *sampleValues)
{
    HD_TRACE_FUNCTION();

    if (maxNumSamples == 0) {
        return 0;
    }

    if (!prim) {
        // If this is not a literal USD prim, it is an instance of
        // other object synthesized by UsdImaging.  Just return
        // the single transform sample from the ValueCache.
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetTransform(prim, prim.GetPath(), 0.0);
        return 1;
    }

    GfInterval interval = _GetCurrentTimeSamplingInterval();

    // Add time samples at the boudary conditions
    std::vector<double> timeSamples;
    timeSamples.push_back(interval.GetMin());
    timeSamples.push_back(interval.GetMax());

    // Gather authored time samples for transforms
    size_t numSamples = _GatherAuthoredTransformTimeSamples(
        prim, 
        interval, 
        _delegate->_xformCache,
        &timeSamples);

    // XXX: We should add caching to the transform computation if this shows
    // up in profiling, but all of our current caches are cleared on time 
    // change so we'd need to write a new structure.
    size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);
    for (size_t i=0; i < numSamplesToEvaluate; ++i) {
        sampleTimes[i] = timeSamples[i] - time.GetValue();
        sampleValues[i] = 
            UsdImaging_XfStrategy::ComputeTransform(
                prim, _delegate->_xformCache.GetRootPath(), timeSamples[i], 
                _delegate->_rigidXformOverrides) 
            * _delegate->_rootXf;
    }

    // Early out if we can't fit the data in the arrays
    if (numSamples > maxNumSamples) {
        return numSamples; 
    }

    // Optimization.
    // Some backends benefit if they can avoid time sample animation
    // for fixed transforms.  This is difficult to compute explicitly
    // due to the hierarchial nature of concated transforms, so we
    // do a post-pass sweep to detect static transforms here.
    for (size_t i=1; i < numSamples; ++i) {
        if (timeSamples[i] != timeSamples[0]) {
            return numSamples;
        }
    }
    // All samples are the same, so just return 1.
    return 1;
}

VtValue
UsdImagingPrimAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const &key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    UsdAttribute const &attr = prim.GetAttribute(key);
    VtValue value;
    if (attr) {
        attr.Get(&value, time);
    }
    return value;
}

bool
UsdImagingPrimAdapter::GetVisible(
    UsdPrim const& prim, 
    SdfPath const& cachePath, 
    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (_delegate->IsInInvisedPaths(prim.GetPath())) {
        return false;
    }

    UsdImaging_VisCache &visCache = _delegate->_visCache;
    if (_IsEnabledVisCache() && visCache.GetTime() == time) {
        return visCache.GetValue(prim) == UsdGeomTokens->inherited;
    } else {
        return UsdImaging_VisStrategy::ComputeVisibility(prim, time)
                    == UsdGeomTokens->inherited;
    }
}

TfToken 
UsdImagingPrimAdapter::GetPurpose(
    UsdPrim const& prim, 
    SdfPath const& cachePath,
    TfToken const& instanceInheritablePurpose) const
{
    HD_TRACE_FUNCTION();

    UsdImaging_PurposeStrategy::value_type purposeInfo = 
        _IsEnabledPurposeCache() ?
            _delegate->_purposeCache.GetValue(prim) :
            UsdImaging_PurposeStrategy::ComputePurposeInfo(prim);

    // Inherit the instance's purpose if our prim has a fallback purpose and
    // there's an instance that provide a purpose to inherit.
    if (!purposeInfo.isInheritable &&
        !instanceInheritablePurpose.IsEmpty()) {
        return instanceInheritablePurpose;
    }

    return purposeInfo.purpose.IsEmpty() ? 
        UsdGeomTokens->default_ : purposeInfo.purpose;
}

TfToken 
UsdImagingPrimAdapter::GetInheritablePurpose(UsdPrim const& prim) const
{
    HD_TRACE_FUNCTION();

    UsdImaging_PurposeStrategy::value_type purposeInfo = 
        _IsEnabledPurposeCache() ?
            _delegate->_purposeCache.GetValue(prim) :
            UsdImaging_PurposeStrategy::ComputePurposeInfo(prim);

    return purposeInfo.GetInheritablePurpose();
}

HdCullStyle 
UsdImagingPrimAdapter::GetCullStyle(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdTimeCode time) const
{
    return HdCullStyleDontCare;
}

SdfPath
UsdImagingPrimAdapter::GetMaterialUsdPath(UsdPrim const& prim) const
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

namespace {
template<typename T>
T
_GetAttrValue(UsdAttribute const& attr, T defaultVal) {
    if (attr) {
        VtValue val;
        attr.Get(&val);
        if (!val.IsEmpty()) {
            return val.UncheckedGet<T>();
        }
    }
    return defaultVal;
}
} // anonymous namespace

HdModelDrawMode
UsdImagingPrimAdapter::GetFullModelDrawMode(UsdPrim const& prim)
{
    HdModelDrawMode modelDrawMode;

    if (!prim.IsModel()) {
        return modelDrawMode;
    }

    // Use UsdImagingDelegate methods for consistency of logic.
    modelDrawMode.drawMode = GetModelDrawMode(prim);
    modelDrawMode.applyDrawMode = _delegate->_IsDrawModeApplied(prim);

    UsdGeomModelAPI geomModelAPI(prim);

    modelDrawMode.drawModeColor = _GetAttrValue<GfVec3f>(
        geomModelAPI.GetModelDrawModeColorAttr(), GfVec3f(0.18));

    modelDrawMode.cardGeometry = _GetAttrValue<TfToken>(
        geomModelAPI.GetModelCardGeometryAttr(), modelDrawMode.cardGeometry);

    modelDrawMode.cardTextureXPos = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureXPosAttr(), SdfAssetPath());
    
    modelDrawMode.cardTextureYPos = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureYPosAttr(), SdfAssetPath());
    
    modelDrawMode.cardTextureZPos = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureZPosAttr(), SdfAssetPath());
    
    modelDrawMode.cardTextureXNeg = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureXNegAttr(), SdfAssetPath());
    
    modelDrawMode.cardTextureYNeg = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureYNegAttr(), SdfAssetPath());
    
    modelDrawMode.cardTextureZNeg = _GetAttrValue<SdfAssetPath>(
        geomModelAPI.GetModelCardTextureZNegAttr(), SdfAssetPath());  

    return modelDrawMode;
}

/*virtual*/ 
VtValue 
UsdImagingPrimAdapter::GetTopology(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time) const
{
    return VtValue();
}

/*virtual*/
GfRange3d 
UsdImagingPrimAdapter::GetExtent(UsdPrim const& prim, 
                                 SdfPath const& cachePath, 
                                 UsdTimeCode time) const
{
    return GfRange3d();
}

/*virtual*/
bool
UsdImagingPrimAdapter::GetDoubleSided(UsdPrim const& prim, 
                                      SdfPath const& cachePath, 
                                      UsdTimeCode time) const
{
    return false;
}

/*virtual*/
SdfPath 
UsdImagingPrimAdapter::GetMaterialId(UsdPrim const& prim, 
                                     SdfPath const& cachePath, 
                                     UsdTimeCode time) const
{
    return SdfPath();
}

/*virtual*/
VtValue
UsdImagingPrimAdapter::GetMaterialResource(UsdPrim const& prim, 
                              SdfPath const& cachePath, 
                              UsdTimeCode time) const
{
    return VtValue();
}

/*virtual*/
const TfTokenVector &
UsdImagingPrimAdapter::GetExtComputationSceneInputNames(
    SdfPath const& cachePath) const
{
    static TfTokenVector emptyTokenVector;
    return emptyTokenVector;
}

/*virtual*/
HdExtComputationInputDescriptorVector
UsdImagingPrimAdapter::GetExtComputationInputs(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* instancerContext) const
{
    return HdExtComputationInputDescriptorVector();
}

/*virtual*/
HdExtComputationOutputDescriptorVector
UsdImagingPrimAdapter::GetExtComputationOutputs(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* instancerContext) const
{
    return HdExtComputationOutputDescriptorVector();
}

/*virtual*/
HdExtComputationPrimvarDescriptorVector
UsdImagingPrimAdapter::GetExtComputationPrimvars(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdInterpolation interpolation,
    const UsdImagingInstancerContext* instancerContext) const
{
    return HdExtComputationPrimvarDescriptorVector();
}

/*virtual*/
VtValue 
UsdImagingPrimAdapter::GetExtComputationInput(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& name,
    UsdTimeCode time,
    const UsdImagingInstancerContext* instancerContext) const
{
    return VtValue();
}

/*virtual*/
size_t
UsdImagingPrimAdapter::SampleExtComputationInput(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& name,
    UsdTimeCode time,
    const UsdImagingInstancerContext* instancerContext,
    size_t maxSampleCount,
    float *sampleTimes,
    VtValue *sampleValues)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetExtComputationInput(prim, cachePath, name, time,
                                                 instancerContext);
        return 1;
    }
    return 0;
}

/*virtual*/ 
std::string 
UsdImagingPrimAdapter::GetExtComputationKernel(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* instancerContext) const
{
    return std::string();
}

/*virtual*/
VtValue
UsdImagingPrimAdapter::GetInstanceIndices(UsdPrim const& instancerPrim,
                                          SdfPath const& instancerCachePath,
                                          SdfPath const& prototypeCachePath,
                                          UsdTimeCode time) const
{
    return VtValue();
}

VtArray<VtIntArray>
UsdImagingPrimAdapter::GetPerPrototypeIndices(UsdPrim const& prim,
                                              UsdTimeCode time) const
{
    TRACE_FUNCTION();

    UsdImaging_PointInstancerIndicesCache &indicesCache =
        _delegate->_pointInstancerIndicesCache;

    if (_IsEnabledPointInstancerIndicesCache() &&
        indicesCache.GetTime() == time) {
        return indicesCache.GetValue(prim);
    } else {
        return UsdImaging_PointInstancerIndicesStrategy::
            ComputePerPrototypeIndices(prim, time);
    }
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

