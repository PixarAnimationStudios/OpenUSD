//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceDashDotLines.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

#include "pxr/usd/usdGeom/dashDotLines.h"

#include "pxr/imaging/hd/dashDotLinesSchema.h"
#include "pxr/imaging/hd/dashDotLinesTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
    const UsdImagingDataSourceCustomPrimvars::Mappings&
        _GetCustomPrimvarMappings(const UsdPrim& usdPrim)
    {
        // Note that both the patternPartCount and pattern primvar will map to the pattern attribute.
        static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
            { HdTokens->pattern, UsdGeomTokens->pattern, HdPrimvarSchemaTokens->constant },
            { HdTokens->patternPeriod, UsdGeomTokens->patternPeriod, HdPrimvarSchemaTokens->constant },
            { HdTokens->patternScale, UsdGeomTokens->patternScale, HdPrimvarSchemaTokens->constant },
            { HdTokens->startCapType, UsdGeomTokens->startCapType, HdPrimvarSchemaTokens->constant },
            { HdTokens->endCapType, UsdGeomTokens->endCapType, HdPrimvarSchemaTokens->constant },
        };

        return mappings;
    }
}

UsdImagingDataSourceDashDotLinesTopology
::UsdImagingDataSourceDashDotLinesTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomDashDotLines usdDashDotLines,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdDashDotLines(usdDashDotLines)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceDashDotLinesTopology::GetNames()
{
    return {
        HdDashDotLinesTopologySchemaTokens->curveVertexCounts,
        HdDashDotLinesTopologySchemaTokens->shapeDetail,
        HdDashDotLinesTopologySchemaTokens->screenSpacePattern,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceDashDotLinesTopology::Get(const TfToken &name)
{
    if (name == HdDashDotLinesTopologySchemaTokens->curveVertexCounts) {
        static const HdDataSourceLocator locator =
            HdDashDotLinesTopologySchema::GetDefaultLocator().Append(name);
        return UsdImagingDataSourceAttribute<VtIntArray>::New(
                _usdDashDotLines.GetCurveVertexCountsAttr(), _stageGlobals,
                _sceneIndexPath, locator);
    }
    else if (name == HdDashDotLinesTopologySchemaTokens->shapeDetail) {
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _usdDashDotLines.GetShapeDetailAttr(), _stageGlobals);
    }
    else if (name == HdDashDotLinesTopologySchemaTokens->screenSpacePattern) {
        return UsdImagingDataSourceAttribute<bool>::New(
            _usdDashDotLines.GetScreenSpacePatternAttr(), _stageGlobals);
    }
    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceDashDotLines::UsdImagingDataSourceDashDotLines(
        const SdfPath &sceneIndexPath,
        UsdGeomDashDotLines usdDashDotLines,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdDashDotLines(usdDashDotLines)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceDashDotLines::GetNames()
{
    return {
        HdDashDotLinesSchemaTokens->topology,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceDashDotLines::Get(const TfToken &name)
{
    if (name == HdDashDotLinesSchemaTokens->topology) {
        return UsdImagingDataSourceDashDotLinesTopology::New(
            _sceneIndexPath, _usdDashDotLines, _stageGlobals);
    }

    return nullptr;
}
// ----------------------------------------------------------------------------

UsdImagingDataSourceDashDotLinesPrim::UsdImagingDataSourceDashDotLinesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceDashDotLinesPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdDashDotLinesSchema::GetSchemaToken());

    return result;
}


HdDataSourceBaseHandle 
UsdImagingDataSourceDashDotLinesPrim::Get(const TfToken &name)
{
    if (name == HdDashDotLinesSchema::GetSchemaToken()) {
        return UsdImagingDataSourceDashDotLines::New(
                _GetSceneIndexPath(),
                UsdGeomDashDotLines(_GetUsdPrim()),
                _GetStageGlobals());
    }

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                HdContainerDataSource::Cast(
                    UsdImagingDataSourceGprim::Get(name)),
                UsdImagingDataSourceCustomPrimvars::New(
                    _GetSceneIndexPath(),
                    _GetUsdPrim(),
                    _GetCustomPrimvarMappings(_GetUsdPrim()),
                    _GetStageGlobals()));
    }

    return UsdImagingDataSourceGprim::Get(name);
}

/*static*/
HdDataSourceLocatorSet
UsdImagingDataSourceDashDotLinesPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    UsdImagingPropertyInvalidationType invalidationType)
{
    HdDataSourceLocatorSet result;

    if (subprim.IsEmpty()) {
        result = UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType);

        for (const TfToken &propertyName : properties) {
            if (propertyName == UsdGeomTokens->curveVertexCounts) {
                result.insert(HdDashDotLinesTopologySchema::GetDefaultLocator()
                    .Append(
                        HdDashDotLinesTopologySchemaTokens->curveVertexCounts));
            } else if (propertyName == UsdGeomTokens->shapeDetail) {
                result.insert(HdDashDotLinesTopologySchema::GetDefaultLocator()
                    .Append(HdDashDotLinesTopologySchemaTokens->shapeDetail));
            } else if (propertyName == UsdGeomTokens->screenSpacePattern) {
                result.insert(HdDashDotLinesTopologySchema::GetDefaultLocator()
                    .Append(HdDashDotLinesTopologySchemaTokens->screenSpacePattern));
            }
        }

        result.insert(
            UsdImagingDataSourceCustomPrimvars::Invalidate(
                properties, _GetCustomPrimvarMappings(prim)));
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
