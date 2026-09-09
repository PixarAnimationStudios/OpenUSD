#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


from __future__ import annotations

import unittest
from types import SimpleNamespace

try:
    from pxr.UsdNoodles.constants import MAX_RENDER_DEPTH
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.Usdviewq.qt import QtGui

    _has_graph = True
except ImportError as e:
    print(f"projection-cache graph imports failed: {e}")
    _has_graph = False


def _expected_world(panX, panY, zoom, width, height):
    projection = QtGui.QMatrix4x4()
    projection.ortho(
        panX,
        panX + width / zoom,
        panY + height / zoom,
        panY,
        -MAX_RENDER_DEPTH,
        MAX_RENDER_DEPTH,
    )
    return projection


def _expected_screen(width, height):
    projection = QtGui.QMatrix4x4()
    projection.ortho(0, width, height, 0, -MAX_RENDER_DEPTH, MAX_RENDER_DEPTH)
    return projection


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class ProjectionMatrixCacheTest(unittest.TestCase):
    """The projection helpers memoize their matrix and rebuild it only when the
    view (pan/zoom/size) changes, so a frame's many render passes reuse a single
    matrix instead of recomputing the same ortho() ~6x/frame."""

    def _make_view(self, panX=0.0, panY=0.0, zoom=1.0, width=200, height=100):
        # Mirror the class-level cache defaults on a lightweight stand-in so the
        # unbound methods can be exercised without building a real GL widget.
        return SimpleNamespace(
            panX=panX,
            panY=panY,
            zoom=zoom,
            width=lambda: width,
            height=lambda: height,
            _worldProjCacheKey=None,
            _worldProjCache=None,
            _screenProjCacheKey=None,
            _screenProjCache=None,
        )

    def test_world_matrix_value_matches_ortho(self):
        view = self._make_view(panX=5.0, panY=7.0, zoom=2.0, width=200, height=100)
        got = GraphView._worldSpaceProjectionMatrix(view)
        expected = _expected_world(5.0, 7.0, 2.0, 200, 100)
        self.assertEqual(list(got.data()), list(expected.data()))

    def test_world_matrix_cache_hit_returns_same_instance(self):
        view = self._make_view()
        first = GraphView._worldSpaceProjectionMatrix(view)
        second = GraphView._worldSpaceProjectionMatrix(view)
        self.assertIs(first, second)

    def test_world_matrix_rebuilds_on_pan_change(self):
        view = self._make_view(panX=0.0)
        first = GraphView._worldSpaceProjectionMatrix(view)
        view.panX = 50.0
        second = GraphView._worldSpaceProjectionMatrix(view)
        self.assertIsNot(first, second)
        self.assertEqual(
            list(second.data()),
            list(_expected_world(50.0, 0.0, 1.0, 200, 100).data()),
        )

    def test_world_matrix_rebuilds_on_zoom_change(self):
        view = self._make_view(zoom=1.0)
        first = GraphView._worldSpaceProjectionMatrix(view)
        view.zoom = 3.0
        second = GraphView._worldSpaceProjectionMatrix(view)
        self.assertIsNot(first, second)

    def test_screen_matrix_value_matches_ortho(self):
        view = self._make_view(width=320, height=240)
        got = GraphView._screenSpaceProjectionMatrix(view)
        self.assertEqual(list(got.data()), list(_expected_screen(320, 240).data()))

    def test_screen_matrix_cache_hit_returns_same_instance(self):
        view = self._make_view()
        first = GraphView._screenSpaceProjectionMatrix(view)
        second = GraphView._screenSpaceProjectionMatrix(view)
        self.assertIs(first, second)

    def test_screen_matrix_rebuilds_on_resize(self):
        view = self._make_view(width=200, height=100)
        first = GraphView._screenSpaceProjectionMatrix(view)
        # The widget reports a new size after a resize.
        view.width = lambda: 400
        second = GraphView._screenSpaceProjectionMatrix(view)
        self.assertIsNot(first, second)
