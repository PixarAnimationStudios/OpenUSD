#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for display color resolution via UsdUINodeGraphNodeAPI.

Verifies that the ui:nodegraph:node:displayColor attribute can be read from prims,
matching the resolution logic used by NodeFactory.create_node_from_prim.

Uses mock prims to avoid SIGSEGV during interpreter shutdown caused by USD
C++ static destructors when real Usd.Stage objects are created in test
binaries (see testUsdNoodlesNodeFactoryIcon.py for the same pattern).
"""

import unittest
from unittest.mock import MagicMock

_DISPLAY_COLOR_ATTR_NAME = "ui:nodegraph:node:displayColor"


def _resolve_display_color(prim):
    """Extract display color, mirroring NodeFactory logic."""
    color_attr = prim.GetAttribute(_DISPLAY_COLOR_ATTR_NAME)
    if color_attr.IsValid() and color_attr.HasAuthoredValue():
        color = color_attr.Get()
        if color is not None:
            return (color[0], color[1], color[2])
    return None


def _mock_prim(color=None, authored=True, has_attr=True):
    """Create a mock prim with configurable displayColor attribute behavior."""
    prim = MagicMock()
    attr = MagicMock()

    if not has_attr:
        attr.IsValid.return_value = False
        prim.GetAttribute.return_value = attr
        return prim

    attr.IsValid.return_value = True
    attr.HasAuthoredValue.return_value = authored

    if authored and color is not None:
        # Mock color as a sequence type (like Gf.Vec3f)
        mock_color = MagicMock()
        mock_color.__getitem__ = lambda self, i: color[i]
        attr.Get.return_value = mock_color

    prim.GetAttribute.return_value = attr
    return prim


class TestDisplayColor(unittest.TestCase):
    """Verify ui:nodegraph:node:displayColor is read from prims."""

    def test_display_color_authored(self):
        prim = _mock_prim(color=(1.0, 0.0, 0.0))

        result = _resolve_display_color(prim)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result[0], 1.0)
        self.assertAlmostEqual(result[1], 0.0)
        self.assertAlmostEqual(result[2], 0.0)

    def test_no_display_color_authored(self):
        prim = _mock_prim(has_attr=False)

        result = _resolve_display_color(prim)
        self.assertIsNone(result)

    def test_display_color_values(self):
        prim = _mock_prim(color=(0.5, 0.7, 0.3))

        result = _resolve_display_color(prim)
        self.assertAlmostEqual(result[0], 0.5, places=5)
        self.assertAlmostEqual(result[1], 0.7, places=5)
        self.assertAlmostEqual(result[2], 0.3, places=5)

    def test_display_color_api_applied_but_no_value(self):
        prim = _mock_prim(authored=False)

        result = _resolve_display_color(prim)
        self.assertIsNone(result)

    def test_display_color_black(self):
        prim = _mock_prim(color=(0.0, 0.0, 0.0))

        result = _resolve_display_color(prim)
        self.assertIsNotNone(result)
        self.assertAlmostEqual(result[0], 0.0)
        self.assertAlmostEqual(result[1], 0.0)
        self.assertAlmostEqual(result[2], 0.0)

    def test_display_color_white(self):
        prim = _mock_prim(color=(1.0, 1.0, 1.0))

        result = _resolve_display_color(prim)
        self.assertEqual(result, (1.0, 1.0, 1.0))


if __name__ == "__main__":
    unittest.main()
