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

from qt import QtCore

from common import (RenderModes, PickModes, SelectionHighlightModes,
    CameraMaskModes, Complexities)

import settings2
from settings2 import StateSource
from constantGroup import ConstantGroup


class ClearColors(ConstantGroup):
    """Names of available background colors."""
    BLACK = "Black"
    DARK_GREY = "Grey (Dark)"
    LIGHT_GREY = "Grey (Light)"
    WHITE = "White"

# Map of clear color names to rgba color tuples.
_CLEAR_COLORS_DICT = {
    ClearColors.BLACK:       (0.0, 0.0, 0.0, 0.0),
    ClearColors.DARK_GREY:   (0.3, 0.3, 0.3, 0.0),
    ClearColors.LIGHT_GREY:  (0.7, 0.7, 0.7, 0.0),
    ClearColors.WHITE:       (1.0, 1.0, 1.0, 0.0)}


class HighlightColors(ConstantGroup):
    """Names of available highlight colors for selected objects."""
    WHITE = "White"
    YELLOW = "Yellow"
    CYAN = "Cyan"

# Map of highlight color names to rgba color tuples.
_HIGHLIGHT_COLORS_DICT = {
    HighlightColors.WHITE:   (1.0, 1.0, 1.0, 0.5),
    HighlightColors.YELLOW:  (1.0, 1.0, 0.0, 0.5),
    HighlightColors.CYAN:    (0.0, 1.0, 1.0, 0.5)}


# Default values for default material components.
DEFAULT_AMBIENT = 0.2
DEFAULT_SPECULAR = 0.1


class ViewSettingsDataModel(QtCore.QObject, StateSource):
    """Data model containing settings related to the rendered view of a USD
    file.
    """

    # emitted when any aspect of the defaultMaterial changes
    signalDefaultMaterialChanged = QtCore.Signal()

    def __init__(self, parent):
        QtCore.QObject.__init__(self)
        StateSource.__init__(self, parent, "model")

        self._cameraMaskColor = tuple(self.stateProperty("cameraMaskColor", default=[0.1, 0.1, 0.1, 1.0]))
        self._cameraReticlesColor = tuple(self.stateProperty("cameraReticlesColor", default=[0.0, 0.7, 1.0, 1.0]))
        self._defaultMaterialAmbient = self.stateProperty("defaultMaterialAmbient", default=DEFAULT_AMBIENT)
        self._defaultMaterialSpecular = self.stateProperty("defaultMaterialSpecular", default=DEFAULT_SPECULAR)
        self._redrawOnScrub = self.stateProperty("redrawOnScrub", default=True)
        self._renderMode = self.stateProperty("renderMode", default=RenderModes.SMOOTH_SHADED)
        self._pickMode = self.stateProperty("pickMode", default=PickModes.PRIMS)

        # We need to store the trinary selHighlightMode state here,
        # because the stageView only deals in True/False (because it
        # cannot know anything about playback state).
        self._selHighlightMode = self.stateProperty("selectionHighlightMode", default=SelectionHighlightModes.ONLY_WHEN_PAUSED)

        # We store the highlightColorName so that we can compare state during
        # initialization without inverting the name->value logic
        self._highlightColorName = self.stateProperty("highlightColor", default="Yellow")
        self._ambientLightOnly = self.stateProperty("cameraLightEnabled", default=True)
        self._keyLightEnabled = self.stateProperty("keyLightEnabled", default=True)
        self._fillLightEnabled = self.stateProperty("fillLightEnabled", default=True)
        self._backLightEnabled = self.stateProperty("backLightEnabled", default=True)
        self._clearColorText = self.stateProperty("backgroundColor", default="Grey (Dark)")
        self._showBBoxPlayback = self.stateProperty("showBBoxesDuringPlayback", default=False)
        self._showBBoxes = self.stateProperty("showBBoxes", default=True)
        self._showAABBox = self.stateProperty("showAABBox", default=True)
        self._showOBBox = self.stateProperty("showOBBox", default=True)
        self._displayGuide = self.stateProperty("displayGuide", default=False)
        self._displayProxy = self.stateProperty("displayProxy", default=True)
        self._displayRender = self.stateProperty("displayRender", default=False)
        self._displayPrimId = self.stateProperty("displayPrimId", default=False)
        self._enableHardwareShading = self.stateProperty("enableHardwareShading", default=True)
        self._cullBackfaces = self.stateProperty("cullBackfaces", default=False)
        self._showInactivePrims = self.stateProperty("showInactivePrims", default=True)
        self._showAllMasterPrims = self.stateProperty("showAllMasterPrims", default=False)
        self._showUndefinedPrims = self.stateProperty("showUndefinedPrims", default=False)
        self._showAbstractPrims = self.stateProperty("showAbstractPrims", default=False)
        self._rolloverPrimInfo = self.stateProperty("rolloverPrimInfo", default=False)
        self._displayCameraOracles = self.stateProperty("cameraOracles", default=False)
        self._cameraMaskMode = self.stateProperty("cameraMaskMode", default=CameraMaskModes.NONE)
        self._showMask_Outline = self.stateProperty("cameraMaskOutline", default=False)
        self._showReticles_Inside = self.stateProperty("cameraReticlesInside", default=False)
        self._showReticles_Outside = self.stateProperty("cameraReticlesOutside", default=False)
        self._showHUD = self.stateProperty("showHUD", default=True)

        self._showHUD_Info = self.stateProperty("showHUDInfo", default=False)
        # XXX Until we can make the "Subtree Info" stats-gathering faster
        # we do not want the setting to persist from session to session.
        self._showHUD_Info = False

        self._showHUD_Complexity = self.stateProperty("showHUDComplexity", default=True)
        self._showHUD_Performance = self.stateProperty("showHUDPerformance", default=True)
        self._showHUD_GPUstats = self.stateProperty("showHUDGPUStats", default=False)

        self._complexity = Complexities.LOW
        self._freeCamera = None

    def onSaveState(self, state):
        state["cameraMaskColor"] = list(self._cameraMaskColor)
        state["cameraReticlesColor"] = list(self._cameraReticlesColor)
        state["defaultMaterialAmbient"] = self._defaultMaterialAmbient
        state["defaultMaterialSpecular"] = self._defaultMaterialSpecular
        state["redrawOnScrub"] = self._redrawOnScrub
        state["renderMode"] = self._renderMode
        state["pickMode"] = self._pickMode
        state["selectionHighlightMode"] = self._selHighlightMode
        state["highlightColor"] = self._highlightColorName
        state["cameraLightEnabled"] = self._ambientLightOnly
        state["keyLightEnabled"] = self._keyLightEnabled
        state["fillLightEnabled"] = self._fillLightEnabled
        state["backLightEnabled"] = self._backLightEnabled
        state["backgroundColor"] = self._clearColorText
        state["showBBoxesDuringPlayback"] = self._showBBoxPlayback
        state["showBBoxes"] = self._showBBoxes
        state["showAABBox"] = self._showAABBox
        state["showOBBox"] = self._showOBBox
        state["displayGuide"] = self._displayGuide
        state["displayProxy"] = self._displayProxy
        state["displayRender"] = self._displayRender
        state["displayPrimId"] = self._displayPrimId
        state["enableHardwareShading"] = self._enableHardwareShading
        state["cullBackfaces"] = self._cullBackfaces
        state["showInactivePrims"] = self._showInactivePrims
        state["showAllMasterPrims"] = self._showAllMasterPrims
        state["showUndefinedPrims"] = self._showUndefinedPrims
        state["showAbstractPrims"] = self._showAbstractPrims
        state["rolloverPrimInfo"] = self._rolloverPrimInfo
        state["cameraOracles"] = self._displayCameraOracles
        state["cameraMaskMode"] = self._cameraMaskMode
        state["cameraMaskOutline"] = self._showMask_Outline
        state["cameraReticlesInside"] = self._showReticles_Inside
        state["cameraReticlesOutside"] = self._showReticles_Outside
        state["showHUD"] = self._showHUD
        state["showHUDInfo"] = self._showHUD_Info
        state["showHUDComplexity"] = self._showHUD_Complexity
        state["showHUDPerformance"] = self._showHUD_Performance
        state["showHUDGPUStats"] = self._showHUD_GPUstats

    @property
    def cameraMaskColor(self):
        return self._cameraMaskColor

    @cameraMaskColor.setter
    def cameraMaskColor(self, color):
        self._cameraMaskColor = color

    @property
    def cameraReticlesColor(self):
        return self._cameraReticlesColor

    @cameraReticlesColor.setter
    def cameraReticlesColor(self, color):
        self._cameraReticlesColor = color

    @property
    def defaultMaterialAmbient(self):
        return self._defaultMaterialAmbient

    @defaultMaterialAmbient.setter
    def defaultMaterialAmbient(self, value):
        if value != self._defaultMaterialAmbient:
            self._defaultMaterialAmbient = value
            self.signalDefaultMaterialChanged.emit()

    @property
    def defaultMaterialSpecular(self):
        return self._defaultMaterialSpecular

    @defaultMaterialSpecular.setter
    def defaultMaterialSpecular(self, value):
        if value != self._defaultMaterialSpecular:
            self._defaultMaterialSpecular = value
            self.signalDefaultMaterialChanged.emit()

    def setDefaultMaterial(self, ambient, specular):
        if (ambient != self._defaultMaterialAmbient
                or specular != self._defaultMaterialSpecular):
            self._defaultMaterialAmbient = ambient
            self._defaultMaterialSpecular = specular
            self.signalDefaultMaterialChanged.emit()

    def resetDefaultMaterial(self):
        self.setDefaultMaterial(DEFAULT_AMBIENT, DEFAULT_SPECULAR)

    @property
    def complexity(self):
        return self._complexity

    @complexity.setter
    def complexity(self, value):
        if value not in Complexities:
            raise ValueError("Expected Complexity, got: '{}'.".format(value))
        self._complexity = value

    @property
    def renderMode(self):
        return self._renderMode

    @renderMode.setter
    def renderMode(self, value):
        self._renderMode = value

    @property
    def pickMode(self):
        return self._pickMode

    @pickMode.setter
    def pickMode(self, value):
        self._pickMode = value

    @property
    def showAABBox(self):
        return self._showAABBox

    @showAABBox.setter
    def showAABBox(self, value):
        self._showAABBox = value

    @property
    def showOBBox(self):
        return self._showOBBox

    @showOBBox.setter
    def showOBBox(self, value):
        self._showOBBox = value

    @property
    def showBBoxes(self):
        return self._showBBoxes

    @showBBoxes.setter
    def showBBoxes(self, value):
        self._showBBoxes = value

    @property
    def showBBoxPlayback(self):
        return self._showBBoxPlayback

    @showBBoxPlayback.setter
    def showBBoxPlayback(self, value):
        self._showBBoxPlayback = value

    @property
    def displayGuide(self):
        return self._displayGuide

    @displayGuide.setter
    def displayGuide(self, value):
        self._displayGuide = value

    @property
    def displayProxy(self):
        return self._displayProxy

    @displayProxy.setter
    def displayProxy(self, value):
        self._displayProxy = value

    @property
    def displayRender(self):
        return self._displayRender

    @displayRender.setter
    def displayRender(self, value):
        self._displayRender = value

    @property
    def displayCameraOracles(self):
        return self._displayCameraOracles

    @displayCameraOracles.setter
    def displayCameraOracles(self, value):
        self._displayCameraOracles = value

    @property
    def displayPrimId(self):
        return self._displayPrimId

    @displayPrimId.setter
    def displayPrimId(self, value):
        self._displayPrimId = value

    @property
    def enableHardwareShading(self):
        return self._enableHardwareShading

    @enableHardwareShading.setter
    def enableHardwareShading(self, value):
        self._enableHardwareShading = value

    @property
    def cullBackfaces(self):
        return self._cullBackfaces

    @cullBackfaces.setter
    def cullBackfaces(self, value):
        self._cullBackfaces = value

    @property
    def showInactivePrims(self):
        return self._showInactivePrims

    @showInactivePrims.setter
    def showInactivePrims(self, value):
        self._showInactivePrims = value

    @property
    def showAllMasterPrims(self):
        return self._showAllMasterPrims

    @showAllMasterPrims.setter
    def showAllMasterPrims(self, value):
        self._showAllMasterPrims = value

    @property
    def showUndefinedPrims(self):
        return self._showUndefinedPrims

    @showUndefinedPrims.setter
    def showUndefinedPrims(self, value):
        self._showUndefinedPrims = value

    @property
    def showAbstractPrims(self):
        return self._showAbstractPrims

    @showAbstractPrims.setter
    def showAbstractPrims(self, value):
        self._showAbstractPrims = value

    @property
    def rolloverPrimInfo(self):
        return self._rolloverPrimInfo

    @rolloverPrimInfo.setter
    def rolloverPrimInfo(self, value):
        self._rolloverPrimInfo = value

    @property
    def cameraMaskMode(self):
        return self._cameraMaskMode

    @cameraMaskMode.setter
    def cameraMaskMode(self, value):
        self._cameraMaskMode = value

    @property
    def showMask(self):
        return self._cameraMaskMode in (CameraMaskModes.FULL, CameraMaskModes.PARTIAL)

    @property
    def showMask_Opaque(self):
        return self._cameraMaskMode == CameraMaskModes.FULL

    @property
    def showMask_Outline(self):
        return self._showMask_Outline

    @showMask_Outline.setter
    def showMask_Outline(self, value):
        self._showMask_Outline = value

    @property
    def showReticles_Inside(self):
        return self._showReticles_Inside

    @showReticles_Inside.setter
    def showReticles_Inside(self, value):
        self._showReticles_Inside = value

    @property
    def showReticles_Outside(self):
        return self._showReticles_Outside

    @showReticles_Outside.setter
    def showReticles_Outside(self, value):
        self._showReticles_Outside = value

    @property
    def showHUD(self):
        return self._showHUD

    @showHUD.setter
    def showHUD(self, value):
        self._showHUD = value

    @property
    def showHUD_Info(self):
        return self._showHUD_Info

    @showHUD_Info.setter
    def showHUD_Info(self, value):
        self._showHUD_Info = value

    @property
    def showHUD_Complexity(self):
        return self._showHUD_Complexity

    @showHUD_Complexity.setter
    def showHUD_Complexity(self, value):
        self._showHUD_Complexity = value

    @property
    def showHUD_Performance(self):
        return self._showHUD_Performance

    @showHUD_Performance.setter
    def showHUD_Performance(self, value):
        self._showHUD_Performance = value

    @property
    def showHUD_GPUstats(self):
        return self._showHUD_GPUstats

    @showHUD_GPUstats.setter
    def showHUD_GPUstats(self, value):
        self._showHUD_GPUstats = value

    @property
    def ambientLightOnly(self):
        return self._ambientLightOnly

    @ambientLightOnly.setter
    def ambientLightOnly(self, value):
        self._ambientLightOnly = value

    @property
    def keyLightEnabled(self):
        return self._keyLightEnabled

    @keyLightEnabled.setter
    def keyLightEnabled(self, value):
        self._keyLightEnabled = value

    @property
    def fillLightEnabled(self):
        return self._fillLightEnabled

    @fillLightEnabled.setter
    def fillLightEnabled(self, value):
        self._fillLightEnabled = value

    @property
    def backLightEnabled(self):
        return self._backLightEnabled

    @backLightEnabled.setter
    def backLightEnabled(self, value):
        self._backLightEnabled = value

    @property
    def clearColorText(self):
        return self._clearColorText

    @clearColorText.setter
    def clearColorText(self, value):
        if value not in ClearColors:
            raise ValueError("Unknown clear color: '{}'".format(value))
        self._clearColorText = value

    @property
    def clearColor(self):
        return _CLEAR_COLORS_DICT[self._clearColorText]

    @property
    def highlightColorName(self):
        return self._highlightColorName

    @highlightColorName.setter
    def highlightColorName(self, value):
        if value not in HighlightColors:
            raise ValueError("Unknown highlight color: '{}'".format(value))
        self._highlightColorName = value

    @property
    def highlightColor(self):
        return _HIGHLIGHT_COLORS_DICT[self._highlightColorName]

    @property
    def selHighlightMode(self):
        return self._selHighlightMode

    @selHighlightMode.setter
    def selHighlightMode(self, value):
        if value not in SelectionHighlightModes:
            raise ValueError("Unknown highlight mode: '{}'".format(value))
        self._selHighlightMode = value

    @property
    def redrawOnScrub(self):
        return self._redrawOnScrub

    @redrawOnScrub.setter
    def redrawOnScrub(self, value):
        self._redrawOnScrub = value

    @property
    def freeCamera(self):
        return self._freeCamera

    @freeCamera.setter
    def freeCamera(self, value):
        self._freeCamera = value
