#
# Copyright 2016 Pixar
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
# Qt Components
from qt import QtCore, QtGui, QtWidgets

# Stdlib components
import re, sys, os, cProfile, pstats, traceback
from itertools import groupby
from time import time, sleep
from collections import deque, OrderedDict

# Usd Library Components
from pxr import Usd, UsdGeom, UsdUtils, UsdImagingGL, Glf, Sdf, Tf, Ar

# UI Components
from ._usdviewq import Utils
from stageView import StageView
from mainWindowUI import Ui_MainWindow
from primContextMenu import PrimContextMenu
from headerContextMenu import HeaderContextMenu
from layerStackContextMenu import LayerStackContextMenu
from attributeViewContextMenu import AttributeViewContextMenu
from customAttributes import (_GetCustomAttributes, BoundingBoxAttribute,
                              LocalToWorldXformAttribute)
from primViewItem import PrimViewItem
from variantComboBox import VariantComboBox
from legendUtil import ToggleLegendWithBrowser
import prettyPrint, adjustClipping, adjustDefaultMaterial, settings
from constantGroup import ConstantGroup

# Common Utilities
from common import (UIBaseColors, UIPropertyValueSourceColors, UIFonts, GetAttributeColor, GetAttributeTextFont,
                    Timer, Drange, BusyContext, DumpMallocTags, GetShortString,
                    GetInstanceIdForIndex,
                    ResetSessionVisibility, InvisRootPrims, GetAssetCreationTime,
                    PropertyViewIndex, PropertyViewIcons, PropertyViewDataRoles, RenderModes, ShadedRenderModes,
                    PickModes, SelectionHighlightModes, CameraMaskModes,
                    PropTreeWidgetTypeIsRel, PrimNotFoundException,
                    GetRootLayerStackInfo, HasSessionVis, GetEnclosingModelPrim,
                    GetPrimsLoadability, GetClosestBoundMaterial)

import settings2
from settings2 import StateSource
from plugContext import PlugContext
from rootDataModel import RootDataModel

SETTINGS_VERSION = "1"

class HUDEntries(ConstantGroup):
    # Upper HUD entries (declared in variables for abstraction)
    PRIM = "Prims"
    CV = "CVs"
    VERT = "Verts"
    FACE = "Faces"

    # Lower HUD entries
    PLAYBACK = "Playback"
    RENDER = "Render"
    GETBOUNDS = "BBox"

    # Name for prims that have no type
    NOTYPE = "Typeless"

class PropertyIndex(ConstantGroup):
    VALUE, METADATA, LAYERSTACK, COMPOSITION = range(4)

class DebugTypes(ConstantGroup):
    # Tf Debug entries to include in debug menu
    HD = "HD"
    HDX = "HDX"
    USD = "USD"
    USDIMAGING = "USDIMAGING"
    USDVIEWQ = "USDVIEWQ"

class UIDefaults(ConstantGroup):
    STAGE_VIEW_WIDTH = 604
    PRIM_VIEW_WIDTH = 521
    ATTRIBUTE_VIEW_WIDTH = 682
    ATTRIBUTE_INSPECTOR_WIDTH = 443
    TOP_HEIGHT = 538
    BOTTOM_HEIGHT = 306

# Name of the Qt binding being used
QT_BINDING = QtCore.__name__.split('.')[0]

class DataModelProxySource(StateSource):
    """XXX Temporary class which allows AppController to serve as two state sources.
    All fields here will be moved into an actual data model class in the future.
    """

    def __init__(self, mainWindow, parent, name):
        StateSource.__init__(self, parent, name)

        self._mainWindow = mainWindow

        self._mainWindow._cameraMaskColor = tuple(self.stateProperty("cameraMaskColor", default=[0.1, 0.1, 0.1, 1.0]))
        self._mainWindow._cameraReticlesColor = tuple(self.stateProperty("cameraReticlesColor", default=[0.0, 0.7, 1.0, 1.0]))
        self._mainWindow._defaultMaterialAmbient = self.stateProperty("defaultMaterialAmbient", default=0.2)
        self._mainWindow._defaultMaterialSpecular = self.stateProperty("defaultMaterialSpecular", default=0.1)
        self._mainWindow._redrawOnScrub = self.stateProperty("redrawOnScrub", default=True)
        self._mainWindow._renderMode = self.stateProperty("renderMode", default=RenderModes.SMOOTH_SHADED)
        self._mainWindow._pickMode = self.stateProperty("pickMode", default=PickModes.PRIMS)

        # We need to store the trinary selHighlightMode state here,
        # because the stageView only deals in True/False (because it
        # cannot know anything about playback state).
        self._mainWindow._selHighlightMode = self.stateProperty("selectionHighlightMode", default=SelectionHighlightModes.ONLY_WHEN_PAUSED)

        # We store the highlightColorName so that we can compare state during
        # initialization without inverting the name->value logic
        self._mainWindow._highlightColorName = self.stateProperty("highlightColor", default="Yellow")
        self._mainWindow._ambientLightOnly = self.stateProperty("cameraLightEnabled", default=True)
        self._mainWindow._keyLightEnabled = self.stateProperty("keyLightEnabled", default=True)
        self._mainWindow._fillLightEnabled = self.stateProperty("fillLightEnabled", default=True)
        self._mainWindow._backLightEnabled = self.stateProperty("backLightEnabled", default=True)
        self._mainWindow._clearColorText = self.stateProperty("backgroundColor", default="Grey (Dark)")
        self._mainWindow._showBBoxPlayback = self.stateProperty("showBBoxesDuringPlayback", default=False)
        self._mainWindow._showBBoxes = self.stateProperty("showBBoxes", default=True)
        self._mainWindow._showAABBox = self.stateProperty("showAABBox", default=True)
        self._mainWindow._showOBBox = self.stateProperty("showOBBox", default=True)
        self._mainWindow._displayGuide = self.stateProperty("displayGuide", default=False)
        self._mainWindow._displayProxy = self.stateProperty("displayProxy", default=True)
        self._mainWindow._displayRender = self.stateProperty("displayRender", default=False)
        self._mainWindow._displayPrimId = self.stateProperty("displayPrimId", default=False)
        self._mainWindow._enableHardwareShading = self.stateProperty("enableHardwareShading", default=True)
        self._mainWindow._cullBackfaces = self.stateProperty("cullBackfaces", default=False)
        self._mainWindow._showInactivePrims = self.stateProperty("showInactivePrims", default=True)
        self._mainWindow._showAllMasterPrims = self.stateProperty("showAllMasterPrims", default=False)
        self._mainWindow._showUndefinedPrims = self.stateProperty("showUndefinedPrims", default=False)
        self._mainWindow._showAbstractPrims = self.stateProperty("showAbstractPrims", default=False)
        self._mainWindow._rolloverPrimInfo = self.stateProperty("rolloverPrimInfo", default=False)
        self._mainWindow._displayCameraOracles = self.stateProperty("cameraOracles", default=False)
        self._mainWindow._cameraMaskMode = self.stateProperty("cameraMaskMode", default=CameraMaskModes.NONE)
        self._mainWindow._showMask_Outline = self.stateProperty("cameraMaskOutline", default=False)
        self._mainWindow._showReticles_Inside = self.stateProperty("cameraReticlesInside", default=False)
        self._mainWindow._showReticles_Outside = self.stateProperty("cameraReticlesOutside", default=False)
        self._mainWindow._showHUD = self.stateProperty("showHUD", default=True)

        self._mainWindow._showHUD_Info = self.stateProperty("showHUDInfo", default=False)
        # XXX Until we can make the "Subtree Info" stats-gathering faster
        # we do not want the setting to persist from session to session.
        self._mainWindow._showHUD_Info = False

        self._mainWindow._showHUD_Complexity = self.stateProperty("showHUDComplexity", default=True)
        self._mainWindow._showHUD_Performance = self.stateProperty("showHUDPerformance", default=True)
        self._mainWindow._showHUD_GPUstats = self.stateProperty("showHUDGPUStats", default=False)

    def onSaveState(self, state):
        state["cameraMaskColor"] = list(self._mainWindow._cameraMaskColor)
        state["cameraReticlesColor"] = list(self._mainWindow._cameraReticlesColor)
        state["defaultMaterialAmbient"] = self._mainWindow._defaultMaterialAmbient
        state["defaultMaterialSpecular"] = self._mainWindow._defaultMaterialSpecular
        state["redrawOnScrub"] = self._mainWindow._redrawOnScrub
        state["renderMode"] = self._mainWindow._renderMode
        state["pickMode"] = self._mainWindow._pickMode
        state["selectionHighlightMode"] = self._mainWindow._selHighlightMode
        state["highlightColor"] = self._mainWindow._highlightColorName
        state["cameraLightEnabled"] = self._mainWindow._ambientLightOnly
        state["keyLightEnabled"] = self._mainWindow._keyLightEnabled
        state["fillLightEnabled"] = self._mainWindow._fillLightEnabled
        state["backLightEnabled"] = self._mainWindow._backLightEnabled
        state["backgroundColor"] = self._mainWindow._clearColorText
        state["showBBoxesDuringPlayback"] = self._mainWindow._showBBoxPlayback
        state["showBBoxes"] = self._mainWindow._showBBoxes
        state["showAABBox"] = self._mainWindow._showAABBox
        state["showOBBox"] = self._mainWindow._showOBBox
        state["displayGuide"] = self._mainWindow._displayGuide
        state["displayProxy"] = self._mainWindow._displayProxy
        state["displayRender"] = self._mainWindow._displayRender
        state["displayPrimId"] = self._mainWindow._displayPrimId
        state["enableHardwareShading"] = self._mainWindow._enableHardwareShading
        state["cullBackfaces"] = self._mainWindow._cullBackfaces
        state["showInactivePrims"] = self._mainWindow._showInactivePrims
        state["showAllMasterPrims"] = self._mainWindow._showAllMasterPrims
        state["showUndefinedPrims"] = self._mainWindow._showUndefinedPrims
        state["showAbstractPrims"] = self._mainWindow._showAbstractPrims
        state["rolloverPrimInfo"] = self._mainWindow._rolloverPrimInfo
        state["cameraOracles"] = self._mainWindow._displayCameraOracles
        state["cameraMaskMode"] = self._mainWindow._cameraMaskMode
        state["cameraMaskOutline"] = self._mainWindow._showMask_Outline
        state["cameraReticlesInside"] = self._mainWindow._showReticles_Inside
        state["cameraReticlesOutside"] = self._mainWindow._showReticles_Outside
        state["showHUD"] = self._mainWindow._showHUD
        state["showHUDInfo"] = self._mainWindow._showHUD_Info
        state["showHUDComplexity"] = self._mainWindow._showHUD_Complexity
        state["showHUDPerformance"] = self._mainWindow._showHUD_Performance
        state["showHUDGPUStats"] = self._mainWindow._showHUD_GPUstats

class UIStateProxySource(StateSource):
    """XXX Temporary class which allows AppController to serve as two state sources.
    All fields here will be moved back into AppController in the future.
    """

    def __init__(self, mainWindow, parent, name):
        StateSource.__init__(self, parent, name)

        self._mainWindow = mainWindow
        primViewColumnVisibility = self.stateProperty("primViewColumnVisibility",
                default=[True, True, True], validator=lambda value: len(value) == 3)
        propertyViewColumnVisibility = self.stateProperty("propertyViewColumnVisibility",
                default=[True, True, True], validator=lambda value: len(value) == 3)
        attributeInspectorCurrentTab = self.stateProperty("attributeInspectorCurrentTab", default=PropertyIndex.VALUE)

        # UI is different when --norender is used so just save the default splitter sizes.
        # TODO Save the loaded state so it doesn't disappear after using --norender.
        if not self._mainWindow._noRender:
            stageViewWidth = self.stateProperty("stageViewWidth", default=UIDefaults.STAGE_VIEW_WIDTH)
            primViewWidth = self.stateProperty("primViewWidth", default=UIDefaults.PRIM_VIEW_WIDTH)
            attributeViewWidth = self.stateProperty("attributeViewWidth", default=UIDefaults.ATTRIBUTE_VIEW_WIDTH)
            attributeInspectorWidth = self.stateProperty("attributeInspectorWidth", default=UIDefaults.ATTRIBUTE_INSPECTOR_WIDTH)
            topHeight = self.stateProperty("topHeight", default=UIDefaults.TOP_HEIGHT)
            bottomHeight = self.stateProperty("bottomHeight", default=UIDefaults.BOTTOM_HEIGHT)
            viewerMode = self.stateProperty("viewerMode", default=False)

            if viewerMode:
                self._mainWindow._ui.primStageSplitter.setSizes([0, 1])
                self._mainWindow._ui.topBottomSplitter.setSizes([1, 0])
            else:
                self._mainWindow._ui.primStageSplitter.setSizes(
                    [primViewWidth, stageViewWidth])
                self._mainWindow._ui.topBottomSplitter.setSizes(
                    [topHeight, bottomHeight])
            self._mainWindow._ui.attribBrowserInspectorSplitter.setSizes(
                [attributeViewWidth, attributeInspectorWidth])
            self._mainWindow._viewerModeEscapeSizes = topHeight, bottomHeight, primViewWidth, stageViewWidth
        else:
            self._mainWindow._ui.primStageSplitter.setSizes(
                [UIDefaults.PRIM_VIEW_WIDTH, UIDefaults.STAGE_VIEW_WIDTH])
            self._mainWindow._ui.attribBrowserInspectorSplitter.setSizes(
                [UIDefaults.ATTRIBUTE_VIEW_WIDTH, UIDefaults.ATTRIBUTE_INSPECTOR_WIDTH])
            self._mainWindow._ui.topBottomSplitter.setSizes(
                [UIDefaults.TOP_HEIGHT, UIDefaults.BOTTOM_HEIGHT])

        for i, visible in enumerate(primViewColumnVisibility):
            self._mainWindow._ui.primView.setColumnHidden(i, not visible)
        for i, visible in enumerate(propertyViewColumnVisibility):
            self._mainWindow._ui.propertyView.setColumnHidden(i, not visible)

        propertyIndex = attributeInspectorCurrentTab
        if propertyIndex not in PropertyIndex:
            propertyIndex = PropertyIndex.VALUE
        self._mainWindow._ui.attributeInspector.setCurrentIndex(propertyIndex)

    def onSaveState(self, state):
        # UI is different when --norender is used so don't load the splitter sizes.
        if not self._mainWindow._noRender:
            primViewWidth, stageViewWidth = self._mainWindow._ui.primStageSplitter.sizes()
            attributeViewWidth, attributeInspectorWidth = self._mainWindow._ui.attribBrowserInspectorSplitter.sizes()
            topHeight, bottomHeight = self._mainWindow._ui.topBottomSplitter.sizes()
            viewerMode = (bottomHeight == 0 and primViewWidth == 0)

            # If viewer mode is active, save the escape sizes instead of the
            # actual sizes. If there are no escape sizes, just save the defaults.
            if viewerMode:
                if self._mainWindow._viewerModeEscapeSizes is not None:
                    topHeight, bottomHeight, primViewWidth, stageViewWidth = self._mainWindow._viewerModeEscapeSizes
                else:
                    primViewWidth = UIDefaults.STAGE_VIEW_WIDTH
                    stageViewWidth = UIDefaults.PRIM_VIEW_WIDTH
                    attributeViewWidth = UIDefaults.ATTRIBUTE_VIEW_WIDTH
                    attributeInspectorWidth = UIDefaults.ATTRIBUTE_INSPECTOR_WIDTH
                    topHeight = UIDefaults.TOP_HEIGHT
                    bottomHeight = UIDefaults.BOTTOM_HEIGHT

            state["primViewWidth"] = primViewWidth
            state["stageViewWidth"] = stageViewWidth
            state["attributeViewWidth"] = attributeViewWidth
            state["attributeInspectorWidth"] = attributeInspectorWidth
            state["topHeight"] = topHeight
            state["bottomHeight"] = bottomHeight
            state["viewerMode"] = viewerMode

        state["primViewColumnVisibility"] = [
            not self._mainWindow._ui.primView.isColumnHidden(c)
            for c in range(self._mainWindow._ui.primView.columnCount())]
        state["propertyViewColumnVisibility"] = [
            not self._mainWindow._ui.propertyView.isColumnHidden(c)
            for c in range(self._mainWindow._ui.propertyView.columnCount())]

        state["attributeInspectorCurrentTab"] = self._mainWindow._ui.attributeInspector.currentIndex()

class AppController(QtCore.QObject):

    ###########
    # Signals #
    ###########

    # emitted when any aspect of the defaultMaterial changes
    signalDefaultMaterialChanged = QtCore.Signal()

    @classmethod
    def clearSettings(cls):
        settingsPath = cls._outputBaseDirectory()
        if settingsPath is None:
            return None
        else:
            settingsPath = os.path.join(settingsPath, 'state')
            if not os.path.exists(settingsPath):
                print 'INFO: ClearSettings requested, but there ' \
                      'were no settings currently stored.'
                return None

            if not os.access(settingsPath, os.W_OK):
                print 'ERROR: Could not remove settings file.'
                return None
            else:
                os.remove(settingsPath)

        print 'INFO: Settings restored to default.'

    def _configurePlugins(self):
        pluginsLoaded = False

        self._plugCtx = PlugContext(self)
        with Timer() as t:
            try:
                from pixar import UsdviewPlug
                UsdviewPlug.ConfigureView(self._plugCtx, self)
                pluginsLoaded = True

            except ImportError:
                # Fails silently if UsdviewPlug is found but a sub-module is not.
                # See bug 152226
                pass

        if self._printTiming and pluginsLoaded:
            t.PrintTime("configure and load plugins.")

    def _openSettings2(self, defaultSettings):
        settingsPathDir = self._outputBaseDirectory()

        if (settingsPathDir is None) or defaultSettings:
            # Create an ephemeral settings object by withholding the file path.
            self._settings2 = settings2.Settings(SETTINGS_VERSION)
        else:
            settings2Path = os.path.join(settingsPathDir, "state.json")
            self._settings2 = settings2.Settings(SETTINGS_VERSION, settings2Path)

        modelProxy = DataModelProxySource(self, self._settings2, "model")
        uiProxy = UIStateProxySource(self, self._settings2, "ui")

    def __del__(self):
        # This is needed to free Qt items before exit; Qt hits failed GTK
        # assertions without it.
        self._primToItemMap.clear()

    def __init__(self, parserData):
        QtCore.QObject.__init__(self)

        with Timer() as uiOpenTimer:

            self._primToItemMap = {}
            self._itemsToPush = []
            self._currentPrims = []
            self._currentProp = None
            self._currentSpec = None
            self._currentLayer = None
            self._console = None
            self._interpreter = None
            self._parserData = parserData
            self._noRender = parserData.noRender
            self._unloaded = parserData.unloaded
            self._currentFrame = Usd.TimeCode.Default()
            self._updateBlock = 0
            self._debug = os.getenv('USDVIEW_DEBUG', False)
            self._printTiming = parserData.timing or self._debug
            self._lastViewContext = {}
            if QT_BINDING == 'PySide':
                self._statusFileName = 'state'
                self._deprecatedStatusFileNames = ('.usdviewrc')
            else:
                self._statusFileName = 'state.%s'%QT_BINDING
                self._deprecatedStatusFileNames = ('state', '.usdviewrc')
            self._mallocTags = parserData.mallocTagStats
            self._bboxCache = None
            self._complexity = parserData.complexity

            self._playing = False

            self._allowViewUpdates = True

            # When viewer mode is active, the panel sizes are cached so they can
            # be restored later.
            self._viewerModeEscapeSizes = None

            AppController._renderer = parserData.renderer
            if AppController._renderer == 'simple':
                os.environ['HD_ENABLED'] = '0'

            self._mainWindow = QtWidgets.QMainWindow(None)
            # Showing the window immediately prevents UI flashing.
            self._mainWindow.show()

            self._ui = Ui_MainWindow()
            self._ui.setupUi(self._mainWindow)

            self._mainWindow.setWindowTitle(parserData.usdFile)
            self._statusBar = QtWidgets.QStatusBar(self._mainWindow)
            self._mainWindow.setStatusBar(self._statusBar)

            # Install our custom event filter.  The member assignment of the
            # filter is just for lifetime management
            from appEventFilter import AppEventFilter
            self._filterObj = AppEventFilter(self)
            QtWidgets.QApplication.instance().installEventFilter(self._filterObj)

            # read the stage here
            stage = self._openStage(
                self._parserData.usdFile, self._parserData.populationMask)
            if not stage:
                sys.exit(0)

            if not stage.GetPseudoRoot():
                print parserData.usdFile, 'has no prims; exiting.'
                sys.exit(0)

            self._rootDataModel = RootDataModel(printTiming=self._printTiming)
            self._rootDataModel.stage = stage

            self._initialSelectPrim = self._rootDataModel.stage.GetPrimAtPath(
                parserData.primPath)
            if not self._initialSelectPrim:
                print 'Could not find prim at path <%s> to select. '\
                    'Ignoring...' % parserData.primPath
                self._initialSelectPrim = None

            self._timeSamples = None
            self._stageView = None
            self._startingPrimCamera = None
            if isinstance(parserData.camera, Sdf.Path):
                self._startingPrimCameraName = None
                self._startingPrimCameraPath = parserData.camera
            else:
                self._startingPrimCameraName = parserData.camera
                self._startingPrimCameraPath = None

            self._openSettings2(parserData.defaultSettings)

            settingsPathDir = self._outputBaseDirectory()
            if settingsPathDir is None or parserData.defaultSettings:
                # Create an ephemeral settings object with a non existent filepath
                self._settings = settings.Settings('', seq=None, ephemeral=True)
            else:
                settingsPath = os.path.join(settingsPathDir, self._statusFileName)
                for deprecatedName in self._deprecatedStatusFileNames:
                    deprecatedSettingsPath = \
                        os.path.join(settingsPathDir, deprecatedName)
                    if (os.path.isfile(deprecatedSettingsPath) and
                        not os.path.isfile(settingsPath)):
                        warning = ('\nWARNING: The settings file at: '
                                + str(deprecatedSettingsPath) + ' is deprecated.\n'
                                + 'These settings are not being used, the new '
                                + 'settings file will be located at: '
                                + str(settingsPath) + '.\n')
                        print warning
                        break

                self._settings = settings.Settings(settingsPath)

                try:
                    self._settings.load()
                except IOError:
                    # try to force out a new settings file
                    try:
                        self._settings.save()
                    except:
                        settings.EmitWarning(settingsPath)

                except EOFError:
                    # try to force out a new settings file
                    try:
                        self._settings.save()
                    except:
                        settings.EmitWarning(settingsPath)
                except:
                    settings.EmitWarning(settingsPath)

            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

            self._timer = QtCore.QTimer(self)
            # Timeout interval in ms. We set it to 0 so it runs as fast as
            # possible. In advanceFrameForPlayback we use the sleep() call
            # to slow down rendering to self.framesPerSecond fps.
            self._timer.setInterval(0)
            self._lastFrameTime = time()

            self._clearColorsDict = {
                'Black':         (0.0, 0.0, 0.0, 0.0),
                'Grey (Dark)':   (0.3, 0.3, 0.3, 0.0),
                'Grey (Light)':  (0.7, 0.7, 0.7, 0.0),
                'White':         (1.0, 1.0, 1.0, 0.0)}
            self._highlightColorsDict = {
                'White':         (1.0, 1.0, 1.0, 0.5),
                'Yellow':        (1.0, 1.0, 0.0, 0.5),
                'Cyan':          (0.0, 1.0, 1.0, 0.5)}

            # Initialize the upper HUD info
            self._upperHUDInfo = dict()

            # declare dictionary keys for the fps info too
            self._fpsHUDKeys = (HUDEntries.RENDER, HUDEntries.PLAYBACK)

            # Initialize fps HUD with empty strings
            self._fpsHUDInfo = dict(zip(self._fpsHUDKeys,
                                        ["N/A", "N/A"]))
            self._startTime = self._endTime = time()

            # Create action groups
            self._ui.threePointLights = QtWidgets.QActionGroup(self)
            self._ui.colorGroup = QtWidgets.QActionGroup(self)

            self._primViewResetTimer = QtCore.QTimer(self)
            self._primViewResetTimer.setInterval(250)
            self._primViewResetTimer.timeout.connect(self._resetPrimView)

            # Idle timer to push off-screen data to the UI.
            self._primViewUpdateTimer = QtCore.QTimer(self)
            self._primViewUpdateTimer.setInterval(0)
            self._primViewUpdateTimer.timeout.connect(self._updatePrimView)

            # This creates the _stageView and restores state from settings file
            self._resetSettings()

            # This is for validating frame values input on the "Frame" line edit
            validator = QtGui.QDoubleValidator(self)
            self._ui.frameField.setValidator(validator)
            self._ui.rangeEnd.setValidator(validator)
            self._ui.rangeBegin.setValidator(validator)

            stepValidator = QtGui.QDoubleValidator(self)
            stepValidator.setRange(0.01, 1e7, 2)
            self._ui.stepSize.setValidator(stepValidator)

            # This causes the last column of the attribute view (the value)
            # to be stretched to fill the available space
            self._ui.propertyView.header().setStretchLastSection(True)

            self._ui.propertyView.setSelectionBehavior(
                QtWidgets.QAbstractItemView.SelectRows)
            self._ui.primView.setSelectionBehavior(
                QtWidgets.QAbstractItemView.SelectRows)
            # This allows ctrl and shift clicking for multi-selecting
            self._ui.propertyView.setSelectionMode(
                QtWidgets.QAbstractItemView.ExtendedSelection)

            self._ui.propertyView.setHorizontalScrollMode(
                QtWidgets.QAbstractItemView.ScrollPerPixel)

            self._ui.frameSlider.setTracking(self._redrawOnScrub)

            for action in (self._ui.actionBlack,
                           self._ui.actionGrey_Dark,
                           self._ui.actionGrey_Light,
                           self._ui.actionWhite):
                action.setChecked(self._clearColorsDict[str(action.text())] == self._clearColor)
                self._ui.colorGroup.addAction(action)
            self._ui.colorGroup.setExclusive(True)

            for action in (self._ui.actionKey,
                           self._ui.actionFill,
                           self._ui.actionBack):
                self._ui.threePointLights.addAction(action)
            self._ui.threePointLights.setExclusive(False)
            self._updateLights()

            self._ui.renderModeActionGroup = QtWidgets.QActionGroup(self)
            for action in (self._ui.actionWireframe,
                           self._ui.actionWireframeOnSurface,
                           self._ui.actionSmooth_Shaded,
                           self._ui.actionFlat_Shaded,
                           self._ui.actionPoints,
                           self._ui.actionGeom_Only,
                           self._ui.actionGeom_Smooth,
                           self._ui.actionGeom_Flat,
                           self._ui.actionHidden_Surface_Wireframe):
                self._ui.renderModeActionGroup.addAction(action)
                action.setChecked(str(action.text()) == self._renderMode)
            self._ui.renderModeActionGroup.setExclusive(True)
            if self._renderMode not in RenderModes:
                print "Warning: Unknown render mode '%s', falling back to '%s'" % (
                            self._renderMode,
                            str(self._ui.renderModeActionGroup.actions()[0].text()))

                self._ui.renderModeActionGroup.actions()[0].setChecked(True)
                self._changeRenderMode(self._ui.renderModeActionGroup.actions()[0])

            self._ui.pickModeActionGroup = QtWidgets.QActionGroup(self)
            for action in (self._ui.actionPick_Prims,
                           self._ui.actionPick_Models,
                           self._ui.actionPick_Instances):
                self._ui.pickModeActionGroup.addAction(action)
                action.setChecked(str(action.text()) == self._pickMode)
            self._ui.pickModeActionGroup.setExclusive(True)
            if self._pickMode not in PickModes:
                print "Warning: Unknown pick mode '%s', falling back to '%s'" % (
                            self._pickMode,
                            str(self._ui.pickModeActionGroup.actions()[0].text()))

                self._ui.pickModeActionGroup.actions()[0].setChecked(True)
                self._changePickMode(self._ui.pickModeActionGroup.actions()[0])

            # The error-checking pattern here seems wrong?  Error checking
            # should happen in the changeXXX methods. All we're checking here
            # is the values we hardcoded ourselves in the init function!
            self._ui.selHighlightModeActionGroup = QtWidgets.QActionGroup(self)
            for action in (self._ui.actionNever,
                           self._ui.actionOnly_when_paused,
                           self._ui.actionAlways):
                self._ui.selHighlightModeActionGroup.addAction(action)
                action.setChecked(str(action.text()) == self._selHighlightMode)
            self._ui.selHighlightModeActionGroup.setExclusive(True)

            self._ui.highlightColorActionGroup = QtWidgets.QActionGroup(self)
            for action in (self._ui.actionSelYellow,
                           self._ui.actionSelCyan,
                           self._ui.actionSelWhite):
                self._ui.highlightColorActionGroup.addAction(action)
                action.setChecked(str(action.text()) == self._highlightColorName)
            self._ui.highlightColorActionGroup.setExclusive(True)

            self._ui.interpolationActionGroup = QtWidgets.QActionGroup(self)
            self._ui.interpolationActionGroup.setExclusive(True)
            for interpolationType in Usd.InterpolationType.allValues:
                action = self._ui.menuInterpolation.addAction(interpolationType.displayName)
                action.setCheckable(True)
                action.setChecked(
                    self._rootDataModel.stage.GetInterpolationType() == interpolationType)
                self._ui.interpolationActionGroup.addAction(action)

            self._ui.primViewDepthGroup = QtWidgets.QActionGroup(self)
            for i in range(1, 9):
                action = getattr(self._ui, "actionLevel_" + str(i))
                self._ui.primViewDepthGroup.addAction(action)

            # setup animation objects for the primView and propertyView
            self._propertyLegendAnim = QtCore.QPropertyAnimation(
                self._ui.propertyLegendContainer, "maximumHeight")
            self._primLegendAnim = QtCore.QPropertyAnimation(
                self._ui.primLegendContainer, "maximumHeight")

            # set the context menu policy for the prim browser and attribute
            # inspector headers. This is so we can have a context menu on the
            # headers that allows you to select which columns are visible.
            self._ui.propertyView.header()\
                    .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
            self._ui.primView.header()\
                    .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

            # Set custom context menu for attribute browser
            self._ui.propertyView\
                    .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

            # Set custom context menu for layer stack browser
            self._ui.layerStackView\
                    .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

            # Set custom context menu for composition tree browser
            self._ui.compositionTreeWidget\
                    .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)

            # Arc path is the most likely to need stretch.
            twh = self._ui.compositionTreeWidget.header()
            twh.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            twh.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            twh.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
            twh.setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)

            # Set the prim view header to have a fixed size type and vis columns
            nvh = self._ui.primView.header()
            nvh.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
            nvh.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            nvh.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)

            pvh = self._ui.propertyView.header()
            pvh.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            pvh.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
            pvh.setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)

            # XXX:
            # To avoid QTBUG-12850 (https://bugreports.qt.io/browse/QTBUG-12850),
            # we force the horizontal scrollbar to always be visible for all
            # QTableWidget widgets in use.
            self._ui.primView.setHorizontalScrollBarPolicy(
                QtCore.Qt.ScrollBarAlwaysOn)
            self._ui.propertyView.setHorizontalScrollBarPolicy(
                QtCore.Qt.ScrollBarAlwaysOn)
            self._ui.metadataView.setHorizontalScrollBarPolicy(
                QtCore.Qt.ScrollBarAlwaysOn)
            self._ui.layerStackView.setHorizontalScrollBarPolicy(
                QtCore.Qt.ScrollBarAlwaysOn)

            self._ui.attributeValueEditor.setAppController(self)

            self._ui.currentPathWidget.editingFinished.connect(
                self._currentPathChanged)

            self._ui.primView.itemSelectionChanged.connect(
                self._itemSelectionChanged)

            self._ui.primView.itemClicked.connect(self._itemClicked)

            self._ui.primView.header().customContextMenuRequested.connect(
                self._primViewHeaderContextMenu)

            self._timer.timeout.connect(self._advanceFrameForPlayback)

            self._ui.primView.customContextMenuRequested.connect(
                self._primViewContextMenu)

            self._ui.primView.expanded.connect(self._primViewExpanded)

            self._ui.frameSlider.valueChanged.connect(self.setFrame)

            self._ui.frameSlider.sliderMoved.connect(self._sliderMoved)

            self._ui.frameSlider.sliderReleased.connect(self._updateOnFrameChange)

            self._ui.frameField.editingFinished.connect(self._frameStringChanged)

            self._ui.rangeBegin.editingFinished.connect(self._rangeBeginChanged)

            self._ui.stepSize.editingFinished.connect(self._stepSizeChanged)

            self._ui.rangeEnd.editingFinished.connect(self._rangeEndChanged)

            self._ui.actionFrame_Forward.triggered.connect(self._advanceFrame)

            self._ui.actionFrame_Backwards.triggered.connect(self._retreatFrame)

            self._ui.actionReset_View.triggered.connect(self._resetView)

            self._ui.topBottomSplitter.splitterMoved.connect(self._cacheViewerModeEscapeSizes)
            self._ui.primStageSplitter.splitterMoved.connect(self._cacheViewerModeEscapeSizes)
            self._ui.actionToggle_Viewer_Mode.triggered.connect(
                self._toggleViewerMode)

            self._ui.showBBoxes.toggled.connect(self._toggleShowBBoxes)

            self._ui.showAABBox.toggled.connect(self._toggleShowAABBox)

            self._ui.showOBBox.toggled.connect(self._toggleShowOBBox)

            self._ui.showBBoxPlayback.toggled.connect(self._toggleShowBBoxPlayback)

            self._ui.useExtentsHint.toggled.connect(self._setUseExtentsHint)

            self._ui.showInterpreter.triggered.connect(self._showInterpreter)

            self._ui.redrawOnScrub.toggled.connect(self._redrawOptionToggled)

            if self._stageView:
                self._ui.actionRecompute_Clipping_Planes.triggered.connect(
                    self._stageView.detachAndReClipFromCurrentCamera)

            self._ui.actionAdjust_Clipping.triggered[bool].connect(
                self._adjustClippingPlanes)

            self._ui.actionAdjust_Default_Material.triggered[bool].connect(
                self._adjustDefaultMaterial)

            self._ui.actionOpen.triggered.connect(self._openFile)

            self._ui.actionSave_Overrides_As.triggered.connect(
                self._saveOverridesAs)

            # Setup quit actions to ensure _cleanAndClose is only invoked once.
            self._ui.actionQuit.triggered.connect(QtWidgets.QApplication.instance().quit)

            QtWidgets.QApplication.instance().aboutToQuit.connect(self._cleanAndClose)

            self._ui.actionReopen_Stage.triggered.connect(self._reopenStage)

            self._ui.actionReload_All_Layers.triggered.connect(self._reloadStage)

            self._ui.actionFrame_Selection.triggered.connect(self._frameSelection)

            self._ui.actionToggle_Framed_View.triggered.connect(self._toggleFramedView)

            self._ui.actionAdjust_FOV.triggered.connect(self._adjustFOV)

            self._ui.actionComplexity.triggered.connect(self._adjustComplexity)

            self._ui.actionDisplay_Guide.toggled.connect(self._toggleDisplayGuide)

            self._ui.actionDisplay_Proxy.toggled.connect(self._toggleDisplayProxy)

            self._ui.actionDisplay_Render.toggled.connect(self._toggleDisplayRender)

            self._ui.actionDisplay_Camera_Oracles.toggled.connect(
                self._toggleDisplayCameraOracles)

            self._ui.actionDisplay_PrimId.toggled.connect(self._toggleDisplayPrimId)

            self._ui.actionEnable_Hardware_Shading.toggled.connect(
                self._toggleEnableHardwareShading)

            self._ui.actionCull_Backfaces.toggled.connect(self._toggleCullBackfaces)

            self._ui.attributeInspector.currentChanged.connect(
                self._updateAttributeInspector)

            self._ui.propertyView.itemClicked.connect(self._propertyViewItemClicked)

            self._ui.propertyView.currentItemChanged.connect(
                self._populateAttributeInspector)

            self._ui.propertyView.header().customContextMenuRequested.\
                connect(self._propertyViewHeaderContextMenu)

            self._ui.propertyView.customContextMenuRequested.connect(
                self._propertyViewContextMenu)

            self._ui.layerStackView.customContextMenuRequested.connect(
                self._layerStackContextMenu)

            self._ui.compositionTreeWidget.customContextMenuRequested.connect(
                self._compositionTreeContextMenu)

            self._ui.compositionTreeWidget.currentItemChanged.connect(
                self._onCompositionSelectionChanged)

            self._ui.renderModeActionGroup.triggered.connect(self._changeRenderMode)

            self._ui.pickModeActionGroup.triggered.connect(self._changePickMode)

            self._ui.selHighlightModeActionGroup.triggered.connect(
                self._changeSelHighlightMode)

            self._ui.highlightColorActionGroup.triggered.connect(
                self._changeHighlightColor)

            self._ui.interpolationActionGroup.triggered.connect(
                self._changeInterpolationType)

            self._ui.actionAmbient_Only.triggered[bool].connect(
                self._ambientOnlyClicked)

            self._ui.actionKey.triggered[bool].connect(self._onKeyLightClicked)

            self._ui.actionFill.triggered[bool].connect(self._onFillLightClicked)

            self._ui.actionBack.triggered[bool].connect(self._onBackLightClicked)

            self._ui.colorGroup.triggered.connect(self._changeBgColor)

            if self._stageView:
                self._ui.threePointLights.triggered.connect(self._stageView.update)

            self._ui.primViewDepthGroup.triggered.connect(self._changePrimViewDepth)

            self._ui.actionExpand_All.triggered.connect(
                lambda: self._expandToDepth(1000000))

            self._ui.actionCollapse_All.triggered.connect(
                self._ui.primView.collapseAll)

            self._ui.actionShow_Inactive_Prims.toggled.connect(
                self._toggleShowInactivePrims)

            self._ui.actionShow_All_Master_Prims.toggled.connect(
                self._toggleShowMasterPrims)

            self._ui.actionShow_Undefined_Prims.toggled.connect(
                self._toggleShowUndefinedPrims)

            self._ui.actionShow_Abstract_Prims.toggled.connect(
                self._toggleShowAbstractPrims)

            self._ui.actionRollover_Prim_Info.toggled.connect(
                self._toggleRolloverPrimInfo)

            self._ui.primViewLineEdit.returnPressed.connect(
                self._ui.primViewFindNext.click)

            self._ui.primViewFindNext.clicked.connect(self._primViewFindNext)

            self._ui.attrViewLineEdit.returnPressed.connect(
                self._ui.attrViewFindNext.click)

            self._ui.attrViewFindNext.clicked.connect(self._attrViewFindNext)

            self._ui.primLegendQButton.clicked.connect(
                self._primLegendToggleCollapse)

            self._ui.propertyLegendQButton.clicked.connect(
                self._propertyLegendToggleCollapse)

            self._ui.playButton.clicked.connect(self._playClicked)

            self._ui.actionCameraMask_Full.triggered.connect(
                self._updateCameraMaskMenu)

            self._ui.actionCameraMask_Partial.triggered.connect(
                self._updateCameraMaskMenu)

            self._ui.actionCameraMask_None.triggered.connect(
                self._updateCameraMaskMenu)

            self._ui.actionCameraMask_Outline.triggered.connect(
                self._updateCameraMaskMenu)

            self._ui.actionCameraMask_Color.triggered.connect(
                self._pickCameraMaskColor)

            self._ui.actionCameraReticles_Inside.triggered.connect(
                self._updateCameraReticlesMenu)

            self._ui.actionCameraReticles_Outside.triggered.connect(
                self._updateCameraReticlesMenu)

            self._ui.actionCameraReticles_Color.triggered.connect(
                self._pickCameraReticlesColor)

            self._ui.actionHUD.triggered.connect(self._HUDMenuChangedInfoRefresh)

            self._ui.actionHUD_Info.triggered.connect(self._HUDMenuChangedInfoRefresh)

            self._ui.actionHUD_Complexity.triggered.connect(self._HUDMenuChanged)

            self._ui.actionHUD_Performance.triggered.connect(self._HUDMenuChanged)

            self._ui.actionHUD_GPUstats.triggered.connect(self._HUDMenuChanged)

            self._mainWindow.addAction(self._ui.actionIncrementComplexity1)
            self._mainWindow.addAction(self._ui.actionIncrementComplexity2)
            self._mainWindow.addAction(self._ui.actionDecrementComplexity)

            self._ui.actionIncrementComplexity1.triggered.connect(
                self._incrementComplexity)

            self._ui.actionIncrementComplexity2.triggered.connect(
                self._incrementComplexity)

            self._ui.actionDecrementComplexity.triggered.connect(
                self._decrementComplexity)

            self._ui.attributeValueEditor.editComplete.connect(self.editComplete)

            # Edit Prim menu
            self._ui.menuEdit_Prim.aboutToShow.connect(self._updateEditPrimMenu)

            self._ui.actionFind_Prims.triggered.connect(
                self._ui.primViewLineEdit.setFocus)

            self._ui.actionJump_to_Stage_Root.triggered.connect(
                self.resetSelectionToPseudoroot)

            self._ui.actionJump_to_Model_Root.triggered.connect(
                self.jumpToEnclosingModelSelectedPrims)

            self._ui.actionJump_to_Bound_Material.triggered.connect(
                self.jumpToBoundMaterialSelectedPrims)

            self._ui.actionMake_Visible.triggered.connect(self.visSelectedPrims)
            # Add extra, Presto-inspired shortcut for Make Visible
            self._ui.actionMake_Visible.setShortcuts(["Shift+H", "Ctrl+Shift+H"])

            self._ui.actionMake_Invisible.triggered.connect(self.invisSelectedPrims)

            self._ui.actionVis_Only.triggered.connect(self.visOnlySelectedPrims)

            self._ui.actionRemove_Session_Visibility.triggered.connect(
                self.removeVisSelectedPrims)

            self._ui.actionReset_All_Session_Visibility.triggered.connect(
                self.resetSessionVisibility)

            self._ui.actionLoad.triggered.connect(self.loadSelectedPrims)

            self._ui.actionUnload.triggered.connect(self.unloadSelectedPrims)

            self._ui.actionActivate.triggered.connect(self.activateSelectedPrims)

            self._ui.actionDeactivate.triggered.connect(self.deactivateSelectedPrims)

            self._setupDebugMenu()

            # configure plugins
            self._configurePlugins()

            # timer for slider. when user stops scrubbing for 0.5s, update stuff.
            self._sliderTimer = QtCore.QTimer(self)
            self._sliderTimer.setInterval(500)

            # Connect the update timer to _frameStringChanged, which will ensure
            # we update _currentTime prior to updating UI
            self._sliderTimer.timeout.connect(self._frameStringChanged)

            # We manually call processEvents() here to make sure that the prim
            # browser and other widgetry get drawn before we draw the first image in
            # the viewer, which might take a long time.
            if self._stageView:
                self._stageView.setUpdatesEnabled(False)

            self._mainWindow.update()

            QtWidgets.QApplication.processEvents()

        if self._printTiming:
            uiOpenTimer.PrintTime('bring up the UI')

        self._drawFirstImage()

        QtWidgets.QApplication.restoreOverrideCursor()

    def _drawFirstImage(self):
        # _resetView is what triggers the first image to be drawn, so time it
        if self._stageView:
            self._stageView.setUpdatesEnabled(True)
        with BusyContext(), Timer() as t:
            try:
                self._resetView(self._initialSelectPrim)
            except Exception:
                pass
        if self._printTiming and self._stageView:
            t.PrintTime("create first image")

        # configure render plugins after stageView initialized its renderer.
        self._configureRendererPlugins()

        if self._mallocTags == 'stageAndImaging':
            DumpMallocTags(self._rootDataModel.stage,
                "stage-loading and imaging")

    def statusMessage(self, msg, timeout = 0):
        self._statusBar.showMessage(msg, timeout * 1000)

    def editComplete(self, msg):
        title = self._mainWindow.windowTitle()
        if title[-1] != '*':
            self._mainWindow.setWindowTitle(title + ' *')

        self.statusMessage(msg, 12)
        with Timer() as t:
            if self._stageView:
                self._stageView.setSelectedPrims(self._prunedCurrentPrims,
                      self._currentFrame, resetCam=False, forceComputeBBox=True)
            self._refreshVars()
        if self._printTiming:
            t.PrintTime("'%s'" % msg)

    def _openStage(self, usdFilePath, populationMaskPaths):
        Ar.GetResolver().ConfigureResolverForAsset(usdFilePath)
        self._pathResolverContext = \
            Ar.GetResolver().CreateDefaultContextForAsset(usdFilePath)

        def _GetFormattedError(reasons=[]):
            err = ("Error: Unable to open stage '{0}'\n".format(usdFilePath))
            if reasons:
                err += "\n".join(reasons) + "\n"
            return err

        if not os.path.isfile(usdFilePath):
            sys.stderr.write(_GetFormattedError(["File not found"]))
            sys.exit(1)

        if self._mallocTags != 'none':
            Tf.MallocTag.Initialize()

        with Timer() as t:
            loadSet = Usd.Stage.LoadNone if self._unloaded else Usd.Stage.LoadAll
            preloadSet = loadSet if self._noRender else Usd.Stage.LoadNone

            popMask = (None if populationMaskPaths is None else
                       Usd.StagePopulationMask())

            # Open as a layer first to make sure its a valid file format
            try:
                layer = Sdf.Layer.FindOrOpen(usdFilePath)
            except Tf.ErrorException as e:
                sys.stderr.write(_GetFormattedError(
                    [err.commentary.strip() for err in e.args]))
                sys.exit(1)

            if popMask:
                for p in populationMaskPaths:
                    popMask.Add(p)
                stage = Usd.Stage.OpenMasked(
                    layer, self._pathResolverContext, popMask, preloadSet)
            else:
                stage = Usd.Stage.Open(
                    layer, self._pathResolverContext, preloadSet)

            # no point in optimizing for editing if we're not redrawing
            if stage and not self._noRender:
                # as described in bug #99309, UsdStage change processing
                # can't yet tell the difference between an effectively
                # "inert" change caused by adding an empty Over primSpec, and
                # creation of a primSpec that has real effect on the stage.
                # Therefore there is a massive invalidation from the root of
                # the stage when the first edit on the stage is made
                # (e.g. invising something), and while the UsdStage itself
                # recovers quickly, clients like Hydra must throw everything
                # away and start over.  Here, we limit the propagation of
                # invalidation due to creation of new overs to the enclosing
                # model by pre-populating the session layer with overs for
                # the interior of the model hierarchy, before the renderer
                # starts listening for changes.
                #
                # When bug #99309 is fixed, we can eliminate the "preloadSet"
                # and double-stage load. It turns out to be typically
                # enormously expensive to recompose the whole stage (which is
                # what happens if we add the overs directly to stage's session
                # layer).  So we "preload" the stage above in an unloaded state
                # and add the overs to a detached layer that we will then use
                # to open the stage a second time with the desired load-state,
                # while still holding the preload-stage open.  This turns out
                # to be much, much cheaper for scenes that are properly
                # payloaded for scalability.
                sl = Sdf.Layer.CreateAnonymous("usdview-session.usda")

                # We can only safely do Sdf-level ops inside an Sdf.ChangeBlock,
                # so gather all the paths from the UsdStage first.
                # We don't technically need the ChangeBlock since no-one is
                # listening to this detached layer, but good batch-editing
                # practice.
                modelPaths = [p.GetPath() for p in \
                                  Usd.PrimRange.Stage(stage,
                                                      Usd.PrimIsModel) ]
                with Sdf.ChangeBlock():
                    for mpp in modelPaths:
                        parent = sl.GetPrimAtPath(mpp.GetParentPath())
                        Sdf.PrimSpec(parent, mpp.name, Sdf.SpecifierOver)

                if popMask:
                    stage2 = Usd.Stage.OpenMasked(
                        layer, sl, self._pathResolverContext,
                        popMask, loadSet)
                else:
                    stage2 = Usd.Stage.Open(
                        layer, sl, self._pathResolverContext, loadSet)
                stage = stage2

        if not stage:
            sys.stderr.write(_GetFormattedError())
        else:
            if self._printTiming:
                t.PrintTime('open stage "%s"' % usdFilePath)
            stage.SetEditTarget(stage.GetSessionLayer())

        if self._mallocTags == 'stage':
            DumpMallocTags(stage, "stage-loading")

        return stage

    def _closeStage(self):
        # Close the USD stage.
        if self._stageView:
            self._stageView.closeRenderer()
        self._rootDataModel.stage = None

    def _setPlayShortcut(self):
        self._ui.playButton.setShortcut(QtGui.QKeySequence(QtCore.Qt.Key_Space))

    # Non-topology dependent UI changes
    def _reloadFixedUI(self, resetStageDataOnly=False):
        # If animation is playing, stop it.
        if self._playing:
            self._ui.playButton.click()

        # frame range supplied by user
        ff = self._parserData.firstframe
        lf = self._parserData.lastframe

        # frame range supplied by stage
        stageStartTimeCode = self._rootDataModel.stage.GetStartTimeCode()
        stageEndTimeCode = self._rootDataModel.stage.GetEndTimeCode()

        # final range results
        self.realStartTimeCode = None
        self.realEndTimeCode = None

        self.framesPerSecond = self._rootDataModel.stage.GetFramesPerSecond()

        if not resetStageDataOnly:
            self.step = self._rootDataModel.stage.GetTimeCodesPerSecond() / self.framesPerSecond
            self._ui.stepSize.setText(str(self.step))

        # if one option is provided(lastframe or firstframe), we utilize it
        if ff is not None and lf is not None:
            self.realStartTimeCode = ff
            self.realEndTimeCode = lf
        elif ff is not None:
            self.realStartTimeCode = ff
            self.realEndTimeCode = stageEndTimeCode
        elif lf is not None:
            self.realStartTimeCode = stageStartTimeCode
            self.realEndTimeCode = lf
        elif self._rootDataModel.stage.HasAuthoredTimeCodeRange():
            self.realStartTimeCode = stageStartTimeCode
            self.realEndTimeCode = stageEndTimeCode

        self._ui.stageBegin.setText(str(stageStartTimeCode))
        self._ui.stageEnd.setText(str(stageEndTimeCode))

        self._UpdateTimeSamples(resetStageDataOnly)

    def _UpdateTimeSamples(self, resetStageDataOnly=False):
        if self.realStartTimeCode is not None and self.realEndTimeCode is not None:
            if self.realStartTimeCode > self.realEndTimeCode:
                sys.stderr.write('Warning: Invalid frame range (%s, %s)\n'
                % (self.realStartTimeCode, self.realEndTimeCode))
                self._timeSamples = []
            else:
                self._timeSamples = Drange(self.realStartTimeCode,
                                           self.realEndTimeCode,
                                           self.step)
        else:
            self._timeSamples = []

        self._geomCounts = dict()
        self._hasTimeSamples = (len(self._timeSamples) > 0)
        self._setPlaybackAvailability() # this sets self._playbackAvailable

        if self._hasTimeSamples:
            self._ui.rangeBegin.setText(str(self._timeSamples[0]))
            self._ui.rangeEnd.setText(str(self._timeSamples[-1]))

        if not resetStageDataOnly:
            self._currentFrame = self._timeSamples[0] if self._hasTimeSamples else 0.0
            self._ui.frameField.setText(str(self._currentFrame))

        if self._playbackAvailable:
            if not resetStageDataOnly:
                self._ui.frameSlider.setRange(0, len(self._timeSamples)-1)
                self._ui.frameSlider.setValue(self._ui.frameSlider.minimum())
            self._setPlayShortcut()
            self._ui.playButton.setCheckable(True)
            self._ui.playButton.setChecked(False)

    # Vars that need updating during a stage reget/refresh
    def _refreshVars(self):
        # Need to refresh selected items to refresh prims/view to new stage
        self._itemSelectionChanged()

    def _refreshBBoxCache(self, useExtentsHint):
        # Unfortunate that we must blow the entire BBoxCache, but we have no
        # other alternative, currently.
        if self._bboxCache and self._bboxCache.GetUseExtentsHint() == useExtentsHint:
            self._bboxCache.Clear()
        else:
            self._bboxCache = UsdGeom.BBoxCache(self._currentFrame,
                                                StageView.DefaultDataModel.BBOXPURPOSES,
                                                useExtentsHint)

    def _clearCaches(self, preserveCamera=False):
        """Clears value and computation caches maintained by the controller.
        Does NOT initiate any GUI updates"""

        self._valueCache = dict()
        self._geomCounts = dict()

        # create new xform, bounding box, and camera caches. If there was an
        # instance of the cache before, this will effectively clear the
        # cache.
        self._xformCache = UsdGeom.XformCache(self._currentFrame)
        self._refreshBBoxCache(self._ui.useExtentsHint.isChecked())
        self._refreshCameraListAndMenu(preserveCurrCamera = preserveCamera)


    # Render plugin support
    def _rendererPluginChanged(self, plugin):
        if self._stageView:
            self._stageView.SetRendererPlugin(plugin)

    def _configureRendererPlugins(self):
        if self._stageView:
            self._ui.rendererPluginActionGroup = QtWidgets.QActionGroup(self)
            self._ui.rendererPluginActionGroup.setExclusive(True)

            pluginTypes = self._stageView.GetRendererPlugins()
            for pluginType in pluginTypes:
                name = self._stageView.GetRendererPluginDisplayName(pluginType)
                action = self._ui.menuRendererPlugin.addAction(name)
                action.setCheckable(True)
                action.pluginType = pluginType
                self._ui.rendererPluginActionGroup.addAction(action)

                action.triggered.connect(lambda pluginType=pluginType:
                        self._rendererPluginChanged(pluginType))

            # If any plugins exist, the first render plugin is the default one.
            if len(self._ui.rendererPluginActionGroup.actions()) > 0:
                self._ui.rendererPluginActionGroup.actions()[0].setChecked(True)
                self._stageView.SetRendererPlugin(pluginTypes[0])

            # Otherwise, put a no-op placeholder in.
            else:
                action = self._ui.menuRendererPlugin.addAction('Default')
                action.setCheckable(True)
                action.setChecked(True)
                self._ui.rendererPluginActionGroup.addAction(action)


    # Topology-dependent UI changes
    def _reloadVaryingUI(self):

        self._clearCaches()

        # The difference between these two is related to multi-selection:
        # - currentPrims contains all prims selected
        # - prunedCurrentPrims contains all prims selected, excluding prims that
        #   already have a parent selected (used to avoid double-rendering)
        self._currentPrims = [self._rootDataModel.stage.GetPseudoRoot()]
        self._prunedCurrentPrims = self._currentPrims

        if self._debug:
            cProfile.runctx('self._resetPrimView(restoreSelection=False)', globals(), locals(), 'resetPrimView')
            p = pstats.Stats('resetPrimView')
            p.strip_dirs().sort_stats(-1).print_stats()
        else:
            self._resetPrimView(restoreSelection=False)

        self._ui.frameSlider.setValue(self._ui.frameSlider.minimum())

        if not self._stageView:

            # The second child is self._ui.glFrame, which disappears if
            # its size is set to zero.
            if self._noRender:
                # remove glFrame from the ui
                self._ui.glFrame.setParent(None)

                # move the attributeBrowser into the primSplitter instead
                self._ui.primStageSplitter.addWidget(self._ui.attributeBrowserFrame)

            else:
                self._stageView = StageView(parent=self._mainWindow,
                    dataModel=self, rootDataModel=self._rootDataModel,
                    printTiming=self._printTiming)

                self._stageView.fpsHUDInfo = self._fpsHUDInfo
                self._stageView.fpsHUDKeys = self._fpsHUDKeys

                self._stageView.signalPrimSelected.connect(self.onPrimSelected)
                self._stageView.signalPrimRollover.connect(self.onRollover)
                self._stageView.signalMouseDrag.connect(self.onStageViewMouseDrag)
                self._stageView.signalErrorMessage.connect(self.statusMessage)

                layout = QtWidgets.QVBoxLayout()
                layout.setContentsMargins(0, 0, 0, 0)
                self._ui.glFrame.setLayout(layout)
                layout.addWidget(self._stageView)

        self._playbackFrameIndex = 0

        self._primSearchResults = deque([])
        self._attrSearchResults = deque([])
        self._primSearchString = ""
        self._attrSearchString = ""
        self._lastPrimSearched = self._currentPrims[0]

        if self._stageView:
            self._stageView.setFocus(QtCore.Qt.TabFocusReason)
            self._stageView.rolloverPicking = self._rolloverPrimInfo

    # This appears to be "reasonably" performant in normal sized pose caches.
    # If it turns out to be too slow, or if we want to do a better job of
    # preserving the view the user currently has, we could look into ways of
    # reconstructing just the prim tree under the "changed" prim(s).  The
    # (far and away) faster solution would be to implement our own TreeView
    # and model in C++.
    def _resetPrimView(self, restoreSelection=True):
        with Timer() as t, BusyContext():
            startingDepth = 3
            self._primViewResetTimer.stop()
            self._computeDisplayPredicate()
            self._ui.primView.setUpdatesEnabled(False)
            self._ui.primView.clear()
            self._primToItemMap.clear()
            self._itemsToPush = []
            # force new search since we are blowing away the primViewItems
            # that may be cached in _primSearchResults
            self._primSearchResults = []
            self._populateRoots()
            # it's confusing to see timing for expand followed by reset with
            # the times being similar (esp when they are large)
            self._expandToDepth(startingDepth, suppressTiming=True)
            if restoreSelection:
                self._setSelectionFromPrimList(self._currentPrims)
            self._ui.primView.setUpdatesEnabled(True)
            self._refreshCameraListAndMenu(preserveCurrCamera = True)
        if self._printTiming:
            t.PrintTime("reset Prim Browser to depth %d" % startingDepth)

    def UpdatePrimViewContents(self):
        """Will schedule a full refresh/resync of the Prim Browser's contents.
        Prefer this to calling _resetPrimView() directly, since it will
        coalesce multiple calls to this method in to a single refresh"""
        self._primViewResetTimer.stop()
        self._primViewResetTimer.start(250)

    def _resetPrimViewVis(self, selItemsOnly=True,
                          authoredVisHasChanged=True):
        """Updates browser rows' Vis columns... can update just selected
        items (and their descendants and ancestors), or all items in the
        primView.  When authoredVisHasChanged is True, we force each item
        to discard any value caches it may be holding onto."""
        with Timer() as t:
            self._ui.primView.setUpdatesEnabled(False)
            rootsToProcess = self.getSelectedItems() if selItemsOnly else \
                [self._ui.primView.invisibleRootItem()]
            for item in rootsToProcess:
                PrimViewItem.propagateVis(item, authoredVisHasChanged)
            self._ui.primView.setUpdatesEnabled(True)
        if self._printTiming:
            t.PrintTime("update vis column")

    def _updatePrimView(self):
        # Process some more prim view items.
        n = min(100, len(self._itemsToPush))
        if n:
            items = self._itemsToPush[-n:]
            del self._itemsToPush[-n:]
            for item in items:
                item.push()
        else:
            self._primViewUpdateTimer.stop()

    # Option windows ==========================================================

    def _incrementComplexity(self):
        self._complexity += .1
        if self._complexity > 1.999:
            self._complexity = 2.0
        if self._stageView:
            self._stageView.update()

    def _decrementComplexity(self):
        self._complexity -= .1
        if self._complexity < 1.001:
            self._complexity = 1.0
        if self._stageView:
            self._stageView.update()

    def _adjustComplexity(self):
        complexity= QtWidgets.QInputDialog.getDouble(self._mainWindow,
            "Adjust complexity", "Enter a value between 1 and 2.\n\n"
            "You can also use ctrl+ or ctrl- to adjust the\n"
            "complexity without invoking this dialog.\n",
            self._complexity, 1.0,2.0,2)
        if complexity[1]:
            self._complexity = complexity[0]
            if self._stageView:
                self._stageView.update()

    def _adjustFOV(self):
        fov = QtWidgets.QInputDialog.getDouble(self._mainWindow, "Adjust FOV",
            "Enter a value between 0 and 180", self._freeCamera.fov, 0, 180)
        if (fov[1]):
            self._freeCamera.fov = fov[0]
            if self._stageView:
                self._stageView.update()

    def _adjustClippingPlanes(self, checked):
        # Eventually, this will not be accessible when _stageView is None.
        # Until then, silently ignore.
        if self._stageView:
            if (checked):
                self._adjustClippingDlg = adjustClipping.AdjustClipping(self._mainWindow,
                                                                     self._stageView)
                self._adjustClippingDlg.finished.connect(
                    lambda status : self._ui.actionAdjust_Clipping.setChecked(False))

                self._adjustClippingDlg.show()
            else:
                self._adjustClippingDlg.close()

    @property
    def xformCache(self):
        return self._xformCache

    @property
    def bboxCache(self):
        return self._bboxCache

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

    @property
    def complexity(self):
        return self._complexity

    @complexity.setter
    def complexity(self, value):
        self._complexity = value

    @property
    def clearColor(self):
        return self._clearColor

    @property
    def renderMode(self):
        return self._renderMode

    @renderMode.setter
    def renderMode(self, value):
        self._renderMode = value

    @property
    def pickMode(self):
        return self._pickMode

    @property
    def freeCamera(self):
        return self._freeCamera

    @freeCamera.setter
    def freeCamera(self, value):
        self._freeCamera = value

    @property
    def playing(self):
        return self._playing

    @property
    def showAABBox(self):
        return self._showAABBox

    @property
    def showOBBox(self):
        return self._showOBBox

    @property
    def showBBoxes(self):
        return self._showBBoxes

    @showBBoxes.setter
    def showBBoxes(self, value):
        self._showBBoxes = value

    @property
    def showBBoxPlayback(self):
        return self._showBBoxPlayback

    @property
    def displayGuide(self):
        return self._displayGuide

    @property
    def displayProxy(self):
        return self._displayProxy

    @property
    def displayRender(self):
        return self._displayRender

    @property
    def displayCameraOracles(self):
        return self._displayCameraOracles

    @property
    def displayPrimId(self):
        return self._displayPrimId

    @property
    def enableHardwareShading(self):
        return self._enableHardwareShading

    @property
    def cullBackfaces(self):
        return self._cullBackfaces

    @property
    def cameraMaskMode(self):
        return self._cameraMaskMode

    @property
    def showMask(self):
        return self._cameraMaskMode in (CameraMaskModes.FULL, CameraMaskModes.PARTIAL)

    @property
    def showMask_Opaque(self):
        return self._cameraMaskMode == CameraMaskModes.FULL

    @property
    def showMask_Outline(self):
        return self._showMask_Outline

    @property
    def showReticles_Inside(self):
        return self._showReticles_Inside

    @property
    def showReticles_Outside(self):
        return self._showReticles_Outside

    @property
    def showHUD(self):
        return self._showHUD

    @showHUD.setter
    def showHUD(self, value):
        self._showHUD = value

    @property
    def showHUD_Info(self):
        return self._showHUD_Info

    @property
    def showHUD_Complexity(self):
        return self._showHUD_Complexity

    @property
    def showHUD_Performance(self):
        return self._showHUD_Performance

    @property
    def showHUD_GPUstats(self):
        return self._showHUD_GPUstats

    @property
    def ambientLightOnly(self):
        return self._ambientLightOnly

    @property
    def keyLightEnabled(self):
        return self._keyLightEnabled

    @property
    def fillLightEnabled(self):
        return self._fillLightEnabled

    @property
    def backLightEnabled(self):
        return self._backLightEnabled

    @property
    def highlightColorName(self):
        return self._highlightColorName

    @property
    def highlightColor(self):
        return self._highlightColor

    @property
    def selHighlightMode(self):
        return self._selHighlightMode

    @selHighlightMode.setter
    def selHighlightMode(self, value):
        self._selHighlightMode = value

    @property
    def drawSelHighlights(self):
        return self._drawSelHighlights

    @drawSelHighlights.setter
    def drawSelHighlights(self, value):
        self._drawSelHighlights = value

    @property
    def redrawOnScrub(self):
        return self._redrawOnScrub

    @redrawOnScrub.setter
    def redrawOnScrub(self, value):
        self._redrawOnScrub = value

    def ResetDefaultMaterialSettings(self):
        self._defaultMaterialAmbient = .2
        self._defaultMaterialSpecular = .1
        self.signalDefaultMaterialChanged.emit()

    def _adjustDefaultMaterial(self, checked):
        if (checked):
            self._adjustDefaultMaterialDlg = adjustDefaultMaterial.AdjustDefaultMaterial(
                self._mainWindow, self)
            self._adjustDefaultMaterialDlg.finished.connect(lambda status :
                self._ui.actionAdjust_Default_Material.setChecked(False))

            self._adjustDefaultMaterialDlg.show()
        else:
            self._adjustDefaultMaterialDlg.close()

    def _redrawOptionToggled(self, checked):
        self._redrawOnScrub = checked
        self._ui.frameSlider.setTracking(self._redrawOnScrub)

    # Frame-by-frame/Playback functionality ===================================

    def _setPlaybackAvailability(self, enabled = True):
        isEnabled = len(self._timeSamples) > 1 and enabled
        self._playbackAvailable = isEnabled

        #If playback is disabled, but the animation is playing...
        if not isEnabled and self._playing:
            self._ui.playButton.click()

        self._ui.playButton.setEnabled(isEnabled)
        self._ui.frameSlider.setEnabled(isEnabled)
        self._ui.actionFrame_Forward.setEnabled(isEnabled)
        self._ui.actionFrame_Backwards.setEnabled(isEnabled)
        self._ui.frameField.setEnabled(isEnabled
                                       if self._hasTimeSamples else False)
        self._ui.frameLabel.setEnabled(isEnabled
                                       if self._hasTimeSamples else False)
        self._ui.stageBegin.setEnabled(isEnabled)
        self._ui.stageEnd.setEnabled(isEnabled)
        self._ui.redrawOnScrub.setEnabled(isEnabled)


    def _playClicked(self):
        if self._ui.playButton.isChecked():
            # Start playback.
            self._playing = True
            self._ui.playButton.setText("Stop")
            # setText() causes the shortcut to be reset to whatever
            # Qt thinks it should be based on the text.  We know better.
            self._setPlayShortcut()
            self._fpsHUDInfo[HUDEntries.PLAYBACK]  = "..."
            self._timer.start()
            # For performance, don't update the prim tree view while playing.
            self._primViewUpdateTimer.stop()
            self._playbackIndex = 0
        else:
            # Stop playback.
            self._playing = False
            self._ui.playButton.setText("Play")
            # setText() causes the shortcut to be reset to whatever
            # Qt thinks it should be based on the text.  We know better.
            self._setPlayShortcut()
            self._fpsHUDInfo[HUDEntries.PLAYBACK]  = "N/A"
            self._timer.stop()
            self._primViewUpdateTimer.start()
            self._updateOnFrameChange(refreshUI=True)

    def _advanceFrameForPlayback(self):
        sleep(max(0, 1. / self.framesPerSecond - (time() - self._lastFrameTime)))
        self._lastFrameTime = time()
        if self._playbackIndex == 0:
            self._startTime = time()
        if self._playbackIndex == 4:
            self._endTime = time()
            delta = (self._endTime - self._startTime)/4.
            ms = delta * 1000.
            fps = 1. / delta
            self._fpsHUDInfo[HUDEntries.PLAYBACK] = "%.2f ms (%.2f FPS)" % (ms, fps)

        self._playbackIndex = (self._playbackIndex + 1) % 5
        self._advanceFrame()

    def _advanceFrame(self):
        if not self._playbackAvailable:
            return
        newValue = self._ui.frameSlider.value() + 1
        if newValue > self._ui.frameSlider.maximum():
            newValue = self._ui.frameSlider.minimum()
        self._ui.frameSlider.setValue(newValue)

    def _retreatFrame(self):
        if not self._playbackAvailable:
            return
        newValue = self._ui.frameSlider.value() - 1
        if newValue < self._ui.frameSlider.minimum():
            newValue = self._ui.frameSlider.maximum()
        self._ui.frameSlider.setValue(newValue)

    def _findIndexOfFieldContents(self, field):
        # don't convert string to float directly because of rounding error
        frameString = str(field.text())

        if frameString.count(".") == 0:
            frameString += ".0"
        elif frameString[-1] == ".":
            frameString += "0"

        field.setText(frameString)

        # Find the index of the closest valid frame
        dist = None
        closestIndex = Usd.TimeCode.Default()

        for i in range(len(self._timeSamples)):
            newDist = abs(self._timeSamples[i] - float(frameString))
            if dist is None or newDist < dist:
                dist = newDist
                closestIndex = i
        return closestIndex

    def _rangeBeginChanged(self):
        self.realStartTimeCode = float(self._ui.rangeBegin.text())
        self._UpdateTimeSamples(resetStageDataOnly=False)

    def _stepSizeChanged(self):
        stepStr = self._ui.stepSize.text()
        self.step = float(stepStr)
        self._UpdateTimeSamples(resetStageDataOnly=False)

    def _rangeEndChanged(self):
        self.realEndTimeCode = float(self._ui.rangeEnd.text())
        self._UpdateTimeSamples(resetStageDataOnly=False)

    def _frameStringChanged(self):
        indexOfFrame = self._findIndexOfFieldContents(self._ui.frameField)

        if (indexOfFrame != Usd.TimeCode.Default()):
            self.setFrame(indexOfFrame, forceUpdate=True)
            self._ui.frameSlider.setValue(indexOfFrame)

        self._ui.frameField.setText(str(self._currentFrame))

    def _sliderMoved(self, value):
        self._ui.frameField.setText(str(self._timeSamples[value]))
        self._sliderTimer.stop()
        self._sliderTimer.start()

    # Prim/Attribute search functionality =====================================

    def _findPrims(self, pattern, useRegex=True):
        """Search the Usd Stage for matching prims
        """
        # If pattern doesn't contain regexp special chars, drop
        # down to simple search, as it's faster
        if useRegex and re.match("^[0-9_A-Za-z]+$", pattern):
            useRegex = False
        if useRegex:
            isMatch = re.compile(pattern, re.IGNORECASE).search
        else:
            pattern = pattern.lower()
            isMatch = lambda x: pattern in x.lower()

        matches = [prim.GetPath() for prim
                   in Usd.PrimRange.Stage(self._rootDataModel.stage,
                                             self._displayPredicate)
                   if isMatch(prim.GetName())]

        if self._showAllMasterPrims:
            for master in self._rootDataModel.stage.GetMasters():
                matches += [prim.GetPath() for prim
                            in Usd.PrimRange(master, self._displayPredicate)
                            if isMatch(prim.GetName())]

        return matches

    def _primViewFindNext(self):
        if (self._primSearchString == self._ui.primViewLineEdit.text() and
            len(self._primSearchResults) > 0 and
            self._lastPrimSearched == self._currentPrims[0]):
            # Go to the next result of the currently ongoing search.
            # First time through, we'll be converting from SdfPaths
            # to items (see the append() below)
            nextResult = self._primSearchResults.popleft()
            if isinstance(nextResult, Sdf.Path):
                nextResult = self._getItemAtPath(nextResult)

            if nextResult:
                self._ui.primView.setCurrentItem(nextResult)
                self._primSearchResults.append(nextResult)
                self._lastPrimSearched = self._currentPrims[0]
            # The path is effectively pruned if we couldn't map the
            # path to an item
        else:
            # Begin a new search
            with Timer() as t:
                self._primSearchString = self._ui.primViewLineEdit.text()
                self._primSearchResults = self._findPrims(str(self._ui.primViewLineEdit.text()))

                self._primSearchResults = deque(self._primSearchResults)
                self._lastPrimSearched = self._currentPrims[0]

                if (len(self._primSearchResults) > 0):
                    self._primViewFindNext()
            if self._printTiming:
                t.PrintTime("match '%s' (%d matches)" %
                            (self._primSearchString,
                             len(self._primSearchResults)))

    def _primLegendToggleCollapse(self):
        ToggleLegendWithBrowser(self._ui.primLegendContainer,
                                self._ui.primLegendQButton,
                                self._primLegendAnim)

    def _propertyLegendToggleCollapse(self):
        ToggleLegendWithBrowser(self._ui.propertyLegendContainer,
                                self._ui.propertyLegendQButton,
                                self._propertyLegendAnim)

    def _attrViewFindNext(self):
        self._ui.propertyView.clearSelection()
        if (self._attrSearchString == self._ui.attrViewLineEdit.text() and
            len(self._attrSearchResults) > 0 and
            self._lastPrimSearched == self._currentPrims[0]):

            # Go to the next result of the currently ongoing search
            nextResult = self._attrSearchResults.popleft()

            nextResult.setSelected(True)
            self._ui.propertyView.scrollToItem(nextResult)
            self._attrSearchResults.append(nextResult)
            self._lastPrimSearched = self._currentPrims[0]

            itemName = str(nextResult.text(PropertyViewIndex.NAME))
            self._ui.attributeValueEditor.populate(itemName, self._currentPrims[0])
            self._updateMetadataView(self._getSelectedObject())
            self._updateLayerStackView(self._getSelectedObject())
        else:
            # Begin a new search
            self._attrSearchString = self._ui.attrViewLineEdit.text()
            self._attrSearchResults = self._ui.propertyView.findItems(
                self._ui.attrViewLineEdit.text(),
                QtCore.Qt.MatchRegExp,
                PropertyViewIndex.NAME)

            # Now just search for the string itself
            otherSearch = self._ui.propertyView.findItems(
                self._ui.attrViewLineEdit.text(),
                QtCore.Qt.MatchContains,
                PropertyViewIndex.NAME)

            self._attrSearchResults += otherSearch

            # We find properties first, then connections/targets
            # Based on the default recursive match finding in Qt.
            self._attrSearchResults.sort()
            self._attrSearchResults = deque(self._attrSearchResults)

            self._lastPrimSearched = self._currentPrims[0]
            if (len(self._attrSearchResults) > 0):
                self._attrViewFindNext()

    @classmethod
    def _outputBaseDirectory(cls):
        homeDirRoot = os.getenv('HOME') or os.path.expanduser('~')
        baseDir = os.path.join(homeDirRoot, '.usdview')

        try:
            if not os.path.exists(baseDir):
                os.makedirs(baseDir)
            return baseDir

        except OSError:
            sys.stderr.write('ERROR: Unable to create base directory '
                             'for settings file, settings will not be saved.\n')
            return None

    # View adjustment functionality ===========================================

    def _storeAndReturnViewState(self):
        lastView = self._lastViewContext
        self._lastViewContext = self._stageView.copyViewState()
        return lastView

    def _frameSelection(self):
        if self._stageView:
            # Save all the pertinent attribute values (for _toggleFramedView)
            self._storeAndReturnViewState() # ignore return val - we're stomping it
            self._stageView.setSelectedPrims(self._prunedCurrentPrims, self._currentFrame,
                                     True, True) # compute bbox on frame selection

    def _toggleFramedView(self):
        if self._stageView:
            self._stageView.restoreViewState(self._storeAndReturnViewState())

    def _resetSettings(self):
        """Reloads the UI and Sets up the initial settings for the
        _stageView object created in _reloadVaryingUI"""

        self._ui.redrawOnScrub.setChecked(self._redrawOnScrub)
        self._ui.actionShow_Inactive_Prims.setChecked(self._showInactivePrims)
        self._ui.actionShow_All_Master_Prims.setChecked(self._showAllMasterPrims)
        self._ui.actionShow_Undefined_Prims.setChecked(self._showUndefinedPrims)
        self._ui.actionShow_Abstract_Prims.setChecked(self._showAbstractPrims)
        self._ui.actionRollover_Prim_Info.setChecked(self._rolloverPrimInfo)

        # Seems like a good time to clear the texture registry
        Glf.TextureRegistry.Reset()

        # RELOAD fixed and varying UI
        self._reloadFixedUI()
        self._reloadVaryingUI()

        self._ui.showAABBox.setChecked(self._showAABBox)

        self._ui.showOBBox.setChecked(self._showOBBox)

        self._ui.showBBoxPlayback.setChecked(self._showBBoxPlayback)

        self._ui.showBBoxes.setChecked(self._showBBoxes)

        self._ui.actionDisplay_Guide.setChecked(self._displayGuide)

        self._ui.actionDisplay_Proxy.setChecked(self._displayProxy)

        self._ui.actionDisplay_Render.setChecked(self._displayRender)

        if self._stageView:
            # Called after displayGuide/displayProxy/displayRender are updated.
            self._stageView.updateBboxPurposes()

        self._ui.actionDisplay_Camera_Oracles.setChecked(self._displayCameraOracles)

        self._ui.actionDisplay_PrimId.setChecked(self._displayPrimId)

        self._ui.actionEnable_Hardware_Shading.setChecked(self._enableHardwareShading)

        self._ui.actionCull_Backfaces.setChecked(self._cullBackfaces)

        self._ui.actionCameraMask_Full.setChecked(self._cameraMaskMode == CameraMaskModes.FULL)
        self._ui.actionCameraMask_Partial.setChecked(self._cameraMaskMode == CameraMaskModes.PARTIAL)
        self._ui.actionCameraMask_None.setChecked(self._cameraMaskMode == CameraMaskModes.NONE)
        self._ui.actionCameraMask_Outline.setChecked(self._showMask_Outline)

        self._ui.actionCameraReticles_Inside.setChecked(self._showReticles_Inside)
        self._ui.actionCameraReticles_Outside.setChecked(self._showReticles_Outside)

        self._ui.actionHUD.setChecked(self._showHUD)
        self._showHUD_Info = False
        self._ui.actionHUD_Info.setChecked(self._showHUD_Info)
        self._ui.actionHUD_Complexity.setChecked(
            self._showHUD_Complexity)
        self._ui.actionHUD_Performance.setChecked(
            self._showHUD_Performance)
        self._ui.actionHUD_GPUstats.setChecked(
            self._showHUD_GPUstats)

        if self._stageView:
            self._stageView.update()

        self._clearColor = self._clearColorsDict[self._clearColorText]
        self._highlightColor = self._highlightColorsDict[self._highlightColorName]
        self._drawSelHighlights = (self._selHighlightMode != SelectionHighlightModes.NEVER)

        # lighting is not activated until a shaded mode is selected
        self._ui.menuLights.setEnabled(self._renderMode in ShadedRenderModes)

        self._ui.actionFreeCam._prim = None
        self._ui.actionFreeCam.triggered.connect(
            lambda : self._cameraSelectionChanged(None))
        if self._stageView:
            self._stageView.signalSwitchedToFreeCam.connect(
                lambda : self._cameraSelectionChanged(None))

        self._refreshCameraListAndMenu(preserveCurrCamera = False)

    def _updateForStageChanges(self):
        """Assuming there have been authoring changes to the already-loaded
        stage, make the minimal updates to the UI required to maintain a
        consistent state.  This may still be over-zealous until we know
        what actually changed, but we should be able to preserve camera and
        playback positions (unless viewing through a stage camera that no
        longer exists"""

        self._clearCaches(preserveCamera=True)

        # Update the UIs (it gets all of them) and StageView on a timer
        self.UpdatePrimViewContents()

    def _cacheViewerModeEscapeSizes(self, pos=None, index=None):
        topHeight, bottomHeight = self._ui.topBottomSplitter.sizes()
        primViewWidth, stageViewWidth = self._ui.primStageSplitter.sizes()
        if bottomHeight > 0 or primViewWidth > 0:
            self._viewerModeEscapeSizes = topHeight, bottomHeight, primViewWidth, stageViewWidth
        else:
            self._viewerModeEscapeSizes = None

    def _toggleViewerMode(self):
        topHeight, bottomHeight = self._ui.topBottomSplitter.sizes()
        primViewWidth, stageViewWidth = self._ui.primStageSplitter.sizes()
        if bottomHeight > 0 or primViewWidth > 0:
            topHeight += bottomHeight
            bottomHeight = 0
            stageViewWidth += primViewWidth
            primViewWidth = 0
        else:
            if self._viewerModeEscapeSizes is not None:
                topHeight, bottomHeight, primViewWidth, stageViewWidth = self._viewerModeEscapeSizes
            else:
                bottomHeight = UIDefaults.BOTTOM_HEIGHT
                topHeight = UIDefaults.TOP_HEIGHT
                primViewWidth = UIDefaults.PRIM_VIEW_WIDTH
                stageViewWidth = UIDefaults.STAGE_VIEW_WIDTH
        self._ui.topBottomSplitter.setSizes([topHeight, bottomHeight])
        self._ui.primStageSplitter.setSizes([primViewWidth, stageViewWidth])

    def _resetView(self,selectPrim = None):
        """ Reverts the GL frame to the initial camera view,
        and clears selection (sets to pseudoRoot), UNLESS 'selectPrim' is
        not None, in which case we'll select and frame it."""
        self._ui.primView.clearSelection()
        pRoot = self._rootDataModel.stage.GetPseudoRoot()
        if selectPrim is None:
            # if we had a command-line specified selection, re-frame it
            selectPrim = self._initialSelectPrim or pRoot

        item = self._getItemAtPath(selectPrim.GetPath())

        # Our response to selection-change includes redrawing.  We do NOT
        # want that to happen here, since we are subsequently going to
        # change the camera framing (and redraw, again), which can cause
        # flickering.  So make sure we don't redraw!
        self._allowViewUpdates = False
        self._ui.primView.setCurrentItem(item)
        self._allowViewUpdates = True

        if self._stageView:
            if (selectPrim and selectPrim != pRoot) or not self._startingPrimCamera:
                # _frameSelection translates the camera from wherever it happens
                # to be at the time.  If we had a starting selection AND a
                # primCam, then before framing, switch back to the prim camera
                if selectPrim == self._initialSelectPrim and self._startingPrimCamera:
                    self._stageView.setCameraPrim(self._startingPrimCamera)
                self._frameSelection()
            else:
                self._stageView.setCameraPrim(self._startingPrimCamera)
                self._stageView.setSelectedPrims(self._prunedCurrentPrims,
                                                 self._currentFrame)

    def _changeRenderMode(self, mode):
        self._renderMode = str(mode.text())
        self._ui.menuLights.setEnabled(self._renderMode in ShadedRenderModes)
        if self._stageView:
            self._stageView.update()

    def _changePickMode(self, mode):
        self._pickMode = str(mode.text())

    def _changeSelHighlightMode(self, mode):
        self._selHighlightMode = str(mode.text())
        self._drawSelHighlights = (self._selHighlightMode != SelectionHighlightModes.NEVER)
        if self._stageView:
            self._stageView.update()

    def _changeHighlightColor(self, color):
        self._highlightColorName = str(color.text())
        self._highlightColor = self._highlightColorsDict[self._highlightColorName]
        if self._stageView:
            self._stageView.update()

    def _changeInterpolationType(self, interpolationType):
        for t in Usd.InterpolationType.allValues:
            if t.displayName == str(interpolationType.text()):
                self._rootDataModel.stage.SetInterpolationType(t)
                self._resetSettings()
                break

    def _ambientOnlyClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._ambientLightOnly = checked

            # If all three lights are disabled, re-enable them all.
            if (not self._keyLightEnabled and not self._fillLightEnabled and
                    not self._backLightEnabled):
                self._keyLightEnabled = True
                self._fillLightEnabled = True
                self._backLightEnabled = True

            self._updateLights()
            self._stageView.update()

    def _onKeyLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._keyLightEnabled = checked
            self._updateLights()
            self._stageView.update()

    def _onFillLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._fillLightEnabled = checked
            self._updateLights()
            self._stageView.update()

    def _onBackLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._backLightEnabled = checked
            self._updateLights()
            self._stageView.update()

    def _updateLights(self):
        """Called whenever any lights settings are modified."""

        # Update the UI and view.
        self._ui.actionAmbient_Only.setChecked(self._ambientLightOnly)
        self._ui.threePointLights.setEnabled(not self._ambientLightOnly)
        self._ui.actionKey.setChecked(self._keyLightEnabled)
        self._ui.actionFill.setChecked(self._fillLightEnabled)
        self._ui.actionBack.setChecked(self._backLightEnabled)

    def _changeBgColor(self, mode):
        self._clearColorText = str(mode.text())
        self._clearColor = self._clearColorsDict[self._clearColorText]
        if self._stageView:
            self._stageView.update()

    def _toggleShowBBoxPlayback(self, state):
        """Called when the menu item for showing BBoxes
        during playback is activated or deactivated."""
        self._showBBoxPlayback = state

    def _setUseExtentsHint(self, state):
        self._refreshBBoxCache(state)

        self._updateAttributeView()

        #recompute and display bbox
        self._refreshBBox()

    def _toggleShowBBoxes(self, state):
        """Called when the menu item for showing BBoxes
        is activated."""
        self._showBBoxes = state
        #recompute and display bbox
        self._refreshBBox()

    def _toggleShowAABBox(self, state):
        """Called when Axis-Aligned bounding boxes
        are activated/deactivated via menu item"""
        self._showAABBox = state
        # recompute and display bbox
        self._refreshBBox()

    def _toggleShowOBBox(self, state):
        """Called when Oriented bounding boxes
        are activated/deactivated via menu item"""
        self._showOBBox = state
        # recompute and display bbox
        self._refreshBBox()

    def _refreshBBox(self):
        """Recompute and hide/show Bounding Box."""
        if self._stageView:
            self._stageView.setSelectedPrims(self._currentPrims,
                                             self._currentFrame,
                                             forceComputeBBox=True)

    def _toggleDisplayGuide(self, checked):
        self._displayGuide = checked
        self._updateAttributeView()
        if self._stageView:
            self._stageView.updateBboxPurposes()
            self._stageView.setSelectedPrims(self._prunedCurrentPrims, self._currentFrame)
            self._stageView.update()

    def _toggleDisplayProxy(self, checked):
        self._displayProxy = checked
        self._updateAttributeView()
        if self._stageView:
            self._stageView.updateBboxPurposes()
            self._stageView.setSelectedPrims(self._prunedCurrentPrims, self._currentFrame)
            self._stageView.update()

    def _toggleDisplayRender(self, checked):
        self._displayRender = checked
        self._updateAttributeView()
        if self._stageView:
            self._stageView.updateBboxPurposes()
            self._stageView.setSelectedPrims(self._prunedCurrentPrims, self._currentFrame)
            self._stageView.update()

    def _toggleDisplayCameraOracles(self, checked):
        self._displayCameraOracles = checked
        if self._stageView:
            self._stageView.update()

    def _toggleDisplayPrimId(self, checked):
        self._displayPrimId = checked
        if self._stageView:
            self._stageView.update()

    def _toggleEnableHardwareShading(self, checked):
        self._enableHardwareShading = checked
        if self._stageView:
            self._stageView.update()

    def _toggleCullBackfaces(self, checked):
        self._cullBackfaces = checked
        if self._stageView:
            self._stageView.update()

    def _showInterpreter(self):
        from pythonExpressionPrompt import Myconsole

        if self._interpreter is None:
            self._interpreter = QtWidgets.QDialog(self._mainWindow)
            self._interpreter.setObjectName("Interpreter")
            self._console = Myconsole(self._interpreter)
            self._interpreter.setFocusProxy(self._console) # this is important!
            lay = QtWidgets.QVBoxLayout()
            lay.addWidget(self._console)
            self._interpreter.setLayout(lay)

        # dock the interpreter window next to the main usdview window
        self._interpreter.move(self._mainWindow.x() + self._mainWindow.frameGeometry().width(),
                               self._mainWindow.y())
        self._interpreter.resize(600, self._mainWindow.size().height()/2)

        self._updateInterpreter()
        self._interpreter.show()
        self._interpreter.activateWindow()
        self._interpreter.setFocus()

    def _updateInterpreter(self):
        from pythonExpressionPrompt import Myconsole

        if self._console is None:
            return

        self._console.reloadConsole(self)

    # Screen capture functionality ===========================================

    def GrabWindowShot(self):
        '''Returns a QImage of the full usdview window '''
        # generate an image of the window. Due to how Qt's rendering
        # works, this will not pick up the GL Widget(_stageView)'s
        # contents, and we'll need to compose it separately.
        windowShot = QtGui.QImage(self._mainWindow.size(),
                                  QtGui.QImage.Format_ARGB32_Premultiplied)
        painter = QtGui.QPainter(windowShot)
        self._mainWindow.render(painter, QtCore.QPoint())

        if self._stageView:
            # overlay the QGLWidget on and return the composed image
            # we offset by a single point here because of Qt.Pos funkyness
            offset = QtCore.QPoint(0,1)
            pos = self._stageView.mapTo(self._mainWindow, self._stageView.pos()) - offset
            painter.drawImage(pos, self.GrabViewportShot())

        return windowShot

    def GrabViewportShot(self):
        '''Returns a QImage of the current stage view in usdview.'''
        if self._stageView:
            return self._stageView.grabFrameBuffer()
        else:
            return None

    # File handling functionality =============================================

    def _cleanAndClose(self):

        self._settings2.save()

        # If the current path widget is focused when closing usdview, it can
        # trigger an "editingFinished()" signal, which will look for a prim in
        # the scene (which is already deleted). This prevents that.

        # XXX:
        # This method is reentrant and calling disconnect twice on a signal
        # causes an exception to be thrown.
        try:
            self._ui.currentPathWidget.editingFinished.disconnect(
                self._currentPathChanged)
        except RuntimeError:
            pass

        # Shut down some timers and our eventFilter
        self._primViewUpdateTimer.stop()
        self._primViewResetTimer.stop()
        QtWidgets.QApplication.instance().removeEventFilter(self._filterObj)
        
        # If the timer is currently active, stop it from being invoked while
        # the USD stage is being torn down.
        if self._timer.isActive():
            self._timer.stop()

        # Close the stage.
        self._closeStage()

        # Tear down the UI window.
        with Timer() as t:
            self._mainWindow.close()
        if self._printTiming:
            t.PrintTime('tear down the UI')

    def _openFile(self):
        (filename, _) = QtWidgets.QFileDialog.getOpenFileName(self._mainWindow, "Select file",".")
        if len(filename) > 0:

            self._parserData.usdFile = str(filename)
            self._reopenStage()

            self._mainWindow.setWindowTitle(filename)

    def _saveOverridesAs(self):
        recommendedFilename = self._parserData.usdFile.rsplit('.', 1)[0]
        recommendedFilename += '_overrides.usd'
        (saveName, _) = QtWidgets.QFileDialog.getSaveFileName(self._mainWindow,
                                                     "Save file (*.usd)",
                                                     "./" + recommendedFilename,
                                                     'Usd Files (*.usd)')
        if len(saveName) <= 0:
            return

        if (saveName.rsplit('.')[-1] != 'usd'):
            saveName += '.usd'

        if self._rootDataModel.stage:
            # In the future, we may allow usdview to be brought up with no file,
            # in which case it would create an in-memory root layer, to which
            # all edits will be targeted.  In order to future proof
            # this, first fetch the root layer, and if it is anonymous, just
            # export it to the given filename. If it isn't anonmyous (i.e., it
            # is a regular usd file on disk), export the session layer and add
            # the stage root file as a sublayer.
            rootLayer = self._rootDataModel.stage.GetRootLayer()
            if not rootLayer.anonymous:
                self._rootDataModel.stage.GetSessionLayer().Export(saveName, 'Created by UsdView')
                targetLayer = Sdf.Layer.FindOrOpen(saveName)
                UsdUtils.CopyLayerMetadata(rootLayer, targetLayer,
                                           skipSublayers=True)

                # We don't ever store self.realStartTimeCode or
                # self.realEndTimeCode in a layer, so we need to author them
                # here explicitly.
                if self.realStartTimeCode:
                    targetLayer.startTimeCode = self.realStartTimeCode
                if self.realEndTimeCode:
                    targetLayer.endTimeCode = self.realEndTimeCode

                targetLayer.subLayerPaths.append(
                    self._rootDataModel.stage.GetRootLayer().realPath)
                targetLayer.RemoveInertSceneDescription()
                targetLayer.Save()
            else:
                self._rootDataModel.stage.GetRootLayer().Export(saveName, 'Created by UsdView')

    def _reopenStage(self):
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

        try:
            # Clear out any Usd objects that may become invalid. We will pick
            # these back up in _refreshVars(), called below.
            self._currentPrims = []
            self._currentProp = None
            self._currentSpec = None
            self._currentLayer = None

            # Close the current stage so that we don't keep it in memory
            # while trying to open another stage.
            self._closeStage()
            stage = self._openStage(
                self._parserData.usdFile, self._parserData.populationMask)
            # We need this for layers which were cached in memory but changed on
            # disk. The additional Reload call should be cheap when nothing
            # actually changed.
            stage.Reload()

            self._rootDataModel.stage = stage

            self._resetSettings()
            self._resetView()
            self._refreshVars()

            self._stepSizeChanged()
            self._stepSizeChanged()
        except Exception as err:
            self.statusMessage('Error occurred reopening Stage: %s' % err)
            traceback.print_exc()
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()

        self.statusMessage('Stage Reopened')

    def _reloadStage(self):
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

        try:
            self._rootDataModel.stage.Reload()
            # Seems like a good time to clear the texture registry
            Glf.TextureRegistry.Reset()
            # reset timeline, and playback settings from stage metadata
            self._reloadFixedUI(resetStageDataOnly=True)
            self._updateForStageChanges()
        except Exception as err:
            self.statusMessage('Error occurred rereading all layers for Stage: %s' % err)
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()

        self.statusMessage('All Layers Reloaded.')

    def _cameraSelectionChanged(self, camera):
        # Because the camera menu can be torn off, we need
        # to update its check-state whenever the selection changes
        cameraPath = None
        if camera:
            cameraPath = camera.GetPath()
        for action in self._ui.menuCamera.actions():
            action.setChecked(action.data() == cameraPath)
        if self._stageView:
            self._stageView.setCameraPrim(camera)
            self._stageView.updateGL()

    def _refreshCameraListAndMenu(self, preserveCurrCamera):
        self._allSceneCameras = Utils._GetAllPrimsOfType(
            self._rootDataModel.stage, Tf.Type.Find(UsdGeom.Camera))
        currCamera = self._startingPrimCamera
        if self._stageView:
            currCamera = self._stageView.getCameraPrim()
            self._stageView.allSceneCameras = self._allSceneCameras
            # if the stageView is holding an expired camera, clear it first
            # and force search for a new one
            if currCamera != None and not (currCamera and currCamera.IsActive()):
                currCamera = None
                self._stageView.setCameraPrim(None)
                preserveCurrCamera = False

        if not preserveCurrCamera:
            cameraWasSet = False
            def setCamera(camera):
                self._startingPrimCamera = currCamera = camera
                if self._stageView:
                    self._stageView.setCameraPrim(camera)
                cameraWasSet = True

            if self._startingPrimCameraPath:
                prim = self._rootDataModel.stage.GetPrimAtPath(
                    self._startingPrimCameraPath)
                if not prim.IsValid():
                    msg = sys.stderr
                    print >> msg, "WARNING: Camera path %r did not exist in " \
                                  "stage" % (str(self._startingPrimCameraPath),)
                    self._startingPrimCameraPath = None
                elif not prim.IsA(UsdGeom.Camera):
                    msg = sys.stderr
                    print >> msg, "WARNING: Camera path %r was not a " \
                                  "UsdGeom.Camera" % \
                                  (str(self._startingPrimCameraPath),)
                    self._startingPrimCameraPath = None
                else:
                    setCamera(prim)

            if not cameraWasSet and self._startingPrimCameraName:
                for camera in self._allSceneCameras:
                    if camera.GetName() == self._startingPrimCameraName:
                        setCamera(camera)
                        break

        # Now that we have the current camera and all cameras, build the menu
        self._ui.menuCamera.clear()
        if len(self._allSceneCameras) == 0:
            self._ui.menuCamera.setEnabled(False)
        else:
            self._ui.menuCamera.setEnabled(True)
            currCameraPath = None
            if currCamera:
                currCameraPath = currCamera.GetPath()
            for camera in self._allSceneCameras:
                action = self._ui.menuCamera.addAction(camera.GetName())
                action.setData(camera.GetPath())
                action.setToolTip(str(camera.GetPath()))
                action.setCheckable(True)

                action.triggered.connect(
                    lambda camera = camera: self._cameraSelectionChanged(camera))
                action.setChecked(action.data() == currCameraPath)

    # ===================================================================
    # ==================== Attribute Inspector ==========================
    def _populateAttributeInspector(self, currentItem = None, previtem = None):
        # We define data 'roles' in the property viewer to distinguish between things
        # like attributes and attributes with connections, relationships and relationships
        # with targets etc etc.
        self._currentProp = self._getSelectedObject(currentItem)

        # In the case of connections and targets, we keep their parent as the selected property
        if currentItem:
            role = currentItem.data(PropertyViewIndex.TYPE, QtCore.Qt.ItemDataRole.WhatsThisRole)
            if role == PropertyViewDataRoles.CONNECTION or role == PropertyViewDataRoles.TARGET:
                parent = currentItem.parent()
                self._currentProp = self._getSelectedObject(parent)

        if isinstance(self._currentProp, Usd.Prim):
            self._currentProp = None
        if self._console:
            self._console.reloadConsole(self)

        if currentItem is not None:
            itemName = str(currentItem.text(PropertyViewIndex.NAME))

            # inform the value editor that we selected a new attribute
            self._ui.attributeValueEditor.populate(itemName, self._currentPrims[0])
        else:
            self._ui.attributeValueEditor.clear()

    def _onCompositionSelectionChanged(self, curr=None, prev=None):
        self._currentSpec = getattr(curr, 'spec', None)
        self._currentLayer = getattr(curr, 'layer', None)
        if self._console:
            self._console.reloadConsole(self)

    def _updateAttributeInspector(self, index=None, obj=None):
        # index must be the first parameter since this method is used as
        # attributeInspector tab widget's currentChanged(int) signal callback
        if index is None:
            index = self._ui.attributeInspector.currentIndex()

        if obj is None:
            obj = self._getSelectedObject()

        if index == PropertyIndex.METADATA:
            self._updateMetadataView(obj)
        elif index == PropertyIndex.LAYERSTACK:
            self._updateLayerStackView(obj)
        elif index == PropertyIndex.COMPOSITION:
            self._updateCompositionView(obj)

    def _refreshAttributeValue(self):
        self._ui.attributeValueEditor.refresh()

    def _propertyViewContextMenu(self, point):
        item = self._ui.propertyView.itemAt(point)
        self.contextMenu = AttributeViewContextMenu(self._mainWindow, item, self)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _layerStackContextMenu(self, point):
        item = self._ui.layerStackView.itemAt(point)
        self.contextMenu = LayerStackContextMenu(self._mainWindow, item)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _compositionTreeContextMenu(self, point):
        item = self._ui.compositionTreeWidget.itemAt(point)
        self.contextMenu = LayerStackContextMenu(self._mainWindow, item)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    # Headers & Columns =================================================
    def _propertyViewHeaderContextMenu(self, point):
        self.contextMenu = HeaderContextMenu(self._ui.propertyView)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _primViewHeaderContextMenu(self, point):
        self.contextMenu = HeaderContextMenu(self._ui.primView)
        self.contextMenu.exec_(QtGui.QCursor.pos())


    # Widget management =================================================

    def _changePrimViewDepth(self, action):
        """Signal handler for view-depth menu items
        """
        actionTxt = str(action.text())
        # recover the depth factor from the action's name
        depth = int(actionTxt[actionTxt.find(" ")+1])
        self._expandToDepth(depth)

    def _expandToDepth(self, depth, suppressTiming=False):
        """Expands treeview prims to the given depth
        """
        with Timer() as t, BusyContext():
            # Populate items down to depth.  Qt will expand items at depth
            # depth-1 so we need to have items at depth.  We know something
            # changed if any items were added to _itemsToPush.
            n = len(self._itemsToPush)
            self._populateItem(self._rootDataModel.stage.GetPseudoRoot(),
                maxDepth=depth)
            changed = (n != len(self._itemsToPush))

            # Expand the tree to depth.
            self._ui.primView.expandToDepth(depth-1)
            if changed:
                # Resize column.
                self._ui.primView.resizeColumnToContents(0)

                # Start pushing prim data to the UI during idle cycles.
                # Qt doesn't need the data unless the item is actually
                # visible (or affects what's visible) but to avoid
                # jerky scrolling when that data is pulled during the
                # scroll, we can do it ahead of time.  But don't do it
                # if we're currently playing to maximize playback
                # performance.
                if not self._playing:
                    self._primViewUpdateTimer.start()

        if self._printTiming and not suppressTiming:
            t.PrintTime("expand Prim browser to depth %d" % depth)

    def _primViewExpanded(self, index):
        """Signal handler for expanded(index), facilitates lazy tree population
        """
        self._populateChildren(self._ui.primView.itemFromIndex(index))
        self._ui.primView.resizeColumnToContents(0)

    def _toggleShowInactivePrims(self, checked):
        self._showInactivePrims = checked
        self._resetPrimView()

    def _toggleShowMasterPrims(self, checked):
        self._showAllMasterPrims = checked
        self._resetPrimView()

    def _toggleShowUndefinedPrims(self, checked):
        self._showUndefinedPrims = checked
        self._resetPrimView()

    def _toggleShowAbstractPrims(self, checked):
        self._showAbstractPrims = checked
        self._resetPrimView()

    def _toggleRolloverPrimInfo(self, checked):
        self._rolloverPrimInfo = checked
        if self._stageView:
            self._stageView.rolloverPicking = self._rolloverPrimInfo

    def _tallyPrimStats(self, prim):
        def _GetType(prim):
            typeString = prim.GetTypeName()
            return HUDEntries.NOTYPE if not typeString else typeString

        childTypeDict = {}
        primCount = 0

        for child in Usd.PrimRange(prim):
            typeString = _GetType(child)
            # skip pseudoroot
            if typeString is HUDEntries.NOTYPE and not prim.GetParent():
                continue
            primCount += 1
            childTypeDict[typeString] = 1 + childTypeDict.get(typeString, 0)

        return (primCount, childTypeDict)

    def _populateChildren(self, item, depth=0, maxDepth=1, childrenToAdd=None):
        """Populates the children of the given item in the prim viewer.
           If childrenToAdd is given its a list of prims to add as
           children."""
        if depth < maxDepth and item.prim.IsActive():
            if item.needsChildrenPopulated() or childrenToAdd:
                # Populate all the children.
                if not childrenToAdd:
                    childrenToAdd = self._getFilteredChildren(item.prim)
                item.addChildren([self._populateItem(child, depth+1, maxDepth)
                                                    for child in childrenToAdd])
            elif depth + 1 < maxDepth:
                # The children already exist but we're recursing deeper.
                for i in xrange(item.childCount()):
                    self._populateChildren(item.child(i), depth+1, maxDepth)

    def _populateItem(self, prim, depth=0, maxDepth=0):
        """Populates a prim viewer item."""
        item = self._primToItemMap.get(prim)
        if not item:
            # Create a new item.  If we want its children we obviously
            # have to create those too.
            children = self._getFilteredChildren(prim)
            item = PrimViewItem(prim, self, len(children) != 0)
            self._primToItemMap[prim] = item
            self._populateChildren(item, depth, maxDepth, children)
            # Push the item after the children so ancestors are processed
            # before descendants.
            self._itemsToPush.append(item)
        else:
            # Item already exists.  Its children may or may not exist.
            # Either way, we need to have them to get grandchildren.
            self._populateChildren(item, depth, maxDepth)
        return item

    def _primShouldBeShown(self, prim):
        if not prim:
            return False

        return ((prim.IsActive() or self._showInactivePrims) and
                (prim.IsDefined() or self._showUndefinedPrims) and
                (not prim.IsAbstract() or self._showAbstractPrims) and
                (not prim.IsInMaster() or self._showAllMasterPrims))

    def _populateRoots(self):
        invisibleRootItem = self._ui.primView.invisibleRootItem()
        rootPrim = self._rootDataModel.stage.GetPseudoRoot()
        rootItem = self._populateItem(rootPrim)
        self._populateChildren(rootItem)

        if self._showAllMasterPrims:
            self._populateChildren(rootItem,
                childrenToAdd=self._rootDataModel.stage.GetMasters())

        # Add all descendents all at once.
        invisibleRootItem.addChild(rootItem)

    def _getFilteredChildren(self, prim):
        return prim.GetFilteredChildren(self._displayPredicate)

    def _computeDisplayPredicate(self):
        # Take current browser filtering into account when discovering
        # prims while traversing

        self._displayPredicate = None

        if not self._showInactivePrims:
            self._displayPredicate = Usd.PrimIsActive \
                if self._displayPredicate is None \
                else self._displayPredicate & Usd.PrimIsActive
        if not self._showUndefinedPrims:
            self._displayPredicate = Usd.PrimIsDefined \
                if self._displayPredicate is None \
                else self._displayPredicate & Usd.PrimIsDefined
        if not self._showAbstractPrims:
            self._displayPredicate = ~Usd.PrimIsAbstract \
                if self._displayPredicate is None \
                else self._displayPredicate & ~Usd.PrimIsAbstract
        if self._displayPredicate is None:
            self._displayPredicate = Usd._PrimFlagsPredicate.Tautology()

        # Unless user experience indicates otherwise, we think we always
        # want to show instance proxies
        self._displayPredicate = Usd.TraverseInstanceProxies(self._displayPredicate)


    def _getItemAtPath(self, path, ensureExpanded=False):
        # If the prim hasn't been expanded yet, drill down into it.
        # Note the explicit str(path) in the following expr is necessary
        # because path may be a QString.
        path = path if isinstance(path, Sdf.Path) else Sdf.Path(str(path))
        parent = self._rootDataModel.stage.GetPrimAtPath(path)
        if not parent:
            raise RuntimeError("Prim not found at path in stage: %s" % str(path))
        pseudoRoot = self._rootDataModel.stage.GetPseudoRoot()
        if parent not in self._primToItemMap:
            # find the first loaded parent
            childList = []

            while parent != pseudoRoot \
                        and not parent in self._primToItemMap:
                childList.append(parent)
                parent = parent.GetParent()

            # go one step further, since the first item found could be hidden
            # under a norgie and we would want to populate its siblings as well
            if parent != pseudoRoot:
                childList.append(parent)

            # now populate down to the child
            for parent in reversed(childList):
                item = self._primToItemMap[parent]
                self._populateChildren(item)
                if ensureExpanded:
                    item.setExpanded(True)

        # finally, return the requested item, which now must be in the map
        return self._primToItemMap[self._rootDataModel.stage.GetPrimAtPath(path)]

    def resetSelectionToPseudoroot(self):
        self.selectPrimByPath("/", UsdImagingGL.GL.ALL_INSTANCES, "replace")

    def selectPrimByPath(self, path, instanceIndex, updateMode,
                         applyPickMode=False):
        """Modifies selection by a stage prim based on a prim path,
        which can be empty.
          path - Sdf.Path to select
          instanceIndex - PointInstancer protoIndices index to select, if
                          'applyPickMode' is True.
          updateMode - one of "add", "replace", or "toggle", determines
                       how path should modify current selection.
          applyPickMode - consult the controller's "Pick Mode" to see if we
                          should apply model or instance selection modes.  If
                          False (the default), we will select the path given.
          If path is empty and updateMode is "replace", we reset the entire
          selection to the pseudoRoot.

          Returns newly (un)selected item
        """
        if not path or path == Sdf.Path.emptyPath:
            # For now, only continue if we're replacing
            if updateMode != "replace":
                return None
            path = self._rootDataModel.stage.GetPseudoRoot().GetPath()

        # If model picking on, find model and select instead, IFF we are
        # requested to apply picking modes
        if applyPickMode and self._pickMode == PickModes.MODELS:
            prim = self._rootDataModel.stage.GetPrimAtPath(str(path))
            model = prim if prim.IsModel() else GetEnclosingModelPrim(prim)
            if model:
                path = model.GetPath()

        # If not in instances picking mode, select all instances.
        if not (applyPickMode and self._pickMode == PickModes.INSTANCES):
            if self._stageView:
                self._stageView.clearInstanceSelection()
            instanceIndex = UsdImagingGL.GL.ALL_INSTANCES

        item = self._getItemAtPath(path, ensureExpanded=True)

        if updateMode == "replace":
            if self._stageView:
                self._stageView.clearInstanceSelection()
                self._stageView.setInstanceSelection(path, instanceIndex, True)
            self._ui.primView.setCurrentItem(item)
        elif updateMode == "add":
            if self._stageView:
                self._stageView.setInstanceSelection(path, instanceIndex, True)
            item.setSelected(True)
        else:   # "toggle"
            if instanceIndex != UsdImagingGL.GL.ALL_INSTANCES:
                if self._stageView:
                    self._stageView.setInstanceSelection(path, instanceIndex,
                        not self._stageView.getInstanceSelection(path, instanceIndex))
                    # if no instances selected, unselect item
                    if len(self._stageView.getSelectedInstanceIndices(path)) == 0:
                        item.setSelected(False)
                    else:
                        item.setSelected(True)
            else:
                if self._stageView:
                    self._stageView.clearInstanceSelection()
                item.setSelected(not item.isSelected())
            # if nothing selected, select root.
            if len(self._ui.primView.selectedItems()) == 0:
                item = self._getItemAtPath(
                    self._rootDataModel.stage.GetPseudoRoot().GetPath())
                item.setSelected(True)

        if instanceIndex != UsdImagingGL.GL.ALL_INSTANCES:
            self._itemSelectionChanged()

        return item


    def _getCommonPrims(self, pathsList):
        commonPrefix = os.path.commonprefix(pathsList)
        ### To prevent /Canopies/TwigA and /Canopies/TwigB
        ### from registering /Canopies/Twig as prefix
        return commonPrefix.rsplit('/', 1)[0]

    def _getAttributePrim(self):
        return self._rootDataModel.stage.GetPrimAtPath(
            self._currentPrims[0].GetPath())

    def _currentPathChanged(self):
        """Called when the currentPathWidget text is changed"""
        newPaths = self._ui.currentPathWidget.text()
        pathList = re.split(", ?", newPaths)
        pathList = filter(lambda path: len(path) != 0, pathList)

        try:
            self.jumpToTargetPaths(pathList)
        except PrimNotFoundException as ex:
            # jumpToTargetPaths couldn't find one of the prims
            sys.stderr.write("ERROR: %s\n" % ex.message)
            self._itemSelectionChanged()
            return

    def _setSelectionFromPrimList(self, primsToSelect):
        """Replaces current selection with the prims in 'primsToSelect'.
        Each member is first tested to make sure the prim is valid, and
        passes the current display filters.  Prims that do not pass are
        silently ignored"""
        # We are making many mutations to the PrimView's selection state.
        # We only want to update once in response, so temporarily disable
        # signals from the TreeWidget and manually sync selection after
        with AppController.UpdateBlocker(self):
            self._ui.primView.clearSelection()
            first = True
            for prim in primsToSelect:
                if self._primShouldBeShown(prim):
                    instanceIndex = UsdImagingGL.GL.ALL_INSTANCES
                    item = self.selectPrimByPath(prim.GetPath(), instanceIndex,
                                                 "replace" if first else "add")
                    first = False
                    # selectPrimByPath expands all of item's parents,
                    # but that doesn't seem to work if you have manually closed
                    # one of its ancestor's noorgies.  This will ensure all
                    # selected items are visible.
                    self._ui.primView.scrollToItem(item)
        # Now resync _currentPrims et al to the new PrimView
        # selection state
        self._itemSelectionChanged()


    class UpdateBlocker:

        def __init__(self, appModel):
            self._appModel = appModel

        def __enter__(self):
            self._appModel._updateBlock += 1

        def __exit__(self, *args):
            self._appModel._updateBlock -= 1


    def _itemSelectionChanged(self):
        if self._updateBlock > 0:
            return

        # grab a list of all the items selected
        selectedItems = self._ui.primView.selectedItems()
        if len(selectedItems) <= 0:
            return

        # get prims, but do not include prims whose parents are selected too.
        prunedPaths = self._getPathsFromItems(selectedItems, True)
        self._prunedCurrentPrims = [
            self._rootDataModel.stage.GetPrimAtPath(pth) for pth in prunedPaths]
        # get all prims selected
        paths = self._getPathsFromItems(selectedItems, False)
        self._currentPrims = [
            self._rootDataModel.stage.GetPrimAtPath(pth) for pth in paths]

        self._ui.currentPathWidget.setText(', '.join([str(p) for p in paths]))

        if self._stageView and self._allowViewUpdates:
            # update the entire upper HUD with fresh information
            # this includes geom counts (slow)
            self._updateHUDPrimStats()
            self._updateHUDGeomCounts()
            # recompute bbox on prim change
            self._stageView.setSelectedPrims(self._prunedCurrentPrims, self._currentFrame,
                                          resetCam=False, forceComputeBBox=True)

        # Clear out any property searches when the selected prim changes
        # We can't hold onto the resulting Qt Widgets, as they are ephemeral.
        self._attrSearchResults = deque([])

        self._updateAttributeInspector(obj=self._getSelectedPrim())
        self._updateAttributeView()
        self._refreshAttributeValue()
        self._updateInterpreter()

    def _itemClicked(self, item, col):
        # onClick() returns True if the click caused a state change (currently
        # this will only be a change to visibility).
        if item.onClick(col):
            self.editComplete('Updated prim visibility')
            with Timer() as t:
                PrimViewItem.propagateVis(item)
            if self._printTiming:
                t.PrintTime("update vis column")
        self._updateAttributeInspector(obj=self._getSelectedPrim())

    def _propertyViewItemClicked(self, item, col):
        role = item.data(PropertyViewIndex.TYPE, QtCore.Qt.ItemDataRole.WhatsThisRole)
        if role == PropertyViewDataRoles.CONNECTION or role == PropertyViewDataRoles.TARGET:
            self._ui.propertyView.setCurrentItem(item.parent())
            item.setSelected(True)

        currIndex = self._ui.attributeInspector.currentIndex()

        # The PropertyIndex.VALUE tab is updated through a separate callback.
        if currIndex != PropertyIndex.VALUE:
            self._updateAttributeInspector(index=currIndex,
                                           obj=self._getSelectedObject())

    def _getPathsFromItems(self, items, prune = False):
        # this function returns a list of paths given a list of items if
        # prune=True, it excludes certain paths if a parent path is already
        # there this avoids double-rendering if both a prim and its parent
        # are selected.
        #
        # Don't include the pseudoroot, though, if it's still selected, because
        # leaving it in the pruned list will cause everything else to get
        # pruned away!
        allPaths = [itm.prim.GetPath() for itm in items]
        if not prune:
            return allPaths
        if len(allPaths) > 1:
            allPaths = [p for p in allPaths if p != Sdf.Path.absoluteRootPath]
        return Sdf.Path.RemoveDescendentPaths(allPaths)

    def _primViewContextMenu(self, point):
        item = self._ui.primView.itemAt(point)
        self._showPrimContextMenu(item)

    def _showPrimContextMenu(self, item):
        self.contextMenu = PrimContextMenu(self._mainWindow, item, self)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def setFrame(self, frameIndex, forceUpdate=False):
        frameAtStart = self._currentFrame
        self._playbackFrameIndex = frameIndex

        frame = self._timeSamples[int(frameIndex)]
        if self._currentFrame != frame:
            minDist = 1.0e30
            closestFrame = None
            for t in self._timeSamples:
                dist = abs(t - frame)
                if dist < minDist:
                    minDist = dist
                    closestFrame = t

            if closestFrame is None:
                return

            self._currentFrame = closestFrame

        # XXX Why do we *always* update the widget, but only
        # conditionally update?  All this function should do, after
        # computing a new frame number, is emit a signal that the
        # time has changed.  Future work.
        self._ui.frameField.setText(str(round(self._currentFrame,2)))

        if self._currentFrame != frameAtStart or forceUpdate:
            # do not update HUD/BBOX if scrubbing or playing
            updateUI = forceUpdate or not (self._playing or
                                          self._ui.frameSlider.isSliderDown())
            self._updateOnFrameChange(updateUI)

    def _updateOnFrameChange(self, refreshUI = True):
        """Called when the frame changed, updates the renderer and such"""
        # set the xformCache and bboxCache's time to the new time
        self._xformCache.SetTime(self._currentFrame)
        self._bboxCache.SetTime(self._currentFrame)

        if refreshUI: # slow stuff that we do only when not playing
            # topology might have changed, recalculate
            self._updateHUDGeomCounts()
            self._updateAttributeView()
            self._refreshAttributeValue()
            self._sliderTimer.stop()

            # value sources of an attribute can change upon frame change
            # due to value clips, so we must update the layer stack.
            self._updateLayerStackView()

            # refresh the visibility column
            self._resetPrimViewVis(selItemsOnly=False, authoredVisHasChanged=False)

        if self._stageView:
            # this is the part that renders
            if self._playing:
                self._stageView.updateForPlayback(self._currentFrame,
                                 self._selHighlightMode == SelectionHighlightModes.ALWAYS)
            else:
                self._stageView.setSelectedPrims(self._currentPrims, self._currentFrame)

    def saveFrame(self, fileName):
        if self._stageView:
            pm =  QtGui.QPixmap.grabWindow(self._stageView.winId())
            pm.save(fileName, 'TIFF')

    def _getAttributeDict(self):
        attributeDict = OrderedDict()

        # leave attribute viewer empty if multiple prims selected
        if len(self._currentPrims) != 1:
            return attributeDict

        prim = self._currentPrims[0]

        composed, rels = _GetCustomAttributes(prim, self._bboxCache, self._xformCache)

        attrs = prim.GetAttributes() + rels
        def cmpFunc(attrA, attrB):
            aName = attrA.GetName()
            bName = attrB.GetName()
            return cmp(aName.lower(), bName.lower())

        attrs.sort(cmp=cmpFunc)

        # Add the special composed attributes usdview generates
        # at the top of our property list.
        for attr in composed:
            attributeDict[attr.GetName()] = attr

        for attr in attrs:
            attributeDict[attr.GetName()] = attr

        return attributeDict

    def _updateAttributeViewInternal(self):
        frame = self._currentFrame
        treeWidget = self._ui.propertyView

        previousSelection = treeWidget.selectedItems()
        prevSelectedAttributeNames = set()
        for i in previousSelection:
            prevSelectedAttributeNames.add(str(i.text(PropertyViewIndex.NAME)))

        # get a dictionary of prim attribs/members and store it in self._attributeDict
        self._attributeDict = self._getAttributeDict()
        treeWidget.clear()
        self._populateAttributeInspector()

        currRow = 0
        for key, attribute in self._attributeDict.iteritems():
            targets = None

            if (isinstance(attribute, BoundingBoxAttribute) or
                isinstance(attribute, LocalToWorldXformAttribute)):
                typeContent = PropertyViewIcons.COMPOSED()
                typeRole = PropertyViewDataRoles.COMPOSED
            elif type(attribute) == Usd.Attribute:
                if attribute.HasAuthoredConnections():
                    typeContent = PropertyViewIcons.ATTRIBUTE_WITH_CONNECTIONS()
                    typeRole = PropertyViewDataRoles.ATTRIBUTE_WITH_CONNNECTIONS
                    targets = attribute.GetConnections()
                else:
                    typeContent = PropertyViewIcons.ATTRIBUTE()
                    typeRole = PropertyViewDataRoles.ATTRIBUTE
            else:
                # Otherwise we have a RelationshipAttribute
                targets = attribute._relationship.GetTargets()

                if targets:
                    typeContent = PropertyViewIcons.RELATIONSHIP_WITH_TARGETS()
                    typeRole = PropertyViewDataRoles.RELATIONSHIP_WITH_TARGETS
                else:
                    typeContent = PropertyViewIcons.RELATIONSHIP()
                    typeRole = PropertyViewDataRoles.RELATIONSHIP

            attrText = GetShortString(attribute, frame) or ""
            treeWidget.addTopLevelItem(
                QtWidgets.QTreeWidgetItem(["", str(key), attrText]))
            treeWidget.topLevelItem(currRow).setIcon(PropertyViewIndex.TYPE, typeContent)
            treeWidget.topLevelItem(currRow).setData(PropertyViewIndex.TYPE,
                                                     QtCore.Qt.ItemDataRole.WhatsThisRole,
                                                     typeRole)

            currItem = treeWidget.topLevelItem(currRow)

            # Need reference to original value for pretty-print on double-click
            if (key in prevSelectedAttributeNames):
                currItem.setSelected(True)
                treeWidget.setCurrentItem(currItem)

            valTextFont = GetAttributeTextFont(attribute, frame)
            if valTextFont:
                currItem.setFont(PropertyViewIndex.VALUE, valTextFont)
                currItem.setFont(PropertyViewIndex.NAME, valTextFont)
            else:
                currItem.setFont(PropertyViewIndex.NAME, UIFonts.BOLD)

            fgColor = GetAttributeColor(attribute, frame)
            currItem.setForeground(PropertyViewIndex.NAME, fgColor)
            currItem.setForeground(PropertyViewIndex.VALUE, fgColor)

            if targets:
                childRow = 0
                for t in targets:
                    valTextFont = GetAttributeTextFont(attribute, frame) or UIFonts.BOLD
                    # USD does not provide or infer values for relationship or
                    # connection targets, so we don't display them here.
                    currItem.addChild(QtWidgets.QTreeWidgetItem(["", str(t), ""]))
                    currItem.setFont(PropertyViewIndex.VALUE, valTextFont)
                    child = currItem.child(childRow)

                    if typeRole == PropertyViewDataRoles.RELATIONSHIP_WITH_TARGETS:
                        child.setIcon(PropertyViewIndex.TYPE, PropertyViewIcons.TARGET())
                        child.setData(PropertyViewIndex.TYPE,
                                      QtCore.Qt.ItemDataRole.WhatsThisRole,
                                      PropertyViewDataRoles.TARGET)
                    else:
                        child.setIcon(PropertyViewIndex.TYPE, PropertyViewIcons.CONNECTION())
                        child.setData(PropertyViewIndex.TYPE,
                                      QtCore.Qt.ItemDataRole.WhatsThisRole,
                                      PropertyViewDataRoles.CONNECTION)

                    childRow += 1

            currRow += 1

    def _updateAttributeView(self):
        """ Sets the contents of the attribute value viewer """
        cursorOverride = not self._timer.isActive()
        if cursorOverride:
            QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
        try:
            self._updateAttributeViewInternal()
        except Exception as err:
            print "Problem encountered updating attribute view: %s" % err
        finally:
            if cursorOverride:
                QtWidgets.QApplication.restoreOverrideCursor()

    def _getSelectedObject(self, selectedAttribute=None):
        if selectedAttribute is None:
            attrs = self._ui.propertyView.selectedItems()
            if len(attrs) > 0:
                selectedAttribute = attrs[0]

        if selectedAttribute:
            attrName = str(selectedAttribute.text(PropertyViewIndex.NAME))

            if PropTreeWidgetTypeIsRel(selectedAttribute):
                obj = self._currentPrims[0].GetRelationship(attrName)
            else:
                obj = self._currentPrims[0].GetAttribute(attrName)

            return obj

        return self._getSelectedPrim()

    def _getSelectedPrim(self):
        return self._currentPrims[0] if self._currentPrims else None

    def _findIndentPos(self, s):
        for index, char in enumerate(s):
            if char != ' ':
                return index

        return len(s) - 1

    def _maxToolTipWidth(self):
        return 90

    def _maxToolTipHeight(self):
        return 32

    def _trimWidth(self, s, isList=False):
        # We special-case the display offset because list
        # items will have </li> tags embedded in them.
        offset = 10 if isList else 5

        if len(s) >= self._maxToolTipWidth():
            # For strings, well do special ellipsis behavior
            # which displays the last 5 chars with an ellipsis
            # in between. For other values, we simply display a
            # trailing ellipsis to indicate more data.
            if s[0] == '\'' and s[-1] == '\'':
                return (s[:self._maxToolTipWidth() - offset]
                        + '...'
                        + s[len(s) - offset:])
            else:
                return s[:self._maxToolTipWidth()] + '...'
        return s

    def _limitToolTipSize(self, s, isList=False):
        ttStr = ''

        lines = s.split('<br>')
        for index, line in enumerate(lines):
            if index+1 > self._maxToolTipHeight():
                break
            ttStr += self._trimWidth(line, isList)
            if not isList and index != len(lines)-1:
                ttStr += '<br>'

        if (len(lines) > self._maxToolTipHeight()):
            ellipsis = ' '*self._findIndentPos(line) + '...'
            if isList:
                ellipsis = '<li>' + ellipsis + '</li>'
            else:
                ellipsis += '<br>'

            ttStr += ellipsis
            ttStr += self._trimWidth(lines[len(lines)-2], isList)

        return ttStr

    def _addRichTextIndicators(self, s):
        # - We'll need to use html-style spaces to ensure they are respected
        # in the toolTip which uses richtext formatting.
        # - We wrap the tooltip as a paragraph to ensure &nbsp; 's are
        # respected by Qt's rendering engine.
        return '<p>' + s.replace(' ', '&nbsp;') + '</p>'

    def _limitValueDisplaySize(self, s):
        maxValueChars = 300
        return s[:maxValueChars]

    def _cleanStr(self, s, repl):
        # Remove redundant char seqs and strip newlines.
        replaced = str(s).replace('\n', repl)
        filtered = [u for (u, _) in groupby(replaced.split())]
        return ' '.join(filtered)

    def _formatMetadataValueView(self, val):
        from pprint import pformat, pprint

        valStr = self._cleanStr(val, ' ')
        ttStr  = ''
        isList = False

        # For iterable things, like VtArrays and lists, we want to print
        # a nice numbered list.
        if isinstance(val, list) or getattr(val, "_isVtArray", False):
            isList = True

            # We manually supply the index for our list elements
            # because Qt's richtext processor starts the <ol> numbering at 1.
            for index, value in enumerate(val):
                last = len(val) - 1
                trimmed = self._cleanStr(value, ' ')
                ttStr += ("<li>" + str(index) + ":  " + trimmed + "</li><br>")

        elif isinstance(val, dict):
            # We stringify all dict elements so they display more nicely.
            # For example, by default, the pprint operation would print a
            # Vt Array as Vt.Array(N, (E1, ....). By running it through
            # str(..). we'd get [(E1, E2), ....] which is more useful to
            # the end user trying to examine their data.
            for k, v in val.items():
                val[k] = str(v)

            # We'll need to strip the quotes generated by the str' operation above
            stripQuotes = lambda s: s.replace('\'', '').replace('\"', "")

            valStr = stripQuotes(self._cleanStr(val, ' '))

            formattedDict = pformat(val)
            formattedDictLines = formattedDict.split('\n')
            for index, line in enumerate(formattedDictLines):
                ttStr += (stripQuotes(line)
                    + ('' if index == len(formattedDictLines) - 1 else '<br>'))
        else:
            ttStr = self._cleanStr(val, '<br>')

        valStr = self._limitValueDisplaySize(valStr)
        ttStr = self._addRichTextIndicators(
                    self._limitToolTipSize(ttStr, isList))
        return valStr, ttStr

    def _updateMetadataView(self, obj=None):
        """ Sets the contents of the metadata viewer"""

        # XXX: this method gets called multiple times on selection, it
        # would be nice to clean that up and ensure we only update as needed.

        tableWidget = self._ui.metadataView
        self._attributeDict = self._getAttributeDict()

        # Setup table widget
        tableWidget.clearContents()
        tableWidget.setRowCount(0)

        if obj is None:
            obj = self._getSelectedObject()

        if not obj:
            return

        m = obj.GetAllMetadata()

        # We have to explicitly add in metadata related to composition arcs
        # and value clips here, since GetAllMetadata prunes them out.
        #
        # XXX: Would be nice to have some official facility to query
        # this.
        compKeys = [# composition related metadata
                    "references", "inheritPaths", "specializes",
                    "payload", "subLayers",

                    # non-template clip metadata
                    "clipAssetPaths", "clipTimes", "clipManifestAssetPath",
                    "clipActive", "clipPrimPath",

                    # template clip metadata
                    "clipTemplateAssetPath",
                    "clipTemplateStartTime", "clipTemplateEndTime",
                    "clipTemplateStride"]


        for k in compKeys:
            v = obj.GetMetadata(k)
            if not v is None:
                m[k] = v

        m["[object type]"] = "Attribute" if type(obj) is Usd.Attribute \
                       else "Prim" if type(obj) is Usd.Prim \
                       else "Relationship" if type(obj) is Usd.Relationship \
                       else "Unknown"
        m["[path]"] = str(obj.GetPath())

        variantSets = {}
        if (isinstance(obj, Usd.Prim)):
            variantSetNames = obj.GetVariantSets().GetNames()
            for variantSetName in variantSetNames:
                variantSet = obj.GetVariantSet(variantSetName)
                variantNames = variantSet.GetVariantNames()
                variantSelection = variantSet.GetVariantSelection()
                combo = VariantComboBox(None, obj, variantSetName, self._mainWindow)
                # First index is always empty to indicate no (or invalid)
                # variant selection.
                combo.addItem('')
                for variantName in variantNames:
                    combo.addItem(variantName)
                indexToSelect = combo.findText(variantSelection)
                combo.setCurrentIndex(indexToSelect)
                variantSets[variantSetName] = combo

        tableWidget.setRowCount(len(m) + len(variantSets))

        for i,key in enumerate(sorted(m.keys())):
            attrName = QtWidgets.QTableWidgetItem(str(key))
            tableWidget.setItem(i, 0, attrName)

            # Get metadata value
            if key == "customData":
                val = obj.GetCustomData()
            else:
                val = m[key]

            valStr, ttStr = self._formatMetadataValueView(val)
            attrVal = QtWidgets.QTableWidgetItem(valStr)
            attrVal.setToolTip(ttStr)

            tableWidget.setItem(i, 1, attrVal)

        rowIndex = len(m)
        for variantSetName, combo in variantSets.iteritems():
            attrName = QtWidgets.QTableWidgetItem(str(variantSetName+ ' variant'))
            tableWidget.setItem(rowIndex, 0, attrName)
            tableWidget.setCellWidget(rowIndex, 1, combo)
            combo.currentIndexChanged.connect(
                lambda i, combo=combo: combo.updateVariantSelection(
                    i, self._updateForStageChanges, self._printTiming))
            rowIndex += 1

        tableWidget.resizeColumnToContents(0)

    def _updateCompositionView(self, obj=None):
        """ Sets the contents of the composition tree view"""
        treeWidget = self._ui.compositionTreeWidget
        treeWidget.clear()

        # Update current spec & current layer, and push those updates
        # to the python console
        self._onCompositionSelectionChanged()

        # If no prim or attribute selected, nothing to show.
        if obj is None:
            obj = self._getSelectedObject()
        if not obj:
            return

        # For brevity, we display only the basename of layer paths.
        def LabelForLayer(l):
            return os.path.basename(l.realPath) if l.realPath else '~session~'

        # Create treeview items for all sublayers in the layer tree.
        def WalkSublayers(parent, node, layerTree, sublayer=False):
            layer = layerTree.layer
            spec = layer.GetObjectAtPath(node.path)
            item = QtWidgets.QTreeWidgetItem(
                parent,
                [
                    LabelForLayer(layer),
                    'sublayer' if sublayer else node.arcType.displayName,
                    str(node.GetPathAtIntroduction()),
                    'yes' if bool(spec) else 'no'
                ] )

            # attributes for selection:
            item.layer = layer
            item.spec = spec
            item.identifier = layer.identifier

            # attributes for LayerStackContextMenu:
            if layer.realPath:
                item.layerPath = layer.realPath
            if spec:
                item.path = node.path

            item.setExpanded(True)
            item.setToolTip(0, layer.identifier)
            if not spec:
                for i in range(item.columnCount()):
                    item.setForeground(i, UIPropertyValueSourceColors.NONE)
            for subtree in layerTree.childTrees:
                WalkSublayers(item, node, subtree, True)
            return item

        # Create treeview items for all nodes in the composition index.
        def WalkNodes(parent, node):
            nodeItem = WalkSublayers(parent, node, node.layerStack.layerTree)
            for child in node.children:
                WalkNodes(nodeItem, child)

        path = obj.GetPath().GetAbsoluteRootOrPrimPath()
        prim = self._rootDataModel.stage.GetPrimAtPath(path)
        if not prim:
            return

        # Populate the treeview with items from the prim index.
        index = prim.GetPrimIndex()
        if index.IsValid():
            WalkNodes(treeWidget, index.rootNode)


    def _updateLayerStackView(self, obj=None):
        """ Sets the contents of the layer stack viewer"""

        tableWidget = self._ui.layerStackView

        # Setup table widget
        tableWidget.clearContents()
        tableWidget.setRowCount(0)

        if obj is None:
            obj = self._getSelectedObject()

        if not obj:
            return

        path = obj.GetPath()

        # The pseudoroot is different enough from prims and properties that
        # it makes more sense to process it separately
        if path == Sdf.Path.absoluteRootPath:
            layers = GetRootLayerStackInfo(
                self._rootDataModel.stage.GetRootLayer())
            tableWidget.setColumnCount(2)
            tableWidget.horizontalHeaderItem(1).setText('Layer Offset')

            tableWidget.setRowCount(len(layers))

            for i, layer in enumerate(layers):
                layerItem = QtWidgets.QTableWidgetItem(layer.GetHierarchicalDisplayString())
                layerItem.layerPath = layer.layer.realPath
                layerItem.identifier = layer.layer.identifier
                toolTip = "<b>identifier:</b> @%s@ <br> <b>resolved path:</b> %s" % \
                    (layer.layer.identifier, layerItem.layerPath)
                toolTip = self._limitToolTipSize(toolTip)
                layerItem.setToolTip(toolTip)
                tableWidget.setItem(i, 0, layerItem)

                offsetItem = QtWidgets.QTableWidgetItem(layer.GetOffsetString())
                offsetItem.layerPath = layer.layer.realPath
                offsetItem.identifier = layer.layer.identifier
                toolTip = self._limitToolTipSize(str(layer.offset))
                offsetItem.setToolTip(toolTip)
                tableWidget.setItem(i, 1, offsetItem)

            tableWidget.resizeColumnToContents(0)
        else:
            specs = []
            tableWidget.setColumnCount(3)
            header = tableWidget.horizontalHeader()
            header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
            header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
            header.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
            tableWidget.horizontalHeaderItem(1).setText('Path')

            if path.IsPropertyPath():
                prop = obj.GetPrim().GetProperty(path.name)
                frameTime = (self._currentFrame if self._currentFrame
                                                else Usd.TimeCode.Default())
                specs = prop.GetPropertyStack(frameTime)
                c3 = "Value" if (len(specs) == 0 or
                                 isinstance(specs[0], Sdf.AttributeSpec)) else "Target Paths"
                tableWidget.setHorizontalHeaderItem(2,
                                                    QtWidgets.QTableWidgetItem(c3))
            else:
                specs = obj.GetPrim().GetPrimStack()
                tableWidget.setHorizontalHeaderItem(2,
                    QtWidgets.QTableWidgetItem('Metadata'))

            tableWidget.setRowCount(len(specs))

            for i, spec in enumerate(specs):
                layerItem = QtWidgets.QTableWidgetItem(spec.layer.GetDisplayName())
                layerItem.setToolTip(self._limitToolTipSize(spec.layer.realPath))
                tableWidget.setItem(i, 0, layerItem)

                pathItem = QtWidgets.QTableWidgetItem(spec.path.pathString)
                pathItem.setToolTip(self._limitToolTipSize(spec.path.pathString))
                tableWidget.setItem(i, 1, pathItem)

                if path.IsPropertyPath():
                    valStr = GetShortString(spec, self._currentFrame)
                    ttStr = valStr
                    valueItem = QtWidgets.QTableWidgetItem(valStr)
                    sampleBased = (spec.HasInfo('timeSamples') and
                        spec.layer.GetNumTimeSamplesForPath(path) != -1)
                    valueItemColor = (UIPropertyValueSourceColors.TIME_SAMPLE if
                        sampleBased else UIPropertyValueSourceColors.DEFAULT)
                    valueItem.setForeground(valueItemColor)
                    valueItem.setToolTip(ttStr)

                else:
                    metadataKeys = spec.GetMetaDataInfoKeys()
                    metadataDict = {}
                    for mykey in metadataKeys:
                        if spec.HasInfo(mykey):
                            metadataDict[mykey] = spec.GetInfo(mykey)
                    valStr, ttStr = self._formatMetadataValueView(metadataDict)
                    valueItem = QtWidgets.QTableWidgetItem(valStr)
                    valueItem.setToolTip(ttStr)

                tableWidget.setItem(i, 2, valueItem)
                # Add the data the context menu needs
                for j in range(3):
                    item = tableWidget.item(i, j)
                    item.layerPath = spec.layer.realPath
                    item.path = spec.path.pathString
                    item.identifier = spec.layer.identifier

    def _isHUDVisible(self):
        """Checks if the upper HUD is visible by looking at the global HUD
        visibility menu as well as the 'Subtree Info' menu"""
        return self._showHUD and self._showHUD_Info

    def _updateCameraMaskMenu(self):
        if self._ui.actionCameraMask_Full.isChecked():
            self._cameraMaskMode = CameraMaskModes.FULL
        elif self._ui.actionCameraMask_Partial.isChecked():
            self._cameraMaskMode = CameraMaskModes.PARTIAL
        else:
            self._cameraMaskMode = CameraMaskModes.NONE
        self._showMask_Outline = self._ui.actionCameraMask_Outline.isChecked()

        if self._stageView:
            self._stageView.updateGL()

    def _pickCameraMaskColor(self):
        QtWidgets.QColorDialog.setCustomColor(0, 0xFF000000)
        QtWidgets.QColorDialog.setCustomColor(1, 0xFF808080)
        color = QtWidgets.QColorDialog.getColor()
        color = (
                color.redF(),
                color.greenF(),
                color.blueF(),
                color.alphaF()
        )
        self.cameraMaskColor = color
        if self._stageView:
            self._stageView.updateGL()

    def _updateCameraReticlesMenu(self):
        self._CameraReticlesMenuChanged()

    def _pickCameraReticlesColor(self):
        QtWidgets.QColorDialog.setCustomColor(0, 0xFF000000)
        QtWidgets.QColorDialog.setCustomColor(1, 0xFF0080FF)
        color = QtWidgets.QColorDialog.getColor()
        color = (
                color.redF(),
                color.greenF(),
                color.blueF(),
                color.alphaF()
        )
        self.cameraReticlesColor = color
        if self._stageView:
            self._stageView.updateGL()

    def _CameraReticlesMenuChanged(self):
        self._showReticles_Inside = self._ui.actionCameraReticles_Inside.isChecked()
        self._showReticles_Outside = self._ui.actionCameraReticles_Outside.isChecked()
        if self._stageView:
            self._stageView.updateGL()

    def _HUDMenuChangedInfoRefresh(self):
        """Called when a HUD menu item that requires info refresh has changed.
        Updates the upper HUD with both prim info and geom counts.
        """
        self._showHUD = self._ui.actionHUD.isChecked()
        self._showHUD_Info = self._ui.actionHUD_Info.isChecked()

        if self._isHUDVisible():
            self._updateHUDPrimStats()
            self._updateHUDGeomCounts()

        if self._stageView:
            self._stageView.updateGL()

    def _HUDMenuChanged(self):
        """Called when a HUD menu item that does not require info refresh has changed."""
        self._showHUD_Complexity = self._ui.actionHUD_Complexity.isChecked()
        self._showHUD_Performance = self._ui.actionHUD_Performance.isChecked()
        self._showHUD_GPUstats = self._ui.actionHUD_GPUstats.isChecked()
        if self._stageView:
            self._stageView.updateGL()

    def _getHUDStatKeys(self):
        ''' returns the keys of the HUD with PRIM and NOTYPE and the top and
         CV, VERT, and FACE at the bottom.'''
        keys = [k for k in self._upperHUDInfo.keys() if k not in (
            HUDEntries.CV, HUDEntries.VERT, HUDEntries.FACE, HUDEntries.PRIM, HUDEntries.NOTYPE)]
        keys = [HUDEntries.PRIM, HUDEntries.NOTYPE] + keys + [HUDEntries.CV, HUDEntries.VERT, HUDEntries.FACE]
        return keys

    def _updateHUDPrimStats(self):
        """update the upper HUD with the proper prim information"""
        self._upperHUDInfo = dict()

        if self._isHUDVisible():
            currentPaths = [n.GetPath() for n in self._prunedCurrentPrims if n.IsActive()]

            for pth in currentPaths:
                count,types = self._tallyPrimStats(
                    self._rootDataModel.stage.GetPrimAtPath(pth))
                # no entry for Prim counts? initilize it
                if not self._upperHUDInfo.has_key(HUDEntries.PRIM):
                    self._upperHUDInfo[HUDEntries.PRIM] = 0
                self._upperHUDInfo[HUDEntries.PRIM] += count

                for type in types.iterkeys():
                    # no entry for this prim type? initilize it
                    if not self._upperHUDInfo.has_key(type):
                        self._upperHUDInfo[type] = 0
                    self._upperHUDInfo[type] += types[type]

            if self._stageView:
                self._stageView.upperHUDInfo = self._upperHUDInfo
                self._stageView.HUDStatKeys = self._getHUDStatKeys()

    def _updateHUDGeomCounts(self):
        """updates the upper HUD with the right geom counts
        calls _getGeomCounts() to get the info, which means it could be cached"""
        if not self._isHUDVisible():
            return

        # we get multiple geom dicts, if we have multiple prims selected
        geomDicts = [self._getGeomCounts(n, self._currentFrame)
                     for n in self._prunedCurrentPrims]

        for key in (HUDEntries.CV, HUDEntries.VERT, HUDEntries.FACE):
            self._upperHUDInfo[key] = 0
            for gDict in geomDicts:
                self._upperHUDInfo[key] += gDict[key]

        if self._stageView:
            self._stageView.upperHUDInfo = self._upperHUDInfo
            self._stageView.HUDStatKeys = self._getHUDStatKeys()

    def _clearGeomCountsForPrimPath(self, primPath):
        entriesToRemove = []
        # Clear all entries whose prim is either an ancestor or a descendant
        # of the given prim path.
        for (p, frame) in self._geomCounts:
            if (primPath.HasPrefix(p.GetPath()) or p.GetPath().HasPrefix(primPath)):
                entriesToRemove.append((p, frame))
        for entry in entriesToRemove:
            del self._geomCounts[entry]

    def _getGeomCounts( self, prim, frame ):
        """returns cached geom counts if available, or calls _calculateGeomCounts()"""
        if not self._geomCounts.has_key((prim,frame)):
            self._calculateGeomCounts( prim, frame )

        return self._geomCounts[(prim,frame)]

    def _accountForFlattening(self,shape):
        """Helper function for computing geomCounts"""
        if len(shape) == 1:
            return shape[0] / 3
        else:
            return shape[0]

    def _calculateGeomCounts(self, prim, frame):
        """Computes the number of CVs, Verts, and Faces for each prim and each
        frame in the stage (for use by the HUD)"""

        # This is expensive enough that we should give the user feedback
        # that something is happening...
        QtWidgets.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
        try:
            thisDict = {HUDEntries.CV: 0, HUDEntries.VERT: 0, HUDEntries.FACE: 0}

            if prim.IsA(UsdGeom.Curves):
                curves = UsdGeom.Curves(prim)
                vertexCounts = curves.GetCurveVertexCountsAttr().Get(frame)
                if vertexCounts is not None:
                    for count in vertexCounts:
                        thisDict[HUDEntries.CV] += count

            elif prim.IsA(UsdGeom.Mesh):
                mesh = UsdGeom.Mesh(prim)
                faceVertexCount = mesh.GetFaceVertexCountsAttr().Get(frame)
                faceVertexIndices = mesh.GetFaceVertexIndicesAttr().Get(frame)
                if faceVertexCount is not None and faceVertexIndices is not None:
                    uniqueVerts = set(faceVertexIndices)

                    thisDict[HUDEntries.VERT] += len(uniqueVerts)
                    thisDict[HUDEntries.FACE] += len(faceVertexCount)

            self._geomCounts[(prim,frame)] = thisDict

            for child in prim.GetChildren():
                childResult = self._getGeomCounts(child, frame)

                for key in (HUDEntries.CV, HUDEntries.VERT, HUDEntries.FACE):
                    self._geomCounts[(prim,frame)][key] += childResult[key]
        except Exception as err:
            print "Error encountered while computing prim subtree HUD info: %s" % err
        finally:
            QtWidgets.QApplication.restoreOverrideCursor()


    def _setupDebugMenu(self):
        def __helper(debugType, menu):
            return lambda: self._createTfDebugMenu(menu, '{0}_'.format(debugType))

        for debugType in DebugTypes:
            menu = self._ui.menuDebug.addMenu('{0} Flags'.format(debugType))
            menu.aboutToShow.connect(__helper(debugType, menu))

    def _createTfDebugMenu(self, menu, flagFilter):
        def __createTriggerLambda(flagToSet, value):
            return lambda: Tf.Debug.SetDebugSymbolsByName(flagToSet, value)

        flags = [flag for flag in Tf.Debug.GetDebugSymbolNames() if flag.startswith(flagFilter)]
        menu.clear()
        for flag in flags:
            action = menu.addAction(flag)
            isEnabled = Tf.Debug.IsDebugSymbolNameEnabled(flag)
            action.setCheckable(True)
            action.setChecked(isEnabled)
            action.setStatusTip(Tf.Debug.GetDebugSymbolDescription(flag))
            action.triggered[bool].connect(__createTriggerLambda(flag, not isEnabled))

    def _updateEditPrimMenu(self):
        """Make the Edit Prim menu items enabled or disabled depending on the
        selected prim."""
        
        # Use the descendent-pruned selection set to avoid redundant
        # traversal of the stage to answer isLoaded...
        anyLoadable, unused = GetPrimsLoadability(self._prunedCurrentPrims)
        removeEnabled = False
        anyImageable = False
        anyModels = False
        anyBoundMaterials = False
        anyActive = False
        anyInactive = False
        for prim in self._currentPrims:
            if prim.IsA(UsdGeom.Imageable):
                imageable = UsdGeom.Imageable(prim)
                anyImageable = anyImageable or bool(imageable)
                removeEnabled = removeEnabled or HasSessionVis(prim)
            anyModels = anyModels or GetEnclosingModelPrim(prim) is not None
            material, bound = GetClosestBoundMaterial(prim)
            anyBoundMaterials = anyBoundMaterials or material is not None
            if prim.IsActive():
                anyActive = True
            else:
                anyInactive = True

        self._ui.actionJump_to_Model_Root.setEnabled(anyModels)
        self._ui.actionJump_to_Bound_Material.setEnabled(anyBoundMaterials)

        self._ui.actionRemove_Session_Visibility.setEnabled(removeEnabled)
        self._ui.actionMake_Visible.setEnabled(anyImageable)
        self._ui.actionVis_Only.setEnabled(anyImageable)
        self._ui.actionMake_Invisible.setEnabled(anyImageable)
        self._ui.actionLoad.setEnabled(anyLoadable)
        self._ui.actionUnload.setEnabled(anyLoadable)
        self._ui.actionActivate.setEnabled(anyInactive)
        self._ui.actionDeactivate.setEnabled(anyActive)


    def getSelectedItems(self):
        return [self._primToItemMap[n] for n in self._currentPrims
                    if n in self._primToItemMap]

    def _getPrimFromPropString(self, p):
        return self._rootDataModel.stage.GetPrimAtPath(p.split('.')[0])

    def jumpToTargetPaths(self, paths):
        prims = []
        for path in paths:
            prim = self._rootDataModel.stage.GetPrimAtPath(
                Sdf.Path(str(path)).GetAbsoluteRootOrPrimPath())

            if not prim:
                raise PrimNotFoundException(path)
            prims.append(prim)

        self._setSelectionFromPrimList(prims)

        if len(paths) == 1:
            path = Sdf.Path(paths[0])

            # If there is no property component
            if not path.IsPropertyPath():
                return

            primName = path.GetPrimPath()
            propName = path.name

            lookup = self._ui.propertyView.findItems(
                propName,
                QtCore.Qt.MatchRegExp | QtCore.Qt.MatchRecursive,
                PropertyViewIndex.NAME)

            if not lookup:
                return

            item = lookup[0]
            item.setSelected(True)
            self._ui.propertyView.setCurrentItem(item)

    def jumpToEnclosingModelSelectedPrims(self):
        newSel = []
        added = set()
        # We don't expect this to take long, so no BusyContext
        for prim in self._currentPrims:
            model = GetEnclosingModelPrim(prim)
            prim = model or prim
            if not (prim in added):
                added.add(prim)
                newSel.append(prim)
        self._setSelectionFromPrimList(newSel)

    def jumpToBoundMaterialSelectedPrims(self):
        newSel = []
        added = set()
        # We don't expect this to take long, so no BusyContext
        for prim in self._currentPrims:
            material, bound = GetClosestBoundMaterial(prim)
            if not (material in added):
                added.add(material)
                newSel.append(material)
        self._setSelectionFromPrimList(newSel)

    def visSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.makeVisible()
            self.editComplete('Made selected prims visible')
            # makeVisible may cause aunt and uncle prims' authored vis
            # to change, so we need to fix up the whole shebang
            self._resetPrimViewVis(selItemsOnly=False)

    def visOnlySelectedPrims(self):
        with BusyContext():
            ResetSessionVisibility(self._rootDataModel.stage)
            InvisRootPrims(self._rootDataModel.stage)
            for item in self.getSelectedItems():
                item.makeVisible()
            self.editComplete('Made ONLY selected prims visible')
            # QTreeWidget does not honor setUpdatesEnabled, and updating
            # the Vis column for all widgets is pathologically slow.
            # It is sadly much much faster to regenerate the entire view
            self._resetPrimView()

    def invisSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.setVisible(False)
            self.editComplete('Made selected prims invisible')
            self._resetPrimViewVis()

    def removeVisSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.removeVisibility()
            self.editComplete("Removed selected prims' visibility opinions")
            self._resetPrimViewVis()

    def resetSessionVisibility(self):
        with BusyContext():
            ResetSessionVisibility(self._rootDataModel.stage)
            self.editComplete('Removed ALL session visibility opinions.')
            # QTreeWidget does not honor setUpdatesEnabled, and updating
            # the Vis column for all widgets is pathologically slow.
            # It is sadly much much faster to regenerate the entire view
            self._resetPrimView()

    def activateSelectedPrims(self):
        with BusyContext():
            primNames=[]
            for item in self.getSelectedItems():
                item.setActive(True)
                primNames.append(item.name)
            self.editComplete("Activated %s." % primNames)

    def deactivateSelectedPrims(self):
        with BusyContext():
            primNames=[]
            for item in self.getSelectedItems():
                item.setActive(False)
                primNames.append(item.name)
            self.editComplete("Deactivated %s." % primNames)

    def loadSelectedPrims(self):
        with BusyContext():
            primNames=[]
            for item in self.getSelectedItems():
                item.setLoaded(True)
                primNames.append(item.name)
            self.editComplete("Loaded %s." % primNames)

    def unloadSelectedPrims(self):
        with BusyContext():
            primNames=[]
            for item in self.getSelectedItems():
                item.setLoaded(False)
                primNames.append(item.name)
            self.editComplete("Unloaded %s." % primNames)

    def onCurrentFrameChanged(self, currentFrame):
        self._ui.frameField.setText(str(currentFrame))

    def onStageViewMouseDrag(self):
        return

    def onPrimSelected(self, path, instanceIndex, button, modifiers):
        if modifiers & QtCore.Qt.ShiftModifier:
            updateMode = "add"
        elif modifiers & QtCore.Qt.ControlModifier:
            updateMode = "toggle"
        else:
            updateMode = "replace"

        # Ignoring middle button until we have something
        # meaningfully different for it to do
        if button in [QtCore.Qt.LeftButton, QtCore.Qt.RightButton]:
            # Expected context-menu behavior is that even with no
            # modifiers, if we are activating on something already selected,
            # do not change the selection
            doContext = (button == QtCore.Qt.RightButton and path
                         and path != Sdf.Path.emptyPath)
            doSelection = True
            item = None
            if doContext:
                for selPrim in self._currentPrims:
                    selPath = selPrim.GetPath()
                    if (selPath != Sdf.Path.absoluteRootPath and
                        path.HasPrefix(selPath)):
                        doSelection = False
                        break
            if doSelection:
                item = self.selectPrimByPath(path, instanceIndex, updateMode,
                                             applyPickMode=True)
                if item and item.prim.GetPath() != Sdf.Path.absoluteRootPath:
                    # Scroll the prim view widget to show the newly selected
                    # item, unless it's the pseudoRoot, which represents "no
                    # selection"
                    self._ui.primView.scrollToItem(item)
            if doContext:
                # The context menu requires an item for validation.  Make sure
                # we have a valid one to give it.
                if not item:
                    item = self._getItemAtPath(path)
                self._showPrimContextMenu(item)

                # context menu steals mouse release event from the StageView.
                # We need to give it one so it can track its interaction
                # mode properly
                mrEvent = QtGui.QMouseEvent(QtCore.QEvent.MouseButtonRelease,
                                            QtGui.QCursor.pos(),
                                            QtCore.Qt.RightButton,
                                            QtCore.Qt.MouseButtons(QtCore.Qt.RightButton),
                                            QtCore.Qt.KeyboardModifiers())
                QtWidgets.QApplication.sendEvent(self._stageView, mrEvent)

    def onRollover(self, path, instanceIndex, modifiers):
        prim = self._rootDataModel.stage.GetPrimAtPath(path)
        if prim:
            headerStr = ""
            propertyStr = ""
            materialStr = ""
            aiStr = ""
            vsStr = ""
            model = GetEnclosingModelPrim(prim)

            def _MakeModelRelativePath(path, model,
                                       boldPrim=True, boldModel=False):
                makeRelative = model and path.HasPrefix(model.GetPath())
                if makeRelative:
                    path = path.MakeRelativePath(model.GetPath().GetParentPath())
                pathParts = str(path).split('/')
                if boldModel and makeRelative:
                    pathParts[0] = "<b>%s</b>" % pathParts[0]
                if boldPrim:
                    pathParts[-1] = "<b>%s</b>" % pathParts[-1]
                return '/'.join(pathParts)

            def _HTMLEscape(s):
                return s.replace('&', '&amp;'). \
                         replace('<', '&lt;'). \
                         replace('>', '&gt;')

            # First add in all model-related data, if present
            if model:
                groupPath = model.GetPath().GetParentPath()
                # Make the model name and prim name bold.
                primModelPath = _MakeModelRelativePath(prim.GetPath(),
                                                       model, True, True)
                headerStr = "%s<br><nobr><small>in group:</small> %s</nobr>" % \
                    (str(primModelPath),str(groupPath))

                # asset info, including computed creation date
                mAPI = Usd.ModelAPI(model)
                assetInfo = mAPI.GetAssetInfo()
                aiStr = "<hr><b>assetInfo</b> for %s:" % model.GetName()
                if assetInfo and len(assetInfo) > 0:
                    specs = model.GetPrimStack()
                    name, time, owner = GetAssetCreationTime(specs,
                                                   mAPI.GetAssetIdentifier())
                    for key, value in assetInfo.iteritems():
                        aiStr += "<br> -- <em>%s</em> : %s" % (key, _HTMLEscape(str(value)))
                    aiStr += "<br><em><small>%s created on %s by %s</small></em>" % \
                        (_HTMLEscape(name), time, _HTMLEscape(owner))
                else:
                    aiStr += "<br><small><em>No assetInfo!</em></small>"

                # variantSets are by no means required/expected, so if there
                # are none, don't bother to declare so.
                mVarSets = model.GetVariantSets()
                setNames = mVarSets.GetNames()
                if len(setNames) > 0:
                    vsStr = "<hr><b>variantSets</b> on %s:" % model.GetName()
                    for name in setNames:
                        sel = mVarSets.GetVariantSelection(name)
                        vsStr += "<br> -- <em>%s</em> = %s" % (name, sel)

            else:
                headerStr = _MakeModelRelativePath(path, None)

            # Property info: advise about rare visibility and purpose conditions
            img = UsdGeom.Imageable(prim)
            propertyStr = "<hr><b>Property Summary for %s '%s':</b>" % \
                (prim.GetTypeName(), prim.GetName())
            # Now cherry pick "important" attrs... could do more, here
            if img:
                if img.GetVisibilityAttr().ValueMightBeTimeVarying():
                    propertyStr += "<br> -- <em>visibility</em> varies over time"
                purpose = img.GetPurposeAttr().Get()
                inheritedPurpose = img.ComputePurpose()
                if inheritedPurpose != UsdGeom.Tokens.default_:
                    propertyStr += "<br> -- <em>purpose</em> is <b>%s</b>%s " %\
                        (inheritedPurpose, "" if purpose == inheritedPurpose \
                             else ", <small>(inherited)</small>")
            gprim = UsdGeom.Gprim(prim)
            if gprim:
                ds = gprim.GetDoubleSidedAttr().Get()
                orient = gprim.GetOrientationAttr().Get()
                propertyStr += "<br> -- <em>doubleSided</em> = %s" % \
                    ( "true" if ds else "false")
                propertyStr += "<br> -- <em>orientation</em> = %s" % orient
            ptBased = UsdGeom.PointBased(prim)
            if ptBased:
                # XXX WBN to not have to read points in to get array size
                # XXX2 Should try to determine varying topology
                points = ptBased.GetPointsAttr().Get(self._currentFrame)
                propertyStr += "<br> -- %d points" % len(points)
            mesh = UsdGeom.Mesh(prim)
            if mesh:
                propertyStr += "<br> -- <em>subdivisionScheme</em> = %s" %\
                    mesh.GetSubdivisionSchemeAttr().Get()
            pi = UsdGeom.PointInstancer(prim)
            if pi:
                indices = pi.GetProtoIndicesAttr().Get(self._currentFrame)
                propertyStr += "<br> -- <em>%d instances</em>" % len(indices)
                protos = pi.GetPrototypesRel().GetForwardedTargets()
                propertyStr += "<br> -- <em>%d unique prototypes</em>" % len(protos)
                if instanceIndex >= 0 and instanceIndex < len(indices):
                    protoIndex = indices[instanceIndex]
                    if protoIndex < len(protos):
                        currProtoPath = protos[protoIndex]
                        # If, as is common, proto is beneath the PI,
                        # strip the PI's prefix for display
                        if currProtoPath.HasPrefix(path):
                            currProtoPath = currProtoPath.MakeRelativePath(path)
                        propertyStr += "<br> -- <em>instance of prototype &lt;%s&gt;</em>" % str(currProtoPath)

            # Material info - this IS expected
            materialStr = "<hr><b>Material assignment:</b><br>"
            material, bound = GetClosestBoundMaterial(prim)
            if material:
                materialPath = material.GetPath()
                # if the material is in the same model, make path model-relative
                materialStr += _MakeModelRelativePath(materialPath, model)

                if bound != prim:
                    boundPath = _MakeModelRelativePath(bound.GetPath(),
                                                       model)
                    materialStr += "<br><small><em>Material binding inherited from ancestor:</em></small><br> %s" % str(boundPath)
            else:
                materialStr += "<small><em>No assigned Material!</em></small>"

            # Instance / master info, if this prim is a native instance, else
            # instance index/id if it's from a PointInstancer
            instanceStr = ""
            if prim.IsInstance():
                instanceStr = "<hr><b>Instancing:</b><br>"
                instanceStr += "<nobr><small><em>Instance of master:</em></small> %s</nobr>" % \
                    str(prim.GetMaster().GetPath())
            elif instanceIndex != -1:
                instanceStr = "<hr><b>Instance Index:</b> %d" % instanceIndex
                instanceId = GetInstanceIdForIndex(prim, instanceIndex,
                                                   self._currentFrame)
                if instanceId is not None:
                    instanceStr += "<br><b>Instance Id:</b> %d" % instanceId

            # Then put it all together
            tip = headerStr + propertyStr + materialStr + instanceStr + aiStr + vsStr

        else:
            tip = ""
        QtWidgets.QToolTip.showText(QtGui.QCursor.pos(), tip, self._stageView)

    def processNavKeyEvent(self, kpEvent):
        # This method is a standin for a hotkey processor... for now greatly
        # limited in scope, as we mostly use Qt's builtin hotkey dispatch.
        # Since we want navigation keys to be hover-context-sensitive, we
        # cannot use the native mechanism.
        key = kpEvent.key()
        if key == QtCore.Qt.Key_Right:
            self._advanceFrame()
            return True
        elif key == QtCore.Qt.Key_Left:
            self._retreatFrame()
            return True
        return False
        
