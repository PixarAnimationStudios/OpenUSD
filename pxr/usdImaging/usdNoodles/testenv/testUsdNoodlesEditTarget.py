#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for NodeGraph edit-target callback management and save-path integration.

Covers:
- Stage-changed and edit-target-changed callback registration, invocation, removal
- Error isolation: a failing callback does not prevent subsequent callbacks
- getEditTargetLayer / setEditTargetLayer round-trip
- _saveStage path: connection authored on the current edit target (root or sublayer)
  persists after layer.Save() + reload
"""

import os
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import MagicMock

try:
    from pxr import Sdf, Usd
    from pxr.UsdNoodles.nodeGraph import (
        NodeGraph,
        subscribe_edit_target_warning,
        unsubscribe_edit_target_warning,
        warn_if_non_persistent_edit_target,
    )

    _has_pxr = True
except ImportError as e:
    print(f"pxr.UsdNoodles.nodeGraph import failed: {e}")
    _has_pxr = False


def _probe_real_stage() -> bool:
    """Check whether Usd.Stage.CreateInMemory() works without crashing.

    The noodles test binary can SIGSEGV on real USD stage creation.
    A subprocess probe detects this so integration tests can be skipped.
    """
    try:
        result = subprocess.run(
            [sys.executable, "-c", "from pxr import Usd; Usd.Stage.CreateInMemory()"],
            timeout=30,
            capture_output=True,
            env={
                **os.environ,
                "PXR_PLUGINPATH_NAME": os.environ.get("PXR_PLUGINPATH_NAME", ""),
            },
        )
        return result.returncode == 0
    except Exception:
        return False


_can_create_stage: bool = _has_pxr and _probe_real_stage()


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestStageChangedCallbacks(unittest.TestCase):
    """Verify stage-changed callback registration, invocation, and removal."""

    def test_callback_invoked_on_set_stage(self):
        ng = NodeGraph()
        received = []
        ng.addStageChangedCallback(lambda s: received.append(s))
        sentinel = object()
        ng.setStage(sentinel)
        self.assertEqual(received, [sentinel])

    def test_multiple_callbacks_all_invoked(self):
        ng = NodeGraph()
        a, b = [], []
        ng.addStageChangedCallback(lambda s: a.append(s))
        ng.addStageChangedCallback(lambda s: b.append(s))
        ng.setStage("stage")
        self.assertEqual(a, ["stage"])
        self.assertEqual(b, ["stage"])

    def test_remove_callback(self):
        ng = NodeGraph()
        received = []

        def cb(s):
            received.append(s)

        ng.addStageChangedCallback(cb)
        ng.removeStageChangedCallback(cb)
        ng.setStage("stage")
        self.assertEqual(received, [])

    def test_remove_nonexistent_callback_is_noop(self):
        ng = NodeGraph()
        ng.removeStageChangedCallback(lambda s: None)

    def test_failing_callback_does_not_block_others(self):
        ng = NodeGraph()
        received = []

        def bad_cb(_):
            raise ValueError("boom")

        ng.addStageChangedCallback(bad_cb)
        ng.addStageChangedCallback(lambda s: received.append(s))
        ng.setStage("stage")
        self.assertEqual(received, ["stage"])


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestEditTargetChangedCallbacks(unittest.TestCase):
    """Verify edit-target-changed callback registration, invocation, and removal."""

    def _make_ng_with_stage(self):
        ng = NodeGraph()
        stage = MagicMock()
        ng.setStage(stage)
        return ng, stage

    def test_callback_invoked_on_set_edit_target(self):
        ng, stage = self._make_ng_with_stage()
        received = []
        ng.addEditTargetChangedCallback(lambda lay: received.append(lay))
        layer = MagicMock()
        ng.setEditTargetLayer(layer)
        stage.SetEditTarget.assert_called_once_with(layer)
        self.assertEqual(received, [layer])

    def test_set_edit_target_noop_without_stage(self):
        ng = NodeGraph()
        received = []
        ng.addEditTargetChangedCallback(lambda lay: received.append(lay))
        ng.setEditTargetLayer(MagicMock())
        self.assertEqual(received, [])

    def test_set_edit_target_noop_with_none_layer(self):
        ng, _ = self._make_ng_with_stage()
        received = []
        ng.addEditTargetChangedCallback(lambda lay: received.append(lay))
        ng.setEditTargetLayer(None)
        self.assertEqual(received, [])

    def test_remove_callback(self):
        ng, _ = self._make_ng_with_stage()
        received = []

        def cb(lay):
            received.append(lay)

        ng.addEditTargetChangedCallback(cb)
        ng.removeEditTargetChangedCallback(cb)
        ng.setEditTargetLayer(MagicMock())
        self.assertEqual(received, [])

    def test_failing_callback_does_not_block_others(self):
        ng, _ = self._make_ng_with_stage()
        received = []

        def bad_cb(_):
            raise RuntimeError("boom")

        ng.addEditTargetChangedCallback(bad_cb)
        ng.addEditTargetChangedCallback(lambda lay: received.append(lay))
        ng.setEditTargetLayer(MagicMock())
        self.assertEqual(len(received), 1)


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestGetEditTargetLayer(unittest.TestCase):
    """Verify getEditTargetLayer returns the correct layer."""

    def test_returns_none_without_stage(self):
        ng = NodeGraph()
        self.assertIsNone(ng.getEditTargetLayer())

    def test_returns_edit_target_layer(self):
        ng = NodeGraph()
        layer = MagicMock()
        stage = MagicMock()
        stage.GetEditTarget.return_value.GetLayer.return_value = layer
        ng.setStage(stage)
        self.assertEqual(ng.getEditTargetLayer(), layer)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class TestSaveStageIntegration(unittest.TestCase):
    """Verify the _saveStage path: layer.Save() on the active edit target.

    Mirrors graphView.py:4526 — stage.GetEditTarget().GetLayer().Save().
    Uses real USD stages and temp files; no Qt required.
    """

    def _make_stage_on_disk(self, suffix=".usda"):
        fd, path = tempfile.mkstemp(suffix=suffix)
        os.close(fd)
        os.unlink(path)  # USD CreateNew/Layer.CreateNew fail if the file already exists
        self.addCleanup(lambda: os.path.exists(path) and os.unlink(path))
        return path

    def _save_active_edit_target(self, stage):
        """Mirror of graphView._saveStage sans Qt error reporting."""
        layer = stage.GetEditTarget().GetLayer()
        if layer and not layer.anonymous:
            layer.Save()

    def test_connection_persists_after_root_layer_save_reload(self):
        """Author a relationship on root layer, save, reload — opinion must persist."""
        path = self._make_stage_on_disk()
        stage = Usd.Stage.CreateNew(path)
        prim = stage.DefinePrim("/Node", "Xform")
        rel = prim.CreateRelationship("inputs:value")
        rel.AddTarget("/Source")
        stage.GetRootLayer().Save()

        reloaded = Usd.Stage.Open(path)
        reloaded_rel = reloaded.GetPrimAtPath("/Node").GetRelationship("inputs:value")
        self.assertTrue(reloaded_rel.IsValid())
        self.assertIn(Sdf.Path("/Source"), reloaded_rel.GetTargets())

    def test_sublayer_edit_target_persists_connection_on_save(self):
        """With a sublayer as edit target, authored connections land on the sublayer.

        Mirrors the new graphView behavior: ``_createConnection`` no longer
        forces the root layer, so a connection authored while the sublayer is
        the active edit target persists when the sublayer is saved.
        """
        root_path = self._make_stage_on_disk(".usda")
        sub_path = self._make_stage_on_disk(".usda")

        # Bootstrap an empty sublayer so it has a real path on disk.
        sub_layer = Sdf.Layer.CreateNew(sub_path)
        sub_layer.Save()

        stage = Usd.Stage.CreateNew(root_path)
        stage.GetRootLayer().subLayerPaths.append(sub_path)

        # User selects the sublayer as edit target, then authors a connection.
        # Production _createConnection no longer overrides the edit target.
        stage.SetEditTarget(sub_layer)
        prim = stage.DefinePrim("/Node", "Xform")
        prim.CreateRelationship("inputs:value").AddTarget("/Source")

        # Ctrl+S saves the active edit target (the sublayer).
        self._save_active_edit_target(stage)

        # The connection opinion is in the sublayer file on disk.
        with open(sub_path, "r") as f:
            sub_contents = f.read()
        self.assertIn(
            "inputs:value",
            sub_contents,
            "Sublayer file should contain the connection opinion now that "
            "_createConnection honors the active edit target",
        )

        # And the root layer file on disk has no opinion for this prim.
        with open(root_path, "r") as f:
            root_contents = f.read()
        self.assertNotIn("inputs:value", root_contents)


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestEditTargetWarningSubscribers(unittest.TestCase):
    """Verify the module-level warning notifier used by all authoring sites.

    Uses MagicMock for stage/layer so the test suite stays headless and
    cross-platform — real USD stage operations segfault on macOS in the
    buck test sandbox.
    """

    def setUp(self):
        self.received = []
        self._cb = self.received.append
        subscribe_edit_target_warning(self._cb)
        self.addCleanup(unsubscribe_edit_target_warning, self._cb)

    def _make_stage(self, edit_target_layer, session_layer=None):
        """Return a mock stage whose edit target resolves to *edit_target_layer*."""
        stage = MagicMock()
        if session_layer is None:
            session_layer = MagicMock()
            session_layer.anonymous = False
        stage.GetSessionLayer.return_value = session_layer
        stage.GetEditTarget.return_value.GetLayer.return_value = edit_target_layer
        return stage

    def test_persistent_edit_target_does_not_warn(self):
        persistent = MagicMock()
        persistent.anonymous = False
        stage = self._make_stage(persistent)

        result = warn_if_non_persistent_edit_target(stage)

        self.assertIsNone(result)
        self.assertEqual(self.received, [])

    def test_session_layer_edit_target_warns(self):
        session = MagicMock()
        session.anonymous = False
        stage = self._make_stage(session, session_layer=session)

        result = warn_if_non_persistent_edit_target(stage)

        self.assertIsNotNone(result)
        self.assertIn("session layer", result)
        self.assertEqual(self.received, [result])

    def test_anonymous_layer_edit_target_warns(self):
        anon = MagicMock()
        anon.anonymous = True
        stage = self._make_stage(anon)

        result = warn_if_non_persistent_edit_target(stage)

        self.assertIsNotNone(result)
        self.assertIn("anonymous", result)
        self.assertEqual(self.received, [result])

    def test_no_stage_is_noop(self):
        result = warn_if_non_persistent_edit_target(None)

        self.assertIsNone(result)
        self.assertEqual(self.received, [])

    def test_unsubscribe_stops_delivery(self):
        unsubscribe_edit_target_warning(self._cb)

        anon = MagicMock()
        anon.anonymous = True
        warn_if_non_persistent_edit_target(self._make_stage(anon))

        self.assertEqual(self.received, [])

    def test_failing_subscriber_does_not_block_others(self):
        def bad(_):
            raise RuntimeError("boom")

        subscribe_edit_target_warning(bad)
        self.addCleanup(unsubscribe_edit_target_warning, bad)

        anon = MagicMock()
        anon.anonymous = True
        warn_if_non_persistent_edit_target(self._make_stage(anon))

        # The good subscriber still received the message even though bad raised.
        self.assertEqual(len(self.received), 1)

    def test_set_edit_target_layer_routes_warning_to_subscribers(self):
        # setEditTargetLayer must hand off to the same notifier used by
        # authoring sites, so changing target into a non-persistent layer
        # surfaces the popup the same way authoring would.
        session = MagicMock()
        session.anonymous = False
        stage = self._make_stage(session, session_layer=session)
        ng = NodeGraph()
        ng.setStage(stage)

        ng.setEditTargetLayer(session)

        self.assertEqual(len(self.received), 1)
        self.assertIn("session layer", self.received[0])
