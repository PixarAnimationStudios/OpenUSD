#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import unittest
from pxr import Usd, Sdf, UsdGeom, UsdPhysics


class TestUsdPhysicsJoint(unittest.TestCase):

    def setup_scene(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)
        self.stage = stage
        UsdGeom.SetStageUpAxis(stage, "Z")
        UsdGeom.SetStageMetersPerUnit(stage, 1.0)

    def test_get_body_resolves_to_owning_body(self):
        self.setup_scene()

        # A rigid body with a collider nested below it. The joint's body rel
        # points at the collider, not the body; GetBody0 walks the ancestors
        # and reports the owning rigid body.
        body0 = UsdGeom.Xform.Define(self.stage, "/body0")
        UsdPhysics.RigidBodyAPI.Apply(body0.GetPrim())
        collider0 = UsdGeom.Cube.Define(self.stage, "/body0/geo/collider")
        UsdPhysics.CollisionAPI.Apply(collider0.GetPrim())

        body1 = UsdGeom.Xform.Define(self.stage, "/body1")
        UsdPhysics.RigidBodyAPI.Apply(body1.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([collider0.GetPrim().GetPath()])
        joint.CreateBody1Rel().SetTargets([body1.GetPrim().GetPath()])

        self.assertEqual(joint.GetBody0().GetPath(),
                         body0.GetPrim().GetPath())
        self.assertEqual(joint.GetBody1().GetPath(),
                         body1.GetPrim().GetPath())

    def test_get_body_skips_disabled_body(self):
        self.setup_scene()

        # A collider below a DISABLED nested body. The disabled body takes no
        # part in simulation, so the joint resolves to the nearest enabled body
        # above it.
        outer = UsdGeom.Xform.Define(self.stage, "/outer")
        UsdPhysics.RigidBodyAPI.Apply(outer.GetPrim())
        disabledBody = UsdGeom.Xform.Define(self.stage, "/outer/disabledBody")
        disabledBodyAPI = UsdPhysics.RigidBodyAPI.Apply(disabledBody.GetPrim())
        disabledBodyAPI.GetRigidBodyEnabledAttr().Set(False)
        collider = UsdGeom.Cube.Define(
            self.stage, "/outer/disabledBody/collider")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([collider.GetPrim().GetPath()])
        self.assertEqual(joint.GetBody0().GetPath(),
                         outer.GetPrim().GetPath())

    def test_get_body_unset_or_no_enabled_body(self):
        self.setup_scene()

        body0 = UsdGeom.Xform.Define(self.stage, "/body0")
        UsdPhysics.RigidBodyAPI.Apply(body0.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([body0.GetPrim().GetPath()])

        # body1 has no target set: a joint with an unset body attaches to the
        # simulation world, so GetBody1 returns an invalid prim.
        self.assertFalse(joint.GetBody1())

        # A body rel pointing at a subtree with only a disabled body has no
        # enabled owner and resolves to an invalid prim.
        disabled = UsdGeom.Xform.Define(self.stage, "/disabled")
        disabledAPI = UsdPhysics.RigidBodyAPI.Apply(disabled.GetPrim())
        disabledAPI.GetRigidBodyEnabledAttr().Set(False)
        collider = UsdGeom.Cube.Define(self.stage, "/disabled/collider")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())
        joint.GetBody1Rel().SetTargets([collider.GetPrim().GetPath()])
        self.assertFalse(joint.GetBody1())


if __name__ == "__main__":
    unittest.main()
