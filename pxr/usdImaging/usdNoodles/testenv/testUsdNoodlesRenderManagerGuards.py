#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


import unittest

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles

    _has_render_manager = hasattr(_noodles, "NodeRenderManager")
except ImportError:
    _has_render_manager = False


@unittest.skipUnless(_has_render_manager, "NodeRenderManager not available in _noodles")
class TestRenderManagerBindingGuards(unittest.TestCase):
    """Verify Boost.Python binding length guards reject short lists."""

    def setUp(self):
        self.mgr = _noodles.NodeRenderManager()

    def _assert_no_crash(self, fn, *args, **kwargs):
        try:
            fn(*args, **kwargs)
        except (IndexError, SystemError) as e:
            self.fail(f"Binding guard failed to reject short list: {e}")

    def test_renderNodes_short_projection(self):
        self._assert_no_crash(
            self.mgr.renderNodes, [], 0, 0, False, [1.0] * 4, 6.0, [0.0] * 4
        )

    def test_renderNodes_short_stroke_color(self):
        self._assert_no_crash(
            self.mgr.renderNodes, [], 0, 0, False, [0.0] * 16, 6.0, [1.0]
        )

    def test_renderNodes_both_short(self):
        self._assert_no_crash(self.mgr.renderNodes, [], 0, 0, False, [], 6.0, [])

    def test_renderPortHighlight_short_projection(self):
        self._assert_no_crash(
            self.mgr.renderPortHighlight, 0, 0, 10, 10, 0, [1.0] * 4, 6.0, [0.0] * 4
        )

    def test_renderPortHighlight_short_color(self):
        self._assert_no_crash(
            self.mgr.renderPortHighlight, 0, 0, 10, 10, 0, [0.0] * 16, 6.0, [1.0]
        )
