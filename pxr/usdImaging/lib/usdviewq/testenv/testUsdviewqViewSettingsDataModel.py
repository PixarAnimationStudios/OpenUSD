#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

import unittest

from pxr.Usdviewq.viewSettingsDataModel import (ClearColors, HighlightColors,
    ViewSettingsDataModel)
from pxr.Usdviewq.common import (
    CameraMaskModes, SelectionHighlightModes, Complexities)


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
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.2)

        # Set ambient then check that it updated and signal fired.
        vsDM.defaultMaterialAmbient = 0.4
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEquals(counter.getAndClearNumSignals(), 1)

        # Set ambient to its current value then check that it did not change.
        # Even though the property was set it was not changed so
        # signalDefaultMaterialChanged should not have fired.
        vsDM.defaultMaterialAmbient = 0.4
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEquals(counter.getAndClearNumSignals(), 0)


        # Test SPECULAR component.

        # Check default.
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.1)

        # Set specular then check that it updated and signal fired.
        vsDM.defaultMaterialSpecular = 0.4
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEquals(counter.getAndClearNumSignals(), 1)

        # Set specular to its current value then check that it did not change.
        # Even though the property was set it was not changed so
        # signalDefaultMaterialChanged should not have fired.
        vsDM.defaultMaterialSpecular = 0.4
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEquals(counter.getAndClearNumSignals(), 0)


        # Check that setting both ambient and specular with setDefaultMaterial
        # works and emits signalDefaultMaterialChanged when at least one
        # changes.

        self.assertEquals(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.4)

        # No change, so signalDefaultMaterialChanged should not have fired.
        vsDM.setDefaultMaterial(0.4, 0.4)
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.4)
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.4)
        self.assertEquals(counter.getAndClearNumSignals(), 0)

        # Change each component separately. Should be 2 signals fired.
        vsDM.setDefaultMaterial(0.5, 0.4)
        vsDM.setDefaultMaterial(0.5, 0.5)
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.5)
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.5)
        self.assertEquals(counter.getAndClearNumSignals(), 2)

        # Change both components at the same time. Only 1 signal should be
        # fired.
        vsDM.setDefaultMaterial(0.6, 0.6)
        self.assertEquals(vsDM.defaultMaterialAmbient, 0.6)
        self.assertEquals(vsDM.defaultMaterialSpecular, 0.6)
        self.assertEquals(counter.getAndClearNumSignals(), 1)

    def test_Complexity(self):
        """Test that complexity must use the preset complexities."""

        vsDM = ViewSettingsDataModel(None, None)

        vsDM.complexity = Complexities.MEDIUM
        self.assertEquals(vsDM.complexity, Complexities.MEDIUM)

        # Users can't create their own complexities.
        with self.assertRaises(ValueError):
            vsDM.complexity = Complexities._Complexity("none", "None", 1.5)
        self.assertEquals(vsDM.complexity, Complexities.MEDIUM)

        # Users can't set the float complexity directly.
        with self.assertRaises(ValueError):
            vsDM.complexity = 1.0
        self.assertEquals(vsDM.complexity, Complexities.MEDIUM)

    def test_ShowMask(self):
        """Test that updating the camera mask mode properly updates showMask and
        showMask_Opaque.
        """

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEquals(vsDM.cameraMaskMode, CameraMaskModes.NONE)
        self.assertEquals(vsDM.showMask, False)
        self.assertEquals(vsDM.showMask_Opaque, False)

        # Check PARTIAL mode.
        vsDM.cameraMaskMode = CameraMaskModes.PARTIAL
        self.assertEquals(vsDM.cameraMaskMode, CameraMaskModes.PARTIAL)
        self.assertEquals(vsDM.showMask, True)
        self.assertEquals(vsDM.showMask_Opaque, False)

        # Check FULL mode.
        vsDM.cameraMaskMode = CameraMaskModes.FULL
        self.assertEquals(vsDM.cameraMaskMode, CameraMaskModes.FULL)
        self.assertEquals(vsDM.showMask, True)
        self.assertEquals(vsDM.showMask_Opaque, True)

    def test_ClearColor(self):
        """Test that setting clearColorText changes the value of clearColor."""

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEquals(vsDM.clearColorText, ClearColors.DARK_GREY)
        self.assertEquals(vsDM.clearColor, (0.3, 0.3, 0.3, 0.0))

        # Switch to a valid color.
        vsDM.clearColorText = ClearColors.BLACK
        self.assertEquals(vsDM.clearColorText, ClearColors.BLACK)
        self.assertEquals(vsDM.clearColor, (0.0, 0.0, 0.0, 0.0))

        # Switch to an invalid color.
        with self.assertRaises(ValueError):
            vsDM.clearColorText = "Octarine"
        self.assertEquals(vsDM.clearColorText, ClearColors.BLACK)
        self.assertEquals(vsDM.clearColor, (0.0, 0.0, 0.0, 0.0))

    def test_HighlightColor(self):
        """Test that setting highlightColorName changes the value of
        highlightColor.
        """

        vsDM = ViewSettingsDataModel(None, None)

        # Check default.
        self.assertEquals(vsDM.highlightColorName, HighlightColors.YELLOW)
        self.assertEquals(vsDM.highlightColor, (1.0, 1.0, 0.0, 0.5))

        # Switch to a valid color.
        vsDM.highlightColorName = HighlightColors.CYAN
        self.assertEquals(vsDM.highlightColorName, HighlightColors.CYAN)
        self.assertEquals(vsDM.highlightColor, (0.0, 1.0, 1.0, 0.5))

        # Switch to an invalid color.
        with self.assertRaises(ValueError):
            vsDM.highlightColorName = "Octarine"
        self.assertEquals(vsDM.highlightColorName, HighlightColors.CYAN)
        self.assertEquals(vsDM.highlightColor, (0.0, 1.0, 1.0, 0.5))


if __name__ == "__main__":
    unittest.main(verbosity=2)
