//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/dataSourceNurbsPatch.h"

#include "pxr/usdImaging/usdImaging/dataSourceSchemaBased.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include "pxr/imaging/hd/nurbsPatchSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// _NurbsPatchDataSource is a container data source for locator
// nurbsPatch.
//
// It contains all non-space USD attributes of the UsdGeomNurbsPatch schema
// (that is, the name contains no ":"). This means it skips the
// trimCurve prefixed attributes from UsdGeomNurbsPatch as well as the
// pointWeights (which is a primvar) and the primvars from UsdGeomGprim.
//
struct _NurbsPatchTranslator
{
    static
    TfToken
    UsdAttributeNameToHdName(const TfToken &name)
    {
        if (SdfPath::TokenizeIdentifier(name.GetString()).size() != 1) {
            return TfToken();
        }
        if (name == UsdGeomTokens->pointWeights) {
            return TfToken();
        }

        return name;
    }

    static
    HdDataSourceLocator
    GetContainerLocator()
    {
        return HdNurbsPatchSchema::GetDefaultLocator();
    }
};

using _NurbsPatchDataSource = UsdImagingDataSourceSchemaBased<
    UsdGeomNurbsPatch,
    /* UsdSchemaBaseTypes = */ std::tuple<UsdGeomGprim>,
    _NurbsPatchTranslator>;

// _TrimCurveTranslator is a container data source for locator
// nurbsPatch/trimCurve.
//
// It contains all USD attributes in the trimCurve namespace
// of the UsdGeomNurbsPatch schema (prefixed by trimCurve:).
//
struct _TrimCurveTranslator
{
    static
    TfToken
    UsdAttributeNameToHdName(const TfToken &name)
    {
        static const TfToken prefix("trimCurve");

        const std::pair<std::string, bool> strippedName =
            SdfPath::StripPrefixNamespace(name, prefix);
        if (strippedName.second) {
            return TfToken(strippedName.first);
        }
        return TfToken();
    }

    static
    HdDataSourceLocator
    GetContainerLocator()
    {
        return HdNurbsPatchTrimCurveSchema::GetDefaultLocator();
    }
};

using _TrimCurveDataSource = UsdImagingDataSourceSchemaBased<
    UsdGeomNurbsPatch,
    /* UsdSchemaBaseTypes = */ std::tuple<>,
    _TrimCurveTranslator>;

const UsdImagingDataSourceCustomPrimvars::Mappings &
_GetCustomPrimvarMappings(const UsdPrim &usdPrim)
{
    static const TfToken &hdPrimvarName = UsdGeomTokens->pointWeights;

    static const UsdImagingDataSourceCustomPrimvars::Mappings mappings = {
        { hdPrimvarName, UsdGeomTokens->pointWeights },
    };

    return mappings;
}

}

// ----------------------------------------------------------------------------

UsdImagingDataSourceNurbsPatchPrim::UsdImagingDataSourceNurbsPatchPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourceGprim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceNurbsPatchPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourceGprim::GetNames();
    result.push_back(HdNurbsPatchSchema::GetSchemaToken());
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceNurbsPatchPrim::Get(const TfToken & name)
{
    if (name == HdNurbsPatchSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::New(
                // Container for locator nurbsPatch
                _NurbsPatchDataSource::New(
                    _GetSceneIndexPath(),
                    UsdGeomNurbsPatch(_GetUsdPrim()),
                    _GetStageGlobals()),
                HdRetainedContainerDataSource::New(
                    HdNurbsPatchTrimCurveSchemaTokens->trimCurve,
                    // Container for locator nurbsPatch/trimCurve
                    _TrimCurveDataSource::New(
                        _GetSceneIndexPath(),
                        UsdGeomNurbsPatch(_GetUsdPrim()),
                        _GetStageGlobals())));
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

HdDataSourceLocatorSet
UsdImagingDataSourceNurbsPatchPrim::Invalidate(
    UsdPrim const& prim,
    const TfToken &subprim,
    const TfTokenVector &properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    TRACE_FUNCTION();

    HdDataSourceLocatorSet locators =
        _NurbsPatchDataSource::Invalidate(
            subprim, properties);

    locators.insert(
        _TrimCurveDataSource::Invalidate(
            subprim, properties));

    locators.insert(
        UsdImagingDataSourceGprim::Invalidate(
            prim, subprim, properties, invalidationType));

    locators.insert(
        UsdImagingDataSourceCustomPrimvars::Invalidate(
            properties, _GetCustomPrimvarMappings(prim)));

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
