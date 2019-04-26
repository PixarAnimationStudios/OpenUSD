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

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestHasAPI()
{
    auto stage = UsdStage::CreateInMemory("TestPrimQueries.usd");
    auto path = SdfPath("/p");
    auto prim = stage->DefinePrim(path);
    
    // Valid cases
    assert(!prim.HasAPI<UsdGeomMotionAPI>());
    UsdGeomMotionAPI::Apply(prim);
    assert(prim.HasAPI<UsdGeomMotionAPI>());

    assert(!prim.HasAPI<UsdGeomModelAPI>());
    UsdGeomModelAPI::Apply(prim);
    assert(prim.HasAPI<UsdGeomModelAPI>());

    std::cerr << "--- BEGIN EXPECTED ERROR --" << std::endl;
    TfErrorMark mark;
    // Passing in a non-empty instance name with a single-apply API schema like
    // UsdGeomMotionAPI results in a coding error
    assert(!prim.HasAPI<UsdGeomMotionAPI>(/*instanceName*/ TfToken("instance")));
    TF_VERIFY(!mark.IsClean());
    std::cerr << "--- END EXPECTED ERROR --" << std::endl;

    // The following cases won't compile, uncomment them to confirm
    // assert(prim.HasAPI<UsdGeomImageable>()); // can't be typed
    // assert(prim.HasAPI<UsdGeomXform>());     // can't be concrete
    // assert(!prim.HasAPI<UsdGeomModelAPI>()); // can't be non-applied API schema

}
 
int main()
{
    TestHasAPI();
}
