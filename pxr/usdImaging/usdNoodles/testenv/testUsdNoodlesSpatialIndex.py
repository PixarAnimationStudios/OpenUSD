#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Test module for SpatialIndex implementation.

This module tests the spatial index functionality and measures performance.
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
class TestSpatialIndex(unittest.TestCase):
    """Test suite for SpatialIndex implementation."""

    def test_basic_construction(self):
        """Test basic SpatialIndex construction."""
        index = _noodles.SpatialIndex()
        self.assertEqual(index.getNodeCount(), 0)
        self.assertEqual(index.getLinkCount(), 0)

    def test_construction_with_bounds(self):
        """Test SpatialIndex construction with custom bounds."""
        bounds = Gf.Range2d(Gf.Vec2d(-1000, -1000), Gf.Vec2d(1000, 1000))
        index = _noodles.SpatialIndex(bounds, 8, 10, 10.0)
        self.assertEqual(index.getNodeCount(), 0)

    def test_insert_node(self):
        """Test inserting a single node."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(100, 50))
        index.insertNode("node1", bounds)
        self.assertEqual(index.getNodeCount(), 1)

    def test_insert_multiple_nodes(self):
        """Test inserting multiple nodes."""
        index = _noodles.SpatialIndex()
        for i in range(100):
            bounds = Gf.Range2d(
                Gf.Vec2d(i * 10, i * 10), Gf.Vec2d(i * 10 + 100, i * 10 + 50)
            )
            index.insertNode(f"node{i}", bounds)
        self.assertEqual(index.getNodeCount(), 100)

    def test_insert_link(self):
        """Test inserting a single link."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(200, 100))
        index.insertLink(0, bounds)
        self.assertEqual(index.getLinkCount(), 1)

    def test_insert_multiple_links(self):
        """Test inserting multiple links."""
        index = _noodles.SpatialIndex()
        for i in range(100):
            bounds = Gf.Range2d(
                Gf.Vec2d(i * 10, i * 5), Gf.Vec2d(i * 10 + 200, i * 5 + 100)
            )
            index.insertLink(i, bounds)
        self.assertEqual(index.getLinkCount(), 100)

    def test_remove_node(self):
        """Test removing a node."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(100, 50))
        index.insertNode("node1", bounds)
        self.assertEqual(index.getNodeCount(), 1)

        result = index.removeNode("node1")
        self.assertTrue(result)
        self.assertEqual(index.getNodeCount(), 0)

        # Removing non-existent node should return False
        result = index.removeNode("node1")
        self.assertFalse(result)

    def test_remove_link(self):
        """Test removing a link."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(200, 100))
        index.insertLink(0, bounds)
        self.assertEqual(index.getLinkCount(), 1)

        result = index.removeLink(0)
        self.assertTrue(result)
        self.assertEqual(index.getLinkCount(), 0)

        # Removing non-existent link should return False
        result = index.removeLink(0)
        self.assertFalse(result)

    def test_update_node(self):
        """Test updating a node's position."""
        index = _noodles.SpatialIndex()
        bounds1 = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(100, 50))
        index.insertNode("node1", bounds1)

        # Update to new position
        bounds2 = Gf.Range2d(Gf.Vec2d(500, 500), Gf.Vec2d(600, 550))
        index.updateNode("node1", bounds2)

        # Should still have 1 node
        self.assertEqual(index.getNodeCount(), 1)

        # Query old position should not find node
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(50, 25))
        self.assertEqual(len(nodeIds), 0)

        # Query new position should find node
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(550, 525))
        self.assertEqual(len(nodeIds), 1)
        self.assertEqual(nodeIds[0], "node1")

    def test_query_point_node(self):
        """Test querying a point that hits a node."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(100, 50))
        index.insertNode("node1", bounds)

        # Point inside node
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(50, 25))
        self.assertEqual(len(nodeIds), 1)
        self.assertEqual(nodeIds[0], "node1")
        self.assertEqual(len(linkIndices), 0)

        # Point outside node
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(200, 200))
        self.assertEqual(len(nodeIds), 0)

    def test_query_point_link(self):
        """Test querying a point that hits a link."""
        index = _noodles.SpatialIndex()
        bounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(200, 100))
        index.insertLink(5, bounds)

        # Point inside link bounds
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(100, 50))
        self.assertEqual(len(nodeIds), 0)
        self.assertEqual(len(linkIndices), 1)
        self.assertEqual(linkIndices[0], 5)

    def test_query_point_multiple_items(self):
        """Test querying a point that hits multiple items."""
        index = _noodles.SpatialIndex()

        # Overlapping node and link
        nodeBounds = Gf.Range2d(Gf.Vec2d(0, 0), Gf.Vec2d(100, 50))
        linkBounds = Gf.Range2d(Gf.Vec2d(50, 25), Gf.Vec2d(150, 75))

        index.insertNode("node1", nodeBounds)
        index.insertLink(0, linkBounds)

        # Query overlapping region
        nodeIds, linkIndices = index.queryPoint(Gf.Vec2d(75, 30))
        self.assertEqual(len(nodeIds), 1)
        self.assertEqual(len(linkIndices), 1)

    def test_query_region(self):
        """Test querying items in a region."""
        index = _noodles.SpatialIndex()

        # Add several nodes
        for i in range(10):
            bounds = Gf.Range2d(Gf.Vec2d(i * 100, 0), Gf.Vec2d(i * 100 + 50, 50))
            index.insertNode(f"node{i}", bounds)

        # Query region that should contain nodes 2, 3, 4
        queryBounds = Gf.Range2d(Gf.Vec2d(200, 0), Gf.Vec2d(500, 100))
        nodeIds, linkIndices = index.queryRegion(queryBounds)

        self.assertGreaterEqual(len(nodeIds), 3)
        self.assertIn("node2", nodeIds)
        self.assertIn("node3", nodeIds)
        self.assertIn("node4", nodeIds)

    def test_bulk_insert(self):
        """Test bulk inserting nodes and links."""
        index = _noodles.SpatialIndex()

        nodeIds = [f"node{i}" for i in range(100)]
        nodeBounds = [
            Gf.Range2d(Gf.Vec2d(i * 10, i * 10), Gf.Vec2d(i * 10 + 100, i * 10 + 50))
            for i in range(100)
        ]
        linkBounds = [
            Gf.Range2d(Gf.Vec2d(i * 5, i * 5), Gf.Vec2d(i * 5 + 200, i * 5 + 100))
            for i in range(50)
        ]

        index.bulkInsert(nodeIds, nodeBounds, linkBounds)

        self.assertEqual(index.getNodeCount(), 100)
        self.assertEqual(index.getLinkCount(), 50)

    def test_clear(self):
        """Test clearing the spatial index."""
        index = _noodles.SpatialIndex()

        # Add items
        for i in range(10):
            bounds = Gf.Range2d(Gf.Vec2d(i * 10, 0), Gf.Vec2d(i * 10 + 50, 50))
            index.insertNode(f"node{i}", bounds)
            index.insertLink(i, bounds)

        self.assertEqual(index.getNodeCount(), 10)
        self.assertEqual(index.getLinkCount(), 10)

        # Clear
        index.clear()
        self.assertEqual(index.getNodeCount(), 0)
        self.assertEqual(index.getLinkCount(), 0)

    def test_get_cells(self):
        """Test getting quadtree cells for visualization."""
        index = _noodles.SpatialIndex()

        # Add some items to create tree structure
        for i in range(20):
            bounds = Gf.Range2d(
                Gf.Vec2d(i * 50, i * 50), Gf.Vec2d(i * 50 + 100, i * 50 + 50)
            )
            index.insertNode(f"node{i}", bounds)

        cells = index.getCells()
        self.assertGreater(len(cells), 0)
        # All cells should be valid Range2d
        for cell in cells:
            self.assertIsInstance(cell, Gf.Range2d)

    def test_bounds_expansion(self):
        """Test that bounds expand to accommodate items."""
        index = _noodles.SpatialIndex()

        # Insert items at far positions
        bounds1 = Gf.Range2d(Gf.Vec2d(-5000, -5000), Gf.Vec2d(-4900, -4950))
        bounds2 = Gf.Range2d(Gf.Vec2d(5000, 5000), Gf.Vec2d(5100, 5050))

        index.insertNode("node1", bounds1)
        index.insertNode("node2", bounds2)

        # Both should be queryable
        nodeIds, _ = index.queryPoint(Gf.Vec2d(-4950, -4975))
        self.assertEqual(len(nodeIds), 1)

        nodeIds, _ = index.queryPoint(Gf.Vec2d(5050, 5025))
        self.assertEqual(len(nodeIds), 1)


@unittest.skipUnless(_has_noodles, "_noodles C++ module not available")
class TestSpatialIndexPerformance(unittest.TestCase):
    """Performance tests for SpatialIndex."""

    def _generate_random_bounds(self, count, spread=10000, size_range=(50, 200)):
        """Generate random bounding boxes."""
        bounds = []
        for _ in range(count):
            x = random.uniform(-spread / 2, spread / 2)
            y = random.uniform(-spread / 2, spread / 2)
            w = random.uniform(size_range[0], size_range[1])
            h = random.uniform(size_range[0] * 0.5, size_range[1] * 0.5)
            bounds.append(Gf.Range2d(Gf.Vec2d(x, y), Gf.Vec2d(x + w, y + h)))
        return bounds

    def test_insertion_performance(self):
        """Benchmark insertion performance."""
        node_count = 1000
        nodeIds = [f"node{i}" for i in range(node_count)]
        nodeBounds = self._generate_random_bounds(node_count)

        index = _noodles.SpatialIndex()
        start = time.perf_counter()
        index.bulkInsert(nodeIds, nodeBounds, [])
        cpp_time = (time.perf_counter() - start) * 1000

        print(f"\n[Perf] Bulk insert {node_count} nodes: {cpp_time:.2f}ms")
        self.assertEqual(index.getNodeCount(), node_count)

    def test_query_point_performance(self):
        """Benchmark point query performance."""
        # Build index with 1000 nodes
        node_count = 1000
        nodeIds = [f"node{i}" for i in range(node_count)]
        nodeBounds = self._generate_random_bounds(node_count)

        index = _noodles.SpatialIndex()
        index.bulkInsert(nodeIds, nodeBounds, [])

        # Time 1000 point queries
        query_count = 1000
        queryPoints = [
            Gf.Vec2d(random.uniform(-5000, 5000), random.uniform(-5000, 5000))
            for _ in range(query_count)
        ]

        start = time.perf_counter()
        for point in queryPoints:
            index.queryPoint(point)
        cpp_time = (time.perf_counter() - start) * 1000

        time_per_query = cpp_time / query_count * 1000  # microseconds

        print(
            f"\n[Perf] {query_count} point queries: {cpp_time:.2f}ms ({time_per_query:.2f}us per query)"
        )
        self.assertEqual(index.getNodeCount(), node_count)

    def test_query_region_performance(self):
        """Benchmark region query performance."""
        # Build index with 1000 nodes
        node_count = 1000
        nodeIds = [f"node{i}" for i in range(node_count)]
        nodeBounds = self._generate_random_bounds(node_count)

        index = _noodles.SpatialIndex()
        index.bulkInsert(nodeIds, nodeBounds, [])

        # Time 1000 region queries
        query_count = 1000
        queryRegions = self._generate_random_bounds(
            query_count, spread=10000, size_range=(200, 500)
        )

        start = time.perf_counter()
        for region in queryRegions:
            index.queryRegion(region)
        cpp_time = (time.perf_counter() - start) * 1000

        time_per_query = cpp_time / query_count * 1000  # microseconds

        print(
            f"\n[Perf] {query_count} region queries: {cpp_time:.2f}ms ({time_per_query:.2f}us per query)"
        )
        self.assertEqual(index.getNodeCount(), node_count)


if __name__ == "__main__":
    unittest.main(verbosity=2)
