#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for cross-side namespaced-group unification in the C++ layout
producer (buildDisplayPins + assignRowSlots).

A namespace like ``skel`` can head a group on BOTH edges of a node at once: its
dual attributes (``skel:joints``, ``skel:blendShapes``) surface as input-side
rows, while its relationships (``skel:animationSource``, ``skel:skeleton``)
surface as output-side rows. Historically that produced TWO ``skel`` group
headers (one per side) that folded together (shared ``foldState`` key) but
rendered as two separate rows.

These tests lock in that such a namespace now occupies a SINGLE shared header
row: the input-side header and the output-side header map to the same display
row slot, and the merged ``displayRowKinds`` carries one header entry, not two.
Both per-side header rows still exist so a folded group keeps its aggregate port
on each edge that carries connections. Direction groups (``inputs``/``outputs``)
are one-sided and never share.

No GL / QApplication / font atlas dependency, so this slots into noodles_pytest
with the same skip guard as testUsdNoodlesRowDrift.
"""

import unittest

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles

    _has_core = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"noodles.core import failed: {e}")
    _has_core = False

try:
    # Importing the models module registers the boost.python
    # std::vector<std::string> <-> list converter that the NodeData pin-list
    # setters rely on; without it, assigning a Python list to
    # originalInputPins/originalOutputPins raises Boost.Python.ArgumentError.
    from pxr.UsdNoodles.models import NodeModel  # noqa: F401

    _has_models = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"pxr.UsdNoodles.models import failed: {e}")
    _has_models = False


def _header_slot(pins, row_kinds, row_slots, group_name):
    """Return the row slot of the group-header row named *group_name*.

    A header row is a kind-1 (folded) or kind-2 (unfolded) row whose display
    pin equals the bare prefix. Returns None if no such header exists.
    """
    for idx, pin in enumerate(pins):
        kind = row_kinds[idx] if idx < len(row_kinds) else 0
        if pin == group_name and kind in (1, 2):
            return row_slots[idx] if idx < len(row_slots) else idx
    return None


@unittest.skipUnless(
    _has_core and _has_models, "noodles.core or pxr.UsdNoodles.models not available"
)
class TestUnifiedCrossSideGroup(unittest.TestCase):
    """A namespace present on both edges shares one header row."""

    def _build(self, fold_skel=False):
        node = _noodles.NodeData()
        node.originalInputPins = ["skel:joints", "skel:blendShapes"]
        node.originalOutputPins = ["skel:animationSource", "skel:skeleton"]
        # Property order interleaves the two sides, matching composed USD order.
        node.orderedPinEntries = [
            ("output", "skel:animationSource"),
            ("input", "skel:joints"),
            ("output", "skel:skeleton"),
            ("input", "skel:blendShapes"),
        ]
        if fold_skel:
            node.foldState = {"skel": True}
        _noodles.buildDisplayPins(node)
        _noodles.assignRowSlots(node)
        return node

    def test_both_sides_produce_a_skel_header(self):
        node = self._build()
        in_slot = _header_slot(
            list(node.displayInputPins),
            list(node.inputRowKinds),
            list(node.inputRowSlots),
            "skel",
        )
        out_slot = _header_slot(
            list(node.displayOutputPins),
            list(node.outputRowKinds),
            list(node.outputRowSlots),
            "skel",
        )
        self.assertIsNotNone(in_slot, "input side must have a skel header")
        self.assertIsNotNone(out_slot, "output side must have a skel header")

    def test_headers_share_one_row_slot(self):
        node = self._build()
        in_slot = _header_slot(
            list(node.displayInputPins),
            list(node.inputRowKinds),
            list(node.inputRowSlots),
            "skel",
        )
        out_slot = _header_slot(
            list(node.displayOutputPins),
            list(node.outputRowKinds),
            list(node.outputRowSlots),
            "skel",
        )
        self.assertEqual(
            in_slot,
            out_slot,
            "the skel group must occupy ONE shared row across both edges",
        )

    def test_merged_rows_have_exactly_one_header(self):
        node = self._build()
        kinds = list(node.displayRowKinds)
        header_rows = [k for k in kinds if k in (1, 2)]
        self.assertEqual(
            len(header_rows),
            1,
            f"expected a single merged skel header, got kinds={kinds}",
        )
        # 1 header + 2 input children + 2 output children = 5 rows (not 6).
        self.assertEqual(len(kinds), 5, f"expected 5 merged rows, got {kinds}")

    def test_folded_group_is_one_shared_header_row(self):
        node = self._build(fold_skel=True)
        kinds = list(node.displayRowKinds)
        # Folded: children hidden, only the single shared header row remains.
        self.assertEqual(kinds, [1], f"folded skel group must be one row, got {kinds}")
        in_slot = _header_slot(
            list(node.displayInputPins),
            list(node.inputRowKinds),
            list(node.inputRowSlots),
            "skel",
        )
        out_slot = _header_slot(
            list(node.displayOutputPins),
            list(node.outputRowKinds),
            list(node.outputRowSlots),
            "skel",
        )
        self.assertEqual(in_slot, 0)
        self.assertEqual(out_slot, 0)


@unittest.skipUnless(
    _has_core and _has_models, "noodles.core or pxr.UsdNoodles.models not available"
)
class TestOneSidedGroupUnchanged(unittest.TestCase):
    """A namespace on a single edge keeps exactly one header (no regression)."""

    def test_output_only_group_has_single_header(self):
        node = _noodles.NodeData()
        node.originalOutputPins = ["skel:animationSource", "skel:skeleton"]
        node.orderedPinEntries = [
            ("output", "skel:animationSource"),
            ("output", "skel:skeleton"),
        ]
        _noodles.buildDisplayPins(node)
        _noodles.assignRowSlots(node)
        kinds = list(node.displayRowKinds)
        header_rows = [k for k in kinds if k in (1, 2)]
        self.assertEqual(len(header_rows), 1)
        # 1 header + 2 children.
        self.assertEqual(len(kinds), 3, f"expected 3 rows, got {kinds}")
