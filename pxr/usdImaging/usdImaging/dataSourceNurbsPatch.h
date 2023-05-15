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

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceNurbsPatchPrim
///
/// A prim data source representing UsdNurbsPatch.
///
class UsdImagingDataSourceNurbsPatchPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceNurbsPatchPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceNurbsPatchPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceNurbsPatchPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H
