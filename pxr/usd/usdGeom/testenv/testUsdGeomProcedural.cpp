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
#include "pxr/usd/usdGeom/procedural.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/vt/value.h" 
#include "pxr/base/arch/fileSystem.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
TestProceduralPrim()
{
    SdfPath primPath("/RootPrim");
    VtValue tmp;

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    // define a prim conforming to UsdGeomProcedural,
    // setting it to 'SomeRandomClass'
    UsdGeomProcedural procedural =
            UsdGeomProcedural::DefineClass(stage, primPath, "SomeRandomClass");
    TF_VERIFY(procedural,
              "Failed to create prim at %s",
              primPath.GetText());

    // check the defined prim
    UsdPrim prim(stage->GetPrimAtPath(primPath));
    TF_VERIFY(prim,
              "Failed to get Prim from %s",
              primPath.GetText());

    // the class attr should be created via Define already
    UsdAttribute classAttr = procedural.GetClassAttr();
    TF_VERIFY(classAttr,
              "Failed to get 'procedural:class' attribute via UsdGeomProcedural at %s",
              primPath.GetText());

    // verify the prim has the 'class' attribute
    TF_VERIFY(prim.GetAttribute(UsdGeomTokens->proceduralClass),
              "Failed to get 'procedural:class' attribute at prim %s",
              primPath.GetText());

    // check the value of 'class' attribute
    TF_VERIFY(classAttr.Get(&tmp, UsdTimeCode(0.0)),
              "Failed to get 'procedural:class' value at %s",
              primPath.GetText());
    std::string result = tmp.Get<TfToken>().GetString();
    TF_VERIFY(result == "SomeRandomClass",
              "procedural:class attribute at %s expected to be SomeRandomClass, not %s",
              primPath.GetText(), result.c_str());

    // this should fail
    TF_VERIFY(!(procedural.GetProceduralAttr("some_random_attr").IsValid()),
              "%s should not contain attribute 'some_random_attr' before its creation",
              primPath.GetText());

    // create procedural (arbitrary, typed) attribute
    TF_VERIFY(procedural.CreateProceduralAttr("some_random_attr", SdfValueTypeNames->Float, VtValue(2.0f)),
            "Failed to create procedural attribute at prim %s",
            primPath.GetText());

    // now we should be able to get it
    TF_VERIFY(procedural.GetProceduralAttr("some_random_attr"),
              "Failed to get procedural attribute 'some_random_attr' at prim %s",
              primPath.GetText());

    // get the value
    // ensure the token is prefixed correctly with 'procedural'
    TfToken randomAttrToken("procedural:some_random_attr");
    TF_VERIFY(prim.GetAttribute(randomAttrToken).Get(&tmp, UsdTimeCode(0.0)),
              "Failed to get 'some_random_attr' value at %s",
              primPath.GetText());

    // check the value type
    TF_VERIFY(tmp.IsHolding<float>(),
              "Expected type float for value of 'some_random_attr' at %s",
              primPath.GetText());

    // check the default value (2.0)
    float attrValueFloat = tmp.UncheckedGet<float>();
    TF_VERIFY(attrValueFloat == 2.0f,
              "Expected default value of 2.0, not %f, for 'some_random_attr' at %s",
              attrValueFloat,
              primPath.GetText());

    // change the value
    TF_VERIFY(prim.GetAttribute(randomAttrToken).Set(VtValue(4.2f), UsdTimeCode(0.0)),
              "Failed to set 'some_random_attr' at %s",
              primPath.GetText());

    // check the new values
    TF_VERIFY(procedural.GetProceduralAttr("some_random_attr").Get(&attrValueFloat, UsdTimeCode(0.0)),
              "Failed to get value for procedural attribute 'some_random_attr' at prim %s",
              primPath.GetText());
    TF_VERIFY(attrValueFloat == 4.2f,
              "Expected new value of 4.2 for 'some_random_attr' at %s",
              primPath.GetText());
}

int main()
{
    TestProceduralPrim();
}

