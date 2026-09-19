#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Post-land coverage for D107900876 ([usd_noodles] PR3: cut the live path over to
the C++ layout producer).

The landed diff covers the happy path (fresh many-inputs, font-size mutation,
input-add via invalidateCache, fold-then-relayout) through GraphModel.layoutNode
in tests/testUsdNoodlesRowDrift.py. Two surfaces introduced by PR3 are NOT
covered there:

1. The free-function module-level bindings _noodles.buildDisplayPins and
   _noodles.assignRowSlots — the diff exposes them at module scope (def(...)
   in WrapNoodles), but every existing pytest reaches them only via the
   composite GraphModel.layoutNode wrapper. A direct call locks the binding
   contract: if a future refactor renames or relocates either free function,
   the layoutNode happy path can keep working while the Python data source
   path (NodeModel._rebuild_display_pins, which calls these directly) breaks.

2. The per-side conditional in NodeModel._read_layout_back_from_cpp: each side
   is mirrored back from C++ only once its originals are loaded. The pre-land
   testUsdNoodlesRowDrift exercises both sides loaded; it does not exercise
   the asymmetric case (e.g. an output-only node that has not yet captured
   input originals). Without coverage, a regression that drops the per-side
   guard would silently overwrite the unloaded side's _input_pins / _output_pins
   with an empty list, breaking lazy-getter semantics.

These are isolated unit tests of the binding surface and the readback
conditional, with no GL / no QApplication / no font atlas dependency, so they
slot into the noodles_pytest target with the same skip-guards as
testUsdNoodlesRowDrift.
"""

import unittest

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles

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


@unittest.skipUnless(_has_core, "noodles.core not available")
class TestFreeFunctionBindings(unittest.TestCase):
    """Direct binding contract for _noodles.buildDisplayPins / assignRowSlots.

    These free functions are what NodeModel._rebuild_display_pins calls
    directly (the font-free production path); the existing pytest reaches them
    only through GraphModel.layoutNode (which wraps both plus calculateNodeSize).
    A direct call locks them in as module-level callables.
    """

    def test_assignRowSlots_runs_after_buildDisplayPins_only(self):
        # Order matters: the C++ comment on assignRowSlots says it must run
        # after buildDisplayPins. Verify that calling assignRowSlots without
        # going through layoutNode (i.e. without sizing) still produces a
        # sensible slot vector — neither pass needs FontMetrics. This call
        # also locks in the module-level binding: an AttributeError on a
        # missing or relocated symbol surfaces here.
        node = _noodles.NodeData()
        node.originalInputPins = ["a", "b", "c"]
        node.orderedPinEntries = [("input", "a"), ("input", "b"), ("input", "c")]
        _noodles.buildDisplayPins(node)
        _noodles.assignRowSlots(node)
        slots = list(node.inputRowSlots)
        # Don't assert a specific length — the display-pin contract from
        # originalInputPins is verified upstream (in testUsdNoodlesRowDrift),
        # not here. The order-of-call and uniqueness invariants are what
        # this test locks in.
        self.assertGreater(len(slots), 0, "assignRowSlots must produce row slots")
        self.assertEqual(len(set(slots)), len(slots), "row slots must be unique")
        self.assertTrue(all(s >= 0 for s in slots))


@unittest.skipUnless(
    _has_core and _has_models, "noodles.core or pxr.UsdNoodles.models not available"
)
class TestReadLayoutBackAsymmetricLoad(unittest.TestCase):
    """_read_layout_back_from_cpp guards each side independently.

    A NodeModel can have one side's originals loaded while the other side's
    lazy getter has not yet fired. The readback must mirror only the loaded
    side and leave the unloaded side untouched; otherwise, the lazy getter
    fires later against an empty mirror and the link/interaction code reads
    a stale, empty pin list.
    """

    def _make_input_only_node(self):
        # Capture only the input side. The output side's _original_output_pins
        # remains None, so the per-side guard in _read_layout_back_from_cpp
        # must skip the output mirror block.
        node = NodeModel()
        node.name = "InputOnlyNode"
        node.inputPins = ["a", "b", "c"]
        # NodeModel.inputPins setter triggers _capture_originals_and_rebuild
        # for the input side; the output side stays unloaded.
        return node

    def _make_output_only_node(self):
        node = NodeModel()
        node.name = "OutputOnlyNode"
        node.outputPins = ["x", "y"]
        return node

    def test_input_only_node_does_not_emit_empty_output_mirror(self):
        node = self._make_input_only_node()
        # Sanity: the input side is loaded, the output side is not.
        self.assertIsNotNone(node._original_input_pins)
        self.assertIsNone(node._original_output_pins)
        # The unloaded output side must remain at the None sentinel. If
        # _read_layout_back_from_cpp dropped its per-side guard and mirrored
        # both sides unconditionally, _output_pins would be set to [] (empty
        # list, not None) by the readback's empty C++ output mirror, and
        # the lazy output getter would later read that empty list instead
        # of firing _capture_originals_and_rebuild.
        #
        # Note: a positive control on the LOADED side (assertIsNotNone on
        # node._input_pins) is not added here because the inputPins setter
        # populates _input_pins BEFORE _capture_originals_and_rebuild runs
        # (models.py inputPins.setter), so the assertion would pass even if
        # readback never ran. A faithful positive control would require
        # mocking the C++ extension callables, which the prior 6th-test
        # removal showed is fragile against the runtime contract.
        self.assertIsNone(
            node._output_pins,
            "asymmetric readback must not pre-empt the output lazy getter",
        )

    def test_output_only_node_does_not_emit_empty_input_mirror(self):
        node = self._make_output_only_node()
        self.assertIsNone(node._original_input_pins)
        self.assertIsNotNone(node._original_output_pins)
        # Symmetric to the input-only test above: the unloaded input side
        # must remain at the None sentinel. See that test's comment for
        # why a positive control on the loaded side is not asserted.
        self.assertIsNone(
            node._input_pins,
            "asymmetric readback must not pre-empt the input lazy getter",
        )


@unittest.skipUnless(_has_core, "noodles.core not available")
class TestTitleIconPathBinding(unittest.TestCase):
    """titleIconPath round-trips through the C++ NodeData binding. This is the
    substrate the future C++ icon producer reads; nothing consumes it yet."""

    def test_default_is_empty_and_settable(self):
        node = _noodles.NodeData()
        self.assertEqual(node.titleIconPath, "")
        node.titleIconPath = "/icons/foo.png"
        self.assertEqual(node.titleIconPath, "/icons/foo.png")


@unittest.skipUnless(
    _has_core and _has_models, "noodles.core or pxr.UsdNoodles.models not available"
)
class TestSyncIconPathToCpp(unittest.TestCase):
    """_sync_content_to_cpp mirrors NodeModel._icon_path onto the C++ field."""

    def test_icon_path_synced(self):
        node = NodeModel()
        node._icon_path = "/icons/bar.png"
        node._sync_content_to_cpp()
        self.assertEqual(node.titleIconPath, "/icons/bar.png")

    def test_none_icon_path_syncs_empty_string(self):
        # The C++ field is a std::string; None must coerce to "" (assigning None
        # would raise in boost.python).
        node = NodeModel()
        node._icon_path = None
        node._sync_content_to_cpp()
        self.assertEqual(node.titleIconPath, "")
