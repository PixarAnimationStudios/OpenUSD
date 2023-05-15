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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class UsdImagingDataSourceGprim
///
/// Data source representing a USD gprim. This is the common base for geometric
/// types and includes features such as materials and primvars.
///
class UsdImagingDataSourceGprim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceGprim);

    /// Returns the data source representing \p name, if valid.
    ///
    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

protected:

    /// Use to construct a new UsdImagingDataSourceGprim.
    ///
    /// \p sceneIndexPath is the path of this object in the scene index.
    ///
    /// \p usdPrim is the USD prim object that this data source represents.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// Note: client code calls this via static New.
    USDIMAGING_API
    UsdImagingDataSourceGprim(
            const SdfPath &sceneIndexPath,
            UsdPrim usdPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceGprim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_GPRIM_H
