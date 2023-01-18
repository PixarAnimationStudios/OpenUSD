#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

import sys, os, unittest
from pxr import Tf, Usd, Sdf, UsdGeom, UsdShade, Gf, UsdPhysics

class TestUsdPhysicsCollisionGroupAPI(unittest.TestCase):
    def validate_table_symmetry(self, table):
        for iA, a in enumerate(table.GetGroups()):
            for iB, b in enumerate(table.GetGroups()):
                self.assertEqual(table.IsCollisionEnabled(iA, iB), 
                        table.IsCollisionEnabled(iB, iA))
                self.assertEqual(table.IsCollisionEnabled(a, b), 
                        table.IsCollisionEnabled(b, a))
                self.assertEqual(table.IsCollisionEnabled(a, b), 
                        table.IsCollisionEnabled(iA, iB))

    def test_collision_group_table(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        a = UsdPhysics.CollisionGroup.Define(stage, "/a")
        b = UsdPhysics.CollisionGroup.Define(stage, "/b")
        c = UsdPhysics.CollisionGroup.Define(stage, "/c")

        b.CreateFilteredGroupsRel().AddTarget(c.GetPath())
        c.CreateFilteredGroupsRel().AddTarget(c.GetPath())

        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)

        # Check the results contain all the groups:
        self.assertTrue(len(table.GetGroups()) == 3)
        self.assertTrue(a.GetPrim().GetPath() in table.GetGroups())
        self.assertTrue(b.GetPrim().GetPath() in table.GetGroups())
        self.assertTrue(c.GetPrim().GetPath() in table.GetGroups())

        # A should collide with everything
        # B should only collide with A and B
        # C should only collide with A
        self.assertTrue(table.IsCollisionEnabled(a, a));
        self.assertTrue(table.IsCollisionEnabled(a, b));
        self.assertTrue(table.IsCollisionEnabled(a, c));
        self.assertTrue(table.IsCollisionEnabled(b, b));
        self.assertFalse(table.IsCollisionEnabled(b, c));
        self.assertFalse(table.IsCollisionEnabled(c, c));
        self.validate_table_symmetry(table)


    def test_collision_group_inversion(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        a = UsdPhysics.CollisionGroup.Define(stage, "/a")
        b = UsdPhysics.CollisionGroup.Define(stage, "/b")
        c = UsdPhysics.CollisionGroup.Define(stage, "/c")

        a.CreateFilteredGroupsRel().AddTarget(c.GetPath())
        a.CreateInvertFilteredGroupsAttr().Set(True)

        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)

        # A should collide with only C
        # B should collide with only B and C
        # C should collide with only B and C
        self.assertFalse(table.IsCollisionEnabled(a, a));
        self.assertFalse(table.IsCollisionEnabled(a, b));
        self.assertTrue(table.IsCollisionEnabled(a, c));
        self.assertTrue(table.IsCollisionEnabled(b, b));
        self.assertTrue(table.IsCollisionEnabled(b, c));
        self.assertTrue(table.IsCollisionEnabled(c, c));
        self.validate_table_symmetry(table)

        # Explicitly test the inversion scenario which may "re-enable" a
        # collision filter pair that has been disabled (refer docs on why care 
        # should be taken to avoid such scenarios)
        allOthers = UsdPhysics.CollisionGroup.Define(stage, '/allOthers')

        # - grpX is set to ONLY collide with grpXCollider by setting an inversion
        grpXCollider = UsdPhysics.CollisionGroup.Define(stage, '/grpXCollider')
        grpX = UsdPhysics.CollisionGroup.Define(stage, '/grpX')
        grpX.CreateFilteredGroupsRel().AddTarget(grpXCollider.GetPath())
        grpX.CreateInvertFilteredGroupsAttr().Set(True)
        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)
        self.assertTrue(table.IsCollisionEnabled(grpX, grpXCollider))
        self.assertFalse(table.IsCollisionEnabled(grpX, allOthers))

        # - grpX is added to a new merge group "mergetest"
        grpX.CreateMergeGroupNameAttr().Set("mergeTest")

        # - grpA now creates a filter to disable its collision with grpXCollider
        grpA = UsdPhysics.CollisionGroup.Define(stage, '/grpA')
        grpA.CreateFilteredGroupsRel().AddTarget(grpXCollider.GetPath())
        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)
        self.assertFalse(table.IsCollisionEnabled(grpA, grpXCollider))
        # - above doesn't affect any of grpX's collision pairs
        self.assertTrue(table.IsCollisionEnabled(grpX, grpXCollider))
        self.assertFalse(table.IsCollisionEnabled(grpX, allOthers))

        # - grpA is now added to same "mergetest" merge group (care was not
        # taken in doing so and this disables all collision pairs!!)
        grpA.CreateMergeGroupNameAttr().Set("mergeTest")
        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)
        self.assertFalse(table.IsCollisionEnabled(grpX, grpXCollider))
        self.assertFalse(table.IsCollisionEnabled(grpX, allOthers))
        self.assertFalse(table.IsCollisionEnabled(grpA, grpXCollider))
        self.assertFalse(table.IsCollisionEnabled(grpA, allOthers))

    def test_collision_group_simple_merging(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        a = UsdPhysics.CollisionGroup.Define(stage, "/a")
        b = UsdPhysics.CollisionGroup.Define(stage, "/b")
        c = UsdPhysics.CollisionGroup.Define(stage, "/c")

        a.CreateFilteredGroupsRel().AddTarget(c.GetPath())
        # Assign A and B to the same merge group:
        a.CreateMergeGroupNameAttr().Set("mergeTest")
        b.CreateMergeGroupNameAttr().Set("mergeTest")

        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)

        # A should collide with only A and B
        # B should collide with only A and B
        # C should collide with only C
        self.assertTrue(table.IsCollisionEnabled(a, a));
        self.assertTrue(table.IsCollisionEnabled(a, b));
        self.assertFalse(table.IsCollisionEnabled(a, c));
        self.assertTrue(table.IsCollisionEnabled(b, b));
        self.assertFalse(table.IsCollisionEnabled(b, c));
        self.assertTrue(table.IsCollisionEnabled(c, c));
        self.validate_table_symmetry(table)

    def test_collision_group_complex_merging(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        a = UsdPhysics.CollisionGroup.Define(stage, "/a")
        b = UsdPhysics.CollisionGroup.Define(stage, "/b")
        c = UsdPhysics.CollisionGroup.Define(stage, "/c")
        d = UsdPhysics.CollisionGroup.Define(stage, "/d")

        a.CreateFilteredGroupsRel().AddTarget(c.GetPath())
        # Assign A and B to the same merge group:
        a.CreateMergeGroupNameAttr().Set("mergeAB")
        b.CreateMergeGroupNameAttr().Set("mergeAB")
        # Assign C and D to the same merge group:
        c.CreateMergeGroupNameAttr().Set("mergeCD")
        d.CreateMergeGroupNameAttr().Set("mergeCD")

        table = UsdPhysics.CollisionGroup.ComputeCollisionGroupTable(stage)

        # A should collide with only A and B
        # B should collide with only A and B
        # C should collide with only C and D
        # D should collide with only C and D
        self.assertTrue(table.IsCollisionEnabled(a, a));
        self.assertTrue(table.IsCollisionEnabled(a, b));
        self.assertFalse(table.IsCollisionEnabled(a, c));
        self.assertFalse(table.IsCollisionEnabled(a, d));

        self.assertTrue(table.IsCollisionEnabled(b, b));
        self.assertFalse(table.IsCollisionEnabled(b, c));
        self.assertFalse(table.IsCollisionEnabled(b, d));

        self.assertTrue(table.IsCollisionEnabled(c, c));
        self.assertTrue(table.IsCollisionEnabled(c, d));
        self.assertTrue(table.IsCollisionEnabled(d, d));
        self.validate_table_symmetry(table)

if __name__ == "__main__":
    unittest.main()
