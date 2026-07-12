#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for node icon resolution via the ui:nodegraph:node:icon attribute.

Verifies that the ui:nodegraph:node:icon attribute can be read from prims,
matching the resolution logic used by NodeFactory.create_node_from_prim.

Uses mock prims to avoid SIGSEGV during interpreter shutdown caused by USD
C++ static destructors when real Usd.Stage objects are created in test
binaries (see testUsdNoodlesNodeLibraries.py for the same pattern).
"""

import unittest
from unittest.mock import MagicMock

_ICON_ATTR_NAME = "ui:nodegraph:node:icon"


def _resolve_icon_path(prim):
    """Extract the icon path from a prim via the ui:nodegraph:node:icon attribute.

    Mirrors the icon resolution logic in NodeFactory.create_node_from_prim.
    """
    icon_attr = prim.GetAttribute(_ICON_ATTR_NAME)
    if icon_attr.IsValid() and icon_attr.HasAuthoredValue():
        icon_asset = icon_attr.Get()
        icon_path = icon_asset.resolvedPath or icon_asset.path
        if icon_path:
            return str(icon_path)
    return None


def _mock_prim(icon_path="", resolved_path="", authored=True, has_attr=True):
    """Create a mock prim with configurable icon attribute behavior."""
    prim = MagicMock()
    attr = MagicMock()

    if not has_attr:
        attr.IsValid.return_value = False
        prim.GetAttribute.return_value = attr
        return prim

    attr.IsValid.return_value = True
    attr.HasAuthoredValue.return_value = authored

    if authored:
        asset = MagicMock()
        asset.resolvedPath = resolved_path
        asset.path = icon_path
        attr.Get.return_value = asset

    prim.GetAttribute.return_value = attr
    return prim


class TestNodeFactoryIcon(unittest.TestCase):
    """Verify ui:nodegraph:node:icon is read from prims."""

    def test_icon_from_nodegraph_api(self):
        prim = _mock_prim(icon_path="/icons/custom.png")

        result = _resolve_icon_path(prim)
        self.assertEqual(result, "/icons/custom.png")

    def test_no_icon_authored(self):
        prim = _mock_prim(has_attr=False)

        result = _resolve_icon_path(prim)
        self.assertIsNone(result)

    def test_icon_empty_path_not_set(self):
        prim = _mock_prim(icon_path="", resolved_path="")

        result = _resolve_icon_path(prim)
        self.assertIsNone(result)

    def test_icon_attr_created_but_no_value(self):
        prim = _mock_prim(authored=False)

        result = _resolve_icon_path(prim)
        self.assertIsNone(result)


if __name__ == "__main__":
    unittest.main()
