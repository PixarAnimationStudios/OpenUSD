#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Vt, Gf, Usd, UsdGeom, UsdLod

import math
import unittest

LOD0_CENTER = Gf.Vec3d(1.0, 0.0, 0.0)
TRANSLATE_X = 3.0

class TestUsdLodCoreApis(unittest.TestCase):


    # Run before each test.
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.world = UsdGeom.Xform.Define(self.stage, "/World")

        self.lodRoot = UsdGeom.Xform.Define(self.stage, "/World/lodRoot")
        self.lodRootPrim = self.lodRoot.GetPrim()
        self.lodRoot.AddTranslateXOp().Set(TRANSLATE_X)

        self.lodItems = [
            UsdGeom.Sphere.Define(self.stage, "/World/lodRoot/lodItem_0"),
            UsdGeom.Cylinder.Define(self.stage, "/World/lodRoot/lodItem_1"),
            UsdGeom.Cube.Define(self.stage, "/World/lodRoot/lodItem_2"),
        ]

        visValues = ["invisible", "inherited", "invisible"];
        for i, lodItem in enumerate(self.lodItems):
            lodItem.CreateVisibilityAttr(visValues[i])

        self.heuristicsScope = UsdGeom.Scope.Define(self.stage,
                                                    "/World/heuristics")

        # Create a couple of heuristics
        self.distance_0 = UsdLod.DistanceHeuristic.Define(
            self.stage, "/World/heuristics/distance_0")
        self.distance_1 = UsdLod.DistanceHeuristic.Define(
            self.stage, "/World/heuristics/distance_1")

        # Setup one for the proprietary "SomeRenderer" domain with a center, no
        # bounds relationship, and threshold values at 5 and 20 units.
        domain0 = "SomeRenderer"
        self.distance_0.CreateLodDomainAttr(domain0)
        self.distance_0.CreateCenterAttr(LOD0_CENTER)
        self.distance_0.CreateThresholdsAttr(Vt.FloatArray([5.0, 20.0]))

        # Setup one for the generic "imaging" domain with a bounds relationship
        # targeting the cube, a center value (that should be ignored due to the
        # bounds relationship), and blendThresholds for blending.
        domain1 = "imaging"
        self.distance_1.CreateLodDomainAttr(domain1)
        self.distance_1.CreateCenterAttr(Gf.Vec3f(999.0, 0.0, 0.0))
        boundsRel = self.distance_1.CreateBoundingVolumeRel()
        boundsRel.AddTarget(self.lodItems[2].GetPath())
        self.distance_1.CreateThresholdsAttr(Vt.FloatArray([5.0, 20.0]));
        self.distance_1.CreateBlendThresholdsAttr(Vt.FloatArray([8.0, 25.0]));

    def test_DistanceHeuristic(self):

        def _expectedDistance0(z):
            distance = math.hypot(z, TRANSLATE_X + LOD0_CENTER[0])
            return distance

        def _expectedDistance1(z):
            # Distance is computed by the heuristic by transforming the viewpoint
            # into object space, finding the closest point on the extent volume,
            # then transforming both back to world coordinates and measuring the
            # distance.

            # These values need to match the contents of the stage created in
            # the setUp method.
            extent = Gf.Range3d(Gf.Vec3d(-1), Gf.Vec3d(1))
            xform = Gf.Matrix4d().SetTranslate(Gf.Vec3d(TRANSLATE_X, 0.0, 0.0))

            viewpoint = Gf.Vec3d(0.0, 0.0, z)

            # Transform the viewpoint into object space
            invViewpoint = xform.GetInverse().Transform(viewpoint)

            # Get the extent bounds
            extMin = extent.GetMin()
            extMax = extent.GetMax()

            # The closest point in the extent is simply the viewpoint clamped
            # to the bounds of the extent.
            closest = Gf.Vec3d(Gf.Clamp(invViewpoint[0], extMin[0], extMax[0]),
                               Gf.Clamp(invViewpoint[1], extMin[1], extMax[1]),
                               Gf.Clamp(invViewpoint[2], extMin[2], extMax[2]))

            worldClosest = xform.Transform(closest)

            distance = (worldClosest - viewpoint).GetLength()
            return distance

        def _expectedLOD0(z, thresholds):
            distance = _expectedDistance0(z)
            for i, threshold in enumerate(thresholds):
                if distance < threshold:
                    return i
            return len(thresholds)

        def _expectedLOD1(z, thresholds, blendThresholds):
            distance = _expectedDistance1(z)
            for i, minDist in enumerate(thresholds):
                maxDist = blendThresholds[i]

                if distance < minDist:
                    return i
                if distance < maxDist:
                    return i + (distance - minDist) / (maxDist - minDist)

            return len(thresholds)


        cache = UsdGeom.XformCache()
        rootToWorld = cache.GetLocalToWorldTransform(self.lodRootPrim)

        # Test the UsdLodDistanceHeuristicQuery
        distQuery0 = self.distance_0.CreateDistanceHeuristicQuery()
        self.assertIsInstance(
            distQuery0, UsdLod.HeuristicQuery,
            msg=(f"Type returned from CreateDistanceHeuristicQuery"
                 f" ({type(distQuery0).__name__}) is not a"
                 f" UsdLod.HeuristicQuery"))
        self.assertIsInstance(
            distQuery0, UsdLod.DistanceHeuristicQuery,
            msg=(f"Type returned from CreateDistanceHeuristicQuery"
                 f" ({type(distQuery0).__name__}) is not a"
                 f" UsdLod.DistanceHeuristicQuery"))
       
        distQuery1 = self.distance_1.CreateDistanceHeuristicQuery()
        self.assertIsInstance(
            distQuery1, UsdLod.HeuristicQuery,
            msg=(f"Type returned from CreateDistanceHeuristicQuery"
                 f" ({type(distQuery1).__name__}) is not a"
                 f" UsdLod.HeuristicQuery"))
        self.assertIsInstance(
            distQuery1, UsdLod.DistanceHeuristicQuery,
            msg=(f"Type returned from CreateDistanceHeuristicQuery"
                 f" ({type(distQuery1).__name__}) is not a"
                 f" UsdLod.DistanceHeuristicQuery"))

        for z in range(30):
            print(f"z = {z}")
            viewpoint = Gf.Vec3d(0.0, 0.0, z)

            expDist0 = _expectedDistance0(z)
            expLod0  = _expectedLOD0(z, [5.0, 20.0])
            
            expDist1 = _expectedDistance1(z)
            expLod1  = _expectedLOD1(z, [5.0, 20.0], [8.0, 25.0])

            self._DoOneDistanceTest("distance_0", self.distance_0,
                                    viewpoint, rootToWorld, expDist0, expLod0)

            self._DoOneDistanceTest("distance_0 query", distQuery0,
                                    viewpoint, rootToWorld, expDist0, expLod0)
            
            self._DoOneDistanceTest("distance_1", self.distance_1,
                                    viewpoint, rootToWorld, expDist1, expLod1)

            self._DoOneDistanceTest("distance_1 query", distQuery1,
                                    viewpoint, rootToWorld, expDist1, expLod1)

    def _DoOneDistanceTest(self, title, queryable, viewpoint, transform,
                           expectedDist, expectedLod):
        """
        Verify that we can compute distances and LODs with various call
        signatures and with and without hysteresis.
        """
        # There are 5 distinct calls with variations to test in each of the
        # heuristic and the heuristicQuery that need to be tested.
        #   a) computeDistance from viewpoint and transform
        #      a0 = distance
        #      a1 = distance + 0.25
        #      a2 = distance + 0.50
        #   b) computeDistance from viewpoint and transform with hysteresis
        #      b0 = no hysteresis, returns a0
        #      b1 = hysteresis returns distance a1
        #      b2 = hysteresis exceded returns distance a0
        #   c) computeLOD from distance
        #      c0 = LOD for a0
        #      c1 = LOD for a1
        #   d) computeLOD from viewpoint and transform
        #   e) computeLOD from viewpoint and transform with hysteresis
        #      e0 = (c0, a0)
        #      e1 = (c1, a1)
        #      e2 = (c0, a0)
        #
        hyst = 0.3
        
        a0 = queryable.ComputeDistance(viewpoint, transform)
        a1 = a0 + 0.25
        a2 = a0 + 0.50

        b0 = queryable.ComputeDistance(viewpoint, transform,
                                       prevDistance=a1,
                                       hysteresis=0.0)
        b1 = queryable.ComputeDistance(viewpoint, transform,
                                       prevDistance=a1,
                                       hysteresis=hyst)
        b2 = queryable.ComputeDistance(viewpoint, transform,
                                       prevDistance=a2,
                                       hysteresis=hyst)
        c0 = queryable.ComputeLOD(b0)
        c1 = queryable.ComputeLOD(b1)
        c2 = queryable.ComputeLOD(b2)

        d = queryable.ComputeLOD(viewpoint, transform)

        e0 = queryable.ComputeLOD(viewpoint, transform,
                                  prevDistance=a1,
                                  hysteresis=0.0)
        e1 = queryable.ComputeLOD(viewpoint, transform,
                                  prevDistance=a1,
                                  hysteresis=hyst)
        e2 = queryable.ComputeLOD(viewpoint, transform,
                                  prevDistance=a2,
                                  hysteresis=hyst)

        # Note that vertices in USD are 32-bit floats but all floating point
        # values in Python are 64-bit doubles. This leads to some slightly
        # different numeric values around the limits of float precision and
        # python computed "expected" values differ from the C++ computed values
        # in the 7th decimal place. Use assertAlmostEqual to 6 places for
        # comparisons between C++ and Python computed values. Comparisons
        # between C++ computed values are exact.

        # Verify distances
        self.assertAlmostEqual(a0, expectedDist, places=6)
        self.assertEqual(b0, a0)
        self.assertEqual(b1, a1)
        self.assertEqual(b2, a0 + hyst)  # trail a0 by hysteresis amt

        # Verify LODs
        self.assertAlmostEqual(c0, expectedLod, places=6)
        self.assertEqual(d, c0)
        self.assertEqual(e0, (c0, b0))
        self.assertEqual(e1, (c1, b1))
        self.assertEqual(e2, (c2, b2))

        print(f"  {title:>16s}: dist ={a0:8.4f}, LOD ={c0:8.4f}")


if __name__ == "__main__":
    unittest.main()
