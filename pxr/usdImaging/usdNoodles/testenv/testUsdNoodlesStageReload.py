#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for stage-reload crash fix.

Covers the logic added/modified for NVIDIA Windows stage-reopen safety:
  1. _onStageReplaced: null-stage early return still schedules
     _finishStageReload (prevents widget freeze)
  2. _onStageReplaced: reload exceptions are logged, not swallowed
  3. _finishStageReload: sets initialized=True to preserve FontAtlas
  4. _isStageViewMidTeardown: detects stageView mid-teardown
  5. _installCloseStageGuard: wraps _closeStage to pre-clean VBOs
  6. event() paint guard: blocks Paint during teardown

GraphView cannot be imported in headless CI (module-level GL imports),
so production logic is mirrored in shims — same approach as
testUsdNoodlesSaveStage and testUsdNoodlesGraphViewShortcut.
"""

import unittest
from unittest.mock import MagicMock


def _on_stage_replaced(view):
    """Mirror of GraphView._onStageReplaced (graphView.py:1110).

    Keep in lockstep with the production implementation.
    """
    view.setUpdatesEnabled(False)
    view.initialized = False

    view._noticeHandler.unregister()

    view._lastPrimTreeSelection = []
    view._lastSelectedNode = None
    view._selectedNodes.clear()
    view._selectedLinks.clear()

    stage = view._usdviewApi.stage if view._usdviewApi else None
    if stage is None:
        view._scheduleFinishReload()
        return

    view._deferRenderCacheClear = True
    try:
        if view._isContainer and view._currentPrimPath:
            view.loadContainer(stage, view._currentPrimPath)
        elif view._currentPrimPath is not None:
            view.loadBlueprint(stage, view._currentPrimPath)
        else:
            view.loadStage(stage)
    except Exception as e:
        view._warnReloadFailed(e)
    finally:
        view._deferRenderCacheClear = False

    view.initialized = False
    view._scheduleFinishReload()


def _finish_stage_reload(view):
    """Mirror of GraphView._finishStageReload (graphView.py:1175)."""
    if view._renderCacheClearPending:
        view._renderCacheClearPending = False
        view.makeCurrent()
        view._nodeRenderManager.clearCache()
        view.doneCurrent()

    view.linksChanged = True
    view.textChanged = True
    view.initialized = True
    view.setUpdatesEnabled(True)
    view.update()


def _is_stage_view_mid_teardown(view):
    """Mirror of GraphView._isStageViewMidTeardown (graphView.py:4031)."""
    if not view._usdviewApi:
        return False
    try:
        controller = view._usdviewApi._UsdviewApi__appController
        sv = getattr(controller, "_stageView", None)
        return sv is not None and not sv.updatesEnabled()
    except AttributeError:
        return False


def _install_close_stage_guard(manager, usdviewApi):
    """Mirror of NoodlesEditorManager._installCloseStageGuard."""
    try:
        controller = usdviewApi._UsdviewApi__appController
        if getattr(controller, "_noodles_close_guard", False):
            return
        original = controller._closeStage

        def _guarded_close():
            for window in manager.getAllWindows():
                gv = window.graphView
                gv.setUpdatesEnabled(False)
                gv.makeCurrent()
                gv._nodeRenderManager.clearCache()
                gv.doneCurrent()
            original()

        controller._closeStage = _guarded_close
        controller._noodles_close_guard = True
    except AttributeError:
        pass


def _make_view():
    """Create a duck-typed GraphView stand-in."""
    view = MagicMock()
    view.initialized = True
    view._selectedNodes = set()
    view._selectedLinks = set()
    view._currentPrimPath = None
    view._isContainer = False
    view._deferRenderCacheClear = False
    view._renderCacheClearPending = False
    view.linksChanged = False
    view.textChanged = False
    return view


class TestOnStageReplacedNullStage(unittest.TestCase):
    """When stage is None, _onStageReplaced must still schedule
    _finishStageReload so setUpdatesEnabled(True) is eventually called."""

    def test_null_stage_schedules_finish(self):
        view = _make_view()
        view._usdviewApi.stage = None

        _on_stage_replaced(view)

        view._scheduleFinishReload.assert_called_once()
        view.setUpdatesEnabled.assert_called_with(False)

    def test_null_stage_does_not_reload_graph(self):
        view = _make_view()
        view._usdviewApi.stage = None

        _on_stage_replaced(view)

        view.loadStage.assert_not_called()
        view.loadBlueprint.assert_not_called()
        view.loadContainer.assert_not_called()

    def test_null_stage_unregisters_notice_handler(self):
        view = _make_view()
        view._usdviewApi.stage = None

        _on_stage_replaced(view)

        view._noticeHandler.unregister.assert_called_once()


class TestOnStageReplacedWithStage(unittest.TestCase):
    """When stage is valid, _onStageReplaced reloads the graph and
    schedules _finishStageReload."""

    def test_calls_load_stage(self):
        view = _make_view()
        stage = MagicMock()
        view._usdviewApi.stage = stage
        view._currentPrimPath = None
        view._isContainer = False

        _on_stage_replaced(view)

        view.loadStage.assert_called_once_with(stage)
        view._scheduleFinishReload.assert_called_once()

    def test_calls_load_blueprint_when_prim_path_set(self):
        view = _make_view()
        stage = MagicMock()
        view._usdviewApi.stage = stage
        view._currentPrimPath = "/Root/MyBlueprint"
        view._isContainer = False

        _on_stage_replaced(view)

        view.loadBlueprint.assert_called_once_with(stage, "/Root/MyBlueprint")

    def test_calls_load_container_when_container(self):
        view = _make_view()
        stage = MagicMock()
        view._usdviewApi.stage = stage
        view._currentPrimPath = "/Root/MyContainer"
        view._isContainer = True

        _on_stage_replaced(view)

        view.loadContainer.assert_called_once_with(stage, "/Root/MyContainer")


class TestOnStageReplacedExceptionLogging(unittest.TestCase):
    """Reload exceptions must be logged, not silently swallowed."""

    def test_load_stage_exception_is_logged(self):
        view = _make_view()
        view._usdviewApi.stage = MagicMock()
        view.loadStage.side_effect = RuntimeError("stage broken")

        _on_stage_replaced(view)

        view._warnReloadFailed.assert_called_once()
        args = view._warnReloadFailed.call_args[0]
        self.assertIsInstance(args[0], RuntimeError)

    def test_load_stage_exception_still_schedules_finish(self):
        view = _make_view()
        view._usdviewApi.stage = MagicMock()
        view.loadStage.side_effect = RuntimeError("stage broken")

        _on_stage_replaced(view)

        view._scheduleFinishReload.assert_called_once()

    def test_defer_flag_reset_after_exception(self):
        view = _make_view()
        view._usdviewApi.stage = MagicMock()
        view.loadStage.side_effect = RuntimeError("stage broken")

        _on_stage_replaced(view)

        self.assertFalse(view._deferRenderCacheClear)


class TestFinishStageReload(unittest.TestCase):
    """_finishStageReload must re-enable updates and preserve initialized."""

    def test_sets_initialized_true(self):
        view = _make_view()
        view.initialized = False
        view._renderCacheClearPending = False

        _finish_stage_reload(view)

        self.assertTrue(view.initialized)

    def test_re_enables_updates(self):
        view = _make_view()
        view._renderCacheClearPending = False

        _finish_stage_reload(view)

        view.setUpdatesEnabled.assert_called_with(True)
        view.update.assert_called_once()

    def test_clears_pending_render_cache(self):
        view = _make_view()
        view._renderCacheClearPending = True

        _finish_stage_reload(view)

        view.makeCurrent.assert_called_once()
        view._nodeRenderManager.clearCache.assert_called_once()
        view.doneCurrent.assert_called_once()
        self.assertFalse(view._renderCacheClearPending)

    def test_sets_text_and_links_changed(self):
        view = _make_view()
        view._renderCacheClearPending = False

        _finish_stage_reload(view)

        self.assertTrue(view.linksChanged)
        self.assertTrue(view.textChanged)


class TestIsStageViewMidTeardown(unittest.TestCase):
    """_isStageViewMidTeardown detects when the stageView renderer
    is being torn down (updatesEnabled=False on stageView)."""

    def test_returns_false_when_no_api(self):
        view = MagicMock()
        view._usdviewApi = None
        self.assertFalse(_is_stage_view_mid_teardown(view))

    def test_returns_false_when_updates_enabled(self):
        view = MagicMock()
        sv = MagicMock()
        sv.updatesEnabled.return_value = True
        view._usdviewApi._UsdviewApi__appController._stageView = sv
        self.assertFalse(_is_stage_view_mid_teardown(view))

    def test_returns_true_when_updates_disabled(self):
        view = MagicMock()
        sv = MagicMock()
        sv.updatesEnabled.return_value = False
        view._usdviewApi._UsdviewApi__appController._stageView = sv
        self.assertTrue(_is_stage_view_mid_teardown(view))

    def test_returns_false_on_attribute_error(self):
        view = MagicMock()
        view._usdviewApi._UsdviewApi__appController = MagicMock(spec=[])
        self.assertFalse(_is_stage_view_mid_teardown(view))

    def test_returns_false_when_no_stage_view(self):
        view = MagicMock()
        view._usdviewApi._UsdviewApi__appController._stageView = None
        self.assertFalse(_is_stage_view_mid_teardown(view))


class TestInstallCloseStageGuard(unittest.TestCase):
    """_installCloseStageGuard wraps _closeStage to pre-clean VBOs."""

    def test_wraps_close_stage(self):
        manager = MagicMock()
        manager.getAllWindows.return_value = []
        api = MagicMock()
        controller = api._UsdviewApi__appController
        controller._noodles_close_guard = False
        original = MagicMock()
        controller._closeStage = original

        _install_close_stage_guard(manager, api)

        self.assertTrue(controller._noodles_close_guard)
        self.assertIsNot(controller._closeStage, original)

    def test_guard_cleans_vbos_then_calls_original(self):
        gv = MagicMock()
        window = MagicMock()
        window.graphView = gv
        manager = MagicMock()
        manager.getAllWindows.return_value = [window]

        api = MagicMock()
        controller = api._UsdviewApi__appController
        controller._noodles_close_guard = False
        original = MagicMock()
        controller._closeStage = original

        _install_close_stage_guard(manager, api)

        controller._closeStage()

        gv.setUpdatesEnabled.assert_called_with(False)
        gv.makeCurrent.assert_called_once()
        gv._nodeRenderManager.clearCache.assert_called_once()
        gv.doneCurrent.assert_called_once()
        original.assert_called_once()

    def test_guard_not_installed_twice(self):
        manager = MagicMock()
        api = MagicMock()
        controller = api._UsdviewApi__appController
        controller._noodles_close_guard = True
        original = controller._closeStage

        _install_close_stage_guard(manager, api)

        self.assertIs(controller._closeStage, original)

    def test_dormant_guard_no_side_effects(self):
        """When no noodles windows are open, the guard just calls original."""
        manager = MagicMock()
        manager.getAllWindows.return_value = []
        api = MagicMock()
        controller = api._UsdviewApi__appController
        controller._noodles_close_guard = False
        original = MagicMock()
        controller._closeStage = original

        _install_close_stage_guard(manager, api)
        controller._closeStage()

        original.assert_called_once()
