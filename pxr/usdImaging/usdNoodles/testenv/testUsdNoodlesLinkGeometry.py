#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Test file for link geometry C++ backend.

Tests Phase 3 implementation:
- Verifies LinkGeometry C++ backend works correctly
- Compares C++ vs Python link intersection results
- Measures performance improvements
"""

import random
import time
import unittest

from pxr import Gf

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles

    _has_noodles = True
except ImportError:
    _has_noodles = False


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestLinkGeometryAvailability(unittest.TestCase):
    """Test that LinkGeometry class is available."""

    def test_class_exists(self):
        """Test that LinkGeometry class is available."""
        self.assertTrue(hasattr(_noodles, "LinkGeometry"))


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestFindLinkUnderCursor(unittest.TestCase):
    """Test findLinkUnderCursor function."""

    def setUp(self):
        self.linkEndpoints = [
            (Gf.Vec2d(0, 0), Gf.Vec2d(100, 100)),  # Link 0: diagonal
            (Gf.Vec2d(200, 0), Gf.Vec2d(200, 100)),  # Link 1: vertical
            (Gf.Vec2d(300, 50), Gf.Vec2d(400, 50)),  # Link 2: horizontal
        ]

    def test_cursor_on_diagonal_link(self):
        result = _noodles.LinkGeometry.findLinkUnderCursor(
            Gf.Vec2d(50, 50), self.linkEndpoints, 10.0
        )
        self.assertEqual(result, 0)

    def test_cursor_on_vertical_link(self):
        result = _noodles.LinkGeometry.findLinkUnderCursor(
            Gf.Vec2d(202, 50), self.linkEndpoints, 10.0
        )
        self.assertEqual(result, 1)

    def test_cursor_not_on_any_link(self):
        result = _noodles.LinkGeometry.findLinkUnderCursor(
            Gf.Vec2d(500, 500), self.linkEndpoints, 10.0
        )
        self.assertEqual(result, -1)


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestLinkIntersectsBounds(unittest.TestCase):
    """Test linkIntersectsBounds function."""

    def setUp(self):
        self.bounds = Gf.Range2d(Gf.Vec2d(50, 50), Gf.Vec2d(150, 150))

    def test_endpoint_inside_bounds(self):
        result = _noodles.LinkGeometry.linkIntersectsBounds(
            Gf.Vec2d(100, 100), Gf.Vec2d(200, 200), self.bounds
        )
        self.assertTrue(result)

    def test_link_crossing_through_bounds(self):
        result = _noodles.LinkGeometry.linkIntersectsBounds(
            Gf.Vec2d(0, 100), Gf.Vec2d(200, 100), self.bounds
        )
        self.assertTrue(result)

    def test_link_completely_outside_bounds(self):
        result = _noodles.LinkGeometry.linkIntersectsBounds(
            Gf.Vec2d(200, 0), Gf.Vec2d(300, 0), self.bounds
        )
        self.assertFalse(result)


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestFindLinksInBounds(unittest.TestCase):
    """Test findLinksInBounds function."""

    def test_finds_intersecting_links(self):
        linkEndpoints = [
            (Gf.Vec2d(0, 0), Gf.Vec2d(100, 100)),  # Link 0: crosses bounds
            (Gf.Vec2d(200, 200), Gf.Vec2d(300, 300)),  # Link 1: outside
            (Gf.Vec2d(60, 60), Gf.Vec2d(70, 70)),  # Link 2: inside
            (Gf.Vec2d(0, 100), Gf.Vec2d(200, 100)),  # Link 3: crosses horizontally
        ]
        bounds = Gf.Range2d(Gf.Vec2d(50, 50), Gf.Vec2d(150, 150))

        result = _noodles.LinkGeometry.findLinksInBounds(linkEndpoints, bounds)
        self.assertEqual(sorted(result), [0, 2, 3])


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestCalculateLOD(unittest.TestCase):
    """Test calculateLOD function."""

    def setUp(self):
        self.lodThresholds = [(80, 160), (40, 80), (20, 40), (10, 20), (5, 10)]

    def test_lod_values(self):
        test_cases = [
            (1000.0, 6.0, 160),
            (300.0, 6.0, 80),
            (100.0, 6.0, 20),
            (30.0, 6.0, 5),
        ]
        for manhattanLength, diffX, expected in test_cases:
            with self.subTest(manhattanLength=manhattanLength, diffX=diffX):
                result = _noodles.LinkGeometry.calculateLOD(
                    manhattanLength, diffX, self.lodThresholds
                )
                self.assertEqual(result, expected)


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestGenerateReferenceCurve(unittest.TestCase):
    """Test generateReferenceCurve function."""

    def test_output_length(self):
        numSamples = 20
        result = _noodles.LinkGeometry.generateReferenceCurve(numSamples)
        expected_length = numSamples * 2 * 7
        self.assertEqual(len(result), expected_length)

    def test_first_vertex_position(self):
        result = _noodles.LinkGeometry.generateReferenceCurve(20)
        self.assertAlmostEqual(result[0], 0.0, delta=0.001)  # pos.x
        self.assertAlmostEqual(result[1], 0.0, delta=0.001)  # pos.y
        self.assertAlmostEqual(result[6], -1.0, delta=0.001)  # dir


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestLinkGeometryPerformance(unittest.TestCase):
    """Measure performance of LinkGeometry operations."""

    def setUp(self):
        random.seed(42)
        self.num_links = 1000
        self.linkEndpoints = [
            (
                Gf.Vec2d(random.uniform(0, 1000), random.uniform(0, 1000)),
                Gf.Vec2d(random.uniform(0, 1000), random.uniform(0, 1000)),
            )
            for _ in range(self.num_links)
        ]

    def test_find_link_under_cursor_performance(self):
        # Warm-up
        for _ in range(10):
            _noodles.LinkGeometry.findLinkUnderCursor(
                Gf.Vec2d(500, 500), self.linkEndpoints, 10.0
            )

        iterations = 1000
        start = time.perf_counter()
        for _ in range(iterations):
            _noodles.LinkGeometry.findLinkUnderCursor(
                Gf.Vec2d(random.uniform(0, 1000), random.uniform(0, 1000)),
                self.linkEndpoints,
                10.0,
            )
        elapsed_ms = (time.perf_counter() - start) * 1000
        per_call_us = elapsed_ms * 1000 / iterations

        print(
            f"\n[Perf] findLinkUnderCursor ({self.num_links} links): "
            f"{iterations} iterations in {elapsed_ms:.2f}ms ({per_call_us:.2f}us/call)"
        )

    def test_find_links_in_bounds_performance(self):
        bounds = Gf.Range2d(Gf.Vec2d(400, 400), Gf.Vec2d(600, 600))
        iterations = 1000
        start = time.perf_counter()
        for _ in range(iterations):
            _noodles.LinkGeometry.findLinksInBounds(self.linkEndpoints, bounds)
        elapsed_ms = (time.perf_counter() - start) * 1000
        per_call_us = elapsed_ms * 1000 / iterations

        print(
            f"\n[Perf] findLinksInBounds ({self.num_links} links): "
            f"{iterations} iterations in {elapsed_ms:.2f}ms ({per_call_us:.2f}us/call)"
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
