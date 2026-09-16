#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import unittest
from pxr import Gf, Usd, Sdf, UsdGeom, UsdPhysics


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

    def test_get_local_pose_unset_is_world_anchor(self):
        self.setup_scene()

        body0 = UsdGeom.Xform.Define(self.stage, "/body0")
        UsdPhysics.RigidBodyAPI.Apply(body0.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([body0.GetPrim().GetPath()])

        # body1 is unset: the joint anchors to the world. This is a well defined
        # pose (the authored local pose), not a failure. GetLocalPose1
        # succeeds and returns the authored values (identity here).
        pose1 = joint.GetLocalPose1()
        self.assertTrue(pose1)
        position1, orientation1 = pose1
        self.assertEqual(position1, Gf.Vec3f(0.0, 0.0, 0.0))
        self.assertEqual(orientation1, Gf.Quatf(1.0))

        # An unset side with an authored world-anchor pose returns that pose:
        # both position and orientation pass through unchanged.
        authored_rot = Gf.Quatf(0.70710678, Gf.Vec3f(0.70710678, 0.0, 0.0))
        joint.CreateLocalPos1Attr().Set(Gf.Vec3f(7.0, 8.0, 9.0))
        joint.CreateLocalRot1Attr().Set(authored_rot)
        position1, orientation1 = joint.GetLocalPose1()
        self.assertEqual(position1, Gf.Vec3f(7.0, 8.0, 9.0))
        self.assertTrue(Gf.IsClose(
            Gf.Vec4f(orientation1.real, *orientation1.imaginary),
            Gf.Vec4f(authored_rot.real, *authored_rot.imaginary), 1e-6))

        # A non-unit authored orientation is normalized on the way out.
        joint.CreateLocalRot1Attr().Set(Gf.Quatf(2.0, Gf.Vec3f(0.0, 0.0, 0.0)))
        _, orientation1 = joint.GetLocalPose1()
        self.assertEqual(orientation1, Gf.Quatf(1.0))

        # body0 targets the body directly, so the authored pose passes through
        # unchanged (no scale on the body frame).
        joint.CreateLocalPos0Attr().Set(Gf.Vec3f(1.0, 2.0, 3.0))
        pose0 = joint.GetLocalPose0()
        self.assertTrue(pose0)
        position0, orientation0 = pose0
        self.assertEqual(position0, Gf.Vec3f(1.0, 2.0, 3.0))
        self.assertEqual(orientation0, Gf.Quatf(1.0))

    def test_get_local_pose_dangling_target_fails(self):
        self.setup_scene()

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        # A relationship target that does not resolve to a prim on the stage is
        # malformed authoring. GetLocalPose reports failure, which the Python
        # binding surfaces as an empty tuple. (In C++ the authored pose is
        # still written to the out parameters so a caller can pass it through;
        # the Python binding returns nothing on failure by convention.)
        joint.CreateBody0Rel().SetTargets(["/does/not/exist"])
        joint.CreateLocalPos0Attr().Set(Gf.Vec3f(7.0, 8.0, 9.0))
        self.assertEqual(joint.GetLocalPose0(), ())

    def test_get_local_pose_bakes_scale(self):
        self.setup_scene()

        # A scaled body with a collider under it. The joint's rel points at the
        # collider (not the body), so the authored local pose is rebased into
        # the body frame and the body's scale is baked into the translation,
        # because physics has no notion of scale.
        body = UsdGeom.Xform.Define(self.stage, "/body")
        body.AddScaleOp().Set(Gf.Vec3f(2.0, 2.0, 2.0))
        UsdPhysics.RigidBodyAPI.Apply(body.GetPrim())
        collider = UsdGeom.Cube.Define(self.stage, "/body/collider")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([collider.GetPrim().GetPath()])
        joint.CreateLocalPos0Attr().Set(Gf.Vec3f(1.0, 0.0, 0.0))

        self.assertEqual(joint.GetBody0().GetPath(), body.GetPrim().GetPath())

        pose0 = joint.GetLocalPose0()
        self.assertTrue(pose0)
        position, _ = pose0
        # The collider sits at the body origin; the authored (1,0,0) is in the
        # collider frame, which coincides with the body frame here, and the
        # body's 2x scale is folded into the translation -> (2,0,0).
        self.assertTrue(Gf.IsClose(position, Gf.Vec3f(2.0, 0.0, 0.0), 1e-5))

    def test_get_local_pose_resolution_and_scale(self):
        """GetLocalPose resolves the attachment frame and bakes its scale: a
        collider resolves to its owning body; a target with no enclosing body is
        anchored to directly.
        """
        stage = Usd.Stage.CreateInMemory()
        UsdPhysics.Scene.Define(stage, "/scene")
        UsdGeom.SetStageUpAxis(stage, "Z")
        UsdGeom.SetStageMetersPerUnit(stage, 1.0)

        # body0 -> a collider at the origin of a scaled body: resolves to the
        # body, authored pose is already in the body frame, only the body's 2x
        # scale is baked -> (2,0,0).
        body = UsdGeom.Xform.Define(stage, "/body")
        body.AddTranslateOp().Set(Gf.Vec3d(0.0, 3.0, 0.0))
        body.AddScaleOp().Set(Gf.Vec3f(2.0, 2.0, 2.0))
        UsdPhysics.RigidBodyAPI.Apply(body.GetPrim())
        collider = UsdGeom.Cube.Define(stage, "/body/collider")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())

        # body1 -> a non-body anchor root. With no enclosing body the joint is
        # anchored directly to the anchor prim: the authored pose is already in
        # its frame, so only the anchor's 3x scale is baked, giving (3,0,0). The
        # anchor's translation is not applied (we do not rebase into world).
        anchor = UsdGeom.Xform.Define(stage, "/anchor")
        anchor.AddTranslateOp().Set(Gf.Vec3d(10.0, 0.0, 0.0))
        anchor.AddScaleOp().Set(Gf.Vec3f(3.0, 3.0, 3.0))

        joint = UsdPhysics.FixedJoint.Define(stage, "/joint")
        joint.CreateBody0Rel().SetTargets([collider.GetPrim().GetPath()])
        joint.CreateBody1Rel().SetTargets([anchor.GetPrim().GetPath()])
        joint.CreateLocalPos0Attr().Set(Gf.Vec3f(1.0, 0.0, 0.0))
        joint.CreateLocalPos1Attr().Set(Gf.Vec3f(1.0, 0.0, 0.0))

        # body0 resolves to a real body; body1 has no enclosing body.
        self.assertEqual(joint.GetBody0().GetPath(), body.GetPrim().GetPath())
        self.assertFalse(joint.GetBody1())

        pos0, _ = joint.GetLocalPose0()
        pos1, _ = joint.GetLocalPose1()
        # body0 goes through the body-frame rebasing path; allow for float
        # round-off. body1 is a direct scale-only bake and is exact.
        self.assertTrue(Gf.IsClose(pos0, Gf.Vec3f(2.0, 0.0, 0.0), 1e-5))
        self.assertEqual(pos1, Gf.Vec3f(3.0, 0.0, 0.0))

        # An optional UsdGeomXformCache does not change the result on an
        # unchanged stage.
        cache = UsdGeom.XformCache()
        cpos0, _ = joint.GetLocalPose0(cache)
        cpos1, _ = joint.GetLocalPose1(cache)
        self.assertEqual(cpos0, pos0)
        self.assertEqual(cpos1, pos1)

    def test_get_local_pose_uses_xform_cache(self):
        """A supplied UsdGeomXformCache is actually consulted for the ancestor
        transforms: it memoizes them, so an edit to an ancestor after the cache
        is populated is not reflected until the cache is cleared, whereas an
        uncached call sees the edit immediately.
        """
        self.setup_scene()

        # A deep hierarchy so the body's world transform depends on several
        # ancestors. The joint targets a collider under the body, so resolving
        # the pose requires the body's world transform (the cached quantity).
        root = UsdGeom.Xform.Define(self.stage, "/root")
        root_scale = root.AddScaleOp()
        root_scale.Set(Gf.Vec3f(1.0, 1.0, 1.0))
        mid = UsdGeom.Xform.Define(self.stage, "/root/mid")
        mid.AddTranslateOp().Set(Gf.Vec3d(0.0, 1.0, 0.0))
        body = UsdGeom.Xform.Define(self.stage, "/root/mid/body")
        body.AddScaleOp().Set(Gf.Vec3f(2.0, 2.0, 2.0))
        UsdPhysics.RigidBodyAPI.Apply(body.GetPrim())
        collider = UsdGeom.Cube.Define(self.stage, "/root/mid/body/collider")
        UsdPhysics.CollisionAPI.Apply(collider.GetPrim())

        joint = UsdPhysics.Joint.Define(self.stage, "/joint")
        joint.CreateBody0Rel().SetTargets([collider.GetPrim().GetPath()])
        joint.CreateLocalPos0Attr().Set(Gf.Vec3f(1.0, 0.0, 0.0))

        # Body world scale is 1 * 2 = 2, so the baked position is (2,0,0).
        cache = UsdGeom.XformCache()
        cached_before, _ = joint.GetLocalPose0(cache)
        self.assertEqual(cached_before, Gf.Vec3f(2.0, 0.0, 0.0))

        # Scale an ancestor: the body's world scale is now 5 * 2 = 10.
        root_scale.Set(Gf.Vec3f(5.0, 5.0, 5.0))

        # The same (now stale) cache still returns the pre-edit transform,
        # proving the cache was used rather than recomputed.
        cached_after, _ = joint.GetLocalPose0(cache)
        self.assertEqual(cached_after, Gf.Vec3f(2.0, 0.0, 0.0))

        # An uncached call recomputes and reflects the edit.
        fresh_after, _ = joint.GetLocalPose0()
        self.assertEqual(fresh_after, Gf.Vec3f(10.0, 0.0, 0.0))

        # After clearing, the cache recomputes and agrees with the uncached
        # result.
        cache.Clear()
        cleared, _ = joint.GetLocalPose0(cache)
        self.assertEqual(cleared, Gf.Vec3f(10.0, 0.0, 0.0))


if __name__ == "__main__":
    unittest.main()
