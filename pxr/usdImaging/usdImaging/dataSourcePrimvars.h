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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIMVARS_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIMVARS_H

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class UsdImagingDataSourcePrimvars
///
/// Data source representing USD primvars. This is a container for all
/// primvars.
///
class UsdImagingDataSourcePrimvars : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePrimvars);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourcePrimvars(
            const SdfPath &sceneIndexPath,
            UsdPrim const &usdPrim,
            UsdGeomPrimvarsAPI usdPrimvars,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    static TfToken _GetPrefixedName(const TfToken &name);

    // Path of the owning prim.
    SdfPath _sceneIndexPath;

    UsdPrim _usdPrim;

    // Stage globals handle.
    const UsdImagingDataSourceStageGlobals &_stageGlobals;

    using _NamespacedPrimvarsMap = std::map<TfToken, UsdGeomPrimvar>;
    _NamespacedPrimvarsMap _namespacedPrimvars;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePrimvars);

// ----------------------------------------------------------------------------

class UsdImagingDataSourceCustomPrimvars : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceCustomPrimvars);

    USDIMAGING_API
    TfTokenVector GetNames() override;
    
    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    struct Mapping {
        Mapping(
            const TfToken &primvarName,
            const TfToken &usdAttrName,
            const TfToken &interpolation = TfToken())
          : primvarName(primvarName)
          , usdAttrName(usdAttrName)
            , interpolation(interpolation)
        { }

        TfToken primvarName;
        TfToken usdAttrName;
        TfToken interpolation;
    };

    // This map is passed to the constructor to specify non-"primvars:"
    // attributes to include as primvars (e.g., "points" and "normals").
    // The first token is the datasource name, and the second the USD name.
    using Mappings = std::vector<Mapping>;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            const TfTokenVector &properties,
            const Mappings &mappings);

private:
    UsdImagingDataSourceCustomPrimvars(
            const SdfPath &sceneIndexPath,
            UsdPrim const &usdPrim,
            const Mappings &mappings,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    // Path of the owning prim.
    SdfPath _sceneIndexPath;

    UsdPrim _usdPrim;

    // Stage globals handle.
    const UsdImagingDataSourceStageGlobals &_stageGlobals;

    using _CustomPrimvarsMap =
        std::map<TfToken,
                 std::pair<UsdAttributeQuery,
                           TfToken /* forced interpolation */>>;
    _CustomPrimvarsMap _customPrimvars;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceCustomPrimvars);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourcePrimvar
///
/// A data source representing a primvar. A primvar contains data,
/// interpolation, and role, but data can be a flat value or a value/index pair.
/// We also take location information for variability tracking.
///
/// Note: the schema for this specifies that you can return "primvarValue"
/// for a flattened value, or "indexedPrimvarValue" and "indices" for
/// an un-flattened value. Since we don't want to duplicate the flattening
/// logic, we check whether indices are present and then return only one of
/// "primvarValue" or "indexedPrimvarValue" from the result of valueQuery.
///
class UsdImagingDataSourcePrimvar : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePrimvar);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken & name) override;

private:
    UsdImagingDataSourcePrimvar(
            const SdfPath &sceneIndexPath,
            const TfToken &name,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            UsdAttributeQuery valueQuery,
            UsdAttributeQuery indicesQuery,
            HdTokenDataSourceHandle interpolation,
            HdTokenDataSourceHandle role);

private:
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
    UsdAttributeQuery _valueQuery;
    UsdAttributeQuery _indicesQuery;
    HdTokenDataSourceHandle _interpolation;
    HdTokenDataSourceHandle _role;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePrimvar);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_PRIMVARS_H
