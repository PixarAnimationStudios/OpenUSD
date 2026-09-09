#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


from __future__ import annotations

import unittest

try:
    from pxr.UsdNoodles.render import NodeRenderManager

    _has_noodles = True
except ImportError:
    _has_noodles = False


@unittest.skipUnless(_has_noodles, "noodles C++ bindings not available")
class TestRenderManagerBindingGuards(unittest.TestCase):
    """Binding-level length guards must silently return on short lists."""

    def setUp(self):
        self.mgr = NodeRenderManager()

    def _assert_no_crash(self, fn, *args, **kwargs):
        try:
            fn(*args, **kwargs)
        except (IndexError, SystemError) as e:
            self.fail(f"Binding guard failed to reject short list: {e}")

    def test_renderEndpointCircle_short_projection(self):
        self._assert_no_crash(
            self.mgr.renderEndpointCircle,
            0.0,
            0.0,
            5.0,
            0.0,
            [1.0] * 4,
            [1.0, 0.0, 0.0, 1.0],
            [0.0, 0.0, 0.0, 1.0],
            1.0,
        )

    def test_renderEndpointCircle_short_fill_color(self):
        self._assert_no_crash(
            self.mgr.renderEndpointCircle,
            0.0,
            0.0,
            5.0,
            0.0,
            [1.0] * 16,
            [1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
            1.0,
        )

    def test_renderEndpointCircle_short_stroke_color(self):
        self._assert_no_crash(
            self.mgr.renderEndpointCircle,
            0.0,
            0.0,
            5.0,
            0.0,
            [1.0] * 16,
            [1.0, 0.0, 0.0, 1.0],
            [],
            1.0,
        )

    def test_renderEndpointTriangle_short_projection(self):
        self._assert_no_crash(
            self.mgr.renderEndpointTriangle,
            0.0,
            0.0,
            10.0,
            0.0,
            5.0,
            10.0,
            0.0,
            [1.0] * 4,
            [1.0, 0.0, 0.0, 1.0],
        )

    def test_renderEndpointTriangle_short_color(self):
        self._assert_no_crash(
            self.mgr.renderEndpointTriangle,
            0.0,
            0.0,
            10.0,
            0.0,
            5.0,
            10.0,
            0.0,
            [1.0] * 16,
            [1.0],
        )
