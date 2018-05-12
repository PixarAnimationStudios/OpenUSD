#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import Usd, UsdSkel, Gf, Vt
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


def _XformsAreClose(a,b, threshold=1e-5):
    return all(Gf.IsClose(x,y,threshold) for x,y in zip(list(a),list(b)))


class TestUsdSkelSkeletonQuery(unittest.TestCase):


    def assertArrayIsClose(self, a, b, epsilon=1e-4):
        self.assertEqual(len(a), len(b))
        self.assertTrue(all(Gf.IsClose(ca,cb,epsilon)
                            for ca,cb in zip(a,b)))

    
    def test_SkeletonQuery(self):

        random.seed(0)
        numFrames = 10

        stage = Usd.Stage.CreateInMemory()

        skelRoot = UsdSkel.Root.Define(stage, "/SkelRoot")

        anim = UsdSkel.PackedJointAnimation.Define(stage, "/SkelRoot/Anim")
        skel = UsdSkel.Skeleton.Define(stage, "/SkelRoot/Skel")

        binding = UsdSkel.BindingAPI.Apply(skelRoot.GetPrim())
        binding.CreateSkeletonRel().SetTargets(
            [skel.GetPrim().GetPath()])
        binding.CreateAnimationSourceRel().SetTargets(
            [anim.GetPrim().GetPath()])

        skelOrder = Vt.TokenArray(["/A", "/A/B", "/A/B/C",
                                   "/D", "/D/E/F"])
        A,AB,ABC,D,DEF = list(xrange(len(skelOrder)))

        # Configure the skel.
        skel.GetJointsAttr().Set(skelOrder)
        restXforms = [_RandomXf() for _ in skelOrder]
        skel.GetRestTransformsAttr().Set(restXforms)

        # Configure root xforms.
        rootXforms = [_RandomXf() for _ in xrange(numFrames)]
        rootXfAttr = skelRoot.MakeMatrixXform()
        for frame,xf in enumerate(rootXforms):
            rootXfAttr.Set(xf, frame)

        # Configure anim.
        # Leave last element off of anim (tests remapping)
        animOrder = skelOrder[:-1]
        anim.GetJointsAttr().Set(animOrder)

        animRootXforms = [_RandomXf() for _ in xrange(numFrames)]
        animRootXfAttr = anim.MakeMatrixXform()
        for frame,xf in enumerate(animRootXforms):
            animRootXfAttr.Set(xf, frame)

        # Apply joint animations.
        animXforms = {i:[_RandomXf() for _ in xrange(len(animOrder))]
                      for i in xrange(numFrames)}
        for frame,xforms in animXforms.items():
            t,r,s = UsdSkel.DecomposeTransforms(Vt.Matrix4dArray(xforms))
            anim.GetTranslationsAttr().Set(t, frame)
            anim.GetRotationsAttr().Set(r, frame)
            anim.GetScalesAttr().Set(s, frame)

        #
        # Constuct query and start testing
        # 

        skelCache = UsdSkel.Cache()
        skelCache.Populate(skelRoot)

        query = skelCache.GetSkelQuery(skelRoot.GetPrim())
        self.assertTrue(query)

        # Validate joint rest xform computations.

        xforms = query.ComputeJointLocalTransforms(0, atRest=True)
        expectedXforms = restXforms
        self.assertArrayIsClose(xforms, expectedXforms)

        # Validate joint xforms computations.
        for t in xrange(numFrames):

            xforms = animXforms[t]

            # Joint local xforms.
            expectedXforms = list(xforms)+[restXforms[-1]]
            computedXforms = query.ComputeJointLocalTransforms(t)
            self.assertArrayIsClose(computedXforms, expectedXforms)

            # Joint skel-space xforms.
            expectedXforms = Vt.Matrix4dArray(
                [
                    xforms[A],
                    xforms[AB]*xforms[A],
                    xforms[ABC]*xforms[AB]*xforms[A],
                    xforms[D],
                    restXforms[DEF]*xforms[D]
                ])
            computedXforms = query.ComputeJointSkelTransforms(t)
            self.assertArrayIsClose(computedXforms, expectedXforms)

            # Joint local rest xforms.
            expectedXforms = restXforms
            computedXforms = query.ComputeJointLocalTransforms(t,atRest=True)
            self.assertArrayIsClose(computedXforms, expectedXforms)

            # Joint skel-space xforms.
            expectedXforms = Vt.Matrix4dArray(
                [
                    restXforms[A],
                    restXforms[AB]*restXforms[A],
                    restXforms[ABC]*restXforms[AB]*restXforms[A],
                    restXforms[D],
                    restXforms[DEF]*restXforms[D]
                ])
            computedXforms = query.ComputeJointSkelTransforms(0, atRest=True)
            self.assertArrayIsClose(computedXforms, expectedXforms)


        # Validate anim root transforms.
        for frame,expectedXf in enumerate(animRootXforms):
            animXf = query.ComputeAnimTransform(frame)
            self.assertTrue(Gf.IsClose(animXf, expectedXf, 1e-5))

        # Validate skel instance transforms.
        for frame,expectedXf in enumerate(rootXforms):
            rootXf = skelRoot.GetLocalTransformation(frame)
            self.assertTrue(Gf.IsClose(rootXf, expectedXf, 1e-5))

        #
        # Test that inactivate animation sources have no effect.
        #
        
        anim.GetPrim().SetActive(False)

        skelCache.Clear()
        skelCache.Populate(skelRoot)

        query = skelCache.GetSkelQuery(skelRoot.GetPrim())

        expectedXforms = restXforms
        computedXforms = query.ComputeJointLocalTransforms(10)
        self.assertArrayIsClose(computedXforms, expectedXforms)

        anim.GetPrim().SetActive(True)

        #
        # Test that blocking transform components of the animation source
        # causes the animation source to be ignored.
        #
        
        anim.GetTranslationsAttr().Block()

        skelCache.Clear()
        skelCache.Populate(skelRoot)

        query = skelCache.GetSkelQuery(skelRoot.GetPrim())

        expectedXforms = restXforms
        computedXforms = query.ComputeJointLocalTransforms(5)
        self.assertArrayIsClose(computedXforms, expectedXforms)


if __name__ == "__main__":
    unittest.main()
