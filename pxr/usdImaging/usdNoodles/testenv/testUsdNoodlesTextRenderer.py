#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for the TextRenderer facade over the C++ TextRenderManager.

These cover the Python-side behavior (config bridging, selection depth-patch
fallback, and the empty-input short circuit) by mocking the C++ ``_cpp`` object,
so they need no GL context.
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
    from pxr.UsdNoodles.textRenderer import TextRenderer

    _has_text_renderer = True
except ImportError as e:
    print(f"pxr.UsdNoodles.textRenderer import failed: {e}")
    _has_text_renderer = False

_has_pxr = _has_render_config and _has_text_renderer


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
class TestTextRendererNodeText(unittest.TestCase):
    def _make_renderer(self):
        renderer = TextRenderer()
        renderer._cpp = mock.Mock()
        return renderer

    def _make_graph(self, nodes=None):
        graph = mock.Mock()
        graph.nodes = {"a": object()} if nodes is None else nodes
        return graph

    def test_renderNodeText_syncs_and_passes_build_render_config(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer.font_atlas = object()
        renderer._cpp.renderNodeTextFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        # An override proves the passed RenderConfig came from build_render_config()
        # (a regression to a bare RenderConfig() would carry the struct default).
        override = RenderConfig().nodeCornerRadius + 7.0
        with _patch_config({"nodeCornerRadius": override}):
            result = renderer.renderNodeText(graph, projection, 1.0, True)

        self.assertTrue(result)
        # text_changed -> re-sync the C++ GraphModel snapshot from the models.
        graph.syncNodesFromModels.assert_called_once_with(graph.nodes)
        args = renderer._cpp.renderNodeTextFromGraph.call_args[0]
        self.assertEqual(len(args), 10)
        self.assertIs(args[0], graph)
        self.assertIsInstance(args[4], RenderConfig)
        self.assertEqual(args[4].nodeCornerRadius, override)
        # Viewport + cull args default to off when the caller omits them.
        self.assertEqual(args[5:], (0.0, 0.0, 0.0, 0.0, False))

    def test_renderNodeText_skips_sync_when_unchanged(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer.font_atlas = object()
        renderer._cpp.needsTextRebuild.return_value = False
        renderer._cpp.renderNodeTextFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        with _patch_config({}):
            renderer.renderNodeText(graph, projection, 1.0, False)

        # Steady-state frame: no re-marshal, but the held buffer is still drawn.
        graph.syncNodesFromModels.assert_not_called()
        renderer._cpp.renderNodeTextFromGraph.assert_called_once()

    def test_renderNodeText_syncs_on_pending_rebuild_without_text_changed(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer.font_atlas = object()
        renderer._cpp.needsTextRebuild.return_value = True
        renderer._cpp.renderNodeTextFromGraph.return_value = True

        graph = self._make_graph()
        projection = mock.Mock()
        projection.data.return_value = [0.0] * 16

        with _patch_config({}):
            renderer.renderNodeText(graph, projection, 1.0, False)

        # A pending C++ rebuild (e.g. the patchNodeTextDepth fallback) must re-sync
        # even though text_changed is False, or the rebuild reads stale node data.
        graph.syncNodesFromModels.assert_called_once_with(graph.nodes)

    def test_renderNodeText_short_circuits_without_nodes(self):
        renderer = self._make_renderer()
        renderer.shaderLibrary = object()
        renderer.font_atlas = object()

        graph = self._make_graph(nodes={})
        result = renderer.renderNodeText(graph, mock.Mock(), 1.0, True)

        self.assertFalse(result)
        graph.syncNodesFromModels.assert_not_called()
        renderer._cpp.renderNodeTextFromGraph.assert_not_called()

    def test_patchNodeTextDepth_patches_when_slice_present(self):
        renderer = self._make_renderer()
        renderer._cpp.patchNodeTextDepth.return_value = True
        node = mock.Mock(id="n1", zOrder=5)

        renderer.patchNodeTextDepth(node)

        renderer._cpp.patchNodeTextDepth.assert_called_once_with("n1", 5)
        renderer._cpp.markNodeTextDirty.assert_not_called()

    def test_patchNodeTextDepth_falls_back_to_rebuild(self):
        renderer = self._make_renderer()
        renderer._cpp.patchNodeTextDepth.return_value = False
        node = mock.Mock(id="n2", zOrder=7)

        renderer.patchNodeTextDepth(node)

        renderer._cpp.patchNodeTextDepth.assert_called_once_with("n2", 7)
        renderer._cpp.markNodeTextDirty.assert_called_once_with(True)


if __name__ == "__main__":
    unittest.main()
