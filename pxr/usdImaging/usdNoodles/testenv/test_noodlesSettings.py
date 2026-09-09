#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for ``NoodlesSettingsDataModel._migrateLegacyGlobalNodeScale``.

The migration runs once during ``__init__`` (before properties are created)
to upgrade settings dicts saved by older versions: it folds a saved
``globalNodeScale`` multiplier into each per-element node-size value and
drops the obsolete key. These tests pin its branching behavior directly
against the saved-state dict, without spinning up Qt or the full
``StateSource`` machinery.

We invoke the migration as an unbound method against a lightweight stub
whose ``_getState()`` returns a pre-populated dict. This keeps the tests
fast (no Qt event loop, no real ``~/.usdview/state.json``) and makes the
mutation explicit: the dict the stub hands back is the dict the test
inspects after the call.
"""

import sys
import types
import unittest


def _ensure_usdviewq_stubs():
    """Pre-populate ``sys.modules`` with minimal stubs for pxr.Usdviewq.

    The ``noodles_pytest`` Buck target deliberately avoids the pyusdviewq
    runtime dep (only ``noodles_headless_pytest`` ships it). Importing
    ``noodlesSettings`` would otherwise fail at module-load time on
    ``from pxr.Usdviewq.qt import QtCore, QtGui`` /
    ``from pxr.Usdviewq.settings import StateSource``. Since the migration
    under test never instantiates ``StateSource`` (we drive it as an
    unbound method against a stub), shimming these symbols is enough.

    We only install stubs for modules NOT already importable, so a future
    BUCK change that adds the real dep does not get masked. ``pxr`` itself
    is a namespace package — we import it (rather than synthesize a stub)
    so that ``pxr.UsdNoodles`` continues to resolve through the real
    package finder.
    """
    import pxr  # noqa: F401 — populate the real pxr namespace package

    if "pxr.Usdviewq" not in sys.modules:
        usdviewq = types.ModuleType("pxr.Usdviewq")
        sys.modules["pxr.Usdviewq"] = usdviewq
        sys.modules["pxr"].Usdviewq = usdviewq  # type: ignore[attr-defined]
    if "pxr.Usdviewq.qt" not in sys.modules:
        qt = types.ModuleType("pxr.Usdviewq.qt")

        class _Signal:
            def __init__(self, *_args, **_kwargs):
                pass

            def emit(self, *_args, **_kwargs):
                pass

        class _QObject:
            def __init__(self, *_args, **_kwargs):
                pass

        qt.QtCore = types.SimpleNamespace(QObject=_QObject, Signal=_Signal)
        qt.QtGui = types.SimpleNamespace(
            QKeySequence=lambda *_a, **_kw: types.SimpleNamespace(isEmpty=lambda: False)
        )
        sys.modules["pxr.Usdviewq.qt"] = qt
    if "pxr.Usdviewq.settings" not in sys.modules:
        settings = types.ModuleType("pxr.Usdviewq.settings")

        class _StateSource:
            def __init__(self, *_args, **_kwargs):
                pass

        settings.StateSource = _StateSource
        sys.modules["pxr.Usdviewq.settings"] = settings


_ensure_usdviewq_stubs()

try:
    from pxr.UsdNoodles.noodlesSettings import NoodlesSettingsDataModel

    _has_module = True
    _import_error = None
except ImportError as e:  # pragma: no cover - environment-dependent
    _has_module = False
    _import_error = e


class _StateStub(NoodlesSettingsDataModel):
    """Stand-in for a fully constructed ``NoodlesSettingsDataModel``.

    Subclasses the real class so class-level attributes
    (``_SCALE_FOLDED_SIZE_KEYS``, ``SETTINGS_SCHEMA``) and the migration
    method itself are inherited as-is, but skips ``__init__`` (which
    requires Qt + StateSource) by overriding it. Only ``_getState`` is
    exercised by the migration.
    """

    def __init__(self, state):  # pylint: disable=super-init-not-called
        self._state = state

    def _getState(self):
        return self._state


def _migrate(state):
    """Run the migration against ``state`` and return the (mutated) dict."""
    stub = _StateStub(state)
    stub._migrateLegacyGlobalNodeScale()
    return stub._state


@unittest.skipUnless(_has_module, f"pxr.UsdNoodles not available: {_import_error}")
class TestMigrateLegacyGlobalNodeScale(unittest.TestCase):
    """Branch-by-branch coverage for the legacy globalNodeScale migration."""

    def test_no_op_when_key_absent(self):
        # Two early-return paths (empty state, key missing) must leave the
        # dict untouched. If either branch silently re-folded sizes, every
        # subsequent app launch would double up the saved values.
        empty = _migrate({})
        self.assertEqual(empty, {})

        intact = {
            "nodeTitleFontSize": 48.0,
            "nodePinFontSize": 36.0,
            "selectedNodeStrokeWidth": 8.0,
            "unrelatedKey": "should-survive",
        }
        result = _migrate(dict(intact))
        self.assertEqual(result, intact)

    def test_legacy_default_scale_two_folds_into_new_defaults(self):
        # The canonical migration: a user on the old defaults
        # (globalNodeScale=2.0, sizes at the old half-size base) must
        # emerge with sizes equal to the new post-diff defaults so the
        # on-screen rendering is unchanged. Every key in
        # _SCALE_FOLDED_SIZE_KEYS gets folded; globalNodeScale is dropped.
        state = {
            "globalNodeScale": 2.0,
            "nodeTitleFontSize": 24.0,
            "nodePinFontSize": 18.0,
            "nodePinTypeFontSize": 14.0,
            "nodeMarginH": 16.0,
            "nodeMarginV": 18.0,
            "nodePortSpacing": 1.0,
            "nodePortWidth": 16.0,
            "nodeCornerRadius": 10.0,
            "selectedNodeStrokeWidth": 4.0,
            "linkLineWidth": 10.0,  # unrelated key must survive untouched
        }
        result = _migrate(state)
        self.assertNotIn("globalNodeScale", result)
        self.assertEqual(result["nodeTitleFontSize"], 48.0)
        self.assertEqual(result["nodePinFontSize"], 36.0)
        self.assertEqual(result["nodePinTypeFontSize"], 28.0)
        self.assertEqual(result["nodeMarginH"], 32.0)
        self.assertEqual(result["nodeMarginV"], 36.0)
        self.assertEqual(result["nodePortSpacing"], 2.0)
        self.assertEqual(result["nodePortWidth"], 32.0)
        self.assertEqual(result["nodeCornerRadius"], 20.0)
        self.assertEqual(result["selectedNodeStrokeWidth"], 8.0)
        self.assertEqual(result["linkLineWidth"], 10.0)

    def test_non_default_scale_preserves_on_screen_size(self):
        # The actual footgun the diff calls out: a user with a persisted
        # scale of 5.0 and the old default sizes had nodes *sized* for
        # scale 5 but *drawn* at scale 2. Folding 5.0 into 24.0 must yield
        # 120.0 — the size that was being computed before — NOT the new
        # default 48.0. Also pins int-typed scales (JSON whole-number
        # round-trip) as accepted by the isinstance check.
        float_scale = _migrate({"globalNodeScale": 5.0, "nodeTitleFontSize": 24.0})
        self.assertNotIn("globalNodeScale", float_scale)
        self.assertEqual(float_scale["nodeTitleFontSize"], 120.0)

        int_scale = _migrate({"globalNodeScale": 3, "nodePinFontSize": 18.0})
        self.assertNotIn("globalNodeScale", int_scale)
        self.assertEqual(int_scale["nodePinFontSize"], 54.0)

    def test_invalid_scale_drops_key_without_folding(self):
        # Pathological globalNodeScale values: hand-edited string, zero,
        # negative. All three must (a) drop the obsolete key (state.pop
        # runs unconditionally before the type/range check) and (b) leave
        # saved sizes alone — multiplying by a string would raise, and
        # multiplying by 0 or a negative would silently destroy the
        # graph. This bundles three subTest variants because they share a
        # single branch shape and would otherwise dilute the file.
        cases = [
            ("string scale", "2.0"),
            ("zero scale", 0),
            ("negative scale", -2.0),
        ]
        for label, bad_scale in cases:
            with self.subTest(label):
                result = _migrate(
                    {"globalNodeScale": bad_scale, "nodeTitleFontSize": 24.0}
                )
                self.assertNotIn("globalNodeScale", result)
                self.assertEqual(result["nodeTitleFontSize"], 24.0)

    def test_scale_folded_size_keys_matches_doubled_node_settings(self):
        # Schema-vs-migration contract: every "nodes"-category setting
        # whose new default is exactly 2x its pre-diff default must
        # appear in _SCALE_FOLDED_SIZE_KEYS, otherwise an upgrading
        # user's saved value for that key silently un-doubles. This guard
        # catches future schema additions that forget to update the
        # migration list. Keys NOT doubled by the diff (font sizes in
        # statusOverlay, minimap settings, link widths, etc.) must NOT
        # appear here — folding them would re-scale unrelated dimensions.
        expected = {
            "nodeTitleFontSize",
            "nodePinFontSize",
            "nodePinTypeFontSize",
            "nodeMarginH",
            "nodeMarginV",
            "nodePortSpacing",
            "nodePortWidth",
            "nodeCornerRadius",
            "selectedNodeStrokeWidth",
        }
        self.assertEqual(
            set(NoodlesSettingsDataModel._SCALE_FOLDED_SIZE_KEYS), expected
        )


@unittest.skipUnless(_has_module, f"pxr.UsdNoodles not available: {_import_error}")
class TestSettingsSchema(unittest.TestCase):
    """Pin the showPinTypeLabels schema entry (default + validator)."""

    def _find(self, name):
        for entries in NoodlesSettingsDataModel.SETTINGS_SCHEMA.values():
            for entry in entries:
                if entry[0] == name:
                    return entry
        return None

    def test_show_pin_type_labels_default_and_validator(self):
        entry = self._find("showPinTypeLabels")
        self.assertIsNotNone(entry, "showPinTypeLabels missing from SETTINGS_SCHEMA")
        _name, default, validator, _desc = entry
        self.assertIs(default, True)  # default preserves the original appearance
        self.assertTrue(validator(True))
        self.assertTrue(validator(False))
        self.assertFalse(validator("not-a-bool"))
