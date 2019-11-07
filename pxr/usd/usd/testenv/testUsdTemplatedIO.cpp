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
#include "pxr/base/vt/value.h" 

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/debug.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <Python.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestTemplates()
{
    // --------------------------------------------------------------------- //
    // This test operates on /RootPrim.foo
    // and /RootPrim.foo:hidden
    // --------------------------------------------------------------------- //
    SdfPath primPath("/RootPrim");
    TfToken prop("foo");
    TfToken metaField("hidden");
    std::string propPath(primPath.GetString() + "." + prop.GetString());

    // --------------------------------------------------------------------- //
    // Author scene and compose the Stage 
    // --------------------------------------------------------------------- //
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    UsdStageRefPtr stage = UsdStage::Open(layer->GetIdentifier());

    TF_VERIFY(stage->OverridePrim(primPath),
              "Failed to create prim at %s",
              primPath.GetText());

    UsdPrim prim(stage->GetPrimAtPath(primPath));
    TF_VERIFY(prim, "Failed to get Prim from %s", primPath.GetText());

    // Grab the attribute we will be testing with.
    UsdAttribute attr =
        prim.CreateAttribute(prop, SdfValueTypeNames->Double3Array);
    TF_VERIFY(attr, "Failed to create property at %s", propPath.c_str());

    // --------------------------------------------------------------------- //
    // Setup some test data 
    // --------------------------------------------------------------------- //
    VtVec3dArray vtVecOut(1);
    VtVec3dArray vtVecIn;
    std::string tmp;

    VtValue value;

    // ===================================================================== //
    // TEST READING METADATA
    // ===================================================================== //

    // --------------------------------------------------------------------- //
    // GetMetadata & SetMetadata the value as a VtValue
    // --------------------------------------------------------------------- //
    TF_VERIFY(attr.SetMetadata(metaField, VtValue(true)),
              "VtValue: Failed to set hidden metadata at %s",
              propPath.c_str());

    // Print the layer for debugging.
    layer->ExportToString(&tmp);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Metadata -- VtValue:" << std::endl;
    std::cout << tmp << std::endl;

    // Verify the result.
    TF_VERIFY(attr.GetMetadata(metaField, &value),
              "Metadata -- VtValue: Failed to get property value at %s",
              propPath.c_str());
    TF_VERIFY(value.IsHolding<bool>(),
              "Metadata -- VtValue: not holding bool%s",
              propPath.c_str());
   TF_VERIFY(value.Get<bool>(),
              "Metadata -- VtValue: value was not true %s",
              propPath.c_str());

    // --------------------------------------------------------------------- //
    // GetMetadata & SetMetadata the value as bool 
    // --------------------------------------------------------------------- //
    bool valueIn = false;
    TF_VERIFY(attr.SetMetadata(metaField, true),
              "Metadata -- bool: Failed to set property at %s",
              propPath.c_str());

    // Print the layer for debugging.
    tmp = "";
    layer->ExportToString(&tmp);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Metadata -- bool:" << std::endl;
    std::cout << tmp << std::endl;

    // Verify Result.
    TF_VERIFY(attr.GetMetadata(metaField, &valueIn),
              "Metadata -- bool: Failed to get property value at %s",
              propPath.c_str());
    TF_VERIFY(valueIn,
              "Metadata -- bool: value was not true %s",
              propPath.c_str());

    
    // ===================================================================== //
    // TEST READING VALUES
    // ===================================================================== //
    
    // --------------------------------------------------------------------- //
    // Get & Set the value as a VtValue
    // --------------------------------------------------------------------- //
    vtVecOut[0] = GfVec3d(9,8,7);
    TF_VERIFY(attr.Set(VtValue(vtVecOut)),
              "VtValue: Failed to set property at %s",
              propPath.c_str());

    // Print the layer for debugging.
    layer->ExportToString(&tmp);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "VtValue:" << std::endl;
    std::cout << tmp << std::endl;

    // Verify the result.
    TF_VERIFY(attr.Get(&value),
              "VtValue: Failed to get property value at %s",
              propPath.c_str());
    TF_VERIFY(value.IsHolding<VtVec3dArray>(),
              "VtValue: not holding VtVec3dArray %s",
              propPath.c_str());
   TF_VERIFY(value.Get<VtVec3dArray>()[0] == vtVecOut[0],
              "VtValue: VtVec3d[0] does not match %s",
              propPath.c_str());

    // --------------------------------------------------------------------- //
    // Get & Set the value as a VtArray 
    // --------------------------------------------------------------------- //
    vtVecOut[0] = GfVec3d(6,5,4);
    TF_VERIFY(attr.Set(vtVecOut),
              "Failed to set property at %s",
              propPath.c_str());

    // Print the layer for debugging.
    tmp = "";
    layer->ExportToString(&tmp);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "VtArray:" << std::endl;
    std::cout << tmp << std::endl;

    // Verify Result.
    TF_VERIFY(attr.Get(&vtVecIn),
              "VtArray: Failed to get property value at %s",
              propPath.c_str());
    TF_VERIFY(vtVecIn[0] == vtVecOut[0],
              "VtArray: VtVec3d[0] does not match %s",
              propPath.c_str());

    // --------------------------------------------------------------------- //
    // Get & Set the value as a VtDictionary (Dictionary composition semantics
    // are exercised in testUsdMetadata).
    // --------------------------------------------------------------------- //
    VtDictionary inDict;
    inDict["$Side"] = "R";
    TF_VERIFY(!prim.HasAuthoredMetadata(SdfFieldKeys->PrefixSubstitutions));
    TF_VERIFY(prim.SetMetadata(SdfFieldKeys->PrefixSubstitutions, inDict));
    VtDictionary outDict;
    TF_VERIFY(prim.HasAuthoredMetadata(SdfFieldKeys->PrefixSubstitutions));
    // Verify bug 97783 - GetMetadata should return true if Usd was able to
    // retrieve/compose a VtDictionary.
    TF_VERIFY(prim.GetMetadata(SdfFieldKeys->PrefixSubstitutions,&outDict));
    TF_VERIFY(inDict == outDict);
    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "VtDictionary:" << std::endl;
    tmp = "";
    layer->ExportToString(&tmp);
    std::cout << tmp << std::endl;
    
}


int main()
{
    TestTemplates();

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED
}


