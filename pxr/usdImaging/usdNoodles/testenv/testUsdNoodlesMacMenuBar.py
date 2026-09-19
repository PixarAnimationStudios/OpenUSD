#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# pyre-strict

import sys
import unittest
from unittest.mock import MagicMock, patch

# _MacMenuBarSwapper and _NoodlesClickFilter are only instantiated when
# _IS_MACOS is True in production (_qt_editor.py). Gate the Qt import on
# platform too — on non-macOS configs `from pxr.Usdviewq.qt import ...` can
# hang the entire test module load (triggering a 10-minute timeout) before
# the @skipUnless decorator below has a chance to evaluate.
_IS_MACOS = sys.platform == "darwin"

if _IS_MACOS:
    try:
        from pxr.Usdviewq.qt import QtCore, QtWidgets

        _has_qt = True
    except ImportError:
        _has_qt = False
else:
    _has_qt = False

MODULE = "pxr.UsdNoodles._qt_editor"


def _make_action(text: str) -> MagicMock:
    action = MagicMock()
    action.text.return_value = text
    action.setVisible = MagicMock()
    return action


def _make_main_window() -> MagicMock:
    main_window = MagicMock()
    file_action = _make_action("&File")
    edit_action = _make_action("&Edit")
    window_action = _make_action("Window")
    menu_bar = MagicMock()
    menu_bar.actions.return_value = [file_action, edit_action, window_action]
    main_window.menuBar.return_value = menu_bar
    return main_window


@unittest.skipUnless(_has_qt and _IS_MACOS, "macOS-only")
class MacMenuBarSwapperTest(unittest.TestCase):
    def _make_swapper(self) -> "tuple":
        from pxr.UsdNoodles._qt_editor import _MacMenuBarSwapper

        main_window = _make_main_window()
        swapper = _MacMenuBarSwapper(main_window)
        return swapper, main_window

    def test_init_finds_edit_and_window_actions(self) -> None:
        swapper, _ = self._make_swapper()
        self.assertIsNotNone(swapper._usdviewEditAction)
        self.assertIsNotNone(swapper._usdviewWindowAction)
        self.assertEqual(swapper._usdviewEditAction.text(), "&Edit")
        self.assertEqual(swapper._usdviewWindowAction.text(), "Window")

    def test_activate_noodles_hides_usdview_shows_noodles(self) -> None:
        swapper, _ = self._make_swapper()
        noodles_actions = [_make_action("&Edit"), _make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)

        swapper.activateNoodles()

        swapper._usdviewEditAction.setVisible.assert_called_with(False)
        swapper._usdviewWindowAction.setVisible.assert_called_with(False)
        for action in noodles_actions:
            action.setVisible.assert_called_with(True)
        self.assertTrue(swapper._noodlesActive)

    def test_activate_noodles_is_idempotent(self) -> None:
        swapper, _ = self._make_swapper()
        noodles_actions = [_make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)

        swapper.activateNoodles()
        noodles_actions[0].setVisible.reset_mock()
        swapper.activateNoodles()

        noodles_actions[0].setVisible.assert_not_called()

    def test_activate_usdview_shows_usdview_hides_noodles(self) -> None:
        swapper, _ = self._make_swapper()
        noodles_actions = [_make_action("&Edit"), _make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)

        swapper.activateNoodles()
        swapper._usdviewEditAction.setVisible.reset_mock()
        swapper._usdviewWindowAction.setVisible.reset_mock()
        swapper.activateUsdview()

        for action in noodles_actions:
            action.setVisible.assert_called_with(False)
        swapper._usdviewEditAction.setVisible.assert_called_with(True)
        swapper._usdviewWindowAction.setVisible.assert_called_with(True)
        self.assertFalse(swapper._noodlesActive)

    def test_activate_usdview_is_idempotent(self) -> None:
        swapper, _ = self._make_swapper()
        swapper.activateUsdview()
        swapper._usdviewEditAction.setVisible.assert_not_called()

    def test_cleanup_restores_usdview_and_removes_actions(self) -> None:
        swapper, main_window = self._make_swapper()
        menu_bar = main_window.menuBar()
        noodles_actions = [_make_action("&Edit"), _make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)
        swapper.activateNoodles()

        swapper.cleanup()

        self.assertFalse(swapper._noodlesActive)
        for action in noodles_actions:
            menu_bar.removeAction.assert_any_call(action)
        self.assertEqual(swapper._noodlesMenuActions, [])

    def test_cleanup_when_already_usdview(self) -> None:
        swapper, _ = self._make_swapper()
        noodles_actions = [_make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)

        swapper.cleanup()

        self.assertFalse(swapper._noodlesActive)
        self.assertEqual(swapper._noodlesMenuActions, [])

    def test_init_handles_missing_edit_menu(self) -> None:
        from pxr.UsdNoodles._qt_editor import _MacMenuBarSwapper

        main_window = MagicMock()
        menu_bar = MagicMock()
        menu_bar.actions.return_value = [_make_action("&File")]
        main_window.menuBar.return_value = menu_bar

        swapper = _MacMenuBarSwapper(main_window)

        self.assertIsNone(swapper._usdviewEditAction)
        self.assertIsNone(swapper._usdviewWindowAction)

    def test_activate_noodles_with_missing_usdview_actions(self) -> None:
        from pxr.UsdNoodles._qt_editor import _MacMenuBarSwapper

        main_window = MagicMock()
        menu_bar = MagicMock()
        menu_bar.actions.return_value = []
        main_window.menuBar.return_value = menu_bar

        swapper = _MacMenuBarSwapper(main_window)
        noodles_actions = [_make_action("&View")]
        swapper.setNoodlesMenuActions(noodles_actions)

        swapper.activateNoodles()
        noodles_actions[0].setVisible.assert_called_with(True)
        self.assertTrue(swapper._noodlesActive)


@unittest.skipUnless(_has_qt and _IS_MACOS, "macOS-only")
class NoodlesClickFilterTest(unittest.TestCase):
    def _make_filter(self) -> "tuple":
        from pxr.UsdNoodles._qt_editor import _NoodlesClickFilter

        editor_widget = MagicMock(spec=QtWidgets.QWidget)
        editor_widget.isAncestorOf = MagicMock(return_value=False)
        filt = _NoodlesClickFilter(editor_widget)
        return filt, editor_widget

    def test_non_mouse_event_ignored(self) -> None:
        filt, _ = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseMove

        result = filt.eventFilter(MagicMock(spec=QtWidgets.QWidget), event)

        self.assertFalse(result)

    def test_non_widget_ignored(self) -> None:
        filt, _ = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress

        result = filt.eventFilter(MagicMock(spec=QtCore.QObject), event)

        self.assertFalse(result)

    def test_qmenu_click_ignored(self) -> None:
        filt, _ = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress
        menu = MagicMock(spec=QtWidgets.QMenu)

        result = filt.eventFilter(menu, event)

        self.assertFalse(result)

    def test_click_on_editor_activates_noodles(self) -> None:
        filt, editor_widget = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress

        swapper = MagicMock()
        manager = MagicMock()
        manager._macSwapper = swapper

        with patch(f"{MODULE}.GetEditorManager", return_value=manager):
            result = filt.eventFilter(editor_widget, event)

        swapper.activateNoodles.assert_called_once()
        self.assertFalse(result)

    def test_click_on_child_activates_noodles(self) -> None:
        filt, editor_widget = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress
        child = MagicMock(spec=QtWidgets.QWidget)
        editor_widget.isAncestorOf.return_value = True

        swapper = MagicMock()
        manager = MagicMock()
        manager._macSwapper = swapper

        with patch(f"{MODULE}.GetEditorManager", return_value=manager):
            result = filt.eventFilter(child, event)

        swapper.activateNoodles.assert_called_once()
        self.assertFalse(result)

    def test_click_outside_activates_usdview(self) -> None:
        filt, editor_widget = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress
        other_widget = MagicMock(spec=QtWidgets.QWidget)
        editor_widget.isAncestorOf.return_value = False

        swapper = MagicMock()
        manager = MagicMock()
        manager._macSwapper = swapper

        with patch(f"{MODULE}.GetEditorManager", return_value=manager):
            result = filt.eventFilter(other_widget, event)

        swapper.activateUsdview.assert_called_once()
        self.assertFalse(result)

    def test_no_swapper_is_noop(self) -> None:
        filt, _ = self._make_filter()
        event = MagicMock()
        event.type.return_value = QtCore.QEvent.MouseButtonPress
        widget = MagicMock(spec=QtWidgets.QWidget)

        manager = MagicMock()
        manager._macSwapper = None

        with patch(f"{MODULE}.GetEditorManager", return_value=manager):
            result = filt.eventFilter(widget, event)

        self.assertFalse(result)
