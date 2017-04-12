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
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/vt/value.h" 

#include <unistd.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestPrim()
{
    SdfPath primPath("/CppFoo");
    TfToken prop("Something");
    std::string propPath(primPath.GetString() + ".Something");
    std::string value = "Foobar";
    std::string result = "not Foobar";
    VtValue tmp;

    unlink("foo.usd");
    UsdStageRefPtr stage = UsdStage::CreateNew("foo.usd");
    SdfLayerHandle layer = stage->GetRootLayer();

    {
        // Listing fields for a property on a non-existent prim path should not
        // post errors (bug 90170).
        TfErrorMark mark;
        TF_VERIFY(layer->ListFields(SdfPath("I_Do_Not_Exist.attribute")).empty());
        TF_VERIFY(mark.IsClean());
    }

    TF_VERIFY(UsdGeomXform::Define(stage, primPath),
              "Failed to create prim at %s",
              primPath.GetText());

    UsdPrim prim(stage->GetPrimAtPath(primPath));
    TF_VERIFY(prim,
              "Failed to get Prim from %s",
              primPath.GetText());

    TF_VERIFY(prim.CreateAttribute(prop, SdfValueTypeNames->String),
              "Failed to create property at %s",
              propPath.c_str());

    TF_VERIFY(prim.GetAttribute(prop).Set(VtValue(value), UsdTimeCode(0.0)),
              "Failed to set property at %s",
              propPath.c_str());

    TF_VERIFY(prim.GetAttribute(prop).Get(&tmp, UsdTimeCode(0.0)),
              "Failed to get property at %s",
              propPath.c_str());

    TF_VERIFY(tmp.IsHolding<std::string>(),
              "Invalid type for value of property %s",
              propPath.c_str());

    result = tmp.UncheckedGet<std::string>();
    TF_VERIFY(result == value,
              "Values do not match for %s, %s != %s", 
              propPath.c_str(),
              result.c_str(), 
              value.c_str());

    // Check that attribute fallback values are correctly returned for
    // time-sample queries when no time samples are present.
    UsdGeomCube cube =
        UsdGeomCube::Define(stage, SdfPath("/Cube"));
    TF_VERIFY(cube);
    UsdAttribute sizeAttr = cube.GetSizeAttr();
    TF_VERIFY(sizeAttr);

    {
        // Query default.
        double val = 0.0;
        sizeAttr.Get(&val, UsdTimeCode::Default());
        TF_VERIFY(val == 2.0);
    }

    {
        // Query at time.
        double val = 0.0;
        sizeAttr.Get(&val, UsdTimeCode(3.0));
        TF_VERIFY(val == 2.0);
    }
}

int main()
{
    TestPrim();
}

