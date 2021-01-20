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
#include "pxr/usd/usd/references.h"
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

// Tests the behavior and return values of the GetConnections for attributes 
// and GetTargets/GetForwardedTargets for relationships. The boolean return
// values are not part of the python API so we test them for the C++ API.
void TestGetTargetsAndConnections()
{
    for (const std::string& fmt : {"usda", "usdc"}) {
        UsdStageRefPtr stage = 
            UsdStage::CreateInMemory("TestGetTargets." + fmt);
        
        // Add an attribute to test connections first.        
        UsdPrim attrPrim = stage->DefinePrim(SdfPath("/TestAttr"));
        UsdAttribute attr = attrPrim.CreateAttribute(
            TfToken("attr"), SdfValueTypeNames->Int);
        SdfPathVector conns;
        // No connections to start, GetConnections returns false when there
        // are no authored connections.
        TF_AXIOM(!attr.GetConnections(&conns));
        TF_AXIOM(conns.empty());
        // Add a connection, GetConnections returns true when there are authored
        // connections
        TF_AXIOM(attr.AddConnection(SdfPath("/TestAttr.dummy")));
        TF_AXIOM(attr.GetConnections(&conns));
        TF_AXIOM(conns == SdfPathVector({SdfPath("/TestAttr.dummy")}));

        // Add a relationship on a new prim to test targets.
        UsdPrim relPrim = stage->DefinePrim(SdfPath("/TestRel"));
        UsdRelationship rel = relPrim.CreateRelationship(TfToken("rel"));
        SdfPathVector targets;
        // No targets to start, GetTargets and GetForwardedTargets return false 
        // when there are no authored targets.
        TF_AXIOM(!rel.GetTargets(&targets));
        TF_AXIOM(targets.empty());
        TF_AXIOM(!rel.GetForwardedTargets(&targets));
        TF_AXIOM(targets.empty());

        // Add another relationship to test relationship forwarding
        UsdRelationship forwardingRel = 
            relPrim.CreateRelationship(TfToken("forwardingRel"));
        // Add a target to the previous relationship, GetTargets 
        // returns true and gets the targeted relationship. However 
        // GetForwardedTargets returns false because the only target is a 
        // relationship that has no authored targets.
        TF_AXIOM(forwardingRel.AddTarget(SdfPath("/TestRel.rel")));
        TF_AXIOM(forwardingRel.GetTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestRel.rel")}));
        TF_AXIOM(!forwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets.empty());

        // Add a target, GetTargets on the first relationship returns true when
        // there are authored targets.
        TF_AXIOM(rel.AddTarget(SdfPath("/TestAttr.dummy")));
        TF_AXIOM(rel.GetTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));
        TF_AXIOM(rel.GetForwardedTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));
        // GetForwardedTargets on the forwarding relationship also returns true 
        // because its targeted  relation now has targets.
        TF_AXIOM(forwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));

        // To test the effect of composition errors, add a new prim with a
        // reference to the prim we defined the prior relationships on. There
        // will be a composition error when building targets for the
        // relationship "rel" because the defined target path "/TestAttr.dummy"
        // can't be mapped across the reference from "/TestRef" to "/TestRel"
        UsdPrim refPrim = stage->DefinePrim(SdfPath("/TestRef"));
        refPrim.GetReferences().AddReference(stage->GetRootLayer()->GetIdentifier(),
                                             relPrim.GetPath());
        // "rel" on the referencing prim will have authored targets, but
        // GetTargets will return false because of the composition error.
        UsdRelationship refRel = refPrim.GetRelationship(TfToken("rel"));
        TF_AXIOM(refRel.HasAuthoredTargets());
        TF_AXIOM(!refRel.GetTargets(&targets));
        TF_AXIOM(targets.empty());
        // Add another valid target. Still returns false because of the other
        // composition errors.
        refRel.AddTarget(SdfPath("/TestAttr.dummy"));
        TF_AXIOM(refRel.HasAuthoredTargets());
        TF_AXIOM(!refRel.GetTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));

        // "forwardingRel" on the referencing prim will still return true for
        // GetTargets because there are no errors mapping the relationship
        // it targets across the reference.
        UsdRelationship refForwardingRel =
            refPrim.GetRelationship(TfToken("forwardingRel"));
        TF_AXIOM(refForwardingRel.HasAuthoredTargets());
        TF_AXIOM(refForwardingRel.GetTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestRef.rel")}));
        // However, GetForwardedTargets will return false because of the
        // target composition errors on "/TestRef.rel"
        TF_AXIOM(!refForwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));

        // Add another valid target directly to "forwardingRel" on the
        // referencing prim. GetForwardedTargets still returns false because
        // of the downstream composition errors, but it does still get any
        // valid forwarded targets it found along the way.
        TF_AXIOM(refForwardingRel.AddTarget(SdfPath("/TestAttr.attr")));
        TF_AXIOM(!refForwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.attr"),
                                           SdfPath("/TestAttr.dummy")}));

        // We do this part after the other test cases so we don't have to set
        // up the state again afterwards.
        // Clear the targets on the original relationship. GetTargets returns
        // false again because there are no authored targets.
        TF_AXIOM(rel.ClearTargets(false));
        TF_AXIOM(!rel.GetTargets(&targets));
        TF_AXIOM(targets.empty());
        // GetForwardedTargets on forwarding rel also returns false
        TF_AXIOM(!forwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets.empty());

        // Now explicitly set empty targets for the original relationship.
        // GetTargets returns true because there is an authored opinion even
        // though there are no targets.
        TF_AXIOM(rel.SetTargets({}));
        TF_AXIOM(rel.GetTargets(&targets));
        TF_AXIOM(targets.empty());
        // GetForwardedTargets on forwarding rel also returns true because the
        // targeted relationship has an explicitly authored opinion.
        TF_AXIOM(forwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets.empty());

        // Clear targets on the first rel again and add a non-relationship 
        // target to forwardingRel. Confirm that GetForwardingTargets returns
        // true because the forwardingRel is no longer "purely forwarding" even
        // even the forwarded relationship has no opinions
        TF_AXIOM(rel.ClearTargets(false));
        TF_AXIOM(forwardingRel.AddTarget(SdfPath("/TestAttr.dummy")));
        // rel has no target opinion
        TF_AXIOM(!rel.GetTargets(&targets));
        TF_AXIOM(targets.empty());
        // forwardRel has a relationship target and an attribute target.
        TF_AXIOM(forwardingRel.GetTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestRel.rel"),
                                           SdfPath("/TestAttr.dummy")}));
        // forwardingRel returns true for GetForwardedTargets.
        TF_AXIOM(forwardingRel.GetForwardedTargets(&targets));
        TF_AXIOM(targets == SdfPathVector({SdfPath("/TestAttr.dummy")}));
    }
}

}

int 
main(int argc, char** argv)
{
    TestTargetSpecs();
    TestGetTargetsAndConnections();
    return 0;
}
