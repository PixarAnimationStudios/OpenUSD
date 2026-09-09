#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for the CppIconRenderer facade over the C++ IconRenderManager.

These cover the Python-side behavior (config bridging, the sync-then-render gate,
graceful degradation when the C++ extension is unavailable, and the empty-input
short circuit) by mocking the C++ ``_cpp`` object, so they need no GL context.
"""

import sys
import types
import unittest
from unittest import mock

try:
    from pxr.UsdNoodles.core import RenderConfig

    _has_render_config = True
except ImportError as e:
    print(f"noodles.core.RenderConfig import failed: {e}")
    _has_render_config = False

try:
    from pxr.UsdNoodles.iconRenderer import CppIconRenderer

    _has_icon_renderer = True
except ImportError as e:
    print(f"pxr.UsdNoodles.iconRenderer import failed: {e}")
    _has_icon_renderer = False

_has_pxr = _has_render_config and _has_icon_renderer


def _patch_config(values):
    """Stub ``NoodlesConfig.get`` via a ``sys.modules`` shim so the inner
    ``from .noodlesConfig import NoodlesConfig`` in ``build_render_config``
    picks it up without importing the real module (which transitively imports
    Qt — not a dep of the pytest target)."""
    fake = types.ModuleType("pxr.UsdNoodles.noodlesConfig")

    class _FakeConfig:
        @classmethod
        def get(cls, key, default=None):
            return values.get(key, default)

    fake.NoodlesConfig = _FakeConfig
    return mock.patch.dict(sys.modules, {"pxr.UsdNoodles.noodlesConfig": fake})


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestCppIconRendererNodeIcons(unittest.TestCase):
    def _make_renderer(self):
        renderer = CppIconRenderer()
        renderer._cpp = mock.Mock()
        # Default to "no pending rebuild" so the unchanged-frame path skips the
        # re-sync. renderNodeIcons gates the sync on content_changed OR
        # needsIconRebuild(); a bare Mock() returns a truthy Mock for the latter,
        # which would spuriously force a sync on a steady-state frame.
        renderer._cpp.needsIconRebuild.return_value = False
        return renderer

    def _make_graph(self, nodes=None):
        graph = mock.Mock()
        graph.nodes = {"a": object()} if nodes is None else nodes
        return graph

    def test_renderNodeIcons_syncs_and_passes_build_render_config(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer._cpp.renderIconsFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        # An override proves the passed RenderConfig came from build_render_config()
        # (a regression to a bare RenderConfig() would carry the struct default).
        override = RenderConfig().nodeCornerRadius + 7.0
        with _patch_config({"nodeCornerRadius": override}):
            result = renderer.renderNodeIcons(graph, projection, 1.0, True)

        self.assertTrue(result)
        # content_changed -> re-sync the C++ GraphModel snapshot from the models.
        graph.syncNodesFromModels.assert_called_once_with(graph.nodes)
        args = renderer._cpp.renderIconsFromGraph.call_args[0]
        self.assertEqual(len(args), 10)
        self.assertIs(args[0], graph)
        self.assertIsInstance(args[4], RenderConfig)
        self.assertEqual(args[4].nodeCornerRadius, override)
        # Viewport + cull args default to off when the caller omits them.
        self.assertEqual(args[5:], (0.0, 0.0, 0.0, 0.0, False))

    def test_renderNodeIcons_forwards_viewport_and_cull_args(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer._cpp.renderIconsFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        with _patch_config({}):
            renderer.renderNodeIcons(
                graph, projection, 2.0, True, 3.0, 4.0, 800.0, 600.0, True
            )

        args = renderer._cpp.renderIconsFromGraph.call_args[0]
        # graph, proj, zoom, content_changed, config, panX, panY, vpW, vpH, cull
        self.assertEqual(args[2], 2.0)
        self.assertTrue(args[3])
        self.assertEqual(args[5:], (3.0, 4.0, 800.0, 600.0, True))

    def test_renderNodeIcons_skips_sync_when_unchanged(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer._cpp.renderIconsFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        with _patch_config({}):
            renderer.renderNodeIcons(graph, projection, 1.0, False)

        # Steady-state frame: no re-marshal, but the held buffer is still drawn.
        graph.syncNodesFromModels.assert_not_called()
        renderer._cpp.renderIconsFromGraph.assert_called_once()

    def test_renderNodeIcons_short_circuits_without_nodes(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()

        graph = self._make_graph(nodes={})
        result = renderer.renderNodeIcons(graph, mock.Mock(), 1.0, True)

        self.assertFalse(result)
        graph.syncNodesFromModels.assert_not_called()
        renderer._cpp.renderIconsFromGraph.assert_not_called()

    def test_renderNodeIcons_short_circuits_without_shader_library(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = None

        graph = self._make_graph()
        result = renderer.renderNodeIcons(graph, mock.Mock(), 1.0, True)

        self.assertFalse(result)
        renderer._cpp.renderIconsFromGraph.assert_not_called()

    def test_markIconsDirty_forwards(self):
        renderer = self._make_renderer()
        renderer.markIconsDirty()
        renderer._cpp.markIconsDirty.assert_called_once_with()

    def test_resetPositionCaches_forwards(self):
        # Pins the load-bearing lifecycle contract: the Python wrapper MUST
        # forward to the C++ `resetPositionCaches` (introduced in D109369587
        # to drop the per-node icon vertex cache in lockstep with
        # text/node-quad caches when NodeTransformFrame::reset re-baselines
        # bases). Regressing this to a no-op (or wiring it to markIconsDirty)
        # reproduces the stranded-icons bug: on `addNodesFromPrimTreeSelection`
        # after a drag, icons re-draw at the old base with a zero offset.
        renderer = self._make_renderer()
        renderer.resetPositionCaches()
        renderer._cpp.resetPositionCaches.assert_called_once_with()
        # Must not confuse the two peer wrappers: resetPositionCaches drops
        # the vertex cache; markIconsDirty only forces an assemble pass and
        # is short-circuited by the unchanged-signature `isDirty()` check.
        renderer._cpp.markIconsDirty.assert_not_called()

    def test_setTransformFrame_forwards(self):
        renderer = self._make_renderer()
        frame = object()
        renderer.setTransformFrame(frame)
        renderer._cpp.setTransformFrame.assert_called_once_with(frame)

    def test_cleanup_forwards(self):
        renderer = self._make_renderer()
        renderer.cleanup()
        renderer._cpp.cleanup.assert_called_once_with()

    def test_degrades_when_cpp_unavailable(self):
        """When the C++ extension can't be constructed, the facade is a no-op:
        isInitialized stays False and the render/lifecycle calls don't raise."""
        renderer = CppIconRenderer()
        renderer._cpp = None

        # initialize is a no-op; isInitialized stays False.
        renderer.initialize(object(), "/some/assets")
        self.assertFalse(renderer.isInitialized)

        graph = self._make_graph()
        self.assertFalse(renderer.renderNodeIcons(graph, mock.Mock(), 1.0, True))
        graph.syncNodesFromModels.assert_not_called()

        # Lifecycle forwards are guarded and must not raise.
        renderer.markIconsDirty()
        renderer.resetPositionCaches()
        renderer.setTransformFrame(object())
        renderer.cleanup()


@unittest.skipUnless(_has_icon_renderer, "pxr.UsdNoodles.iconRenderer not available")
class TestIconRenderManagerExport(unittest.TestCase):
    """Regression: the C++ IconRenderManager must be re-exported from noodles.render."""

    def test_icon_render_manager_symbol_is_resolved(self):
        # iconRenderer.py does `from pxr.UsdNoodles.render import IconRenderManager`,
        # caught silently to None on failure. If the C++ binding is registered in
        # the extension but NOT re-exported from noodles.render, that symbol is
        # None, the CppIconRenderer facade never initializes, and with useCppIcons
        # ON the icons disappear (the branch no-ops and its Python elif fallback is
        # skipped, with no error logged). Assert the symbol the facade depends on
        # actually resolved to the bound class — this is the exact failure that
        # the _cpp-mocking facade tests above could not catch.
        import pxr.UsdNoodles.iconRenderer as icon_renderer

        self.assertIsNotNone(
            icon_renderer.IconRenderManager,
            "IconRenderManager is not re-exported from noodles.render — the C++ "
            "icon path silently no-ops and icons vanish when useCppIcons is on",
        )


if __name__ == "__main__":
    unittest.main()
