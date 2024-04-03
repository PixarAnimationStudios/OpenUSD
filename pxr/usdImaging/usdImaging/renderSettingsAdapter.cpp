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
#include "pxr/usdImaging/usdImaging/renderSettingsAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceRenderPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderSettings.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/var.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
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

    // Check for Integrator, Sample and Display Filter Connections:
    // 1, Forward to their adapter for populating corresponding Hydra prims
    // 2. Add dependency *from* the corressponding USD prim(s) *to* the hydra
    //    render settings prim.
    {
        const TfToken outputTokens[] = {
            _tokens->outputsRiIntegrator,
            _tokens->outputsRiSampleFilters,
            _tokens->outputsRiDisplayFilters
        };

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
                    }
                }
            }
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
    UsdImagingInstancerContext const* 
    instancerContext) const
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
        return VtValue(
            UsdRenderComputeNamespacedSettings(
                prim, _GetRenderSettingsNamespaces()));
    }

    if (key == HdRenderSettingsPrimTokens->renderProducts) {
        const UsdRenderSpec renderSpec = UsdRenderComputeSpec(
            UsdRenderSettings(prim), _GetRenderSettingsNamespaces());

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
