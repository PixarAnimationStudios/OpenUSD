#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Regression test for the vertical row-layout DRIFT bug, now FIXED by the single
C++ layout producer.

On a node with many inputs, the row background STRIPES (drawn from
``inputRowSlots`` + ``layoutPortStartY`` + ``layoutPortLineHeight``) used to
drift away from the text labels and port circles (drawn from the cached
``layoutInputCenterY``). They only agreed right after a sizing pass, because the
per-pin centers are derived from the slots::

    layoutInputCenterY[i] = layoutPortStartY
                            + inputRowSlots[i] * layoutPortLineHeight
                            + 0.5 * layoutPortLineHeight

The old split — Python produced the slots, C++ produced the centers, at
different times — let them desync after any relayout (pin add/remove, fold,
font-size change), and the drift grew with the row index.

``GraphModel.layoutNode`` now produces the slots AND the centers in one atomic
pass, so they can never desync. This test drives the REAL model path
(``NodeModel`` setters/orchestration) plus ``layoutNode`` (the same call the
render chokepoint makes) and asserts ZERO per-row drift — fresh AND after the
mutations that used to cause drift. It needs no GL, so it runs anywhere.
"""

import unittest

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles
    from pxr.UsdNoodles.core import FontMetrics, RenderConfig

    _has_core = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"noodles.core import failed: {e}")
    _has_core = False

try:
    from pxr.UsdNoodles.models import NodeModel

    _has_models = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"pxr.UsdNoodles.models import failed: {e}")
    _has_models = False


def _text_width(text, size):
    return len(text) * size * 0.5


def _font_metrics():
    fm = FontMetrics()
    fm.ascender = 0.8
    fm.descender = -0.2
    fm.lineHeight = 1.2
    return fm


def _assert_zero_drift(test, node, *, side="input"):
    """Every pin's cached center must sit exactly at its stripe band center."""
    start = node.layoutPortStartY
    line = node.layoutPortLineHeight
    if side == "input":
        slots = list(node.inputRowSlots)
        centers = list(node.layoutInputCenterY)
    else:
        slots = list(node.outputRowSlots)
        centers = list(node.layoutOutputCenterY)
    test.assertEqual(
        len(slots), len(centers), f"{side}: slot/center count mismatch (stale cache)"
    )
    for i, (slot, center) in enumerate(zip(slots, centers)):
        stripe_center = start + slot * line + line * 0.5
        test.assertAlmostEqual(
            center,
            stripe_center,
            places=6,
            msg=f"{side} row {i} (slot {slot}) drifted: "
            f"center={center} stripe={stripe_center}",
        )


@unittest.skipUnless(_has_core, "noodles.core not available")
class TestLayoutNodeNoDrift(unittest.TestCase):
    """layoutNode keeps centers aligned to the stripe grid through mutations."""

    def test_fresh_many_inputs_aligned(self):
        model = _noodles.GraphModel()
        node = NodeModel() if _has_models else _noodles.NodeData()
        node.name = "ManyInputNode"
        node.inputPins = [f"in{i}" for i in range(14)]
        node.outputPins = [f"out{i}" for i in range(2)]
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        _assert_zero_drift(self, node, side="input")
        _assert_zero_drift(self, node, side="output")

    def test_font_size_change_relayouts_without_drift(self):
        # The exact mutation that used to drift: change the pin font size and
        # re-lay-out. Centers must track the NEW line height, not stick to the
        # old one.
        model = _noodles.GraphModel()
        node = NodeModel() if _has_models else _noodles.NodeData()
        node.name = "FontNode"
        node.inputPins = [f"in{i}" for i in range(14)]

        cfg = RenderConfig()
        model.layoutNode(node, _text_width, _font_metrics(), cfg)
        _assert_zero_drift(self, node, side="input")
        line_before = node.layoutPortLineHeight

        cfg.nodePinFontSize = cfg.nodePinFontSize * 1.5
        model.layoutNode(node, _text_width, _font_metrics(), cfg)
        self.assertGreater(node.layoutPortLineHeight, line_before)
        _assert_zero_drift(self, node, side="input")  # tracks the new grid

    def test_add_inputs_relayouts_without_drift(self):
        model = _noodles.GraphModel()
        node = NodeModel() if _has_models else _noodles.NodeData()
        node.name = "GrowNode"
        node.inputPins = [f"in{i}" for i in range(8)]
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        _assert_zero_drift(self, node, side="input")

        # Add 6 more inputs the way the live editor does: a USD change calls
        # invalidateCache (resetting captured originals) so the next pin set
        # re-captures. Then re-lay-out (what the render chokepoint does). All 14
        # rows must be aligned — no stale tail.
        node._original_input_pins = None
        node.inputPins = [f"in{i}" for i in range(14)]
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        self.assertEqual(len(list(node.layoutInputCenterY)), 14)
        _assert_zero_drift(self, node, side="input")


@unittest.skipUnless(
    _has_core and _has_models, "noodles.core or pxr.UsdNoodles.models not available"
)
class TestLayoutNodeNoDriftWithGroups(unittest.TestCase):
    """Grouped/folded nodes (non-identity slots) also stay aligned."""

    def _grouped_node(self):
        node = NodeModel()
        node.name = "GroupNode"
        ordered = [
            ("input", "a"),
            ("input", "grp:x"),
            ("input", "grp:y"),
            ("input", "grp:z"),
            ("input", "b"),
        ]
        node.set_ordered_pin_entries(ordered)
        node.inputPins = ["a", "grp:x", "grp:y", "grp:z", "b"]
        return node

    def test_unfolded_group_aligned(self):
        model = _noodles.GraphModel()
        node = self._grouped_node()
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        _assert_zero_drift(self, node, side="input")

    def test_fold_then_relayout_aligned(self):
        model = _noodles.GraphModel()
        node = self._grouped_node()
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        # Fold the group (changes slots), then re-lay-out: still aligned.
        node.toggle_fold("grp")
        model.layoutNode(node, _text_width, _font_metrics(), RenderConfig())
        _assert_zero_drift(self, node, side="input")


if __name__ == "__main__":
    unittest.main()
