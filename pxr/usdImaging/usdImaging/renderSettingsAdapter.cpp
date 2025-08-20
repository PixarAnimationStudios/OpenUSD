//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/renderSettingsAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceRenderPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderSettings.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/var.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(LEGACY_PXR_RENDER_TERMINALS_API_ALLOWED_AND_WARN, true,
    "By default, we allow specification of connections for display "
    "filters, sample filters, and integrators to propagate to RenderSettings "
    "while producing a warning prompting users to specify relationships "
    "instead. In a future release, this will be updated to 'false', "
    "disallowing specification of connections and requiring relationships "
    "to specify display filters, sample filters, and integrators.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (RenderSettings)
    ((riIntegrator, "ri:integrator"))
    ((riSampleFilters, "ri:sampleFilters"))
    ((riDisplayFilters, "ri:displayFilters"))
    // Deprecated in favor of corresponding ri* tokens
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingRenderSettingsAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingRenderSettingsAdapter::~UsdImagingRenderSettingsAdapter() 
{
}

// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdImagingRenderSettingsAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingRenderSettingsAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->renderSettings;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingRenderSettingsAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceRenderSettingsPrim::New(
                    prim.GetPath(), prim, stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingRenderSettingsAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceRenderSettingsPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
// -------------------------------------------------------------------------- //

bool
UsdImagingRenderSettingsAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return index->IsBprimTypeSupported(HdPrimTypeTokens->renderSettings);
}

// XXX: We explicitly populate PxrRenderTerminalsAPI relationships
// to RenderSettings, avoiding populating all relationships; this
// https://jira.pixar.com/browse/HYD-3280
void
_StripRelsFromSettings(
    UsdPrim const& prim,
    VtDictionary *settings)
{
    std::vector<std::string> toErase;
    for (const auto& it : *settings) {
        const TfToken name = TfToken(it.first);
        UsdRelationship rel = prim.GetRelationship(name);
        if (rel && name != _tokens->riIntegrator
                && name != _tokens->riSampleFilters
                && name != _tokens->riDisplayFilters) {
            toErase.push_back(it.first);
        }
    }

    for (const std::string& name : toErase) {
        settings->erase(name);
    }
}

SdfPath
UsdImagingRenderSettingsAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    SdfPath const rsPrimPath = prim.GetPath();
    index->InsertBprim(HdPrimTypeTokens->renderSettings, rsPrimPath, prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    // Find render products (and transitively) render var prims targeted by
    // this prim and add dependency *from* the USD prim(s) *to* the hydra
    // render settings prim. This is necessary because we *don't* populate
    // Hydra prims for render product and render var USD prims and thus have to
    // forward change notices from the USD prims to the hydra render settings
    // prim.
    //
    // XXX Populate a cache to hold the targeting settings prim for each product
    //     and var to aid with change processing.
    {   
        UsdRenderSettings rs(prim);
        SdfPathVector targets;
        rs.GetProductsRel().GetForwardedTargets(&targets);

        for (SdfPath const & target : targets) {
            if (UsdRenderProduct product =
                    UsdRenderProduct(prim.GetStage()->GetPrimAtPath(target))) {

                index->AddDependency(/* to   */rsPrimPath,
                                     /* from */product.GetPrim());

                SdfPathVector renderVarPaths;
                product.GetOrderedVarsRel().GetForwardedTargets(
                    &renderVarPaths);
                for (SdfPath const& renderVarPath: renderVarPaths ) {
                    UsdPrim rv = prim.GetStage()->GetPrimAtPath(renderVarPath);
                    if (rv && rv.IsA<UsdRenderVar>()) {
                        index->AddDependency(/* to   */rsPrimPath,
                                             /* from */rv);
                    }
                }
            }
        }
    }

    // XXX: This code is PxrRenderTerminalsAPI-specific, a schema that
    // comes from renderman. Therefore this should be moved to
    // usdRiPxrImaging/renderTerminalsAPIAdapter in an upcoming change.
    // https://jira.pixar.com/browse/HYD-3280
    const UsdSchemaRegistry &reg = UsdSchemaRegistry::GetInstance();
    const UsdPrimDefinition* rsDef = reg.FindConcretePrimDefinition(
        _tokens->RenderSettings);
    const TfTokenVector &rsPropNames = rsDef->GetPropertyNames();
    const bool rsSchemaHasRelationships = std::find(
        rsPropNames.begin(), rsPropNames.end(), _tokens->riIntegrator)
        != rsPropNames.end();

    // Check for Integrator, Sample and Display Filter relationships:
    // 1, Forward to their adapter for populating corresponding Hydra prims
    // 2. Add dependency *from* the corresponding USD prim(s) *to* the hydra
    //    render settings prim.
    bool populatedRelationships = false;
    if (rsSchemaHasRelationships) {
        const TfToken outputTokens[] = {
            _tokens->riIntegrator,
            _tokens->riSampleFilters,
            _tokens->riDisplayFilters
        };

        for (const auto &token : outputTokens) {
            SdfPathVector relationships;
            SdfPathVector targets;
            prim.GetRelationship(token).GetTargets(&targets);
            for (auto const& targetPath : targets) {
                const UsdPrim &targetPrim = prim.GetStage()->GetPrimAtPath(
                    targetPath.GetPrimPath());
                if (targetPrim) {
                    UsdImagingPrimAdapterSharedPtr adapter =
                        _GetPrimAdapter(targetPrim);
                    if (adapter) {
                        index->AddDependency(/* to   */prim.GetPath(),
                                             /* from */targetPrim);
                        adapter->Populate(targetPrim, index, nullptr);
                        populatedRelationships = true;
                    }
                }
            }
        }
    }

    // The following behavior is deprecated in favor of the above block.
    // Check for Integrator, Sample and Display Filter Connections:
    // 1, Forward to their adapter for populating corresponding Hydra prims
    // 2. Add dependency *from* the corresponding USD prim(s) *to* the hydra
    //    render settings prim.
    const bool terminalsWarn =
        TfGetEnvSetting(LEGACY_PXR_RENDER_TERMINALS_API_ALLOWED_AND_WARN);
    if (!populatedRelationships && (
            !rsSchemaHasRelationships || terminalsWarn)) {
        const TfToken outputTokens[] = {
            _tokens->outputsRiIntegrator,
            _tokens->outputsRiSampleFilters,
            _tokens->outputsRiDisplayFilters
        };

        bool populatedAdaptor = false;
        for (const auto &token : outputTokens) {
            SdfPathVector connections;
            prim.GetAttribute(token).GetConnections(&connections);
            for (auto const& connPath : connections) {
                const UsdPrim &connPrim = prim.GetStage()->GetPrimAtPath(
                    connPath.GetPrimPath());
                if (connPrim) {
                    UsdImagingPrimAdapterSharedPtr adapter =
                        _GetPrimAdapter(connPrim);
                    if (adapter) {
                        index->AddDependency(/* to   */prim.GetPath(),
                                             /* from */connPrim);
                        adapter->Populate(connPrim, index, nullptr);
                        populatedAdaptor = true;
                    }
                }
            }
        }
        if (populatedAdaptor && rsSchemaHasRelationships) {
            TF_WARN("outputs:ri:sampleFilters, outputs:ri:displayFilters, "
                    "outputs:ri:integrator on RenderSettings are deprecated "
                    "in favor of ri:sampleFilters, ri:displayFilters, "
                    "ri:integrator.");
        }
    }

    return prim.GetPath();
}

void
UsdImagingRenderSettingsAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveBprim(HdPrimTypeTokens->renderSettings, cachePath);
}

void 
UsdImagingRenderSettingsAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // If any of the RenderSettings attributes are time varying 
    // we will assume all RenderSetting params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyParams;
        }
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingRenderSettingsAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
}

HdDirtyBits
UsdImagingRenderSettingsAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    if (propertyName == UsdRenderTokens->includedPurposes) {
        return HdRenderSettings::DirtyIncludedPurposes;
    }
    if (propertyName == UsdRenderTokens->materialBindingPurposes) {
        return HdRenderSettings::DirtyMaterialBindingPurposes;
    }
    if (propertyName == UsdRenderTokens->renderingColorSpace) {
        return HdRenderSettings::DirtyRenderingColorSpace;
    }
    // XXX Bucket all other changes as product or namespaced setting related.
    return HdRenderSettings::DirtyNamespacedSettings |
           HdRenderSettings::DirtyRenderProducts;
}

void
UsdImagingRenderSettingsAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkBprimDirty(cachePath, dirty);
}

namespace {

static HdRenderSettings::RenderProducts
_ToHdRenderProducts(UsdRenderSpec const &renderSpec)
{
    HdRenderSettings::RenderProducts hdProducts;
    hdProducts.reserve(renderSpec.products.size());

    for (auto const &product : renderSpec.products) {
        HdRenderSettings::RenderProduct hdProduct;
        hdProduct.productPath = product.renderProductPath;
        hdProduct.type = product.type;
        hdProduct.name = product.name;
        hdProduct.resolution = product.resolution;

        hdProduct.renderVars.reserve(product.renderVarIndices.size());
        for (size_t const &varId : product.renderVarIndices) {
            UsdRenderSpec::RenderVar const &rv =
                renderSpec.renderVars[varId];
            HdRenderSettings::RenderProduct::RenderVar hdVar;
            hdVar.varPath = rv.renderVarPath;
            hdVar.dataType = rv.dataType;
            hdVar.sourceName = rv.sourceName;
            hdVar.sourceType = rv.sourceType;
            hdVar.namespacedSettings = rv.namespacedSettings;

            hdProduct.renderVars.push_back(std::move(hdVar));
        }

        hdProduct.cameraPath = product.cameraPath;
        hdProduct.pixelAspectRatio = product.pixelAspectRatio;
        hdProduct.aspectRatioConformPolicy =
            product.aspectRatioConformPolicy;
        hdProduct.apertureSize = product.apertureSize;
        hdProduct.dataWindowNDC = product.dataWindowNDC;

        hdProduct.disableMotionBlur = product.disableMotionBlur;
        hdProduct.disableDepthOfField = product.disableDepthOfField;
        hdProduct.namespacedSettings = product.namespacedSettings;

        hdProducts.push_back(std::move(hdProduct));
    }

    return hdProducts;
}

}

VtValue
UsdImagingRenderSettingsAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    // Gather authored settings attributes on the render settings prim.
    if (key == HdRenderSettingsPrimTokens->namespacedSettings) {
        VtDictionary settings = UsdRenderComputeNamespacedSettings(
            prim, _GetRenderSettingsNamespaces());
        _StripRelsFromSettings(prim, &settings);
        return VtValue(settings);
    }

    if (key == HdRenderSettingsPrimTokens->renderProducts) {
        UsdRenderSpec renderSpec = UsdRenderComputeSpec(
            UsdRenderSettings(prim), _GetRenderSettingsNamespaces());
        for (UsdRenderSpec::Product& product : renderSpec.products) {
            UsdPrim rpPrim
                = prim.GetStage()->GetPrimAtPath(product.renderProductPath);
            _StripRelsFromSettings(rpPrim, &product.namespacedSettings);
        }
        _StripRelsFromSettings(prim, &(renderSpec.namespacedSettings));

        return VtValue(_ToHdRenderProducts(renderSpec));
    }

    if (key == HdRenderSettingsPrimTokens->includedPurposes) {
        VtArray<TfToken> purposes;
        UsdRenderSettings(prim).GetIncludedPurposesAttr().Get(&purposes);
        return VtValue(purposes);
    }

    if (key == HdRenderSettingsPrimTokens->materialBindingPurposes) {
        VtArray<TfToken> purposes;
        UsdRenderSettings(prim).GetMaterialBindingPurposesAttr().Get(&purposes);
        return VtValue(purposes);
    }

    if (key == HdRenderSettingsPrimTokens->renderingColorSpace) {
        TfToken colorSpace;
        UsdRenderSettings(prim).GetRenderingColorSpaceAttr().Get(&colorSpace);
        return VtValue(colorSpace);
    }

    TF_CODING_ERROR(
        "Property %s not supported for RenderSettings by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
