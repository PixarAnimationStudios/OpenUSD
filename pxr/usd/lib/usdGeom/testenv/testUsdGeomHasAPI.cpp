//
// Copyright 2017 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/motionAPI.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/modelAPI.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
TestHasAPI()
{
    auto stage = UsdStage::CreateInMemory("TestPrimQueries.usd");
    auto path = SdfPath("/p");
    auto prim = stage->DefinePrim(path);
    
    // Valid cases
    assert(!prim.HasAPI<UsdGeomMotionAPI>());
    UsdGeomMotionAPI::Apply(stage, path);
    assert(prim.HasAPI<UsdGeomMotionAPI>());

    // Ensure both UsdModelAPI and UsdGeomModelAPI get picked up
    // as we check for derived schema classes
    assert(!prim.HasAPI<UsdGeomModelAPI>());
    assert(!prim.HasAPI<UsdModelAPI>());
    UsdGeomModelAPI::Apply(stage, path);
    assert(prim.HasAPI<UsdGeomModelAPI>());
    assert(prim.HasAPI<UsdModelAPI>());

    // The following cases won't compile, uncomment them to confirm
    // assert(prim.HasAPI<UsdGeomImageable>()); // cant be typed
    // assert(prim.HasAPI<UsdGeomXform>());     // cant be concrete
}
 
int main()
{
    TestHasAPI();
}
