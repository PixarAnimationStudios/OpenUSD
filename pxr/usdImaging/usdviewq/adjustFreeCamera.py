#
# Copyright 2021 Pixar
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
from .qt import QtCore, QtGui, QtWidgets
from .adjustFreeCameraUI import Ui_AdjustFreeCamera
from .common import FixableDoubleValidator

from pxr import Gf, UsdGeom

class AdjustFreeCamera(QtWidgets.QDialog):

    """Dialog to adjust the free camera settings (clipping, fov, aspect ratio).
    """
    def __init__(self, parent, dataModel, signalFrustumChanged):
        QtWidgets.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustFreeCamera()
        self._ui.setupUi(self)

        self._dataModel = dataModel
        self._signalFrustumChanged = signalFrustumChanged

        self._signalFrustumChanged.connect(self._frustumChanged)

        # When the checkboxes change, we want to update instantly
        self._ui.overrideNear.stateChanged.connect(self._overrideNearToggled)

        self._ui.overrideFar.stateChanged.connect(self._overrideFarToggled)

        # we also want to update the clipping planes as the user is typing
        self._ui.nearSpinBox.valueChanged.connect(self._nearChanged)

        self._ui.farSpinBox.valueChanged.connect(self._farChanged)

        # Set the checkboxes to their initial state
        self._ui.overrideNear.setChecked(
            self._dataModel.viewSettings.freeCameraOverrideNear is not None)
        self._ui.overrideFar.setChecked(
            self._dataModel.viewSettings.freeCameraOverrideFar is not None)

        # load the initial values for the text boxes, but first deactivate them
        # if their corresponding checkbox is off.
        clipRange = self._getCurrentClippingRange()
        self._ui.nearSpinBox.setEnabled(self._ui.overrideNear.isChecked())
        self._ui.nearSpinBox.setValue(clipRange.min)
        self._ui.farSpinBox.setEnabled(self._ui.overrideFar.isChecked())
        self._ui.farSpinBox.setValue(clipRange.max)

        self._ui.lockFreeCamAspect.setChecked(
            bool(self._dataModel.viewSettings.lockFreeCameraAspect))
        self._ui.freeCamAspect.setValue(self._getCurrentAspectRatio())
        self._ui.freeCamAspect.setEnabled(
            bool(self._dataModel.viewSettings.lockFreeCameraAspect))

        self._ui.lockFreeCamAspect.stateChanged.connect(
            self._lockFreeCamAspectToggled)
        self._ui.freeCamAspect.valueChanged.connect(self._aspectSpinBoxChanged)

        self._ui.freeCamFov.setValue(
            self._dataModel.viewSettings.freeCameraFOV)
        self._ui.freeCamFov.valueChanged.connect(self._freeCamFovChanged)

    def _overrideNearToggled(self, state):
        """Called when the "Override Near" checkbox is toggled"""
        self._ui.nearSpinBox.setEnabled(state)
        if state:
            self._dataModel.viewSettings.freeCameraOverrideNear = \
                self._getCurrentClippingRange().min
        else:
            self._dataModel.viewSettings.freeCameraOverrideNear = None

    def _overrideFarToggled(self, state):
        """Called when the "Override Far" checkbox is toggled"""
        self._ui.farSpinBox.setEnabled(state)
        if state:
            self._dataModel.viewSettings.freeCameraOverrideFar = \
                self._getCurrentClippingRange().max
        else:
            self._dataModel.viewSettings.freeCameraOverrideFar = None

    def _nearChanged(self, value):
        """Called when the Near spin box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if self._ui.nearSpinBox.isEnabled():
            self._dataModel.viewSettings.freeCameraOverrideNear = value

    def _farChanged(self, value):
        """Called when the Far spin box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if self._ui.farSpinBox.isEnabled():
            self._dataModel.viewSettings.freeCameraOverrideFar = value

    def _lockFreeCamAspectToggled(self, state):
        lockFreeCameraAspect = state != QtCore.Qt.Unchecked
        self._ui.freeCamAspect.setEnabled(lockFreeCameraAspect)
        self._dataModel.viewSettings.lockFreeCameraAspect = \
            lockFreeCameraAspect
        if lockFreeCameraAspect:
            self._dataModel.viewSettings.freeCameraAspect = \
                self._ui.freeCamAspect.value()

    def _aspectSpinBoxChanged(self, value):
        """Updates the camera's aspect ratio based on the spin box value."""
        if self._dataModel.viewSettings.lockFreeCameraAspect:
            self._dataModel.viewSettings.freeCameraAspect = value

    def _getCurrentAspectRatio(self):
        """Returns the current aspect ratio that should be displayed in the spin
        box.

        If a camera prim is active, reflect that value. Otherwise, use the
        current setting."""
        cameraPrim = self._dataModel.viewSettings.cameraPrim
        if cameraPrim and cameraPrim.IsActive():
            gfCam = UsdGeom.Camera(cameraPrim).GetCamera(
                self._dataModel.currentFrame)
            return gfCam.aspectRatio

        return self._dataModel.viewSettings.freeCameraAspect

    def _getCurrentFov(self):
        """Returns the current vertical field of view that should be displayed
        in the spin box.

        If a camera prim is active, reflect that value. Otherwise, use the
        current setting."""
        cameraPrim = self._dataModel.viewSettings.cameraPrim
        if cameraPrim and cameraPrim.IsActive():
            gfCam = UsdGeom.Camera(cameraPrim).GetCamera(
                self._dataModel.currentFrame)
            return gfCam.GetFieldOfView(Gf.Camera.FOVVertical)

        return self._dataModel.viewSettings.freeCameraFOV

    def _getCurrentClippingRange(self):
        """Returns the current clipping range (near, far) that should be
        displayed in the spin boxes.

        If the view settings have values for freeCameraOverrideNear/Far, then
        those values should be chosen. Otherwise, take the clipping range from
        the current camera (whether a camera prim or the free camera)."""

        viewSettings = self._dataModel.viewSettings
        near = viewSettings.freeCameraOverrideNear
        far = viewSettings.freeCameraOverrideFar
        if near is not None and far is not None:
            return Gf.Range1f(near, far)

        cameraPrim = viewSettings.cameraPrim
        if cameraPrim and cameraPrim.IsActive():
            gfCam = UsdGeom.Camera(cameraPrim).GetCamera(
                self._dataModel.currentFrame)
            if near is None:
                near = gfCam.clippingRange.min
            if far is None:
                far = gfCam.clippingRange.max
        elif viewSettings.freeCamera:
            if near is None:
                near = viewSettings.freeCamera.near
            if far is None:
                far = viewSettings.freeCamera.far

        if near is None:
            near = 1.0
        if far is None:
            far = 1e6
        return Gf.Range1f(near, far)

    def _freeCamFovChanged(self, value):
        self._dataModel.viewSettings.freeCameraFOV = value

    def _frustumChanged(self):
        """Updates the UI to reflect the current camera frustum."""
        self._ui.freeCamAspect.blockSignals(True)
        self._ui.freeCamAspect.setValue(self._getCurrentAspectRatio())
        self._ui.freeCamAspect.blockSignals(False)

        self._ui.freeCamFov.blockSignals(True)
        self._ui.freeCamFov.setValue(self._getCurrentFov())
        self._ui.freeCamFov.blockSignals(False)

        clippingRange = self._getCurrentClippingRange()
        self._ui.nearSpinBox.blockSignals(True)
        self._ui.nearSpinBox.setValue(clippingRange.min)
        self._ui.nearSpinBox.blockSignals(False)
        self._ui.farSpinBox.blockSignals(True)
        self._ui.farSpinBox.setValue(clippingRange.max)
        self._ui.farSpinBox.blockSignals(False)

        self.update()

    def closeEvent(self, event):
        # Ensure that even if the dialog doesn't get destroyed right away,
        # we'll stop doing work.
        self._signalFrustumChanged.disconnect(self._frustumChanged)

        event.accept()
        # Since the dialog is the immediate-edit kind, we consider
        # window-close to be an accept, so our clients can know the dialog is
        # done by listening for the finished(int) signal
        self.accept()
