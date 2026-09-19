#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# Qt-dependent UI classes for the Noodles Editor, extracted from __init__.py.
# This module is conditionally imported by __init__.py when Qt/Usdviewq is
# available. The separation keeps __init__.py under the C901 complexity
# threshold and allows the noodles package to be imported in headless
# (non-Qt) environments without errors.

import os
import sys

from pxr.UsdNoodles.core import LinkSelectionMode
from pxr import Tf
from pxr.Usdviewq import plugin
from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

from .graphView import GraphView
from .noodlesConfig import NoodlesConfig
from .utils import M, MenuBuilder
from .widgets.keyBindingDialog import KeyBindingDialog

_IS_MACOS = sys.platform == "darwin"


class _MacMenuBarSwapper:
    def __init__(self, mainWindow):
        self._mainWindow = mainWindow
        self._mainMenuBar = mainWindow.menuBar()
        self._noodlesActive = False
        self._noodlesMenuActions = []
        self._usdviewEditAction = None
        self._usdviewWindowAction = None
        for action in self._mainMenuBar.actions():
            text = action.text().replace("&", "")
            if text == "Edit":
                self._usdviewEditAction = action
            elif text == "Window":
                self._usdviewWindowAction = action

    def setNoodlesMenuActions(self, actions):
        self._noodlesMenuActions = actions

    def activateNoodles(self):
        if self._noodlesActive:
            return
        self._noodlesActive = True
        if self._usdviewEditAction:
            self._usdviewEditAction.setVisible(False)
        if self._usdviewWindowAction:
            self._usdviewWindowAction.setVisible(False)
        for action in self._noodlesMenuActions:
            action.setVisible(True)

    def activateUsdview(self):
        if not self._noodlesActive:
            return
        self._noodlesActive = False
        for action in self._noodlesMenuActions:
            action.setVisible(False)
        if self._usdviewEditAction:
            self._usdviewEditAction.setVisible(True)
        if self._usdviewWindowAction:
            self._usdviewWindowAction.setVisible(True)

    def cleanup(self):
        self.activateUsdview()
        for action in self._noodlesMenuActions:
            self._mainMenuBar.removeAction(action)
        self._noodlesMenuActions = []


class _NoodlesClickFilter(QtCore.QObject):
    def __init__(self, editorWidget, parent=None):
        super().__init__(parent)
        self._editorWidget = editorWidget

    def eventFilter(self, obj, event):
        if event.type() != QtCore.QEvent.MouseButtonPress:
            return False
        if not isinstance(obj, QtWidgets.QWidget):
            return False
        if isinstance(obj, QtWidgets.QMenu):
            return False
        manager = GetEditorManager()
        if not manager._macSwapper:
            return False
        if self._editorWidget.isAncestorOf(obj) or obj is self._editorWidget:
            manager._macSwapper.activateNoodles()
        else:
            manager._macSwapper.activateUsdview()
        return False


def _layerDisplayName(layer, role):
    """Return the user-facing name shown in the edit-target combo."""
    if role == "session":
        return "Session Layer"
    if role == "root":
        return "Root Layer"
    # Sublayer — strip directory + extension when possible.
    identifier = layer.identifier if layer else ""
    base = os.path.basename(identifier) if identifier else ""
    return base or identifier or "<anonymous>"


def _enumerateLayersForCombo(stage):
    """Yield (display_name, layer) tuples in the order they should appear."""
    if stage is None:
        return
    sessionLayer = stage.GetSessionLayer()
    rootLayer = stage.GetRootLayer()
    seen_ids = set()
    if sessionLayer is not None:
        yield _layerDisplayName(sessionLayer, "session"), sessionLayer
        seen_ids.add(sessionLayer.identifier)
    if rootLayer is not None:
        yield _layerDisplayName(rootLayer, "root"), rootLayer
        seen_ids.add(rootLayer.identifier)
    try:
        layerStack = stage.GetLayerStack(includeSessionLayers=True)
    except Exception as e:
        Tf.Warn(f"_enumerateLayersForCombo: GetLayerStack failed: {e}")
        layerStack = []
    for layer in layerStack:
        if layer is None or layer.identifier in seen_ids:
            continue
        yield _layerDisplayName(layer, "sublayer"), layer
        seen_ids.add(layer.identifier)


class NoodlesEditorWidget(QtWidgets.QWidget):
    def __init__(self, usdviewApi, parent=None):
        super(NoodlesEditorWidget, self).__init__(parent)

        self._usdviewApi = usdviewApi
        self._noodlesMenuActions = []

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        if _IS_MACOS and usdviewApi:
            self._menuBar = usdviewApi.qMainWindow.menuBar()
            self._existingActions = set(self._menuBar.actions())
        else:
            self._menuBar = QtWidgets.QMenuBar(self)
            layout.addWidget(self._menuBar)
            self._existingActions = None

        # Get settings object from usdviewApi
        settingsObject = None
        if usdviewApi:
            try:
                settingsObject = usdviewApi.GetSettings()
            except Exception:
                pass

        # Create GraphView with settings
        self._graphView = GraphView(self, settingsObject=settingsObject)
        self._graphView.setUsdviewApi(usdviewApi)
        self._graphView.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        layout.addWidget(self._graphView, 1)  # stretchFactor=1

        self._setupMenus()

    def _setupMenus(self):
        gv = self._graphView

        # Build library menu items dynamically
        libraryMenuItems = []
        for library in gv._nodeLibraries:
            # Skip libraries that failed discovery (e.g. missing plugins)
            if getattr(library, "_discovery_failed", False):
                continue
            libraryMenuItems.append(
                (
                    M.CHECK,
                    library.get_name(),
                    None,
                    lambda checked, lib=library: self._toggleLibrary(lib, checked),
                    library.enabled,
                )
            )

        menuConf = [
            (
                "&Edit",
                [
                    (M.ACTION, "&Undo", "Ctrl+Z", gv._performUndo),
                    (M.ACTION, "&Redo", "Ctrl+Shift+Z", gv._performRedo),
                    (M.SEPARATOR,),
                    (
                        M.SUBMENU,
                        "&Create",
                        [],
                    ),
                    (M.SEPARATOR,),
                    (
                        M.ACTION,
                        "&Keyboard Shortcuts...",
                        None,
                        self._openKeyBindingDialog,
                    ),
                ],
            ),
            (
                "&View",
                [
                    (
                        M.CHECK,
                        "Show &Links",
                        None,
                        self._toggleLinks,
                        gv.drawLinks,
                        "_showLinksAction",
                    ),
                    (
                        M.CHECK,
                        "Show &Nodes",
                        None,
                        self._toggleNodes,
                        gv.drawNodes,
                        "_showNodesAction",
                    ),
                    (
                        M.CHECK,
                        "Show &Text",
                        None,
                        self._toggleText,
                        gv.drawText,
                        "_showTextAction",
                    ),
                    (M.SEPARATOR,),
                    (
                        M.CHECK,
                        "Link &Dimming",
                        None,
                        self._toggleLinkDimming,
                        gv.linkDimming.target > 0.0,
                        "_linkDimmingAction",
                    ),
                ],
            ),
            (
                "&Navigate",
                [
                    (M.ACTION, "Frame &All", None, gv.frameAll),
                    (M.ACTION, "Frame &Selection", None, gv.frameScene),
                    (
                        M.ACTION,
                        "Frame Selection with &Connections",
                        None,
                        gv.frameSelectionWithConnections,
                    ),
                ],
            ),
            (
                "&Select",
                [
                    (M.ACTION, "Select &All", None, gv.selectAll),
                    (
                        M.ACTION,
                        "&Clear Selection",
                        None,
                        self._clearSelection,
                    ),
                    (M.SEPARATOR,),
                    (
                        M.SUBMENU_GROUP,
                        "Link Selection &Mode",
                        [
                            (
                                M.GROUP_ITEM,
                                "Nodes &Only",
                                LinkSelectionMode.NODES_ONLY,
                                True,
                            ),
                            (
                                M.GROUP_ITEM,
                                "With &All Links",
                                LinkSelectionMode.WITH_ALL_LINKS,
                                False,
                            ),
                            (
                                M.GROUP_ITEM,
                                "With &Input Links",
                                LinkSelectionMode.WITH_INPUTS,
                                False,
                            ),
                            (
                                M.GROUP_ITEM,
                                "With &Output Links",
                                LinkSelectionMode.WITH_OUTPUTS,
                                False,
                            ),
                        ],
                        "_linkSelActionGroup",
                        self._onLinkSelectionModeChanged,
                    ),
                    (M.SEPARATOR,),
                    (
                        M.CHECK,
                        "Frame in &Prim Tree on Select",
                        None,
                        self._toggleFrameInPrimTree,
                        gv._frameInPrimTreeOnSelect,
                        "_frameInPrimTreeAction",
                    ),
                ],
            ),
            (
                "&Node Libraries",
                libraryMenuItems,
            ),
        ]

        builder = MenuBuilder(self)
        builder.buildMenuBar(self._menuBar, menuConf)

    def _toggleLinks(self, checked):
        self._graphView.drawLinks = checked
        self._graphView.update()

    def _toggleNodes(self, checked):
        self._graphView.drawNodes = checked
        self._graphView.update()

    def _toggleText(self, checked):
        self._graphView.drawText = checked
        self._graphView.update()

    def _toggleLinkDimming(self, checked):
        self._graphView.linkDimming.target = 1.0 if checked else 0.0
        self._graphView.update()

    def _clearSelection(self):
        self._graphView.clearSelection()
        self._graphView.clearLinkSelection()
        self._graphView._syncSelectionToPrimtree()

    def _onLinkSelectionModeChanged(self, action):
        mode = action.data()
        if mode is not None:
            self._graphView.linkSelectionMode = mode

    def _openKeyBindingDialog(self):
        dialog = KeyBindingDialog(self, self._graphView)
        dialog.exec_()

    def _toggleFrameInPrimTree(self, checked):
        self._graphView._frameInPrimTreeOnSelect = checked

    def _toggleLibrary(self, library, checked):
        library.enabled = checked
        self._graphView._updateNodeCreationHotbox()
        library_name = library.get_name()
        status = "enabled" if checked else "disabled"
        self._graphView._showPopupMessage(f"{library_name}: {status}")

        enabled_names = [
            lib.get_name() for lib in self._graphView._nodeLibraries if lib.enabled
        ]
        NoodlesConfig.set("enabledLibraries", enabled_names)
        NoodlesConfig.save()

    @property
    def graphView(self):
        return self._graphView


class NoodlesEditorWindow(QtWidgets.QDockWidget):
    windowClosed = QtCore.Signal(int)
    windowActivated = QtCore.Signal(int)

    def __init__(self, usdviewApi, windowId, parent=None):
        super(NoodlesEditorWindow, self).__init__("Noodles Editor", parent)

        self._usdviewApi = usdviewApi
        self._windowId = windowId
        self._windowTitle = "Noodles Editor"
        self._mainWindow = parent

        self._previousFloatingState = None

        self.setAllowedAreas(
            QtCore.Qt.LeftDockWidgetArea
            | QtCore.Qt.RightDockWidgetArea
            | QtCore.Qt.BottomDockWidgetArea
        )

        self._editorWidget = NoodlesEditorWidget(usdviewApi, self)
        self.setWidget(self._editorWidget)

        self.setMinimumSize(400, 300)
        self.resize(800, 600)

        self.topLevelChanged.connect(self._onTopLevelChanged)

        self._setupWindowMenu()

        self._updateWindowTitle()

        self._clickFilter = None
        if _IS_MACOS:
            self._finalizeNoodlesMenus()
            self._clickFilter = _NoodlesClickFilter(self._editorWidget, self)
            app = QtWidgets.QApplication.instance()
            if app:
                app.installEventFilter(self._clickFilter)

    def _finalizeNoodlesMenus(self):
        existingActions = self._editorWidget._existingActions
        if existingActions is None:
            return
        mainMenuBar = self._editorWidget._menuBar
        newActions = [a for a in mainMenuBar.actions() if a not in existingActions]
        editAction = None
        for a in existingActions:
            if a.text().replace("&", "") == "Edit":
                editAction = a
                break
        for action in newActions:
            mainMenuBar.removeAction(action)
        for action in newActions:
            mainMenuBar.insertAction(editAction, action)
        for action in newActions:
            action.setVisible(False)
        self._editorWidget._noodlesMenuActions = newActions

    def _setupWindowMenu(self):
        menuBar = self._editorWidget._menuBar
        windowMenu = menuBar.addMenu("&Window")

        preferencesAction = QtGui.QAction("&Noodles Preferences...", self)
        preferencesAction.setMenuRole(QtGui.QAction.MenuRole.NoRole)
        preferencesAction.triggered.connect(self._openPreferences)
        windowMenu.addAction(preferencesAction)

        windowMenu.addSeparator()

        self._floatAction = QtGui.QAction("&Float", self)
        self._floatAction.setCheckable(True)
        self._floatAction.setChecked(self.isFloating())
        self._floatAction.setEnabled(False)
        self._floatAction.triggered.connect(self._toggleFloat)
        windowMenu.addAction(self._floatAction)

    def _openPreferences(self):
        self._editorWidget._graphView._openPreferences()

    def _toggleFloat(self, checked):
        if checked:
            if not self.isFloating():
                self.setFloating(True)
        else:
            if self.isFloating() and self._mainWindow:
                self._mainWindow.addDockWidget(QtCore.Qt.RightDockWidgetArea, self)
                self.setFloating(False)

    def _onTopLevelChanged(self, topLevel):
        if hasattr(self, "_floatAction"):
            self._floatAction.setChecked(topLevel)

        if topLevel:
            self.setWindowFlags(
                QtCore.Qt.Window
                | QtCore.Qt.WindowTitleHint
                | QtCore.Qt.WindowSystemMenuHint
                | QtCore.Qt.WindowMinimizeButtonHint
                | QtCore.Qt.WindowMaximizeButtonHint
                | QtCore.Qt.WindowCloseButtonHint
            )
            self.show()

        self._previousFloatingState = topLevel

    @property
    def windowId(self):
        return self._windowId

    def _updateWindowTitle(self):
        primPath = self._editorWidget.graphView.currentPrimPath
        if primPath:
            self.setWindowTitle(f"{self._windowTitle} - {primPath}")
        else:
            self.setWindowTitle(f"{self._windowTitle}")

    def closeEvent(self, event):
        if _IS_MACOS and self._clickFilter:
            app = QtWidgets.QApplication.instance()
            if app:
                app.removeEventFilter(self._clickFilter)
            self._clickFilter = None
        self.windowClosed.emit(self._windowId)
        super().closeEvent(event)

    def changeEvent(self, event):
        super().changeEvent(event)
        if event.type() == QtCore.QEvent.ActivationChange:
            if self.isActiveWindow():
                self.windowActivated.emit(self._windowId)

    def event(self, event):
        if event.type() == QtCore.QEvent.FocusIn:
            self.windowActivated.emit(self._windowId)
        return super().event(event)

    @property
    def graphView(self):
        return self._editorWidget.graphView


class NoodlesEditorManager:
    def __init__(self):
        self._windows = {}
        self._nextWindowId = 1
        self._activeWindowId = None
        self._usdviewApi = None
        self._testGraphIndex = 0
        self._macSwapper = None

    def _getNextTestGraphPath(self):
        from . import _getAssetsPath, TEST_GRAPH_FILES

        assetsPath = _getAssetsPath()

        for _ in range(len(TEST_GRAPH_FILES)):
            filename = TEST_GRAPH_FILES[self._testGraphIndex]
            self._testGraphIndex = (self._testGraphIndex + 1) % len(TEST_GRAPH_FILES)

            graphPath = assetsPath / filename
            if graphPath.exists():
                return graphPath

        return None

    def setUsdviewApi(self, usdviewApi):
        self._usdviewApi = usdviewApi
        self._installCloseStageGuard(usdviewApi)

    def _installCloseStageGuard(self, usdviewApi):
        try:
            controller = usdviewApi._UsdviewApi__appController
            if getattr(controller, "_noodles_close_guard", False):
                return
            original = controller._closeStage
            manager = self

            def _guarded_close():
                try:
                    for window in manager.getAllWindows():
                        gv = window.graphView
                        gv.setUpdatesEnabled(False)
                        gv.makeCurrent()
                        gv._nodeRenderManager.clearCache()
                        gv.doneCurrent()
                finally:
                    original()

            controller._closeStage = _guarded_close
            controller._noodles_close_guard = True
        except AttributeError:
            pass

    def _ensureLayerEditorVisible(self):
        """Show the Layer Editor alongside the Noodles Editor.

        Layer Editor and Noodles Editor are independent features, but the
        product expectation is that opening a Noodles Editor surfaces the
        Layer Editor too.
        """
        if not self._usdviewApi:
            return
        _layerEditorManager.setUsdviewApi(self._usdviewApi)
        existing = _layerEditorManager.getWindow()
        if existing is None:
            _layerEditorManager.showOrCreate()
        else:
            existing.show()
            existing.raise_()
            existing.editorWidget.reattach()

    def _getNextWindowId(self):
        windowId = self._nextWindowId
        self._nextWindowId += 1
        return windowId

    def _onWindowClosed(self, windowId):
        if windowId in self._windows:
            del self._windows[windowId]

        if self._activeWindowId == windowId:
            if self._windows:
                self._activeWindowId = max(self._windows.keys())
            else:
                self._activeWindowId = None

        if not self._windows and self._macSwapper:
            self._macSwapper.cleanup()
            self._macSwapper = None

    def _onWindowActivated(self, windowId):
        if windowId in self._windows:
            self._activeWindowId = windowId
        # If the Layer Editor is open, ask it to re-subscribe to the new active
        # editor so its display + edit-target writes target the right NodeGraph.
        layerWindow = _layerEditorManager.getWindow()
        if layerWindow is not None:
            layerWindow.editorWidget.reattach()

    def createNewEditor(
        self,
        floating=True,
        primPath=None,
        loadTestGraph=False,
        isContainer=False,
    ):
        if not self._usdviewApi:
            Tf.Warn("NoodlesEditorManager: usdviewApi not set")
            return None

        if _IS_MACOS and self._windows:
            window = next(iter(self._windows.values()))
            window.show()
            window.raise_()
            window.activateWindow()
            if primPath is not None:
                stage = self._usdviewApi.stage
                if isContainer:
                    window.graphView.loadContainer(stage, primPath)
                else:
                    window.graphView.loadBlueprint(stage, primPath)
                window._updateWindowTitle()
            if self._macSwapper:
                self._macSwapper.activateNoodles()
            self._ensureLayerEditorVisible()
            return window

        mainWindow = self._usdviewApi.qMainWindow
        windowId = self._getNextWindowId()

        window = NoodlesEditorWindow(self._usdviewApi, windowId, mainWindow)

        window.windowClosed.connect(self._onWindowClosed)
        window.windowActivated.connect(self._onWindowActivated)

        self._windows[windowId] = window
        self._activeWindowId = windowId

        currentSize = mainWindow.size()
        editorWidth = currentSize.height()

        if floating:
            window.setFloating(True)
            window.resize(editorWidth, editorWidth)

            mainGeometry = mainWindow.frameGeometry()
            offset = (len(self._windows) - 1) * 30
            window.move(
                mainGeometry.right() + 10 + offset,
                mainGeometry.top() + offset,
            )
        else:
            window.setMinimumWidth(editorWidth)

            newWidth = currentSize.width() + editorWidth
            mainWindow.resize(newWidth, currentSize.height())

            mainWindow.addDockWidget(QtCore.Qt.RightDockWidgetArea, window)
            mainWindow.resizeDocks([window], [editorWidth], QtCore.Qt.Horizontal)

            def relaxMinimum():
                window.setMinimumWidth(200)

            QtCore.QTimer.singleShot(100, relaxMinimum)

        window.show()
        window.raise_()

        if _IS_MACOS:
            self._ensureMacSwapper(window, mainWindow)

        self._loadContentInWindow(window, primPath, isContainer, loadTestGraph)

        return window

    def _ensureMacSwapper(self, window, mainWindow):
        if not self._macSwapper:
            self._macSwapper = _MacMenuBarSwapper(mainWindow)
        if not self._macSwapper._noodlesMenuActions:
            self._macSwapper.setNoodlesMenuActions(
                window._editorWidget._noodlesMenuActions
            )
        self._macSwapper.activateNoodles()

    def _loadContentInWindow(self, window, primPath, isContainer, loadTestGraph):
        if primPath is not None:
            stage = self._usdviewApi.stage
            if isContainer:
                window.graphView.loadContainer(stage, primPath)
            else:
                window.graphView.loadBlueprint(stage, primPath)
            window._updateWindowTitle()
        elif loadTestGraph:
            self._loadTestGraphInWindow(window)
        else:
            self._loadStageInWindow(window)

        self._ensureLayerEditorVisible()

        return window

    def _loadTestGraphInWindow(self, window):
        testGraphPath = self._getNextTestGraphPath()
        if not testGraphPath:
            return

        def loadDeferred():
            graphView = window.graphView
            if graphView.initialized:
                graphView._loadSampleGraph(str(testGraphPath))
                graphView.textChanged = True
                if graphView.nodes:
                    graphView.frameScene()
                window.setWindowTitle(f"{window._windowTitle} - {testGraphPath.name}")
            else:
                QtCore.QTimer.singleShot(50, loadDeferred)

        QtCore.QTimer.singleShot(100, loadDeferred)

    def _loadStageInWindow(self, window):
        stage = self._usdviewApi.stage
        if not stage:
            return

        def loadDeferred():
            graphView = window.graphView
            if graphView.initialized:
                graphView.loadStage(stage)
                window._updateWindowTitle()
            else:
                QtCore.QTimer.singleShot(50, loadDeferred)

        QtCore.QTimer.singleShot(100, loadDeferred)

    def showOrCreateEditor(self, floating=True, primPath=None):
        """Show the existing editor if one is open, otherwise create a new one."""
        window = None
        if self._activeWindowId is not None and self._activeWindowId in self._windows:
            window = self._windows[self._activeWindowId]
        elif self._windows:
            window = self._windows[max(self._windows.keys())]

        if window is None:
            return self.createNewEditor(floating=floating, primPath=primPath)

        window.show()
        window.raise_()
        window.activateWindow()

        if primPath is not None:
            stage = self._usdviewApi.stage
            window.graphView.loadBlueprint(stage, primPath)
            window._updateWindowTitle()

        self._activeWindowId = window.windowId
        self._ensureLayerEditorVisible()
        return window

    def getWindow(self, windowId):
        return self._windows.get(windowId)

    def getActiveWindow(self):
        if self._activeWindowId is not None:
            return self._windows.get(self._activeWindowId)
        return None

    def getAllWindows(self):
        return list(self._windows.values())

    def getWindowCount(self):
        return len(self._windows)

    def closeAllWindows(self):
        for window in list(self._windows.values()):
            window.close()
        self._windows.clear()
        self._activeWindowId = None
        if self._macSwapper:
            self._macSwapper.cleanup()
            self._macSwapper = None


class NoodlesLayerEditorWidget(QtWidgets.QWidget):
    """Standalone layer editor for picking the USD edit target.

    Lists the layers in the active stage and lets the user pick which one
    subsequent attribute / node-creation edits author to. The active edit
    target is shown bold with a leading marker. Single-click to set; right-
    click for context menu.
    """

    def __init__(self, usdviewApi, parent=None):
        super().__init__(parent)
        self._usdviewApi = usdviewApi
        self._refreshing = False
        # The NodeGraph we are currently subscribed to (so we can detach
        # cleanly when the active editor changes).
        self._attachedNodeGraph = None
        # The GraphView whose nodeGraphReplaced signal we're listening to.
        self._attachedGraphView = None

        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(4, 4, 4, 4)
        layout.setSpacing(4)

        header = QtWidgets.QLabel("Edit Target")
        headerFont = header.font()
        headerFont.setBold(True)
        header.setFont(headerFont)
        layout.addWidget(header)

        self._listWidget = QtWidgets.QListWidget(self)
        self._listWidget.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self._listWidget.itemClicked.connect(self._onItemClicked)
        self._listWidget.customContextMenuRequested.connect(
            self._onContextMenuRequested
        )
        layout.addWidget(self._listWidget, 1)

        self._statusLabel = QtWidgets.QLabel()
        self._statusLabel.setStyleSheet("color: gray;")
        layout.addWidget(self._statusLabel)

        self._refreshSubscription()
        self.refresh()

    def _activeNoodlesWindow(self):
        manager = GetEditorManager()
        return manager.getActiveWindow() if manager else None

    def _activeNodeGraph(self):
        window = self._activeNoodlesWindow()
        if window is None or window.graphView is None:
            return None
        return window.graphView.nodeGraph

    def _activeStage(self):
        ng = self._activeNodeGraph()
        if ng is not None:
            stage = ng.getStage()
            if stage is not None:
                return stage
        if self._usdviewApi is not None:
            try:
                return self._usdviewApi.stage
            except Exception:
                return None
        return None

    def _refreshSubscription(self):
        """Re-attach signal/callback wiring to the currently-active editor."""
        window = self._activeNoodlesWindow()
        graphView = window.graphView if window is not None else None
        nodeGraph = self._activeNodeGraph()

        if graphView is not self._attachedGraphView:
            if self._attachedGraphView is not None:
                try:
                    self._attachedGraphView.nodeGraphReplaced.disconnect(
                        self._onNodeGraphReplaced
                    )
                except (TypeError, RuntimeError):
                    pass
            if graphView is not None:
                graphView.nodeGraphReplaced.connect(self._onNodeGraphReplaced)
            self._attachedGraphView = graphView

        if nodeGraph is not self._attachedNodeGraph:
            if self._attachedNodeGraph is not None:
                self._attachedNodeGraph.removeStageChangedCallback(self._onStageChanged)
                self._attachedNodeGraph.removeEditTargetChangedCallback(
                    self._onEditTargetChanged
                )
            if nodeGraph is not None:
                nodeGraph.addStageChangedCallback(self._onStageChanged)
                nodeGraph.addEditTargetChangedCallback(self._onEditTargetChanged)
            self._attachedNodeGraph = nodeGraph

    def refresh(self):
        if self._refreshing:
            return
        self._refreshing = True
        try:
            self._listWidget.clear()
            stage = self._activeStage()
            if stage is None:
                self._statusLabel.setText("No active stage.")
                return
            try:
                activeLayer = stage.GetEditTarget().GetLayer()
            except Exception:
                activeLayer = None
            for name, layer in _enumerateLayersForCombo(stage):
                marker = "● " if layer == activeLayer else "  "
                item = QtWidgets.QListWidgetItem(marker + name)
                item.setData(QtCore.Qt.UserRole, layer)
                font = item.font()
                font.setBold(layer == activeLayer)
                item.setFont(font)
                if layer == activeLayer:
                    item.setToolTip("Current edit target")
                else:
                    item.setToolTip("Click to set as edit target")
                self._listWidget.addItem(item)
            ng = self._activeNodeGraph()
            from .nodeGraph import describeNonPersistentEditTarget

            warning = describeNonPersistentEditTarget(activeLayer, stage)
            if ng is None:
                base = "No active Noodles Editor; using usdview stage."
                self._statusLabel.setText(f"{base}\n⚠ {warning}" if warning else base)
            elif warning:
                self._statusLabel.setText(f"⚠ {warning}")
            else:
                self._statusLabel.setText("")
        finally:
            self._refreshing = False

    def _setEditTarget(self, layer):
        if layer is None:
            return
        ng = self._activeNodeGraph()
        if ng is not None:
            ng.setEditTargetLayer(layer)
            return
        # Fallback: no active Noodles editor — author directly to the stage.
        stage = self._activeStage()
        if stage is None:
            return
        try:
            stage.SetEditTarget(layer)
        except Exception as e:
            Tf.Warn(f"NoodlesLayerEditor: SetEditTarget failed: {e}")
            self.refresh()
            return
        from .nodeGraph import describeNonPersistentEditTarget

        warning = describeNonPersistentEditTarget(layer, stage)
        if warning:
            Tf.Warn(warning)
        self.refresh()

    def _onItemClicked(self, item):
        if self._refreshing:
            return
        layer = item.data(QtCore.Qt.UserRole)
        self._setEditTarget(layer)

    def _onContextMenuRequested(self, pos):
        item = self._listWidget.itemAt(pos)
        if item is None:
            return
        layer = item.data(QtCore.Qt.UserRole)
        if layer is None:
            return
        menu = QtWidgets.QMenu(self)
        setAction = menu.addAction("Set as Edit Target")
        setAction.triggered.connect(lambda: self._setEditTarget(layer))
        menu.exec_(self._listWidget.viewport().mapToGlobal(pos))

    def _onNodeGraphReplaced(self):
        self._refreshSubscription()
        self.refresh()

    def _onStageChanged(self, _stage):
        self.refresh()

    def _onEditTargetChanged(self, _layer):
        self.refresh()

    def reattach(self):
        """Re-subscribe and refresh — call when active Noodles editor changes."""
        self._refreshSubscription()
        self.refresh()

    def detach(self):
        """Remove all callbacks and disconnect signals — call before widget is hidden."""
        if self._attachedGraphView is not None:
            try:
                self._attachedGraphView.nodeGraphReplaced.disconnect(
                    self._onNodeGraphReplaced
                )
            except (TypeError, RuntimeError):
                pass
            self._attachedGraphView = None
        if self._attachedNodeGraph is not None:
            self._attachedNodeGraph.removeStageChangedCallback(self._onStageChanged)
            self._attachedNodeGraph.removeEditTargetChangedCallback(
                self._onEditTargetChanged
            )
            self._attachedNodeGraph = None


class NoodlesLayerEditorWindow(QtWidgets.QDockWidget):
    windowClosed = QtCore.Signal()

    def __init__(self, usdviewApi, parent=None):
        super().__init__("Layer Editor", parent)
        self._usdviewApi = usdviewApi
        self.setAllowedAreas(
            QtCore.Qt.LeftDockWidgetArea
            | QtCore.Qt.RightDockWidgetArea
            | QtCore.Qt.BottomDockWidgetArea
        )
        self._editorWidget = NoodlesLayerEditorWidget(usdviewApi, self)
        self.setWidget(self._editorWidget)
        self.setMinimumSize(240, 200)
        self.resize(320, 320)

    @property
    def editorWidget(self):
        return self._editorWidget

    def closeEvent(self, event):
        self._editorWidget.detach()
        self.windowClosed.emit()
        super().closeEvent(event)


class NoodlesLayerEditorManager:
    def __init__(self):
        self._window = None
        self._usdviewApi = None

    def setUsdviewApi(self, usdviewApi):
        self._usdviewApi = usdviewApi

    def showOrCreate(self, floating=True):
        if not self._usdviewApi:
            Tf.Warn("NoodlesLayerEditorManager: usdviewApi not set")
            return None
        if self._window is not None:
            self._window.show()
            self._window.raise_()
            self._window.activateWindow()
            self._window.editorWidget.reattach()
            return self._window
        mainWindow = self._usdviewApi.qMainWindow
        self._window = NoodlesLayerEditorWindow(self._usdviewApi, mainWindow)
        self._window.windowClosed.connect(self._onWindowClosed)
        if floating:
            self._window.setFloating(True)
            mainGeometry = mainWindow.frameGeometry()
            self._window.move(
                mainGeometry.right() + 10,
                mainGeometry.top() + 60,
            )
        else:
            mainWindow.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self._window)
        self._window.show()
        self._window.raise_()
        return self._window

    def _onWindowClosed(self):
        self._window = None

    def getWindow(self):
        return self._window


_layerEditorManager = NoodlesLayerEditorManager()


def GetLayerEditorManager():
    return _layerEditorManager


def ShowNoodlesLayerEditor(usdviewApi, floating=True):
    _layerEditorManager.setUsdviewApi(usdviewApi)
    return _layerEditorManager.showOrCreate(floating=floating)


def ShowNoodlesEditor(usdviewApi, floating=True, primPath=None):
    _editorManager.setUsdviewApi(usdviewApi)
    return _editorManager.showOrCreateEditor(floating=floating, primPath=primPath)


def ShowNoodlesEditorForBlueprint(appController, primPath):
    _editorManager.setUsdviewApi(appController._usdviewApi)
    return _editorManager.createNewEditor(floating=True, primPath=primPath)


def ShowNoodlesEditorForContainer(appController, primPath):
    _editorManager.setUsdviewApi(appController._usdviewApi)
    return _editorManager.createNewEditor(
        floating=True, primPath=primPath, isContainer=True
    )


class NoodlesPluginContainer(plugin.PluginContainer):
    def registerPlugins(self, plugRegistry, plugCtx):
        self._showEditorCommand = plugRegistry.registerCommandPlugin(
            "NoodlesPluginContainer.ShowNoodlesEditor",
            "Noodles Editor",
            ShowNoodlesEditor,
            "Opens the Noodles Editor (focuses existing if already open)",
        )
        self._showLayerEditorCommand = plugRegistry.registerCommandPlugin(
            "NoodlesPluginContainer.ShowNoodlesLayerEditor",
            "Layer Editor",
            ShowNoodlesLayerEditor,
            "Opens the Layer Editor (pick which USD layer subsequent edits author to)",
        )

    def configureView(self, plugRegistry, plugUIBuilder):
        windowMenu = plugUIBuilder.findOrCreateMenu("Window")
        windowMenu.addItem(self._showEditorCommand, "N")
        windowMenu.addItem(self._showLayerEditorCommand, "L")


_editorManager = NoodlesEditorManager()


def GetEditorManager():
    return _editorManager
