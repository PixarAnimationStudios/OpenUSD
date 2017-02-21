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
from PySide import QtGui, QtCore
import stageView
from stageView import StageView
from mainWindowUI import Ui_MainWindow
from nodeContextMenu import NodeContextMenu
from headerContextMenu import HeaderContextMenu
from layerStackContextMenu import LayerStackContextMenu
from attributeViewContextMenu import AttributeViewContextMenu
from customAttributes import _GetCustomAttributes
from nodeViewItem import NodeViewItem
from pxr import Usd, UsdGeom, UsdUtils, UsdImagingGL
from pxr import Glf
from pxr import Sdf
from pxr import Tf
from pxr import Plug

from ._usdviewq import Utils

from collections import deque
from collections import OrderedDict
from time import time, sleep
import re, sys, os

import prettyPrint
import watchWindow
import adjustClipping
import referenceEditor
from settings import Settings

from common import (FallbackTextColor, NoValueTextColor, TimeSampleTextColor,
                    ValueClipsTextColor, DefaultTextColor, HeaderColor, 
                    RedColor, BoldFont, ItalicFont, GetAttributeColor,
                    GetAttributeTextFont, HasArcsColor, InstanceColor,
                    NormalColor, MasterColor, Timer, 
                    BusyContext, DumpMallocTags)

# Upper HUD entries (declared in variables for abstraction)
PRIM = "Prims"
CV = "CVs"
VERT = "Verts"
FACE = "Faces"

# Lower HUD entries
PLAYBACK = "Playback"
RENDER = "Render"
GETBOUNDS = "BBox"

# Name for nodes that have no type
NOTYPE = "Typeless"

INDEX_VALUE, INDEX_METADATA, INDEX_LAYERSTACK, INDEX_COMPOSITION = range(4)

# Tf Debug entries to include in debug menu
TF_DEBUG_MENU_ENTRIES = ["HD", "HDX", "USD", "USDIMAGING", "USDVIEWQ"]

def isWritableUsdPath(path):
    if not os.access(path, os.W_OK):
        return (False, 'Path is not a writable file.')

    return (True, '')

def getBackupFile(path):
    # get a backup file name like myfile.0.usd
    number = 0

    extindex = path.rfind('.')
    if extindex == -1:
        exindex = len(path)

    while os.access(path[:extindex] + '.' + str(number) + path[extindex:], os.F_OK):
        number = number + 1

    return path[:extindex] + '.' + str(number) + path[extindex:]

def uniquify_tablewidgetitems(a):
    """ Eliminates duplicate list entries in a list
        of TableWidgetItems. It relies on the row property
        being available for comparison.
    """
    if (len(a) == 0):
        tmp = []
    else:
        tmp = [a[0]]
        # XXX: we need to compare row #s because
        # PySide doesn't allow equality comparison for QTableWidgetItems
        tmp_rows = set() 
        tmp_rows.add(a[0].row())

        for i in range(1,len(a)):
            if (a[i].row() not in tmp_rows):
                tmp.append(a[i])
                tmp_rows.add(a[i].row())
    return tmp

def drange(start, stop, step):
    """Like builtin range() but allows decimals and is a closed interval 
        that is, it's inclusive of stop"""
    r = start
    lst = []
    epsilon = 1e-3 * step
    while r <= stop+epsilon:
        lst.append(r)
        r += step
    return lst

def _settingsWarning(filePath):
    """Send a warning because the settings file should never fail to load
    """
    import traceback
    import sys
    msg = sys.stderr
    print >> msg, "------------------------------------------------------------"
    print >> msg, "WARNING: Unknown problem while trying to access settings:"
    print >> msg, "------------------------------------------------------------"
    print >> msg, "This message is being sent because the settings file (%s) " \
                  "could not be read" % filePath
    print >> msg, "--"
    traceback.print_exc(file=msg)
    print >> msg, "--"
    print >> msg, "Please file a bug if this warning persists"
    print >> msg, "Attempting to continue... "
    print >> msg, "------------------------------------------------------------"

class VariantComboBox(QtGui.QComboBox):
    def __init__(self, parent, prim, variantSetName, mainWindow):
        QtGui.QComboBox.__init__(self, parent)
        self.prim = prim
        self.variantSetName = variantSetName
        self.mainWindow = mainWindow

    def updateVariantSelection(self, index):
        variantSet = self.prim.GetVariantSet(self.variantSetName)
        currentVariantSelection = variantSet.GetVariantSelection()
        newVariantSelection = str(self.currentText())
        if currentVariantSelection != newVariantSelection:
            with Timer() as t:
                variantSet.SetVariantSelection(newVariantSelection)
                # We need to completely re-generate the prim tree because
                # changing a prim's variant set can change the namespace
                # hierarchy.
                self.mainWindow._updatePrimTreeForVariantSwitch()
            if self.mainWindow._printTiming:
                t.PrintTime("change variantSet %s to %s" % 
                            (variantSet.GetName(), newVariantSelection))

def _GetShortString(prop, frame):
    from customAttributes import CustomAttribute
    if isinstance(prop, (Usd.Attribute, CustomAttribute)):
        val = prop.Get(frame)
    elif isinstance(prop, Sdf.AttributeSpec):
        if frame == Usd.TimeCode.Default():
            val = prop.default
        else:
            numTimeSamples = -1
            if prop.HasInfo('timeSamples'):
                numTimeSamples = prop.layer.GetNumTimeSamplesForPath(prop.path)
            if numTimeSamples == -1:
                val = prop.default
            elif numTimeSamples == 1:
                return "1 time sample"
            else:
                return str(numTimeSamples) + " time samples"
    elif isinstance(prop, Sdf.RelationshipSpec):
        return str(prop.targetPathList)

    from scalarTypes import GetScalarTypeFromAttr
    scalarType, isArray = GetScalarTypeFromAttr(prop)
    result = ''
    if isArray and not isinstance(val, Sdf.ValueBlock):
        def arrayToStr(a):
            from itertools import chain
            elems = a if len(a) <= 6 else chain(a[:3], ['...'], a[-3:])
            return '[' + ', '.join(map(str, elems)) + ']'
        if val is not None and len(val):
            result = "%s[%d]: %s" % (scalarType, len(val), arrayToStr(val))
        else:
            result = "%s[]" % scalarType
    else:
        result = str(val)

    return result[:500]


class MainWindow(QtGui.QMainWindow):

    @classmethod
    def clearSettings(cls):
        settingsPath = cls._outputBaseDirectory()
        settingsPath = os.path.join(settingsPath, 'state')
        if not os.path.exists(settingsPath):
            print "INFO: ClearSettings requested, but there were no settings " \
                    "currently stored"
            return
        os.remove(settingsPath)
        print "INFO: Settings restored to default"

    def _configurePlugins(self):
        from plugContext import PlugContext
        plugCtx = PlugContext(self)
        try:
            from pixar import UsdviewPlug
            UsdviewPlug.ConfigureView(plugCtx)

        except ImportError:
            pass

    def __del__(self):
        # This is needed to free Qt items before exit; Qt hits failed GTK
        # assertions without it.
        self._nodeToItemMap.clear()

    def __init__(self, parent, parserData):
        QtGui.QMainWindow.__init__(self, parent)
        self._nodeToItemMap = {}
        self._itemsToPush = []
        self._currentNodes = []
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
        import os
        self._debug = os.getenv('USDVIEW_DEBUG', False)
        self._printTiming = parserData.timing or self._debug
        self._lastViewContext = {}
        self._statusFileName = 'state'
        self._deprecatedStatusFileName = '.usdviewrc'
        self._mallocTags  = parserData.mallocTagStats

        self._propertyLegendCollapsed = False
        self._nodeLegendCollapsed = False

        self._propertyLegendHeightOffset = 50
        self._nodeLegendHeightOffset = 100
        self._legendButtonSelectedStyle = ('background: rgb(189, 155, 84); '
                                           'color: rgb(227, 227, 227);')

       
        MainWindow._renderer = parserData.renderer
        if MainWindow._renderer == 'simple':
            os.environ['HD_ENABLED'] = '0'
 
        self.show()
        self._ui = Ui_MainWindow()
        self._ui.setupUi(self)
        self.installEventFilter(self)

        # read the stage here
        self._stage = self._openStage(self._parserData.usdFile,
                                      self._parserData.populationMask)
        if not self._stage:
            sys.exit(0)

        if not self._stage.GetPseudoRoot():
            print parserData.usdFile, 'has no prims; exiting.'
            sys.exit(0)

        self._initialSelectNode = self._stage.GetPrimAtPath(parserData.primPath)
        if not self._initialSelectNode:
            print 'Could not find prim at path <%s> to select. '\
                'Ignoring...' % parserData.primPath
            self._initialSelectNode = None

        self._timeSamples = None
        self._stageView = None
        self._startingPrimCamera = None

        self.setWindowTitle(parserData.usdFile)
        self._statusBar = QtGui.QStatusBar(self)
        self.setStatusBar(self._statusBar)

        settingsPathDir = self._outputBaseDirectory()
        settingsPath = os.path.join(settingsPathDir, self._statusFileName)
        deprecatedSettingsPath = \
            os.path.join(settingsPathDir, self._deprecatedStatusFileName)
        if (os.path.isfile(deprecatedSettingsPath) and
            not os.path.isfile(settingsPath)):
            warning = ('\nWARNING: The settings file at: '
                       + str(deprecatedSettingsPath) + ' is deprecated.\n'
                       + 'These settings are not being used, the new '
                       + 'settings file will be located at: '
                       + str(settingsPath) + '.\n')
            print warning

        self._settings = Settings(settingsPath)

        try:
            self._settings.load()
        except IOError:
            # try to force out a new settings file
            try:
                self._settings.save()
            except:
                _settingsWarning(settingsPath)

        except EOFError:
            # try to force out a new settings file
            try:
                self._settings.save()
            except:
                _settingsWarning(settingsPath)
        except:
            _settingsWarning(settingsPath)

        QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

        self._timer = QtCore.QTimer(self)
        # Timeout interval in ms. We set it to 0 so it runs as fast as
        # possible. In advanceFrameForPlayback we use the sleep() call
        # to slow down rendering to self.framesPerSecond fps.
        self._timer.setInterval(0)
        self._lastFrameTime = time()

        self._colorsDict = {'Black':(0.0,0.0,0.0,0.0),
                            'Grey (Dark)':(0.3,0.3,0.3,0.0),
                            'Grey (Light)':(0.7,0.7,0.7,0.0),
                            'White':(1.0,1.0,1.0,0.0),
                            'Yellow':(1.0,1.0,0.0,0.0),
                            'Cyan':(0.0,1.0,1.0,0.0)}

        # We need to store the trinary selHighlightMode state here,
        # because the stageView only deals in True/False (because it
        # cannot know anything about playback state).
        # We store the highlightColorName so that we can compare state during
        # initialization without inverting the name->value logic
        self._selHighlightMode = "Only when paused"
        self._highlightColorName = "Yellow"

        # Initialize the upper HUD info
        self._upperHUDInfo = dict()

        # declare dictionary keys for the fps info too
        self._fpsHUDKeys = (RENDER, PLAYBACK)

        # Initialize fps HUD with empty strings
        self._fpsHUDInfo = dict(zip(self._fpsHUDKeys,
                                    ["N/A", "N/A"]))
        self._startTime = self._endTime = time()
        
        # Create action groups
        self._ui.threePointLights = QtGui.QActionGroup(self)
        self._ui.colorGroup = QtGui.QActionGroup(self)
        
        self._nodeViewResetTimer = QtCore.QTimer(self)
        self._nodeViewResetTimer.setInterval(250)
        QtCore.QObject.connect(self._nodeViewResetTimer, 
                               QtCore.SIGNAL('timeout()'),
                               self._resetNodeView)

        # Idle timer to push off-screen data to the UI.
        self._nodeViewUpdateTimer = QtCore.QTimer(self)
        self._nodeViewUpdateTimer.setInterval(0)
        QtCore.QObject.connect(self._nodeViewUpdateTimer, 
                               QtCore.SIGNAL('timeout()'),
                               self._updateNodeView)

        # This creates the _stageView and restores state from setings file
        self._resetSettings()
        
        if self._stageView:
            self._stageView.complexity = parserData.complexity

        # This is for validating frame values input on the "Frame" line edit
        validator = QtGui.QDoubleValidator(self)
        self._ui.frameField.setValidator(validator)
        self._ui.rangeEnd.setValidator(validator)
        self._ui.rangeBegin.setValidator(validator)

        stepValidator = QtGui.QDoubleValidator(self)
        stepValidator.setRange(0.01, 1e7, 2)
        self._ui.stepSize.setValidator(stepValidator)

        # This causes the last column of the attribute view (the value)
        #  to be stretched to fill the available space
        self._ui.propertyView.horizontalHeader().setStretchLastSection(True)

        self._ui.propertyView.setSelectionBehavior(
            QtGui.QAbstractItemView.SelectRows)
        self._ui.nodeView.setSelectionBehavior(
            QtGui.QAbstractItemView.SelectRows)
        # This allows ctrl and shift clicking for multi-selecting
        self._ui.propertyView.setSelectionMode(
            QtGui.QAbstractItemView.ExtendedSelection)

        self._ui.propertyView.setHorizontalScrollMode(
            QtGui.QAbstractItemView.ScrollPerPixel)

        self._ui.frameSlider.setTracking(self._ui.redrawOnScrub.isChecked())
        
        for action in (self._ui.actionBlack,
                       self._ui.actionGrey_Dark,
                       self._ui.actionGrey_Light,
                       self._ui.actionWhite):
            self._ui.colorGroup.addAction(action)
            action.setChecked(str(action.text()) == self._stageView.clearColor)
        self._ui.colorGroup.setExclusive(True)
        
        for action in (self._ui.actionKey, 
                       self._ui.actionFill, 
                       self._ui.actionBack):
            self._ui.threePointLights.addAction(action)
        self._ui.threePointLights.setExclusive(False)

        self._ui.renderModeActionGroup = QtGui.QActionGroup(self)
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
            action.setChecked(str(action.text()) == self._stageView.renderMode)
        self._ui.renderModeActionGroup.setExclusive(True)
        if self._stageView.renderMode not in \
                            [str(a.text()) for a in
                                    self._ui.renderModeActionGroup.actions()]:
            print "Warning: Unknown render mode '%s', falling back to '%s'" % (
                        self._stageView.renderMode,
                        str(self._ui.renderModeActionGroup.actions()[0].text()))

            self._ui.renderModeActionGroup.actions()[0].setChecked(True)
            self._changeRenderMode(self._ui.renderModeActionGroup.actions()[0])

        self._ui.pickModeActionGroup = QtGui.QActionGroup(self)
        for action in (self._ui.actionPick_Prims,
                       self._ui.actionPick_Models,
                       self._ui.actionPick_Instances):
            self._ui.pickModeActionGroup.addAction(action)
            action.setChecked(str(action.text()) == self._stageView.pickMode)
        self._ui.pickModeActionGroup.setExclusive(True)
        if self._stageView.pickMode not in \
                            [str(a.text()) for a in
                                    self._ui.pickModeActionGroup.actions()]:
            print "Warning: Unknown pick mode '%s', falling back to '%s'" % (
                        self._stageView.pickMode,
                        str(self._ui.pickModeActionGroup.actions()[0].text()))

            self._ui.pickModeActionGroup.actions()[0].setChecked(True)
            self._changePickMode(self._ui.pickModeActionGroup.actions()[0])

        # The error-checking pattern here seems wrong?  Error checking
        # should happen in the changeXXX methods. All we're checking here
        # is the values we hardcoded ourselves in the init function!
        self._ui.selHighlightModeActionGroup = QtGui.QActionGroup(self)
        for action in (self._ui.actionNever,
                       self._ui.actionOnly_when_paused,
                       self._ui.actionAlways):
            self._ui.selHighlightModeActionGroup.addAction(action)
            action.setChecked(str(action.text()) == self._selHighlightMode)
        self._ui.selHighlightModeActionGroup.setExclusive(True)
            
        self._ui.highlightColorActionGroup = QtGui.QActionGroup(self)
        for action in (self._ui.actionSelYellow,
                       self._ui.actionSelCyan,
                       self._ui.actionSelWhite):
            self._ui.highlightColorActionGroup.addAction(action)
            action.setChecked(str(action.text()) == self._highlightColorName)
        self._ui.highlightColorActionGroup.setExclusive(True)
            
        self._ui.interpolationActionGroup = QtGui.QActionGroup(self)
        self._ui.interpolationActionGroup.setExclusive(True)
        for interpolationType in Usd.InterpolationType.allValues:
            action = self._ui.menuInterpolation.addAction(interpolationType.displayName)
            action.setCheckable(True)
            action.setChecked(self._stage.GetInterpolationType() == interpolationType)
            self._ui.interpolationActionGroup.addAction(action)

        self._ui.nodeViewDepthGroup = QtGui.QActionGroup(self)
        for i in range(1,9):
            action = getattr(self._ui,"actionLevel_" + str(i))
            self._ui.nodeViewDepthGroup.addAction(action)

        # Configure stretch behavior for node and property panes
        self._ui.propertyLegendContainer.setMaximumHeight(
            self._propertyLegendHeightOffset)
        self._ui.propertyLegendContainer.setContentsMargins(5,0,5,0)
        propertyLegendPolicy = self._ui.propertyLegendContainer.sizePolicy()
        propertyLegendPolicy.setHorizontalPolicy(QtGui.QSizePolicy.Policy.Fixed)
        propertyLegendPolicy.setVerticalPolicy(QtGui.QSizePolicy.Policy.Fixed)
        self._ui.propertyLegendContainer.setSizePolicy(propertyLegendPolicy)

        nodeViewPolicy = self._ui.nodeView.sizePolicy()
        nodeViewPolicy.setVerticalPolicy(QtGui.QSizePolicy.Policy.Expanding)
        self._ui.nodeView.setSizePolicy(nodeViewPolicy)

        self._ui.nodeLegendContainer.setMaximumHeight(
            self._nodeLegendHeightOffset)
        self._ui.nodeLegendContainer.setContentsMargins(5,0,5,0)
        nodeLegendPolicy = self._ui.nodeLegendContainer.sizePolicy()
        nodeLegendPolicy.setHorizontalPolicy(QtGui.QSizePolicy.Policy.Fixed)
        nodeLegendPolicy.setVerticalPolicy(QtGui.QSizePolicy.Policy.Fixed)
        self._ui.nodeLegendContainer.setSizePolicy(nodeLegendPolicy)

        # set initial styling of '?' buttons
        self._ui.nodeLegendQButton.setStyleSheet(
            self._legendButtonSelectedStyle)
        self._ui.propertyLegendQButton.setStyleSheet(
            self._legendButtonSelectedStyle)

        # needed to set color of boxes
        graphicsScene = QtGui.QGraphicsScene()
        self._ui.propertyLegendColorFallback.setScene(graphicsScene)
        self._ui.propertyLegendColorDefault.setScene(graphicsScene)
        self._ui.propertyLegendColorTimeSample.setScene(graphicsScene)
        self._ui.propertyLegendColorNoValue.setScene(graphicsScene)
        self._ui.propertyLegendColorValueClips.setScene(graphicsScene)
        self._ui.propertyLegendColorCustom.setScene(graphicsScene)
        
        self._ui.nodeLegendColorHasArcs.setScene(graphicsScene)
        self._ui.nodeLegendColorNormal.setScene(graphicsScene)
        self._ui.nodeLegendColorInstance.setScene(graphicsScene)
        self._ui.nodeLegendColorMaster.setScene(graphicsScene)

        # set color of attribute viewer legend boxes
        self._ui.propertyLegendColorFallback.setForegroundBrush(FallbackTextColor)
        self._ui.propertyLegendColorDefault.setForegroundBrush(DefaultTextColor)
        self._ui.propertyLegendColorTimeSample.setForegroundBrush(TimeSampleTextColor)
        self._ui.propertyLegendColorNoValue.setForegroundBrush(NoValueTextColor)
        self._ui.propertyLegendColorValueClips.setForegroundBrush(ValueClipsTextColor)
        self._ui.propertyLegendColorCustom.setForegroundBrush(RedColor)

        self._ui.nodeLegendColorHasArcs.setForegroundBrush(HasArcsColor)
        self._ui.nodeLegendColorNormal.setForegroundBrush(NormalColor)
        self._ui.nodeLegendColorInstance.setForegroundBrush(InstanceColor)
        self._ui.nodeLegendColorMaster.setForegroundBrush(MasterColor)

        # set color of attribute viewer text items
        legendTextUpdate = lambda t, c: (('<font color=\"%s\">' % c.color().name())
                                         + t.text() + '</font>') 
        timeSampleLegend = self._ui.propertyLegendLabelTimeSample
        timeSampleLegend.setText(legendTextUpdate(timeSampleLegend, TimeSampleTextColor))
        
        fallbackLegend = self._ui.propertyLegendLabelFallback
        fallbackLegend.setText(legendTextUpdate(fallbackLegend, FallbackTextColor))

        valueClipLegend = self._ui.propertyLegendLabelValueClips
        valueClipLegend.setText(legendTextUpdate(valueClipLegend, ValueClipsTextColor))

        noValueLegend = self._ui.propertyLegendLabelNoValue
        noValueLegend.setText(legendTextUpdate(noValueLegend, NoValueTextColor))

        defaultLegend = self._ui.propertyLegendLabelDefault
        defaultLegend.setText(legendTextUpdate(defaultLegend, DefaultTextColor))

        customLegend = self._ui.propertyLegendLabelCustom 
        customLegend.setText(legendTextUpdate(customLegend, RedColor))

        normalLegend = self._ui.nodeLegendLabelNormal
        normalLegend.setText(legendTextUpdate(normalLegend, NormalColor))

        masterLegend = self._ui.nodeLegendLabelMaster
        masterLegend.setText(legendTextUpdate(masterLegend, MasterColor))

        instanceLegend = self._ui.nodeLegendLabelInstance
        instanceLegend.setText(legendTextUpdate(instanceLegend, InstanceColor))

        hasArcsLegend = self._ui.nodeLegendLabelHasArcs
        hasArcsLegend.setText(legendTextUpdate(hasArcsLegend, HasArcsColor))

       
        # Format partial strings in the maps for nodeView and propertyView
        # t indicates the whole (t)ext
        # s indicates the desired (s)ubstring
        # m indicates a text (m)ode
        labelUpdate = lambda t, s, m: t.replace(s,'<'+m+'>'+s+'</'+m+'>')
        italicize = lambda t, s: labelUpdate(t, s, 'i') 
        bolden = lambda t, s: labelUpdate(t, s, 'b')

        spanStr = lambda r,g,b: "span style=\"color:rgb(%d, %d, %d);\"" % (r,g,b)
        colorize = lambda t, s, r, g, b: labelUpdate(t, s, spanStr(r,g,b))

        undefinedFontLegend = self._ui.nodeLegendLabelFontsUndefined
        undefinedFontLegend.setText(italicize(undefinedFontLegend.text(), 
                                              undefinedFontLegend.text()))

        definedFontLegend = self._ui.nodeLegendLabelFontsDefined
        definedFontLegend.setText(bolden(definedFontLegend.text(), 
                                         definedFontLegend.text()))


        # Set three individual colors in the text line to indicate
        # the dimmed version of each primary node color
        dimmedLegend = self._ui.nodeLegendLabelDimmed
        dimmedLegendText = dimmedLegend.text()
        dimmedLegendText = colorize(dimmedLegendText, "Dimmed colors", 148, 105, 30)
        dimmedLegendText = colorize(dimmedLegendText, "denote", 78,91,145)
        dimmedLegendText = colorize(dimmedLegendText, "inactive prims", 151,151,151)
        dimmedLegend.setText(dimmedLegendText)

        interpolatedStr = 'Interpolated'
        tsLabel = self._ui.propertyLegendLabelTimeSample
        tsLabel.setText(italicize(tsLabel.text(), interpolatedStr))

        vcLabel = self._ui.propertyLegendLabelValueClips
        vcLabel.setText(italicize(vcLabel.text(), interpolatedStr))

        # setup animation objects for the primView and propertyView
        self._propertyLegendAnim = QtCore.QPropertyAnimation(
            self._ui.propertyLegendContainer, "maximumHeight")
        self._propertyBrowserAnim = QtCore.QPropertyAnimation(
            self._ui.propertyView, "maximumHeight")

        self._nodeLegendAnim = QtCore.QPropertyAnimation(
            self._ui.nodeLegendContainer, "maximumHeight")
        self._nodeBrowserAnim = QtCore.QPropertyAnimation(
            self._ui.nodeView, "maximumHeight")

        # set the context menu policy for the node browser and attribute
        # inspector headers. This is so we can have a context menu on the 
        # headers that allows you to select which columns are visible.
        self._ui.propertyView.horizontalHeader()\
                .setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self._ui.nodeView.header()\
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
        twh.setResizeMode(0, QtGui.QHeaderView.ResizeToContents)
        twh.setResizeMode(1, QtGui.QHeaderView.ResizeToContents)
        twh.setResizeMode(2, QtGui.QHeaderView.Stretch)
        twh.setResizeMode(3, QtGui.QHeaderView.ResizeToContents)

        # Set the node view header to have a fixed size type and vis columns
        nvh = self._ui.nodeView.header()
        nvh.setResizeMode(0, QtGui.QHeaderView.Stretch)
        nvh.setResizeMode(1, QtGui.QHeaderView.ResizeToContents)
        nvh.setResizeMode(2, QtGui.QHeaderView.ResizeToContents)

        avh = self._ui.propertyView.horizontalHeader()
        avh.setResizeMode(2, QtGui.QHeaderView.ResizeToContents)

        # XXX: 
        # To avoid QTBUG-12850 (https://bugreports.qt.io/browse/QTBUG-12850),
        # we force the horizontal scrollbar to always be visible for all
        # QTableWidget widgets in use.
        self._ui.nodeView.setHorizontalScrollBarPolicy(
            QtCore.Qt.ScrollBarAlwaysOn)
        self._ui.propertyView.setHorizontalScrollBarPolicy(
            QtCore.Qt.ScrollBarAlwaysOn)
        self._ui.metadataView.setHorizontalScrollBarPolicy(
            QtCore.Qt.ScrollBarAlwaysOn)
        self._ui.layerStackView.setHorizontalScrollBarPolicy(
            QtCore.Qt.ScrollBarAlwaysOn)

        self._ui.attributeValueEditor.setMainWindow(self)

        QtCore.QObject.connect(self._ui.currentPathWidget,
                               QtCore.SIGNAL('editingFinished()'),
                               self._currentPathChanged)

        QtCore.QObject.connect(self._ui.nodeView,
                               QtCore.SIGNAL('itemSelectionChanged()'),
                               self._itemSelectionChanged)

        QtCore.QObject.connect(self._ui.nodeView,
                               QtCore.SIGNAL('itemClicked(QTreeWidgetItem *, int)'),
                               self._itemClicked)

        QtCore.QObject.connect(self._ui.nodeView.header(),
                               QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                               self._nodeViewHeaderContextMenu)

        QtCore.QObject.connect(self._timer, QtCore.SIGNAL('timeout()'),
                               self._advanceFrameForPlayback)

        QtCore.QObject.connect(self._ui.nodeView, QtCore.SIGNAL(
            'customContextMenuRequested(QPoint)'),
                               self._nodeViewContextMenu)

        QtCore.QObject.connect(self._ui.nodeView, QtCore.SIGNAL(
            'expanded(QModelIndex)'),
                               self._nodeViewExpanded)

        QtCore.QObject.connect(self._ui.frameSlider,
                               QtCore.SIGNAL('valueChanged(int)'),
                               self.setFrame)

        QtCore.QObject.connect(self._ui.frameSlider,
                               QtCore.SIGNAL('sliderMoved(int)'),
                               self._sliderMoved)

        QtCore.QObject.connect(self._ui.frameSlider,
                               QtCore.SIGNAL('sliderReleased()'),
                               self._updateOnFrameChange)

        QtCore.QObject.connect(self._ui.frameField,
                               QtCore.SIGNAL('editingFinished()'),
                               self._frameStringChanged)

        QtCore.QObject.connect(self._ui.rangeBegin,
                               QtCore.SIGNAL('editingFinished()'),
                               self._rangeBeginChanged)

        QtCore.QObject.connect(self._ui.stepSize,
                               QtCore.SIGNAL('editingFinished()'),
                               self._stepSizeChanged)

        QtCore.QObject.connect(self._ui.rangeEnd,
                               QtCore.SIGNAL('editingFinished()'),
                               self._rangeEndChanged)

        QtCore.QObject.connect(self._ui.actionFrame_Forward,
                               QtCore.SIGNAL('triggered()'),
                               self._advanceFrame)

        QtCore.QObject.connect(self._ui.actionFrame_Backwards,
                               QtCore.SIGNAL('triggered()'),
                               self._retreatFrame)

        QtCore.QObject.connect(self._ui.actionReset_View,
                               QtCore.SIGNAL('triggered()'),
                               self._resetView)

        QtCore.QObject.connect(self._ui.actionToggle_Viewer_Mode,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleViewerMode)

        QtCore.QObject.connect(self._ui.actionRenderGraphDefault,
                               QtCore.SIGNAL('triggered()'),
                               self._defaultRenderGraph)

        QtCore.QObject.connect(self._ui.showBBoxes,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._showBBoxes)

        QtCore.QObject.connect(self._ui.showAABBox,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._showAABBox)

        QtCore.QObject.connect(self._ui.showOBBox,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._showOBBox)

        QtCore.QObject.connect(self._ui.showBBoxPlayback,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._showBBoxPlayback)

        QtCore.QObject.connect(self._ui.useExtentsHint,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._setUseExtentsHint)

        QtCore.QObject.connect(self._ui.showInterpreter,
                               QtCore.SIGNAL('triggered()'),
                               self._showInterpreter)

        QtCore.QObject.connect(self._ui.redrawOnScrub,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._redrawOptionToggled)

        QtCore.QObject.connect(self._ui.actionWatch_Window,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._watchWindowToggled)
        
        QtCore.QObject.connect(self._ui.actionRecompute_Clipping_Planes,
                               QtCore.SIGNAL('triggered()'),
                               self._stageView.detachAndReClipFromCurrentCamera)

        QtCore.QObject.connect(self._ui.actionAdjust_Clipping,
                               QtCore.SIGNAL('triggered(bool)'),
                               self._adjustClippingPlanes)

        QtCore.QObject.connect(self._ui.actionOpen,
                               QtCore.SIGNAL('triggered()'),
                               self._openFile)

        QtCore.QObject.connect(self._ui.actionSave_Overrides_As,
                               QtCore.SIGNAL('triggered()'),
                               self._saveOverridesAs)

        QtCore.QObject.connect(self._ui.actionQuit,
                               QtCore.SIGNAL('triggered()'),
                               self._cleanAndClose)

        QtCore.QObject.connect(self._ui.actionReopen_Stage,
                               QtCore.SIGNAL('triggered()'),
                               self._reopenStage)

        QtCore.QObject.connect(self._ui.actionReload_All_Layers,
                               QtCore.SIGNAL('triggered()'),
                               self._reloadStage)

        QtCore.QObject.connect(self._ui.actionFrame_Selection,
                               QtCore.SIGNAL('triggered()'),
                               self._frameSelection)

        QtCore.QObject.connect(self._ui.actionToggle_Framed_View,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleFramedView)

        QtCore.QObject.connect(self._ui.actionAdjust_FOV,
                               QtCore.SIGNAL('triggered()'),
                               self._adjustFOV)

        QtCore.QObject.connect(self._ui.actionComplexity,
                               QtCore.SIGNAL('triggered()'),
                               self._adjustComplexity)
        
        QtCore.QObject.connect(self._ui.actionDisplay_Guide,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleDisplayGuide)

        QtCore.QObject.connect(self._ui.actionDisplay_Proxy,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleDisplayProxy)

        QtCore.QObject.connect(self._ui.actionDisplay_Render,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleDisplayRender)

        QtCore.QObject.connect(self._ui.actionDisplay_Camera_Oracles,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleDisplayCameraOracles)

        QtCore.QObject.connect(self._ui.actionDisplay_PrimId,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleDisplayPrimId)

        QtCore.QObject.connect(self._ui.actionEnable_Hardware_Shading,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleEnableHardwareShading)        

        QtCore.QObject.connect(self._ui.actionCull_Backfaces,
                               QtCore.SIGNAL('toggled(bool)'),
                               self._toggleCullBackfaces)

        QtCore.QObject.connect(self._ui.attributeInspector,
                               QtCore.SIGNAL('currentChanged(int)'),
                               self._updateAttributeInspector)

        QtCore.QObject.connect(self._ui.propertyView,
                               QtCore.SIGNAL('itemSelectionChanged()'),
                               self._refreshWatchWindow)

        QtCore.QObject.connect(self._ui.propertyView,
                               QtCore.SIGNAL('cellClicked(int,int)'),
                               self._propertyViewItemClicked)

        QtCore.QObject.connect(self._ui.propertyView, QtCore.SIGNAL(
            'currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)'),
                               self._populateAttributeInspector)

        QtCore.QObject.connect(self._ui.propertyView.horizontalHeader(),
                               QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                               self._propertyViewHeaderContextMenu)

        QtCore.QObject.connect(self._ui.propertyView,
                               QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                               self._propertyViewContextMenu)

        QtCore.QObject.connect(self._ui.layerStackView,
                               QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                               self._layerStackContextMenu)

        QtCore.QObject.connect(self._ui.compositionTreeWidget,
                               QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                               self._compositionTreeContextMenu)

        QtCore.QObject.connect(self._ui.compositionTreeWidget, QtCore.SIGNAL(
            'currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)'),
                               self._onCompositionSelectionChanged)

        QtCore.QObject.connect(self._ui.renderModeActionGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeRenderMode)

        QtCore.QObject.connect(self._ui.pickModeActionGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changePickMode)

        QtCore.QObject.connect(self._ui.selHighlightModeActionGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeSelHighlightMode)

        QtCore.QObject.connect(self._ui.highlightColorActionGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeHighlightColor)

        QtCore.QObject.connect(self._ui.interpolationActionGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeInterpolationType)

        QtCore.QObject.connect(self._ui.actionAmbient_Only,
                               QtCore.SIGNAL('triggered(bool)'),
                               self._ambientOnlyClicked)
                               
        QtCore.QObject.connect(self._ui.actionKey,
                               QtCore.SIGNAL('triggered(bool)'),
                               self._onKeyLightClicked)
                               
        QtCore.QObject.connect(self._ui.actionFill,
                               QtCore.SIGNAL('triggered(bool)'),
                               self._onFillLightClicked)
                               
        QtCore.QObject.connect(self._ui.actionBack,
                               QtCore.SIGNAL('triggered(bool)'),
                               self._onBackLightClicked)

        QtCore.QObject.connect(self._ui.colorGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeBgColor)

        QtCore.QObject.connect(self._ui.threePointLights,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._stageView.update)

        QtCore.QObject.connect(self._ui.nodeViewDepthGroup,
                               QtCore.SIGNAL('triggered(QAction *)'),
                               self._changeNodeViewDepth)

        QtCore.QObject.connect(self._ui.actionExpand_All,
                               QtCore.SIGNAL('triggered()'),
                               lambda: self._expandToDepth(1000000))

        QtCore.QObject.connect(self._ui.actionCollapse_All,
                               QtCore.SIGNAL('triggered()'),
                               self._ui.nodeView.collapseAll)

        QtCore.QObject.connect(self._ui.actionShow_Inactive_Nodes,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleShowInactiveNodes)

        QtCore.QObject.connect(self._ui.actionShow_Master_Prims,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleShowMasterPrims)

        QtCore.QObject.connect(self._ui.actionShow_Undefined_Prims,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleShowUndefinedPrims)

        QtCore.QObject.connect(self._ui.actionShow_Abstract_Prims,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleShowAbstractPrims)

        QtCore.QObject.connect(self._ui.actionRollover_Prim_Info,
                               QtCore.SIGNAL('triggered()'),
                               self._toggleRolloverPrimInfo)

        QtCore.QObject.connect(self._ui.nodeViewLineEdit,
                               QtCore.SIGNAL('returnPressed()'),
                               self._ui.nodeViewFindNext.click)

        QtCore.QObject.connect(self._ui.nodeViewFindNext,
                               QtCore.SIGNAL('clicked()'),
                               self._nodeViewFindNext)

        QtCore.QObject.connect(self._ui.attrViewLineEdit,
                               QtCore.SIGNAL('returnPressed()'),
                               self._ui.attrViewFindNext.click)

        QtCore.QObject.connect(self._ui.attrViewFindNext,
                               QtCore.SIGNAL('clicked()'),
                               self._attrViewFindNext)

        QtCore.QObject.connect(self._ui.nodeLegendQButton,
                               QtCore.SIGNAL('clicked()'),
                               self._nodeLegendToggleCollapse)

        QtCore.QObject.connect(self._ui.propertyLegendQButton,
                               QtCore.SIGNAL('clicked()'),
                               self._propertyLegendToggleCollapse)

        QtCore.QObject.connect(self._ui.playButton,
                               QtCore.SIGNAL('clicked()'),
                               self._playClicked)

        QtCore.QObject.connect(self._ui.actionHUD,
                               QtCore.SIGNAL('triggered()'),
                               self._updateHUDMenu)

        QtCore.QObject.connect(self._ui.actionHUD_Info,
                               QtCore.SIGNAL('triggered()'),
                               self._updateHUDMenu)

        QtCore.QObject.connect(self._ui.actionHUD_Complexity,
                               QtCore.SIGNAL('triggered()'),
                               self._HUDMenuChanged)

        QtCore.QObject.connect(self._ui.actionHUD_Performance,
                               QtCore.SIGNAL('triggered()'),
                               self._HUDMenuChanged)

        QtCore.QObject.connect(self._ui.actionHUD_GPUstats,
                               QtCore.SIGNAL('triggered()'),
                               self._HUDMenuChanged)

        self.addAction(self._ui.actionIncrementComplexity1)
        self.addAction(self._ui.actionIncrementComplexity2)
        self.addAction(self._ui.actionDecrementComplexity)

        QtCore.QObject.connect(self._ui.actionIncrementComplexity1,
                               QtCore.SIGNAL('triggered()'),
                               self._incrementComplexity)

        QtCore.QObject.connect(self._ui.actionIncrementComplexity2,
                               QtCore.SIGNAL('triggered()'),
                               self._incrementComplexity)

        QtCore.QObject.connect(self._ui.actionDecrementComplexity,
                               QtCore.SIGNAL('triggered()'),
                               self._decrementComplexity)

        QtCore.QObject.connect(self._ui.attributeValueEditor,
                               QtCore.SIGNAL('editComplete(QString)'),
                               self.editComplete)

        # Edit Prim menu
        QtCore.QObject.connect(self._ui.menuEdit_Node,
                               QtCore.SIGNAL('aboutToShow()'),
                               self._updateEditNodeMenu)

        QtCore.QObject.connect(self._ui.actionFind_Prims,
                               QtCore.SIGNAL('triggered()'),
                               self._ui.nodeViewLineEdit.setFocus)

        QtCore.QObject.connect(self._ui.actionJump_to_Stage_Root,
                               QtCore.SIGNAL('triggered()'),
                               self.resetSelectionToPseudoroot)

        QtCore.QObject.connect(self._ui.actionJump_to_Model_Root,
                               QtCore.SIGNAL('triggered()'),
                               self.jumpToEnclosingModelSelectedPrims)

        QtCore.QObject.connect(self._ui.actionJump_to_Bound_Material,
                               QtCore.SIGNAL('triggered()'),
                               self.jumpToBoundMaterialSelectedPrims)

        QtCore.QObject.connect(self._ui.actionJump_to_Master,
                               QtCore.SIGNAL('triggered()'),
                               self.jumpToMasterSelectedPrims)

        QtCore.QObject.connect(self._ui.actionMake_Visible,
                               QtCore.SIGNAL('triggered()'),
                               self.visSelectedPrims)
        # Add extra, Presto-inspired shortcut for Make Visible
        self._ui.actionMake_Visible.setShortcuts(["Shift+H", "Ctrl+Shift+H"])

        QtCore.QObject.connect(self._ui.actionMake_Invisible,
                               QtCore.SIGNAL('triggered()'),
                               self.invisSelectedPrims)

        QtCore.QObject.connect(self._ui.actionVis_Only,
                               QtCore.SIGNAL('triggered()'),
                               self.visOnlySelectedPrims)

        QtCore.QObject.connect(self._ui.actionRemove_Session_Visibility,
                               QtCore.SIGNAL('triggered()'),
                               self.removeVisSelectedPrims)

        QtCore.QObject.connect(self._ui.actionReset_All_Session_Visibility,
                               QtCore.SIGNAL('triggered()'),
                               self.resetSessionVisibility)

        QtCore.QObject.connect(self._ui.actionLoad,
                               QtCore.SIGNAL('triggered()'),
                               self.loadSelectedPrims)

        QtCore.QObject.connect(self._ui.actionUnload,
                               QtCore.SIGNAL('triggered()'),
                               self.unloadSelectedPrims) 

        QtCore.QObject.connect(self._ui.actionActivate,
                               QtCore.SIGNAL('triggered()'),
                               self.activateSelectedPrims)

        QtCore.QObject.connect(self._ui.actionDeactivate,
                               QtCore.SIGNAL('triggered()'),
                               self.deactivateSelectedPrims) 

        self._setupDebugMenu()

        
        # configure plugins 
        self._configurePlugins()

        # save splitter states
        QtCore.QObject.connect(self._ui.nodeStageSplitter,
                               QtCore.SIGNAL('splitterMoved(int, int)'),
                               self._splitterMoved)
        QtCore.QObject.connect(self._ui.topBottomSplitter,
                               QtCore.SIGNAL('splitterMoved(int, int)'),
                               self._splitterMoved)
        QtCore.QObject.connect(self._ui.attribBrowserInspectorSplitter,
                               QtCore.SIGNAL('splitterMoved(int, int)'),
                               self._splitterMoved)

        #create a timer for saving splitter states only when they stop moving
        self._splitterTimer = QtCore.QTimer(self)
        self._splitterTimer.setInterval(500)

        QtCore.QObject.connect(self._splitterTimer, QtCore.SIGNAL('timeout()'),
                               self._saveSplitterStates)

        # timer for slider. when user stops scrubbing for 0.5s, update stuff.
        self._sliderTimer = QtCore.QTimer(self)
        self._sliderTimer.setInterval(500)

        # Connect the update timer to _frameStringChanged, which will ensure
        # we update _currentTime prior to updating UI
        QtCore.QObject.connect(self._sliderTimer, QtCore.SIGNAL('timeout()'),
                               self._frameStringChanged)

        # We manually call processEvents() here to make sure that the prim
        # browser and other widgetry get drawn before we draw the first image in
        # the viewer, which might take a long time.
        if self._stageView:
            self._stageView.setUpdatesEnabled(False)

        self.update()

        QtGui.qApp.processEvents()

        self._drawFirstImage()

        QtGui.QApplication.restoreOverrideCursor()

    def _drawFirstImage(self):

        # _resetView is what triggers the first image to be drawn, so time it
        if self._stageView:
            self._stageView.setUpdatesEnabled(True)
        with BusyContext(), Timer() as t:
            try:
                self._resetView(self._initialSelectNode)
            except Exception:
                pass
        if self._printTiming and not self._noRender:
            t.PrintTime("create first image")

        # configure render graph plugins after stageView initialized its renderer.
        self._configureRenderGraphPlugins()
        
        if self._mallocTags == 'stageAndImaging':
            DumpMallocTags(self._stage, "stage-loading and imaging")
        
    def eventFilter(self, widget, event):
        if event.type() == QtCore.QEvent.KeyPress:
            key = event.key()
            if key == QtCore.Qt.Key_Escape:
                self.setFocus()
                return True
        return QtGui.QMainWindow.eventFilter(self, widget, event)

    def statusMessage(self, msg, timeout = 0):
        self._statusBar.showMessage(msg, timeout * 1000)

    def editComplete(self, msg):
        title = self.windowTitle()
        if title[-1] != '*':
            self.setWindowTitle(title + ' *')

        self.statusMessage(msg, 12)
        with Timer() as t:
            self._stageView.setNodes(self._prunedCurrentNodes, 
                                     self._currentFrame, 
                                     resetCam=False, forceComputeBBox=True)
            self._refreshVars()
        if self._printTiming:
            t.PrintTime("'%s'" % msg)

    def _openStage(self, usdFilePath, populationMaskPaths):
        # Attempt to do specialized asset resolution based on the
        # UsdviewPlug installed plugin, otherwise use the configured
        # Ar instance for asset resolution.
        try:
            from pixar import UsdviewPlug
            self._pathResolverContext = \
                UsdviewPlug.ConfigureAssetResolution(usdFilePath)

        except ImportError:
            from pxr import Ar
            Ar.GetResolver().ConfigureResolverForAsset(usdFilePath)
            self._pathResolverContext = \
                Ar.GetResolver().CreateDefaultContextForAsset(usdFilePath) 

        if not os.path.isfile(usdFilePath):
            print >> sys.stderr, "Error: File not found '" + usdFilePath + \
                        "'. Stage was not opened."
            return None

        if self._mallocTags != 'none':
            Tf.MallocTag.Initialize()

        with Timer() as t:
            loadSet = Usd.Stage.LoadNone if self._unloaded else Usd.Stage.LoadAll
            popMask = (None if populationMaskPaths is None else
                       Usd.StagePopulationMask())
            if popMask:
                for p in populationMaskPaths:
                    popMask.Add(p)
                stage = Usd.Stage.OpenMasked(
                    usdFilePath, self._pathResolverContext, popMask, loadSet)
            else:
                stage = Usd.Stage.Open(
                    usdFilePath, self._pathResolverContext, loadSet)

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
                sl = stage.GetSessionLayer()
                # We can only safely do Sdf-level ops inside an Sdf.ChangeBlock,
                # so gather all the paths from the UsdStage first
                modelPaths = [p.GetPath() for p in \
                                  Usd.TreeIterator.Stage(stage, 
                                                         Usd.PrimIsGroup) ]
                with Sdf.ChangeBlock():
                    for mpp in modelPaths:
                        parent = sl.GetPrimAtPath(mpp.GetParentPath())
                        Sdf.PrimSpec(parent, mpp.name, Sdf.SpecifierOver)
        if not stage:
            print >> sys.stderr, "Error opening stage '" + usdFilePath + "'"
        else:
            if self._printTiming:
                t.PrintTime('open stage "%s"' % usdFilePath)
            stage.SetEditTarget(stage.GetSessionLayer())

        if self._mallocTags == 'stage':
            DumpMallocTags(stage, "stage-loading")

        return stage

    def _setPlayShortcut(self):
        self._ui.playButton.setShortcut(QtGui.QKeySequence(QtCore.Qt.Key_Space))

    # Non-topology dependent UI changes
    def _reloadFixedUI(self):
        # If animation is playing, stop it.
        if self._ui.playButton.isChecked():
            self._ui.playButton.click()
       
        # frame range supplied by user
        ff = self._parserData.firstframe
        lf = self._parserData.lastframe
        
        # frame range supplied by stage
        stageStartTimeCode = self._stage.GetStartTimeCode()
        stageEndTimeCode = self._stage.GetEndTimeCode()

        # final range results
        self.realStartTimeCode = None
        self.realEndTimeCode = None

        self.framesPerSecond = self._stage.GetFramesPerSecond()

        self.step = self._stage.GetTimeCodesPerSecond() / self.framesPerSecond
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
        elif self._stage.HasAuthoredTimeCodeRange():
            self.realStartTimeCode = stageStartTimeCode
            self.realEndTimeCode = stageEndTimeCode

        self._ui.stageBegin.setText(str(stageStartTimeCode))
        self._ui.stageEnd.setText(str(stageEndTimeCode))

        self._UpdateTimeSamples()

    def _UpdateTimeSamples(self):
        if self.realStartTimeCode is not None and self.realEndTimeCode is not None:
            if self.realStartTimeCode > self.realEndTimeCode:
                sys.stderr.write('Warning: Invalid frame range (%s, %s)\n'  
                % (self.realStartTimeCode, self.realEndTimeCode))
                self._timeSamples = []
            else:
                self._timeSamples = drange(self.realStartTimeCode, 
                                           self.realEndTimeCode, 
                                           self.step)
        else:
            self._timeSamples = []

        if self._stageView:
            self._stageView.timeSamples = self._timeSamples
            
        self._geomCounts = dict()
        self._hasTimeSamples = (len(self._timeSamples) > 0)
        self._setPlaybackAvailability() # this sets self._playbackAvailable

        if self._hasTimeSamples:
            self._currentFrame = self._timeSamples[0]
            self._ui.frameField.setText(str(self._currentFrame))
            self._ui.rangeBegin.setText(str(self._timeSamples[0]))
            self._ui.rangeEnd.setText(str(self._timeSamples[-1]))

        if self._playbackAvailable:
            self._ui.frameSlider.setRange(0, len(self._timeSamples)-1)
            self._ui.frameSlider.setValue(self._ui.frameSlider.minimum())
            self._setPlayShortcut()
            self._ui.playButton.setCheckable(True)
            self._ui.playButton.setChecked(False)
        elif not self._hasTimeSamples: 
            # There are no time samples in the stage. Set the effective query 
            # time that usdview uses to 0.0 in this case.
            # This way, we get the following desirable behavior: 
            # * the frame slider will be inactive, 
            # * non-animatd attributes with default values will be read correctly.
            # * animated attributes (in some cases with a single time sample) 
            #   with no default value will also have the expected value in 
            #   the attribute browser.
            #   
            self._currentFrame = 0.0
            self._ui.frameField.setText("0.0")

    # Vars that need updating during a stage reget/refresh
    def _refreshVars(self):
        # Need to refresh selected items to refresh nodes/view to new stage
        self._itemSelectionChanged()

    def _clearCaches(self):
        self._valueCache = dict()

        # create new bounding box cache and xform cache. If there was an instance
        # of the cache before, this will effectively clear the cache.
        self._xformCache = UsdGeom.XformCache(self._currentFrame)
        if self._stageView:
            self._stageView.xformCache = self._xformCache
        self._setUseExtentsHint(self._ui.useExtentsHint.isChecked())
        self._refreshCameraListAndMenu(preserveCurrCamera = False)


    # Render graph plugin support
    def _defaultRenderGraph(self):
        self._stageView.SetRenderGraphPlugin(Tf.Type())

    def _pluginRenderGraphChanged(self, plugin):
        self._stageView.SetRenderGraphPlugin(plugin)

    def _configureRenderGraphPlugins(self):
        if self._stageView:
            self._ui.renderGraphActionGroup = QtGui.QActionGroup(self)
            self._ui.renderGraphActionGroup.setExclusive(True)
            self._ui.renderGraphActionGroup.addAction(
                self._ui.actionRenderGraphDefault)

            pluginTypes = self._stageView.GetRenderGraphPlugins()
            for pluginType in pluginTypes:
                name = Plug.Registry().GetStringFromPluginMetaData(
                    pluginType, 'displayName')
                action = self._ui.menuRenderGraph.addAction(name)
                action.setCheckable(True)
                action.pluginType = pluginType
                self._ui.renderGraphActionGroup.addAction(action)

                QtCore.QObject.connect(
                    action,
                    QtCore.SIGNAL('triggered()'),
                    lambda pluginType = pluginType:
                        self._pluginRenderGraphChanged(pluginType))

    # Topology-dependent UI changes
    def _reloadVaryingUI(self):

        # We must call ReloadStage() before _clearCaches() to avoid a crash in
        # the case when we have reopened the stage. The problem is when the
        # stage is being reopened, its internal state is inconsistent, but as a
        # result of calling _clearCaches(), we will attempt to update bounding
        # boxes via _setUseExtentHints.
        if self._stageView:
            self._stageView.ReloadStage(self._stage)

        self._clearCaches()

        # The difference between these two is related to multi-selection: 
        # - currentNodes contains all nodes selected
        # - prunedCurrentNodes contains all nodes selected, excluding nodes that
        #   already have a parent selected (used to avoid double-rendering)
        self._currentNodes = [self._stage.GetPseudoRoot()]
        self._prunedCurrentNodes = self._currentNodes

        if self._debug:
            import cProfile, pstats
            cProfile.runctx('self._resetNodeView(restoreSelection=False)', globals(), locals(), 'resetNodeView')
            p = pstats.Stats('resetNodeView')
            p.strip_dirs().sort_stats(-1).print_stats()
        else:
            self._resetNodeView(restoreSelection=False)

        self._ui.frameSlider.setValue(self._ui.frameSlider.minimum())

        # Make sure to clear the texture registry, as its contents depend
        # on the GL state established by the StageView (bug 56866).
        Glf.TextureRegistry.Reset()
        if not self._stageView:
            self._stageView = StageView(parent=self, 
                                        xformCache=self._xformCache,
                                        bboxCache=self._bboxCache)
            self._stageView.SetStage(self._stage)
            self._stageView.noRender = self._noRender

            self._stageView.fpsHUDInfo = self._fpsHUDInfo
            self._stageView.fpsHUDKeys = self._fpsHUDKeys
            
            self._stageView.signalCurrentFrameChanged.connect(
                self.onCurrentFrameChanged)
            self._stageView.signalPrimSelected.connect(self.onPrimSelected)
            self._stageView.signalPrimRollover.connect(self.onRollover)
            self._stageView.signalMouseDrag.connect(self.onStageViewMouseDrag)
            self._stageView.signalErrorMessage.connect(self.statusMessage)
            
            # The second child is self._ui.glFrame, which disappears if
            # its size is set to zero.
            if self._noRender:
                # remove glFrame from the ui
                self._ui.glFrame.setParent(None)

                # move the attributeBrowser into the nodeSplitter instead
                self._ui.nodeStageSplitter.addWidget(self._ui.attributeBrowserFrame)

            else:
                layout = QtGui.QVBoxLayout()
                layout.setContentsMargins(0, 0, 0, 0)
                self._ui.glFrame.setLayout(layout)
                layout.addWidget(self._stageView)

        self._playbackFrameIndex = 0

        self._nodeSearchResults = []
        self._attrSearchResults = []
        self._nodeSearchString = ""
        self._attrSearchString = ""
        self._lastNodeSearched = self._currentNodes[0]

        self._stageView.setFocus(QtCore.Qt.TabFocusReason)
        self._stageView.rolloverPicking = \
            self._settings.get("actionRollover_Prim_Info", False)

    # This appears to be "reasonably" performant in normal sized pose caches.
    # If it turns out to be too slow, or if we want to do a better job of
    # preserving the view the user currently has, we could look into ways of 
    # reconstructing just the prim tree under the "changed" prim(s).  The
    # (far and away) faster solution would be to implement our own TreeView
    # and model in C++.
    def _resetNodeView(self, restoreSelection=True):
        with Timer() as t, BusyContext():
            startingDepth = 3
            self._nodeViewResetTimer.stop()
            self._computeDisplayPredicate()
            self._ui.nodeView.setUpdatesEnabled(False)
            self._ui.nodeView.clear()
            self._nodeToItemMap.clear()
            self._itemsToPush = []
            # force new search since we are blowing away the nodeViewItems
            # that may be cached in _nodeSearchResults
            self._nodeSearchResults = []
            self._populateRoots()
            # it's confusing to see timing for expand followed by reset with
            # the times being similar (esp when they are large)
            self._expandToDepth(startingDepth, suppressTiming=True)
            if restoreSelection:
                self._setSelectionFromPrimList(self._currentNodes)
            self._ui.nodeView.setUpdatesEnabled(True)
            self._refreshCameraListAndMenu(preserveCurrCamera = True)
        if self._printTiming:
            t.PrintTime("reset Prim Browser to depth %d" % startingDepth)

    def UpdateNodeViewContents(self):
        """Will schedule a full refresh/resync of the Prim Browser's contents.
        Prefer this to calling _resetNodeView() directly, since it will 
        coalesce multiple calls to this method in to a single refresh"""
        self._nodeViewResetTimer.stop()
        self._nodeViewResetTimer.start(250)

    def _resetNodeViewVis(self, selItemsOnly=True,
                          authoredVisHasChanged=True):
        """Updates browser rows' Vis columns... can update just selected
        items (and their descendants and ancestors), or all items in the
        nodeView.  When authoredVisHasChanged is True, we force each item
        to discard any value caches it may be holding onto."""
        with Timer() as t:
            self._ui.nodeView.setUpdatesEnabled(False)
            rootsToProcess = self.getSelectedItems() if selItemsOnly else \
                [self._ui.nodeView.invisibleRootItem()]
            for item in rootsToProcess:
                NodeViewItem.propagateVis(item, authoredVisHasChanged)
            self._ui.nodeView.setUpdatesEnabled(True)
        if self._printTiming:
            t.PrintTime("update vis column")

    def _updateNodeView(self):
        # Process some more node view items.
        n = min(100, len(self._itemsToPush))
        if n:
            items = self._itemsToPush[-n:]
            del self._itemsToPush[-n:]
            for item in items:
                item.push()
        else:
            self._nodeViewUpdateTimer.stop()

    # Option windows ==========================================================

    def _incrementComplexity(self):
        self._stageView.complexity += .1
        if self._stageView.complexity > 1.999:
            self._stageView.complexity = 2.0
        self._stageView.update()

    def _decrementComplexity(self):
        self._stageView.complexity -= .1
        if self._stageView.complexity < 1.001:
            self._stageView.complexity = 1.0
        self._stageView.update()

    def _adjustComplexity(self):
        complexity= QtGui.QInputDialog.getDouble(self, 
            "Adjust complexity", "Enter a value between 1 and 2.\n\n" 
            "You can also use ctrl+ or ctrl- to adjust the\n"
            "complexity without invoking this dialog.\n", 
            self._stageView.complexity, 1.0,2.0,2)
        if complexity[1]:
            self._stageView.complexity = complexity[0]
            self._stageView.update()

    def _adjustFOV(self):
        fov = QtGui.QInputDialog.getDouble(self, "Adjust FOV", 
            "Enter a value between 0 and 180", self._stageView.freeCamera.fov, 0, 180)
        if (fov[1]):
            self._stageView.freeCamera.fov = fov[0]
            self._stageView.update()

    def _adjustClippingPlanes(self, checked):
        if (not checked):
            self._adjustClipping.accept()
        else:
            self._adjustClipping = adjustClipping.AdjustClipping(self)
            self._adjustClipping.show()

    def _redrawOptionToggled(self, checked):
        self._settings.setAndSave(RedrawOnScrub=checked)
        self._ui.frameSlider.setTracking(checked)

    def _watchWindowToggled(self, checked):
        if (not checked):
            self._watchWindow.accept()
        else:
            self._watchWindow = watchWindow.WatchWindow(self)
            # dock the watch window next to the main usdview window
            self._watchWindow.move(self.x() + self.frameGeometry().width(),
                                   self.y())
            self._watchWindow.resize(self.size())
            self._watchWindow.show()
            self._refreshWatchWindow()

    # XXX USD WATCH WINDOW DEACTIVATED
    def _refreshWatchWindow(self, updateVaryingOnly = False):
        if (self._ui.actionWatch_Window.isChecked()):
            varyScrollPos = \
                self._watchWindow._ui.varyingEdit.verticalScrollBar().value()
            unvaryScrollPos = \
                self._watchWindow._ui.unvaryingEdit.verticalScrollBar().value()

            self._watchWindow.clearContents()
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

            for i in self._ui.propertyView.selectedItems():
                if i.column() == 0:        # make sure first column is selected
                
                    attrName = str(i.text())
                    # index [0] is the the type, which can be:
                       # attribType.UNVARYING, AUTHORED OR INTERPOLATED
                    type = self._nodeDict[attrName][0]
                        
                    # # # # # # # # # # # # # #
                    # populate UNVARYING side #
                    dictKey = self._currentNodes[0].GetPath() + \
                              str(Usd.Scene.FRAME_UNVARYING) + \
                              attrName + '_UNVARYING'

                    if (dictKey not in self._valueCache or
                        self._valueCache[dictKey][0].find(
                            "Pretty-printing canceled") != -1):
                            
                        # unvarying data is in self._nodeDict[attrName][2],
                        # so this checks if there is unvarying data
                        if len(self._nodeDict[attrName]) > 2:
                            col = UnvaryingTextColor.color()
                            valString = prettyPrint.prettyPrint(
                                        self._nodeDict[attrName][2])
                        else:   # no unvarying data
                            col = RedColor.color()
                            valString = "Not defined in unvarying section"

                        self._valueCache[dictKey] = (valString, col)

                    text = self._valueCache[dictKey][0]
                    color = self._valueCache[dictKey][1]
                    self._watchWindow.appendUnvaryingContents(
                        attrName + "\n" + text, color)

                    # # # # # # # # # # # # #
                    # populate VARYING side #
                    dictKey = self._currentNodes[0].GetPath() + \
                              str(self._currentFrame) + \
                              attrName + '_VARYING'

                    if (dictKey not in self._valueCache or 
                        self._valueCache[dictKey][0].find(
                            "Pretty-printing canceled") != -1):
                                
                        # varying data is stored at index [1],
                        # in self._nodeDict[attrName][1]
                        if type == attribType.AUTHORED:
                            col = AuthoredTextColor.color()
                            valString = prettyPrint.prettyPrint(
                                        self._nodeDict[attrName][1])
                        elif type == attribType.INTERPOLATED:
                            col = InterpolatedTextColor.color()
                            valString = prettyPrint.prettyPrint(
                                        self._nodeDict[attrName][1])
                        else:
                            col = RedColor.color()
                            valString = "Not defined in varying section"

                        self._valueCache[dictKey] = (valString, col)

                    text = self._valueCache[dictKey][0]
                    color = self._valueCache[dictKey][1]
                    self._watchWindow.appendContents(
                        attrName + "\n" + text, color)

                    # tell the watch window to print a "====" separator
                    self._watchWindow.appendSeparator()

            QtGui.QApplication.restoreOverrideCursor()
            self._watchWindow._ui.varyingEdit.verticalScrollBar().setValue(
                varyScrollPos)
            self._watchWindow._ui.unvaryingEdit.verticalScrollBar().setValue(
                unvaryScrollPos)

    # Frame-by-frame/Playback functionality ===================================

    def _setPlaybackAvailability(self, enabled = True):
        isEnabled = len(self._timeSamples) > 1 and enabled
        self._playbackAvailable = isEnabled

        #If playback is disabled, but the animation is playing...
        if not isEnabled and self._ui.playButton.isChecked():
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
            self._ui.playButton.setText("Stop")
            # setText() causes the shortcut to be reset to whatever
            # Qt thinks it should be based on the text.  We know better.
            self._setPlayShortcut()
            self._fpsHUDInfo[PLAYBACK]  = "..."
            self._timer.start()
            # For performance, don't update the node tree view while playing.
            self._nodeViewUpdateTimer.stop()
            self._stageView.playing = True
            self._playbackIndex = 0
        else:
            # Stop playback.
            self._ui.playButton.setText("Play")
            # setText() causes the shortcut to be reset to whatever
            # Qt thinks it should be based on the text.  We know better.
            self._setPlayShortcut()
            self._fpsHUDInfo[PLAYBACK]  = "N/A"
            self._timer.stop()
            self._nodeViewUpdateTimer.start()
            self._stageView.playing = False
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
            self._fpsHUDInfo[PLAYBACK] = "%.2f ms (%.2f FPS)" % (ms, fps)

        self._playbackIndex = (self._playbackIndex + 1) % 5
        self._advanceFrame()

    def _advanceFrame(self):
        newValue = self._ui.frameSlider.value() + 1
        if newValue > self._ui.frameSlider.maximum():
            newValue = self._ui.frameSlider.minimum()
        self._ui.frameSlider.setValue(newValue)

    def _retreatFrame(self):
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
        self._UpdateTimeSamples()

    def _stepSizeChanged(self):
        stepStr = self._ui.stepSize.text()
        self.step = float(stepStr)
        self._UpdateTimeSamples()
    
    def _rangeEndChanged(self):
        self.realEndTimeCode = float(self._ui.rangeEnd.text())
        self._UpdateTimeSamples()

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

    # Node/Attribute search functionality =====================================

    def _findNodes(self, pattern, useRegex=True):
        """Search the Usd Stage for matching nodes
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
                   in Usd.TreeIterator.Stage(self._stage, 
                                             self._displayPredicate)
                   if isMatch(prim.GetName())]

        showMasters = self._ui.actionShow_Master_Prims.isChecked()
        if showMasters:
            for master in self._stage.GetMasters():
                matches += [prim.GetPath() for prim
                            in Usd.TreeIterator(master, self._displayPredicate)
                            if isMatch(prim.GetName())]
        
        return matches

    def _nodeViewFindNext(self):
        if (self._nodeSearchString == self._ui.nodeViewLineEdit.text() and
            len(self._nodeSearchResults) > 0 and
            self._lastNodeSearched == self._currentNodes[0]):
            # Go to the next result of the currently ongoing search.
            # First time through, we'll be converting from SdfPaths
            # to items (see the append() below)
            nextResult = self._nodeSearchResults.popleft()
            if isinstance(nextResult, Sdf.Path):
                nextResult = self._getItemAtPath(nextResult)

            if nextResult:
                self._ui.nodeView.setCurrentItem(nextResult)
                self._nodeSearchResults.append(nextResult)
                self._lastNodeSearched = self._currentNodes[0]
            # The path is effectively pruned if we couldn't map the
            # path to an item
        else:
            # Begin a new search
            with Timer() as t:
                self._nodeSearchString = self._ui.nodeViewLineEdit.text()
                self._nodeSearchResults = self._findNodes(str(self._ui.nodeViewLineEdit.text()))
                
                self._nodeSearchResults = deque(self._nodeSearchResults)
                self._lastNodeSearched = self._currentNodes[0]
                
                if (len(self._nodeSearchResults) > 0):
                    self._nodeViewFindNext()
            if self._printTiming:
                t.PrintTime("match '%s' (%d matches)" % 
                            (self._nodeSearchString, 
                             len(self._nodeSearchResults)))

    def _setAnimValues(self, anim, a1, a2):
        anim.setStartValue(a1)
        anim.setEndValue(a2)

    # A function which takes a two-pane area and transforms it to
    # open or close the bottom pane.
    #
    #    legendHeight       
    #    |   separator height
    #    |   |  browser height
    #    |   |  |->     ___________                ___________
    #    |   |  |      |           |              |           |
    #    |   |  |      |           |              |           |
    #    |   |  |->    |           |    <--->     |           |
    #    |---|-------> +++++++++++++              |           |
    #    |             |           |    <--->     |           |
    #    |             |           |              |           |
    #    |----------->  -----------                +++++++++++
    def _toggleLegendWithBrowser(self, button, legendMinimized, 
                                 legendHeight, legendResetHeight, legendAnim,
                                 browserHeight, browserAnim, separatorHeight):


        # We are dragging downward, so collapse the legend and expand the
        # attribute viewer panel to take up the remaining space.
        if legendMinimized:
            button.setStyleSheet('')
            self._setAnimValues(legendAnim, legendHeight, 0)
            self._setAnimValues(browserAnim, browserHeight, 
                                browserHeight+legendHeight)
        # We are expanding, so do the opposite.
        else:
            button.setStyleSheet(self._legendButtonSelectedStyle)
            self._setAnimValues(legendAnim, 0, legendResetHeight)
            self._setAnimValues(browserAnim, browserHeight,
                                browserHeight-legendResetHeight+separatorHeight)

        legendAnim.start()
        browserAnim.start()

    def _nodeLegendToggleCollapse(self):
        # Toggle status and update the button text accordingly
        self._nodeLegendCollapsed = not self._nodeLegendCollapsed
        self._toggleLegendWithBrowser(self._ui.nodeLegendQButton,
                                      self._nodeLegendCollapsed,
                                      self._ui.nodeLegendContainer.height(),
                                      self._nodeLegendHeightOffset,
                                      self._nodeLegendAnim,
                                      self._ui.nodeView.height(),
                                      self._nodeBrowserAnim,
                                      self._ui.nodeView.verticalScrollBar().height())

    def _propertyLegendToggleCollapse(self):
        self._propertyLegendCollapsed = not self._propertyLegendCollapsed
        self._toggleLegendWithBrowser(self._ui.propertyLegendQButton,
                                      self._propertyLegendCollapsed,
                                      self._ui.propertyLegendContainer.height(), 
                                      self._propertyLegendHeightOffset,
                                      self._propertyLegendAnim,
                                      self._ui.propertyView.height(),
                                      self._propertyBrowserAnim,
                                      self._ui.propertyView.verticalScrollBar().height())

    def _attrViewFindNext(self):
        self._ui.propertyView.clearSelection()
        if (self._attrSearchString == self._ui.attrViewLineEdit.text() and
            len(self._attrSearchResults) > 0 and
            self._lastNodeSearched == self._currentNodes[0]):
            # Go to the next result of the currently ongoing search

            nextResult = self._attrSearchResults.popleft()

            # the 0 stands for column 0, so it selects the first column.
            self._ui.propertyView.item(nextResult.row(), 0).setSelected(True)
            self._ui.propertyView.scrollToItem(nextResult)
            self._attrSearchResults.append(nextResult)
            self._lastNodeSearched = self._currentNodes[0]
        else:
            # Begin a new search
            self._attrSearchString = self._ui.attrViewLineEdit.text()
            self._attrSearchResults = self._ui.propertyView.findItems(
                self._ui.attrViewLineEdit.text(),
                QtCore.Qt.MatchRegExp)

            # Now just search for the string itself
            otherSearch = self._ui.propertyView.findItems(
                self._ui.attrViewLineEdit.text(), 
                QtCore.Qt.MatchContains)
            self._attrSearchResults += otherSearch

            self._attrSearchResults = \
                uniquify_tablewidgetitems(self._attrSearchResults)
            self._attrSearchResults.sort(cmp, QtGui.QTableWidgetItem.row)
            self._attrSearchResults = deque(self._attrSearchResults)

            self._lastNodeSearched = self._currentNodes[0]
            if (len(self._attrSearchResults) > 0):
                self._attrViewFindNext()

    @classmethod
    def _outputBaseDirectory(cls):
        import os

        baseDir = os.getenv('HOME') + "/.usdview/"

        if not os.path.exists(baseDir):
            os.makedirs(baseDir)
        return baseDir
                   
    # View adjustment functionality ===========================================

    def _storeAndReturnViewState(self):
        lastView = self._lastViewContext
        self._lastViewContext = self._stageView.copyViewState()
        return lastView

    def _frameSelection(self):
        # Save all the pertinent attribute values (for _toggleFramedView)
        self._storeAndReturnViewState() # ignore return val - we're stomping it
        self._stageView.setNodes(self._prunedCurrentNodes, self._currentFrame,
                                 True, True) # compute bbox on frame selection

    def _toggleFramedView(self):
        self._stageView.restoreViewState(self._storeAndReturnViewState())

    def _resetSettings(self):
        """Reloads the UI and Sets up the initial settings for the 
        _stageView object created in _reloadVaryingUI"""
        self._ui.actionShow_Inactive_Nodes.setChecked(\
                        self._settings.get("actionShow_Inactive_Nodes", True))
        self._ui.actionShow_Master_Prims.setChecked(\
                        self._settings.get("actionShow_Master_Prims", False))
        self._ui.actionShow_Undefined_Prims.setChecked(\
                        self._settings.get("actionShow_Undefined_Prims", False))
        self._ui.actionShow_Abstract_Prims.setChecked(\
                        self._settings.get("actionShow_Abstract_Prims", False))
        self._ui.redrawOnScrub.setChecked(self._settings.get("RedrawOnScrub", True))

        # RELOAD fixed and varying UI
        self._reloadFixedUI()
        self._reloadVaryingUI()

        # restore splitter positions
        splitterSettings1 = self._settings.get('nodeStageSplitter')
        if not splitterSettings1 is None:
            self._ui.nodeStageSplitter.restoreState(splitterSettings1)

        splitterSettings2 = self._settings.get('topBottomSplitter')
        if not splitterSettings2 is None:
            self._ui.topBottomSplitter.restoreState(splitterSettings2)

        splitterSettings3 = self._settings.get('attribBrowserInspectorSplitter')
        if not splitterSettings3 is None:
            self._ui.attribBrowserInspectorSplitter.restoreState(splitterSettings3)

        nodeViewHeaderSettings = self._settings.get('nodeViewHeader', [])
        for i,b in enumerate(nodeViewHeaderSettings):
            self._ui.nodeView.setColumnHidden(i, b)

        self._ui.attributeInspector.\
            setCurrentIndex(self._settings.get("AttributeInspectorCurrentTab", 
                                               INDEX_VALUE))
            
        propertyViewHeaderSettings = self._settings.get('propertyViewHeader', [])
        for i,b in enumerate(propertyViewHeaderSettings):
            self._ui.propertyView.setColumnHidden(i, b)

        self._stageView.showAABBox = self._settings.get("ShowAABBox", True)
        self._ui.showAABBox.setChecked(self._stageView.showAABBox)
        self._stageView.showOBBox = self._settings.get("ShowOBBox", False)
        self._ui.showOBBox.setChecked(self._stageView.showOBBox)
        self._ui.showBBoxPlayback.setChecked(
                                self._settings.get("ShowBBoxPlayback", False))
        self._stageView.showBBoxes = self._settings.get("ShowBBoxes", True) 
        self._ui.showBBoxes.setChecked(self._stageView.showBBoxes)

        displayGuide = self._settings.get("DisplayGuide", False)
        self._ui.actionDisplay_Guide.setChecked(displayGuide)
        self._stageView.setDisplayGuide(displayGuide)

        displayProxy = self._settings.get("DisplayProxy", True)
        self._ui.actionDisplay_Proxy.setChecked(displayProxy)
        self._stageView.setDisplayProxy(displayProxy)

        displayRender = self._settings.get("DisplayRender", False)
        self._ui.actionDisplay_Render.setChecked(displayRender)
        self._stageView.setDisplayRender(displayRender)

        displayCameraOracles = self._settings.get("DisplayCameraOracles", False)
        self._ui.actionDisplay_Camera_Oracles.setChecked(displayCameraOracles)
        self._stageView.setDisplayCameraOracles(displayCameraOracles)

        displayPrimId = self._settings.get("DisplayPrimId", False)
        self._ui.actionDisplay_PrimId.setChecked(displayPrimId)
        self._stageView.setDisplayPrimId(displayPrimId)

        enableHardwareShading = self._settings.get("EnableHardwareShading", True)
        self._ui.actionEnable_Hardware_Shading.setChecked(enableHardwareShading)
        self._stageView.setEnableHardwareShading(enableHardwareShading)

        cullBackfaces = self._settings.get("CullBackfaces", False)
        self._ui.actionCull_Backfaces.setChecked(cullBackfaces)
        self._stageView.setCullBackfaces(cullBackfaces)
        
        self._stageView.showHUD = self._settings.get("actionHUD", True)
        self._ui.actionHUD.setChecked(self._stageView.showHUD)
        # XXX Until we can make the "Subtree Info" stats-gathering faster,
        # we do not want the setting to persist from session to session.
        # self._stageView.showHUD_Info = \
        #     self._settings.get("actionHUD_Info", False)
        self._stageView.showHUD_Info = False
        self._ui.actionHUD_Info.setChecked(self._stageView.showHUD_Info)
        self._stageView.showHUD_Complexity = \
            self._settings.get("actionHUD_Complexity", True)
        self._ui.actionHUD_Complexity.setChecked(
            self._stageView.showHUD_Complexity)
        self._stageView.showHUD_Performance = \
            self._settings.get("actionHUD_Performance", True)
        self._ui.actionHUD_Performance.setChecked(
            self._stageView.showHUD_Performance)
        self._stageView.showHUD_GPUstats = \
            self._settings.get("actionHUD_GPUstats", False)
        self._ui.actionHUD_GPUstats.setChecked(
            self._stageView.showHUD_GPUstats)
            
        # Three point lights are disabled by default. They turn on when the
        #  "Ambient Only" mode is unchecked
        ambOnly = self._settings.get("AmbientOnly", True)
        self._ui.actionAmbient_Only.setChecked(ambOnly)
        self._ui.threePointLights.setEnabled(not ambOnly)
        
        key = self._settings.get("KeyLightEnabled", False)
        fill = self._settings.get("FillLightEnabled", False)
        back = self._settings.get("BackLightEnabled", False)
        self._lightsChecked = [key, fill, back]
        self._ui.actionKey.setChecked(key)
        self._ui.actionFill.setChecked(fill)
        self._ui.actionBack.setChecked(back)
        self._stageView.ambientLightOnly = ambOnly
        self._stageView.keyLightEnabled = key
        self._stageView.fillLightEnabled = fill
        self._stageView.backLightEnabled = back

        self._stageView.update()

        self._stageView.clearColor = self._colorsDict[self._settings.get("ClearColor", "Grey (Dark)")]
        self._stageView.renderMode = self._settings.get("RenderMode",
                                                        "Smooth Shaded")
        self._stageView.pickMode = self._settings.get("PickMode",
                                                      "Prims")
        self._highlightColorName = str(self._settings.get("HighlightColor", "Yellow"))
        self._stageView.highlightColor = self._colorsDict[self._highlightColorName]
        self._selHighlightMode = self._settings.get("SelHighlightMode",
                                                 "Only when paused")
        self._stageView.drawSelHighlights = ( self._selHighlightMode != "Never")
        
        # lighting is not activated until a shaded mode is selected
        self._ui.menuLights.setEnabled(self._stageView.renderMode in ('Smooth Shaded',
                                                                      'Flat Shaded',
                                                                      'WireframeOnSurface',
                                                                      'Geom Flat',
                                                                      'Geom Smooth'))

        self._ui.actionFreeCam._node = None
        QtCore.QObject.connect(
            self._ui.actionFreeCam,
            QtCore.SIGNAL('triggered()'),
            lambda : self._cameraSelectionChanged(None))
        QtCore.QObject.connect(
            self._stageView,
            QtCore.SIGNAL('signalSwitchedToFreeCam()'),
            lambda : self._cameraSelectionChanged(None))

        self._refreshCameraListAndMenu(preserveCurrCamera = False)

    def _saveSplitterStates(self):
        # we dont want any of the splitter positions to be saved when using
        # -norender
        if not self._noRender:
            self._settings.setAndSave(
                    nodeStageSplitter = self._ui.nodeStageSplitter.saveState())

            self._settings.setAndSave(
                    topBottomSplitter = self._ui.topBottomSplitter.saveState())

            self._settings.setAndSave(
                    attribBrowserInspectorSplitter = self._ui.attribBrowserInspectorSplitter.\
                                                                saveState())
        self._splitterTimer.stop()

    def _splitterMoved(self, pos, index):
        # reset the timer every time a splitter moves
        # when the splitters stop moving for half a second, save state
        self._splitterTimer.stop()
        self._splitterTimer.start()

    def _toggleViewerMode(self):
        splitter1 = self._ui.topBottomSplitter
        splitter2 = self._ui.nodeStageSplitter
        sz1 = splitter1.sizes()
        sz2 = splitter2.sizes()
        if sz1[1] > 0 or sz2[0] > 0:
            sz1[0] += sz1[1]
            sz1[1] = 0
            sz2[1] += sz2[0]
            sz2[0] = 0
            splitter1.setSizes(sz1)
            splitter2.setSizes(sz2)
        else:
            # restore saved state
            splitterSettings1 = self._settings.get('nodeStageSplitter')
            if not splitterSettings1 is None:
                self._ui.nodeStageSplitter.restoreState(splitterSettings1)
            splitterSettings2 = self._settings.get('topBottomSplitter')
            if not splitterSettings2 is None:
                self._ui.topBottomSplitter.restoreState(splitterSettings2)

            # Make sure the restored state isn't also collapsed
            sz1 = splitter1.sizes()
            sz2 = splitter2.sizes()
            if sz1[1] == 0 and sz2[0] == 0:
                sz1[1] = .25 * sz1[0]
                sz1[0] = .75 * sz1[0]
                splitter1.setSizes(sz1)
                sz2[0] = .25 * sz2[1]
                sz2[1] = .75 * sz2[1]
                splitter2.setSizes(sz2)

    def _resetView(self,selectNode = None):
        """ Reverts the GL frame to the initial camera view,
        and clears selection (sets to pseudoRoot), UNLESS 'selectNode' is
        not None, in which case we'll select and frame it."""
        self._ui.nodeView.clearSelection()
        pRoot = self._stage.GetPseudoRoot()
        if selectNode is None:
            # if we had a command-line specified selection, re-frame it
            selectNode = self._initialSelectNode or pRoot

        item = self._getItemAtPath(selectNode.GetPath())
        # Our response to selection-change includes redrawing.  We do NOT
        # want that to happen here, since we are subsequently going to
        # change the camera framing (and redraw, again), which can cause 
        # flickering.  So make sure we don't redraw!
        suppressRendering = self._noRender
        self._noRender = True
        self._ui.nodeView.setCurrentItem(item)
        self._noRender = suppressRendering

        if (selectNode and selectNode != pRoot) or not self._startingPrimCamera:
            # _frameSelection translates the camera from wherever it happens
            # to be at the time.  If we had a starting selection AND a
            # primCam, then before framing, switch back to the prim camera
            if selectNode == self._initialSelectNode and self._startingPrimCamera:
                self._stageView.setCameraPrim(self._startingPrimCamera)
            self._frameSelection()
        else:
            self._stageView.setCameraPrim(self._startingPrimCamera)
            self._stageView.setNodes(self._prunedCurrentNodes, 
                                     self._currentFrame)

    def _changeRenderMode(self, mode):
        self._stageView.renderMode = str(mode.text())
        self._settings.setAndSave(RenderMode=self._stageView.renderMode)
        if (self._stageView.renderMode in ('Smooth Shaded',
                                           'Flat Shaded',
                                           'WireframeOnSurface',
                                           'Geom Smooth',
                                           'Geom Flat')):
            self._ui.menuLights.setEnabled(True)
        else:
            self._ui.menuLights.setEnabled(False)
        self._stageView.update()

    def _changePickMode(self, mode):
        self._settings.setAndSave(PickMode=mode.text())

    def _changeSelHighlightMode(self, mode):
        self._settings.setAndSave(SelHighlightMode=str(mode.text()))
        self._selHighlightMode = str(mode.text())
        self._stageView.drawSelHighlights = (self._selHighlightMode != "Never")
        self._stageView.update()

    def _changeHighlightColor(self, color):
        self._settings.setAndSave(HighlightColor=str(color.text()))
        color = str(color.text())
        self._highlightColorName = color
        self._stageView.highlightColor = self._colorsDict[color]
        self._stageView.update()

    def _changeInterpolationType(self, interpolationType):
        for t in Usd.InterpolationType.allValues:
            if t.displayName == str(interpolationType.text()):
                self._stage.SetInterpolationType(t)
                self._resetSettings()
                break

    def _ambientOnlyClicked(self, checked):
        if checked:
            self._lightsChecked = [self._ui.actionKey.isChecked(),
                                   self._ui.actionFill.isChecked(),
                                   self._ui.actionBack.isChecked()]
            self._ui.actionKey.setChecked(False)
            self._ui.actionFill.setChecked(False)
            self._ui.actionBack.setChecked(False)
            self._ui.threePointLights.setEnabled(False)
            self._settings.setAndSave(AmbientOnly=True)
            self._settings.setAndSave(KeyLightEnabled=False,
                                      BackLightEnabled=False,
                                      FillLightEnabled=False)
        else:
            self._settings.setAndSave(AmbientOnly=False)
            self._ui.threePointLights.setEnabled(True)
            if self._lightsChecked == [False, False, False]:
                self._lightsChecked = [True, True, True]
            self._ui.actionKey.setChecked(self._lightsChecked[0])
            self._ui.actionFill.setChecked(self._lightsChecked[1])
            self._ui.actionBack.setChecked(self._lightsChecked[2])
            self._settings.setAndSave(KeyLightEnabled=self._lightsChecked[0],
                                      FillLightEnabled=self._lightsChecked[1],
                                      BackLightEnabled=self._lightsChecked[2])
        if self._stageView:
            self._stageView.ambientLightOnly = checked
            self._stageView.keyLightEnabled = \
                not checked or self._lightsChecked[0]
            self._stageView.fillLightEnabled = \
                not checked or self._lightsChecked[1]
            self._stageView.backLightEnabled = \
                not checked or self._lightsChecked[2]
            self._stageView.update()
        
    def _onKeyLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._stageView.keyLightEnabled = checked
            self._stageView.update()
            self._settings.setAndSave(KeyLightEnabled=checked)
            
    def _onFillLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._stageView.fillLightEnabled = checked
            self._stageView.update()
            self._settings.setAndSave(FillLightEnabled=checked)
            
    def _onBackLightClicked(self, checked=None):
        if self._stageView and checked is not None:
            self._stageView.backLightEnabled = checked    
            self._stageView.update()
            self._settings.setAndSave(BackLightEnabled=checked)

    def _changeBgColor(self, mode):
        self._stageView.clearColor = self._colorsDict[str(mode.text())]
        self._settings.setAndSave(ClearColor=str(mode.text()))
        self._stageView.update()

    def _showBBoxPlayback(self, state):
        """Called when the menu item for showing BBoxes
        during playback is activated or deactivated."""
        self._settings.setAndSave(ShowBBoxPlayback=state)

    def _setUseExtentsHint(self, state):
        self._bboxCache = UsdGeom.BBoxCache(self._currentFrame, 
                                            stageView.BBOXPURPOSES, 
                                            useExtentsHint=state)

        if self._stageView:
            self._stageView.bboxCache = self._bboxCache
            self._updateAttributeView()

        #recompute and display bbox
        self._refreshBBox()

    def _showBBoxes(self, state):
        """Called when the menu item for showing BBoxes
        is activated."""
        self._settings.setAndSave(ShowBBoxes=state)
        self._stageView.showBBoxes = state
        #recompute and display bbox
        self._refreshBBox()

    def _showAABBox(self, state):
        """Called when Axis-Aligned bounding boxes
        are activated/deactivated via menu item"""
        self._settings.setAndSave(ShowAABBox=state)
        self._stageView.showAABBox = state
        # recompute and display bbox
        self._refreshBBox()

    def _showOBBox(self, state):
        """Called when Oriented bounding boxes
        are activated/deactivated via menu item"""
        self._settings.setAndSave(ShowOBBox=state)
        self._stageView.showOBBox = state
        # recompute and display bbox
        self._refreshBBox()

    def _refreshBBox(self):
        """Recompute and hide/show Bounding Box."""
        if not self._stageView:
            return
        self._stageView.setNodes(self._currentNodes,
                                self._currentFrame,
                                forceComputeBBox=True)

    def _toggleDisplayGuide(self, checked):
        self._settings.setAndSave(DisplayGuide=checked)
        self._stageView.setDisplayGuide(checked)
        self._stageView.setNodes(self._prunedCurrentNodes, self._currentFrame)
        self._updateAttributeView()
        self._stageView.update()

    def _toggleDisplayProxy(self, checked):
        self._settings.setAndSave(DisplayProxy=checked)
        self._stageView.setDisplayProxy(checked)
        self._stageView.setNodes(self._prunedCurrentNodes, self._currentFrame)
        self._updateAttributeView()
        self._stageView.update()

    def _toggleDisplayRender(self, checked):
        self._settings.setAndSave(DisplayRender=checked)
        self._stageView.setDisplayRender(checked)
        self._stageView.setNodes(self._prunedCurrentNodes, self._currentFrame)
        self._updateAttributeView()
        self._stageView.update()

    def _toggleDisplayCameraOracles(self, checked):
        self._settings.setAndSave(DisplayCameraGuides=checked)
        self._stageView.setDisplayCameraOracles(checked)
        self._stageView.update()

    def _toggleDisplayPrimId(self, checked):
        self._settings.setAndSave(DisplayPrimId=checked)
        self._stageView.setDisplayPrimId(checked)
        self._stageView.update()

    def _toggleEnableHardwareShading(self, checked):
        self._settings.setAndSave(EnableHardwareShading=checked)
        self._stageView.setEnableHardwareShading(checked)
        self._stageView.update()

    def _toggleCullBackfaces(self, checked):
        self._settings.setAndSave(CullBackfaces=checked)
        self._stageView.setCullBackfaces(checked)
        self._stageView.update()

    def _showInterpreter(self):
        from pythonExpressionPrompt import Myconsole
            
        if self._interpreter is None:
            self._interpreter = QtGui.QDialog(self)
            self._console = Myconsole(self._interpreter)
            lay = QtGui.QVBoxLayout()
            lay.addWidget(self._console)
            self._interpreter.setLayout(lay)

        # dock the interpreter window next to the main usdview window
        self._interpreter.move(self.x() + self.frameGeometry().width(),
                               self.y())
        self._interpreter.resize(600, self.size().height()/2)
        
        self._updateInterpreter()
        self._interpreter.show() 
        self._interpreter.activateWindow() 
        self._console.setFocus() 

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
        # contents, and well need to compose it separately.
        windowShot = QtGui.QImage(self.size(), 
                                  QtGui.QImage.Format_ARGB32_Premultiplied)
        painter = QtGui.QPainter(windowShot)
        self.render(painter, QtCore.QPoint())

        # overlay the QGLWidget on and return the composed image
        # we offset by a single point here because of Qt.Pos funkyness
        offset = QtCore.QPoint(0,1)
        pos = self._stageView.mapTo(self, self._stageView.pos()) - offset
        painter.drawImage(pos, self.GrabViewportShot())    
        return windowShot 

    def GrabViewportShot(self):
        '''Returns a QImage of the current stage view in usdview.'''
        return self._stageView.grabFrameBuffer() 
        
    # File handling functionality =============================================

    def closeEvent(self, event):
        """The window is closing, either by clicking exit or by clicking the "x" 
        provided by the window manager.
        """
        self._cleanAndClose()

    def _cleanAndClose(self):
        self._settings.setAndSave(nodeViewHeader = \
                [self._ui.nodeView.isColumnHidden(c) \
                    for c in range(self._ui.nodeView.columnCount())])

        self._settings.setAndSave(propertyViewHeader = \
                [self._ui.propertyView.isColumnHidden(c) \
                    for c in range(self._ui.propertyView.columnCount())])

        # If the current path widget is focused when closing usdview, it can
        # trigger an "editingFinished()" signal, which will look for a prim in
        # the scene (which is already deleted). This prevents that.
        QtCore.QObject.disconnect(self._ui.currentPathWidget,
                                  QtCore.SIGNAL('editingFinished()'),
                                  self._currentPathChanged)

        # Shut down some timers.
        self._nodeViewUpdateTimer.stop()
        self._nodeViewResetTimer.stop()

        # If the timer is currently active, stop it from being invoked while 
        # the USD stage is being torn down. 
        if self._timer.isActive():
            self._timer.stop()

        # Turn off the imager before killing the stage
        if self._stageView:
            with Timer() as t:
                self._stageView.SetStage(None)
            if self._printTiming:
                t.PrintTime('shut down Hydra')

        # Close the USD stage.
        with Timer() as t:
            self._stage.Close()
        if self._printTiming:
            t.PrintTime('close UsdStage')

        # Tear down the UI window.
        with Timer() as t:
            self.close()
        if self._printTiming:
            t.PrintTime('tear down the UI')

    def _openFile(self):
        (filename, _) = QtGui.QFileDialog.getOpenFileName(self, "Select file",".")
        if len(filename) > 0:

            self._parserData.usdFile = str(filename)
            self._reopenStage()

            self.setWindowTitle(filename)

    def _saveOverridesAs(self):
        recommendedFilename = self._parserData.usdFile.rsplit('.', 1)[0]
        recommendedFilename += '_overrides.usd'
        (saveName, _) = QtGui.QFileDialog.getSaveFileName(self,
                                                     "Save file (*.usd)",
                                                     "./" + recommendedFilename,
                                                     'Usd Files (*.usd)')
        if len(saveName) <= 0:
            return

        if (saveName.rsplit('.')[-1] != 'usd'):
            saveName += '.usd'
            
        if self._stage:
            # In the future, we may allow usdview to be brought up with no file, 
            # in which case it would create an in-memory root layer, to which 
            # all edits will be targeted.  In order to future proof 
            # this, first fetch the root layer, and if it is anonymous, just 
            # export it to the given filename. If it isn't anonmyous (i.e., it 
            # is a regular usd file on disk), export the session layer and add 
            # the stage root file as a sublayer.
            rootLayer = self._stage.GetRootLayer()
            if not rootLayer.anonymous:
                self._stage.GetSessionLayer().Export(saveName, 'Created by UsdView')
                targetLayer = Sdf.Layer.FindOrOpen(saveName)
                UsdUtils.CopyLayerMetadata(rootLayer, targetLayer,
                                           skipSublayers=True)
                
                # We don't ever store self.realStartTimeCode or 
                # self.realEndTimeCode in a layer, so we need to author them
                # here explicitly.
                targetLayer.startTimeCode = self.realStartTimeCode
                targetLayer.endTimeCode = self.realEndTimeCode

                targetLayer.subLayerPaths.append(self._stage.GetRootLayer().realPath)
                targetLayer.RemoveInertSceneDescription()
                targetLayer.Save()
            else:
                self._stage.GetRootLayer().Export(saveName, 'Created by UsdView')

    def _reopenStage(self):
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

        try:
            # Clear out any Usd objects that may become invalid. We will pick
            # these back up in _refreshVars(), called below.
            self._currentNodes = []
            self._currentProp = None
            self._currentSpec = None
            self._currentLayer = None

            self._stage.Close()
            self._stage = self._openStage(self._parserData.usdFile,
                                          self._parserData.populationMask)
            # We need this for layers which were cached in memory but changed on
            # disk. The additional Reload call should be cheap when nothing
            # actually changed.
            self._stage.Reload()

            self._resetSettings()
            self._resetView()
            self._refreshVars()

            self._stepSizeChanged()
            self._stepSizeChanged()
        except Exception as err:
            self.statusMessage('Error occurred reopening Stage: %s' % err)
            import traceback
            traceback.print_exc()
        finally:
            QtGui.QApplication.restoreOverrideCursor()

        self.statusMessage('Stage Reopened')

    def _reloadStage(self):
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)

        try:
            self._stage.Reload()
            self._resetSettings() # reload topology, attributes, GL view
            self._resetView()
        except Exception as err:
            self.statusMessage('Error occurred rereading all layers for Stage: %s' % err)
        finally:
            QtGui.QApplication.restoreOverrideCursor()

        self.statusMessage('All Layers Reloaded.')

    def _cameraSelectionChanged(self, camera):
        # Because the camera menu can be torn off, we need
        # to update its check-state whenever the selection changes
        for action in self._ui.menuCamera.actions():
            action.setChecked(action._node == camera)
        self._stageView.setCameraPrim(camera)
        self._stageView.updateGL()

    def _refreshCameraListAndMenu(self, preserveCurrCamera):
        self._allSceneCameras = Utils._GetAllPrimsOfType(self._stage,
                                                 Tf.Type.Find(UsdGeom.Camera))
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
            for camera in self._allSceneCameras:
                # XXX This logic needs to be de-pixarified.  Perhaps
                # a "primaryCamera" attribute on UsdGeomCamera?
                if camera.GetName() == "main_cam":
                    self._startingPrimCamera = currCamera = camera
                    if self._stageView:
                        self._stageView.setCameraPrim(camera)
                    break
        # Now that we have the current camera and all cameras, build the menu
        self._ui.menuCamera.clear()
        for camera in self._allSceneCameras:
            action = self._ui.menuCamera.addAction(camera.GetName())
            action.setCheckable(True)
            action._node = camera
            
            QtCore.QObject.connect(
                action,
                QtCore.SIGNAL('triggered()'),
                lambda camera = camera: self._cameraSelectionChanged(camera))
            action.setChecked(action._node == currCamera)

    # ===================================================================
    # ==================== Attribute Inspector ==========================
    def _populateAttributeInspector(self, currentItem = None, previtem = None):
        self._currentProp = self._getSelectedObject(currentItem)
        if isinstance(self._currentProp, Usd.Prim):
            self._currentProp = None
        if self._console:
            self._console.reloadConsole(self)

        if currentItem is not None:
            itemName = str(self._ui.propertyView.item(currentItem.row(), 0).text())

            # inform the value editor that we selected a new attribute
            self._ui.attributeValueEditor.populate(itemName, self._currentNodes[0])
        else:
            self._ui.attributeValueEditor.clear()

    def _onCompositionSelectionChanged(self, curr=None, prev=None):
        self._currentSpec = getattr(curr, 'spec', None)
        self._currentLayer = getattr(curr, 'layer', None)
        if self._console:
            self._console.reloadConsole(self)

    def _updateAttributeInspector(self, index=None, obj=None,
                                  updateAttributeView=True):
        # index must be the first parameter since this method is used as
        # attributeInspector tab widget's currentChanged(int) signal callback
        if index is None:
            index = self._ui.attributeInspector.currentIndex()
        else:
            self._settings.setAndSave(AttributeInspectorCurrentTab=index)

        if obj is None:
            obj = self._getSelectedObject()

        if index == INDEX_VALUE or updateAttributeView:
            self._updateAttributeView()

        if index == INDEX_METADATA:
            self._updateMetadataView(obj)
        elif index == INDEX_LAYERSTACK:
            self._updateLayerStackView(obj)
        elif index == INDEX_COMPOSITION:
            self._updateCompositionView(obj)

    def _refreshAttributeValue(self):
        self._ui.attributeValueEditor.refresh()

    def _propertyViewContextMenu(self, point):
        item = self._ui.propertyView.itemAt(point)
        self.contextMenu = AttributeViewContextMenu(self, item)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _layerStackContextMenu(self, point):
        item = self._ui.layerStackView.itemAt(point)
        self.contextMenu = LayerStackContextMenu(self, item)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _compositionTreeContextMenu(self, point):
        item = self._ui.compositionTreeWidget.itemAt(point)
        self.contextMenu = LayerStackContextMenu(self, item)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    # Headers & Columns =================================================
    def _propertyViewHeaderContextMenu(self, point):
        self.contextMenu = HeaderContextMenu(self._ui.propertyView)
        self.contextMenu.exec_(QtGui.QCursor.pos())

    def _nodeViewHeaderContextMenu(self, point):
        self.contextMenu = HeaderContextMenu(self._ui.nodeView)
        self.contextMenu.exec_(QtGui.QCursor.pos())


    # Widget management =================================================

    def _changeNodeViewDepth(self, action):
        """Signal handler for view-depth menu items
        """
        actionTxt = str(action.text())
        # recover the depth factor from the action's name
        depth = int(actionTxt[actionTxt.find(" ")+1])
        self._expandToDepth(depth)

    def _expandToDepth(self, depth, suppressTiming=False):
        """Expands treeview nodes to the given depth
        """
        with Timer() as t, BusyContext():
            # Populate items down to depth.  Qt will expand items at depth
            # depth-1 so we need to have items at depth.  We know something
            # changed if any items were added to _itemsToPush.
            n = len(self._itemsToPush)
            self._populateItem(self._stage.GetPseudoRoot(), maxDepth=depth)
            changed = (n != len(self._itemsToPush))

            # Expand the tree to depth.
            self._ui.nodeView.expandToDepth(depth-1)
            if changed:
                # Resize column.
                self._ui.nodeView.resizeColumnToContents(0)

                # Start pushing prim data to the UI during idle cycles.
                # Qt doesn't need the data unless the item is actually
                # visible (or affects what's visible) but to avoid
                # jerky scrolling when that data is pulled during the
                # scroll, we can do it ahead of time.  But don't do it
                # if we're currently playing to maximize playback
                # performance.
                if not self._stageView or not self._stageView.playing:
                    self._nodeViewUpdateTimer.start()

        if self._printTiming and not suppressTiming:
            t.PrintTime("expand Prim browser to depth %d" % depth)

    def _nodeViewExpanded(self, index):
        """Signal handler for expanded(index), facilitates lazy tree population
        """
        self._populateChildren(self._ui.nodeView.itemFromIndex(index))
        self._ui.nodeView.resizeColumnToContents(0)

    def _toggleShowInactiveNodes(self):
        self._settings.setAndSave(actionShow_Inactive_Nodes = 
                self._ui.actionShow_Inactive_Nodes.isChecked())
        self._resetNodeView()

    def _toggleShowMasterPrims(self):
        self._settings.setAndSave(actionShow_Master_Prims = 
                self._ui.actionShow_Master_Prims.isChecked())
        self._resetNodeView()

    def _toggleShowUndefinedPrims(self):
        self._settings.setAndSave(actionShow_Undefined_Prims=
                self._ui.actionShow_Undefined_Prims.isChecked())
        self._resetNodeView()

    def _toggleShowAbstractPrims(self):
        self._settings.setAndSave(actionShow_Abstract_Prims=
                self._ui.actionShow_Abstract_Prims.isChecked())
        self._resetNodeView()

    def _toggleRolloverPrimInfo(self):
        self._settings.setAndSave(actionRollover_Prim_Info=
                self._ui.actionRollover_Prim_Info.isChecked())
        if self._stageView:
            self._stageView.rolloverPicking = \
                self._settings.get("actionRollover_Prim_Info", False)

    def _tallyNodeStats(self, prim):
        def _GetType(prim):
            typeString = prim.GetTypeName()
            return NOTYPE if not typeString else typeString

        childTypeDict = {} 
        primCount = 0

        for child in Usd.TreeIterator(prim):
            typeString = _GetType(child)
            # skip pseudoroot
            if typeString is NOTYPE and not prim.GetParent():
                continue
            primCount += 1
            childTypeDict[typeString] = 1 + childTypeDict.get(typeString, 0)

        return (primCount, childTypeDict)

    def _populateChildren(self, item, depth=0, maxDepth=1, childrenToAdd=None):
        """Populates the children of the given item in the node viewer.
           If childrenToAdd is given its a list of nodes to add as
           children."""
        if depth < maxDepth and item.node.IsActive():
            if item.needsChildrenPopulated() or childrenToAdd:
                # Populate all the children.
                if not childrenToAdd:
                    childrenToAdd = self._getFilteredChildren(item.node)
                item.addChildren([self._populateItem(child, depth+1, maxDepth)
                                                    for child in childrenToAdd])
            elif depth + 1 < maxDepth:
                # The children already exist but we're recursing deeper.
                for i in xrange(item.childCount()):
                    self._populateChildren(item.child(i), depth+1, maxDepth)

    def _populateItem(self, node, depth=0, maxDepth=0):
        """Populates a node viewer item."""
        item = self._nodeToItemMap.get(node)
        if not item:
            # Create a new item.  If we want its children we obviously
            # have to create those too.
            children = self._getFilteredChildren(node)
            item = NodeViewItem(node, self, len(children) != 0)
            self._nodeToItemMap[node] = item
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
        showInactive = self._ui.actionShow_Inactive_Nodes.isChecked()
        showUndefined = self._ui.actionShow_Undefined_Prims.isChecked()
        showAbstract = self._ui.actionShow_Abstract_Prims.isChecked()
        showMasters = self._ui.actionShow_Master_Prims.isChecked()
        
        return ((prim.IsActive() or showInactive) and
                (prim.IsDefined() or showUndefined) and
                (not prim.IsAbstract() or showAbstract) and
                (not prim.IsInMaster() or showMasters))

    def _populateRoots(self):
        invisibleRootItem = self._ui.nodeView.invisibleRootItem()
        rootNode = self._stage.GetPseudoRoot()
        rootItem = self._populateItem(rootNode)
        self._populateChildren(rootItem)

        showMasters = self._ui.actionShow_Master_Prims.isChecked()
        if showMasters:
            self._populateChildren(rootItem,
                                   childrenToAdd=self._stage.GetMasters())

        # Add all descendents all at once.
        invisibleRootItem.addChild(rootItem)

    def _getFilteredChildren(self, prim):
        return prim.GetFilteredChildren(self._displayPredicate)

    def _computeDisplayPredicate(self):
        # Take current browser filtering into account when discovering
        # prims while traversing
        showInactive = self._ui.actionShow_Inactive_Nodes.isChecked()
        showUndefined = self._ui.actionShow_Undefined_Prims.isChecked()
        showAbstract = self._ui.actionShow_Abstract_Prims.isChecked()
        showMasters = self._ui.actionShow_Master_Prims.isChecked()

        self._displayPredicate = None

        if not showInactive:
            self._displayPredicate = Usd.PrimIsActive \
                if self._displayPredicate is None \
                else self._displayPredicate & Usd.PrimIsActive
        if not showUndefined:
            self._displayPredicate = Usd.PrimIsDefined \
                if self._displayPredicate is None \
                else self._displayPredicate & Usd.PrimIsDefined
        if not showAbstract:
            self._displayPredicate = ~Usd.PrimIsAbstract \
                if self._displayPredicate is None \
                else self._displayPredicate & ~Usd.PrimIsAbstract
        if self._displayPredicate is None:
            self._displayPredicate = Usd._PrimFlagsPredicate.Tautology()


    def _getItemAtPath(self, path, ensureExpanded=False):
        # If the node hasn't been expanded yet, drill down into it.
        # Note the explicit str(path) in the following expr is necessary
        # because path may be a QString.
        path = path if isinstance(path, Sdf.Path) else Sdf.Path(str(path))
        parent = self._stage.GetPrimAtPath(path)
        if not parent:
            raise RuntimeError("Prim not found at path in stage: %s" % str(path))
        pseudoRoot = self._stage.GetPseudoRoot()
        if parent not in self._nodeToItemMap:
            # find the first loaded parent
            childList = []

            while parent != pseudoRoot \
                        and not parent in self._nodeToItemMap:
                childList.append(parent)
                parent = parent.GetParent() 

            # go one step further, since the first item found could be hidden 
            # under a norgie and we would want to populate its siblings as well
            if parent != pseudoRoot:
                childList.append(parent)

            # now populate down to the child
            for parent in reversed(childList):
                item = self._nodeToItemMap[parent]
                self._populateChildren(item)
                if ensureExpanded:
                    item.setExpanded(True)

        # finally, return the requested item, which now must be in the map
        return self._nodeToItemMap[self._stage.GetPrimAtPath(path)]

    def resetSelectionToPseudoroot(self):
        self.selectNodeByPath("/", UsdImagingGL.GL.ALL_INSTANCES, "replace")

    def selectNodeByPath(self, path, instanceIndex, updateMode, 
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
            path = self._stage.GetPseudoRoot().GetPath()
        
        # If model picking on, find model and select instead, IFF we are
        # requested to apply picking modes
        if applyPickMode and self._ui.actionPick_Models.isChecked():
            from common import GetEnclosingModelPrim
            
            prim = self._stage.GetPrimAtPath(str(path))
            model = prim if prim.IsModel() else GetEnclosingModelPrim(prim)
            if model:
                path = model.GetPath()

        # If not in instances picking mode, select all instances.
        if not (applyPickMode and self._ui.actionPick_Instances.isChecked()):
            self._stageView.clearInstanceSelection()
            instanceIndex = UsdImagingGL.GL.ALL_INSTANCES

        item = self._getItemAtPath(path, ensureExpanded=True)

        if updateMode == "replace":
            self._stageView.clearInstanceSelection()
            self._stageView.setInstanceSelection(path, instanceIndex, True)
            self._ui.nodeView.setCurrentItem(item)
        elif updateMode == "add":
            self._stageView.setInstanceSelection(path, instanceIndex, True)
            item.setSelected(True)
        else:   # "toggle"
            if instanceIndex != UsdImagingGL.GL.ALL_INSTANCES:
                self._stageView.setInstanceSelection(path, instanceIndex,
                    not self._stageView.getInstanceSelection(path, instanceIndex))
                # if no instances selected, unselect item
                if len(self._stageView.getSelectedInstanceIndices(path)) == 0:
                    item.setSelected(False)
                else:
                    item.setSelected(True)
            else:
                self._stageView.clearInstanceSelection()
                item.setSelected(not item.isSelected())
            # if nothing selected, select root.
            if len(self._ui.nodeView.selectedItems()) == 0:
                item = self._getItemAtPath(self._stage.GetPseudoRoot().GetPath())
                item.setSelected(True)

        if instanceIndex != UsdImagingGL.GL.ALL_INSTANCES:
            self._itemSelectionChanged()

        return item

    def _updatePrimTreeForVariantSwitch(self):
        self._clearCaches()

        for prim in self._currentNodes:
            if prim:
                self._clearGeomCountsForPrimPath(prim.GetPath())

        self._resetNodeView()

        # Finally redraw the bbox and transitively the scene.
        self._refreshBBox()

    def _getCommonNodes(self, pathsList):
        import os

        commonPrefix = os.path.commonprefix(pathsList)
        ### To prevent /Canopies/TwigA and /Canopies/TwigB
        ### from registering /Canopies/Twig as prefix
        return commonPrefix.rsplit('/', 1)[0]

    def _getAttributeNode(self):
        return self._stage.GetPrimAtPath(self._currentNodes[0].GetPath())

    def _currentPathChanged(self):
        """Called when the currentPathWidget text is changed"""
        import re
        newPaths = self._ui.currentPathWidget.text()
        pathList = re.split(", ?", newPaths)
        pathList = filter(lambda path: len(path) != 0, pathList)

        itemList = []
        for primPath in pathList:
            try:
                treeItem = self._getItemAtPath(str(primPath))
            except Exception, err:  
                # prim not found
                sys.stderr.write('ERROR: %s\n' % str(err))
                self._itemSelectionChanged()
                return

            itemList.append(treeItem)

        primList = [item.node for item in itemList]
        self._setSelectionFromPrimList(primList)

    def _setSelectionFromPrimList(self, primsToSelect):
        """Replaces current selection with the prims in 'primsToSelect'.
        Each member is first tested to make sure the prim is valid, and
        passes the current display filters.  Prims that do not pass are
        silently ignored"""
        # We are making many mutations to the PrimView's selection state.
        # We only want to update once in response, so temporarily disable
        # signals from the TreeWidget and manually sync selection after
        with MainWindow.UpdateBlocker(self):
            self._ui.nodeView.clearSelection()
            first = True
            for prim in primsToSelect:
                if self._primShouldBeShown(prim):
                    instanceIndex = UsdImagingGL.GL.ALL_INSTANCES
                    item = self.selectNodeByPath(prim.GetPath(), instanceIndex,
                                                 "replace" if first else "add")
                    first = False
                    # selectNodeByPath expands all of item's parents,
                    # but that doesn't seem to work if you have manually closed
                    # one of its ancestor's noorgies.  This will ensure all
                    # selected items are visible.
                    self._ui.nodeView.scrollToItem(item)
        # Now resync _currentNodes et al to the new PrimView 
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
        selectedItems = self._ui.nodeView.selectedItems()
        if len(selectedItems) <= 0:
            return

        # get nodes, but do not include nodes whose parents are selected too.
        prunedPaths = self._getPathsFromItems(selectedItems, True)
        self._prunedCurrentNodes = [self._stage.GetPrimAtPath(pth) for pth in prunedPaths]
        # get all nodes selected
        paths = self._getPathsFromItems(selectedItems, False)
        self._currentNodes = [self._stage.GetPrimAtPath(pth) for pth in paths]

        self._ui.currentPathWidget.setText(', '.join([str(p) for p in paths]))
                         
        if not self._noRender:
            # update the entire upper HUD with fresh information
            # this includes geom counts (slow)
            self._updateHUDNodeStats()
            self._updateHUDGeomCounts()
            # recompute bbox on node change
            self._stageView.setNodes(self._prunedCurrentNodes, self._currentFrame,
                                     resetCam=False, forceComputeBBox=True) 

        self._updateAttributeInspector(obj=self._getSelectedPrim(),
                                       updateAttributeView=True)

        self._refreshAttributeValue()
        self._updateInterpreter()

    def _itemClicked(self, item, col):
        # onClick() returns True if the click caused a state change (currently
        # this will only be a change to visibility).
        if item.onClick(col):
            self.editComplete('Updated node visibility')
            with Timer() as t:
                NodeViewItem.propagateVis(item)
            if self._printTiming:
                t.PrintTime("update vis column")
        self._updateAttributeInspector(obj=self._getSelectedPrim(),
                                       updateAttributeView=False)

    def _propertyViewItemClicked(self, item, col):
        self._updateAttributeInspector(obj=self._getSelectedObject(),
                                       updateAttributeView=False)

    def _getPathsFromItems(self, items, prune = False):
        # this function returns a list of paths given a list of items if
        # prune=True, it excludes certain paths if a parent path is already
        # there this avoids double-rendering if both a prim and its parent
        # are selected.
        #
        # Don't include the pseudoroot, though, if it's still selected, because
        # leaving it in the pruned list will cause everything else to get
        # pruned away!
        allPaths = [itm.node.GetPath() for itm in items]
        if not prune:
            return allPaths
        if len(allPaths) > 1:
            allPaths = [p for p in allPaths if p != Sdf.Path.absoluteRootPath]
        return Sdf.Path.RemoveDescendentPaths(allPaths)
        
    def _nodeViewContextMenu(self, point):
        item = self._ui.nodeView.itemAt(point)
        self._showNodeContextMenu(item)

    def _showNodeContextMenu(self, item):
        self.contextMenu = NodeContextMenu(self, item)
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

        self._ui.frameField.setText(str(round(self._currentFrame,2)))

        if self._currentFrame != frameAtStart or forceUpdate:
            # do not update HUD/BBOX if scrubbing or playing
            updateUI = forceUpdate or not (self._ui.playButton.isChecked() or 
                                          self._ui.frameSlider.isSliderDown())
            self._updateOnFrameChange(updateUI)

    def _updateOnFrameChange(self, refreshUI = True):
        """Called when the frame changed, updates the renderer and such"""
        # set the xformCache and bboxCache's time to the new time
        self._xformCache.SetTime(self._currentFrame)
        self._bboxCache.SetTime(self._currentFrame)
        
        playing = self._ui.playButton.isChecked() 

        # grey out the HUD when playing at interactive rates (its disabled)
        self._stageView.showBBoxes = \
                         self._ui.showBBoxes.isChecked() and \
                        (not playing or self._ui.showBBoxPlayback.isChecked())

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
            self._resetNodeViewVis(selItemsOnly=False, authoredVisHasChanged=False)

        # this is the part that renders
        if playing:
            self._stageView.updateForPlayback(self._currentFrame,
                                             self._selHighlightMode == "Always")
        else:
            self._stageView.setNodes(self._currentNodes, self._currentFrame)

    def setHUDVisible(self, hudVisible):
        self._ui.actionHUD.setChecked(False)

    def saveFrame(self, fileName):
        pm =  QtGui.QPixmap.grabWindow(self._stageView.winId())
        pm.save(fileName, 'TIFF')

    def _getAttributeDict(self):
        attributeDict = OrderedDict()

        # leave attribute viewer empty if multiple nodes selected
        if len(self._currentNodes) != 1:
            return attributeDict

        prim = self._currentNodes[0]

        customAttrs = _GetCustomAttributes(prim, self._bboxCache, self._xformCache)
        attrs = prim.GetAttributes()
                        
        for cAttr in customAttrs:
            attributeDict[cAttr.GetName()] = cAttr

        for attr in attrs:
            attributeDict[attr.GetName()] = attr

        return attributeDict

    def _updateAttributeViewInternal(self):
        frame = self._currentFrame
        tableWidget = self._ui.propertyView

        previousSelection = tableWidget.selectedItems()
        prevSelectedAttributeNames = set()
        for i in previousSelection:
            prevSelectedAttributeNames.add(
                str(tableWidget.item(i.row(),0).text()))

        # get a dictionary of prim attribs/members and store it in self._attributeDict
        self._attributeDict = self._getAttributeDict()

        # Setup table widget
        tableWidget.clearContents()
        tableWidget.setRowCount(len(self._attributeDict))

        self._populateAttributeInspector()

        attributeCount = 0
        for key, attribute in self._attributeDict.iteritems():
            # Get the attribute's value and display color
            fgColor = GetAttributeColor(attribute, frame)

            attrName = QtGui.QTableWidgetItem(str(key))
            attrName.setFont(BoldFont)
            attrName.setForeground(fgColor)
            tableWidget.setItem(attributeCount, 0, attrName)

            attrText = _GetShortString(attribute, frame)
            attrVal = QtGui.QTableWidgetItem(attrText)
            valTextFont = GetAttributeTextFont(attribute, frame)
            if valTextFont:
                attrVal.setFont(valTextFont)
                attrName.setFont(valTextFont)
            attrVal.setForeground(fgColor)
            tableWidget.setItem(attributeCount, 1, attrVal)

            # Need reference to original value for pretty-print on double-click
            if (key in prevSelectedAttributeNames):
                tableWidget.item(attributeCount,0).setSelected(True)
                tableWidget.setCurrentItem(tableWidget.item(attributeCount, 0))

            attributeCount += 1

        tableWidget.resizeColumnToContents(0)

    def _updateAttributeView(self):
        """ Sets the contents of the attribute value viewer """
        cursorOverride = not self._timer.isActive()
        if cursorOverride:
            QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
        try:
            self._updateAttributeViewInternal()
        except Exception as err:
            print "Problem encountered updating attribute view: %s" % err
        finally:
            if cursorOverride:
                QtGui.QApplication.restoreOverrideCursor()

    def _getSelectedObject(self, selectedAttribute=None):
        if selectedAttribute is None:
            attrs = self._ui.propertyView.selectedItems()
            if len(attrs) > 0:
                selectedAttribute = attrs[0]

        if selectedAttribute:
            attrName = str(self._ui.propertyView.item(selectedAttribute.row(),0).text())
            if attrName.startswith("[Relationship]"):
                attrName = attrName[len("[Relationship] "):]
                obj = self._currentNodes[0].GetRelationship(attrName)
            else:
                obj = self._currentNodes[0].GetAttribute(attrName)

            return obj

        return self._getSelectedPrim()

    def _getSelectedPrim(self):
        return self._currentNodes[0] if self._currentNodes else None

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
        from itertools import groupby
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
                combo = VariantComboBox(None, obj, variantSetName, self)
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
            attrName = QtGui.QTableWidgetItem(str(key))
            tableWidget.setItem(i, 0, attrName)

            # Get metadata value
            if key == "customData":
                val = obj.GetCustomData()
            else:
                val = m[key]

            valStr, ttStr = self._formatMetadataValueView(val) 
            attrVal = QtGui.QTableWidgetItem(valStr)
            attrVal.setToolTip(ttStr)

            tableWidget.setItem(i, 1, attrVal)

        rowIndex = len(m)
        for variantSetName, combo in variantSets.iteritems():
            attrName = QtGui.QTableWidgetItem(str(variantSetName+ ' variant'))
            tableWidget.setItem(rowIndex, 0, attrName)
            tableWidget.setCellWidget(rowIndex, 1, combo)
            combo.currentIndexChanged.connect(combo.updateVariantSelection)
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
            item = QtGui.QTreeWidgetItem(
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

            # attributes for LayerStackContextMenu:
            if layer.realPath:
                item.layerPath = layer.realPath
            if spec:
                item.path = node.path
            
            item.setExpanded(True)
            item.setToolTip(0, layer.identifier)
            if not spec:
                for i in range(item.columnCount()):
                    item.setForeground(i, NoValueTextColor)
            for subtree in layerTree.childTrees:
                WalkSublayers(item, node, subtree, True)
            return item

        # Create treeview items for all nodes in the composition index.
        def WalkNodes(parent, node):
            nodeItem = WalkSublayers(parent, node, node.layerStack.layerTree)
            for child in node.children:
                WalkNodes(nodeItem, child)

        path = obj.GetPath().GetAbsoluteRootOrPrimPath()
        prim = self._stage.GetPrimAtPath(path)
        if not prim:
            return

        # Populate the treeview with items from the prim index.
        index = prim.GetPrimIndex()
        WalkNodes(treeWidget, index.rootNode)


    def _updateLayerStackView(self, obj=None):
        """ Sets the contents of the layer stack viewer"""

        from pxr import Sdf
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
            from common import GetRootLayerStackInfo
            layers = GetRootLayerStackInfo(self._stage.GetRootLayer())
            tableWidget.setColumnCount(2)
            tableWidget.horizontalHeaderItem(1).setText('Layer Offset')
            
            tableWidget.setRowCount(len(layers))
            
            for i, layer in enumerate(layers):
                layerItem = QtGui.QTableWidgetItem(layer.GetHierarchicalDisplayString())
                layerItem.layerPath = layer.layer.realPath
                toolTip = "<b>identifier:</b> @%s@ <br> <b>resolved path:</b> %s" % \
                    (layer.layer.identifier, layerItem.layerPath)
                toolTip = self._limitToolTipSize(toolTip)
                layerItem.setToolTip(toolTip)
                tableWidget.setItem(i, 0, layerItem)

                offsetItem = QtGui.QTableWidgetItem(layer.GetOffsetString())
                offsetItem.layerPath = layer.layer.realPath
                toolTip = self._limitToolTipSize(str(layer.offset)) 
                offsetItem.setToolTip(toolTip)
                tableWidget.setItem(i, 1, offsetItem)
                
            tableWidget.resizeColumnToContents(0)
        else:
            specs = []
            tableWidget.setColumnCount(3)
            header = tableWidget.horizontalHeader()
            header.setResizeMode(0, QtGui.QHeaderView.ResizeToContents)
            header.setResizeMode(1, QtGui.QHeaderView.Stretch)
            header.setResizeMode(2, QtGui.QHeaderView.ResizeToContents)
            tableWidget.horizontalHeaderItem(1).setText('Path')

            if path.IsPropertyPath():
                prop = obj.GetPrim().GetProperty(path.name)
                frameTime = (self._currentFrame if self._currentFrame 
                                                else Usd.TimeCode.Default())
                specs = prop.GetPropertyStack(frameTime)
                c3 = "Value" if (len(specs) == 0 or 
                                 isinstance(specs[0], Sdf.AttributeSpec)) else "Target Paths"
                tableWidget.setHorizontalHeaderItem(2,
                                                    QtGui.QTableWidgetItem(c3))
            else:
                specs = obj.GetPrim().GetPrimStack()
                tableWidget.setHorizontalHeaderItem(2,
                    QtGui.QTableWidgetItem('Metadata'))

            tableWidget.setRowCount(len(specs))

            for i, spec in enumerate(specs):
                layerItem = QtGui.QTableWidgetItem(spec.layer.GetDisplayName())
                layerItem.setToolTip(self._limitToolTipSize(spec.layer.realPath))
                tableWidget.setItem(i, 0, layerItem)

                pathItem = QtGui.QTableWidgetItem(spec.path.pathString)
                pathItem.setToolTip(self._limitToolTipSize(spec.path.pathString))
                tableWidget.setItem(i, 1, pathItem)

                if path.IsPropertyPath():
                    valStr = _GetShortString(spec, self._currentFrame)
                    ttStr = valStr
                    valueItem = QtGui.QTableWidgetItem(valStr)
                    sampleBased = (spec.HasInfo('timeSamples') and
                        spec.layer.GetNumTimeSamplesForPath(path) != -1)
                    valueItemColor = (TimeSampleTextColor if 
                        sampleBased else DefaultTextColor)
                    valueItem.setForeground(valueItemColor)
                    valueItem.setToolTip(ttStr)

                else:
                    metadataKeys = spec.GetMetaDataInfoKeys()
                    metadataDict = {}
                    for mykey in metadataKeys:
                        if spec.HasInfo(mykey):
                            metadataDict[mykey] = spec.GetInfo(mykey)
                    valStr, ttStr = self._formatMetadataValueView(metadataDict)
                    valueItem = QtGui.QTableWidgetItem(valStr)
                    valueItem.setToolTip(ttStr)

                tableWidget.setItem(i, 2, valueItem)
                # Add the data the context menu needs
                for j in range(3):
                    item = tableWidget.item(i, j)
                    item.layerPath = spec.layer.realPath
                    item.path = spec.path.pathString

    def _isHUDVisible(self):
        """Checks if the upper HUD is visible by looking at the global HUD
        visibility menu as well as the 'Subtree Info' menu"""
        return self._ui.actionHUD.isChecked() and self._ui.actionHUD_Info.isChecked()
        
    def _updateHUDMenu(self):
        """updates the upper HUD with both prim info and geom counts
        this function is called by the UI when the HUD is hidden or shown"""
        if self._isHUDVisible():
            self._updateHUDNodeStats()
            self._updateHUDGeomCounts()

        self._HUDMenuChanged()

    def _HUDMenuChanged(self):
        """called when a HUD menu item has changed that does not require info refresh"""
        self._stageView.showHUD = self._ui.actionHUD.isChecked()
        self._stageView.showHUD_Info = self._ui.actionHUD_Info.isChecked()
        self._stageView.showHUD_Complexity = \
            self._ui.actionHUD_Complexity.isChecked()
        self._stageView.showHUD_Performance = \
            self._ui.actionHUD_Performance.isChecked()
        self._stageView.showHUD_GPUstats = \
            self._ui.actionHUD_GPUstats.isChecked()
        self._stageView.updateGL()

        self._settings.setAndSave(actionHUD=self._ui.actionHUD.isChecked())
        self._settings.setAndSave(actionHUD_Info=self._ui.actionHUD_Info.isChecked())
        self._settings.setAndSave(actionHUD_Complexity=\
                                        self._ui.actionHUD_Complexity.isChecked())
        self._settings.setAndSave(actionHUD_Performance=\
                                        self._ui.actionHUD_Performance.isChecked())
        self._settings.setAndSave(actionHUD_GPUstats=\
                                        self._ui.actionHUD_GPUstats.isChecked())
        
    def _getHUDStatKeys(self):
        ''' returns the keys of the HUD with PRIM and NOTYPE and the top and
         CV, VERT, and FACE at the bottom.'''
        keys = [k for k in self._upperHUDInfo.keys() if k not in (CV,VERT,FACE,PRIM,NOTYPE)]
        keys = [PRIM,NOTYPE] + keys + [CV,VERT,FACE]
        return keys

    def _updateHUDNodeStats(self):
        """update the upper HUD with the proper node information"""
        self._upperHUDInfo = dict()

        if self._isHUDVisible():
            currentPaths = [n.GetPath() for n in self._prunedCurrentNodes if n.IsActive()]

            for pth in currentPaths:
                count,types = self._tallyNodeStats(self._stage.GetPrimAtPath(pth))
                # no entry for Node counts? initilize it
                if not self._upperHUDInfo.has_key(PRIM):
                    self._upperHUDInfo[PRIM] = 0
                self._upperHUDInfo[PRIM] += count 
                
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
                     for n in self._prunedCurrentNodes]
        
        for key in (CV, VERT, FACE):
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
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.BusyCursor)
        try:
            thisDict = {CV: 0, VERT: 0, FACE: 0}

            if prim.IsA(UsdGeom.Curves):
                curves = UsdGeom.Curves(prim)
                vertexCounts = curves.GetCurveVertexCountsAttr().Get(frame)
                if vertexCounts is not None:
                    for count in vertexCounts:
                        thisDict[CV] += count

            elif prim.IsA(UsdGeom.Mesh):
                mesh = UsdGeom.Mesh(prim)
                faceVertexCount = mesh.GetFaceVertexCountsAttr().Get(frame)
                faceVertexIndices = mesh.GetFaceVertexIndicesAttr().Get(frame)
                if faceVertexCount is not None and faceVertexIndices is not None:
                    uniqueVerts = set(faceVertexIndices)
                    
                    thisDict[VERT] += len(uniqueVerts)
                    thisDict[FACE] += len(faceVertexCount)

            self._geomCounts[(prim,frame)] = thisDict

            for child in prim.GetChildren():
                childResult = self._getGeomCounts(child, frame)
            
                for key in (CV, VERT, FACE):
                    self._geomCounts[(prim,frame)][key] += childResult[key]
        except Exception as err:
            print "Error encountered while computing prim subtree HUD info: %s" % err
        finally:
            QtGui.QApplication.restoreOverrideCursor()


    def _setupDebugMenu(self):
        def __helper(debugType, menu):
            return lambda: self._createTfDebugMenu(menu, '{0}_'.format(debugType))

        for debugType in TF_DEBUG_MENU_ENTRIES:
            menu = self._ui.menuDebug.addMenu('{0} Flags'.format(debugType))
            QtCore.QObject.connect(menu,
                                   QtCore.SIGNAL('aboutToShow()'),
                                   __helper(debugType, menu))

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
            menu.connect(action,
                         QtCore.SIGNAL('triggered(bool)'),
                         __createTriggerLambda(flag, not isEnabled))

    def _updateEditNodeMenu(self):
        """Make the Edit Prim menu items enabled or disabled depending on the
        selected prim."""
        from common import HasSessionVis, GetEnclosingModelPrim, \
            GetPrimsLoadability, GetClosestBoundMaterial

        # Use the descendent-pruned selection set to avoid redundant
        # traversal of the stage to answer isLoaded...
        anyLoadable, unused = GetPrimsLoadability(self._prunedCurrentNodes)
        removeEnabled = False
        anyImageable = False
        anyModels = False
        anyBoundMaterials = False
        anyActive = False
        anyInactive = False
        anyInstances = False
        for prim in self._currentNodes:
            if prim.IsA(UsdGeom.Imageable):
                imageable = UsdGeom.Imageable(prim)
                anyImageable = anyImageable or bool(imageable)
                removeEnabled = removeEnabled or HasSessionVis(prim)
            anyModels = anyModels or GetEnclosingModelPrim(prim) is not None
            material, bound = GetClosestBoundMaterial(prim)
            anyBoundMaterials = anyBoundMaterials or material is not None
            anyInstances = anyInstances or prim.IsInstance()
            if prim.IsActive():
                anyActive = True
            else:
                anyInactive = True

        self._ui.actionJump_to_Model_Root.setEnabled(anyModels)
        self._ui.actionJump_to_Bound_Material.setEnabled(anyBoundMaterials)
        self._ui.actionJump_to_Master.setEnabled(anyInstances)

        self._ui.actionRemove_Session_Visibility.setEnabled(removeEnabled)
        self._ui.actionMake_Visible.setEnabled(anyImageable)
        self._ui.actionVis_Only.setEnabled(anyImageable)
        self._ui.actionMake_Invisible.setEnabled(anyImageable)
        self._ui.actionLoad.setEnabled(anyLoadable)
        self._ui.actionUnload.setEnabled(anyLoadable)
        self._ui.actionActivate.setEnabled(anyInactive)
        self._ui.actionDeactivate.setEnabled(anyActive)


    def getSelectedItems(self):
        return [self._nodeToItemMap[n] for n in self._currentNodes
                    if n in self._nodeToItemMap]

    def editSelectedReference(self):
        """ Invoke the reference editing dialog """
        prim = self._currentNodes[0]
        if not prim.IsReference():
            return

        editor = referenceEditor.ReferenceEditor(self, self._stage, prim)
        editor.exec_()

    def jumpToEnclosingModelSelectedPrims(self):
        from common import GetEnclosingModelPrim
        newSel = []
        added = set()
        # We don't expect this to take long, so no BusyContext
        for prim in self._currentNodes:
            model = GetEnclosingModelPrim(prim)
            prim = model or prim
            if not (prim in added):
                added.add(prim)
                newSel.append(prim)
        self._setSelectionFromPrimList(newSel)

    def jumpToBoundMaterialSelectedPrims(self):
        from common import GetClosestBoundMaterial
        newSel = []
        added = set()
        # We don't expect this to take long, so no BusyContext
        for prim in self._currentNodes:
            material, bound = GetClosestBoundMaterial(prim)
            if not (material in added):
                added.add(material)
                newSel.append(material)
        self._setSelectionFromPrimList(newSel)

    def jumpToMasterSelectedPrims(self):
        foundMasters = False
        newSel = []
        added = set()
        # We don't expect this to take long, so no BusyContext
        for prim in self._currentNodes:
            if prim.IsInstance():
                prim = prim.GetMaster()
                foundMasters = True
            if not (prim in added):
                added.add(prim)
                newSel.append(prim)

        showMasters = self._ui.actionShow_Master_Prims.isChecked()
        if foundMasters and not showMasters:
            self._ui.actionShow_Master_Prims.setChecked(True)
            self._toggleShowMasterPrims()

        self._setSelectionFromPrimList(newSel)
        
    def visSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.makeVisible()
            self.editComplete('Made selected prims visible')
            # makeVisible may cause aunt and uncle prims' authored vis
            # to change, so we need to fix up the whole shebang
            self._resetNodeViewVis(selItemsOnly=False)

    def visOnlySelectedPrims(self):
        with BusyContext():
            from common import ResetSessionVisibility, InvisRootPrims
            ResetSessionVisibility(self._stage)
            InvisRootPrims(self._stage)
            for item in self.getSelectedItems():
                item.makeVisible()
            self.editComplete('Made ONLY selected prims visible')
            # QTreeWidget does not honor setUpdatesEnabled, and updating
            # the Vis column for all widgets is pathologically slow.
            # It is sadly much much faster to regenerate the entire view
            self._resetNodeView()

    def invisSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.setVisible(False)
            self.editComplete('Made selected prims invisible')
            self._resetNodeViewVis()

    def removeVisSelectedPrims(self):
        with BusyContext():
            for item in self.getSelectedItems():
                item.removeVisibility()
            self.editComplete("Removed selected prims' visibility opinions")
            self._resetNodeViewVis()

    def resetSessionVisibility(self):
        with BusyContext():
            from common import ResetSessionVisibility
            ResetSessionVisibility(self._stage)
            self.editComplete('Removed ALL session visibility opinions.')
            # QTreeWidget does not honor setUpdatesEnabled, and updating
            # the Vis column for all widgets is pathologically slow.
            # It is sadly much much faster to regenerate the entire view
            self._resetNodeView()

    def activateSelectedPrims(self):
        with BusyContext():
            nodeNames=[]
            for item in self.getSelectedItems():
                item.setActive(True)
                nodeNames.append(item.name)
            self.editComplete("Activated %s." % nodeNames)

    def deactivateSelectedPrims(self):
        with BusyContext():
            nodeNames=[]
            for item in self.getSelectedItems():
                item.setActive(False)
                nodeNames.append(item.name)
            self.editComplete("Deactivated %s." % nodeNames)

    def loadSelectedPrims(self):
        with BusyContext():
            nodeNames=[]
            for item in self.getSelectedItems():
                item.setLoaded(True)
                nodeNames.append(item.name)
            self.editComplete("Loaded %s." % nodeNames)

    def unloadSelectedPrims(self):
        with BusyContext():
            nodeNames=[]
            for item in self.getSelectedItems():
                item.setLoaded(False)
                nodeNames.append(item.name)
            self.editComplete("Unloaded %s." % nodeNames)

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
                for selPrim in self._currentNodes:
                    selPath = selPrim.GetPath()
                    if (selPath != Sdf.Path.absoluteRootPath and 
                        path.HasPrefix(selPath)):
                        doSelection = False
                        break
            if doSelection:
                item = self.selectNodeByPath(path, instanceIndex, updateMode,
                                             applyPickMode=True)
                if item and item.node.GetPath() != Sdf.Path.absoluteRootPath:
                    # Scroll the node view widget to show the newly selected
                    # item, unless it's the pseudoRoot, which represents "no
                    # selection"
                    self._ui.nodeView.scrollToItem(item)
            if doContext:
                # The context menu requires an item for validation.  Make sure
                # we have a valid one to give it.
                if not item:
                    item = self._getItemAtPath(path)
                self._showNodeContextMenu(item)
                # context menu steals mouse release event from the StageView.
                # We need to give it one so it can track its interaction
                # mode properly
                mrEvent = QtGui.QMouseEvent(QtCore.QEvent.MouseButtonRelease,
                                            QtGui.QCursor.pos(),
                                            QtCore.Qt.RightButton, 
                                            QtCore.Qt.MouseButtons(QtCore.Qt.RightButton),
                                            QtCore.Qt.KeyboardModifiers())
                QtGui.QApplication.sendEvent(self._stageView, mrEvent)

    def onRollover(self, path, instanceIndex, modifiers):
        from common import GetEnclosingModelPrim, GetClosestBoundMaterial
        
        prim = self._stage.GetPrimAtPath(path)
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
                    from common import GetAssetCreationTime
                    specs = model.GetPrimStack()
                    results = GetAssetCreationTime(specs, 
                                                   mAPI.GetAssetIdentifier())
                    for key, value in assetInfo.iteritems():
                        aiStr += "<br> -- <em>%s</em> : %s" % (key, str(value))
                    aiStr += "<br><em><small>%s created on %s by %s</small></em>" % \
                        results
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

            # Instance / master info, if this prim is an instance
            instanceStr = ""
            if prim.IsInstance():
                instanceStr = "<hr><b>Instancing:</b><br>"
                instanceStr += "<nobr><small><em>Instance of master:</em></small> %s</nobr>" % \
                    str(prim.GetMaster().GetPath())
                
            # Finally, important properties (mesh scheme, vis animated?, 
            # non-default purpose, doubleSided)

            # Then put it all together
            tip = headerStr + propertyStr + materialStr + instanceStr + aiStr + vsStr
            
        else:
            tip = ""
        QtGui.QToolTip.showText(QtGui.QCursor.pos(), tip, self._stageView)
