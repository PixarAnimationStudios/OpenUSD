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
#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceGprim::UsdImagingDataSourceGprim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
    , _primvars(nullptr)
{
    if (usdPrim.IsA<UsdGeomPointBased>()) {
        _AddCustomPrimvar(HdTokens->points, UsdGeomTokens->points);
        _AddCustomPrimvar(HdTokens->velocities, UsdGeomTokens->velocities);
        _AddCustomPrimvar(HdTokens->accelerations, UsdGeomTokens->accelerations);
        _AddCustomPrimvar(HdTokens->nonlinearSampleCount,
                          UsdGeomTokens->motionNonlinearSampleCount);
        _AddCustomPrimvar(HdTokens->blurScale,
                          UsdGeomTokens->motionBlurScale);
        _AddCustomPrimvar(HdTokens->normals, UsdGeomTokens->normals);
    }
}

void
UsdImagingDataSourceGprim::_AddCustomPrimvar(
        const TfToken &primvarName, const TfToken &attrName)
{
    _customPrimvarMapping.emplace_back(primvarName, attrName);
}

bool
UsdImagingDataSourceGprim::Has(const TfToken& name)
{
    if (name == HdPrimvarsSchemaTokens->primvars) {
        return true;
    }
    // XXX: See "GetNames()" for some stuff we're missing...

    return UsdImagingDataSourcePrim::Has(name);
}

TfTokenVector 
UsdImagingDataSourceGprim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(HdPrimvarsSchemaTokens->primvars);

    // XXX(USD-6634): Add support for the following, here or elsewhere...
#if 0
    result.push_back(HdMaterialBindingSchemaTokens->materialBinding);
    result.push_back(HdCoordSysBindingSchemaTokens->coordSysBinding);
    result.push_back(HdInstancedBySchemaTokens->instancedBy);
    result.push_back(HdCategoriesSchemaTokens->categories);
    result.push_back(HdDisplayStyleSchemaTokens->displayStyle);
#endif

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourceGprim::Get(const TfToken & name)
{
    if (name == HdPrimvarsSchemaTokens->primvars) {
        auto primvars = UsdImagingDataSourcePrimvars::AtomicLoad(_primvars);
        if (!primvars) {
            primvars = UsdImagingDataSourcePrimvars::New(
                _GetSceneIndexPath(),
                UsdGeomPrimvarsAPI(_GetUsdPrim()),
                _customPrimvarMapping,
                _GetStageGlobals());
            UsdImagingDataSourcePrimvars::AtomicStore(_primvars, primvars);
        }
        return primvars;
    }

    return UsdImagingDataSourcePrim::Get(name);
}

PXR_NAMESPACE_CLOSE_SCOPE
