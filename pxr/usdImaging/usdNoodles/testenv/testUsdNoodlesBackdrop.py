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
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

try:
    from pxr import Gf, Sdf
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.UsdNoodles.primAuthoring import PrimAuthor
    from pxr.UsdNoodles.widgets.groupSticker import GroupSticker, GroupStickerRenderer

    _has_pxr = True
except ImportError:
    _has_pxr = False

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


class _RecordingAttr:
    def __init__(self):
        self.value = None

    def Set(self, value):
        self.value = value
        return True


class _FakeNodeVertex:
    __slots__ = ("args",)

    def __init__(self, *args):
        self.args = args


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestBackdropAuthoring(unittest.TestCase):
    def test_create_backdrop_prim_uses_usdui_schema(self):
        parent_prim = MagicMock()
        parent_prim.IsValid.return_value = True
        parent_prim.GetChildren.return_value = []

        created_prim = MagicMock()
        created_prim.IsValid.return_value = True
        created_prim.GetPath.return_value = Sdf.Path("/Graph/Backdrop1")

        stage = MagicMock()

        def get_prim_at_path(path):
            path_str = str(path)
            if path_str == "/Graph":
                return parent_prim
            if path_str == "/Graph/Backdrop1":
                return created_prim
            return None

        stage.GetPrimAtPath.side_effect = get_prim_at_path

        pos_attr = _RecordingAttr()
        size_attr = _RecordingAttr()
        color_attr = _RecordingAttr()
        api = SimpleNamespace(
            CreatePosAttr=MagicMock(return_value=pos_attr),
            CreateSizeAttr=MagicMock(return_value=size_attr),
            CreateDisplayColorAttr=MagicMock(return_value=color_attr),
        )

        desc_attr = _RecordingAttr()
        backdrop = SimpleNamespace(
            GetPrim=MagicMock(return_value=created_prim),
            CreateDescriptionAttr=MagicMock(return_value=desc_attr),
        )
        fake_usdui = SimpleNamespace(
            Backdrop=SimpleNamespace(Define=MagicMock(return_value=backdrop)),
            NodeGraphNodeAPI=MagicMock(return_value=api),
        )

        with patch("pxr.UsdNoodles.primAuthoring.UsdUI", fake_usdui):
            prim_path = PrimAuthor.create_backdrop_prim(
                stage,
                "/Graph",
                Gf.Vec2d(1000.0, 2000.0),
                Gf.Vec2d(300.0, 400.0),
                description="My Backdrop",
                color=Gf.Vec3f(0.2, 0.3, 0.4),
            )

        self.assertEqual(str(prim_path), "/Graph/Backdrop1")
        define_path = fake_usdui.Backdrop.Define.call_args[0][1]
        self.assertEqual(str(define_path), "/Graph/Backdrop1")
        fake_usdui.NodeGraphNodeAPI.assert_called_once_with(created_prim)
        self.assertAlmostEqual(pos_attr.value[0], 1.0)
        self.assertAlmostEqual(pos_attr.value[1], 2.0)
        self.assertAlmostEqual(size_attr.value[0], 0.3)
        self.assertAlmostEqual(size_attr.value[1], 0.4)
        self.assertEqual(desc_attr.value, "My Backdrop")
        self.assertAlmostEqual(color_attr.value[0], 0.2)
        self.assertAlmostEqual(color_attr.value[1], 0.3)
        self.assertAlmostEqual(color_attr.value[2], 0.4)


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestBackdropFromSelection(unittest.TestCase):
    def test_compute_backdrop_bounds_for_nodes(self):
        nodes = [
            SimpleNamespace(position=(10.0, 20.0), size=(100.0, 50.0)),
            SimpleNamespace(position=(200.0, 5.0), size=(60.0, 80.0)),
        ]

        position, size = GraphView._computeBackdropBoundsForNodes(nodes, 10.0)

        self.assertEqual(position[0], 0.0)
        self.assertEqual(position[1], -5.0)
        self.assertEqual(size[0], 270.0)
        self.assertEqual(size[1], 100.0)

    def test_create_backdrop_from_selection_authors_and_adds_sticker(self):
        node_a = SimpleNamespace(
            id="/Graph/A", position=(100.0, 200.0), size=(50.0, 30.0), selected=True
        )
        node_b = SimpleNamespace(
            id="/Graph/B", position=(10.0, 50.0), size=(20.0, 40.0), selected=True
        )
        prim_path = Sdf.Path("/Graph/Backdrop1")
        prim = MagicMock()
        prim.IsValid.return_value = True
        prim.GetPath.return_value = prim_path

        stage = MagicMock()
        stage.GetPrimAtPath.return_value = prim
        sticker = SimpleNamespace(_prim=prim)
        notice_handler = SimpleNamespace(setEnabled=MagicMock())
        view = SimpleNamespace(
            nodes={"/Graph/A": node_a, "/Graph/B": node_b},
            _selectedNodes={"/Graph/A", "/Graph/B"},
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _usdviewApi=None,
            _resolveBackdropParentPath=MagicMock(return_value="/Graph"),
            groupStickers=[],
            _noticeHandler=notice_handler,
            _groupStickerRenderer=SimpleNamespace(markDirty=MagicMock()),
            _showPopupMessage=MagicMock(),
            update=MagicMock(),
        )

        with (
            patch(
                "pxr.UsdNoodles.primAuthoring.PrimAuthor.create_backdrop_prim",
                return_value=prim_path,
            ) as create_backdrop,
            patch(
                "pxr.UsdNoodles.graphView.GroupSticker.from_prim",
                return_value=sticker,
            ),
            patch("pxr.UsdNoodles.graphView._push_undo_command") as push_undo,
        ):
            GraphView.createBackdropFromSelection(view)

        create_backdrop.assert_called_once()
        _, parent_path, position, size = create_backdrop.call_args[0][:4]
        self.assertEqual(parent_path, "/Graph")
        self.assertEqual(position[0], -70.0)
        self.assertEqual(position[1], -30.0)
        self.assertEqual(size[0], 300.0)
        self.assertEqual(size[1], 340.0)
        self.assertEqual(create_backdrop.call_args.kwargs["description"], "Backdrop")
        self.assertEqual(view.groupStickers, [sticker])
        notice_handler.setEnabled.assert_any_call(False)
        notice_handler.setEnabled.assert_any_call(True)
        push_undo.assert_called_once()
        self.assertEqual(push_undo.call_args[0][0], "Create Backdrop")
        view._showPopupMessage.assert_called_with("Created backdrop for 2 node(s)")

        undo = push_undo.call_args[0][2]
        redo = push_undo.call_args[0][1]
        undo()
        self.assertEqual(view.groupStickers, [])
        prim.SetActive.assert_called_with(False)

        with patch(
            "pxr.UsdNoodles.graphView.GroupSticker.from_prim",
            return_value=sticker,
        ):
            redo()
        self.assertEqual(view.groupStickers, [sticker])
        prim.SetActive.assert_called_with(True)

    def test_create_backdrop_from_selection_requires_selection(self):
        view = SimpleNamespace(
            nodes={},
            _selectedNodes=set(),
            _showPopupMessage=MagicMock(),
        )

        GraphView.createBackdropFromSelection(view)

        view._showPopupMessage.assert_called_once_with("No nodes selected")


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestGroupStickerRenderer(unittest.TestCase):
    def test_render_stickers_uses_cpp_renderer(self):
        cpp_renderer = MagicMock()
        renderer = GroupStickerRenderer()
        renderer.initialize("shader-library", cpp_renderer=cpp_renderer)

        sticker = GroupSticker(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(30.0, 40.0),
        )
        projection = [1.0 if i in (0, 5, 10, 15) else 0.0 for i in range(16)]

        self.assertTrue(renderer.renderStickers([sticker], projection, _FakeNodeVertex))

        cpp_renderer.initialize.assert_called_once_with("shader-library")
        cpp_renderer.renderStickers.assert_called_once()
        vertices, projection_arg, corner_radius, stroke_color = (
            cpp_renderer.renderStickers.call_args[0]
        )
        self.assertEqual(len(vertices), 6)
        self.assertEqual(projection_arg, projection)
        self.assertEqual(corner_radius, sticker.cornerRadius)
        self.assertEqual(stroke_color, list(sticker.strokeColor))
