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

    """Dialog to adjust the free camera settings (clipping and aspect ratio).

    The clippingDataModel must conform to the following interface:

    Editable properties:
       overrideNear (float or None, which indicates the override is disabled)
       overrideFar  (float or None, which indicates the override is disabled)

    Readable properties:
       cameraFrustum (Gf.Frustum, or struct that has a Gf.Range1d 'nearFar' member)
    """
    def __init__(self, parent, usdviewDataModel, clippingDataModel,
            signalFrustumChanged):
        QtWidgets.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustFreeCamera()
        self._ui.setupUi(self)

        self._usdviewDataModel = usdviewDataModel
        self._clippingDataModel = clippingDataModel
        self._signalFrustumChanged = signalFrustumChanged

        clipRange = self._clippingDataModel.cameraFrustum.nearFar
        self._nearCache = self._clippingDataModel.overrideNear or clipRange.min
        self._farCache = self._clippingDataModel.overrideFar or clipRange.max

        self._signalFrustumChanged.connect(self._frustumChanged)

        # When the checkboxes change, we want to update instantly
        self._ui.overrideNear.stateChanged.connect(self._overrideNearToggled)

        self._ui.overrideFar.stateChanged.connect(self._overrideFarToggled)

        # we also want to update the clipping planes as the user is typing
        self._ui.nearSpinBox.valueChanged.connect(self._nearChanged)

        self._ui.farSpinBox.valueChanged.connect(self._farChanged)

        # Set the checkboxes to their initial state
        self._ui.overrideNear.setChecked(self._clippingDataModel.overrideNear \
                                             is not None)
        self._ui.overrideFar.setChecked(self._clippingDataModel.overrideFar \
                                            is not None)

        # load the initial values for the text boxes, but first deactivate them
        # if their corresponding checkbox is off.
        self._ui.nearSpinBox.setEnabled(self._ui.overrideNear.isChecked())
        self._ui.nearSpinBox.setValue(self._nearCache)

        self._ui.farSpinBox.setEnabled(self._ui.overrideFar.isChecked())
        self._ui.farSpinBox.setValue(self._farCache)

        self._ui.lockFreeCamAspect.setChecked(
            bool(self._usdviewDataModel.viewSettings.lockFreeCameraAspect))
        self._ui.freeCamAspect.setValue(self._getCurrentAspectRatio())
        self._ui.freeCamAspect.setEnabled(
            bool(self._usdviewDataModel.viewSettings.lockFreeCameraAspect))

        self._ui.lockFreeCamAspect.stateChanged.connect(
            self._lockFreeCamAspectToggled)
        self._ui.freeCamAspect.valueChanged.connect(self._aspectSpinBoxChanged)

        self._ui.freeCamFov.setValue(
            self._usdviewDataModel.viewSettings.freeCameraFOV)
        self._ui.freeCamFov.valueChanged.connect(self._freeCamFovChanged)

    def _updateEditorsFromDataModel(self):
        """Read the dataModel-computed clipping planes and put them
           in the spin boxes when they are deactivated."""
        clipRange = self._clippingDataModel.cameraFrustum.nearFar
        if (not self._ui.overrideNear.isChecked()) and \
                self._nearCache != clipRange.min :
            self._nearCache = clipRange.min
            self._ui.nearSpinBox.setValue(self._nearCache)

        if (not self._ui.overrideFar.isChecked()) and \
                self._farCache != clipRange.max :
            self._farCache = clipRange.max
            self._ui.farSpinBox.setValue(self._farCache)

    def paintEvent(self, paintEvent):
        """Overridden from base class so we can perform JIT updating
        of editors to limit the number of redraws we perform"""
        self._updateEditorsFromDataModel()
        super(AdjustFreeCamera, self).paintEvent(paintEvent)

    def _overrideNearToggled(self, state):
        """Called when the "Override Near" checkbox is toggled"""
        self._ui.nearSpinBox.setEnabled(state)
        if state:
            self._clippingDataModel.overrideNear = self._nearCache
        else:
            self._clippingDataModel.overrideNear = None

    def _overrideFarToggled(self, state):
        """Called when the "Override Far" checkbox is toggled"""
        self._ui.farSpinBox.setEnabled(state)
        if state:
            self._clippingDataModel.overrideFar = self._farCache
        else:
            self._clippingDataModel.overrideFar = None

    def _nearChanged(self, value):
        """Called when the Near spin box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if self._ui.nearSpinBox.isEnabled():
            self._clippingDataModel.overrideNear = value

    def _farChanged(self, value):
        """Called when the Far spin box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if self._ui.farSpinBox.isEnabled():
            self._clippingDataModel.overrideFar = value

    def _lockFreeCamAspectToggled(self, state):
        lockFreeCameraAspect = state != QtCore.Qt.Unchecked
        self._ui.freeCamAspect.setEnabled(lockFreeCameraAspect)
        self._usdviewDataModel.viewSettings.lockFreeCameraAspect = \
            lockFreeCameraAspect
        if lockFreeCameraAspect:
            self._usdviewDataModel.viewSettings.freeCameraAspect = \
                self._ui.freeCamAspect.value()

    def _aspectSpinBoxChanged(self, value):
        """Updates the camera's aspect ratio based on the spin box value."""
        if self._usdviewDataModel.viewSettings.lockFreeCameraAspect:
            self._usdviewDataModel.viewSettings.freeCameraAspect = value

    def _getCurrentAspectRatio(self):
        """Returns the current aspect ratio that should be displayed in the spin
        box.

        If a camera prim is active, reflect that value. Otherwise, use the
        current setting."""
        cameraPrim = self._usdviewDataModel.viewSettings.cameraPrim
        if cameraPrim and cameraPrim.IsActive():
            gfCam = UsdGeom.Camera(cameraPrim).GetCamera(
                self._usdviewDataModel.currentFrame)
            return gfCam.aspectRatio

        return self._usdviewDataModel.viewSettings.freeCameraAspect

    def _getCurrentFov(self):
        """Returns the current vertical field of view that should be displayed
        in the spin box.

        If a camera prim is active, reflect that value. Otherwise, use the
        current setting."""
        cameraPrim = self._usdviewDataModel.viewSettings.cameraPrim
        if cameraPrim and cameraPrim.IsActive():
            gfCam = UsdGeom.Camera(cameraPrim).GetCamera(
                self._usdviewDataModel.currentFrame)
            return gfCam.GetFieldOfView(Gf.Camera.FOVVertical)

        return self._usdviewDataModel.viewSettings.freeCameraFOV        

    def _freeCamFovChanged(self, value):
        self._usdviewDataModel.viewSettings.freeCameraFOV = value

    def _frustumChanged(self):
        """Updates the UI to reflect the current camera frustum."""
        self._ui.freeCamAspect.blockSignals(True)
        self._ui.freeCamAspect.setValue(self._getCurrentAspectRatio())
        self._ui.freeCamAspect.blockSignals(False)

        self._ui.freeCamFov.blockSignals(True)
        self._ui.freeCamFov.setValue(self._getCurrentFov())
        self._ui.freeCamFov.blockSignals(False)

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
