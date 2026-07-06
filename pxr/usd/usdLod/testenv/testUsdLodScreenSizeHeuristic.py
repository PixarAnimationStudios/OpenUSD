#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Vt, Gf, Usd, UsdGeom, UsdLod

import math
import unittest

CUBE_SIZE = 3.0
THRESHOLDS = [0.25, 0.10]
BLENDS = [0.20, 0.05]

class TestUsdLodCoreApis(unittest.TestCase):


    # Run before each test.
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.world = UsdGeom.Xform.Define(self.stage, "/World")

        self.lodRoot = UsdGeom.Xform.Define(self.stage, "/World/lodRoot")
        self.lodRootPrim = self.lodRoot.GetPrim()

        # Translate it so that one corner of the cube would be at the origin
        # extending back in -Z.
        self.lodRoot.AddTranslateOp().Set(Gf.Vec3d(CUBE_SIZE / 2.0,
                                                   CUBE_SIZE / 2.0,
                                                   -CUBE_SIZE / 2.0))

        self.lodItems = [
            UsdGeom.Sphere.Define(self.stage, "/World/lodRoot/lodItem_0"),
            UsdGeom.Cylinder.Define(self.stage, "/World/lodRoot/lodItem_1"),
            UsdGeom.Cube.Define(self.stage, "/World/lodRoot/lodItem_2"),
        ]

        visValues = ["invisible", "inherited", "invisible"]
        for i, lodItem in enumerate(self.lodItems):
            visAttr = lodItem.CreateVisibilityAttr(visValues[i])

        cube = self.lodItems[2]
        cube.CreateSizeAttr(CUBE_SIZE)

        self.heuristicsScope = UsdGeom.Scope.Define(self.stage,
                                                    "/World/heuristics")

        # Create two size heuristics, one using projected sphere and one
        # using projected extent
        methods = ["projectedSphere", "projectedExtent"]
        self.heuristics = []
        for i in range(2):
            heuristic =  UsdLod.ScreenSizeHeuristic.Define(
                self.stage, f"/World/heuristics/screenSize_{i}")
            heuristic.CreateProjectionMethodAttr(methods[i])

            # Setup for the generic "imaging" domain with a bounds relationship
            # targeting the cube, and blendThresholds for blending.
            heuristic.CreateLodDomainAttr("imaging")
            boundsRel = heuristic.CreateBoundingVolumeRel()
            boundsRel.AddTarget(self.lodItems[2].GetPath())
            heuristic.CreateExtentAttr()
            heuristic.CreateThresholdsAttr(Vt.FloatArray([.25, .10]))
            heuristic.CreateBlendThresholdsAttr(Vt.FloatArray([.20, .05]))

            self.heuristics.append(heuristic)

    def test_ScreenSizeHeuristic(self):

        def _expectedScreenSizes(z):
            """
            Return a pair of sizes (sphereSize, extentSize) where sphereSize is the
            calculated size for bounding sphere of the extent and extentSize is
            the projected area of the extent.  Both sizes are fractions of the
            screen window with 1.0 being 100% coverage of the window and 0.0
            being no coverage.
            """
            
            # The default view frustum has a viewpoint at the origin, a window
            # of -1 .. +1, and a near plane at 1.0. This gives us a 90 degree
            # FOV with the edges of the frustum following diagonal lines from
            # the viewpoint. At a distance of 3.0 from the viewpoint, the view
            # window is -3 .. +3.
            #
            # We've created the cube so that one edge is on the -Z axis and the
            # width and height are 3 units. So when the viewpoint is 3 units
            # from the cube it should occlude exactly 25% of the screen.
            #
            # These values need to match the contents of the stage created in
            # the setUp method. The cube should extend from the origin to
            # (3.0, 3.0, -3.0)
            extent = Gf.Range3d(Gf.Vec3d(-1.5), Gf.Vec3d(1.5))
            xform = Gf.Matrix4d().SetTranslate(Gf.Vec3d(CUBE_SIZE / 2,
                                                        CUBE_SIZE / 2,
                                                        -CUBE_SIZE / 2))

            frustum = Gf.Frustum()
            frustum.position = Gf.Vec3d(0.0, 0.0, z)

            planes = frustum.ComputePlanes()

            # Compute sphere size
            worldMin    = xform.Transform(extent.GetMin())
            worldMax    = xform.Transform(extent.GetMax())

            worldCenter = (worldMax + worldMin) / 2
            worldRadius = (worldMax - worldMin).GetLength() / 2

            # Check for trivial rejection
            clipped = False
            for plane in planes:
                if plane.GetDistance(worldCenter) < -worldRadius:
                    clipped = True
                    sphereSize = 0.0
                    break

            if not clipped:
                worldDistance = (worldCenter - Gf.Vec3d(0, 0, z)).GetLength()

                radius = worldRadius / worldDistance
                area = math.pi * radius**2

                # The area of a perspective frustum window is 4
                sphereSize = area / 4

            # Compute projected extent size

            # Trivial reject
            bbox = Gf.BBox3d(extent, xform)
            clipped = not frustum.Intersects(bbox)

            if clipped:
                extentSize = 0.0
                return (sphereSize, extentSize)

            # At least part of the extent is visible.
            ndcTransform = (xform *
                            frustum.ComputeViewMatrix() *
                            frustum.ComputeProjectionMatrix())

            corners = [extent.GetCorner(i) for i in range(8)]
            corners4 = [Gf.Vec4d(*corners[i], 1) for i in range(8)]
            clipCorners = [corners4[i] * ndcTransform for i in range(8)]

            edgeIndices = [
                (0, 1), (2, 3), (4, 5), (6, 7),  # parallel to x-axis
                (0, 2), (1, 3), (4, 6), (5, 7),  # parallel to y-axis
                (0, 4), (1, 5), (2, 6), (3, 7),  # parallel to z-axis
            ]

            # If nearPlane * point < 0, the point is clipped by nearPlane
            nearPlane = Gf.Vec4d(0, 0, 1, 1)

            # Clip against the near plane.
            outputPoints = []
            for i, j in edgeIndices:
                p0 = clipCorners[i]
                p1 = clipCorners[j]

                tMin = 0.0
                tMax = 1.0
                
                d0 = nearPlane * p0
                d1 = nearPlane * p1

                if d0 < 0 and d1 < 0:
                    continue
                elif d0 < 0 or d1 < 0:
                    t = d0 / (d0 - d1)

                    if d0 < 0:
                        tMin = max(tMin, t)
                    else:
                        tMax = min(tMax, t)

                ndc0 = Gf.Project(Gf.Lerp(tMin, p0, p1))
                ndc1 = Gf.Project(Gf.Lerp(tMax, p0, p1))

                # Store 2-tuples
                outputPoints.append((ndc0[0], ndc0[1]))
                outputPoints.append((ndc1[0], ndc1[1]))

            # Sort the points
            outputPoints.sort()

            # Remove duplicates
            i = 1
            while i < len(outputPoints):
                if (Gf.IsClose(outputPoints[i-1][0], outputPoints[i][0], 1e-6) and
                    Gf.IsClose(outputPoints[i-1][1], outputPoints[i][1], 1e-6)):
                    del outputPoints[i]
                else:
                    i += 1
                
            # Project the shape and find its area
            # Compute the 2d cross product of (b - a) x (c - a)
            cross = lambda a, b, c: ((b[0] - a[0]) * (c[1] - a[1]) -
                                     (b[1] - a[1]) * (c[0] - a[0]))
                
            lower = []
            for p in outputPoints:
                while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
                    lower.pop()
                lower.append(p)

            upper = []
            for p in reversed(outputPoints):
                while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
                    upper.pop()
                upper.append(p)

            # Note that the first point of lower is repeated as the last
            # point of upper and so appears in hull twice. This makes the
            # shoelace area formula easier. (The first point in upper is
            # also the last point in lower but we don't want it to appear
            # in the middle of the hull so we discard it.)
            hull = lower + upper[1:]

            area2 = sum(hull[i][0] * hull[i+1][1] - hull[i][1] * hull[i+1][0]
                        for i in range(len(hull) - 1))

            # The area of the NDC space window is 4 and area2 is twice the area
            # so divide by 8 to get the covered fraction.
            extentSize = area2 / 8

            return (sphereSize, extentSize)

        def _expectedLODs(sizes, thresholds, blends):
            """
            Return a pair of LOD indices, one for each size in the sizes list.
            """
            lods = []
            for i, size in enumerate(sizes):
                for j, threshold in enumerate(thresholds):
                    blend = blends[j] if j < len(blends) else None

                    if size > threshold:
                        lods.append(float(j))
                        break
                    elif blend is not None and size > blend:
                        lods.append(j + (threshold - size) / (threshold - blend))
                        break
                else:
                    lods.append(len(thresholds))

            return lods

        cache = UsdGeom.XformCache()
        rootToWorld = cache.GetLocalToWorldTransform(self.lodRootPrim)

        # Test the UsdLodScreenSizeHeuristicQuery
        queries = []
        for heuristic in self.heuristics:
            query = heuristic.CreateScreenSizeHeuristicQuery()

            self.assertIsInstance(
                query, UsdLod.HeuristicQuery,
                msg=(f"Type returned from CreateScreenSizeHeuristicQuery"
                     f" ({type(query).__name__}) is not a"
                     f" UsdLod.HeuristicQuery"))
            self.assertIsInstance(
                query, UsdLod.ScreenSizeHeuristicQuery,
                msg=(f"Type returned from CreateScreenSizeHeuristicQuery"
                     f" ({type(query).__name__}) is not a"
                     f" UsdLod.ScreenSizeHeuristicQuery"))

            queries.append(query)

        for z in range(0, 30):
            print(f"z = {z}", flush=True)
            frustum = Gf.Frustum()
            frustum.position = Gf.Vec3d(0.0, 0.0, z)

            expSizes = _expectedScreenSizes(z)
            expLods  = _expectedLODs(expSizes, THRESHOLDS, BLENDS)

            projections = ["sphere", "extent"]

            for i in range(2):
                self._DoOneScreenSizeTest(
                    f"screenSize_{i} ({projections[i]})", self.heuristics[i],
                    frustum, rootToWorld, expSizes[i], expLods[i])

                self._DoOneScreenSizeTest(
                    f"screenSize_{i} query ({projections[i]})", queries[i],
                    frustum, rootToWorld, expSizes[i], expLods[i])

    def _DoOneScreenSizeTest(self, title, queryable, frustum, transform,
                             expectedSize, expectedLod):
        """
        Verify that we can compute screenSizes and LODs with various call
        signatures and with and without hysteresis.
        """
        # There are 5 distinct calls with variations to test in each of the
        # heuristic and the heuristicQuery that need to be tested.
        #   a) computeScreenSize from frustum and transform
        #      a0 = screenSize
        #      a1 = screenSize + 0.05
        #      a2 = screenSize + 0.25
        #   b) computeScreenSize from frustum and transform with hysteresis
        #      b0 = no hysteresis, returns a0
        #      b1 = hysteresis returns screenSize a1
        #      b2 = hysteresis exceded returns screenSize a0
        #   c) computeLOD from screenSize
        #      c0 = LOD for a0
        #      c1 = LOD for a1
        #   d) computeLOD from frustum and transform = c0
        #   e) computeLOD from frustum and transform with hysteresis
        #      e0 = (c0, a0)
        #      e1 = (c1, a1)
        #      e2 = (c0, a0)
        #
        hyst = 0.1
        
        a0 = queryable.ComputeScreenSize(frustum, transform)
        a1 = a0 + 0.05
        a2 = a0 + 0.25

        b0 = queryable.ComputeScreenSize(frustum, transform,
                                         prevSize=a1,
                                         hysteresis=0.0)
        b1 = queryable.ComputeScreenSize(frustum, transform,
                                         prevSize=a1,
                                         hysteresis=hyst)
        b2 = queryable.ComputeScreenSize(frustum, transform,
                                         prevSize=a2,
                                         hysteresis=hyst)
        c0 = queryable.ComputeLOD(b0)
        c1 = queryable.ComputeLOD(b1)
        c2 = queryable.ComputeLOD(b2)

        d = queryable.ComputeLOD(frustum, transform)

        e0 = queryable.ComputeLOD(frustum, transform,
                                  prevSize=a1,
                                  hysteresis=0.0)
        e1 = queryable.ComputeLOD(frustum, transform,
                                  prevSize=a1,
                                  hysteresis=hyst)
        e2 = queryable.ComputeLOD(frustum, transform,
                                  prevSize=a2,
                                  hysteresis=hyst)

        # Note that vertices in USD are 32-bit floats but all floating point
        # values in Python are 64-bit doubles. This leads to some slightly
        # different numeric values around the limits of float precision and
        # python computed "expected" values differ from the C++ computed values
        # in the 7th decimal place. Use assertAlmostEqual to 6 places for
        # comparisons between C++ and Python computed values. Comparisons
        # between C++ computed values are exact.

        print(f"  {title:>32s}: size ={a0:8.4f}, LOD ={c0:8.4f}", flush=True)
        print(f"  {'expected':>32s}: size ={expectedSize:8.4f}, LOD ={expectedLod:8.4f}", flush=True)

        # Verify screenSizes
        self.assertAlmostEqual(a0, expectedSize, places=6,
                               msg=f"{frustum.position = }")
        self.assertEqual(b0, a0)
        self.assertEqual(b1, a1)
        self.assertEqual(b2, a0 + hyst)

        # Verify LODs
        self.assertAlmostEqual(c0, expectedLod, places=6)
        self.assertEqual(d, c0)
        self.assertEqual(e0, (c0, b0))
        self.assertEqual(e1, (c1, b1))
        self.assertEqual(e2, (c2, b2))


if __name__ == "__main__":
    unittest.main()
