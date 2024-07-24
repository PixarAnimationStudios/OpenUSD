#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest

from pxr.Usdviewq.viewSettingsDataModel import (ClearColors, HighlightColors,
    ViewSettingsDataModel)
from pxr.Usdviewq.common import CameraMaskModes, SelectionHighlightModes
from pxr.UsdAppUtils.complexityArgs import RefinementComplexities


class SignalCounter(object):
    """Counts the number of times a signal is emitted."""

    def __init__(self, signal):

        signal.connect(self._signalEmitted)

        self._numSignals = 0

    def _signalEmitted(self, *args, **kwargs):
        """Fired when a signal is recieved. Increments the signal count."""

        self._numSignals += 1

    def getAndClearNumSignals(self):
        """Get the number of signals fired since the last call to
        getAndClearNumSignals().
        """

        numSignals = self._numSignals
        self._numSignals = 0
        return numSignals


class TestViewSettingsDataModel(unittest.TestCase):

    def test_DefaultMaterial(self):
        """Test getting/setting of default material settings and check that
        signalDefaultMaterialChanged is emitted properly.
        """

        vsDM = ViewSettingsDataModel(None, None)
        counter = SignalCounter(vsDM.signalDefaultMaterialChanged)


        # Test AMBIENT component.

        # Check default.
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.2)

        # Set ambient then check that it updated and signal fired.
        vsDM.defaultMaterialAmbient = 0.4
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set ambient to its current value then check that it did not change.
        # Even though the property was set it was not changed so
        # signalDefaultMaterialChanged should not have fired.
        vsDM.defaultMaterialAmbient = 0.4
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEqual(counter.getAndClearNumSignals(), 0)


        # Test SPECULAR component.

        # Check default.
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.1)

        # Set specular then check that it updated and signal fired.
        vsDM.defaultMaterialSpecular = 0.4
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

        # Set specular to its current value then check that it did not change.
        # Even though the property was set it was not changed so
        # signalDefaultMaterialChanged should not have fired.
        vsDM.defaultMaterialSpecular = 0.4
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEqual(counter.getAndClearNumSignals(), 0)


        # Check that setting both ambient and specular with setDefaultMaterial
        # works and emits signalDefaultMaterialChanged when at least one
        # changes.

        self.assertEqual(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.4)

        # No change, so signalDefaultMaterialChanged should not have fired.
        vsDM.setDefaultMaterial(0.4, 0.4)
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEqual(counter.getAndClearNumSignals(), 0)

        # Change each component separately. Should be 2 signals fired.
        vsDM.setDefaultMaterial(0.5, 0.4)
        vsDM.setDefaultMaterial(0.5, 0.5)
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.5)
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.5)
        self.assertEqual(counter.getAndClearNumSignals(), 2)

        # Change both components at the same time. Only 1 signal should be
        # fired.
        vsDM.setDefaultMaterial(0.6, 0.6)
        self.assertEqual(vsDM.defaultMaterialAmbient, 0.6)
        self.assertEqual(vsDM.defaultMaterialSpecular, 0.6)
        self.assertEqual(counter.getAndClearNumSignals(), 1)

    def test_Complexity(self):
        """Test that complexity must use the preset complexities."""

        vsDM = ViewSettingsDataModel(None, None)

        vsDM.complexity = RefinementComplexities.MEDIUM
        self.assertEqual(vsDM.complexity, RefinementComplexities.MEDIUM)

        # Users can't create their own complexities.
        with self.assertRaises(ValueError):
            vsDM.complexity = RefinementComplexities._RefinementComplexity(
                "none", "None", 1.5)
        self.assertEqual(vsDM.complexity, RefinementComplexities.MEDIUM)

        # Users can't set the float complexity directly.
        with self.assertRaises(ValueError):
            vsDM.complexity = 1.0
        self.assertEqual(vsDM.complexity, RefinementComplexities.MEDIUM)

    def test_ShowMask(self):
        """Test that updating the camera mask mode properly updates showMask and
        showMask_Opaque.
        """

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEqual(vsDM.cameraMaskMode, CameraMaskModes.NONE)
        self.assertEqual(vsDM.showMask, False)
        self.assertEqual(vsDM.showMask_Opaque, False)

        # Check PARTIAL mode.
        vsDM.cameraMaskMode = CameraMaskModes.PARTIAL
        self.assertEqual(vsDM.cameraMaskMode, CameraMaskModes.PARTIAL)
        self.assertEqual(vsDM.showMask, True)
        self.assertEqual(vsDM.showMask_Opaque, False)

        # Check FULL mode.
        vsDM.cameraMaskMode = CameraMaskModes.FULL
        self.assertEqual(vsDM.cameraMaskMode, CameraMaskModes.FULL)
        self.assertEqual(vsDM.showMask, True)
        self.assertEqual(vsDM.showMask_Opaque, True)

    def test_ClearColor(self):
        """Test that setting clearColorText changes the value of clearColor."""

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEqual(vsDM.clearColorText, ClearColors.DARK_GREY)
        self.assertEqual(vsDM.clearColor, (0.07074, 0.07074, 0.07074, 1.0))

        # Switch to a valid color.
        vsDM.clearColorText = ClearColors.BLACK
        self.assertEqual(vsDM.clearColorText, ClearColors.BLACK)
        self.assertEqual(vsDM.clearColor, (0.0, 0.0, 0.0, 1.0))

        # Switch to an invalid color.
        with self.assertRaises(ValueError):
            vsDM.clearColorText = "Octarine"
        self.assertEqual(vsDM.clearColorText, ClearColors.BLACK)
        self.assertEqual(vsDM.clearColor, (0.0, 0.0, 0.0, 1.0))

    def test_HighlightColor(self):
        """Test that setting highlightColorName changes the value of
        highlightColor.
        """

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEqual(vsDM.highlightColorName, HighlightColors.YELLOW)
        self.assertEqual(vsDM.highlightColor, (1.0, 1.0, 0.0, 0.5))

        # Switch to a valid color.
        vsDM.highlightColorName = HighlightColors.CYAN
        self.assertEqual(vsDM.highlightColorName, HighlightColors.CYAN)
        self.assertEqual(vsDM.highlightColor, (0.0, 1.0, 1.0, 0.5))

        # Switch to an invalid color.
        with self.assertRaises(ValueError):
            vsDM.highlightColorName = "Octarine"
        self.assertEqual(vsDM.highlightColorName, HighlightColors.CYAN)
        self.assertEqual(vsDM.highlightColor, (0.0, 1.0, 1.0, 0.5))


if __name__ == "__main__":
    unittest.main(verbosity=2)
