#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for NodeGraph attribute contracts.

GraphView delegates several properties to NodeGraph (nodes, links,
groupStickers, linksChanged). If any of these attributes are missing
from a freshly constructed NodeGraph, paintGL crashes at runtime.

These tests verify the contract is maintained after refactors.
"""

import sys
import types
import unittest
from unittest import mock

try:
    from pxr.UsdNoodles.nodeGraph import NodeGraph

    _has_pxr = True
except ImportError as e:
    print(f"pxr.UsdNoodles.nodeGraph import failed: {e}")
    _has_pxr = False

try:
    from pxr.UsdNoodles.core import RenderConfig

    _has_render_config = True
except ImportError as e:
    print(f"noodles.core.RenderConfig import failed: {e}")
    _has_render_config = False

try:
    from pxr.UsdNoodles.nodeGraph import build_render_config, createNodeRenderer

    _has_helpers = True
except ImportError as e:
    print(f"pxr.UsdNoodles.nodeGraph config helpers import failed: {e}")
    _has_helpers = False

_has_config = _has_render_config and _has_helpers


def _patch_config(values):
    """Stub ``NoodlesConfig.get`` to return *values* (key -> value), else default.

    Installs a fake ``pxr.UsdNoodles.noodlesConfig`` module in ``sys.modules``
    so the inner ``from .noodlesConfig import NoodlesConfig`` in
    ``build_render_config`` picks up the stub. Importing the real module would
    pull in Qt (via ``noodlesSettings``), which is not a dep of the pytest
    target — ``mock.patch("pxr.UsdNoodles.noodlesConfig.NoodlesConfig.get")``
    would error at context-manager entry.
    """
    fake = types.ModuleType("pxr.UsdNoodles.noodlesConfig")

    class _FakeConfig:
        @classmethod
        def get(cls, key, default=None):
            return values.get(key, default)

    fake.NoodlesConfig = _FakeConfig
    return mock.patch.dict(sys.modules, {"pxr.UsdNoodles.noodlesConfig": fake})


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestNodeGraphContract(unittest.TestCase):
    """Verify NodeGraph exposes all attributes that GraphView delegates to."""

    def _make_node_graph(self):
        return NodeGraph()

    def test_has_nodes(self):
        graph = self._make_node_graph()
        self.assertIsInstance(graph.nodes, dict)

    def test_has_links(self):
        graph = self._make_node_graph()
        self.assertIsInstance(graph.links, list)

    def test_has_group_stickers(self):
        graph = self._make_node_graph()
        result = graph.groupStickers
        self.assertIsInstance(result, list)

    def test_has_links_changed(self):
        graph = self._make_node_graph()
        val = graph.linksChanged
        self.assertIsInstance(val, bool)

    def test_links_changed_settable(self):
        graph = self._make_node_graph()
        graph.linksChanged = False
        self.assertFalse(graph.linksChanged)
        graph.linksChanged = True
        self.assertTrue(graph.linksChanged)

    def test_has_stickers(self):
        graph = self._make_node_graph()
        self.assertIsInstance(graph.stickers, list)

    def test_default_links_changed_is_true(self):
        graph = self._make_node_graph()
        self.assertTrue(graph.linksChanged)


class TestGraphModelBindings(unittest.TestCase):
    """Verify C++ GraphModel bindings expose required methods."""

    def _make_graph_model(self):
        from pxr.UsdNoodles import _usdNoodles as _noodles

        return _noodles.GraphModel()

    def test_is_links_changed_default(self):
        model = self._make_graph_model()
        self.assertTrue(model.isLinksChanged())

    def test_mark_links_changed(self):
        model = self._make_graph_model()
        model.buildConnectionCache()
        self.assertFalse(model.isLinksChanged())
        model.markLinksChanged()
        self.assertTrue(model.isLinksChanged())

    def test_clear_resets_links_changed(self):
        model = self._make_graph_model()
        model.buildConnectionCache()
        self.assertFalse(model.isLinksChanged())
        model.clear()
        self.assertTrue(model.isLinksChanged())

    def test_link_data_is_relationship_field(self):
        # is_relationship_link is now a real bound C++ field (LinkData::
        # isRelationship), not a dynamic attribute; it must round-trip so the
        # render-snapshot sync can carry the regular/relationship classification.
        from pxr.UsdNoodles import _usdNoodles as _noodles

        link = _noodles.LinkData()
        self.assertFalse(link.is_relationship_link)
        link.is_relationship_link = True
        self.assertTrue(link.is_relationship_link)

    def test_get_link_count_default_zero(self):
        model = self._make_graph_model()
        self.assertEqual(model.getLinkCount(), 0)

    def test_sync_links_from_models_populates_held_vector(self):
        from pxr.UsdNoodles import _usdNoodles as _noodles

        model = self._make_graph_model()
        a = _noodles.LinkData()
        a.sourceNodeId = "a"
        a.targetNodeId = "b"
        a.is_relationship_link = True
        model.syncLinksFromModels([a, _noodles.LinkData()])
        self.assertEqual(model.getLinkCount(), 2)

    def test_sync_links_from_models_replaces_previous(self):
        from pxr.UsdNoodles import _usdNoodles as _noodles

        model = self._make_graph_model()
        model.syncLinksFromModels([_noodles.LinkData(), _noodles.LinkData()])
        self.assertEqual(model.getLinkCount(), 2)
        model.syncLinksFromModels([_noodles.LinkData()])
        self.assertEqual(model.getLinkCount(), 1)
        model.syncLinksFromModels([])
        self.assertEqual(model.getLinkCount(), 0)

    def test_content_fields_roundtrip(self):
        # The raw USD-derived content the C++ layout producer will consume must
        # round-trip through the new bindings (originals, fold state, ordered
        # entries, direction-group membership, dual pins).
        from pxr.UsdNoodles import _usdNoodles as _noodles

        node = _noodles.NodeData()
        # Plain std::vector<string> fields use USD's _tf to_python converter,
        # which isn't registered in this minimal _noodles-only context (same as
        # the existing inputPins, which is only ever set here, not read). Just
        # confirm assignment is accepted; the producer reads these in C++ tests.
        node.originalInputPins = ["a", "b"]
        node.originalOutputPins = ["c"]

        node.originalInputPinTypes["a"] = "float"
        self.assertEqual(node.originalInputPinTypes["a"], "float")

        node.foldState = {"In": True, "Out": False}
        fs = node.foldState
        self.assertEqual(fs["In"], True)
        self.assertEqual(fs["Out"], False)

        node.orderedPinEntries = [("input", "a"), ("output", "c")]
        self.assertEqual(
            [tuple(e) for e in node.orderedPinEntries],
            [("input", "a"), ("output", "c")],
        )

        node.inputDirectionGroupPins = ["a"]
        node.outputDirectionGroupPins = ["c"]
        node.dualPinNames = ["a"]
        self.assertEqual(set(node.inputDirectionGroupPins), {"a"})
        self.assertEqual(set(node.outputDirectionGroupPins), {"c"})
        self.assertEqual(set(node.dualPinNames), {"a"})

    def test_calculate_node_size_exposes_per_pin_center_y(self):
        # The per-pin row centers are the single source of truth that Python
        # hit-tests read; verify they are exposed and honor authored row slots.
        from pxr.UsdNoodles import _usdNoodles as _noodles

        model = self._make_graph_model()
        node = _noodles.NodeData()
        node.inputPins = ["a", "b"]
        node.inputRowSlots = [0, 2]
        node.outputPins = ["c"]
        node.outputRowSlots = [1]
        fm = _noodles.FontMetrics()
        fm.ascender = 0.8
        fm.descender = -0.2
        fm.lineHeight = 1.2
        model.calculateNodeSize(
            node, lambda text, size: len(text) * size * 0.5, fm, _noodles.RenderConfig()
        )

        s = node.layoutPortStartY
        line = node.layoutPortLineHeight
        self.assertEqual(len(node.layoutInputCenterY), 2)
        self.assertAlmostEqual(node.layoutInputCenterY[0], s + 0 * line + line * 0.5)
        self.assertAlmostEqual(node.layoutInputCenterY[1], s + 2 * line + line * 0.5)
        self.assertEqual(len(node.layoutOutputCenterY), 1)
        self.assertAlmostEqual(node.layoutOutputCenterY[0], s + 1 * line + line * 0.5)


@unittest.skipUnless(_has_config, "pxr.UsdNoodles config helpers not available")
class TestBuildRenderConfig(unittest.TestCase):
    """Verify the NoodlesConfig -> RenderConfig bridge used by sizing, the
    renderer, and text layout."""

    # Every key bridged by build_render_config; the no-op contract must hold
    # for all of them, including the appearance keys newly bridged by this diff.
    _BRIDGED_KEYS = (
        "nodeTitleFontSize",
        "nodePinFontSize",
        "nodePinTypeFontSize",
        "showPinTypeLabels",
        "nodeMarginH",
        "nodeMarginV",
        "nodePortSpacing",
        "nodePortWidth",
        "nodePortRadius",
        "nodeFontSize",
        "nodeCornerRadius",
        "selectedNodeStrokeWidth",
        "nodeBgHigh",
        "nodeBgLow",
        "nodeBgAlpha",
        "nodeShadowFactor",
        "nodeTypeSaturation",
        "nodeTypeBrightness",
    )

    def test_noop_on_default_config(self):
        # With no overrides, build_render_config must equal RenderConfig()'s
        # struct defaults so appearance is unchanged on an unmodified config.
        defaults = RenderConfig()
        with _patch_config({}):
            cfg = build_render_config()
        for key in self._BRIDGED_KEYS:
            self.assertEqual(
                getattr(cfg, key),
                getattr(defaults, key),
                f"{key} drifted from RenderConfig() default on unmodified config",
            )

    def test_override_applied(self):
        with _patch_config({"nodeCornerRadius": 24.0}):
            cfg = build_render_config()
        self.assertEqual(cfg.nodeCornerRadius, 24.0)

    def test_show_pin_type_labels_override(self):
        # The bool toggle bridges through to the C++ RenderConfig (and coerces to
        # bool), so hiding pin-type labels reaches sizing + text layout.
        with _patch_config({"showPinTypeLabels": False}):
            cfg = build_render_config()
        self.assertFalse(cfg.showPinTypeLabels)
        self.assertIsInstance(cfg.showPinTypeLabels, bool)

    def test_value_coerced_to_field_type(self):
        # nodeBgHigh is an integer field; a float setting is coerced to its type
        # so boost.python doesn't reject it.
        field_type = type(RenderConfig().nodeBgHigh)
        with _patch_config({"nodeBgHigh": 200.0}):
            cfg = build_render_config()
        self.assertEqual(cfg.nodeBgHigh, 200)
        self.assertIsInstance(cfg.nodeBgHigh, field_type)

    def test_bad_value_skipped_not_raised(self):
        # A non-numeric value for a numeric field must not raise; the field keeps
        # its default (exercises the error-handling branch).
        default_radius = RenderConfig().nodeCornerRadius
        with _patch_config({"nodeCornerRadius": "not-a-number"}):
            cfg = build_render_config()  # must not raise
        self.assertEqual(cfg.nodeCornerRadius, default_radius)


@unittest.skipUnless(_has_config, "pxr.UsdNoodles config helpers not available")
class TestCreateNodeRenderer(unittest.TestCase):
    """Verify the node renderer is constructed with the NoodlesConfig-derived
    RenderConfig, with a back-compat fallback for legacy plugin renderers."""

    def test_passes_render_config_to_renderer(self):
        captured = {}

        class FakeRenderer:
            def __init__(self, config=None):
                captured["config"] = config

        with (
            mock.patch(
                "pxr.UsdNoodles.nodeGraph._get_renderer_registry",
                return_value={"default": FakeRenderer},
            ),
            _patch_config({}),
        ):
            createNodeRenderer("default")
        self.assertIsInstance(captured["config"], RenderConfig)

    def test_falls_back_to_no_arg_for_legacy_renderer(self):
        calls = []

        class LegacyRenderer:
            def __init__(self):  # rejects a RenderConfig argument
                calls.append("noarg")

        with (
            mock.patch(
                "pxr.UsdNoodles.nodeGraph._get_renderer_registry",
                return_value={"default": LegacyRenderer},
            ),
            _patch_config({}),
        ):
            renderer = createNodeRenderer("default")
        self.assertIsInstance(renderer, LegacyRenderer)
        self.assertEqual(calls, ["noarg"])


if __name__ == "__main__":
    unittest.main()
