#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for GraphView._shouldInterceptShortcut.

Tests every branch of the shortcut interception logic that prevents
usdview's ApplicationShortcut-scoped actions (e.g. 'J' for Toggle
Framed View) from firing while Noodles has focus.

GraphView cannot be imported in headless CI (GL context init hangs),
so we call the method through a lightweight shim that uses the same
Qt constant values.  The shim is kept in lockstep with the production
code via a docstring cross-reference to graphView.py:3745.
"""

import unittest
from unittest.mock import MagicMock

# Standard Qt enum values — stable across PySide2/6 and PyQt5/6.
# Using raw ints avoids importing pxr.Usdviewq.qt (which pulls in GL).
_KEY_TAB = 0x01000001
_KEY_A = 0x41
_KEY_J = 0x4A
_KEY_X = 0x58
_KEY_F = 0x46
_KEY_5 = 0x35
_KEY_F1 = 0x01000030
_KEY_ESCAPE = 0x01000000
_KEY_RETURN = 0x01000004

_NO_MODIFIER = 0x00000000
_SHIFT_MODIFIER = 0x02000000
_CTRL_MODIFIER = 0x04000000
_ALT_MODIFIER = 0x08000000
_KEYPAD_MODIFIER = 0x20000000


def _should_intercept_shortcut(view, event):
    """Mirror of GraphView._shouldInterceptShortcut (graphView.py:3745).

    Kept as a thin shim so the test exercises the same branching logic.
    If the production method changes, update this shim to match.
    """
    if view.nodeCreationHotbox.is_showing:
        return True

    if event.key() == _KEY_TAB:
        return True

    modifiers = event.modifiers() & ~_KEYPAD_MODIFIER
    if modifiers in (_NO_MODIFIER, _SHIFT_MODIFIER):
        text = event.text()
        if text and text.isprintable():
            return True

    return False


def _make_event(key, modifiers=_NO_MODIFIER, text=""):
    """Create a mock QKeyEvent with the given key, modifiers, and text."""
    event = MagicMock()
    event.key.return_value = key
    event.modifiers.return_value = modifiers
    event.text.return_value = text
    return event


def _make_view(hotbox_showing=False):
    """Create a minimal mock standing in for GraphView."""
    view = MagicMock(spec=[])
    view.nodeCreationHotbox = MagicMock()
    view.nodeCreationHotbox.is_showing = hotbox_showing
    return view


class TestShouldInterceptShortcut(unittest.TestCase):
    """Tests for GraphView._shouldInterceptShortcut branch coverage.

    Branch map (graphView.py:3745-3771):
      1. hotbox showing  → True
      2. Tab key          → True
      3. no-mod printable → True
      4. Shift+printable  → True
      5. Ctrl/Alt combo   → False
      6. non-printable    → False
      7. KeypadModifier   → stripped, then re-evaluated
    """

    # -- Branch 1: hotbox showing → intercept ALL keys --

    def test_hotbox_showing_intercepts_printable_key(self):
        view = _make_view(hotbox_showing=True)
        event = _make_event(_KEY_J, text="j")
        self.assertTrue(_should_intercept_shortcut(view, event))

    def test_hotbox_showing_intercepts_modifier_combo(self):
        view = _make_view(hotbox_showing=True)
        event = _make_event(_KEY_X, _CTRL_MODIFIER)
        self.assertTrue(_should_intercept_shortcut(view, event))

    # -- Branch 2: Tab always intercepted --

    def test_tab_intercepted_when_hotbox_hidden(self):
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_TAB)
        self.assertTrue(_should_intercept_shortcut(view, event))

    # -- Branch 3: unmodified printable key intercepted --

    def test_unmodified_printable_key_intercepted(self):
        """'j' with no modifier — prevents usdview 'J' shortcut."""
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_J, _NO_MODIFIER, text="j")
        self.assertTrue(_should_intercept_shortcut(view, event))

    # -- Branch 4: Shift + printable key intercepted --

    def test_shift_printable_key_intercepted(self):
        """Shift+A producing 'A' is intercepted."""
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_A, _SHIFT_MODIFIER, text="A")
        self.assertTrue(_should_intercept_shortcut(view, event))

    # -- Branch 5: Ctrl / Alt combos NOT intercepted --

    def test_ctrl_key_not_intercepted(self):
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_X, _CTRL_MODIFIER)
        self.assertFalse(_should_intercept_shortcut(view, event))

    def test_alt_key_not_intercepted(self):
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_F, _ALT_MODIFIER)
        self.assertFalse(_should_intercept_shortcut(view, event))

    def test_ctrl_shift_not_intercepted(self):
        """Ctrl+Shift+A must pass through to usdview."""
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_A, _CTRL_MODIFIER | _SHIFT_MODIFIER)
        self.assertFalse(_should_intercept_shortcut(view, event))

    # -- Branch 6: non-printable keys without modifiers NOT intercepted --

    def test_escape_not_intercepted(self):
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_ESCAPE)
        self.assertFalse(_should_intercept_shortcut(view, event))

    def test_function_key_not_intercepted(self):
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_F1)
        self.assertFalse(_should_intercept_shortcut(view, event))

    def test_return_key_not_intercepted(self):
        r"""Return key text ('\r') is not printable — returns False."""
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_RETURN, _NO_MODIFIER, text="\r")
        self.assertFalse(_should_intercept_shortcut(view, event))

    # -- Branch 7: KeypadModifier stripped before check --

    def test_keypad_modifier_stripped_printable_intercepted(self):
        """Numpad '5' has KeypadModifier but should still be intercepted."""
        view = _make_view(hotbox_showing=False)
        event = _make_event(_KEY_5, _KEYPAD_MODIFIER, text="5")
        self.assertTrue(_should_intercept_shortcut(view, event))


def _resolve_shortcut_conflicts(shortcuts, settings):
    """Mirror of GraphView._resolveShortcutConflicts.

    Walks the shortcuts list, normalizes each loaded shortcut string via the
    settings.normalize() callback, and resets any earlier configKey whose
    loaded value collides with a later configKey's value to its schema
    default. Kept in lockstep with graphView.py:_resolveShortcutConflicts.
    """
    if settings is None:
        return

    loaded = []
    for _name, configKey, *_rest in shortcuts:
        shortcutStr = settings.get(configKey, "")
        normalized = settings.normalize(shortcutStr) if shortcutStr else ""
        loaded.append((configKey, normalized))

    seen = {}
    for configKey, normalized in reversed(loaded):
        if not normalized:
            continue
        if normalized in seen:
            info = settings.getSettingInfo(configKey)
            if not info:
                continue
            defaultStr = info.get("default", "")
            defaultNormalized = settings.normalize(defaultStr) if defaultStr else ""
            if defaultNormalized == normalized:
                continue
            settings.set(configKey, defaultStr)
        else:
            seen[normalized] = configKey


class _FakeSettings:
    """Minimal fake of NoodlesConfig that records set() calls."""

    def __init__(self, values, defaults):
        self._values = dict(values)
        self._defaults = dict(defaults)
        self.set_calls = []

    def get(self, key, fallback=""):
        return self._values.get(key, fallback)

    def set(self, key, value):
        self._values[key] = value
        self.set_calls.append((key, value))

    def getSettingInfo(self, key):
        if key not in self._defaults:
            return None
        return {"name": key, "default": self._defaults[key]}

    @staticmethod
    def normalize(value):
        # Production code uses QKeySequence.toString() for normalization.
        # For the shim we just upper-case so "d" and "D" compare equal.
        return value.upper()


class TestResolveShortcutConflicts(unittest.TestCase):
    """Tests for GraphView._resolveShortcutConflicts migration logic."""

    def test_no_settings_is_noop(self):
        # Should not raise when settings is None.
        _resolve_shortcut_conflicts([("Toggle", "shortcutToggle")], None)

    def test_no_conflict_leaves_values_alone(self):
        settings = _FakeSettings(
            values={"shortcutA": "D", "shortcutB": "G"},
            defaults={"shortcutA": "D", "shortcutB": "G"},
        )
        shortcuts = [("A", "shortcutA"), ("B", "shortcutB")]
        _resolve_shortcut_conflicts(shortcuts, settings)
        self.assertEqual(settings.set_calls, [])

    def test_conflict_resets_earlier_to_default(self):
        # User has shortcutToggle = "D" but the new shortcutRemove default is
        # also "D". The earlier entry should be reset to its new default "G".
        settings = _FakeSettings(
            values={"shortcutToggle": "D", "shortcutRemove": "D"},
            defaults={"shortcutToggle": "G", "shortcutRemove": "D"},
        )
        shortcuts = [
            ("Toggle Link Dimming", "shortcutToggle"),
            ("Remove from Graph", "shortcutRemove"),
        ]
        _resolve_shortcut_conflicts(shortcuts, settings)
        self.assertEqual(settings.set_calls, [("shortcutToggle", "G")])

    def test_conflict_skipped_when_default_would_not_free_key(self):
        # If resetting to default would keep the same key, skip to avoid
        # looping the same conflict on every launch.
        settings = _FakeSettings(
            values={"shortcutA": "D", "shortcutB": "D"},
            defaults={"shortcutA": "D", "shortcutB": "D"},
        )
        shortcuts = [("A", "shortcutA"), ("B", "shortcutB")]
        _resolve_shortcut_conflicts(shortcuts, settings)
        self.assertEqual(settings.set_calls, [])

    def test_empty_shortcut_is_ignored(self):
        # Empty stored value (user cleared the shortcut) is never a conflict.
        settings = _FakeSettings(
            values={"shortcutA": "", "shortcutB": "D"},
            defaults={"shortcutA": "G", "shortcutB": "D"},
        )
        shortcuts = [("A", "shortcutA"), ("B", "shortcutB")]
        _resolve_shortcut_conflicts(shortcuts, settings)
        self.assertEqual(settings.set_calls, [])

    def test_latest_definition_wins_across_three_entries(self):
        # Three entries all claim "D"; only the latest should keep it, the
        # other two get reset to their respective defaults.
        settings = _FakeSettings(
            values={"shortcutA": "D", "shortcutB": "D", "shortcutC": "D"},
            defaults={"shortcutA": "X", "shortcutB": "Y", "shortcutC": "D"},
        )
        shortcuts = [("A", "shortcutA"), ("B", "shortcutB"), ("C", "shortcutC")]
        _resolve_shortcut_conflicts(shortcuts, settings)
        self.assertEqual(
            sorted(settings.set_calls),
            sorted([("shortcutA", "X"), ("shortcutB", "Y")]),
        )
