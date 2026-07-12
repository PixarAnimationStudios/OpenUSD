#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""Unit tests for KeyBindingDialog conflict detection.

Tests the find_shortcut_conflicts logic that detects duplicate key sequences
across remappable shortcuts and hardcoded non-remappable keys.

The production function lives in widgets/keyBindingDialog.py. This test
mirrors it as a shim (same pattern as testUsdNoodlesGraphViewShortcut.py)
because importing the widget module triggers PySide6/GL initialization
which is unavailable in headless CI.
"""

import unittest


_NON_REMAPPABLE_KEYS = [
    "Tab",
    "Space",
    "Ctrl+Z",
    "Ctrl+Shift+Z",
    "Ctrl+Y",
    "Delete",
    "Backspace",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "Shift+1",
    "Shift+2",
    "Shift+3",
    "Shift+4",
    "Shift+5",
    "Shift+6",
    "Shift+7",
    "Shift+8",
    "Shift+9",
]


def _find_shortcut_conflicts(bindings, non_remappable_keys):
    """Mirror of keyBindingDialog.find_shortcut_conflicts.

    Kept as a thin shim so the test exercises the same logic.
    If the production function changes, update this shim to match.
    """
    key_to_actions = {}

    for _configKey, display_name, key_str in bindings:
        if not key_str:
            continue
        if key_str not in key_to_actions:
            key_to_actions[key_str] = []
        key_to_actions[key_str].append(display_name)

    for non_remap_key in non_remappable_keys:
        if non_remap_key in key_to_actions:
            key_to_actions[non_remap_key].append(f"(hardcoded: {non_remap_key})")

    return {k: v for k, v in key_to_actions.items() if len(v) > 1}


class TestFindShortcutConflicts(unittest.TestCase):
    def test_no_conflicts(self):
        bindings = [
            ("k1", "Toggle Links", "Ctrl+Shift+L"),
            ("k2", "Toggle Nodes", "Ctrl+Shift+N"),
        ]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertEqual(result, {})

    def test_two_actions_same_key(self):
        bindings = [
            ("k1", "Toggle Links", "Ctrl+Shift+L"),
            ("k2", "Toggle Nodes", "Ctrl+Shift+L"),
        ]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertIn("Ctrl+Shift+L", result)
        self.assertEqual(len(result["Ctrl+Shift+L"]), 2)

    def test_conflict_with_hardcoded_tab(self):
        bindings = [("k1", "Toggle Links", "Tab")]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertIn("Tab", result)
        self.assertIn("(hardcoded: Tab)", result["Tab"])

    def test_empty_key_ignored(self):
        bindings = [("k1", "Toggle Links", ""), ("k2", "Toggle Nodes", "")]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertEqual(result, {})

    def test_conflict_with_delete(self):
        bindings = [("k1", "Select All", "Delete")]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertIn("Delete", result)

    def test_conflict_with_number_key(self):
        bindings = [("k1", "Frame Selection", "5")]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertIn("5", result)

    def test_three_way_conflict(self):
        bindings = [
            ("k1", "Action A", "F"),
            ("k2", "Action B", "F"),
            ("k3", "Action C", "F"),
        ]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertEqual(len(result["F"]), 3)

    def test_no_conflict_different_keys(self):
        bindings = [
            ("k1", "Action A", "A"),
            ("k2", "Action B", "B"),
            ("k3", "Action C", "C"),
        ]
        result = _find_shortcut_conflicts(bindings, _NON_REMAPPABLE_KEYS)
        self.assertEqual(result, {})
