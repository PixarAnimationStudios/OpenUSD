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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H

#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceMaterial
///
/// A data source mapping to HdMaterialSchema from an UsdShadeMaterial prim
///
class UsdImagingDataSourceMaterial : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceMaterial);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    ~UsdImagingDataSourceMaterial();

private:

    /// If \p fixedTerminalName is specified, the provided \p usdPrim will be
    /// treated as the terminal shader node in the graph rather than as a
    /// material (with relationships to the prims serving as terminal shader
    /// nodes). This is relevant for light and light filter cases.
    UsdImagingDataSourceMaterial(
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const TfToken &fixedTerminalName = TfToken()
        );


private:

    UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;

    // Cache the networks by context name.
    using _ContextMap = tbb::concurrent_unordered_map<
        TfToken, HdDataSourceBaseHandle, TfToken::HashFunctor>;


    TfToken _fixedTerminalName;
    _ContextMap _networks;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceMaterial);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_MATERIAL_H
