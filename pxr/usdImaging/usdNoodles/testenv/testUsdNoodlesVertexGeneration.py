#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Test file for vertex generation.

Tests:
- Vertex generation produces correct output
- Performance benchmarks
"""

import time
import unittest

from pxr import Gf

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles

    _has_vertex_gen = hasattr(_noodles, "VertexGenerator")
except ImportError:
    _has_vertex_gen = False


@unittest.skipUnless(_has_vertex_gen, "VertexGenerator not available in _noodles")
class TestVertexGeneration(unittest.TestCase):
    """Test that vertex generation produces correct output."""

    def _generate_vertices(self):
        return _noodles.VertexGenerator.generateDefaultNodeVertices(
            Gf.Vec2d(100, 200),  # nodePos
            Gf.Vec2d(150, 80),  # nodeSize
            0.5,  # depth
            20.0,  # titleHeight
            # Background colors
            50,
            40,
            248,
            30,
            24,
            # Selected background colors
            65,
            52,
            39,
            31,
            # Title colors
            128,
            64,
            32,
            77,
            38,
            19,
            # Selected title colors
            166,
            83,
            42,
            100,
            49,
            25,
            # Flags
            0.0,  # innerStroke
            0.0,  # selectedFlag
        )

    def test_vertex_count(self):
        """Test that vertex generation produces 12 vertices."""
        vertices = self._generate_vertices()
        self.assertEqual(len(vertices), 12)

    def test_vertex_structure(self):
        """Test that vertices have the expected attributes."""
        vertices = self._generate_vertices()
        v0 = vertices[0]
        self.assertTrue(hasattr(v0, "x"))
        self.assertTrue(hasattr(v0, "y"))
        self.assertTrue(hasattr(v0, "z"))
        self.assertTrue(hasattr(v0, "r"))
        self.assertTrue(hasattr(v0, "g"))
        self.assertTrue(hasattr(v0, "b"))
        self.assertTrue(hasattr(v0, "a"))

    def test_vertex_positions(self):
        """Test that first vertex has correct position."""
        vertices = self._generate_vertices()
        v0 = vertices[0]
        self.assertAlmostEqual(v0.x, 100, delta=0.01)
        self.assertAlmostEqual(v0.y, 200, delta=0.01)


@unittest.skipUnless(_has_vertex_gen, "VertexGenerator not available in _noodles")
class TestVertexGenerationPerformance(unittest.TestCase):
    """Measure vertex generation performance."""

    def test_performance(self):
        """Benchmark vertex generation."""
        # Warm-up
        for _ in range(10):
            _noodles.VertexGenerator.generateDefaultNodeVertices(
                Gf.Vec2d(100, 200),
                Gf.Vec2d(150, 80),
                0.5,
                20.0,
                50,
                40,
                248,
                30,
                24,
                65,
                52,
                39,
                31,
                128,
                64,
                32,
                77,
                38,
                19,
                166,
                83,
                42,
                100,
                49,
                25,
                0.0,
                0.0,
            )

        iterations = 1000
        start = time.perf_counter()

        for _ in range(iterations):
            _noodles.VertexGenerator.generateDefaultNodeVertices(
                Gf.Vec2d(100, 200),
                Gf.Vec2d(150, 80),
                0.5,
                20.0,
                50,
                40,
                248,
                30,
                24,
                65,
                52,
                39,
                31,
                128,
                64,
                32,
                77,
                38,
                19,
                166,
                83,
                42,
                100,
                49,
                25,
                0.0,
                0.0,
            )

        elapsed_ms = (time.perf_counter() - start) * 1000
        per_call_us = elapsed_ms * 1000 / iterations

        print(f"\n[Perf] {iterations} iterations in {elapsed_ms:.2f}ms")
        print(f"[Perf] {per_call_us:.2f}us per call")


if __name__ == "__main__":
    unittest.main(verbosity=2)
