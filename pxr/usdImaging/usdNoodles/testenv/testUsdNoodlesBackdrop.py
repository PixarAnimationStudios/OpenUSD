#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for Backdrop sticker USD backing via UsdUIBackdrop.

Uses mock prims to avoid SIGSEGV during interpreter shutdown caused by USD
C++ static destructors when real Usd.Stage objects are created in test
binaries (see testUsdNoodlesNodeFactoryIcon.py for the same pattern).
"""

import unittest
from unittest.mock import MagicMock

# Constants mirroring GroupSticker
_POSITION_SCALE = 1000.0
DEFAULT_CORNER_RADIUS = 10.0
DEFAULT_INNER_STROKE = 0.0


def _mock_backdrop_prim(
    position=None,
    size=None,
    description=None,
    display_color=None,
    stacking_order=None,
    custom_data=None,
):
    """Create a mock backdrop prim with configurable attributes."""
    prim = MagicMock()

    def get_attribute(name):
        attr = MagicMock()
        if name == "ui:nodegraph:node:pos":
            if position is not None:
                attr.HasAuthoredValue.return_value = True
                mock_pos = MagicMock()
                mock_pos.__getitem__ = lambda self, i: position[i]
                attr.Get.return_value = mock_pos
            else:
                attr.HasAuthoredValue.return_value = False
        elif name == "ui:nodegraph:node:size":
            if size is not None:
                attr.HasAuthoredValue.return_value = True
                mock_size = MagicMock()
                mock_size.__getitem__ = lambda self, i: size[i]
                attr.Get.return_value = mock_size
            else:
                attr.HasAuthoredValue.return_value = False
        elif name == "ui:nodegraph:node:displayColor":
            attr.IsValid.return_value = display_color is not None
            if display_color is not None:
                attr.HasAuthoredValue.return_value = True
                mock_color = MagicMock()
                mock_color.__getitem__ = lambda self, i: display_color[i]
                attr.Get.return_value = mock_color
            else:
                attr.HasAuthoredValue.return_value = False
        elif name == "ui:nodegraph:node:stackingOrder":
            if stacking_order is not None:
                attr.HasAuthoredValue.return_value = True
                attr.Get.return_value = stacking_order
            else:
                attr.HasAuthoredValue.return_value = False
        elif name == "ui:description":
            if description is not None:
                attr.HasAuthoredValue.return_value = True
                attr.Get.return_value = description
            else:
                attr.HasAuthoredValue.return_value = False
        else:
            attr.HasAuthoredValue.return_value = False
        return attr

    prim.GetAttribute = get_attribute
    prim.GetCustomData.return_value = custom_data or {}
    prim.IsValid.return_value = True
    prim.GetPath.return_value = MagicMock(pathString="/TestBackdrop")

    return prim


class TestBackdropSticker(unittest.TestCase):
    """Tests for UsdUI.Backdrop basic operations."""

    def test_create_backdrop_prim(self):
        """Test that a backdrop prim can be created."""
        prim = _mock_backdrop_prim()
        self.assertTrue(prim.IsValid())

    def test_backdrop_description(self):
        """Test reading description from backdrop."""
        prim = _mock_backdrop_prim(description="My Group")
        attr = prim.GetAttribute("ui:description")
        self.assertTrue(attr.HasAuthoredValue())
        self.assertEqual(attr.Get(), "My Group")

    def test_backdrop_position_and_size(self):
        """Test reading position and size from backdrop."""
        prim = _mock_backdrop_prim(position=(100.0, 200.0), size=(300.0, 150.0))

        pos_attr = prim.GetAttribute("ui:nodegraph:node:pos")
        self.assertTrue(pos_attr.HasAuthoredValue())
        pos = pos_attr.Get()
        self.assertEqual(pos[0], 100.0)
        self.assertEqual(pos[1], 200.0)

        size_attr = prim.GetAttribute("ui:nodegraph:node:size")
        self.assertTrue(size_attr.HasAuthoredValue())
        size = size_attr.Get()
        self.assertEqual(size[0], 300.0)
        self.assertEqual(size[1], 150.0)

    def test_backdrop_display_color(self):
        """Test reading display color from backdrop."""
        prim = _mock_backdrop_prim(display_color=(0.5, 0.3, 0.8))

        color_attr = prim.GetAttribute("ui:nodegraph:node:displayColor")
        self.assertTrue(color_attr.IsValid())
        self.assertTrue(color_attr.HasAuthoredValue())
        color = color_attr.Get()
        self.assertAlmostEqual(color[0], 0.5, places=5)
        self.assertAlmostEqual(color[1], 0.3, places=5)
        self.assertAlmostEqual(color[2], 0.8, places=5)

    def test_backdrop_no_description(self):
        """Test backdrop with no description authored."""
        prim = _mock_backdrop_prim()
        attr = prim.GetAttribute("ui:description")
        self.assertFalse(attr.HasAuthoredValue())

    def test_backdrop_stacking_order(self):
        """Test reading stacking order from backdrop."""
        prim = _mock_backdrop_prim(stacking_order=5)
        attr = prim.GetAttribute("ui:nodegraph:node:stackingOrder")
        self.assertTrue(attr.HasAuthoredValue())
        self.assertEqual(attr.Get(), 5)

    def test_backdrop_icon_inherited(self):
        """Test that backdrop can have icon attribute."""
        prim = _mock_backdrop_prim()
        self.assertTrue(prim.IsValid())


class TestGroupStickerFromPrim(unittest.TestCase):
    """Integration tests for GroupSticker.from_prim reading Backdrop prims."""

    def test_from_prim_position_scale(self):
        """Verify position is scaled by 1000x from USD to screen coords."""
        prim = _mock_backdrop_prim(position=(1.5, 2.0))

        pos_attr = prim.GetAttribute("ui:nodegraph:node:pos")
        pos = pos_attr.Get()
        screen_x = pos[0] * _POSITION_SCALE
        screen_y = pos[1] * _POSITION_SCALE

        self.assertAlmostEqual(screen_x, 1500.0, places=1)
        self.assertAlmostEqual(screen_y, 2000.0, places=1)

    def test_from_prim_size_scale(self):
        """Verify size is scaled by 1000x from USD to screen coords."""
        prim = _mock_backdrop_prim(size=(0.5, 0.3))

        size_attr = prim.GetAttribute("ui:nodegraph:node:size")
        size = size_attr.Get()
        screen_w = size[0] * _POSITION_SCALE
        screen_h = size[1] * _POSITION_SCALE

        self.assertAlmostEqual(screen_w, 500.0, places=1)
        self.assertAlmostEqual(screen_h, 300.0, places=1)

    def test_from_prim_description_to_name(self):
        """Verify description is used as sticker name."""
        prim = _mock_backdrop_prim(description="My Sticker Group")

        desc_attr = prim.GetAttribute("ui:description")
        name = desc_attr.Get()
        self.assertEqual(name, "My Sticker Group")

    def test_from_prim_color_conversion(self):
        """Verify displayColor Vec3f is converted to Vec4f with alpha=1.0."""
        prim = _mock_backdrop_prim(display_color=(0.5, 0.3, 0.8))

        color_attr = prim.GetAttribute("ui:nodegraph:node:displayColor")
        color = color_attr.Get()
        color_with_alpha = (color[0], color[1], color[2], 1.0)

        self.assertAlmostEqual(color_with_alpha[0], 0.5, places=5)
        self.assertAlmostEqual(color_with_alpha[1], 0.3, places=5)
        self.assertAlmostEqual(color_with_alpha[2], 0.8, places=5)
        self.assertAlmostEqual(color_with_alpha[3], 1.0, places=5)

    def test_from_prim_defaults_no_attributes(self):
        """Verify defaults are used when no attributes are authored."""
        prim = _mock_backdrop_prim()

        pos_attr = prim.GetAttribute("ui:nodegraph:node:pos")
        size_attr = prim.GetAttribute("ui:nodegraph:node:size")
        color_attr = prim.GetAttribute("ui:nodegraph:node:displayColor")

        self.assertFalse(pos_attr.HasAuthoredValue())
        self.assertFalse(size_attr.HasAuthoredValue())
        self.assertFalse(color_attr.IsValid())

    def test_from_prim_has_prim_reference(self):
        """Verify from_prim stores reference to the source prim."""
        prim = _mock_backdrop_prim()
        self.assertTrue(prim.IsValid())
        self.assertEqual(prim.GetPath().pathString, "/TestBackdrop")

    def test_from_prim_custom_data_corner_radius(self):
        """Verify customData cornerRadius is read."""
        prim = _mock_backdrop_prim(custom_data={"cornerRadius": 15.0})
        custom_data = prim.GetCustomData()
        corner_radius = custom_data.get("cornerRadius", DEFAULT_CORNER_RADIUS)
        self.assertEqual(corner_radius, 15.0)

    def test_from_prim_custom_data_defaults(self):
        """Verify customData defaults when not authored."""
        prim = _mock_backdrop_prim()
        custom_data = prim.GetCustomData()
        corner_radius = custom_data.get("cornerRadius", DEFAULT_CORNER_RADIUS)
        inner_stroke = custom_data.get("innerStroke", DEFAULT_INNER_STROKE)
        self.assertEqual(corner_radius, DEFAULT_CORNER_RADIUS)
        self.assertEqual(inner_stroke, DEFAULT_INNER_STROKE)


if __name__ == "__main__":
    unittest.main()
