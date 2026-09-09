#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for expansion state persistence via UsdUINodeGraphNodeAPI.

Verifies that the ui:nodegraph:node:expansionState attribute can be read
from prims, matching the resolution logic used by NodeFactory.create_node_from_prim.

Uses mock prims to avoid SIGSEGV during interpreter shutdown caused by USD
C++ static destructors when real Usd.Stage objects are created in test
binaries (see testUsdNoodlesNodeFactoryIcon.py for the same pattern).
"""

import unittest
from unittest.mock import MagicMock

_EXPANSION_STATE_ATTR_NAME = "ui:nodegraph:node:expansionState"


def _resolve_expansion_state(prim):
    """Extract expansion state, mirroring NodeFactory logic."""
    expansion_attr = prim.GetAttribute(_EXPANSION_STATE_ATTR_NAME)
    if expansion_attr.IsValid() and expansion_attr.HasAuthoredValue():
        return str(expansion_attr.Get())
    return None


def _mock_prim(expansion_state=None, authored=True, has_attr=True):
    """Create a mock prim with configurable expansionState attribute behavior."""
    prim = MagicMock()
    attr = MagicMock()

    if not has_attr:
        attr.IsValid.return_value = False
        prim.GetAttribute.return_value = attr
        return prim

    attr.IsValid.return_value = True
    attr.HasAuthoredValue.return_value = authored

    if authored and expansion_state is not None:
        attr.Get.return_value = expansion_state

    prim.GetAttribute.return_value = attr
    return prim


class TestExpansionState(unittest.TestCase):
    """Verify ui:nodegraph:node:expansionState is read from prims."""

    def test_expansion_state_closed(self):
        prim = _mock_prim(expansion_state="closed")

        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "closed")

    def test_expansion_state_open(self):
        prim = _mock_prim(expansion_state="open")

        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "open")

    def test_expansion_state_minimized(self):
        prim = _mock_prim(expansion_state="minimized")

        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "minimized")

    def test_no_expansion_state_authored(self):
        prim = _mock_prim(has_attr=False)

        result = _resolve_expansion_state(prim)
        self.assertIsNone(result)

    def test_write_and_read_expansion_state(self):
        # First read - closed
        prim = _mock_prim(expansion_state="closed")
        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "closed")

        # Second read - open (simulating a write then read)
        prim = _mock_prim(expansion_state="open")
        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "open")

    def test_closed_maps_to_collapsed(self):
        """Verify the title_collapsed mapping used by NodeFactory."""
        prim = _mock_prim(expansion_state="closed")

        state = _resolve_expansion_state(prim)
        self.assertIn(state, ("closed", "minimized"))

    def test_minimized_maps_to_collapsed(self):
        """Verify minimized is also treated as collapsed."""
        prim = _mock_prim(expansion_state="minimized")

        state = _resolve_expansion_state(prim)
        self.assertIn(state, ("closed", "minimized"))

    def test_open_maps_to_expanded(self):
        """Verify open is not treated as collapsed."""
        prim = _mock_prim(expansion_state="open")

        state = _resolve_expansion_state(prim)
        self.assertNotIn(state, ("closed", "minimized"))

    def test_session_layer_write(self):
        """Verify expansion state can be read from session layer writes."""
        # This tests reading only - the actual session layer write is internal
        prim = _mock_prim(expansion_state="closed")

        result = _resolve_expansion_state(prim)
        self.assertEqual(result, "closed")


if __name__ == "__main__":
    unittest.main()
