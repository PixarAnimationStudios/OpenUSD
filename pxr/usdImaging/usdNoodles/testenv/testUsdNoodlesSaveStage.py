#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for GraphView._saveStage and GraphView._persistNodePositions.

Covers:
  _persistNodePositions (graphView.py:5541):
    1. All node positions are written via _writePositionToUsdRaw
    2. Edit target is set to the given layer during writes
    3. Edit target is restored even when _writePositionToUsdRaw raises

  _saveStage (graphView.py:5559):
    1. Early-returns: no stage, no edit-target layer, anonymous layer
    2. Successful save: _persistNodePositions + layer.Save() + popup
    3. Unsaved-layer warnings when other layers are dirty
    4. Outer try/except: unexpected failure shows "Save failed: ..."

GraphView cannot be imported in headless CI (module-level GL imports
hang the test runner), so production logic is mirrored in shims —
same approach used by ``testUsdNoodlesGraphViewShortcut``.
"""

import os
import unittest
from collections import OrderedDict
from unittest.mock import MagicMock


# -- shims -----------------------------------------------------------------


def _persist_node_positions(view, stage, layer):
    """Mirror of GraphView._persistNodePositions (graphView.py:5541).

    Omits ``Sdf.ChangeBlock()`` — that is a USD batching optimisation
    and does not affect the try/finally restore logic under test.
    """
    prev = stage.GetEditTarget()
    stage.SetEditTarget(layer)
    try:
        for node in view.nodes.values():
            node._writePositionToUsdRaw(node.position)
    finally:
        stage.SetEditTarget(prev)


def _save_stage(view):
    """Mirror of GraphView._saveStage (graphView.py:5559).

    ``view`` is a duck-typed stand-in carrying the attributes the real
    method touches.  Keep this shim in lockstep with the production
    implementation.
    """
    stage = view.nodeGraph.getStage() if view.nodeGraph else None
    if not stage:
        view._showPopupMessage("No stage to save")
        return

    try:
        layer = stage.GetEditTarget().GetLayer()

        if not layer:
            view._showPopupMessage("No edit target layer to save")
            return

        if layer.anonymous:
            view._showPopupMessage(
                f"Layer '{layer.identifier}' is anonymous — use File > Save As"
            )
            return

        _persist_node_positions(view, stage, layer)
        layer.Save()

        unsaved = []
        try:
            for otherLayer in stage.GetLayerStack(includeSessionLayers=False):
                if otherLayer and otherLayer != layer and otherLayer.dirty:
                    unsaved.append(
                        os.path.basename(otherLayer.identifier) or otherLayer.identifier
                    )
        except Exception:
            pass
        if unsaved:
            names = ", ".join(unsaved[:3])
            suffix = f" (+{len(unsaved) - 3} more)" if len(unsaved) > 3 else ""
            view._showPopupMessage(
                f"Saved layer: {layer.identifier}\n"
                f"⚠ Unsaved changes on: {names}{suffix}"
            )
        else:
            view._showPopupMessage(f"Saved layer: {layer.identifier}")

    except Exception as e:
        view._showPopupMessage(f"Save failed: {e}")
        import traceback

        traceback.print_exc()


# -- helpers ---------------------------------------------------------------


def _make_node(position=(100.0, 200.0)):
    """Build a mock node with a position and _writePositionToUsdRaw stub."""
    node = MagicMock()
    node.position = position
    return node


def _make_layer(identifier="/tmp/scene.usda", anonymous=False, dirty=False):
    """Build a mock SdfLayer."""
    layer = MagicMock()
    layer.identifier = identifier
    layer.anonymous = anonymous
    layer.dirty = dirty
    return layer


def _make_stage(*, edit_target_layer=None, layer_stack=None):
    """Build a mock USD stage with an edit target that returns the given layer."""
    stage = MagicMock()
    edit_target = MagicMock()
    edit_target.GetLayer.return_value = edit_target_layer
    stage.GetEditTarget.return_value = edit_target
    stage.GetLayerStack.return_value = layer_stack or []
    return stage


def _make_view(stage=None, has_node_graph=True, nodes=None):
    """Build a minimal mock view with the surface the shims touch."""
    view = MagicMock(spec=[])
    view.nodeGraph = MagicMock() if has_node_graph else None
    if has_node_graph:
        view.nodeGraph.getStage.return_value = stage
    view._showPopupMessage = MagicMock()
    view.nodes = nodes if nodes is not None else {}
    return view


# -- _persistNodePositions tests -------------------------------------------


class TestPersistNodePositions(unittest.TestCase):
    """Coverage gap pattern #7 (LIFECYCLE_CLEANUP):
    edit target must be set to the layer, positions written, then the
    previous target restored — even when _writePositionToUsdRaw raises."""

    def test_positions_written_to_all_nodes(self):
        nodeA = _make_node(position=(10.0, 20.0))
        nodeB = _make_node(position=(30.0, 40.0))
        view = _make_view(nodes={"a": nodeA, "b": nodeB})
        stage = MagicMock()
        layer = _make_layer()

        _persist_node_positions(view, stage, layer)

        nodeA._writePositionToUsdRaw.assert_called_once_with((10.0, 20.0))
        nodeB._writePositionToUsdRaw.assert_called_once_with((30.0, 40.0))

    def test_edit_target_set_to_layer_then_restored(self):
        prev_target = MagicMock(name="prev_target")
        stage = MagicMock()
        stage.GetEditTarget.return_value = prev_target
        layer = _make_layer()
        view = _make_view(nodes={"n": _make_node()})

        _persist_node_positions(view, stage, layer)

        calls = stage.SetEditTarget.call_args_list
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[0][0][0], layer)
        self.assertEqual(calls[1][0][0], prev_target)

    def test_empty_nodes_still_restores_edit_target(self):
        prev_target = MagicMock(name="prev_target")
        stage = MagicMock()
        stage.GetEditTarget.return_value = prev_target
        layer = _make_layer()
        view = _make_view(nodes={})

        _persist_node_positions(view, stage, layer)

        calls = stage.SetEditTarget.call_args_list
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[0][0][0], layer)
        self.assertEqual(calls[1][0][0], prev_target)

    def test_edit_target_restored_on_write_error(self):
        prev_target = MagicMock(name="prev_target")
        stage = MagicMock()
        stage.GetEditTarget.return_value = prev_target
        layer = _make_layer()
        bad_node = _make_node()
        bad_node._writePositionToUsdRaw.side_effect = RuntimeError("write boom")
        view = _make_view(nodes={"bad": bad_node})

        with self.assertRaises(RuntimeError):
            _persist_node_positions(view, stage, layer)

        calls = stage.SetEditTarget.call_args_list
        self.assertEqual(len(calls), 2)
        self.assertEqual(calls[0][0][0], layer)
        self.assertEqual(calls[1][0][0], prev_target)

    def test_partial_write_restores_edit_target(self):
        """First node succeeds, second raises — edit target still restored."""
        prev_target = MagicMock(name="prev_target")
        stage = MagicMock()
        stage.GetEditTarget.return_value = prev_target
        layer = _make_layer()

        good_node = _make_node(position=(1.0, 2.0))
        bad_node = _make_node()
        bad_node._writePositionToUsdRaw.side_effect = RuntimeError("boom")
        nodes = OrderedDict([("good", good_node), ("bad", bad_node)])
        view = _make_view(nodes=nodes)

        with self.assertRaises(RuntimeError):
            _persist_node_positions(view, stage, layer)

        good_node._writePositionToUsdRaw.assert_called_once_with((1.0, 2.0))
        calls = stage.SetEditTarget.call_args_list
        self.assertEqual(calls[1][0][0], prev_target)


# -- _saveStage: early returns --------------------------------------------


class TestSaveStageEarlyReturns(unittest.TestCase):
    """Coverage gap pattern #1 (STATE_MACHINE): early-return branches."""

    def test_no_node_graph_shows_popup(self):
        view = _make_view(has_node_graph=False)
        _save_stage(view)
        view._showPopupMessage.assert_called_once_with("No stage to save")

    def test_no_stage_shows_popup(self):
        view = _make_view(stage=None)
        _save_stage(view)
        view._showPopupMessage.assert_called_once_with("No stage to save")

    def test_no_edit_target_layer_shows_popup(self):
        stage = _make_stage(edit_target_layer=None)
        view = _make_view(stage=stage)
        _save_stage(view)
        view._showPopupMessage.assert_called_once_with("No edit target layer to save")

    def test_anonymous_layer_shows_popup(self):
        layer = _make_layer(identifier="anon:0001", anonymous=True)
        stage = _make_stage(edit_target_layer=layer)
        view = _make_view(stage=stage)
        _save_stage(view)
        view._showPopupMessage.assert_called_once_with(
            "Layer 'anon:0001' is anonymous — use File > Save As"
        )
        layer.Save.assert_not_called()


# -- _saveStage: successful save ------------------------------------------


class TestSaveStageHappyPath(unittest.TestCase):
    """Coverage gap pattern #7 (LIFECYCLE_CLEANUP):
    _persistNodePositions must run before layer.Save()."""

    def test_persist_positions_called_before_save(self):
        layer = _make_layer(identifier="/tmp/scene.usda")
        stage = _make_stage(edit_target_layer=layer)
        node = _make_node()
        view = _make_view(stage=stage, nodes={"n": node})

        order = []
        node._writePositionToUsdRaw.side_effect = lambda pos: order.append("persist")
        layer.Save.side_effect = lambda: order.append("save")

        _save_stage(view)

        self.assertEqual(order, ["persist", "save"])

    def test_successful_save_shows_saved_popup(self):
        layer = _make_layer(identifier="/tmp/scene.usda")
        stage = _make_stage(edit_target_layer=layer)
        view = _make_view(stage=stage)

        _save_stage(view)

        layer.Save.assert_called_once()
        view._showPopupMessage.assert_called_once_with("Saved layer: /tmp/scene.usda")

    def test_unsaved_dirty_layers_shown_in_warning(self):
        layer = _make_layer(identifier="/tmp/scene.usda")
        other = _make_layer(identifier="/tmp/materials.usda", dirty=True)
        stage = _make_stage(
            edit_target_layer=layer,
            layer_stack=[layer, other],
        )
        view = _make_view(stage=stage)

        _save_stage(view)

        msg = view._showPopupMessage.call_args[0][0]
        self.assertIn("Saved layer: /tmp/scene.usda", msg)
        self.assertIn("materials.usda", msg)

    def test_clean_layers_no_warning(self):
        layer = _make_layer(identifier="/tmp/scene.usda")
        other = _make_layer(identifier="/tmp/materials.usda", dirty=False)
        stage = _make_stage(
            edit_target_layer=layer,
            layer_stack=[layer, other],
        )
        view = _make_view(stage=stage)

        _save_stage(view)

        view._showPopupMessage.assert_called_once_with("Saved layer: /tmp/scene.usda")


# -- _saveStage: error handling --------------------------------------------


class TestSaveStageErrorHandling(unittest.TestCase):
    """Coverage gap pattern #4 (ERROR_HANDLING): outer try/except."""

    def test_save_failure_shows_popup(self):
        layer = _make_layer()
        layer.Save.side_effect = OSError("disk full")
        stage = _make_stage(edit_target_layer=layer)
        view = _make_view(stage=stage)

        _save_stage(view)

        msg = view._showPopupMessage.call_args[0][0]
        self.assertTrue(msg.startswith("Save failed:"), msg)
        self.assertIn("disk full", msg)

    def test_persist_failure_shows_popup_and_skips_save(self):
        layer = _make_layer()
        stage = _make_stage(edit_target_layer=layer)
        node = _make_node()
        node._writePositionToUsdRaw.side_effect = RuntimeError("write boom")
        view = _make_view(stage=stage, nodes={"n": node})

        _save_stage(view)

        msg = view._showPopupMessage.call_args[0][0]
        self.assertTrue(msg.startswith("Save failed:"), msg)
        self.assertIn("write boom", msg)
        layer.Save.assert_not_called()

    def test_layer_stack_error_swallowed(self):
        """GetLayerStack failure must not abort the save popup."""
        layer = _make_layer(identifier="/tmp/scene.usda")
        stage = _make_stage(edit_target_layer=layer)
        stage.GetLayerStack.side_effect = RuntimeError("stack error")
        view = _make_view(stage=stage)

        _save_stage(view)

        layer.Save.assert_called_once()
        view._showPopupMessage.assert_called_once_with("Saved layer: /tmp/scene.usda")
