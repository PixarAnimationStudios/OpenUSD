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
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{

// Test that relationship target and attribute connection specs
// created in the .usd file formats have the appropriate spec
// type.
void TestTargetSpecs()
{
    for (const std::string& fmt : {"usda", "usdc"}) {
        UsdStageRefPtr stage = 
            UsdStage::CreateInMemory("TestTargetSpecs." + fmt);
        
        UsdPrim prim = stage->DefinePrim(SdfPath("/Test"));

        UsdAttribute attr = prim.CreateAttribute(
            TfToken("attr"), SdfValueTypeNames->Int);
        TF_AXIOM(attr.AddConnection(SdfPath("/Test.dummy")));

        SdfSpecType connSpecType = stage->GetRootLayer()->GetSpecType(
            attr.GetPath().AppendTarget(SdfPath("/Test.dummy")));
        TF_AXIOM(connSpecType == SdfSpecTypeConnection);                

        UsdRelationship rel = prim.CreateRelationship(TfToken("rel"));
        TF_AXIOM(rel.AddTarget(SdfPath("/Test.dummy")));

        SdfSpecType relSpecType = stage->GetRootLayer()->GetSpecType(
            rel.GetPath().AppendTarget(SdfPath("/Test.dummy")));
        TF_AXIOM(relSpecType == SdfSpecTypeRelationshipTarget);
    }
}

}

int 
main(int argc, char** argv)
{
    TestTargetSpecs();

    return 0;
}
