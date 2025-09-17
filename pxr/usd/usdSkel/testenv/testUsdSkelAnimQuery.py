#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdSkel, Gf, Sdf, Tf, Vt
import unittest, random


def _RandomXf():
    return Gf.Matrix4d(Gf.Rotation(Gf.Vec3d(1,0,0),
                                   random.random()*360)*
                       Gf.Rotation(Gf.Vec3d(0,1,0),
                                   random.random()*360)*
                       Gf.Rotation(Gf.Vec3d(0,0,1),
                                   random.random()*360),
                       Gf.Vec3d((random.random()-0.5)*10,
                                (random.random()-0.5)*10,
                                (random.random()-0.5)*10))


class TestUsdSkelAnimQuery(unittest.TestCase):


    def assertArrayIsClose(self, a, b, epsilon=1e-5):
        self.assertEqual(len(a), len(b))
        self.assertTrue(all(Gf.IsClose(ca,cb,epsilon)
                            for ca,cb in zip(a,b)))

        
    def test_SkelAnimation(self):
        """
        Tests anim query implementation for SkelAnimation.
        """
        self._TestSkelAnimation(UsdSkel.Animation)


    def _TestSkelAnimation(self, animSchema):

        numFrames = 10
        random.seed(0)

        stage = Usd.Stage.CreateInMemory()

        anim = animSchema.Define(stage, "/Anim")

        joints = Vt.TokenArray(["/A", "/B", "/C"])
        blendshapes = Vt.TokenArray(["shapeA", "shapeB", "shapeC"])

        anim.GetJointsAttr().Set(joints)
        anim.GetBlendShapesAttr().Set(blendshapes)

        xformsPerFrame = [[_RandomXf() for _ in range(len(joints))]
                          for _ in range(numFrames)]

        for frame,xforms in enumerate(xformsPerFrame):
            anim.SetTransforms(Vt.Matrix4dArray(xforms), frame)

        weightsPerFrame = [[random.random() for _ in range(len(blendshapes))]
                           for _ in range(numFrames)]
        
        for frame,weights in enumerate(weightsPerFrame):
            anim.GetBlendShapeWeightsAttr().Set(Vt.FloatArray(weights), frame)

        # Now try reading that all back via computations...

        cache = UsdSkel.Cache()

        query = cache.GetAnimQuery(anim)

        self.assertEqual(query.GetPrim(), anim.GetPrim())
        self.assertEqual(query.GetJointOrder(), joints)
        self.assertEqual(query.GetBlendShapeOrder(), blendshapes)
        self.assertTrue(query.JointTransformsMightBeTimeVarying())
        self.assertEqual(query.GetJointTransformTimeSamples(),
                         list(range(numFrames)))

        for frame,xforms in enumerate(xformsPerFrame):

            computedXforms = query.ComputeJointLocalTransforms(frame)
            self.assertArrayIsClose(computedXforms, xforms)

            computedXforms = anim.GetTransforms(frame)
            self.assertArrayIsClose(computedXforms, xforms)

        for frame,weights in enumerate(weightsPerFrame):
            computedWeights = query.ComputeBlendShapeWeights(frame)
            self.assertArrayIsClose(computedWeights, weights)


if __name__ == "__main__":
    unittest.main()
