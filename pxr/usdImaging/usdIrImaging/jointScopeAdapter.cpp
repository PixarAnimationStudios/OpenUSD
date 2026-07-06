//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdIrImaging/jointScopeAdapter.h"

#include "pxr/usdImaging/usdIrImaging/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/execIr/tokens.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdIrImagingJointScopeAdapter;
    const TfType type =
        TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    type.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

TfTokenVector
UsdIrImagingJointScopeAdapter::GetImagingSubprims(
    UsdPrim const& prim)
{
    return {UsdIrImagingTokens->baseSphere, UsdIrImagingTokens->zAxisCone};
}

TfToken
UsdIrImagingJointScopeAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim == UsdIrImagingTokens->baseSphere) {
        return HdPrimTypeTokens->sphere;
    }
    if (subprim == UsdIrImagingTokens->zAxisCone) {
        return HdPrimTypeTokens->cone;
    }

    if (!subprim.IsEmpty()) {
        TF_CODING_ERROR("Unsupported subprim '%s'", subprim.GetText());
    }
    return {};
}

static HdDataSourceBaseHandle
_GetDisplayColorDataSource(
    UsdPrim const& prim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    const UsdAttribute colorAttr = prim.GetAttribute(
        ExecIrTokens->guideDisplayColor);
    TF_VERIFY(colorAttr);

    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(
            UsdImagingDataSourceAttribute<GfVec3f>::New(colorAttr, stageGlobals))
        .SetInterpolation(
            HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant))
        .Build();
}

static HdDataSourceBaseHandle
_GetDisplayOpacityDataSource(
    UsdPrim const& prim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    const UsdAttribute opacityAttr = prim.GetAttribute(
        ExecIrTokens->guideDisplayOpacity);
    TF_VERIFY(opacityAttr);

    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(
            UsdImagingDataSourceAttribute<float>::New(opacityAttr, stageGlobals))
        .SetInterpolation(
            HdPrimvarSchema::BuildInterpolationDataSource(
                HdPrimvarSchemaTokens->constant))
        .Build();
}

HdContainerDataSourceHandle
UsdIrImagingJointScopeAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    static const TfToken primvarTokens[] =
        {HdTokens->displayColor, HdTokens->displayOpacity};

    if (subprim == UsdIrImagingTokens->baseSphere) {
        const HdDataSourceBaseHandle primvarValues[] = {
            _GetDisplayColorDataSource(prim, stageGlobals),
            _GetDisplayOpacityDataSource(prim, stageGlobals)
        };

        return HdRetainedContainerDataSource::New(
            HdSphereSchema::GetSchemaToken(),
            HdSphereSchema::Builder().Build(),

            HdPurposeSchema::GetSchemaToken(),
            HdPurposeSchema::Builder()
                .SetPurpose(HdRetainedTypedSampledDataSource<TfToken>::New(
                                HdRenderTagTokens->guide))
                .Build(),

            HdPrimvarsSchema::GetSchemaToken(),
            HdPrimvarsSchema::BuildRetained(2, primvarTokens, primvarValues)
        );
    }
    if (subprim == UsdIrImagingTokens->zAxisCone) {
        const UsdAttribute lengthAttr =
            prim.GetAttribute(ExecIrTokens->guideLength);
        TF_VERIFY(lengthAttr);

        const HdDataSourceBaseHandle primvarValues[] = {
            _GetDisplayColorDataSource(prim, stageGlobals),
            _GetDisplayOpacityDataSource(prim, stageGlobals)
        };

        return HdRetainedContainerDataSource::New(
            HdConeSchema::GetSchemaToken(),
            HdConeSchema::Builder()
                .SetAxis(HdRetainedTypedSampledDataSource<TfToken>::New(
                             HdConeSchemaTokens->Z))
                // The height of the cone is the value of the guide:length
                // attribute.
                .SetHeight(UsdImagingDataSourceAttribute<double>::New(
                               lengthAttr, stageGlobals))
                .Build(),

            // Translate the cone so the base sits at Z=0 (based on the value of
            // guide:length).
            HdXformSchema::GetSchemaToken(),
            HdXformSchema::Builder()
                .SetMatrix(
                    [&] {
                        double length = 0;
                        if (lengthAttr) {
                            lengthAttr.Get(&length);
                        }
                        return HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                            GfMatrix4d(
                                GfRotation({1, 0, 0}, 0), {0, 0, length/2.0}));
                    }())
                .Build(),

            HdPurposeSchema::GetSchemaToken(),
            HdPurposeSchema::Builder()
                .SetPurpose(HdRetainedTypedSampledDataSource<TfToken>::New(
                                HdRenderTagTokens->guide))
                .Build(),

            HdPrimvarsSchema::GetSchemaToken(),
            HdPrimvarsSchema::BuildRetained(2, primvarTokens, primvarValues)
        );
    }

    if (!subprim.IsEmpty()) {
        TF_CODING_ERROR("Unsupported subprim '%s'", subprim.GetText());
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdIrImagingJointScopeAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    // Fully invalidate the cone if any of the guide attributes have changed.
    // This ensures that we will re-build the retained data sources that depend
    // on the values of these attributes.
    if (subprim == UsdIrImagingTokens->zAxisCone) {
        if (const auto it = std::find_if(
                properties.begin(), properties.end(),
                [](const TfToken &propName) {
                    return
                        propName == ExecIrTokens->guideLength ||
                        propName == ExecIrTokens->guideDisplayColor ||
                        propName == ExecIrTokens->guideDisplayOpacity;
                });
            it != properties.end()) {
            return HdDataSourceLocatorSet::UniversalSet();
        }
    }

    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
