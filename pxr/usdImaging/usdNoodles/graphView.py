#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

import ctypes
import math
import os
import time
from collections import defaultdict
from contextlib import contextmanager
from pathlib import Path

from pxr.UsdNoodles.core import (
    Animator,
    layoutGraphPositions,
    LayoutParams,
    LinkData,
    LinkSelectionMode,
    positionWriteGeneration,
)
from pxr.UsdNoodles.render import (
    FontAtlas,
    NODE_VERTEX_ATTRIB_LAYOUT,
    NodeRenderManager,
    NodeTransformFrame,
    NodeVertex,
    RenderProfiler,
    ShaderLibrary,
)
from pxr.UsdNoodles.spatial import SpatialIndex
from OpenGL import GL
from OpenGL.error import GLError
from pxr import Gf, Glf, Sdf, Tf
from pxr.Usdviewq.qt import QGLFormat, QGLWidget, QtCore, QtGui, QtWidgets

from ._schema_pin_names import (
    get_display_pin_name_for_property_name,
    resolve_property_name_for_display_pin,
)
from .constants import MAX_RENDER_DEPTH
from .linkRenderer import computeLinkCurveBounds, LinkRenderer, sampleLinkCurve
from .nodeFactory import NodeFactory
from .nodeGraph import (
    createNodeRenderer,
    NodeGraph,
    subscribe_edit_target_warning,
    unsubscribe_edit_target_warning,
    warn_if_non_persistent_edit_target,
)
from .nodeGraphBlueprint import NodeGraphBlueprint
from .nodeGraphJson import NodeGraphJson
from .nodeLibs.registry import NodeLibraryRegistry
from .noodlesConfig import NoodlesConfig
from .noodlesPreferences import NoodlesPreferences
from .pinUtils import (
    arrowhead_points,
    collect_links_for_prim,
    direction_hint_candidate_names,
    get_relationship_link_metadata,
    is_hidden_namespace_pin,
    is_multi_target_relationship_input_pin,
    is_relationship_input_pin,
    is_relationship_pin,
    split_direction_hint,
)
from .textRenderer import TextRenderer
from .usdNoticeHandler import UsdNoticeHandler
from .utils import M, MenuBuilder
from .widgets import (
    GroupSticker,
    GroupStickerRenderer,
    HotkeyPopup,
    Minimap,
    NavigationHotkeyPopup,
    NodeCreationHotbox,
    StatusOverlay,
)


def _push_undo_command(description, do_func, undo_func):
    """Create and push a LambdaCommand to the undo manager.

    Args:
        description: Human-readable description for undo/redo menu.
        do_func: Callable invoked on redo (re-applies the edit).
        undo_func: Callable invoked on undo (reverses the edit).
    """
    try:
        from pxr.UsdNoodles._usdNoodles import pushLambdaCommand

        pushLambdaCommand(description, do_func, undo_func)
        Tf.Status(f"Pushed undo command: {description}")
    except ImportError:
        Tf.Warn(f"Undo not available (ImportError), skipped: {description}")
    except Exception as e:
        Tf.Warn(f"Failed to push undo command '{description}': {e}")


def _make_prim_activation_undo(gv, paths, activate):
    """Return a callable that activates or deactivates prims at *paths*.

    The activation opinion is authored to the stage's current edit target.

    Args:
        gv: GraphView reference (prevents closure over self).
        paths: List of prim path strings.
        activate: True to set prims active, False to deactivate.
    """

    def _fn(_paths=paths, _activate=activate):
        stage = gv.nodeGraph.getStage()
        if not stage:
            return
        for path_str in _paths:
            prim = stage.GetPrimAtPath(path_str)
            if prim:
                prim.SetActive(_activate)

    return _fn


def _make_connection_edit(
    gv,
    src_id,
    src_port,
    tgt_id,
    tgt_port,
    create,
    *,
    source_property_name="",
    target_property_name="",
):
    """Return a callable that creates or deletes a single USD connection.

    The connection is authored on the stage's current edit target. If the
    user has selected a sublayer, the connection lands there.

    Args:
        create: True to create the connection, False to delete it.
    """

    def _fn():
        stage = gv.nodeGraph.getStage()
        if not stage:
            return
        out_p = stage.GetPrimAtPath(src_id)
        in_p = stage.GetPrimAtPath(tgt_id)
        if not (out_p and in_p):
            return
        lib = gv._findLibraryForPrim(in_p)
        if not lib:
            return
        connection_kwargs = {}
        if source_property_name or target_property_name:
            connection_kwargs = {
                "source_property_name": source_property_name,
                "target_property_name": target_property_name,
            }
        if create:
            lib.create_connection(
                stage, out_p, src_port, in_p, tgt_port, **connection_kwargs
            )
        else:
            lib.delete_connection(
                stage, out_p, src_port, in_p, tgt_port, **connection_kwargs
            )

    return _fn


def _make_reconnect_edits(
    gv,
    old_src,
    old_src_port,
    old_tgt,
    old_tgt_port,
    new_src,
    new_src_port,
    new_tgt,
    new_tgt_port,
    *,
    sourcePropertyName="",
    targetPropertyName="",
):
    """Return (redo, undo) callables for a reconnection."""
    delete_old = _make_connection_edit(
        gv, old_src, old_src_port, old_tgt, old_tgt_port, create=False
    )
    create_old = _make_connection_edit(
        gv, old_src, old_src_port, old_tgt, old_tgt_port, create=True
    )
    delete_new = _make_connection_edit(
        gv,
        new_src,
        new_src_port,
        new_tgt,
        new_tgt_port,
        create=False,
        source_property_name=sourcePropertyName,
        target_property_name=targetPropertyName,
    )
    create_new = _make_connection_edit(
        gv,
        new_src,
        new_src_port,
        new_tgt,
        new_tgt_port,
        create=True,
        source_property_name=sourcePropertyName,
        target_property_name=targetPropertyName,
    )

    def redo():
        delete_old()
        create_new()
        gv.linksChanged = True
        gv.update()

    def undo():
        delete_new()
        create_old()
        gv.linksChanged = True
        gv.update()

    return redo, undo


class _ProfileSection:
    """Context manager that records elapsed time into a RenderProfiler."""

    __slots__ = ("_profiler", "_name", "_t0")

    def __init__(self, profiler, name):
        self._profiler = profiler
        self._name = name

    def __enter__(self):
        self._t0 = time.perf_counter()
        return self

    def __exit__(self, *_exc):
        elapsed_ms = (time.perf_counter() - self._t0) * 1000.0
        self._profiler.recordTime(self._name, elapsed_ms)
        return False


class GraphView(QGLWidget):
    # Emitted whenever ``self.nodeGraph`` is reassigned (e.g. when a new stage
    # or blueprint is loaded and the underlying NodeGraph instance is swapped).
    # Listeners that subscribed to NodeGraph callbacks need this so they can
    # re-register on the new instance.
    nodeGraphReplaced = QtCore.Signal()

    # Per-instance cache for the ortho projection matrices. A single frame runs
    # many render passes (nodes, links, text, stickers, port highlights,
    # overlays), each of which previously rebuilt the same QMatrix4x4 via
    # .ortho() ~6x/frame. The matrices are memoized and keyed on the view state
    # that defines them, so the cache self-invalidates on any pan/zoom/resize
    # without explicit invalidation hooks. These class-level defaults are
    # shadowed by per-instance values on the first call (None is immutable, so
    # no cross-instance sharing).
    _worldProjCacheKey = None
    _worldProjCache = None
    _screenProjCacheKey = None
    _screenProjCache = None
    _BACKDROP_PADDING = 80.0

    @property
    def nodeGraph(self):
        return self._nodeGraph

    @nodeGraph.setter
    def nodeGraph(self, value):
        self._nodeGraph = value
        self.nodeGraphReplaced.emit()

    def __init__(self, parent=None, settingsObject=None):
        super().__init__(parent)

        # Initialize backing field before any property setter touches it.
        self._nodeGraph = None
        self._assetsPath = None

        glFormat = QGLFormat()
        glFormat.setVersion(3, 2)
        glFormat.setProfile(QGLFormat.OpenGLContextProfile.CoreProfile)
        glFormat.setSamples(4)
        # needed on osx
        QGLFormat.setDefaultFormat(glFormat)

        self.setFormat(glFormat)

        from .noodlesConfig import NoodlesConfig

        # Initialize configuration system if settings provided
        if settingsObject:
            NoodlesConfig.initialize(settingsObject, graphView=self)

        self.zoom = 1.0
        self.panX = 0.0
        self.panY = 0.0
        self.lastMousePos = QtCore.QPoint()

        self.nodeGraph = NodeGraph()

        # Show a toast popup when an authoring site lands an opinion on a
        # non-persistent layer (session / anonymous). Subscriber is module-
        # level so widgets/models without a back-reference to this view can
        # still surface the warning. Removed in closeEvent below.
        subscribe_edit_target_warning(self._showWarningPopup)

        self._currentPrimPath = None

        self.drawLinks = True
        self.drawNodes = True
        self.drawText = True

        self.animator = Animator()

        self.linkDimming = self.animator.addProperty()
        self.linkDimming.speed = 1.0
        self.linkDimming.target = 1.0
        self.linkDimming.current = 1.0

        self.initialized = False

        self.shaderLibrary = None
        self.linkRenderer = LinkRenderer()
        self.textRenderer = TextRenderer()
        self.statusOverlay = StatusOverlay()

        # Fallback renderer for nodes without a custom renderer, and the source
        # of the global port-hit extent / link-circle radius. Built through the
        # same factory as node renderers so it honors NoodlesConfig too (default
        # style; equals the struct defaults on an unmodified config).
        self.defaultRenderer = createNodeRenderer("default")

        self.draggingNodes = False
        self.spacePressed = False
        self.ctrlPressedOnMouseDown = False
        self.mousePosOnMouseDown = QtCore.QPoint()
        self.nodeIdUnderCursor = ""

        self.marqueeActive = False
        self.marqueeStart = QtCore.QPointF()
        self.marqueeCurrent = QtCore.QPointF()
        self._marqueeBaseSelectedNodes = set()
        self._marqueeBaseSelectedLinks = set()
        self._marqueePrevSelectedNodes = set()
        self._marqueePrevSelectedLinks = set()

        self.textChanged = True

        # Get pan animation speed from config
        panSpeed = NoodlesConfig.get("panAnimationSpeed", 18.0)
        self._panXAnim = self.animator.addProperty()
        self._panXAnim.speed = panSpeed
        self._panXAnim.target = 0.0
        self._panXAnim.current = 0.0

        self._panYAnim = self.animator.addProperty()
        self._panYAnim.speed = panSpeed
        self._panYAnim.target = 0.0
        self._panYAnim.current = 0.0

        self._panAnimating = False

        self.hotkeyPopup = HotkeyPopup(self.animator)
        self.hotkeyPopup.set_parent_widget(self)

        self.navigationHotkeyPopup = NavigationHotkeyPopup(self.animator)

        self.frameTimer = QtCore.QElapsedTimer()

        self._usdviewApi = None

        self.minimap = Minimap(self)

        # Node creation hotbox with library system
        # Discover node libraries via plugInfo.json registry
        self._nodeLibraries = NodeLibraryRegistry().discover()

        # Restore persisted library enable/disable state
        enabled_names = NoodlesConfig.get("enabledLibraries", None)
        if enabled_names is not None:
            for lib in self._nodeLibraries:
                lib.enabled = lib.get_name() in enabled_names

        # Initialize node factory for creating nodes from USD prims
        self._nodeFactory = NodeFactory(self._nodeLibraries)

        self.nodeCreationHotbox = NodeCreationHotbox(self.animator)
        self.nodeCreationHotbox.set_parent_widget(self)
        self._updateNodeCreationHotbox()  # Configure from libraries
        self.nodeCreationHotbox.on_create = self._createNodeFromHotbox

        self.setAttribute(QtCore.Qt.WA_Hover, True)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setMouseTracking(True)
        self.setMinimumSize(400, 300)

        # Flag to prevent recursive selection sync
        self._syncingSelection = False
        # Canvas click resets usdview prim selection to '/'; cache the last real selection.
        self._lastPrimTreeSelection = []

        self._previousActiveWindow = None

        self._linkSelectionMode = LinkSelectionMode.NODES_ONLY
        self._selectedLinks = set()
        self._selectedNodes = set()
        self._linkUnderCursor = -1
        # Gate for the per-frame link snapshot sync (see _syncLinksIfNeeded):
        # _linksNeedSync is set whenever a rendered link field (selection,
        # highlight, endpoints, membership) changes in place; _lastSyncedLinkHover
        # tracks the hovered index already mirrored to C++. Both start "dirty" so
        # the first paint mirrors the links.
        self._linksNeedSync = True
        self._lastSyncedLinkHover = -1
        # Gate for the C++ port-highlight rebuild (see _renderPortHighlightsCpp):
        # set when connections change in place; hover / drag / content are derived
        # live. Starts dirty so the first paint builds the highlight geometry.
        self._portHighlightsDirty = True
        # Watermarks for _reconcileMovedNodes: the global move generation last
        # reconciled (O(1) skip when nothing moved) and the per-node position
        # version already applied to the spatial index / links / highlights.
        self._reconcilePositionGen = 0
        self._reconciledPositionVersions = {}

        # Port hit radius multiplier default (read live from config in _updateHoveredPort)
        self._portHitRadiusMultiplierDefault = 1.5

        # Port hover state for highlighting
        self._hoveredPort = None  # tuple: (nodeId, portName, isOutput) or None
        self._hoveredPortColor = (1.0, 1.0, 0.0, 1.0)  # Yellow

        # Property tooltip state
        self._tooltipProperty = None  # (nodeId, pinName) or None
        self._tooltipScreenPos = None  # QPoint for tooltip position
        self._tooltipTimer = QtCore.QTimer()
        self._tooltipTimer.setSingleShot(True)
        self._tooltipTimer.setInterval(500)
        self._tooltipTimer.timeout.connect(self._showPropertyTooltip)

        # Link creation/dragging state
        self._draggingLink = False
        self._dragLinkSourceNode = None  # nodeId
        self._dragLinkSourcePort = None  # portName
        self._dragLinkSourceIsOutput = None  # bool
        self._dragLinkTempLink = None  # Reserved temporary LinkData (initialized below)
        self._dragLinkCursorPos = None  # Current cursor world position
        self._dragLinkValidTarget = False  # Is current hover a valid connection?
        self._dragLinkSyntheticTarget = None  # nodeId, portName, isOutput
        self._dragLinkResolvedTarget = None  # nodeId, portName, isOutput, propertyName

        # Initialize temporary link object (not added to self.links array)
        self._dragLinkTempLink = LinkData()
        self._dragLinkTempLink.sourceNodeId = ""
        self._dragLinkTempLink.targetNodeId = ""
        self._dragLinkTempLink.sourcePort = ""
        self._dragLinkTempLink.targetPort = ""
        self._dragLinkTempLink.sourcePropertyName = ""
        self._dragLinkTempLink.targetPropertyName = ""
        self._dragLinkTempLink.is_relationship_link = False

        # Link reconnection state (click-on-link to re-drag)
        self._reconnectingLink = False
        self._reconnectOldSourceNodeId = None
        self._reconnectOldSourcePort = None
        self._reconnectOldTargetNodeId = None
        self._reconnectOldTargetPort = None
        self._reconnectOldSourcePropertyName = ""
        self._reconnectOldTargetPropertyName = ""

        # Frame in prim tree option - when enabled, selecting nodes will scroll
        # the prim tree view to show the selected prim
        self._frameInPrimTreeOnSelect = NoodlesConfig.get(
            "frameInPrimTreeOnSelect", True
        )

        # Track last jump direction for navigation (default to "target" for forward traversal)
        # "target" means: after jumping, select outputs to continue forward
        # "source" means: after jumping, select inputs to continue backward
        self._lastJumpDirection = "target"

        # Track the last selected node for link navigation
        # This allows pressing 1,2,3 etc. to switch links from the same node
        self._lastSelectedNode = None
        # Track the last pin index and cycle index for cycling through multiple links on same pin
        self._lastPinIndex = -1
        self._lastLinkCycleIndex = 0

        # Navigation history: track which link was used to arrive at the current node
        # This allows going back to the same link instead of just the first one
        # _lastInputLinkIndex: The link index we used when arriving via an input (going forward)
        # _lastOutputLinkIndex: The link index we used when arriving via an output (going backward)
        self._lastInputLinkIndex = -1
        self._lastOutputLinkIndex = -1

        # Track the last dangling link that was selected via navigation
        # This allows detecting when user tries to "follow" a dangling link a second time
        self._lastDanglingLinkIndex = -1
        self._danglingLinkFollowCount = 0

        self._contextMenu = None
        self._contextMenuActions = {}
        self._initContextMenu()

        self._setupShortcuts()

        # Preferences dialog instance (created on first open)
        self._preferencesDlg = None

        # Cached settings (updated on settings change, not every frame)
        self._initCachedSettings()

        # Connect to settings changed signal for live updates
        settings = NoodlesConfig.settings()
        if settings:
            settings.signalSettingChanged.connect(self._onSettingsChanged)

        # Rendering
        self.fontAtlas = None

        # Profiler
        self._profiler = RenderProfiler(60)
        self._profiler.enabled = NoodlesConfig.get("enableRenderProfiling", False)
        self._profilingFrameCount = 0

        # Rendering managers - encapsulate rendering logic and data
        self._nodeRenderManager = NodeRenderManager()
        # Shared node-local coordinate frame: the text and node-quad paths sample
        # one transform texture, so a drag moves both with a single update.
        self._nodeTransformFrame = NodeTransformFrame()
        # Cached C++ RenderConfig shared by the node-quad and link paths; rebuilt
        # lazily after settings change (invalidated to None in _initCachedSettings).
        self._cachedRenderConfig = None
        self._groupStickerRenderer = GroupStickerRenderer()

        # When True, _clearRenderCache() defers its GL-side work to
        # _finishStageReload() instead of running it inline.  Used during
        # _onStageReplaced() to avoid issuing glDelete*/glGen* against the
        # NVIDIA Windows driver while the stageView's renderer is mid-teardown.
        self._deferRenderCacheClear = False
        self._renderCacheClearPending = False

        # Spatial acceleration structure for selection operations
        self._spatialIndex = SpatialIndex(maxDepth=4, maxItems=20)

        # Z-order counter for node layering (bring-to-front on selection)
        self._nextZOrder = 0

        # USD Notice handler for responding to external USD changes
        self._noticeHandler = UsdNoticeHandler(self)

        # Register Ctrl+S shortcut for saving using QAction
        self._saveAction = QtGui.QAction("Save Stage", self)
        self._saveAction.setShortcut(QtGui.QKeySequence.Save)
        self._saveAction.setShortcutContext(QtCore.Qt.WidgetWithChildrenShortcut)
        self._saveAction.triggered.connect(self._saveStage)
        self.addAction(self._saveAction)

    def _updateNodeCreationHotbox(self):
        """
        Update the node creation hotbox configuration from enabled libraries.

        This method iterates through all enabled node libraries, collects their
        families and node types, and configures the hotbox to display them.
        Library references are embedded in the node data for later use.
        """
        # Collect all node types with library references from enabled libraries
        node_items = []

        for library in self._nodeLibraries:
            if not library.enabled:
                continue

            # Get families from this library
            families = library.get_families()

            for family in families:
                # Get node types for this family
                node_types = library.get_node_types(family)

                if not node_types:
                    continue

                # Create node items with embedded library references
                for node_type in node_types:
                    node_item = {
                        "name": node_type["name"],
                        "family": family,
                        "identifier": node_type["identifier"],
                        "library": library,  # Reference to the library for creation
                    }
                    node_items.append(node_item)

        # Configure the hotbox with the collected node items
        self.nodeCreationHotbox.configure_from_libraries(node_items)

    @property
    def nodes(self):
        return self.nodeGraph.nodes

    @property
    def links(self):
        return self.nodeGraph.links

    @property
    def groupStickers(self):
        return self.nodeGraph.groupStickers

    @property
    def linksChanged(self):
        return self.nodeGraph.linksChanged

    @linksChanged.setter
    def linksChanged(self, value):
        self.nodeGraph.linksChanged = value

    @property
    def linkSelectionMode(self):
        return self._linkSelectionMode

    @linkSelectionMode.setter
    def linkSelectionMode(self, mode):
        self._linkSelectionMode = mode
        self._updateLinkSelectionFromNodes()
        self.update()

    def _updateLinkSelectionFromNodes(self):
        with _ProfileSection(self._profiler, "Update Link Selection"):
            for link in self.links:
                link.highlighted = False
            # Highlight flags changed on the link snapshot; re-mirror to C++.
            self._linksNeedSync = True

            if self._linkSelectionMode == LinkSelectionMode.NODES_ONLY:
                return

            selectedNodeIds = {
                nodeId for nodeId, node in self.nodes.items() if node.selected
            }

            if not selectedNodeIds:
                return

            for _i, link in enumerate(self.links):
                dataSource = getattr(link, "data_sourceNodeId", None)
                dataTarget = getattr(link, "data_targetNodeId", None)

                sourceSelected = dataSource in selectedNodeIds if dataSource else False
                targetSelected = dataTarget in selectedNodeIds if dataTarget else False

                shouldHighlight = False
                if self._linkSelectionMode == LinkSelectionMode.WITH_ALL_LINKS:
                    shouldHighlight = sourceSelected or targetSelected
                elif self._linkSelectionMode == LinkSelectionMode.WITH_INPUTS:
                    shouldHighlight = targetSelected
                elif self._linkSelectionMode == LinkSelectionMode.WITH_OUTPUTS:
                    shouldHighlight = sourceSelected

                if shouldHighlight:
                    link.highlighted = True

    def _selectLink(self, linkIndex, addToSelection=False):
        if linkIndex < 0 or linkIndex >= len(self.links):
            return

        if not addToSelection:
            # Clear existing link selection
            for link in self.links:
                link.selected = False
            self._selectedLinks.clear()

        self.links[linkIndex].selected = True
        self._selectedLinks.add(linkIndex)
        self._linksNeedSync = True

        # Sync link selection to primtree and property editor
        self._syncLinkSelectionToPrimtree()

        self.update()

    def _toggleLinkSelection(self, linkIndex):
        if linkIndex < 0 or linkIndex >= len(self.links):
            return

        link = self.links[linkIndex]
        link.selected = not link.selected

        if link.selected:
            self._selectedLinks.add(linkIndex)
        else:
            self._selectedLinks.discard(linkIndex)
        self._linksNeedSync = True

        # Sync link selection to primtree and property editor
        self._syncLinkSelectionToPrimtree()

        self.update()

    def _syncLinkSelectionToPrimtree(self):
        """Sync selected links to the primtree and property editor.

        When a link is selected, this selects:
        1. The prim (node) that owns the connection attribute
        2. The specific attribute that defines the connection

        Note: There is no public API to expand the selected property in the
        property view to show relationship targets/connection paths. This could
        be done by accessing internal APIs:
            appController = usdviewApi._UsdviewApi__appController
            propertyView = appController._ui.propertyView  # QTreeWidget
            item.setExpanded(True)  # On the matching QTreeWidgetItem
        See appController._updatePropertyViewSelection() for reference.
        """
        if self._syncingSelection:
            return

        # Check if the current graph supports prim tree syncing
        if not self.nodeGraph.syncSelectionToPrimTree:
            return

        if not self._usdviewApi:
            return

        # Get selected links
        selectedLinks = [
            self.links[i] for i in self._selectedLinks if i < len(self.links)
        ]
        if not selectedLinks:
            return

        try:
            dataModel = self._usdviewApi.dataModel
            if not dataModel or not dataModel.selection:
                return

            stage = self._usdviewApi.stage
            if not stage:
                return

            self._syncingSelection = True
            try:
                primsToSelect = []
                propsToSelect = []

                for link in selectedLinks:
                    primPath = (
                        getattr(link, "propertyOwnerNodeId", "") or link.sourceNodeId
                    )

                    # Skip virtual Blueprint I/O nodes (they have # in the path)
                    if "#" in primPath:
                        continue

                    prim = stage.GetPrimAtPath(primPath)
                    if not prim or not prim.IsValid():
                        continue

                    primsToSelect.append(prim)

                    propertyName = getattr(link, "propertyName", "")
                    if getattr(link, "is_relationship_link", False) and propertyName:
                        relationship = prim.GetRelationship(propertyName)
                        if relationship and relationship.IsValid():
                            propsToSelect.append(relationship)
                        continue

                    if propertyName:
                        attr = prim.GetAttribute(propertyName)
                        if attr and attr.IsValid():
                            propsToSelect.append(attr)
                            continue

                    # Legacy fallback for attribute-only links that predate
                    # explicit property owner/name metadata. Try every hint
                    # spelling plus the bare/literal name so namespaced pins
                    # (e.g. rig1:space) and in:/out: hints also resolve.
                    isInput = getattr(link, "is_input_link", True)
                    pinName = link.sourcePort
                    for attrName in direction_hint_candidate_names(
                        pinName, is_input=isInput
                    ):
                        attr = prim.GetAttribute(attrName)
                        if attr and attr.IsValid():
                            propsToSelect.append(attr)
                            break

                # Select prims
                if primsToSelect:
                    with dataModel.selection.batchPrimChanges:
                        dataModel.selection.clearPrims()
                        for prim in primsToSelect:
                            dataModel.selection.addPrim(prim)

                    # Frame the first prim in the prim tree if option is enabled
                    if self._frameInPrimTreeOnSelect:
                        self._frameInPrimTree(primsToSelect[0])

                # Select properties
                if propsToSelect:
                    with dataModel.selection.batchPropChanges:
                        dataModel.selection.clearProps()
                        for prop in propsToSelect:
                            dataModel.selection.addProp(prop)

            finally:
                self._syncingSelection = False

        except Exception:
            self._syncingSelection = False

    def clearLinkSelection(self):
        for link in self.links:
            link.selected = False
        self._selectedLinks.clear()
        self._linksNeedSync = True

    def _getFirstSelectedLink(self):
        if not self._selectedLinks:
            return None

        # Get the first selected link index
        firstIndex = min(self._selectedLinks)
        if firstIndex < len(self.links):
            return self.links[firstIndex]
        return None

    def _centerOnNode(self, node, animate=True):
        nodeCenter = node.position + node.size * 0.5

        # Calculate the viewport size in world coordinates
        vpWidth = self.width() / self.zoom
        vpHeight = self.height() / self.zoom

        # Calculate target pan position
        targetPanX = nodeCenter[0] - vpWidth * 0.5
        targetPanY = nodeCenter[1] - vpHeight * 0.5

        if animate:
            # Set up animated pan with cached speed
            panSpeed = self._cachedPanAnimationSpeed

            self._panXAnim.current = self.panX
            self._panXAnim.target = targetPanX
            self._panXAnim.speed = panSpeed

            self._panYAnim.current = self.panY
            self._panYAnim.target = targetPanY
            self._panYAnim.speed = panSpeed

            self._panAnimating = True
        else:
            # Immediate jump
            self.panX = targetPanX
            self.panY = targetPanY
            self._panAnimating = False

        self.update()

    def _showHotkeyPopup(self, keyText, direction):
        if direction == "left":
            message = f"<- {keyText}"
        else:
            message = f"{keyText} ->"

        cursorPos = self.mapFromGlobal(QtGui.QCursor.pos())
        self.hotkeyPopup.show(message, cursorPos)
        self.update()

    # Warning toasts (non-persistent edit target, save errors) linger 1s longer
    # than navigation popups so users have time to read them.
    _WARNING_POPUP_DURATION_MS = 1400

    def _showPopupMessage(self, message, duration_ms=None):
        cursorPos = self.mapFromGlobal(QtGui.QCursor.pos())
        self.hotkeyPopup.show(message, cursorPos, duration_ms=duration_ms)
        self.update()

    def _showWarningPopup(self, message):
        self._showPopupMessage(message, duration_ms=self._WARNING_POPUP_DURATION_MS)

    def timerEvent(self, event):
        if self.hotkeyPopup.handle_timer_event(event.timerId()):
            self.update()

    def navigateBackward(self):
        """Navigate backward (toward inputs/sources) using the [ key.

        Behavior:
        - If a link is selected: jump to the source node (data provider)
        - If a node is selected: select the first input link
        - If the node has no inputs: swap direction and select first output link
        """
        link = self._getFirstSelectedLink()
        node = self._getFirstSelectedNode()

        if link and not node:
            self._jumpToSourceNode(link)
        elif node:
            self._selectFirstLinkInDirection(node, "input")

    def navigateForward(self):
        """Navigate forward (toward outputs/targets) using the ] key.

        Behavior:
        - If a link is selected: jump to the target node (data receiver)
        - If a node is selected: select the first output link
        - If the node has no outputs: swap direction and select first input link
        """
        link = self._getFirstSelectedLink()
        node = self._getFirstSelectedNode()

        if link and not node:
            self._jumpToTargetNode(link)
        elif node:
            self._selectFirstLinkInDirection(node, "output")

    def _jumpToSourceNode(self, link):
        self._showHotkeyPopup("[", "left")

        # Track direction for subsequent pin selection
        self._lastJumpDirection = "source"

        # Get the data source node (the node providing data via its output)
        sourceNodeId = getattr(link, "data_sourceNodeId", None)

        # Check if this is a dangling link
        if link.isDangling and sourceNodeId and sourceNodeId not in self.nodes:
            # Get the link index
            linkIndex = -1
            for i, l in enumerate(self.links):
                if l is link:
                    linkIndex = i
                    break

            # Check if this is the same dangling link we tried to follow before
            if linkIndex == self._lastDanglingLinkIndex:
                self._danglingLinkFollowCount += 1
                if self._danglingLinkFollowCount >= 1:
                    # Second attempt - add the missing node
                    if self._addNodeFromDanglingLink(link):
                        self._lastDanglingLinkIndex = -1
                        self._danglingLinkFollowCount = 0
                        # Now jump to the newly added node
                        if sourceNodeId in self.nodes:
                            self._jumpToSourceNodeInternal(link, sourceNodeId)
                    return
            else:
                # First time selecting this dangling link - just show it's selected
                self._lastDanglingLinkIndex = linkIndex
                self._danglingLinkFollowCount = 0
                self._showPopupMessage("Follow again to add node")
                return

        if not sourceNodeId or sourceNodeId not in self.nodes:
            return

        self._jumpToSourceNodeInternal(link, sourceNodeId)

    def _jumpToSourceNodeInternal(self, link, sourceNodeId):
        """Internal helper to complete the jump to source node."""
        sourceNode = self.nodes[sourceNodeId]

        # Record the link index we used to arrive at this node
        # Since we're going backward (to source), record this as the output link
        # we can use to go forward again
        for i, l in enumerate(self.links):
            if l is link:
                self._lastOutputLinkIndex = i
                break

        # Clear current selection and select the source node
        self.clearSelection()
        self.clearLinkSelection()
        sourceNode.selected = True
        self._selectedNodes.add(sourceNode.id)
        self._lastSelectedNode = sourceNode

        # Reset dangling link tracking
        self._lastDanglingLinkIndex = -1
        self._danglingLinkFollowCount = 0

        # Center on the node (maintains current zoom)
        self._centerOnNode(sourceNode)

        # Sync selection to primtree
        self._syncSelectionToPrimtree()

    def _jumpToTargetNode(self, link):
        self._showHotkeyPopup("]", "right")

        # Track direction for subsequent pin selection
        self._lastJumpDirection = "target"

        # Get the data target node (the node receiving data via its input)
        targetNodeId = getattr(link, "data_targetNodeId", None)

        # Check if this is a dangling link
        if link.isDangling and targetNodeId and targetNodeId not in self.nodes:
            # Get the link index
            linkIndex = -1
            for i, l in enumerate(self.links):
                if l is link:
                    linkIndex = i
                    break

            # Check if this is the same dangling link we tried to follow before
            if linkIndex == self._lastDanglingLinkIndex:
                self._danglingLinkFollowCount += 1
                if self._danglingLinkFollowCount >= 1:
                    # Second attempt - add the missing node
                    if self._addNodeFromDanglingLink(link):
                        self._lastDanglingLinkIndex = -1
                        self._danglingLinkFollowCount = 0
                        # Now jump to the newly added node
                        if targetNodeId in self.nodes:
                            self._jumpToTargetNodeInternal(link, targetNodeId)
                    return
            else:
                # First time selecting this dangling link - just show it's selected
                self._lastDanglingLinkIndex = linkIndex
                self._danglingLinkFollowCount = 0
                self._showPopupMessage("Follow again to add node")
                return

        if not targetNodeId or targetNodeId not in self.nodes:
            return

        self._jumpToTargetNodeInternal(link, targetNodeId)

    def _jumpToTargetNodeInternal(self, link, targetNodeId):
        """Internal helper to complete the jump to target node."""
        targetNode = self.nodes[targetNodeId]

        # Record the link index we used to arrive at this node
        # Since we're going forward (to target), record this as the input link
        # we can use to go backward again
        for i, l in enumerate(self.links):
            if l is link:
                self._lastInputLinkIndex = i
                break

        # Clear current selection and select the target node
        self.clearSelection()
        self.clearLinkSelection()
        targetNode.selected = True
        self._selectedNodes.add(targetNode.id)
        self._lastSelectedNode = targetNode

        # Reset dangling link tracking
        self._lastDanglingLinkIndex = -1
        self._danglingLinkFollowCount = 0

        # Center on the node (maintains current zoom)
        self._centerOnNode(targetNode)

        # Sync selection to primtree
        self._syncSelectionToPrimtree()

    def _selectFirstLinkInDirection(self, node, direction):
        """Select the first link in the given direction from a node.

        Args:
            node: The NodeModel to find links for.
            direction: "input" or "output"

        If we previously arrived at this node via a link in the requested direction,
        we'll return to that same link instead of selecting the first one.

        If no links exist in the requested direction, swaps to the opposite
        direction and selects from there instead.
        """
        # Check if we have a history link to return to
        historyLinkIndex = -1
        if direction == "input" and self._lastInputLinkIndex >= 0:
            # Check if this link still connects to the current node
            if self._lastInputLinkIndex < len(self.links):
                historyLink = self.links[self._lastInputLinkIndex]
                historyTarget = getattr(historyLink, "data_targetNodeId", None)
                if historyTarget == node.id:
                    historyLinkIndex = self._lastInputLinkIndex
        elif direction == "output" and self._lastOutputLinkIndex >= 0:
            # Check if this link still connects to the current node
            if self._lastOutputLinkIndex < len(self.links):
                historyLink = self.links[self._lastOutputLinkIndex]
                historySource = getattr(historyLink, "data_sourceNodeId", None)
                if historySource == node.id:
                    historyLinkIndex = self._lastOutputLinkIndex

        # If we have a valid history link, use it
        if historyLinkIndex >= 0:
            # Show popup with direction indicator
            popupKey = "[" if direction == "input" else "]"
            popupDirection = "left" if direction == "input" else "right"
            self._showHotkeyPopup(popupKey, popupDirection)

            # Update tracking
            self._lastJumpDirection = "source" if direction == "input" else "target"
            self._lastSelectedNode = node

            # Clear the history since we're using it now
            if direction == "input":
                self._lastInputLinkIndex = -1
            else:
                self._lastOutputLinkIndex = -1

            # Clear node selection and select the link
            self.clearSelection()
            self.clearLinkSelection()
            self._selectLink(historyLinkIndex, addToSelection=False)

            self.update()
            return

        # No history link - try the requested direction first
        links = self._getLinksForNodeFlat(node, direction)

        # If no links in requested direction, swap to opposite
        if not links:
            oppositeDirection = "output" if direction == "input" else "input"
            links = self._getLinksForNodeFlat(node, oppositeDirection)
            if links:
                direction = oppositeDirection  # Update for popup display

        if not links:
            return  # No links at all

        # Select the first link
        linkIndex, _link = links[0]

        # Show popup with direction indicator
        popupKey = "[" if direction == "input" else "]"
        popupDirection = "left" if direction == "input" else "right"
        self._showHotkeyPopup(popupKey, popupDirection)

        # Update tracking for 1,2,3 key navigation
        self._lastJumpDirection = "source" if direction == "input" else "target"
        self._lastSelectedNode = node
        self._lastPinIndex = 0
        self._lastLinkCycleIndex = 0

        # Clear node selection and select the link
        self.clearSelection()
        self.clearLinkSelection()
        self._selectLink(linkIndex, addToSelection=False)

        self.update()

    def _getFirstSelectedNode(self):
        for node in self.nodes.values():
            if node.selected:
                return node
        return None

    def _getLinksForNodeDirection(self, node, direction):
        pinsToLinks = {}

        for i, link in enumerate(self.links):
            if direction == "output":
                # Links where this node is the data source (outputs)
                dataSource = getattr(link, "data_sourceNodeId", None)
                if dataSource == node.id:
                    # Find pin index for sorting
                    pinName = link.sourcePort
                    pinIdx = self._findPinIndex(node.outputPins, pinName)
                    if pinIdx == -1:
                        pinIdx = 999  # Put unknown pins at end
                    if pinIdx not in pinsToLinks:
                        pinsToLinks[pinIdx] = []
                    pinsToLinks[pinIdx].append((i, link))
            else:  # direction == "input"
                # Links where this node is the data target (inputs)
                dataTarget = getattr(link, "data_targetNodeId", None)
                if dataTarget == node.id:
                    # For input links, the current node receives data
                    if link.sourceNodeId == node.id:
                        pinName = link.sourcePort
                    else:
                        pinName = link.targetPort
                    pinIdx = self._findPinIndex(node.inputPins, pinName)
                    if pinIdx == -1:
                        pinIdx = 999
                    if pinIdx not in pinsToLinks:
                        pinsToLinks[pinIdx] = []
                    pinsToLinks[pinIdx].append((i, link))

        # Sort links within each pin by link index for stable ordering
        for pinIdx in pinsToLinks:
            pinsToLinks[pinIdx].sort(key=lambda x: x[0])

        return pinsToLinks

    def _getLinksForNodeFlat(self, node, direction):
        pinsToLinks = self._getLinksForNodeDirection(node, direction)

        # Flatten to list sorted by pin index
        result = []
        for pinIdx in sorted(pinsToLinks.keys()):
            result.extend(pinsToLinks[pinIdx])

        return result

    def selectLinkByPinIndex(self, pinIndex, reverseDirection=False):
        """Select a link connected to the nth input or output of a node.

        Enhanced navigation features:
        - Uses _lastSelectedNode if no node is currently selected (allows 1,2,3 switching)
        - Supports reverseDirection (Shift+number) to go opposite direction
        - Falls back to opposite direction if current direction has no links
        - Cycles through multiple links on the same pin when pressed repeatedly

        Args:
            pinIndex: 0-based index of the pin (0 = first, 1 = second, etc.)
            reverseDirection: If True, use opposite direction from _lastJumpDirection
        """
        # Get the node to work with - either currently selected or last selected
        node = self._getFirstSelectedNode()

        if node is None and self._lastSelectedNode is not None:
            # No node selected, but we have a last selected node
            # This allows pressing 1,2,3 to switch links from the same node
            node = self._lastSelectedNode
        elif node is not None:
            # A node is selected - remember it for future use
            self._lastSelectedNode = node

        if not node:
            return

        # Determine primary direction based on last jump and reverse flag
        if self._lastJumpDirection == "target":
            primaryDirection = "output"
            fallbackDirection = "input"
        else:
            primaryDirection = "input"
            fallbackDirection = "output"

        # Reverse if Shift is held
        if reverseDirection:
            primaryDirection, fallbackDirection = fallbackDirection, primaryDirection

        # Get links for primary direction
        pinsToLinks = self._getLinksForNodeDirection(node, primaryDirection)

        # If no links in primary direction, try fallback
        if not pinsToLinks:
            pinsToLinks = self._getLinksForNodeDirection(node, fallbackDirection)

        if not pinsToLinks:
            return  # No links at all

        # Get sorted list of pin indices
        sortedPins = sorted(pinsToLinks.keys())

        if pinIndex >= len(sortedPins):
            return  # No pin at this index

        # Get the actual pin index from our sorted list
        actualPinIdx = sortedPins[pinIndex]
        linksForPin = pinsToLinks[actualPinIdx]

        if not linksForPin:
            return

        # Determine which link to select (cycling through multiple links on same pin)
        if pinIndex == self._lastPinIndex and len(linksForPin) > 1:
            # Same pin pressed again - cycle to next link
            self._lastLinkCycleIndex = (self._lastLinkCycleIndex + 1) % len(linksForPin)
        else:
            # Different pin - reset cycle
            self._lastPinIndex = pinIndex
            self._lastLinkCycleIndex = 0

        linkIndex, _link = linksForPin[self._lastLinkCycleIndex]

        # Determine popup direction and key text
        # If primaryDirection is "output", we're selecting outputs (arrow right)
        # If primaryDirection is "input", we're selecting inputs (arrow left)
        popupDirection = "right" if primaryDirection == "output" else "left"
        keyText = str(pinIndex + 1)  # Just show the number, arrow indicates direction
        self._showHotkeyPopup(keyText, popupDirection)

        # Clear node selection and select the link
        self.clearSelection()
        self.clearLinkSelection()
        self._selectLink(linkIndex, addToSelection=False)

        self.update()

    def setUsdviewApi(self, usdviewApi):
        self._usdviewApi = usdviewApi

        # Connect to usdview selection changes
        if usdviewApi and hasattr(usdviewApi, "dataModel"):
            dataModel = usdviewApi.dataModel
            if dataModel and hasattr(dataModel, "selection"):
                # Listen for selection changes in the primtree
                dataModel.selection.signalPrimSelectionChanged.connect(
                    self._onPrimSelectionChanged
                )
                # Listen for property selection changes
                dataModel.selection.signalPropSelectionChanged.connect(
                    self._onPropSelectionChanged
                )
            if hasattr(dataModel, "signalStageReplaced"):
                dataModel.signalStageReplaced.connect(self._onStageReplaced)

    def closeEvent(self, event):
        unsubscribe_edit_target_warning(self._showWarningPopup)
        super().closeEvent(event)

    def _onStageReplaced(self):
        # Block paint events at the Qt level.  Python-level guards
        # (self.initialized) are insufficient because QGLWidget's
        # internal event handling still touches the GL surface before
        # paintGL() runs, and on NVIDIA Windows drivers this can crash
        # if the stageView's context was just destroyed/recreated on
        # the same thread.
        self.setUpdatesEnabled(False)
        self.initialized = False

        self._noticeHandler.unregister()
        self._invalidateBackReferenceLinkDataCache()

        # Clear state that holds dead Usd.Prim / Usd.Stage references
        self._lastPrimTreeSelection = []
        self._lastSelectedNode = None
        self._selectedNodes.clear()
        self._selectedLinks.clear()
        self._linkUnderCursor = -1
        # Drop per-node position watermarks for the torn-down graph; the next
        # graph's nodes reconcile fresh (and this bounds the map's growth).
        self._reconciledPositionVersions.clear()
        self._lastPinIndex = -1
        self._lastLinkCycleIndex = 0
        self._lastInputLinkIndex = -1
        self._lastOutputLinkIndex = -1
        self._lastDanglingLinkIndex = -1

        stage = self._usdviewApi.stage if self._usdviewApi else None
        if stage is None:
            QtCore.QTimer.singleShot(0, self._finishStageReload)
            return

        # Reload the graph from the new stage using the same load path
        # that was used originally.  loadBlueprint/loadContainer/loadStage
        # replace the nodeGraph, re-register the notice handler, and
        # clear all render caches.
        #
        # We set _deferRenderCacheClear so that _clearRenderCache (called
        # from inside loadStage/loadBlueprint/loadContainer) skips its GL
        # work and only marks the clear as pending.  _finishStageReload
        # runs the real glDelete* calls after the QTimer tick, when the
        # NVIDIA driver is no longer in its non-functional window.
        from .nodeGraphBpContainer import NodeGraphBpContainer

        self._deferRenderCacheClear = True
        try:
            if (
                isinstance(self.nodeGraph, NodeGraphBpContainer)
                and self._currentPrimPath
            ):
                self.loadContainer(stage, self._currentPrimPath)
            elif self._currentPrimPath is not None:
                self.loadBlueprint(stage, self._currentPrimPath)
            else:
                self.loadStage(stage)
        except Exception as e:
            Tf.Warn(f"Failed to reload graph after stage replace: {e}")
        finally:
            self._deferRenderCacheClear = False

        # Defer re-enabling rendering to the next event-loop iteration.
        # On NVIDIA Windows drivers, the legacy QGLWidget context is
        # temporarily non-functional while the stageView's renderer is
        # being torn down and recreated on the same thread.  By the
        # time the event loop runs the deferred callback, the driver's
        # internal state has stabilized and makeCurrent() works again.
        self.initialized = False
        QtCore.QTimer.singleShot(0, self._finishStageReload)

    def _finishStageReload(self):
        # Do NOT set self.fontAtlas/self.shaderLibrary to None here.
        # CPython's refcounting would immediately call their C++
        # destructors (which call glDelete*) with no GL context
        # current, corrupting the NVIDIA driver state.  Instead let
        # initializeGL() replace them while the context IS current —
        # the old destructors then run safely.

        # If _onStageReplaced deferred the render-cache clear, run it
        # now that the driver state has stabilized.  Doing this inside
        # _onStageReplaced would race with the stageView's renderer
        # teardown and crash the NVIDIA driver on its worker thread.
        if self._renderCacheClearPending:
            self._renderCacheClearPending = False
            self.makeCurrent()
            self._nodeRenderManager.clearCache()
            self.doneCurrent()

        self.linksChanged = True
        self.textChanged = True
        self.initialized = True
        self.setUpdatesEnabled(True)
        self.update()

    def _onPropSelectionChanged(self):
        if self._syncingSelection:
            return

        if not self._usdviewApi:
            return

        try:
            dataModel = self._usdviewApi.dataModel
            if not dataModel or not dataModel.selection:
                return

            selectedProps = dataModel.selection.getProps()
            if not selectedProps:
                return

            self._syncingSelection = True
            try:
                linksToSelect = []

                for prop in selectedProps:
                    propPath = prop.GetPath()
                    primPath = str(propPath.GetPrimPath())
                    propName = prop.GetName()

                    # Classify the property: input/output direction hint, bare
                    # (dual pin), or any other namespace (kept-name pin nested
                    # under a header, e.g. rig1:space / xformOp:translate).
                    side, stripped = split_direction_hint(propName)
                    isInput = side == "input"
                    isOutput = side == "output"
                    isBare = side is None and ":" not in propName
                    isNamespaced = side is None and ":" in propName

                    if isInput or isOutput:
                        pinName = stripped
                    else:
                        pinName = propName  # bare or kept full namespaced name

                    # Bare and kept-name namespaced pins can sit on either side,
                    # so match both endpoints; hinted pins match their own side.
                    matchEitherSide = isBare or isNamespaced

                    for i, link in enumerate(self.links):
                        if matchEitherSide:
                            if (
                                link.sourceNodeId == primPath
                                and link.sourcePort == pinName
                            ) or (
                                link.targetNodeId == primPath
                                and link.targetPort == pinName
                            ):
                                linksToSelect.append(i)
                        elif (
                            link.sourceNodeId == primPath and link.sourcePort == pinName
                        ):
                            # Verify the link type matches
                            linkIsInput = getattr(link, "is_input_link", True)
                            if linkIsInput == isInput:
                                linksToSelect.append(i)

                if linksToSelect:
                    self.clearLinkSelection()

                    # Select the matching links
                    for linkIndex in linksToSelect:
                        if linkIndex < len(self.links):
                            self.links[linkIndex].selected = True
                            self._selectedLinks.add(linkIndex)

                    self.update()

            finally:
                self._syncingSelection = False

        except Exception:
            self._syncingSelection = False

    def _onPrimSelectionChanged(self):
        if self._syncingSelection:
            return

        if not self._usdviewApi:
            return

        try:
            dataModel = self._usdviewApi.dataModel
            if not dataModel or not dataModel.selection:
                return

            selectedPrims = dataModel.selection.getPrims()

            # Track last meaningful selection (non-root prims) for use by addNodesFromPrimTreeSelection
            nonRootPrims = [p for p in selectedPrims if p.GetPath().pathString != "/"]
            if nonRootPrims:
                self._lastPrimTreeSelection = nonRootPrims

            self._syncingSelection = True
            try:
                self.clearSelection()

                for prim in selectedPrims:
                    primPathStr = str(prim.GetPath())
                    if primPathStr in self.nodes:
                        self.nodes[primPathStr].selected = True
                        self._selectedNodes.add(primPathStr)

                self.update()
            finally:
                self._syncingSelection = False
        except Exception:
            self._syncingSelection = False

    def _handleUsdChanges(self, resyncedPaths, infoChangedPaths):
        """Handle USD changes detected via the Notice system.

        This method is called by UsdNoticeHandler when USD prims/properties
        change. It invalidates caches and triggers redraws as needed.

        Args:
            resyncedPaths: Set of path strings that had structural changes
                          (prims added/removed, connections changed)
            infoChangedPaths: Set of path strings that had property value changes
                             (positions, metadata)
        """
        # Ignore changes if we're in the middle of syncing selection
        # (those are likely our own edits)
        if self._syncingSelection:
            return

        needsLinkRebuild = False
        needsRepaint = False
        needsTextUpdate = False

        # Resynced paths represent structural USD changes (prims added/removed,
        # connection authored/cleared). Drop the cached per-prim back-reference
        # scan so _addBackReferenceDanglingLinks re-traverses the stage on the
        # next _rebuildLinks. Info-only changes (positions, metadata) do not
        # affect what USD authored, so they leave the cache intact.
        if resyncedPaths:
            self._invalidateBackReferenceLinkDataCache()

        # Process resynced paths (structural changes - more expensive)
        for pathStr in resyncedPaths:
            # Check if this is a node we have in our graph
            if pathStr in self.nodes:
                node = self.nodes[pathStr]
                node.invalidateCache()
                needsLinkRebuild = True
                needsRepaint = True
                needsTextUpdate = True
            else:
                # Check if this is a child/parent path of a node we care about
                # (e.g., a new prim was added under a container we're viewing)
                for nodeId in self.nodes:
                    if pathStr.startswith(nodeId) or nodeId.startswith(pathStr):
                        self.nodes[nodeId].invalidateCache()
                        needsLinkRebuild = True
                        needsRepaint = True
                        needsTextUpdate = True
                        break

        # Process info-only changes (property value changes - cheaper)
        for pathStr in infoChangedPaths:
            # Property paths look like: /Prim.property or /Prim.inputs:foo
            if "." in pathStr:
                primPath, propName = pathStr.rsplit(".", 1)
            else:
                primPath = pathStr
                propName = ""

            if primPath in self.nodes:
                node = self.nodes[primPath]

                # Check what kind of property changed
                if "ui:nodegraph:node:pos" in propName:
                    # Position changed - invalidate position cache
                    node._cachedPosition = None
                    needsRepaint = True
                elif "ui:nodegraph:node:expansionState" in propName:
                    # Expansion state changed - update title collapse
                    try:
                        from pxr import UsdUI

                        exp_attr = UsdUI.NodeGraphNodeAPI(
                            node._prim
                        ).GetExpansionStateAttr()
                        if exp_attr.IsValid() and exp_attr.HasAuthoredValue():
                            state = str(exp_attr.Get())
                            collapsed = state in ("closed", "minimized")
                        else:
                            collapsed = False
                        if node._title_collapsed != collapsed:
                            node._title_collapsed = collapsed
                            node.titleCollapsed = collapsed
                            needsRepaint = True
                            needsTextUpdate = True
                            needsLinkRebuild = True
                    except Exception:
                        pass
                elif self._isPinPropertyChange(propName):
                    # Pin or connection changed (direction-hint, relationship,
                    # or any other namespaced pin such as rig1:space)
                    node.invalidateCache()
                    needsLinkRebuild = True
                    needsRepaint = True
                    needsTextUpdate = True
                elif "info:implementationSource" in propName:
                    # Node type changed
                    node._cachedType = None
                    needsRepaint = True
                    needsTextUpdate = True
                else:
                    # Some other property - do a full cache invalidation to be safe
                    node.invalidateCache()
                    needsRepaint = True
                    # Bare attributes may have connections - trigger link rebuild
                    if ":" not in propName and node._prim:
                        attr = node._prim.GetAttribute(propName)
                        if attr and attr.IsValid() and attr.HasAuthoredConnections():
                            needsLinkRebuild = True

        # Apply the updates
        if needsLinkRebuild:
            self.linksChanged = True
            self._clearRenderCache()

        if needsTextUpdate:
            self.textChanged = True

        if needsRepaint:
            self.update()

    def _syncSelectionToPrimtree(self):
        with _ProfileSection(self._profiler, "Sync Selection to Primtree"):
            if self._syncingSelection:
                return

            if not self.nodeGraph.syncSelectionToPrimTree:
                return

            if not self._usdviewApi:
                return

            try:
                dataModel = self._usdviewApi.dataModel
                if not dataModel or not dataModel.selection:
                    return

                stage = self._usdviewApi.stage
                if not stage:
                    return

                # Get selected nodes
                selectedNodes = [node for node in self.nodes.values() if node.selected]

                self._syncingSelection = True
                try:
                    primsToSelect = []

                    for node in selectedNodes:
                        # Skip virtual Blueprint I/O nodes (they have # in the path)
                        if "#" in node.id:
                            continue

                        prim = stage.GetPrimAtPath(node.id)
                        if not prim or not prim.IsValid():
                            continue

                        primsToSelect.append(prim)

                    # Select prims
                    if primsToSelect:
                        with dataModel.selection.batchPrimChanges:
                            dataModel.selection.clearPrims()
                            for prim in primsToSelect:
                                dataModel.selection.addPrim(prim)

                        # Frame the first prim in the prim tree if option is enabled
                        if self._frameInPrimTreeOnSelect:
                            self._frameInPrimTree(primsToSelect[0])
                    elif not selectedNodes:
                        # No nodes selected - clear selection
                        with dataModel.selection.batchPrimChanges:
                            dataModel.selection.clearPrims()

                finally:
                    self._syncingSelection = False

            except Exception:
                self._syncingSelection = False

    def _frameInPrimTree(self, prim):
        if not self._usdviewApi or not prim:
            return

        try:
            appController = getattr(
                self._usdviewApi, "_UsdviewApi__appController", None
            )
            if not appController:
                return

            primPath = prim.GetPath()
            item = appController._getItemAtPath(primPath, ensureExpanded=True)

            if (
                item
                and hasattr(appController, "_ui")
                and hasattr(appController._ui, "primView")
            ):
                primView = appController._ui.primView
                # Scroll to make the item visible, centered if possible
                primView.scrollToItem(item)

        except Exception:
            pass

    def enterEvent(self, event):
        app = QtWidgets.QApplication.instance()
        if app:
            self._previousActiveWindow = app.activeWindow()

        topLevel = self.window()
        if topLevel:
            topLevel.activateWindow()
        self.setFocus(QtCore.Qt.MouseFocusReason)

        super().enterEvent(event)

    def leaveEvent(self, event):
        if self._previousActiveWindow and self._previousActiveWindow.isVisible():
            self._previousActiveWindow.activateWindow()
        self._previousActiveWindow = None

        self._tooltipTimer.stop()
        self._tooltipProperty = None
        QtWidgets.QToolTip.hideText()

        super().leaveEvent(event)

    def _resolveShortcutConflicts(self, shortcuts):
        """Reset stored shortcuts that collide with a later-defined action.

        When a default shortcut is reassigned between releases (e.g. "D" moved
        from shortcutToggleLinkDimming to shortcutRemoveFromGraph), a user's
        persisted state can still bind the key to the original action. The
        manual dispatch in keyPressEvent iterates _shortcutActions in insertion
        order and triggers the first match, which means the older action wins
        and the new one is silently masked.

        Walk the shortcuts list and, for each duplicate normalized key
        sequence, reset every earlier configKey to its schema default so the
        latest definition takes effect.
        """
        settings = NoodlesConfig.settings()
        if settings is None:
            return

        # Collect the configured key string for each shortcut in declaration
        # order. We compare on QKeySequence.toString() to normalize formatting
        # (e.g. "ctrl+s" vs "Ctrl+S").
        loaded = []
        for _name, configKey, _checkable, _checkedGetter, _handler in shortcuts:
            shortcutStr = NoodlesConfig.get(configKey, "")
            normalized = (
                QtGui.QKeySequence(shortcutStr).toString() if shortcutStr else ""
            )
            loaded.append((configKey, normalized))

        # For each later entry, blank out any earlier entry that holds the
        # same key. Walking from the end keeps the latest-defined action
        # authoritative without re-iterating.
        seen = {}
        for configKey, normalized in reversed(loaded):
            if not normalized:
                continue
            if normalized in seen:
                # An entry defined LATER already claims this key, so reset
                # this earlier entry to its schema default to break the tie.
                info = settings.getSettingInfo(configKey)
                if not info:
                    continue
                defaultStr = info.get("default", "")
                defaultNormalized = (
                    QtGui.QKeySequence(defaultStr).toString() if defaultStr else ""
                )
                # Only rewrite when the default would actually free the key,
                # otherwise we would loop the same conflict on the next launch.
                if defaultNormalized == normalized:
                    continue
                Tf.Status(
                    f"Resetting shortcut {configKey!r} from {normalized!r} to "
                    f"default {defaultStr!r} (conflicts with {seen[normalized]!r})"
                )
                NoodlesConfig.set(configKey, defaultStr)
            else:
                seen[normalized] = configKey

    def _setupShortcuts(self):
        """Setup shortcuts from configuration."""
        shortcuts = [
            # (name, configKey, checkable, initialCheckedGetter, handler)
            (
                "Toggle Links",
                "shortcutToggleLinks",
                True,
                lambda: self.drawLinks,
                self._toggleDrawLinks,
            ),
            (
                "Toggle Nodes",
                "shortcutToggleNodes",
                True,
                lambda: self.drawNodes,
                self._toggleDrawNodes,
            ),
            (
                "Toggle Text",
                "shortcutToggleText",
                True,
                lambda: self.drawText,
                self._toggleDrawText,
            ),
            (
                "Toggle Link Dimming",
                "shortcutToggleLinkDimming",
                True,
                lambda: self.linkDimming.target > 0.0,
                self._toggleLinkDimming,
            ),
            ("Select All", "shortcutSelectAll", False, None, self.selectAll),
            (
                "Navigate Backward",
                "shortcutNavigateBackward",
                False,
                None,
                self.navigateBackward,
            ),
            (
                "Navigate Forward",
                "shortcutNavigateForward",
                False,
                None,
                self.navigateForward,
            ),
            ("Frame Selection", "shortcutFrameSelection", False, None, self.frameScene),
            ("Frame All", "shortcutFrameAll", False, None, self.frameAll),
            (
                "Frame with Connections",
                "shortcutFrameWithConnections",
                False,
                None,
                self.frameSelectionWithConnections,
            ),
            (
                "Remove from Graph",
                "shortcutRemoveFromGraph",
                False,
                None,
                self.removeSelectedNodes,
            ),
            (
                "Add from Prim Tree",
                "shortcutAddFromPrimTree",
                False,
                None,
                self.addNodesFromPrimTreeSelection,
            ),
        ]

        # Migration: resolve shortcut conflicts. When a default shortcut moves
        # from one action to another (e.g. "D" moved from shortcutToggleLinkDimming
        # to shortcutRemoveFromGraph), a user's persisted value can still claim
        # the key, masking the newer action. Reset any earlier action whose
        # current value collides with a later action's value so the later
        # definition wins.
        self._resolveShortcutConflicts(shortcuts)

        # Store actions for dynamic updates
        self._shortcutActions = {}

        for name, configKey, checkable, checkedGetter, handler in shortcuts:
            # Get shortcut from config (with default fallback already in schema)
            shortcutStr = NoodlesConfig.get(configKey, "")

            action = QtGui.QAction(name, self)

            # Only set shortcut if not empty
            if shortcutStr:
                action.setShortcut(QtGui.QKeySequence(shortcutStr))

            action.setShortcutContext(QtCore.Qt.WidgetShortcut)

            if checkable:
                action.setCheckable(True)
                action.setChecked(checkedGetter())

            action.triggered.connect(handler)
            self.addAction(action)

            # Store for dynamic updates
            self._shortcutActions[configKey] = action

    def updateShortcuts(self):
        """Update all shortcuts from current configuration.
        Called when preferences are applied."""
        if not hasattr(self, "_shortcutActions"):
            return

        for configKey, action in self._shortcutActions.items():
            shortcutStr = NoodlesConfig.get(configKey, "")
            if shortcutStr:
                action.setShortcut(QtGui.QKeySequence(shortcutStr))
            else:
                # Clear shortcut if empty string
                action.setShortcut(QtGui.QKeySequence())

    def _onSettingsChanged(self):
        """Called when settings change via preferences dialog.
        Updates cached values so we don't read from config every frame."""
        self._initCachedSettings()
        # Update shortcuts as well
        self.updateShortcuts()
        # Update profiler state
        self._profiler.enabled = self._cachedEnableRenderProfiling
        if self._profiler.enabled:
            self._profiler.reset()
        # Update pan animation speed
        self._panXAnim.speed = self._cachedPanAnimationSpeed
        self._panYAnim.speed = self._cachedPanAnimationSpeed
        # Recalculate all node sizes + layouts (font sizes / margins may have
        # changed, which affects the cached layout metrics every renderer reads).
        if self.nodeGraph and self.fontAtlas:
            for node in self.nodes.values():
                self.nodeGraph._calculateNodeSize(
                    node,
                    self.textRenderer.calculateTextWidth
                    if self.textRenderer
                    else lambda t, s: s * 5,
                    self.fontAtlas,
                )
        self.textChanged = True
        self._clearRenderCache()
        # Trigger redraw to apply new settings immediately
        self.update()

    def _initCachedSettings(self):
        """Initialize/update all cached settings from NoodlesConfig.
        Called on init and when settings change."""
        # Drop the cached RenderConfig so the next paint rebuilds it from the new
        # settings.
        self._cachedRenderConfig = None
        # Link colors
        self._cachedLinkHoveredColor = NoodlesConfig.get(
            "linkHoveredColor", [1.0, 1.0, 0.0, 1.0]
        )
        self._cachedLinkSelectedColor = NoodlesConfig.get(
            "linkSelectedColor", [1.0, 1.0, 0.3, 1.0]
        )
        self._cachedLinkActiveColor = NoodlesConfig.get(
            "linkActiveColor", [0.0, 0.8, 1.0, 1.0]
        )
        self._cachedLinkHighlightedColor = NoodlesConfig.get(
            "linkHighlightedColor", [0.31, 0.78, 0.47, 1.0]
        )
        self._cachedLinkAttributeBaseColor = NoodlesConfig.get(
            "linkAttributeBaseColor", [0.31, 0.55, 0.86, 1.0]
        )
        self._cachedLinkRelationshipBaseColor = NoodlesConfig.get(
            "linkRelationshipBaseColor", [0.78, 0.24, 0.24, 1.0]
        )
        self._cachedNodeSelectedStrokeColor = NoodlesConfig.get(
            "nodeSelectedStrokeColor", [1.0, 1.0, 0.3, 1.0]
        )
        # Animation
        self._cachedPanAnimationSpeed = NoodlesConfig.get("panAnimationSpeed", 18.0)
        # Hit detection. Link picking matches the drawn ribbon width (its visible
        # 0.5 SDF edge); see linkRenderer.findLinkUnderCursor.
        self._cachedLinkLineWidth = NoodlesConfig.get("linkLineWidth", 12.0)
        self._cachedPortHitRadiusMultiplier = NoodlesConfig.get(
            "portHitRadiusMultiplier", 1.5
        )
        # Behavior
        self._cachedFrameInPrimTreeOnSelect = NoodlesConfig.get(
            "frameInPrimTreeOnSelect", True
        )
        # Profiling
        self._cachedEnableRenderProfiling = NoodlesConfig.get(
            "enableRenderProfiling", False
        )
        self._cachedProfilingPrintInterval = NoodlesConfig.get(
            "profilingPrintInterval", 60
        )
        # Node layout
        self._cachedNodeMarginV = NoodlesConfig.get("nodeMarginV", 18.0)
        self._cachedNodePinFontSize = NoodlesConfig.get("nodePinFontSize", 18.0)
        # Port colors
        self._cachedPortRingThickness = NoodlesConfig.get("portRingThickness", 2.0)
        self._cachedPortConnectedFillColor = NoodlesConfig.get(
            "portConnectedFillColor", [0.7, 0.8, 0.0, 1.0]
        )
        self._cachedPortConnectedRingColor = NoodlesConfig.get(
            "portConnectedRingColor", [1.0, 1.0, 1.0, 0.0]
        )
        self._cachedPortDisconnectedFillColor = NoodlesConfig.get(
            "portDisconnectedFillColor", [0.235, 0.235, 0.235, 1.0]
        )
        self._cachedPortDisconnectedRingColor = NoodlesConfig.get(
            "portDisconnectedRingColor", [0.7, 0.8, 0.0, 1.0]
        )
        self._cachedPortHoverColor = NoodlesConfig.get(
            "portHoverColor", [0.4, 0.7, 1.0, 1.0]
        )
        # Minimap overlay visibility. Off skips both the per-frame minimap draw
        # (which uploads a rect per node) and its mouse hit-testing.
        self._cachedShowMinimap = NoodlesConfig.get("showMinimap", True)

    def _toggleDrawLinks(self, checked):
        self.drawLinks = checked
        self.update()

    def _toggleDrawNodes(self, checked):
        self.drawNodes = checked
        self.update()

    def _toggleDrawText(self, checked):
        self.drawText = checked
        self.update()

    def _toggleLinkDimming(self, checked):
        self.linkDimming.target = 1.0 if checked else 0.0
        self.update()

    def _zoomAroundPoint(self, newZoom, focusPoint):
        """Zoom the view around a specific point.

        Zoom limits:
        - Min zoom: graph bounds occupy about 25% of screen (can zoom out 4x beyond fit)
        - Max zoom: a typical node (200x100) takes up about 1/5 of the screen
        """
        # Calculate min zoom so graph bounds occupy ~25% of screen
        # This means we can zoom out 4x beyond the "fit to screen" level
        minZoom = 0.001
        graphBounds = self.minimap.getGraphBounds()
        if not graphBounds.IsEmpty():
            graphSize = graphBounds.GetSize()
            if graphSize[0] > 0 and graphSize[1] > 0:
                # Calculate zoom where graph fills screen, then allow 4x more zoom out
                # (graph at 25% of screen = 4x zoom out from fill)
                zoomForWidth = self.width() / graphSize[0] if graphSize[0] > 0 else 1.0
                zoomForHeight = (
                    self.height() / graphSize[1] if graphSize[1] > 0 else 1.0
                )
                fitZoom = min(zoomForWidth, zoomForHeight)
                minZoom = fitZoom * 0.25  # Allow zooming out to 25% of fit
                minZoom = max(0.001, minZoom)

        # Calculate max zoom so one node takes up about 1/5 of the screen
        # Typical node size is around 200x100 units
        typicalNodeWidth = 200.0
        typicalNodeHeight = 100.0
        # Node should be ~1/5 of screen, so screen = 5 * node
        maxZoomForWidth = (
            self.width() / (typicalNodeWidth * 5.0) if typicalNodeWidth > 0 else 100.0
        )
        maxZoomForHeight = (
            self.height() / (typicalNodeHeight * 5.0)
            if typicalNodeHeight > 0
            else 100.0
        )
        # Use the larger zoom (more zoomed in) but cap at reasonable maximum
        maxZoom = min(max(maxZoomForWidth, maxZoomForHeight), 100.0)
        # Ensure max is at least greater than min
        maxZoom = max(maxZoom, minZoom * 2.0)

        newZoom = max(minZoom, min(newZoom, maxZoom))
        focus = Gf.Vec2d(focusPoint[0], focusPoint[1])
        delta = focus * (1.0 / self.zoom - 1.0 / newZoom)
        self.panX += delta[0]
        self.panY += delta[1]
        self.zoom = newZoom

    def _setZoom(self, newZoom):
        center = Gf.Vec2d(self.width() * 0.5, self.height() * 0.5)
        self._zoomAroundPoint(newZoom, center)
        self.update()

    def _clearRenderCache(self):
        """Clear the node render manager cache within this widget's GL context.

        makeCurrent()/doneCurrent() ensures that glDeleteBuffers/glDeleteVertexArrays
        operate on this widget's OpenGL context, not the stageView's.  Without the
        context switch the 3D viewport's render state may be corrupted.

        When self._deferRenderCacheClear is True (set by _onStageReplaced),
        only flag the pending clear; _finishStageReload will run the actual
        glDelete* calls after the next event-loop tick.  This avoids issuing
        GL deletes during the NVIDIA Windows non-functional window while the
        stageView's renderer is mid-teardown.
        """
        if self._deferRenderCacheClear:
            self._renderCacheClearPending = True
            return
        self.makeCurrent()
        self._nodeRenderManager.clearCache()
        self.doneCurrent()

    def _cleanupNodeVBOs(self):
        """
        Clean up all rendering resources.

        Called when the OpenGL context is lost or being reinitialized.
        """
        self._nodeRenderManager.cleanup()
        self._groupStickerRenderer.cleanup()
        self.linkRenderer.cleanup()
        self.textRenderer._cpp.cleanup()
        self._nodeTransformFrame.cleanup()
        if hasattr(self, "_cppIconRenderer") and self._cppIconRenderer.isInitialized:
            self._cppIconRenderer.cleanup()

    def _reinitializeGL(self):
        """Handle GL context change — clean up stale resources and reinitialize."""
        self.makeCurrent()
        self._cleanupNodeVBOs()
        if self.shaderLibrary:
            self.shaderLibrary.cleanup()
        if self.fontAtlas:
            self.fontAtlas.cleanup()
        self.initialized = False
        self.fontAtlas = None
        self.shaderLibrary = None
        self.initializeGL()
        # initializeGL() calls doneCurrent() — restore context for callers
        # (paintGL needs it current for _paintGLInner)
        self.makeCurrent()
        self.linksChanged = True

    def initializeGL(self):
        needsInit = not self.initialized

        if self.initialized:
            self.makeCurrent()
            contextChanged = False

            if self.fontAtlas and self.fontAtlas.texture:
                if not GL.glIsTexture(self.fontAtlas.texture):
                    contextChanged = True
            elif self.fontAtlas is None:
                contextChanged = True

            if contextChanged:
                self._cleanupNodeVBOs()
                if self.shaderLibrary:
                    self.shaderLibrary.cleanup()
                if self.fontAtlas:
                    self.fontAtlas.cleanup()
                needsInit = True
                self.initialized = False
                self.fontAtlas = None
                self.shaderLibrary = None

        if not needsInit:
            return

        self.makeCurrent()

        Glf.RegisterDefaultDebugOutputMessageCallback()

        from pxr import Plug

        plugin = Plug.Registry().GetPluginWithName("pxr.UsdNoodles")

        if plugin:
            # Plugin resource path points to: <install>/plugin/usd/usdNoodles/resources/
            resourcePath = Path(plugin.resourcePath)

            assetsInResources = resourcePath / "assets"
            if assetsInResources.exists():
                assetsPath = assetsInResources
            else:
                assetsPath = resourcePath
        else:
            resourcePath = Path(__file__).parent
            assetsPath = resourcePath / "assets"
            if not assetsPath.exists():
                assetsPath = resourcePath
        self._assetsPath = assetsPath

        atlasTexturePath = assetsPath / "fonts" / "Poppins-Regular.png"
        fontMetricsPath = assetsPath / "fonts" / "Poppins-Regular.json"

        if not atlasTexturePath.exists():
            Tf.Warn(f"Font atlas not found at {atlasTexturePath}")
            self.doneCurrent()
            return

        self.fontAtlas = FontAtlas()
        self.fontAtlas.initialize(str(atlasTexturePath), str(fontMetricsPath))

        if not self.nodes:
            sampleGraphPath = assetsPath / "bodyGraph.json"
            if sampleGraphPath.exists():
                self._loadSampleGraph(str(sampleGraphPath))
                if self.nodes:
                    self.frameScene()

        self.shaderLibrary = ShaderLibrary()
        self.shaderLibrary.initialize(str(assetsPath))

        # Initialize icon renderer
        from .iconRenderer import CppIconRenderer

        # C++ icon path. Shares the shader library + assets path with the other
        # render paths; the transform frame is shared below so icons ride node
        # drags. Construction degrades to a no-op if the C++ extension is
        # unavailable, so the editor still opens.
        self._cppIconRenderer = CppIconRenderer()
        self._cppIconRenderer.initialize(self.shaderLibrary, str(assetsPath))

        # Initialize rendering managers
        self._nodeRenderManager.initialize(self.shaderLibrary)
        self._groupStickerRenderer.initialize(self.shaderLibrary)
        self.linkRenderer.initialize(self.shaderLibrary)
        self.textRenderer.initialize(self.shaderLibrary, self.fontAtlas)

        # All C++ render paths share one transform frame (text + node quads +
        # icons) so they sample one transform texture and move together on drag.
        self._nodeRenderManager.setTransformFrame(self._nodeTransformFrame)
        self.textRenderer.setTransformFrame(self._nodeTransformFrame)
        self._cppIconRenderer.setTransformFrame(self._nodeTransformFrame)

        self.initialized = True
        self.textChanged = True
        self.doneCurrent()

    def _loadSampleGraph(self, jsonPath):
        self.nodeGraph = NodeGraphJson()
        self.nodeGraph.load(
            jsonPath, self.textRenderer.calculateTextWidth, self.fontAtlas
        )
        self.linksChanged = True

    def loadBlueprint(self, stage, primPath, useFieldnames=True):
        self._currentPrimPath = primPath

        self.nodeGraph = NodeGraphBlueprint()
        self.nodeGraph.setStage(stage)
        self.nodeGraph.load(
            stage,
            primPath,
            self.textRenderer.calculateTextWidth,
            self.fontAtlas,
            useFieldnames,
            nodeFactory=self._nodeFactory,  # Pass the factory
        )

        self._seedNodeZOrders()
        self._gridPlaceNodes(list(self.nodes.values()))

        self._resetRenderCachesAfterBulkMove()

        self._noticeHandler.register(stage)

        # Attach undo state delegates to track layer edits
        self._initUndoTracking(stage)

        if self.nodes:
            self.frameAll()

    def loadContainer(self, stage, primPath, useFieldnames=True):
        from .nodeGraphBpContainer import NodeGraphBpContainer

        self._currentPrimPath = primPath

        self.nodeGraph = NodeGraphBpContainer()
        self.nodeGraph.setStage(stage)
        self.nodeGraph.load(
            stage,
            primPath,
            self.textRenderer.calculateTextWidth,
            self.fontAtlas,
            useFieldnames,
            nodeFactory=self._nodeFactory,  # Pass the factory
        )

        self._seedNodeZOrders()
        self._gridPlaceNodes(list(self.nodes.values()))

        self._resetRenderCachesAfterBulkMove()

        # Register for USD change notices on this stage
        self._noticeHandler.register(stage)

        # Attach undo state delegates to track layer edits
        self._initUndoTracking(stage)

        if self.nodes:
            self.frameAll()

    def loadStage(self, stage):
        """Load all valid graph prims found at the stage root.

        Scans root-level children for ExecNode, Container, or Shader types
        and loads them all into a single graph view.
        """
        from .nodeGraphStage import NodeGraphStage

        self._currentPrimPath = None

        self.nodeGraph = NodeGraphStage()
        self.nodeGraph.setStage(stage)
        self.nodeGraph.load(
            stage,
            self.textRenderer.calculateTextWidth,
            self.fontAtlas,
            nodeFactory=self._nodeFactory,
        )

        self._seedNodeZOrders()
        self._gridPlaceNodes(list(self.nodes.values()))

        self._resetRenderCachesAfterBulkMove()
        self._noticeHandler.register(stage)

        # Attach undo state delegates to track layer edits
        self._initUndoTracking(stage)

        if self.nodes:
            self.frameAll()

    # Auto-layout grid for nodes that have no position of their own, in
    # logical/world coordinates (independent of window size): ~30 columns per
    # row, cell sized to the batch's largest node plus a gap so content-sized
    # nodes keep clear spacing.
    _AUTO_GRID_COLUMNS = 30
    _AUTO_GRID_GAP = 150.0

    def _gridPlaceNodes(self, nodes):
        """Lay out the (0, 0) nodes among ``nodes`` in a grid and return them.

        Only nodes still at the USD default (0, 0) are placed; nodes with their
        own position (an authored ``ui:nodegraph:node:pos`` or one the loader set
        for virtual input/output nodes) are left untouched. The batch is anchored
        at world origin when nothing else is placed (a fresh load) and otherwise
        just below the bounding box of the already-placed nodes, so adding into an
        arranged graph never lands on top of it. Columns are a uniform width and
        each row is only as tall as the tallest node in that row, so one very tall
        node doesn't inflate every row's spacing. Placement is display-only (no USD
        authoring), so it can't fail on a prim that won't take the attribute and
        opening a stage never dirties it.
        """
        placeable = [n for n in nodes if n.position[0] == 0.0 and n.position[1] == 0.0]
        if not placeable:
            return []
        placeableIds = {n.id for n in placeable}

        # Anchor below everything already placed (origin if there is nothing).
        anchorX, anchorY = 0.0, 0.0
        placed = [
            n
            for n in self.nodes.values()
            if n.id not in placeableIds
            and (n.position[0] != 0.0 or n.position[1] != 0.0)
        ]
        if placed:
            anchorX = min(n.position[0] for n in placed)
            anchorY = (
                max(n.position[1] + n.size[1] for n in placed) + self._AUTO_GRID_GAP
            )

        # Uniform column width keeps columns aligned; each row's height is sized
        # to the tallest node IN THAT ROW (not the whole batch) so a single very
        # tall node doesn't inflate every row's vertical gap.
        cellW = max(n.size[0] for n in placeable) + self._AUTO_GRID_GAP
        cols = self._AUTO_GRID_COLUMNS
        y = anchorY
        for rowStart in range(0, len(placeable), cols):
            rowNodes = placeable[rowStart : rowStart + cols]
            for col, node in enumerate(rowNodes):
                # Display-only (no USD authoring): never fails on a prim that can't
                # take ui:nodegraph:node:pos, and opening a stage doesn't dirty it.
                node.setDisplayPosition(Gf.Vec2d(anchorX + col * cellW, y))
            y += max(n.size[1] for n in rowNodes) + self._AUTO_GRID_GAP
        return placeable

    def _seedNodeZOrders(self):
        """Give each freshly loaded node a distinct, increasing zOrder.

        Loaded nodes otherwise keep the C++ default zOrder = 0, so every node
        lands in the same depth band; overlapping node backgrounds, title bars,
        shadows, and title text then z-fight under GL_LEQUAL on the first frame.
        Assigning a distinct zOrder per node (mirroring the runtime add-node and
        reload paths) separates the depth bands so overlaps render cleanly
        front-to-back. _nextZOrder is left at the max so later add / bring-to-front
        actions stack on top.
        """
        self._nextZOrder = 0
        for node in self.nodes.values():
            self._nextZOrder += 1
            node.zOrder = self._nextZOrder

    def _initUndoTracking(self, _stage):
        """Clear the undo stack for the fresh graph."""
        try:
            from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager

            NoodlesUndoManager.instance().clear()
        except ImportError:
            pass

    def _autoLayoutNodes(self):
        """Arrange every node with the Sugiyama auto-layout (L / context menu).

        Only data-flow links drive the layout; relationship links keep their
        top-center anchors and are re-drawn over the placed nodes by the link
        rebuild. The new positions are authored to USD as a single undo step.
        """
        if not self.nodes:
            return

        positions = layoutGraphPositions(self.nodes, self.links, LayoutParams())
        if not positions:
            return

        # Capture old and new positions so the whole arrange is one undo step
        # (mirrors _finalizeNodeDrag).
        old_positions = {}
        new_positions = {}
        for node_id, node in self.nodes.items():
            new_pos = positions.get(node_id)
            if new_pos is None:
                continue
            old = node.position
            old_positions[node_id] = (old[0], old[1])
            new_positions[node_id] = (new_pos[0], new_pos[1])

        if not new_positions:
            return

        self._applyNodePositions(new_positions)

        gv = self  # avoid closing over self directly

        def _restore_positions(gv_ref, positions_map):
            gv_ref._applyNodePositions(positions_map)

        _push_undo_command(
            "Auto Layout",
            lambda p=new_positions: _restore_positions(gv, p),
            lambda p=old_positions: _restore_positions(gv, p),
        )

        self.frameAll()

    def _resetRenderCachesAfterBulkMove(self, rebuild_links=True):
        """Invalidate every baked vertex cache + the drag-transform frame and flag
        text for rebuild, after nodes are repositioned or (re)loaded wholesale.

        ``rebuild_links`` controls link refresh. The load paths pass True
        (connections are new, so a full rebuild is required). A pure move
        (auto-layout) passes False and refreshes only the moved links' endpoints
        afterward via ``_updateLinksForMovedNodes`` -- connections are unchanged,
        so re-reading every prim's connections from USD would be wasted work.
        """
        self._clearRenderCache()
        self.textRenderer.resetPositionCaches()
        self._cppIconRenderer.resetPositionCaches()
        self._nodeRenderManager.resetNodeQuadCaches()
        self._nodeTransformFrame.reset()
        self.textChanged = True
        if rebuild_links:
            self.linksChanged = True

    def _applyNodePositions(self, positions):
        """Author ``{nodeId: (x, y)}`` to USD in one batched change and re-render.

        Writes every position inside a single ``Sdf.ChangeBlock`` with the USD
        notice handler disabled (mirrors ``_finalizeNodeDrag`` /
        ``_persistNodePositions``) so the move emits one notice instead of N,
        then re-bakes the node/text/icon caches and refreshes only the moved
        links' endpoints (connections are unchanged, so it skips the full
        connection re-read). Used by auto-layout and its undo/redo.
        """
        self._noticeHandler.setEnabled(False)
        try:
            with Sdf.ChangeBlock():
                for node_id, pos in positions.items():
                    self._authorNodePosition(node_id, pos)
        finally:
            self._noticeHandler.setEnabled(True)
        # Only positions changed (not connections): re-bake the node/text/icon
        # drawables, then refresh the moved links' endpoints incrementally
        # (paintLinks re-syncs to the C++ renderer each frame) -- far cheaper at
        # scale than a full _rebuildLinks that re-reads every prim's connections.
        # Authoring above bumped each node's position version, so
        # _reconcileMovedNodes refreshes the spatial index, link endpoints, and
        # port highlights before the next paint and hit-test. This method only
        # re-bakes the drawables (which bake in the position) and requests a
        # repaint.
        self._resetRenderCachesAfterBulkMove(rebuild_links=False)
        self.update()

    def _updateNodeSpatialBounds(self, node_ids):
        """Refresh the spatial-index bounds for the given moved nodes.

        Hit-testing (click, marquee, hover) resolves nodes through the spatial
        index, so any path that changes a node's position must keep the index
        consistent with the drawn position. This is the shared helper that does
        it; callers that move nodes route through here instead of recomputing
        bounds inline.
        """
        for node_id in node_ids:
            node = self.nodes.get(node_id)
            if node is None:
                continue
            bounds = Gf.Range2d(
                Gf.Vec2d(node.position),
                Gf.Vec2d(node.position + node.size),
            )
            self._spatialIndex.updateNode(node_id, bounds)

    def _applyMovedNodeDerivedState(self, moved):
        """Refresh the state derived from a node's position for ``moved``:
        spatial-index bounds (hit-testing), link endpoints, and port highlights.

        Both callers that move nodes (the per-frame reconcile and the live drag)
        go through here, so the set of position-derived state is defined in one
        place. Render re-baking is not included; it differs by caller (bulk moves
        call _resetRenderCachesAfterBulkMove, a drag rides the transform frame).
        """
        if not moved:
            return
        self._updateNodeSpatialBounds(moved)
        self._updateLinksForMovedNodes(moved)
        self._portHighlightsDirty = True

    def _reconcileMovedNodes(self):
        """Refresh position-derived state for every node whose ``position``
        version advanced since the last call (see ``VersionedVec2d``). Movers only
        set the position; this pass updates everything that depends on it, so none
        of them can leave it half-updated.

        A process-global move generation keeps the common case (nothing moved,
        e.g. a pan) at O(1). A drag refreshes its own derived state and is skipped
        here; the release frame reconciles the final positions.
        """
        if self.draggingNodes:
            return
        gen = positionWriteGeneration()
        if gen == self._reconcilePositionGen:
            return
        self._reconcilePositionGen = gen
        seen = self._reconciledPositionVersions
        moved = set()
        for node_id, node in self.nodes.items():
            version = node.positionVersion
            if seen.get(node_id) != version:
                seen[node_id] = version
                moved.add(node_id)
        self._applyMovedNodeDerivedState(moved)

    def _authorNodePosition(self, node_id, pos):
        """Write one node's position to USD and update its render caches."""
        node = self.nodes.get(node_id)
        if node is None:
            return
        vec = Gf.Vec2d(pos[0], pos[1])
        node._writePositionToUsdRaw(vec)
        node.setDisplayPosition(vec)

    @property
    def currentPrimPath(self):
        return self._currentPrimPath

    def _getContextMenuConf(self):
        return [
            (
                M.SUBMENU,
                "View",
                [
                    (
                        M.CHECK,
                        "Show Links",
                        None,
                        self._toggleDrawLinks,
                        self.drawLinks,
                    ),
                    (
                        M.CHECK,
                        "Show Nodes",
                        None,
                        self._toggleDrawNodes,
                        self.drawNodes,
                    ),
                    (M.CHECK, "Show Text", None, self._toggleDrawText, self.drawText),
                    (M.SEPARATOR,),
                    (
                        M.CHECK,
                        "Link Dimming",
                        None,
                        self._toggleLinkDimming,
                        self.linkDimming.target > 0.0,
                    ),
                ],
            ),
            (
                M.SUBMENU,
                "Zoom",
                [
                    (M.ACTION, "100%", None, lambda: self._setZoom(1.0)),
                    (M.ACTION, "50%", None, lambda: self._setZoom(0.5)),
                    (M.ACTION, "25%", None, lambda: self._setZoom(0.25)),
                    (M.SEPARATOR,),
                    (M.ACTION, "Zoom In", None, lambda: self._setZoom(self.zoom * 1.5)),
                    (
                        M.ACTION,
                        "Zoom Out",
                        None,
                        lambda: self._setZoom(self.zoom / 1.5),
                    ),
                ],
            ),
            (M.SEPARATOR,),
            (M.ACTION, "Frame All (A)", None, self.frameAll),
            (M.ACTION, "Frame Selection (F)", None, self.frameScene),
            (
                M.ACTION,
                "Frame Selection with Connections (Shift+F)",
                None,
                self.frameSelectionWithConnections,
            ),
            (M.ACTION, "Auto Layout (L)", None, self._autoLayoutNodes),
            (M.SEPARATOR,),
            (M.ACTION, "Select All (Ctrl+A)", None, self.selectAll),
            (M.ACTION, "Clear Selection", None, self._clearSelectionAndSync),
            (
                M.ACTION,
                "Delete Selected Nodes (Delete)",
                None,
                self.deleteSelectedNodes,
            ),
            (
                M.ACTION,
                "Create Backdrop from Selection",
                None,
                self.createBackdropFromSelection,
            ),
            (M.SEPARATOR,),
            (
                M.SUBMENU,
                "Link Selection Mode",
                [
                    (
                        M.CHECK,
                        "Nodes Only",
                        None,
                        lambda: self._setLinkSelectionMode(
                            LinkSelectionMode.NODES_ONLY
                        ),
                        self._linkSelectionMode == LinkSelectionMode.NODES_ONLY,
                    ),
                    (
                        M.CHECK,
                        "With All Links",
                        None,
                        lambda: self._setLinkSelectionMode(
                            LinkSelectionMode.WITH_ALL_LINKS
                        ),
                        self._linkSelectionMode == LinkSelectionMode.WITH_ALL_LINKS,
                    ),
                    (
                        M.CHECK,
                        "With Input Links",
                        None,
                        lambda: self._setLinkSelectionMode(
                            LinkSelectionMode.WITH_INPUTS
                        ),
                        self._linkSelectionMode == LinkSelectionMode.WITH_INPUTS,
                    ),
                    (
                        M.CHECK,
                        "With Output Links",
                        None,
                        lambda: self._setLinkSelectionMode(
                            LinkSelectionMode.WITH_OUTPUTS
                        ),
                        self._linkSelectionMode == LinkSelectionMode.WITH_OUTPUTS,
                    ),
                ],
            ),
            (M.SEPARATOR,),
            (M.ACTION, "Navigate Backward ([)", None, self.navigateBackward),
            (M.ACTION, "Navigate Forward (])", None, self.navigateForward),
            (M.SEPARATOR,),
            (
                M.CHECK,
                "Frame in Prim Tree on Select",
                None,
                self._toggleFrameInPrimTree,
                self._frameInPrimTreeOnSelect,
            ),
            (M.SEPARATOR,),
            (M.ACTION, "Open Container", None, self._openSelectedContainer),
        ]

    def _initContextMenu(self):
        menuConf = self._getContextMenuConf()
        builder = MenuBuilder(self)
        self._contextMenu, self._contextMenuActions = (
            builder.buildContextMenuWithActions(menuConf)
        )

        # Pre-warm the menu to force Qt to cache layout/rendering data
        # This significantly improves the first-time display performance
        # by triggering Qt's internal menu preparation once during initialization
        self._contextMenu.popup(QtCore.QPoint(-10000, -10000))
        self._contextMenu.hide()

    def _updateContextMenuState(self):
        actions = self._contextMenuActions

        hasSelection = len(self._selectedNodes) > 0
        hasLinkSelection = len(self._selectedLinks) > 0

        actions["Show Links"].setChecked(self.drawLinks)
        actions["Show Nodes"].setChecked(self.drawNodes)
        actions["Show Text"].setChecked(self.drawText)
        actions["Link Dimming"].setChecked(self.linkDimming.target > 0.0)

        actions["Frame Selection (F)"].setEnabled(hasSelection)
        actions["Frame Selection with Connections (Shift+F)"].setEnabled(hasSelection)
        actions["Clear Selection"].setEnabled(hasSelection or hasLinkSelection)
        actions["Create Backdrop from Selection"].setEnabled(hasSelection)

        actions["Nodes Only"].setChecked(
            self._linkSelectionMode == LinkSelectionMode.NODES_ONLY
        )
        actions["With All Links"].setChecked(
            self._linkSelectionMode == LinkSelectionMode.WITH_ALL_LINKS
        )
        actions["With Input Links"].setChecked(
            self._linkSelectionMode == LinkSelectionMode.WITH_INPUTS
        )
        actions["With Output Links"].setChecked(
            self._linkSelectionMode == LinkSelectionMode.WITH_OUTPUTS
        )

        hasLinkOrNodeSelection = hasLinkSelection or hasSelection
        actions["Navigate Backward ([)"].setEnabled(hasLinkOrNodeSelection)
        actions["Navigate Forward (])"].setEnabled(hasLinkOrNodeSelection)

        actions["Frame in Prim Tree on Select"].setChecked(
            self._frameInPrimTreeOnSelect
        )

        # Enable "Open Container" only if exactly one node is selected and it's a container
        canOpenContainer = self._canOpenSelectedContainer()
        actions["Open Container"].setEnabled(canOpenContainer)

    def contextMenuEvent(self, event):
        self._updateContextMenuState()
        self._contextMenu.exec_(event.globalPos())

    def _setLinkSelectionMode(self, mode):
        self.linkSelectionMode = mode

    def _toggleFrameInPrimTree(self, checked):
        self._frameInPrimTreeOnSelect = checked

    def _clearSelectionAndSync(self):
        self.clearSelection()
        self._syncSelectionToPrimtree()

    def _canOpenSelectedContainer(self):
        """Check if exactly one node is selected and it's a Container type."""
        if len(self._selectedNodes) != 1:
            return False

        selectedNodeId = next(iter(self._selectedNodes))
        if selectedNodeId not in self.nodes:
            return False

        node = self.nodes[selectedNodeId]

        # Check if the node's type is "Container"
        # Containers can be:
        # 1. A node with type "Container" (directly set from the prim)
        # 2. A node whose ID is a valid USD prim path that has type "Container"
        if node.type == "Container":
            return True

        # Also check the actual prim type in case the node type wasn't set correctly
        if self._usdviewApi:
            stage = self._usdviewApi.stage
            if stage and "#" not in node.id:  # Skip virtual nodes
                prim = stage.GetPrimAtPath(node.id)
                if prim and prim.IsValid() and prim.GetTypeName() == "Container":
                    return True

        return False

    def _openSelectedContainer(self):
        """Open the selected container node in a new Noodles editor window."""
        if not self._canOpenSelectedContainer():
            Tf.Warn("Cannot open container: no single container node selected")
            return

        selectedNodeId = next(iter(self._selectedNodes))
        node = self.nodes[selectedNodeId]

        # Use the node's ID as the prim path (it should be a valid USD path)
        containerPath = node.id

        # Skip virtual nodes (they have # in their path)
        if "#" in containerPath:
            Tf.Warn(f"Cannot open virtual node as container: {containerPath}")
            return

        if not self._usdviewApi:
            Tf.Warn("Cannot open container: usdviewApi not available")
            return

        # Import here to avoid circular dependency
        from pxr.UsdNoodles import OpenNewNoodlesEditorForContainer

        try:
            OpenNewNoodlesEditorForContainer(self._usdviewApi, containerPath)
        except Exception as e:
            Tf.Warn(f"Error opening container: {e}")
            import traceback

            traceback.print_exc()

    def _openPreferences(self):
        """Open the preferences dialog."""
        if self._preferencesDlg is None:
            self._preferencesDlg = NoodlesPreferences(self, self)
            # Clean up when dialog closes
            self._preferencesDlg.finished.connect(self._onPreferencesClosed)

        self._preferencesDlg.show()
        self._preferencesDlg.raise_()
        self._preferencesDlg.activateWindow()

    def _onPreferencesClosed(self):
        """Called when preferences dialog closes."""
        # Keep instance for reuse, but could set to None to recreate each time
        pass

    def _drawNodeVertices(
        self,
        vertexData,
        projection,
        cornerRadius,
        strokeColor,
        generation=0,
        selectionChanged=False,
    ):
        """
        Draw vertex data using the node shader (used by UI overlays).

        This is a simplified version for drawing dynamic UI overlays like status bars,
        hotkey popups, and marquee selections. Does not use caching since overlay
        data changes every frame.

        Args:
            vertexData: List of NodeVertex objects to render
            projection: Projection matrix for rendering
            cornerRadius: Corner radius for rounded corners
            strokeColor: Stroke color tuple (r, g, b, a)
            generation: Ignored (kept for API compatibility)
            selectionChanged: Ignored (kept for API compatibility)
        """
        if not vertexData or not self.shaderLibrary:
            return

        shader = self.shaderLibrary.get("node")
        if not shader:
            return

        packedData = b"".join(v.pack() for v in vertexData)
        vertexCount = len(vertexData)

        vbo = None
        vao = None
        try:
            vbo = GL.glGenBuffers(1)
            vao = GL.glGenVertexArrays(1)

            GL.glBindVertexArray(vao)
            GL.glBindBuffer(GL.GL_ARRAY_BUFFER, vbo)
            GL.glBufferData(
                GL.GL_ARRAY_BUFFER, len(packedData), packedData, GL.GL_STREAM_DRAW
            )

            # Configure vertex attributes
            stride = NodeVertex.size()
            for (
                index,
                numComponents,
                glTypeName,
                normalized,
                _stride,
                offset,
            ) in NODE_VERTEX_ATTRIB_LAYOUT:
                glType = getattr(GL, glTypeName)
                GL.glVertexAttribPointer(
                    index,
                    numComponents,
                    glType,
                    normalized,
                    stride,
                    ctypes.c_void_p(offset),
                )
                GL.glEnableVertexAttribArray(index)

            # Set up shader and draw
            shader.use()

            matrixData = projection.data()
            matrix = (ctypes.c_float * 16)(*matrixData)
            projLoc = shader.getUniformLocation("uProjection")
            if projLoc >= 0:
                GL.glUniformMatrix4fv(projLoc, 1, GL.GL_FALSE, matrix)

            radiusLoc = shader.getUniformLocation("uCornerRadius")
            if radiusLoc >= 0:
                GL.glUniform1f(radiusLoc, cornerRadius)

            strokeLoc = shader.getUniformLocation("uInnerStrokeColor")
            if strokeLoc >= 0:
                GL.glUniform4f(
                    strokeLoc,
                    strokeColor[0],
                    strokeColor[1],
                    strokeColor[2],
                    strokeColor[3],
                )

            GL.glEnable(GL.GL_BLEND)
            GL.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE_MINUS_SRC_ALPHA)
            GL.glDrawArrays(GL.GL_TRIANGLES, 0, vertexCount)
        except GLError as e:
            Tf.Warn(f"GL error during node rendering: {e}")
        finally:
            # Clean up temporary resources
            GL.glBindVertexArray(0)
            GL.glBindBuffer(GL.GL_ARRAY_BUFFER, 0)
            shader.release()
            if vbo is not None:
                GL.glDeleteBuffers(1, [vbo])
            if vao is not None:
                GL.glDeleteVertexArrays(1, [vao])

    def _findPinIndex(self, pins, pinName):
        for i, pin in enumerate(pins):
            if pin == pinName:
                return i
        return -1

    def _nodeHasVisiblePort(self, node, pinName, isOutput):
        if hasattr(node, "has_visible_port"):
            return node.has_visible_port(pinName, isOutput)
        pins = node.outputPins if isOutput else node.inputPins
        if pinName in pins:
            return True
        if isOutput:
            return pinName in getattr(node, "_dual_pin_names", set())
        return pinName in getattr(node, "_output_dual_pin_names", set())

    @staticmethod
    def _getNodeTopCenterPosition(node):
        return Gf.Vec2d(
            node.position[0] + node.size[0] * 0.5,
            node.position[1],
        )

    def _resolveLinkDisplayPinName(self, node, pinName, propertyName, isOutput):
        if propertyName and hasattr(node, "_prim") and node._prim:
            display_pin_name = get_display_pin_name_for_property_name(
                node._prim,
                propertyName,
            )
            if display_pin_name is not None:
                pinName = display_pin_name
        elif propertyName:
            # No prim available: strip any direction hint and keep namespaced
            # names intact (they double as their own display pin name).
            pinName = split_direction_hint(propertyName)[1]

        if hasattr(node, "resolve_pin_name"):
            resolution_side = self._getPropertyOutputSide(propertyName)
            if resolution_side is None:
                resolution_side = isOutput
            try:
                return node.resolve_pin_name(pinName, is_output=resolution_side)
            except TypeError:
                return node.resolve_pin_name(pinName)
        return pinName

    @staticmethod
    def _getPropertyOutputSide(propertyName):
        side, _stripped = split_direction_hint(propertyName)
        if side == "output" or is_relationship_pin(propertyName):
            return True
        if side == "input" or is_relationship_input_pin(propertyName):
            return False
        # Bare attrs (dual pins) and other namespaced attrs have no inherent
        # side here; the caller falls back to the link's own direction.
        return None

    @staticmethod
    def _isPinPropertyChange(propertyName):
        """True when a property change should rebuild links.

        Covers any direction-hint pin (``in:``/``input:``/``inputs:`` and
        ``out:``/``output:``/``outputs:``), relationship pins, and any other
        namespaced attribute shown as a pin (e.g. ``rig1:space``) — but not
        hidden ``ui:`` editor metadata. Bare attrs are left to the existing
        value-change handling.
        """
        if split_direction_hint(propertyName)[0] is not None:
            return True
        if is_relationship_pin(propertyName):
            return True
        if ":" not in propertyName:
            return False
        hide_ui = bool(NoodlesConfig.get("hideUiNamespacePins", True))
        return not is_hidden_namespace_pin(propertyName, hide_ui=hide_ui)

    @staticmethod
    def _mirrorPortPosition(node, portPos):
        mirrored_x = node.position[0] + node.size[0] - (portPos[0] - node.position[0])
        return type(portPos)(mirrored_x, portPos[1])

    def _getVisiblePortPosition(self, node, pinName, isOutput):
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        if isOutput and pinName in getattr(node, "_dual_pin_names", set()):
            inPos = renderer.getPortPosition(node, pinName, False, self.fontAtlas)
            if inPos:
                return self._mirrorPortPosition(node, inPos)
            return None
        if not isOutput and pinName in getattr(node, "_output_dual_pin_names", set()):
            outPos = renderer.getPortPosition(node, pinName, True, self.fontAtlas)
            if outPos:
                return self._mirrorPortPosition(node, outPos)
            return None
        if not self._nodeHasVisiblePort(node, pinName, isOutput):
            return None
        return renderer.getPortPosition(node, pinName, isOutput, self.fontAtlas)

    def _getRelationshipRowEndpointPosition(self, node, pinName, isOutput):
        visiblePos = self._getVisiblePortPosition(node, pinName, isOutput)
        if visiblePos is not None:
            return visiblePos

        oppositePos = self._getVisiblePortPosition(node, pinName, not isOutput)
        if oppositePos is not None:
            return self._mirrorPortPosition(node, oppositePos)

        fallbackPos = self._getPortPosition(node, pinName, isOutput)
        if fallbackPos is not None:
            return fallbackPos
        return self._getNodeTopCenterPosition(node)

    def _getLinkEndpointPosition(
        self,
        node,
        pinName,
        isOutput,
        propertyName="",
        preferPropertySide=False,
    ):
        if not pinName and not propertyName:
            return self._getNodeTopCenterPosition(node)

        resolved_pin = self._resolveLinkDisplayPinName(
            node,
            pinName,
            propertyName,
            isOutput,
        )
        if propertyName and is_relationship_pin(propertyName):
            return self._getRelationshipRowEndpointPosition(
                node,
                resolved_pin,
                isOutput,
            )

        property_side = self._getPropertyOutputSide(propertyName)

        if property_side is None:
            return self._getPortPosition(node, resolved_pin, isOutput)
        if preferPropertySide:
            return self._getPortPosition(node, resolved_pin, property_side)
        if property_side == isOutput:
            return self._getPortPosition(node, resolved_pin, isOutput)

        opposite_pos = self._getPortPosition(node, resolved_pin, property_side)
        if opposite_pos is None:
            return self._getPortPosition(node, resolved_pin, isOutput)
        return self._mirrorPortPosition(node, opposite_pos)

    def _resolvePortPropertyName(self, node, pinName, isOutput):
        if not hasattr(node, "_prim") or not node._prim:
            return pinName
        side = "output" if isOutput else "input"
        property_name = resolve_property_name_for_display_pin(node._prim, pinName, side)
        if property_name is not None:
            return property_name
        return pinName

    def _getPortPosition(self, node, pinName, isOutput):
        """Resolve a pin's port position via the C++ View Model.

        Thin shim over ``GraphNodeRenderer.resolvePortPosition``, the single
        node-local resolver that handles synthetic relationship ports, dual /
        output-dual mirrors, and normal pins. Python no longer computes port
        geometry — it just reads the View Model.
        """
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        return renderer.resolvePortPosition(node, pinName, isOutput)

    def _setupLinkEndpoints(self, link, startNode, endNode, startPinName, endPinName):
        """Set up link endpoints using renderer-provided port positions.

        Args:
            link: LinkData to configure
            startNode: Node where the link starts (has the output port)
            endNode: Node where the link ends (has the input port)
            startPinName: Name of the output port on startNode
            endPinName: Name of the input port on endNode
        """
        # Get port positions from renderers (isOutput=True for start, False for end)
        link.start = self._getLinkEndpointPosition(
            startNode,
            startPinName,
            True,
            str(getattr(link, "sourcePropertyName", "")),
            preferPropertySide=bool(getattr(link, "is_relationship_link", False)),
        )
        link.end = self._getLinkEndpointPosition(
            endNode,
            endPinName,
            False,
            str(getattr(link, "targetPropertyName", "")),
            preferPropertySide=bool(getattr(link, "is_relationship_link", False)),
        )

    def _updateNodeLinkEndpoints(self, node):
        """Update link endpoints for all links connected to a specific node.

        This is much faster than rebuilding all links when only one node has moved.

        Args:
            node: NodeModel whose connected links need updating
        """
        # Update input links (where this node receives data)
        # For input links: sourceNodeId = connected node (output), targetNodeId = this node
        for link in node.inputLinks:
            if link.sourceNodeId not in self.nodes:
                continue

            connectedNode = self.nodes[link.sourceNodeId]

            # Update endpoints
            self._setupLinkEndpoints(
                link, connectedNode, node, link.sourcePort, link.targetPort
            )

        # Update output links (where this node provides data)
        for link in node.outputLinks:
            if link.targetNodeId not in self.nodes:
                continue

            connectedNode = self.nodes[link.targetNodeId]

            # Update endpoints
            self._setupLinkEndpoints(
                link, node, connectedNode, link.sourcePort, link.targetPort
            )

    def _updateLinksForMovedNodes(self, movedNodeIds):
        """Update all links that connect to any of the moved nodes.

        This finds links where EITHER end connects to a moved node and updates
        both endpoints. Much more efficient than rebuilding all links.

        Args:
            movedNodeIds: Set of node IDs that moved
        """
        for linkIdx, link in enumerate(self.links):
            # Check if either end of the link connects to a moved node
            sourceNodeId = getattr(link, "data_sourceNodeId", None)
            targetNodeId = getattr(link, "data_targetNodeId", None)

            if sourceNodeId in movedNodeIds or targetNodeId in movedNodeIds:
                # At least one end moved - need to update both endpoints
                if sourceNodeId not in self.nodes or targetNodeId not in self.nodes:
                    continue

                sourceNode = self.nodes[sourceNodeId]
                targetNode = self.nodes[targetNodeId]

                # Both input and output links: sourceNode provides output, targetNode receives input
                self._setupLinkEndpoints(
                    link,
                    sourceNode,
                    targetNode,
                    link.sourcePort,
                    link.targetPort,
                )

                # The endpoints just moved, so this link's spatial-index bounds
                # are stale. Re-insert (insertLink is an upsert) so the link
                # stays pickable during the drag instead of only after the next
                # full rebuild on linksChanged.
                self._spatialIndex.insertLink(linkIdx, self._computeLinkBounds(link))

        # Endpoints moved on the link snapshot; re-mirror to C++ next paint.
        self._linksNeedSync = True

    def _setupDanglingLinkEndpoint(self, link, node, pinName, isOutput):
        """Set up a dangling link endpoint as a short straight line from the port.

        Args:
            link: LinkData to configure as dangling
            node: The node that owns the port
            pinName: Name of the port
            isOutput: True if this is an output port, False for input
        """
        # Get the port position (handles dual pins via mirroring)
        property_name = (
            str(getattr(link, "sourcePropertyName", ""))
            if isOutput
            else str(getattr(link, "targetPropertyName", ""))
        )
        portPos = self._getLinkEndpointPosition(
            node,
            pinName,
            isOutput,
            property_name,
            preferPropertySide=bool(getattr(link, "is_relationship_link", False)),
        )

        # Set the start and end points for the dangling link
        link.start = portPos

        # Calculate end point as a short line in the appropriate direction
        linkLength = LinkData.DANGLING_LINK_LENGTH
        if isOutput:
            # Output ports point to the right
            link.end = Gf.Vec2d(portPos[0] + linkLength, portPos[1])
        else:
            # Input ports point to the left
            link.end = Gf.Vec2d(portPos[0] - linkLength, portPos[1])

        link.isDangling = True
        link.danglingDirection = "output" if isOutput else "input"

    def _computeLinkBounds(self, link):
        """Curve-inclusive Gf.Range2d for the spatial index.

        Uses the shaped-curve bounds (covering the backward S-bow) rather than
        the straight start/end box, so broad-phase queries do not mis-cull
        backward links whose curve bulges outside their endpoint box. The bounds
        are zoom-independent; the 20px margin covers link line width and the
        prim-target arrow inset.
        """
        return computeLinkCurveBounds(link, margin=20.0)

    @staticmethod
    def _makeLinkFromUsdData(link_data):
        link = LinkData()
        link.sourceNodeId = link_data["sourceNodeId"]
        link.sourcePort = link_data["sourcePinName"]
        link.targetNodeId = link_data["targetNodeId"]
        link.targetPort = link_data["targetPinName"]
        link.is_input_link = bool(link_data["is_input_link"])
        link.propertyOwnerNodeId = str(link_data.get("propertyOwnerNodeId", ""))
        link.propertyName = str(link_data.get("propertyName", ""))
        link.is_relationship_link = bool(link_data.get("is_relationship_link", False))
        link.sourcePropertyName = str(link_data.get("sourcePropertyName", ""))
        link.targetPropertyName = str(link_data.get("targetPropertyName", ""))
        return link

    def _populateNodeLinksFromPrim(self, node, prim):
        node.inputLinks = []
        node.outputLinks = []
        for link_data in collect_links_for_prim(prim):
            link = self._makeLinkFromUsdData(link_data)
            if link.is_input_link:
                node.inputLinks.append(link)
            else:
                node.outputLinks.append(link)

    def _rebuildLinks(self):
        """Rebuild all links by reading USD connections directly.

        This method reads connection data directly from USD prims,
        ensuring links always reflect the current USD state. This is
        important for responding to external USD edits.
        """
        self.links.clear()

        # Phase 1: refresh each USD-backed node's cached link lists from its
        # prim WITHOUT positioning yet, so every connection endpoint is known
        # before we materialize pins for it in phase 2.
        for node in self.nodes.values():
            # Virtual nodes (Blueprint I/O) have no USD prim; keep their cached
            # inputLinks/outputLinks as-is.
            if "#" in node.id:
                continue
            # For USD-backed nodes, read connections fresh from the prim
            # (clears the cached links and rebuilds from USD).
            if node._prim and node._prim.IsValid():
                self._populateNodeLinksFromPrim(node, node._prim)

        # Phase 2: surface a pin/row for every connection endpoint that lacks
        # one, so phase-3 positioning snaps each noodle to a real pin instead of
        # the bare node edge. This covers outputs that exist only as connection
        # targets on prims whose schema is not registered (so USD never exposes
        # them as attributes), treating all properties uniformly.
        self._surfaceConnectionEndpointPins()

        # Phase 3: position links into self.links now that all rows exist.
        for node in self.nodes.values():
            self._processNodeLinks(node)

        # Scan the stage for back-references: connections authored on absent
        # prims that target present prims. Without this, an authored connection
        # like B.inputs:x.connect = A.outputs:result is invisible from A when B
        # is not in the graph, leaving A's output pin appearing disconnected.
        self._addBackReferenceDanglingLinks()

        # Deduplicate links (bare/dual-pin attributes can be discovered
        # from both endpoints, producing duplicate link entries).
        seen_links = set()
        deduped = []
        for link in self.links:
            key = (
                link.sourceNodeId,
                link.sourcePort,
                link.targetNodeId,
                link.targetPort,
                str(getattr(link, "sourcePropertyName", "")),
                str(getattr(link, "targetPropertyName", "")),
            )
            if key not in seen_links:
                seen_links.add(key)
                deduped.append(link)
        if len(deduped) != len(self.links):
            self.links.clear()
            self.links.extend(deduped)

        # Sync the freshly-rebuilt links into the C++ View Model BEFORE rebuilding
        # the connection cache, so the cache (isPortConnected / folded-header /
        # synthetic-relationship endpoints, consumed by the port-highlight pass)
        # reflects the current links rather than the previous frame's snapshot.
        # paintLinks re-syncs the same list later for per-frame hover flags; this
        # closes the update-path gap on the link-change frame itself.
        self.nodeGraph.syncLinksFromModels(self.links)
        # The snapshot was just refreshed from the rebuilt links; clear the
        # incremental sync flag and force hover to re-derive against the new link
        # indices on the next paint.
        self._linksNeedSync = False
        self._lastSyncedLinkHover = -1
        # Connections were rebuilt, so per-port connected/disconnected colors may
        # have changed; force the port-highlight geometry to rebuild next paint.
        self._portHighlightsDirty = True
        self._rebuildConnectionCache()

        # Rebuild spatial index with updated nodes and links
        with _ProfileSection(self._profiler, "Rebuild Spatial Index"):
            self._spatialIndex.clear()
            for nodeId, node in self.nodes.items():
                bounds = Gf.Range2d(
                    Gf.Vec2d(node.position),
                    Gf.Vec2d(node.position + node.size),
                )
                self._spatialIndex.insertNode(nodeId, bounds)
            for linkIndex, link in enumerate(self.links):
                bounds = self._computeLinkBounds(link)
                self._spatialIndex.insertLink(linkIndex, bounds)

        self.linksChanged = False

    def _surfaceConnectionEndpointPins(self):
        """Ensure every connection endpoint renders as a pin/row.

        A connection can reference a property that is not an authored or
        schema-declared attribute on its prim -- e.g. a typed prim whose schema
        is not registered, so its outputs exist only as the *targets* of other
        prims' connections. Such endpoints have no pin row, so the noodle
        attaches to the bare node edge with no pin. This surfaces every
        non-relationship endpoint as a dual pin row on its node, uniformly,
        regardless of schema availability. Relationship endpoints are left to
        the synthetic-port path. Idempotent: endpoints that already have a
        visible port are skipped.
        """
        endpoint_inputs: dict[str, list[str]] = {}
        endpoint_outputs: dict[str, list[str]] = {}

        def _note(mapping, node_id, pin_name):
            if not node_id or not pin_name or node_id not in self.nodes:
                return
            mapping.setdefault(node_id, []).append(pin_name)

        for node in self.nodes.values():
            for link in list(node.inputLinks) + list(node.outputLinks):
                if getattr(link, "is_relationship_link", False):
                    continue
                _note(endpoint_outputs, link.sourceNodeId, link.sourcePort)
                _note(endpoint_inputs, link.targetNodeId, link.targetPort)

        for node_id in set(endpoint_inputs) | set(endpoint_outputs):
            node = self.nodes.get(node_id)
            if node is None or not hasattr(node, "add_connection_endpoint_pins"):
                continue
            added = node.add_connection_endpoint_pins(
                endpoint_inputs.get(node_id, ()),
                endpoint_outputs.get(node_id, ()),
            )
            # Added rows change the node's row count, so recompute its size for
            # correct hit-testing and spatial bounds.
            if added and self.fontAtlas:
                self.nodeGraph._calculateNodeSize(
                    node,
                    self.textRenderer.calculateTextWidth,
                    self.fontAtlas,
                )

    def _processNodeLinks(self, node):
        """Process a node's inputLinks and outputLinks into renderable links.

        Args:
            node: NodeModel whose links should be processed
        """
        # Process input links (connections on input attributes)
        # For input links: sourceNodeId = connected node (provides output),
        #                   targetNodeId = this node (receives input)
        for link in node.inputLinks:
            if link.sourceNodeId not in self.nodes:
                # Source node is not in the graph - create dangling link.
                # Applies to both attribute and relationship links: the port
                # stays filled and double-click adds the missing prim back.
                self._setupDanglingLinkEndpoint(
                    link, node, link.targetPort, isOutput=False
                )
                link.data_sourceNodeId = (
                    link.sourceNodeId
                )  # The missing node would be source
                link.data_targetNodeId = node.id  # This node receives data
                link.is_input_link = True
                self.links.append(link)
                continue

            connectedNode = self.nodes[link.sourceNodeId]

            # Setup link endpoints using renderer port positions
            # startNode = connectedNode (output side), endNode = this node (input side)
            self._setupLinkEndpoints(
                link, connectedNode, node, link.sourcePort, link.targetPort
            )

            # Track actual data flow direction:
            # For inputLinks: data flows FROM connectedNode TO current node
            link.data_sourceNodeId = (
                connectedNode.id
            )  # Node providing data (has output)
            link.data_targetNodeId = node.id  # Node receiving data (has input)

            link.is_input_link = True
            link.isDangling = False

            self.links.append(link)

        # Process output links (connections on output attributes)
        for link in node.outputLinks:
            if link.targetNodeId not in self.nodes:
                # Target node is not in the graph - create dangling link.
                # Applies to both attribute and relationship links: the port
                # stays filled and double-click adds the missing prim back.
                self._setupDanglingLinkEndpoint(
                    link, node, link.sourcePort, isOutput=True
                )
                link.data_sourceNodeId = node.id  # This node provides data
                link.data_targetNodeId = (
                    link.targetNodeId
                )  # The missing node would receive
                link.is_input_link = False
                self.links.append(link)
                continue

            connectedNode = self.nodes[link.targetNodeId]

            # Setup link endpoints using renderer port positions
            self._setupLinkEndpoints(
                link, node, connectedNode, link.sourcePort, link.targetPort
            )

            # Track actual data flow direction:
            # For outputLinks: data flows FROM current node TO connectedNode
            link.data_sourceNodeId = node.id  # Node providing data (has output)
            link.data_targetNodeId = connectedNode.id  # Node receiving data (has input)

            link.is_input_link = False
            link.isDangling = False

            self.links.append(link)

    def _invalidateBackReferenceLinkDataCache(self):
        """Drop the cached per-prim USD link scan used for back-references.

        Called when stage composition changes (resync notice, stage replace)
        so the next ``_addBackReferenceDanglingLinks`` call re-traverses the
        stage. In steady state (no USD edits between paints) the cache is
        reused, avoiding the O(prim-count) traversal on every paint.
        """
        self._backReferenceLinkDataCache = None

    def _addBackReferenceDanglingLinks(self):
        """Add dangling links for connections authored on prims NOT in the graph
        that reference prims IN the graph.

        USD attribute connections are usually authored on the input side
        (``downstream.inputs:x.connect = </Upstream.outputs:result>``). When the
        downstream prim is in the graph but the upstream prim is not, the input
        side is handled by ``_processNodeLinks`` via the downstream node's
        cached link list. The reverse — upstream in graph, downstream absent —
        is NOT visible from the upstream prim's own attributes, because the
        connection is authored on the downstream prim's input.

        This scan walks the stage once and caches the per-prim USD link data
        keyed by prim path. The cache is invalidated by
        ``_invalidateBackReferenceLinkDataCache`` when stage composition
        changes (see ``_handleUsdChanges`` and ``_onStageReplaced``); between
        invalidations the cached data is reused, so subsequent
        ``_rebuildLinks`` calls do not re-traverse the stage. The graph-side
        ``self.nodes`` membership filter is applied at use time, not cache
        time, because graph membership changes (add/remove node from the
        editor) do not affect what USD authored.
        """
        nodeGraph = getattr(self, "nodeGraph", None)
        stage = nodeGraph.getStage() if nodeGraph is not None else None
        if not stage:
            return

        # Build / reuse the per-prim USD link cache.
        cache = getattr(self, "_backReferenceLinkDataCache", None)
        if cache is None:
            cache = {}
            for prim in stage.Traverse():
                try:
                    link_dicts = collect_links_for_prim(prim)
                except (RuntimeError, TypeError, AttributeError):
                    continue
                if not link_dicts:
                    continue
                cache[str(prim.GetPath())] = link_dicts
            self._backReferenceLinkDataCache = cache

        # Build a key set of links already added so we don't duplicate the
        # forward direction the per-node pass already produced.
        existing_keys = set()
        for existing in self.links:
            existing_keys.add(
                (
                    existing.sourceNodeId,
                    existing.sourcePort,
                    existing.targetNodeId,
                    existing.targetPort,
                    str(getattr(existing, "sourcePropertyName", "")),
                    str(getattr(existing, "targetPropertyName", "")),
                )
            )

        for prim_path_str, link_dicts in cache.items():
            if prim_path_str in self.nodes:
                # Forward direction already handled by _processNodeLinks.
                continue

            for link_data in link_dicts:
                src_id = str(link_data.get("sourceNodeId", ""))
                tgt_id = str(link_data.get("targetNodeId", ""))

                present_is_source = src_id in self.nodes
                present_is_target = tgt_id in self.nodes
                if not present_is_source and not present_is_target:
                    continue

                key = (
                    src_id,
                    str(link_data.get("sourcePinName", "")),
                    tgt_id,
                    str(link_data.get("targetPinName", "")),
                    str(link_data.get("sourcePropertyName", "")),
                    str(link_data.get("targetPropertyName", "")),
                )
                if key in existing_keys:
                    continue
                existing_keys.add(key)

                link = self._makeLinkFromUsdData(link_data)

                if present_is_source:
                    present_node = self.nodes[src_id]
                    self._setupDanglingLinkEndpoint(
                        link, present_node, link.sourcePort, isOutput=True
                    )
                    link.data_sourceNodeId = src_id
                    link.data_targetNodeId = tgt_id
                    link.is_input_link = False
                else:
                    present_node = self.nodes[tgt_id]
                    self._setupDanglingLinkEndpoint(
                        link, present_node, link.targetPort, isOutput=False
                    )
                    link.data_sourceNodeId = src_id
                    link.data_targetNodeId = tgt_id
                    link.is_input_link = True

                self.links.append(link)

    def _rebuildConnectionCache(self):
        self.nodeGraph.buildConnectionCache()

    def getConnectedNodes(self, nodeId):
        return self.nodeGraph.getConnectedNodeIds(nodeId)

    def resizeGL(self, w, h):
        self.makeCurrent()
        ctx = self.context()
        if ctx is None or not ctx.isValid():
            return
        GL.glViewport(0, 0, w, h)
        # Invalidate caches on resize
        self._nodeRenderManager.invalidateAll()
        self.doneCurrent()

    def paintGL(self):
        if not self.isVisible():
            return

        if not self.initialized:
            self.initializeGL()
            if not self.initialized:
                return

        self.makeCurrent()

        # Verify the GL context is actually current before issuing GL calls.
        # When docked alongside another QOpenGLWidget (e.g. usdview's Hydra
        # viewport), makeCurrent() can silently fail if the context was lost
        # or the surface is not yet valid.
        ctx = self.context()
        if ctx is None or not ctx.isValid():
            return

        # Detect stale GL context (e.g. after usdview "Reopen Stage").
        # The font atlas texture serves as a canary — if it is no longer
        # a valid texture name, the context was recreated and all cached
        # GL resources (VAOs, VBOs, shaders) are stale.
        if self.fontAtlas and self.fontAtlas.texture:
            if not GL.glIsTexture(self.fontAtlas.texture):
                self._reinitializeGL()
                if not self.initialized:
                    return

        try:
            self._paintGLInner()
        except GLError as e:
            Tf.Warn(f"GL error during paintGL: {e}")

    def _paintGLInner(self):
        # Enable profiling based on cached config
        self._profiler.enabled = self._cachedEnableRenderProfiling
        self._profiler.startFrame()

        with _ProfileSection(self._profiler, "Animator Update"):
            self._updateAnimator()

        with _ProfileSection(self._profiler, "Pan Animation"):
            self._updatePanAnimation()

        with _ProfileSection(self._profiler, "Keyboard State"):
            self._updateKeyboardState()

        # Apply any node-position changes to the spatial index / links / port
        # highlights before rendering or hit-testing this frame. O(1) when
        # nothing moved.
        with _ProfileSection(self._profiler, "Reconcile Moves"):
            self._reconcileMovedNodes()

        with _ProfileSection(self._profiler, "Rebuild Links"):
            if self.linksChanged:
                self._rebuildLinks()

        with _ProfileSection(self._profiler, "GL Clear"):
            GL.glClearColor(0.25, 0.25, 0.25, 1.0)
            GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
            GL.glDepthFunc(GL.GL_LEQUAL)

        # Rendering:

        with _ProfileSection(self._profiler, "Group Stickers"):
            # group stickers first (background layer, behind everything)
            if self.drawNodes and self.groupStickers:
                self.paintGroupStickers()

        with _ProfileSection(self._profiler, "Links"):
            # links (below nodes)
            if self.drawLinks:
                self.paintLinks()

        # Render temporary link during drag (after links, before nodes)
        if self._draggingLink and self._dragLinkTempLink:
            self._renderTemporaryLink()

        with _ProfileSection(self._profiler, "Nodes"):
            # nodes (on top of links)
            if self.drawNodes:
                self.paintNodes()

        with _ProfileSection(self._profiler, "Port Highlights"):
            if self.nodes:
                self._renderPortHighlightsCpp()

        with _ProfileSection(self._profiler, "Text"):
            # text (on top of nodes)
            if self.drawText:
                self.paintText()

        with _ProfileSection(self._profiler, "Status Overlay"):
            # status overlay (screen space, on top of everything)
            self.paintStatusOverlay()

        with _ProfileSection(self._profiler, "Minimap"):
            # minimap (screen space, upper right corner)
            if self._cachedShowMinimap:
                self.minimap.paint(NodeVertex, MAX_RENDER_DEPTH)

        with _ProfileSection(self._profiler, "Hotkey Popups"):
            self._paintHotkeyPopups()

        with _ProfileSection(self._profiler, "Marquee"):
            # marquee selection (on top of everything else)
            if self.marqueeActive:
                self.paintMarquee()

        with _ProfileSection(self._profiler, "Node Creation Hotbox"):
            if self.nodeCreationHotbox.is_visible:
                self._paintNodeCreationHotbox()

        self._profiler.endFrame()
        self._printProfilingStatsIfNeeded()

    def _updateAnimator(self):
        """Advance the animator by the elapsed frame time."""
        if not self.frameTimer.isValid():
            self.frameTimer.start()
            self.animator.update(0)
        else:
            nsecsElapsed = self.frameTimer.nsecsElapsed()
            self.frameTimer.restart()
            dt = nsecsElapsed / 1e9
            self.animator.update(dt)

    def _updatePanAnimation(self):
        """Update pan position from animation and check for completion."""
        if not self._panAnimating:
            return
        self.panX = self._panXAnim.current
        self.panY = self._panYAnim.current

        panXDone = abs(self._panXAnim.current - self._panXAnim.target) < 0.5
        panYDone = abs(self._panYAnim.current - self._panYAnim.target) < 0.5
        if panXDone and panYDone:
            self.panX = self._panXAnim.target
            self.panY = self._panYAnim.target
            self._panAnimating = False

    def _updateKeyboardState(self):
        """Poll keyboard modifiers and update animation state."""
        if self.animator.isAnimating():
            self.update()
        else:
            self.frameTimer.invalidate()

    def _renderPortHighlightsCpp(self):
        """C++-owned port-highlight pass.

        Delegates the entire port-circle / relationship-triangle assembly to
        NodeRenderManager.renderPortHighlightsFromGraph, which reads the
        authoritative GraphModel snapshot (synced in paintNodes/paintLinks) and
        the C++ connection index. This is the sole port-highlight render path.
        """
        graph = self.nodeGraph
        config = self._renderConfig()
        # Push the cached port appearance onto the shared render config (cheap and
        # idempotent); the C++ producer reads these instead of the Python caches.
        # These writes intentionally persist after useCppPortHighlights is toggled
        # off: the fields have no other consumers, so leaving them set is harmless.
        config.nodePortRingThickness = self._cachedPortRingThickness
        config.portConnectedFillColor = list(self._cachedPortConnectedFillColor)
        config.portConnectedRingColor = list(self._cachedPortConnectedRingColor)
        config.portDisconnectedFillColor = list(self._cachedPortDisconnectedFillColor)
        config.portDisconnectedRingColor = list(self._cachedPortDisconnectedRingColor)
        config.portHoverColor = list(self._cachedPortHoverColor)

        projList = list(self._worldSpaceProjectionMatrix().data())
        cornerRadius = config.nodePortWidth * 0.5

        hovered = self._hoveredPort
        # Rebuild the highlight geometry only when something it depends on
        # changed: an in-place connection change (_portHighlightsDirty), a
        # content / color / relayout / move change (textChanged is still set this
        # frame -- paintText clears it later), or an in-progress node / link drag
        # (the circles bake the live drag offset, so they must rebuild each drag
        # frame). Hover and link-drag-state changes are detected C++-side by
        # comparing PortHighlightState, so they need not be flagged here. A plain
        # pan / zoom changes none of these, so the C++ side skips the O(ports)
        # build + hash and just redraws the cached geometry.
        port_geometry_changed = bool(
            self._portHighlightsDirty
            or self.textChanged
            or self.draggingNodes
            or self._draggingLink
        )
        self._nodeRenderManager.renderPortHighlightsFromGraph(
            graph,
            config,
            projList,
            cornerRadius,
            hovered is not None,
            hovered[0] if hovered else "",
            hovered[1] if hovered else "",
            bool(hovered[2]) if hovered else False,
            bool(self._draggingLink),
            self._dragLinkSourceNode or "",
            self._dragLinkSourcePort or "",
            bool(self._dragLinkSourceIsOutput),
            port_geometry_changed,
        )
        self._portHighlightsDirty = False

    def _paintHotkeyPopups(self):
        """Render hotkey popup overlays."""
        if self.hotkeyPopup.is_visible:
            self.paintHotkeyPopup()
        if self.navigationHotkeyPopup.is_visible:
            self.paintNavigationHotkeyPopup()

    def _paintNodeCreationHotbox(self):
        """Render the node creation hotbox overlay."""
        projection = self._screenSpaceProjectionMatrix()
        self.nodeCreationHotbox.render(
            self.textRenderer,
            NodeVertex,
            self._drawNodeVertices,
            projection,
            self.width(),
            self.height(),
            MAX_RENDER_DEPTH,
        )

    def _printProfilingStatsIfNeeded(self):
        """Print profiling stats every N frames when profiling is enabled."""
        if self._profiler.enabled:
            self._profilingFrameCount += 1
            print_interval = self._cachedProfilingPrintInterval
            if self._profilingFrameCount >= print_interval:
                self._profiler.printStats()
                self._profilingFrameCount = 0
                self._profiler.reset()

    def _worldSpaceProjectionMatrix(self):
        # Reuse the cached matrix unless the view (pan/zoom/size) changed since
        # it was built; the key comparison makes this self-invalidating.
        key = (self.panX, self.panY, self.zoom, self.width(), self.height())
        if key != self._worldProjCacheKey:
            projection = QtGui.QMatrix4x4()
            scaledWidth = self.width() / self.zoom
            scaledHeight = self.height() / self.zoom
            projection.ortho(
                self.panX,
                self.panX + scaledWidth,
                self.panY + scaledHeight,
                self.panY,
                -MAX_RENDER_DEPTH,
                MAX_RENDER_DEPTH,
            )
            self._worldProjCacheKey = key
            self._worldProjCache = projection
        return self._worldProjCache

    def _screenSpaceProjectionMatrix(self):
        key = (self.width(), self.height())
        if key != self._screenProjCacheKey:
            projection = QtGui.QMatrix4x4()
            projection.ortho(
                0, self.width(), self.height(), 0, -MAX_RENDER_DEPTH, MAX_RENDER_DEPTH
            )
            self._screenProjCacheKey = key
            self._screenProjCache = projection
        return self._screenProjCache

    def wheelEvent(self, event):
        # If hotbox is visible, scroll the list instead of zooming
        if self.nodeCreationHotbox.is_visible:
            # Convert wheel delta to scroll steps
            delta = event.angleDelta().y()
            steps = (
                -delta // 120
            )  # Negative delta (wheel down) = positive steps (scroll down)
            self.nodeCreationHotbox.handle_scroll(steps)
            self.update()
            return

        mousePos = event.position()
        delta = event.angleDelta().y()
        # Use multiplicative zoom factor for smoother, bounded zooming
        # Each wheel notch (typically 120 units) zooms by this factor
        zoomFactor = 1.15  # 15% zoom per wheel notch
        steps = delta / 120.0  # Normalize to wheel notches
        newZoom = self.zoom * pow(zoomFactor, steps)
        self._zoomAroundPoint(newZoom, (mousePos.x(), mousePos.y()))
        self.update()

    def _collectSiblingDanglingLinks(self, seed_link):
        """Return every dangling link that shares ``seed_link``'s present-side port.

        A USD relationship with N targets produces N separate dangling
        ``LinkData`` objects that all share the source prim/port and stack
        their indicators on top of each other.  When the user expands the
        indicator we want to bring in ALL of those missing targets, not just
        the first one a port-hit search happens to return.
        """
        if not getattr(seed_link, "isDangling", False):
            return [seed_link]

        if seed_link.sourceNodeId in self.nodes:
            present_id = seed_link.sourceNodeId
            present_port = seed_link.sourcePort
            present_property = str(getattr(seed_link, "sourcePropertyName", ""))
            present_attr = "sourceNodeId"
            present_port_attr = "sourcePort"
            present_property_attr = "sourcePropertyName"
        elif seed_link.targetNodeId in self.nodes:
            present_id = seed_link.targetNodeId
            present_port = seed_link.targetPort
            present_property = str(getattr(seed_link, "targetPropertyName", ""))
            present_attr = "targetNodeId"
            present_port_attr = "targetPort"
            present_property_attr = "targetPropertyName"
        else:
            return [seed_link]

        siblings = []
        for link in self.links:
            if not getattr(link, "isDangling", False):
                continue
            if getattr(link, present_attr, "") != present_id:
                continue
            if getattr(link, present_port_attr, "") != present_port:
                continue
            if str(getattr(link, present_property_attr, "")) != present_property:
                continue
            siblings.append(link)
        return siblings or [seed_link]

    def _addNodesFromDanglingLink(self, seed_link):
        """Add every missing target for the dangling-link expansion ``seed_link``.

        Returns True when at least one node was added.  Wraps
        :meth:`_addNodeFromDanglingLink` so a USD relationship with multiple
        targets is fully expanded in a single user gesture.
        """
        added_any = False
        for link in self._collectSiblingDanglingLinks(seed_link):
            if self._addNodeFromDanglingLink(link):
                added_any = True
        return added_any

    def _addNodeFromDanglingLink(self, link):
        """Add the missing node referenced by a dangling link.

        Args:
            link: The dangling LinkData whose missing node should be added

        Returns:
            True if the node was successfully added, False otherwise
        """
        if not self._usdviewApi:
            self._showPopupMessage("Cannot add node: no USD stage")
            return False

        stage = self._usdviewApi.stage
        if not stage:
            self._showPopupMessage("Cannot add node: no USD stage")
            return False

        # Determine which node is missing
        missingNodeId = None
        if link.isDangling:
            # Check which end of the link is missing
            if link.data_sourceNodeId and link.data_sourceNodeId not in self.nodes:
                missingNodeId = link.data_sourceNodeId
            elif link.data_targetNodeId and link.data_targetNodeId not in self.nodes:
                missingNodeId = link.data_targetNodeId

        if not missingNodeId:
            self._showPopupMessage("No missing node to add")
            return False

        # Get the prim from USD
        prim = stage.GetPrimAtPath(missingNodeId)
        if not prim or not prim.IsValid():
            self._showPopupMessage(f"Prim not found: {missingNodeId}")
            return False

        # Use NodeFactory to create the node
        node = self._nodeFactory.create_node_from_prim(prim, stage)
        if not node:
            self._showPopupMessage(
                f"Cannot create node for prim type: {prim.GetTypeName()}"
            )
            return False

        self._populateNodeLinksFromPrim(node, prim)

        # Calculate node size
        if self.fontAtlas:
            self.nodeGraph._calculateNodeSize(
                node, self.textRenderer.calculateTextWidth, self.fontAtlas
            )

        # Position the new node in the viewport (avoids USD default position
        # leaving the node at (0,0)) and assign a z-order, matching the
        # _addPrimAsNode setup so the renderer can compute pin positions
        # consistently before _rebuildLinks runs on the next paint.
        self._positionNodeInViewport(node)
        self.nodes[node.id] = node
        self._nextZOrder += 1
        node.zOrder = self._nextZOrder

        # Enable prim tree syncing since we now have nodes with valid USD paths
        self.nodeGraph.syncSelectionToPrimTree = True

        self.linksChanged = True
        self.textChanged = True
        self._clearRenderCache()
        self.update()

        self._showPopupMessage(f"Added: {prim.GetName()}")
        return True

    def _findDanglingLinkAtPort(self, worldPos):
        """Find a dangling link whose present-node port is under worldPos.

        Returns the index into self.links, or -1 if none.
        """
        for i, link in enumerate(self.links):
            if not link.isDangling:
                continue
            if link.sourceNodeId in self.nodes:
                nodeId = link.sourceNodeId
                portName = link.sourcePort
                isOutput = True
            elif link.targetNodeId in self.nodes:
                nodeId = link.targetNodeId
                portName = link.targetPort
                isOutput = False
            else:
                continue
            node = self.nodes.get(nodeId)
            if not node:
                continue
            renderer = node.renderer if node.renderer else self.defaultRenderer
            if not renderer:
                continue
            portPos = self._getPortPosition(node, portName, isOutput)
            if not portPos:
                continue
            hitRadius = renderer.getPortWidth() * self._cachedPortHitRadiusMultiplier
            dx = worldPos[0] - portPos[0]
            dy = worldPos[1] - portPos[1]
            if dx * dx + dy * dy <= hitRadius * hitRadius:
                return i
        return -1

    def mouseDoubleClickEvent(self, event):
        """Handle double-click events.

        Double-clicking on a dangling link or on a connected port whose
        connected node is missing adds the missing node to the graph.
        Double-clicking a non-dangling relationship link jumps to the
        endpoint farthest from the cursor so users can follow links to
        connected nodes that lie outside the current viewport.
        """
        if event.button() != QtCore.Qt.LeftButton:
            return

        worldMousePos = (
            QtCore.QPointF(event.position().toPoint()) / self.zoom
        ) + QtCore.QPointF(self.panX, self.panY)
        worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())

        # Check if we clicked on a connected port that has a dangling link.
        # _findDanglingLinkAtPort by construction only returns indices of
        # links with isDangling=True, so we don't re-check the flag here.
        portDanglingIdx = self._findDanglingLinkAtPort(worldPos)
        if portDanglingIdx >= 0 and portDanglingIdx < len(self.links):
            self._addNodesFromDanglingLink(self.links[portDanglingIdx])
            return

        # Also check via curve hit-test for backward compatibility
        clickedLinkIndex = self.linkRenderer.findLinkUnderCursor(
            worldPos,
            self.links,
            self.zoom,
            self._cachedLinkLineWidth,
            spatialIndex=self._spatialIndex,
            debug=False,
        )

        if clickedLinkIndex >= 0 and clickedLinkIndex < len(self.links):
            link = self.links[clickedLinkIndex]
            if link.isDangling:
                self._addNodesFromDanglingLink(link)
                return
            if getattr(link, "is_relationship_link", False):
                if self._navigateAlongRelationshipLink(link, worldPos):
                    return

        # Default behavior for double-click on non-dangling elements
        super().mouseDoubleClickEvent(event)

    def _navigateAlongRelationshipLink(self, link, worldPos):
        """Jump to the endpoint of ``link`` farthest from ``worldPos``.

        Returns True when navigation occurred. Used by the double-click
        handler so users can follow a relationship into a connected node
        that may be scrolled outside the current viewport.
        """
        sourceNodeId = getattr(link, "data_sourceNodeId", "") or getattr(
            link, "sourceNodeId", ""
        )
        targetNodeId = getattr(link, "data_targetNodeId", "") or getattr(
            link, "targetNodeId", ""
        )

        startPos = getattr(link, "start", None)
        endPos = getattr(link, "end", None)
        prefer_target = True
        if startPos is not None and endPos is not None:
            dx_start = float(worldPos[0]) - float(startPos[0])
            dy_start = float(worldPos[1]) - float(startPos[1])
            dx_end = float(worldPos[0]) - float(endPos[0])
            dy_end = float(worldPos[1]) - float(endPos[1])
            prefer_target = (dx_start * dx_start + dy_start * dy_start) <= (
                dx_end * dx_end + dy_end * dy_end
            )

        if prefer_target and targetNodeId and targetNodeId in self.nodes:
            self._jumpToTargetNodeInternal(link, targetNodeId)
            return True
        if not prefer_target and sourceNodeId and sourceNodeId in self.nodes:
            self._jumpToSourceNodeInternal(link, sourceNodeId)
            return True
        # Fall back to whichever endpoint is reachable.
        if targetNodeId and targetNodeId in self.nodes:
            self._jumpToTargetNodeInternal(link, targetNodeId)
            return True
        if sourceNodeId and sourceNodeId in self.nodes:
            self._jumpToSourceNodeInternal(link, sourceNodeId)
            return True
        return False

    def mousePressEvent(self, event):
        self.setFocus()
        self.lastMousePos = event.position().toPoint()
        self.ctrlPressedOnMouseDown = event.modifiers() & QtCore.Qt.ControlModifier
        self.mousePosOnMouseDown = event.position().toPoint()
        # Resolve any unpainted position change before hit-testing what is under
        # the cursor (e.g. a programmatic move then an immediate click). O(1) when
        # nothing moved.
        self._reconcileMovedNodes()

        # If hotbox is visible, check if click is within hotbox
        if self.nodeCreationHotbox.is_visible:
            if self.nodeCreationHotbox.handle_mouse_press(event.position().toPoint()):
                self.update()
                return

        if event.buttons() & QtCore.Qt.LeftButton and self._cachedShowMinimap:
            if self.minimap.handleMousePress(event.position().toPoint()):
                return

        self._updateNodeUnderCursor(event.position().toPoint())

        if event.buttons() & QtCore.Qt.LeftButton and not self.spacePressed:
            # Check if clicking on the title bar caret to toggle title collapse
            if self.nodeIdUnderCursor and self.nodeIdUnderCursor in self.nodes:
                clickedNodeForTitle = self.nodes[self.nodeIdUnderCursor]
                if clickedNodeForTitle.renderer:
                    worldMousePos = (
                        QtCore.QPointF(event.position().toPoint()) / self.zoom
                    ) + QtCore.QPointF(self.panX, self.panY)
                    worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())
                    renderer = clickedNodeForTitle.renderer
                    nodeMarginH = renderer.getPortMarginH()
                    titleFontSize = renderer.getTitleFontSize()
                    # Cached title height (includes the schema-type subtitle) so the
                    # caret hit box matches the rendered title bar exactly.
                    titleHeight = clickedNodeForTitle.layoutTitleHeight
                    nx = clickedNodeForTitle.position[0]
                    ny = clickedNodeForTitle.position[1]
                    caret_size = titleFontSize * 0.5
                    caret_x = nx + nodeMarginH
                    caret_y_top = ny
                    caret_y_bot = ny + titleHeight
                    caret_x_right = caret_x + caret_size
                    if (
                        caret_x <= worldPos[0] <= caret_x_right
                        and caret_y_top <= worldPos[1] <= caret_y_bot
                    ):
                        clickedNodeForTitle.toggle_title_fold()
                        if self.fontAtlas:
                            self.nodeGraph._calculateNodeSize(
                                clickedNodeForTitle,
                                self.textRenderer.calculateTextWidth,
                                self.fontAtlas,
                            )
                        self.textChanged = True
                        self.linksChanged = True
                        self._nodeRenderManager.invalidateAll()
                        self.update()
                        return

            # Check if clicking on a group header to toggle fold
            if self.nodeIdUnderCursor and self.nodeIdUnderCursor in self.nodes:
                clickedNodeForFold = self.nodes[self.nodeIdUnderCursor]
                if clickedNodeForFold.renderer:
                    worldMousePos = (
                        QtCore.QPointF(event.position().toPoint()) / self.zoom
                    ) + QtCore.QPointF(self.panX, self.panY)
                    worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())
                    headerResult = clickedNodeForFold.renderer.getGroupHeaderAtPoint(
                        clickedNodeForFold, worldPos, self.fontAtlas
                    )
                    if headerResult and headerResult["found"]:
                        headerName = headerResult["portName"]
                        isOutput = headerResult["isOutput"]
                        clickedNodeForFold.toggle_fold(headerName, isOutput)
                        if self.fontAtlas:
                            self.nodeGraph._calculateNodeSize(
                                clickedNodeForFold,
                                self.textRenderer.calculateTextWidth,
                                self.fontAtlas,
                            )
                        self.textChanged = True
                        self.linksChanged = True
                        self._nodeRenderManager.invalidateAll()
                        self.update()
                        return

            # Check if clicking on a port to start link creation
            if self._hoveredPort:
                nodeId, portName, isOutput = self._hoveredPort
                # Start link drag
                self._startLinkDrag(nodeId, portName, isOutput)
                self.update()
                return

            worldMousePosForLink = (
                QtCore.QPointF(event.position().toPoint()) / self.zoom
            ) + QtCore.QPointF(self.panX, self.panY)
            worldPosForLink = Gf.Vec2d(
                worldMousePosForLink.x(), worldMousePosForLink.y()
            )

            # Grab a relationship arrowhead to reconnect it (or drop on empty to
            # disconnect). Checked before node selection so an arrowhead that
            # overlaps its target node is still grabbable.
            arrowheadLinkIndex = self._findArrowheadUnderCursor(worldPosForLink)
            if arrowheadLinkIndex >= 0:
                self.clearSelection()
                self.clearLinkSelection()
                self._startLinkReconnect(arrowheadLinkIndex, worldPosForLink)
                self.update()
                return

            # Start a new connection by dragging from a row's left/right edge
            # band (the outer tenth of each side), even with no visible pin.
            # Presses in the middle of the row, on the title, or below the last
            # row fall through to node selection/drag.
            if self.nodeIdUnderCursor and self.nodeIdUnderCursor in self.nodes:
                edgeNode = self.nodes[self.nodeIdUnderCursor]
                edgeHit = self._findRowEdgeDragStart(edgeNode, worldPosForLink)
                if edgeHit:
                    self._startLinkDrag(self.nodeIdUnderCursor, edgeHit[0], edgeHit[1])
                    self.update()
                    return

            clickedNode = None
            if self.nodeIdUnderCursor and self.nodeIdUnderCursor in self.nodes:
                clickedNode = self.nodes[self.nodeIdUnderCursor]

            if clickedNode:
                if not self.ctrlPressedOnMouseDown:
                    if not clickedNode.selected:
                        self.clearSelection()
                        self.clearLinkSelection()
                    clickedNode.selected = True
                    self._selectedNodes.add(clickedNode.id)
                else:
                    clickedNode.selected = not clickedNode.selected
                    if clickedNode.selected:
                        self._selectedNodes.add(clickedNode.id)
                    else:
                        self._selectedNodes.discard(clickedNode.id)
                self.draggingNodes = True

                # Bring the clicked node to front. Node-background quads are
                # regenerated from node.zOrder every frame, so the box raises
                # for free; for text we patch only this node's depth in the held
                # vertex buffer (O(node glyphs)). We must NOT set
                # self.textChanged here — that re-lays out every node's text and
                # cost ~2s for a dozen nodes due to the O(N^2) string-by-string
                # Python<->C++ vertex marshalling.
                self._nextZOrder += 1
                clickedNode.zOrder = self._nextZOrder
                self.textRenderer.patchNodeTextDepth(clickedNode)
                # Node-quad depth is baked at generation; re-bumping z-order needs
                # a regen (the selection-set check above misses a re-click on an
                # already-selected node).
                self._nodeRenderManager.markNodeQuadDirty(True)
                # The title icon's depth is also baked at generation, but the icon
                # path only rebuilds on a content change (which selection skips for
                # perf). Without this, the icon stays at its old depth and the
                # re-raised node quad occludes it (the icon visibly drops on
                # select). markIconsDirty re-bakes it at the new zOrder next frame,
                # reading the snapshot the node-quad path re-syncs this same frame,
                # so the icon rides to front with the node.
                self._cppIconRenderer.markIconsDirty()

                # Begin drag on all selected nodes for batched USD writes
                for node in self.nodes.values():
                    if node.selected:
                        node.beginDrag()

                # Update link selection based on current mode
                self._updateLinkSelectionFromNodes()
            else:
                worldMousePos = (
                    QtCore.QPointF(event.position().toPoint()) / self.zoom
                ) + QtCore.QPointF(self.panX, self.panY)
                worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())

                clickedLinkIndex = self.linkRenderer.findLinkUnderCursor(
                    worldPos,
                    self.links,
                    self.zoom,
                    self._cachedLinkLineWidth,
                    spatialIndex=self._spatialIndex,
                    debug=False,
                )

                if clickedLinkIndex >= 0:
                    if self.ctrlPressedOnMouseDown:
                        self._toggleLinkSelection(clickedLinkIndex)
                    else:
                        clickedLink = self.links[clickedLinkIndex]
                        # Dangling links are selectable on click
                        # Regular and relationship links start reconnect on click
                        if clickedLink.isDangling:
                            self.clearSelection()
                            self._selectLink(clickedLinkIndex, addToSelection=False)
                        else:
                            self.clearSelection()
                            self.clearLinkSelection()
                            self._startLinkReconnect(clickedLinkIndex, worldPos)
                else:
                    if not self.ctrlPressedOnMouseDown:
                        self.clearSelection()
                        self.clearLinkSelection()
                        # Do not sync empty selection back to prim tree — the user
                        # may press A next to add the still-selected prim(s).
                    self._marqueeBaseSelectedNodes = set(self._selectedNodes)
                    self._marqueeBaseSelectedLinks = set(self._selectedLinks)
                    self.marqueeActive = True
                    self.marqueeStart = worldMousePos
                    self.marqueeCurrent = worldMousePos

        self.update()

    def mouseMoveEvent(self, event):
        # If hotbox is visible, update hover state
        if self.nodeCreationHotbox.is_visible:
            if self.nodeCreationHotbox.handle_mouse_move(event.position().toPoint()):
                self.update()
                return

        if self._cachedShowMinimap:
            self.minimap.updateHoverState(event.position().toPoint())

            if self.minimap.handleMouseMove(event.position().toPoint()):
                self.lastMousePos = event.position().toPoint()
                return

        # During an interactive pan or pan-zoom drag (middle-button, or
        # space + left-button), skip all hover/hit-testing entirely. The cursor
        # is not hovering content during a viewport drag, and these per-move
        # scans (node-under-cursor, hovered port, property tooltip, and link
        # hit-testing) are the dominant cost on large scenes -- they are why
        # panning feels heavier than zooming. The pan/zoom itself is handled
        # below; this mirrors the node-drag and link-drag branches, which also
        # bypass hover updates.
        isViewportDrag = bool(event.buttons() & QtCore.Qt.MiddleButton) or (
            self.spacePressed and bool(event.buttons() & QtCore.Qt.LeftButton)
        )

        if not isViewportDrag:
            self._updateNodeUnderCursor(event.position().toPoint())

            if not self.draggingNodes:
                if self._updateHoveredPort(event.position().toPoint()):
                    self.update()

            if not self.draggingNodes and not self._draggingLink:
                # Update property tooltip on hover
                self._updatePropertyTooltip(event.position().toPoint())

                # Update link under cursor for hover highlighting
                worldMousePos = (
                    QtCore.QPointF(event.position().toPoint()) / self.zoom
                ) + QtCore.QPointF(self.panX, self.panY)
                worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())

                newLinkUnderCursor = self.linkRenderer.findLinkUnderCursor(
                    worldPos,
                    self.links,
                    self.zoom,
                    self._cachedLinkLineWidth,
                    spatialIndex=self._spatialIndex,
                )
                if newLinkUnderCursor != self._linkUnderCursor:
                    self._linkUnderCursor = newLinkUnderCursor
                    self.update()

        # Handle link dragging
        if self._draggingLink:
            self._updateLinkDrag(event.position().toPoint())
            self.update()
            return

        if event.buttons() & QtCore.Qt.MiddleButton or (
            self.spacePressed and event.buttons() & QtCore.Qt.LeftButton
        ):
            if self.ctrlPressedOnMouseDown:
                newZoom = self.zoom * (
                    1.0
                    + (event.position().toPoint().x() - self.lastMousePos.x()) / 120.0
                )
                focus = (self.mousePosOnMouseDown.x(), self.mousePosOnMouseDown.y())
                self._zoomAroundPoint(newZoom, focus)
            else:
                self.panX -= (
                    event.position().toPoint().x() - self.lastMousePos.x()
                ) / self.zoom
                self.panY -= (
                    event.position().toPoint().y() - self.lastMousePos.y()
                ) / self.zoom
            self.update()
            self.lastMousePos = event.position().toPoint()
            return

        if event.buttons() & QtCore.Qt.LeftButton:
            if self.marqueeActive:
                worldMousePos = (
                    QtCore.QPointF(event.position().toPoint()) / self.zoom
                ) + QtCore.QPointF(self.panX, self.panY)
                self.marqueeCurrent = worldMousePos
                self._updateMarqueeSelection()
                self.update()
                return

            if self.draggingNodes:
                dx = (
                    event.position().toPoint().x() - self.lastMousePos.x()
                ) / self.zoom
                dy = (
                    event.position().toPoint().y() - self.lastMousePos.y()
                ) / self.zoom

                movedNodeIds = set()
                for node in self.nodes.values():
                    if node.selected:
                        node.position = node.position + Gf.Vec2d(dx, dy)
                        movedNodeIds.add(node.id)
                        # Cheap render-offset move (text + quad ride one frame).
                        self._nodeTransformFrame.translate(node.id, dx, dy)

                # Refresh derived state for the dragged nodes (the reconcile is
                # skipped while dragging). This does not advance the reconcile
                # watermark, so the release frame reconciles these nodes once more
                # from their final positions.
                self._applyMovedNodeDerivedState(movedNodeIds)

        self.lastMousePos = event.position().toPoint()
        self.update()

    def mouseReleaseEvent(self, event):
        # Handle link drag completion
        if self._draggingLink:
            # Refresh hover state at the release position so that
            # _completeLinkDrag sees the actual cursor location.  On Windows,
            # Qt may not emit a final mouseMoveEvent before the release,
            # leaving _hoveredPort / _dragLinkValidTarget stale from a
            # previous frame — which prevents _finishReconnectDisconnect
            # from running and skips the "Disconnect" undo entry.
            releasePos = event.position().toPoint()
            self._updateHoveredPort(releasePos)
            self._updateLinkDrag(releasePos)
            self._completeLinkDrag()
            self._draggingLink = False
            self._dragLinkSyntheticTarget = None
            self._dragLinkResolvedTarget = None
            self.update()
            return

        # Always release a minimap viewport drag, even if the minimap was hidden
        # mid-drag: handleMouseRelease() is a no-op when no drag is in progress,
        # but if "Show Minimap" is toggled off after a drag starts, gating this on
        # _cachedShowMinimap would strand _viewportDragging=True and let a later
        # move hijack a pan once the minimap is shown again.
        if self.minimap.handleMouseRelease():
            return

        # End drag on all selected nodes - writes final positions to USD session layer
        if self.draggingNodes:
            self._finalizeNodeDrag()
            # Final node positions were just baked into the snapshot; rebuild the
            # port highlights once so the circles match (the drag itself kept them
            # live via the geometryChanged signal while draggingNodes was True).
            self._portHighlightsDirty = True

        self.draggingNodes = False

        if self.marqueeActive:
            self._updateMarqueeSelection()
            self.marqueeActive = False
            self._marqueeBaseSelectedNodes.clear()
            self._marqueeBaseSelectedLinks.clear()
            self._marqueePrevSelectedNodes.clear()
            self._marqueePrevSelectedLinks.clear()
            self.update()

        hasSelectedNodes = any(node.selected for node in self.nodes.values())

        if hasSelectedNodes:
            self._syncSelectionToPrimtree()
        # Do not sync empty selection back to prim tree — the user may click
        # the canvas to focus the editor and then press A to add the prim they
        # had selected in the prim tree.

    def _finalizeNodeDrag(self):
        """Finalize a node drag: write positions to USD and push undo command."""
        # Capture old positions (pre-drag) and new positions (post-drag)
        # before calling endDrag() which writes to USD.
        old_positions = {}
        new_positions = {}
        for nid, node in self.nodes.items():
            if node.selected and node._dragging:
                old_positions[nid] = (node._position[0], node._position[1])
                if node._dragPosition is not None:
                    new_positions[nid] = (
                        node._dragPosition[0],
                        node._dragPosition[1],
                    )

        # Temporarily disable notice handler to avoid recursive updates
        self._noticeHandler.setEnabled(False)
        try:
            for node in self.nodes.values():
                if node.selected:
                    node.endDrag()
        finally:
            self._noticeHandler.setEnabled(True)

        # Push undo command if positions actually changed
        Tf.Status(
            f"_finalizeNodeDrag: old_positions={len(old_positions)}, "
            f"new_positions={len(new_positions)}"
        )
        if old_positions and new_positions:
            gv = self  # prevent closure over self directly

            def _restore_positions(gv_ref, positions):
                from pxr import Gf

                for nid, pos in positions.items():
                    node = gv_ref.nodes.get(nid)
                    if node:
                        node._writePositionToUsdRaw(Gf.Vec2d(pos[0], pos[1]))
                        node._position = Gf.Vec2d(pos[0], pos[1])

            _push_undo_command(
                "Move Nodes",
                lambda p=new_positions: _restore_positions(gv, p),
                lambda p=old_positions: _restore_positions(gv, p),
            )

    def keyPressEvent(self, event):
        # Tab key - toggle hotbox
        if event.key() == QtCore.Qt.Key_Tab:
            if self.nodeCreationHotbox.is_showing:
                self.nodeCreationHotbox.hide()
            else:
                self.nodeCreationHotbox.show(self.lastMousePos)
            self.update()
            return

        # When hotbox is showing, delegate key handling to it
        if self.nodeCreationHotbox.is_showing:
            if self.nodeCreationHotbox.handle_key(event):
                self.update()
                return

        if event.key() == QtCore.Qt.Key_Space:
            self.spacePressed = True
            self.setCursor(QtCore.Qt.OpenHandCursor)
        elif event.key() == QtCore.Qt.Key_Z:
            modifiers = event.modifiers()
            if modifiers & QtCore.Qt.ControlModifier:
                if modifiers & QtCore.Qt.ShiftModifier:
                    self._performRedo()
                else:
                    self._performUndo()
                return
        elif event.key() == QtCore.Qt.Key_Y:
            if event.modifiers() & QtCore.Qt.ControlModifier:
                self._performRedo()
                return
        # 1-9 for selecting links by pin index
        # Shift+number reverses the direction (inputs instead of outputs, or vice versa)
        elif event.key() >= QtCore.Qt.Key_1 and event.key() <= QtCore.Qt.Key_9:
            pinIndex = event.key() - QtCore.Qt.Key_1  # 0-based index
            reverseDirection = bool(event.modifiers() & QtCore.Qt.ShiftModifier)
            self.selectLinkByPinIndex(pinIndex, reverseDirection)

        # Delete or Backspace: Delete all selected items at once
        elif event.key() in (QtCore.Qt.Key_Delete, QtCore.Qt.Key_Backspace):
            hasLinks = bool(self._selectedLinks)
            hasNodes = bool(self._selectedNodes)
            if hasLinks or hasNodes:
                self._deleteAllSelected()
            return

        # L: auto-layout the whole graph (plain key, no Ctrl/Alt/Meta).
        elif event.key() == QtCore.Qt.Key_L and not (
            event.modifiers()
            & (
                QtCore.Qt.ControlModifier
                | QtCore.Qt.AltModifier
                | QtCore.Qt.MetaModifier
            )
        ):
            self._autoLayoutNodes()
            event.accept()
            return

        # Fallback: dispatch to configurable shortcut QActions.
        # _shouldInterceptShortcut intercepts ShortcutOverride for all keys
        # that match a configured shortcut, which bypasses Qt's shortcut
        # system. QAction shortcuts don't fire in that case, so we manually
        # match and trigger them here.
        if hasattr(self, "_shortcutActions"):
            event_key = int(event.key()) | (
                event.modifiers().value & ~QtCore.Qt.KeypadModifier.value
            )
            for _configKey, action in self._shortcutActions.items():
                shortcut = action.shortcut()
                if shortcut.isEmpty():
                    continue
                combo = shortcut[0]
                combo_int = (
                    combo.toCombined() if hasattr(combo, "toCombined") else int(combo)
                )
                if combo_int == event_key:
                    action.trigger()
                    event.accept()
                    return

    def _performUndo(self):
        """Execute undo via the NoodlesUndoManager."""
        try:
            from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager
        except ImportError:
            return

        mgr = NoodlesUndoManager.instance()
        if not mgr.canUndo():
            Tf.Status("Undo: nothing to undo")
            return

        Tf.Status(f"Undo: executing '{mgr.undoDescription()}'")
        try:
            mgr.undo()
        except Exception as e:
            Tf.Warn(f"Undo callback failed: {e}")
        # Always reload so the visual state matches the USD layer state,
        # even when the undo callback raised an exception.
        self._reloadCurrentGraph()

    def _performRedo(self):
        """Execute redo via the NoodlesUndoManager."""
        try:
            from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager
        except ImportError:
            return

        mgr = NoodlesUndoManager.instance()
        if not mgr.canRedo():
            return

        try:
            mgr.redo()
        except Exception as e:
            Tf.Warn(f"Redo callback failed: {e}")
        # Always reload so the visual state matches the USD layer state,
        # even when the redo callback raised an exception.
        self._reloadCurrentGraph()

    def _reloadCurrentGraph(self):
        """Reload the graph from USD after undo/redo.

        Creates a fresh nodeGraph and re-runs the full load path so that
        nodes, links, spatial index, and text are rebuilt cleanly from
        the (now-modified) USD layer state.
        """
        from .nodeGraphBpContainer import NodeGraphBpContainer
        from .nodeGraphStage import NodeGraphStage

        stage = self.nodeGraph.getStage()
        if not stage:
            self.update()
            return

        # Save z-order map before reload so restored nodes keep their
        # front-to-back visual ordering (important for undo of deletes).
        savedZOrders = {nid: node.zOrder for nid, node in self.nodes.items()}

        # Disable notice handler during reload to avoid recursive processing
        self._noticeHandler.setEnabled(False)
        try:
            # Create a fresh nodeGraph of the same type
            if isinstance(self.nodeGraph, NodeGraphBpContainer):
                self.nodeGraph = NodeGraphBpContainer()
                self.nodeGraph.setStage(stage)
                self.nodeGraph.load(
                    stage,
                    self._currentPrimPath,
                    self.textRenderer.calculateTextWidth,
                    self.fontAtlas,
                    nodeFactory=self._nodeFactory,
                )
            elif isinstance(self.nodeGraph, NodeGraphStage):
                self.nodeGraph = NodeGraphStage()
                self.nodeGraph.setStage(stage)
                self.nodeGraph.load(
                    stage,
                    self.textRenderer.calculateTextWidth,
                    self.fontAtlas,
                    nodeFactory=self._nodeFactory,
                )
            else:
                self.nodeGraph = NodeGraphBlueprint()
                self.nodeGraph.setStage(stage)
                self.nodeGraph.load(
                    stage,
                    self._currentPrimPath,
                    self.textRenderer.calculateTextWidth,
                    self.fontAtlas,
                    nodeFactory=self._nodeFactory,
                )

            # Restore saved zOrder values for nodes that existed before reload.
            # New nodes (from undo restoring deleted nodes) get the next zOrder.
            # This preserves front-to-back visual ordering across undo/redo.
            maxSaved = max(savedZOrders.values()) if savedZOrders else 0
            self._nextZOrder = maxSaved
            for nid, node in self.nodes.items():
                if nid in savedZOrders:
                    node.zOrder = savedZOrders[nid]
                else:
                    self._nextZOrder += 1
                    node.zOrder = self._nextZOrder

            self.textChanged = True
            self.linksChanged = True
            self._clearRenderCache()
            self._nodeRenderManager.invalidateAll()
            # Reset position caches so text, icons, and node quads pick up new positions
            self.textRenderer.resetPositionCaches()
            self._cppIconRenderer.resetPositionCaches()
            self._nodeRenderManager.resetNodeQuadCaches()
            self._nodeTransformFrame.reset()
            self.update()
        finally:
            self._noticeHandler.setEnabled(True)

    def keyReleaseEvent(self, event):
        if event.key() == QtCore.Qt.Key_Space:
            self.spacePressed = False
            self.setCursor(QtCore.Qt.ArrowCursor)

    def _shouldInterceptShortcut(self, event):
        """Check if a ShortcutOverride event should be intercepted by the graph view.

        Prevents usdview's ApplicationShortcut-scoped actions (e.g. 'J' for
        Toggle Framed View) from firing while Noodles has focus.
        """
        # When the hotbox is showing, intercept ALL keys so they reach
        # the text input instead of triggering usdview shortcuts
        if self.nodeCreationHotbox.is_showing:
            return True

        # Tab is always intercepted (toggles the hotbox). Tab is not a
        # printable character so the check below won't catch it.
        if event.key() == QtCore.Qt.Key_Tab:
            return True

        # Intercept unmodified (or Shift-only) printable keys to prevent
        # usdview's ApplicationShortcut-scoped actions from firing while
        # Noodles has focus. Modifier combos (Ctrl+X, Alt+X) are left
        # alone so they can still reach usdview or QAction shortcuts,
        # UNLESS they match a configured Noodles shortcut.
        modifiers = event.modifiers() & ~QtCore.Qt.KeypadModifier
        if modifiers in (QtCore.Qt.NoModifier, QtCore.Qt.ShiftModifier):
            text = event.text()
            if text and text.isprintable():
                return True

        # Intercept keys that match configurable Noodles shortcuts (including
        # Ctrl/Alt combos) so they reach keyPressEvent instead of being
        # captured by usdview's ApplicationShortcut-scoped QActions.
        if hasattr(self, "_shortcutActions"):
            event_key = int(event.key()) | (
                event.modifiers().value & ~QtCore.Qt.KeypadModifier.value
            )
            for action in self._shortcutActions.values():
                shortcut = action.shortcut()
                if shortcut.isEmpty():
                    continue
                combo = shortcut[0]
                combo_int = (
                    combo.toCombined() if hasattr(combo, "toCombined") else int(combo)
                )
                if combo_int == event_key:
                    return True

        return False

    def _isStageViewMidTeardown(self):
        if not self._usdviewApi:
            return False
        try:
            controller = self._usdviewApi._UsdviewApi__appController
            sv = getattr(controller, "_stageView", None)
            return sv is not None and not sv.updatesEnabled()
        except AttributeError:
            return False

    def event(self, event):
        if event.type() == QtCore.QEvent.Paint:
            if self._isStageViewMidTeardown():
                return True
        if event.type() == QtCore.QEvent.ShortcutOverride:
            if self._shouldInterceptShortcut(event):
                event.accept()
                return True
        if event.type() == QtCore.QEvent.HoverMove:
            self.lastMousePos = event.position().toPoint()
            self._updateNodeUnderCursor(self.lastMousePos)
            if not self.hasFocus():
                self.setFocus()
            self.update()
        try:
            return super().event(event)
        except RuntimeError as e:
            Tf.Warn(f"RuntimeError in event handling: {e}")
            return False

    def _appendTriangleVertices(self, vertices, points, depth, color):
        min_x = min(point[0] for point in points)
        max_x = max(point[0] for point in points)
        min_y = min(point[1] for point in points)
        max_y = max(point[1] for point in points)
        width = max(max_x - min_x, 1.0)
        height = max(max_y - min_y, 1.0)
        r = int(color[0] * 255)
        g = int(color[1] * 255)
        b = int(color[2] * 255)
        a = int(color[3] * 255)
        for point_x, point_y in points:
            vertices.append(
                NodeVertex(
                    float(point_x),
                    float(point_y),
                    depth,
                    float(point_x - min_x),
                    float(point_y - min_y),
                    float(width),
                    float(height),
                    r,
                    g,
                    b,
                    a,
                    0.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

    def _updateNodeUnderCursor(self, pos):
        with _ProfileSection(self._profiler, "Update Node Under Cursor"):
            worldMousePos = (QtCore.QPointF(pos) / self.zoom) + QtCore.QPointF(
                self.panX, self.panY
            )
            mousePoint = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())
            self.nodeIdUnderCursor = self._getNodeIdAtWorldPos(mousePoint)

    def _getNodeIdAtWorldPos(self, worldPos, margin=0.0):
        if margin > 0.0:
            queryBounds = Gf.Range2d(
                Gf.Vec2d(worldPos[0] - margin, worldPos[1] - margin),
                Gf.Vec2d(worldPos[0] + margin, worldPos[1] + margin),
            )
            nodeIds, _ = self._spatialIndex.queryRegion(queryBounds)
        else:
            nodeIds, _ = self._spatialIndex.queryPoint(worldPos)

        sortedNodeIds = sorted(
            (nid for nid in nodeIds if nid in self.nodes),
            key=lambda nid: self.nodes[nid].zOrder,
            reverse=True,
        )

        margin_vec = Gf.Vec2d(float(margin), float(margin))
        for nodeId in sortedNodeIds:
            node = self.nodes[nodeId]
            nodeBounds = Gf.Range2d(
                node.position - margin_vec,
                node.position + node.size + margin_vec,
            )
            if nodeBounds.Contains(worldPos):
                return nodeId
        return ""

    def _updateHoveredPort(self, pos):
        """
        Update which port (if any) the cursor is hovering over.

        Args:
            pos: Screen position of cursor (QPoint)

        Returns:
            bool: True if hover state changed, False otherwise
        """
        worldMousePos = (QtCore.QPointF(pos) / self.zoom) + QtCore.QPointF(
            self.panX, self.panY
        )
        worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())

        # Hit radius = the drawn port circle (renderer.getPortWidth() ==
        # nodePortWidth / 2) scaled by the configured multiplier. One radius for
        # every port kind, matching what is drawn.
        portWidth = self.defaultRenderer.getPortWidth() if self.defaultRenderer else 8.0
        hitRadius = portWidth * self._cachedPortHitRadiusMultiplier

        # Single C++ hit-test over every node's ports (normal / folded-header /
        # dual / output-dual / synthetic relationship / collapsed aggregate),
        # spatially rejected to the cursor's neighbourhood inside portAtPoint.
        hit = self._nodeRenderManager.portAtPoint(
            self.nodeGraph,
            worldPos[0],
            worldPos[1],
            hitRadius,
        )
        if hit and hit["found"]:
            return self._setHoveredPort(
                (hit["nodeId"], hit["portName"], hit["isOutput"])
            )

        # During a relationship-link drag an attribute can also land on a whole
        # relationship / property row (a wider target than the port glyph).
        rowHover = self._relationshipDragRowHover(worldPos, portWidth * 2.0)
        if rowHover is not None:
            return self._setHoveredPort(rowHover)

        # No port found - clear hover if needed
        if self._hoveredPort is not None:
            self._hoveredPort = None
            return True

        return False  # No change

    def _relationshipDragRowHover(self, worldPos, portExtent):
        """Whole-row relationship/property drop target during a relationship drag.

        Returns ``(nodeId, pinName, isOutput)`` for the relationship or property
        row under ``worldPos``, or ``None``. Only active while dragging a
        relationship link; the port glyphs themselves are hit by the C++
        ``portAtPoint`` pass in :meth:`_updateHoveredPort`.
        """
        _temp_link = getattr(self, "_dragLinkTempLink", None)
        if not self._draggingLink or not getattr(
            _temp_link, "is_relationship_link", False
        ):
            return None

        candidateNodes = []
        if self.nodeIdUnderCursor and self.nodeIdUnderCursor in self.nodes:
            candidateNodes.append(self.nodeIdUnderCursor)
        queryBounds = Gf.Range2d(
            Gf.Vec2d(worldPos[0] - portExtent, worldPos[1] - portExtent),
            Gf.Vec2d(worldPos[0] + portExtent, worldPos[1] + portExtent),
        )
        nearbyNodeIds, _ = self._spatialIndex.queryRegion(queryBounds)
        for nodeId in nearbyNodeIds:
            if nodeId not in candidateNodes:
                candidateNodes.append(nodeId)

        for nodeId in candidateNodes:
            node = self.nodes.get(nodeId)
            if node is None or not node.renderer:
                continue
            rowHover = self._findRelationshipRowHover(node, nodeId, worldPos)
            if rowHover is None:
                rowHover = self._findPropertyRowHover(node, nodeId, worldPos)
            if rowHover is not None:
                return rowHover
        return None

    def _setHoveredPort(self, newHover):
        """Update _hoveredPort, returning True iff it actually changed."""
        if self._hoveredPort != newHover:
            self._hoveredPort = newHover
            return True
        return False

    def _getRowHitMetrics(self, node, renderer):
        # Read from the cached layout (single source of truth) so hover row bands
        # match the rendered rows exactly. port_start_y is the band-area top (world);
        # row_slot * port_line_height gives each band top, [top, top+line_height].
        port_line_height = node.layoutPortLineHeight
        if port_line_height <= 0.0:
            # Layout not computed yet — degrade without crashing.
            port_line_height = max(renderer.getPortWidth() * 2.0, 16.0)
        return node.position[1] + node.layoutPortStartY, port_line_height

    def _findRelationshipRowHover(self, node, nodeId, worldPos):
        _temp_link = getattr(self, "_dragLinkTempLink", None)
        if not self._draggingLink or not getattr(
            _temp_link, "is_relationship_link", False
        ):
            return None
        if getattr(node, "_title_collapsed", False):
            return None
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        if (
            worldPos[0] < node.position[0]
            or worldPos[0] > node.position[0] + node.size[0]
        ):
            return None

        is_relationship_pin_for_node = getattr(
            node,
            "is_relationship_pin",
            lambda _pin_name, _is_output: False,
        )
        output_row_kinds = list(node.outputRowKinds)
        output_row_slots = list(node.outputRowSlots)
        port_start_y, port_line_height = self._getRowHitMetrics(node, renderer)

        closest_hover = None
        closest_distance = None
        for i, pinName in enumerate(getattr(node, "outputPins", [])):
            if not is_relationship_pin_for_node(pinName, True):
                continue
            if i < len(output_row_kinds) and output_row_kinds[i] == 2:
                continue
            row_slot = output_row_slots[i] if i < len(output_row_slots) else i
            row_y = port_start_y + row_slot * port_line_height
            row_y1 = row_y + port_line_height
            if worldPos[1] < row_y or worldPos[1] > row_y1:
                continue
            port_pos = self._getPortPosition(node, pinName, True)
            row_center_y = (
                float(port_pos[1])
                if port_pos is not None
                else row_y + port_line_height * 0.5
            )
            distance = abs(float(worldPos[1]) - row_center_y)
            if closest_distance is None or distance < closest_distance:
                closest_distance = distance
                closest_hover = (nodeId, pinName, True)
        return closest_hover

    def _findPropertyRowHover(self, node, nodeId, worldPos):
        _temp_link = getattr(self, "_dragLinkTempLink", None)
        if not self._draggingLink or not getattr(
            _temp_link, "is_relationship_link", False
        ):
            return None
        if getattr(node, "_title_collapsed", False):
            return None
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        if (
            worldPos[0] < node.position[0]
            or worldPos[0] > node.position[0] + node.size[0]
        ):
            return None

        is_relationship_pin_for_node = getattr(
            node,
            "is_relationship_pin",
            lambda _pin_name, _is_output: False,
        )
        port_start_y, port_line_height = self._getRowHitMetrics(node, renderer)
        center_x = node.position[0] + node.size[0] * 0.5
        prefer_output = worldPos[0] >= center_x
        output_spec = (
            True,
            getattr(node, "outputPins", []),
            list(node.outputRowKinds),
            list(node.outputRowSlots),
        )
        input_spec = (
            False,
            getattr(node, "inputPins", []),
            list(node.inputRowKinds),
            list(node.inputRowSlots),
        )
        side_specs = (
            [output_spec, input_spec] if prefer_output else [input_spec, output_spec]
        )

        closest_hover = None
        closest_distance = None
        for is_output, pins, row_kinds, row_slots in side_specs:
            for i, pin_name in enumerate(pins):
                if is_relationship_pin_for_node(pin_name, is_output):
                    continue

                row_kind = row_kinds[i] if i < len(row_kinds) else 0
                if row_kind == 1 or row_kind == 2:
                    continue

                row_slot = row_slots[i] if i < len(row_slots) else i
                row_y = port_start_y + row_slot * port_line_height
                row_y1 = row_y + port_line_height
                if worldPos[1] < row_y or worldPos[1] > row_y1:
                    continue

                port_pos = self._getPortPosition(node, pin_name, is_output)
                row_center_y = (
                    float(port_pos[1])
                    if port_pos is not None
                    else row_y + port_line_height * 0.5
                )
                # Prefer the side closest to the cursor half when both sides
                # share the same visible row slot.
                side_bias = 0.0 if (is_output == prefer_output) else port_line_height
                distance = abs(float(worldPos[1]) - row_center_y) + side_bias
                if closest_distance is None or distance < closest_distance:
                    closest_distance = distance
                    closest_hover = (nodeId, pin_name, is_output)
        return closest_hover

    def _rowEdgePinCandidates(self, node, is_output):
        """(pin_name, row_slot) pairs that own a port on the ``is_output`` side.

        Output candidates include real output pins plus dual pins (whose right
        edge mirrors an input row); input candidates likewise include real input
        pins plus output-dual pins (whose left edge mirrors an output row). So a
        press on either edge of any dual row resolves to the right pin regardless
        of which list backs it.
        """
        candidates = []
        if is_output:
            out_pins = getattr(node, "outputPins", [])
            out_slots = list(node.outputRowSlots)
            out_kinds = list(node.outputRowKinds)
            for i, pin in enumerate(out_pins):
                if i < len(out_kinds) and out_kinds[i] == 2:
                    continue
                candidates.append((pin, out_slots[i] if i < len(out_slots) else i))
            out_set = set(out_pins)
            dual = getattr(node, "_dual_pin_names", set())
            in_pins = getattr(node, "inputPins", [])
            in_slots = list(node.inputRowSlots)
            in_kinds = list(node.inputRowKinds)
            for i, pin in enumerate(in_pins):
                if pin not in dual or pin in out_set:
                    continue
                if i < len(in_kinds) and in_kinds[i] == 2:
                    continue
                candidates.append((pin, in_slots[i] if i < len(in_slots) else i))
        else:
            in_pins = getattr(node, "inputPins", [])
            in_slots = list(node.inputRowSlots)
            in_kinds = list(node.inputRowKinds)
            for i, pin in enumerate(in_pins):
                if i < len(in_kinds) and in_kinds[i] == 2:
                    continue
                candidates.append((pin, in_slots[i] if i < len(in_slots) else i))
            in_set = set(in_pins)
            out_dual = getattr(node, "_output_dual_pin_names", set())
            out_pins = getattr(node, "outputPins", [])
            out_slots = list(node.outputRowSlots)
            out_kinds = list(node.outputRowKinds)
            for i, pin in enumerate(out_pins):
                if pin not in out_dual or pin in in_set:
                    continue
                if i < len(out_kinds) and out_kinds[i] == 2:
                    continue
                candidates.append((pin, out_slots[i] if i < len(out_slots) else i))
        return candidates

    def _pinAtRowEdge(self, node, worldPos, is_output):
        """Pin whose row band contains ``worldPos[1]`` on the ``is_output`` side."""
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        port_start_y, port_line_height = self._getRowHitMetrics(node, renderer)
        best = None
        best_distance = None
        for pin_name, row_slot in self._rowEdgePinCandidates(node, is_output):
            row_y = port_start_y + row_slot * port_line_height
            if worldPos[1] < row_y or worldPos[1] > row_y + port_line_height:
                continue
            port_pos = self._getPortPosition(node, pin_name, is_output)
            row_center_y = (
                float(port_pos[1])
                if port_pos is not None
                else row_y + port_line_height * 0.5
            )
            distance = abs(float(worldPos[1]) - row_center_y)
            if best_distance is None or distance < best_distance:
                best_distance = distance
                best = pin_name
        return best

    def _findRowEdgeDragStart(self, node, worldPos):
        """Resolve a press near a node's left/right edge to ``(pin, is_output)``
        for starting a new connection.

        Only the outer tenth of the node width on each side starts a connection:
        a press in the left band starts an input-side drag, the right band an
        output-side drag. A press in the middle of the row, on the title (above
        the first row), or below the last row falls through to a node-body drag
        (moving the node). A row is grabbable from its side even when no pin
        glyph is drawn (REQ 3 hides it until the pin is connected or dragged) -
        aim near the node edge rather than the inward-offset row label.
        """
        if getattr(node, "_title_collapsed", False):
            return None
        renderer = node.renderer if node.renderer else self.defaultRenderer
        if not renderer:
            return None
        nx = float(node.position[0])
        nw = float(node.size[0])
        if worldPos[0] < nx or worldPos[0] > nx + nw:
            return None
        # Only the outer tenth of each side starts a connection; the middle 80%
        # stays a node-body drag handle for moving the node.
        edge_band = nw * 0.1
        if worldPos[0] >= nx + nw - edge_band:
            is_output = True
        elif worldPos[0] <= nx + edge_band:
            is_output = False
        else:
            return None
        pin_name = self._pinAtRowEdge(node, worldPos, is_output)
        if pin_name is None:
            return None
        return (pin_name, is_output)

    def _startLinkReconnect(self, linkIndex, worldClickPos):
        """Disconnect a link from its nearest end and start dragging to reconnect.

        The link is removed visually (local graph only) while the user drags.
        USD is not modified until the drag completes, so cancelling restores
        the original connection and a successful reconnection is a single undo.
        """
        link = self.links[linkIndex]

        is_relationship_link = bool(getattr(link, "is_relationship_link", False))

        if is_relationship_link:
            # Relationship links always anchor on the source (the relationship
            # pin) because the target is a whole-node prim target, not a pin.
            anchorNodeId = link.sourceNodeId
            anchorPort = link.sourcePort
            anchorIsOutput = True
        else:
            startPos = link.start
            endPos = link.end
            startDistSq = (worldClickPos[0] - startPos[0]) ** 2 + (
                worldClickPos[1] - startPos[1]
            ) ** 2
            endDistSq = (worldClickPos[0] - endPos[0]) ** 2 + (
                worldClickPos[1] - endPos[1]
            ) ** 2

            if startDistSq < endDistSq:
                anchorNodeId = link.targetNodeId
                anchorPort = link.targetPort
                anchorIsOutput = False
            else:
                anchorNodeId = link.sourceNodeId
                anchorPort = link.sourcePort
                anchorIsOutput = True

        self._reconnectingLink = True
        self._reconnectOldSourceNodeId = link.sourceNodeId
        self._reconnectOldSourcePort = link.sourcePort
        self._reconnectOldTargetNodeId = link.targetNodeId
        self._reconnectOldTargetPort = link.targetPort
        self._reconnectOldSourcePropertyName = str(
            getattr(link, "sourcePropertyName", "")
        )
        self._reconnectOldTargetPropertyName = str(
            getattr(link, "targetPropertyName", "")
        )

        self._removeLinkFromGraph(
            link.sourceNodeId,
            link.sourcePort,
            link.targetNodeId,
            link.targetPort,
            sourcePropertyName=str(getattr(link, "sourcePropertyName", "")),
            targetPropertyName=str(getattr(link, "targetPropertyName", "")),
        )

        if anchorNodeId not in self.nodes:
            self._cancelLinkReconnect()
            return

        self._startLinkDrag(anchorNodeId, anchorPort, anchorIsOutput)

        if anchorIsOutput:
            self._dragLinkTempLink.end = worldClickPos
        else:
            self._dragLinkTempLink.start = worldClickPos

    def _clearReconnectState(self):
        """Zero reconnect bookkeeping fields."""
        self._reconnectingLink = False
        self._reconnectOldSourceNodeId = None
        self._reconnectOldSourcePort = None
        self._reconnectOldTargetNodeId = None
        self._reconnectOldTargetPort = None
        self._reconnectOldSourcePropertyName = ""
        self._reconnectOldTargetPropertyName = ""

    def _cancelLinkReconnect(self):
        """Restore the original link when reconnection fails (error recovery)."""
        srcId = self._reconnectOldSourceNodeId
        srcPort = self._reconnectOldSourcePort
        tgtId = self._reconnectOldTargetNodeId
        tgtPort = self._reconnectOldTargetPort
        srcPropName = self._reconnectOldSourcePropertyName
        tgtPropName = self._reconnectOldTargetPropertyName
        self._clearReconnectState()

        srcNode = self.nodes.get(srcId)
        tgtNode = self.nodes.get(tgtId)
        if srcNode and tgtNode:
            self._addLinkToGraph(
                srcId,
                srcPort,
                tgtId,
                tgtPort,
                srcNode,
                tgtNode,
                sourcePropertyName=srcPropName,
                targetPropertyName=tgtPropName,
            )

    def _finishReconnectDisconnect(self):
        """Delete the original connection when dropped on empty space."""
        srcId = self._reconnectOldSourceNodeId
        srcPort = self._reconnectOldSourcePort
        tgtId = self._reconnectOldTargetNodeId
        tgtPort = self._reconnectOldTargetPort

        inputNode = self.nodes.get(tgtId)
        outputNode = self.nodes.get(srcId)
        if not inputNode or not outputNode:
            self._cancelLinkReconnect()
            return

        inputPrim = inputNode.getUsdPrim()
        outputPrim = outputNode.getUsdPrim()
        if not inputPrim or not outputPrim:
            self._cancelLinkReconnect()
            return

        library = self._findLibraryForPrim(inputPrim)
        if not library:
            self._cancelLinkReconnect()
            return

        self._clearReconnectState()

        stage = self.nodeGraph.getStage()
        warn_if_non_persistent_edit_target(stage)

        self._noticeHandler.setEnabled(False)
        try:
            success = library.delete_connection(
                stage, outputPrim, srcPort, inputPrim, tgtPort
            )
            if success:
                gv = self
                delete_fn = _make_connection_edit(
                    gv, srcId, srcPort, tgtId, tgtPort, create=False
                )
                create_fn = _make_connection_edit(
                    gv, srcId, srcPort, tgtId, tgtPort, create=True
                )

                def redo():
                    delete_fn()
                    gv.linksChanged = True
                    gv.update()

                def undo():
                    create_fn()
                    gv.linksChanged = True
                    gv.update()

                _push_undo_command("Disconnect", redo, undo)

                # Re-derive links from USD so the disconnected pin's port
                # highlight (the relationship arrow / port circle) reflects the
                # now-empty target. GetTargets is the source of truth; the
                # in-place removal done at reconnect-start is not guaranteed to
                # match relationship links (their propertyName fields can differ
                # from the record), which would otherwise leave a stale link in
                # self.links keeping the port "connected" (arrow stuck yellow).
                # Mirrors the not-found branch below and _deleteConnection.
                self.linksChanged = True
                self._rebuildLinks()
                self.update()
            else:
                # Connection not found in USD — it may have been replaced by
                # another connection to the same pin. Rebuild links from USD
                # so the UI reflects the actual stage state.
                self.linksChanged = True
                self._rebuildLinks()
        finally:
            self._noticeHandler.setEnabled(True)

    def _getPropertyAtPoint(self, screenPos):
        """Return (nodeId, pinName) for the property row under screenPos, or None."""
        worldMousePos = (QtCore.QPointF(screenPos) / self.zoom) + QtCore.QPointF(
            self.panX, self.panY
        )
        worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())

        renderer = self.defaultRenderer
        if not renderer:
            return None

        # Iterate topmost-first so overlapping nodes return the visible one.
        sortedNodes = sorted(
            self.nodes.items(),
            key=lambda kv: getattr(kv[1], "zOrder", 0),
            reverse=True,
        )
        for nodeId, node in sortedNodes:
            if node.is_title_collapsed:
                continue
            if (
                worldPos[0] < node.position[0]
                or worldPos[0] > node.position[0] + node.size[0]
            ):
                continue

            # Each pin's band is its cached center +/- half the row height — the
            # same per-pin layout the renderer uses, so clicks match what's drawn.
            half = node.layoutPortLineHeight * 0.5
            inputCenters = node.layoutInputCenterY
            for i, pinName in enumerate(node.inputPins):
                if i >= len(inputCenters):
                    break
                center = node.position[1] + inputCenters[i]
                if center - half <= worldPos[1] <= center + half:
                    return (nodeId, pinName)

            outputCenters = node.layoutOutputCenterY
            for i, pinName in enumerate(node.outputPins):
                if i >= len(outputCenters):
                    break
                center = node.position[1] + outputCenters[i]
                if center - half <= worldPos[1] <= center + half:
                    return (nodeId, pinName)

        return None

    def _updatePropertyTooltip(self, screenPos):
        """Update tooltip state based on which property row the cursor is over."""
        prop = self._getPropertyAtPoint(screenPos)
        if prop != self._tooltipProperty:
            self._tooltipTimer.stop()
            QtWidgets.QToolTip.hideText()
            self._tooltipProperty = prop
            if prop is not None:
                self._tooltipTimer.start()
        if prop is not None:
            # Track the latest cursor position so the eventual tooltip appears
            # where the user is actually pointing, not where they entered the row.
            self._tooltipScreenPos = self.mapToGlobal(screenPos)

    def _showPropertyTooltip(self):
        """Show the tooltip for the currently hovered property."""
        if self._tooltipProperty is None or self._tooltipScreenPos is None:
            return
        nodeId, pinName = self._tooltipProperty
        node = self.nodes.get(nodeId)
        if node is None:
            return
        doc = ""
        if hasattr(node, "get_pin_documentation"):
            doc = node.get_pin_documentation(pinName)
        if not doc:
            return
        QtWidgets.QToolTip.showText(self._tooltipScreenPos, doc, self)

    def _startLinkDrag(self, nodeId, portName, isOutput):
        """
        Begin dragging a new link from a port.

        Args:
            nodeId: ID of the source node
            portName: Name of the source port
            isOutput: True if dragging from output port, False for input port
        """
        self._draggingLink = True
        self._dragLinkSourceNode = nodeId
        self._dragLinkSourcePort = portName
        self._dragLinkSourceIsOutput = isOutput
        self._dragLinkSyntheticTarget = None
        self._dragLinkResolvedTarget = None
        _start_node = self.nodes.get(self._dragLinkSourceNode)
        self._dragLinkTempLink.is_relationship_link = (
            _start_node is not None
            and _start_node.is_relationship_pin(
                str(portName), self._dragLinkSourceIsOutput
            )
        )
        self._dragLinkTempLink.sourcePropertyName = ""
        self._dragLinkTempLink.targetPropertyName = ""

        # Initialize temporary link
        node = self.nodes[nodeId]
        portPos = self._getPortPosition(node, portName, isOutput)

        # Link renderer expects start=output, end=input for correct bezier direction.
        # When dragging from an input port, the fixed port is the "end" (input side)
        # and the cursor will be the "start" (output side).
        if isOutput:
            self._dragLinkTempLink.sourceNodeId = nodeId
            self._dragLinkTempLink.sourcePort = portName
            self._dragLinkTempLink.targetNodeId = ""
            self._dragLinkTempLink.targetPort = ""
            if self._dragLinkTempLink.is_relationship_link:
                self._dragLinkTempLink.sourcePropertyName = str(portName)
            self._dragLinkTempLink.start = portPos
            self._dragLinkTempLink.end = portPos
        else:
            self._dragLinkTempLink.sourceNodeId = ""
            self._dragLinkTempLink.sourcePort = ""
            self._dragLinkTempLink.targetNodeId = nodeId
            self._dragLinkTempLink.targetPort = portName
            if self._dragLinkTempLink.is_relationship_link:
                self._dragLinkTempLink.targetPropertyName = str(portName)
            self._dragLinkTempLink.end = portPos
            self._dragLinkTempLink.start = portPos

        self._dragLinkValidTarget = False

        # When dragging from a connected input port, lift the existing
        # connection so releasing on empty or an invalid target deletes it
        # (matches Presto "drag to empty to delete" parity: T262819088).
        if not isOutput and not self._reconnectingLink:
            existingLink = self._findLinkToInput(nodeId, portName)
            if existingLink and not getattr(existingLink, "isDangling", False):
                self._reconnectingLink = True
                self._reconnectOldSourceNodeId = existingLink.sourceNodeId
                self._reconnectOldSourcePort = existingLink.sourcePort
                self._reconnectOldTargetNodeId = existingLink.targetNodeId
                self._reconnectOldTargetPort = existingLink.targetPort
                self._reconnectOldSourcePropertyName = str(
                    getattr(existingLink, "sourcePropertyName", "")
                )
                self._reconnectOldTargetPropertyName = str(
                    getattr(existingLink, "targetPropertyName", "")
                )
                self._removeLinkFromGraph(
                    existingLink.sourceNodeId,
                    existingLink.sourcePort,
                    existingLink.targetNodeId,
                    existingLink.targetPort,
                    sourcePropertyName=str(
                        getattr(existingLink, "sourcePropertyName", "")
                    ),
                    targetPropertyName=str(
                        getattr(existingLink, "targetPropertyName", "")
                    ),
                )

    def _snapLinkDragToPort(self, nodeId, portName, isOutput, relationship_hover):
        """Snap the temporary drag link to a hovered port and record the resolved target."""
        targetNode = self.nodes[nodeId]
        if relationship_hover:
            # Attribute → relationship pin: store a resolved target so
            # _completeLinkDrag uses it directly instead of falling back
            # to _hoveredPort, which would apply the output-swap logic
            # and incorrectly invert source/target routing.
            targetPropertyName = self._resolvePortPropertyName(
                targetNode, portName, isOutput
            )
            self._dragLinkResolvedTarget = (
                nodeId,
                portName,
                isOutput,
                targetPropertyName,
            )
            portPos = self._getPortPosition(targetNode, portName, isOutput)
            self._dragLinkTempLink.targetNodeId = nodeId
            self._dragLinkTempLink.targetPort = portName
            self._dragLinkTempLink.end = portPos
        else:
            portPos = self._getPortPosition(targetNode, portName, isOutput)
            if isOutput:
                self._dragLinkTempLink.sourceNodeId = nodeId
                self._dragLinkTempLink.sourcePort = portName
                self._dragLinkTempLink.start = portPos
            else:
                self._dragLinkTempLink.targetNodeId = nodeId
                self._dragLinkTempLink.targetPort = portName
                self._dragLinkTempLink.end = portPos

    def _updateLinkDrag(self, screenPos):
        """
        Update the temporary link being dragged.

        Args:
            screenPos: Screen position of cursor (QPoint)
        """
        worldMousePos = (QtCore.QPointF(screenPos) / self.zoom) + QtCore.QPointF(
            self.panX, self.panY
        )
        worldPos = Gf.Vec2d(worldMousePos.x(), worldMousePos.y())
        self._dragLinkCursorPos = worldPos

        # Link renderer expects start=output, end=input for correct bezier direction.
        # When dragging from an output port, cursor moves the "end" (input side).
        # When dragging from an input port, cursor moves the "start" (output side).
        if self._dragLinkSourceIsOutput:
            self._dragLinkTempLink.targetNodeId = ""
            self._dragLinkTempLink.targetPort = ""
            self._dragLinkTempLink.targetPropertyName = ""
            self._dragLinkTempLink.end = worldPos
        else:
            self._dragLinkTempLink.sourceNodeId = ""
            self._dragLinkTempLink.sourcePort = ""
            self._dragLinkTempLink.sourcePropertyName = ""
            self._dragLinkTempLink.start = worldPos

        # Check if hovering over a valid target port
        self._dragLinkValidTarget = False
        self._dragLinkSyntheticTarget = None
        self._dragLinkResolvedTarget = None

        if self._hoveredPort:
            nodeId, portName, isOutput = self._hoveredPort

            # Validate: output can only connect to input (and vice versa)
            relationship_drag = getattr(
                self._dragLinkTempLink, "is_relationship_link", False
            )
            _hover_node = self.nodes.get(nodeId)
            relationship_hover = _hover_node is not None and getattr(
                _hover_node, "is_relationship_pin", lambda *_: False
            )(str(portName), isOutput)
            if relationship_drag:
                if nodeId in self.nodes and not (
                    nodeId == self._dragLinkSourceNode
                    and portName == self._dragLinkSourcePort
                    and isOutput == self._dragLinkSourceIsOutput
                ):
                    targetNode = self.nodes[nodeId]
                    targetPropertyName = self._resolvePortPropertyName(
                        targetNode,
                        portName,
                        isOutput,
                    )
                    linkTargetIsOutput = not bool(self._dragLinkSourceIsOutput)
                    self._dragLinkResolvedTarget = (
                        nodeId,
                        portName,
                        linkTargetIsOutput,
                        targetPropertyName,
                    )
                    self._dragLinkValidTarget = True
                    self._dragLinkTempLink.is_relationship_link = True
                    portPos = self._getLinkEndpointPosition(
                        targetNode,
                        portName,
                        linkTargetIsOutput,
                        targetPropertyName,
                        preferPropertySide=relationship_drag,
                    )
                    if linkTargetIsOutput:
                        self._dragLinkTempLink.sourceNodeId = nodeId
                        self._dragLinkTempLink.sourcePort = portName
                        self._dragLinkTempLink.sourcePropertyName = targetPropertyName
                        self._dragLinkTempLink.start = portPos
                    else:
                        self._dragLinkTempLink.targetNodeId = nodeId
                        self._dragLinkTempLink.targetPort = portName
                        self._dragLinkTempLink.targetPropertyName = targetPropertyName
                        self._dragLinkTempLink.end = portPos
                    return
            elif (
                isOutput != self._dragLinkSourceIsOutput or relationship_hover
            ) and not (
                nodeId == self._dragLinkSourceNode
                and portName == self._dragLinkSourcePort
            ):
                # Valid direction: opposite port types, or attribute dragged onto a
                # relationship pin (USD relationships are directionless so the
                # output/input distinction is relaxed for relationship drop targets).
                self._dragLinkValidTarget = True
                self._snapLinkDragToPort(nodeId, portName, isOutput, relationship_hover)
                return

        if getattr(self._dragLinkTempLink, "is_relationship_link", False):
            targetNodeId = self.nodeIdUnderCursor
            if not targetNodeId:
                hover_margin = max(2.0, 4.0 / max(float(self.zoom), 1e-6))
                targetNodeId = self._getNodeIdAtWorldPos(worldPos, margin=hover_margin)
            if targetNodeId and targetNodeId in self.nodes:
                targetNode = self.nodes[targetNodeId]
                # Restrict the prim-target drop zone to the title bar area at the top of
                # the node. Without this, dropping on a property row (especially when the
                # relationship pin is itself the first property) creates a prim-target
                # connection — which surprises users who meant to drop on the property's
                # port or to cancel the drag. Self-prim-targets remain allowed as long
                # as the drop lands inside the title bar.
                renderer = getattr(targetNode, "renderer", None) or getattr(
                    self, "defaultRenderer", None
                )
                if renderer is not None:
                    get_metrics = getattr(self, "_getRowHitMetrics", None)
                    if get_metrics is not None:
                        port_start_y, _ = get_metrics(targetNode, renderer)
                        if worldPos[1] >= port_start_y:
                            return
                targetIsOutput = not bool(self._dragLinkSourceIsOutput)
                portPos = self._getNodeTopCenterPosition(
                    targetNode,
                )
                self._dragLinkSyntheticTarget = (
                    targetNodeId,
                    "",
                    targetIsOutput,
                    "",
                )
                self._dragLinkValidTarget = True
                self._dragLinkTempLink.is_relationship_link = True
                if targetIsOutput:
                    self._dragLinkTempLink.sourceNodeId = targetNodeId
                    self._dragLinkTempLink.sourcePort = ""
                    self._dragLinkTempLink.sourcePropertyName = ""
                    self._dragLinkTempLink.start = portPos
                else:
                    self._dragLinkTempLink.targetNodeId = targetNodeId
                    self._dragLinkTempLink.targetPort = ""
                    self._dragLinkTempLink.targetPropertyName = ""
                    self._dragLinkTempLink.end = portPos

    def _completeLinkDrag(self):
        """
        Complete link drag - create connection if valid target.

        This is called when the mouse is released during a link drag.
        If a valid target port is hovered, creates the connection.
        If the target is a collapsed node with multiple pins, shows a
        context menu to let the user choose which pin to connect to.
        Otherwise (released on empty space), cancels the drag.
        """
        targetData = self._dragLinkResolvedTarget or self._dragLinkSyntheticTarget
        _temp_link = getattr(self, "_dragLinkTempLink", None)
        relationship_drag = getattr(_temp_link, "is_relationship_link", False)
        if targetData is None and self._hoveredPort and not relationship_drag:
            hoveredNodeId, hoveredPortName, hoveredIsOutput = self._hoveredPort
            targetData = (hoveredNodeId, hoveredPortName, hoveredIsOutput, "")
        if not self._dragLinkValidTarget or not targetData:
            # Cancelled - released on empty space or invalid target
            self._dragLinkSyntheticTarget = None
            self._dragLinkResolvedTarget = None
            if self._reconnectingLink:
                self._finishReconnectDisconnect()
            return

        targetNodeId, targetPortName, targetIsOutput, targetPropertyName = targetData
        usingSyntheticTarget = targetData == self._dragLinkSyntheticTarget

        # Check if target node is title-collapsed with multiple pins on this side
        if (
            not relationship_drag
            and not usingSyntheticTarget
            and targetNodeId in self.nodes
        ):
            targetNode = self.nodes[targetNodeId]
            if getattr(targetNode, "_title_collapsed", False):
                # Get the original pins for this side
                if targetIsOutput:
                    originalPins = list(
                        getattr(targetNode, "_original_output_pins", None)
                        or targetNode.outputPins
                    )
                else:
                    originalPins = list(
                        getattr(targetNode, "_original_input_pins", None)
                        or targetNode.inputPins
                    )
                if len(originalPins) > 1:
                    # Show context menu to choose pin
                    self._showPinSelectionMenu(
                        targetNodeId,
                        originalPins,
                        targetIsOutput,
                    )
                    return
                elif len(originalPins) == 1:
                    # Only one pin, use it directly
                    targetPortName = originalPins[0]

        # Normalize so we know which is input and which is output
        target_node_model = self.nodes.get(targetNodeId)
        target_is_relationship = (
            target_node_model is not None
            and target_node_model.is_relationship_pin(
                str(targetPortName), targetIsOutput
            )
        )
        if relationship_drag:
            if self._dragLinkSourceIsOutput:
                outputNodeId, outputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
                inputNodeId, inputPortName = targetNodeId, targetPortName
                outputPropertyName = str(self._dragLinkSourcePort)
                inputPropertyName = targetPropertyName
            else:
                outputNodeId, outputPortName = targetNodeId, targetPortName
                inputNodeId, inputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
                outputPropertyName = targetPropertyName
                inputPropertyName = str(self._dragLinkSourcePort)
        elif target_is_relationship:
            # Attribute → relationship pin: preserve the drag-source's original
            # direction so the relationship-owner ends up on the opposite endpoint
            # and create_connection can find it via GetRelationship(target_port).
            if self._dragLinkSourceIsOutput:
                outputNodeId, outputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
                inputNodeId, inputPortName = targetNodeId, targetPortName
            else:
                outputNodeId, outputPortName = targetNodeId, targetPortName
                inputNodeId, inputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
            outputPropertyName = ""
            inputPropertyName = ""
        elif targetIsOutput:
            # Target is output, source must be input - swap them
            outputNodeId, outputPortName = targetNodeId, targetPortName
            inputNodeId, inputPortName = (
                self._dragLinkSourceNode,
                self._dragLinkSourcePort,
            )
            outputPropertyName = ""
            inputPropertyName = ""
        else:
            # Target is input, source must be output
            inputNodeId, inputPortName = targetNodeId, targetPortName
            outputNodeId, outputPortName = (
                self._dragLinkSourceNode,
                self._dragLinkSourcePort,
            )
            outputPropertyName = ""
            inputPropertyName = ""

        if self._reconnectingLink and (
            outputNodeId == self._reconnectOldSourceNodeId
            and outputPortName == self._reconnectOldSourcePort
            and inputNodeId == self._reconnectOldTargetNodeId
            and inputPortName == self._reconnectOldTargetPort
        ):
            self._cancelLinkReconnect()
            return

        # Reject exact duplicate connections
        if self._hasExactLink(
            outputNodeId,
            outputPortName,
            inputNodeId,
            inputPortName,
            outputPropertyName,
            inputPropertyName,
        ):
            if self._reconnectingLink:
                self._cancelLinkReconnect()
            return

        # Check if target input already has a connection
        oldLink = (
            None
            if relationship_drag or target_is_relationship
            else self._findLinkToInput(inputNodeId, inputPortName)
        )
        if oldLink:
            # Highlight old link in red for brief moment
            oldLink.highlighted = True
            oldLink.highlightColor = (1.0, 0.0, 0.0, 1.0)
            self._linksNeedSync = True
            self.update()

            # Snapshot and consume reconnect state now so a new reconnect
            # started during the highlight delay cannot overwrite it.
            reconnect_state = None
            if self._reconnectingLink:
                reconnect_state = (
                    self._reconnectOldSourceNodeId,
                    self._reconnectOldSourcePort,
                    self._reconnectOldTargetNodeId,
                    self._reconnectOldTargetPort,
                )
                self._clearReconnectState()

            # Use QTimer to remove highlight after 200ms, then create new connection
            from PySide6 import QtCore

            QtCore.QTimer.singleShot(
                200,
                lambda: self._finishConnectionReplacement(
                    oldLink,
                    outputNodeId,
                    outputPortName,
                    inputNodeId,
                    inputPortName,
                    reconnect_state,
                    outputPropertyName,
                    inputPropertyName,
                ),
            )
        else:
            # No existing connection, create directly
            self._createConnection(
                outputNodeId,
                outputPortName,
                inputNodeId,
                inputPortName,
                sourcePropertyName=outputPropertyName,
                targetPropertyName=inputPropertyName,
            )
        self._dragLinkSyntheticTarget = None
        self._dragLinkResolvedTarget = None

    def _finishConnectionReplacement(
        self,
        oldLink,
        outputNodeId,
        outputPortName,
        inputNodeId,
        inputPortName,
        reconnect_state=None,
        outputPropertyName="",
        inputPropertyName="",
    ):
        """
        Finish replacing an old connection after highlighting it.

        Args:
            oldLink: The old link to remove
            outputNodeId: ID of the new output node
            outputPortName: Name of the new output port
            inputNodeId: ID of the new input node
            inputPortName: Name of the new input port
            reconnect_state: Captured reconnect endpoints, if this was a
                reconnect drag.  Passed through to _createConnection so
                the deferred call is immune to instance-state overwrites.
        """
        oldLink.highlighted = False
        oldLink.highlightColor = None
        self._linksNeedSync = True

        self._createConnection(
            outputNodeId,
            outputPortName,
            inputNodeId,
            inputPortName,
            reconnect_state,
            sourcePropertyName=outputPropertyName,
            targetPropertyName=inputPropertyName,
        )

        # Rebuild links from USD so the UI reflects whether the old
        # connection was replaced or both coexist (multi-connection pins).
        self.linksChanged = True
        self._rebuildLinks()

    def _showPinSelectionMenu(self, targetNodeId, pins, targetIsOutput):
        """Show a context menu to select which pin to connect to on a collapsed node."""
        menu = QtWidgets.QMenu(self)
        menu.setTitle("Select Pin")
        for pin in pins:
            action = menu.addAction(pin)
            action.setData(pin)

        def onPinSelected(action):
            selectedPin = action.data()
            if targetIsOutput:
                outputNodeId, outputPortName = targetNodeId, selectedPin
                inputNodeId, inputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
            else:
                inputNodeId, inputPortName = targetNodeId, selectedPin
                outputNodeId, outputPortName = (
                    self._dragLinkSourceNode,
                    self._dragLinkSourcePort,
                )
            if self._reconnectingLink and (
                outputNodeId == self._reconnectOldSourceNodeId
                and outputPortName == self._reconnectOldSourcePort
                and inputNodeId == self._reconnectOldTargetNodeId
                and inputPortName == self._reconnectOldTargetPort
            ):
                self._cancelLinkReconnect()
                return
            self._createConnection(
                outputNodeId, outputPortName, inputNodeId, inputPortName
            )

        menu.triggered.connect(onPinSelected)
        if self._reconnectingLink:
            from PySide6 import QtCore

            menu.aboutToHide.connect(
                lambda: QtCore.QTimer.singleShot(
                    0,
                    lambda: (
                        self._finishReconnectDisconnect()
                        if self._reconnectingLink
                        else None
                    ),
                )
            )
        menu.popup(self.mapToGlobal(self.lastMousePos))

    def _hasExactLink(
        self,
        outputNodeId,
        outputPortName,
        inputNodeId,
        inputPortName,
        outputPropertyName="",
        inputPropertyName="",
    ):
        """Return True if an identical link already exists."""
        out_prop = outputPropertyName or ""
        in_prop = inputPropertyName or ""
        for existing in self.links:
            if (
                existing.sourceNodeId == outputNodeId
                and existing.sourcePort == outputPortName
                and existing.targetNodeId == inputNodeId
                and existing.targetPort == inputPortName
                and (getattr(existing, "sourcePropertyName", "") or "") == out_prop
                and (getattr(existing, "targetPropertyName", "") or "") == in_prop
            ):
                return True
        return False

    def _findLinkToInput(self, nodeId, portName):
        """
        Find link connected to specified input port.

        Args:
            nodeId: ID of the node
            portName: Name of the input port

        Returns:
            LinkData if found, None otherwise
        """
        if is_multi_target_relationship_input_pin(portName):
            return None
        for link in self.links:
            if link.targetNodeId == nodeId and link.targetPort == portName:
                return link
        return None

    def _persistNodePositions(self, stage, layer):
        """Write all current node positions to the given layer.

        Positions live in the session layer during interactive edits.
        This flushes them to a persistent layer before saving so they
        survive reload.
        """
        from pxr import Sdf

        prev = stage.GetEditTarget()
        with Sdf.ChangeBlock():
            stage.SetEditTarget(layer)
            try:
                for node in self.nodes.values():
                    node._writePositionToUsdRaw(node.position)
            finally:
                stage.SetEditTarget(prev)

    def _saveStage(self):
        """Save the current edit target layer to disk.

        Saves whichever layer is currently selected as the Edit Target.
        Triggered by Ctrl+S (Cmd+S on Mac).
        """
        stage = self.nodeGraph.getStage() if self.nodeGraph else None
        if not stage:
            self._showPopupMessage("No stage to save")
            return

        try:
            layer = stage.GetEditTarget().GetLayer()

            if not layer:
                self._showPopupMessage("No edit target layer to save")
                return

            if layer.anonymous:
                self._showPopupMessage(
                    f"Layer '{layer.identifier}' is anonymous — use File > Save As"
                )
                return

            self._persistNodePositions(stage, layer)
            layer.Save()

            # Warn if other layers have unsaved edits that this save didn't cover.
            unsaved = []
            try:
                for otherLayer in stage.GetLayerStack(includeSessionLayers=False):
                    if otherLayer and otherLayer != layer and otherLayer.dirty:
                        unsaved.append(
                            os.path.basename(otherLayer.identifier)
                            or otherLayer.identifier
                        )
            except Exception:
                pass
            if unsaved:
                names = ", ".join(unsaved[:3])
                suffix = f" (+{len(unsaved) - 3} more)" if len(unsaved) > 3 else ""
                self._showPopupMessage(
                    f"Saved layer: {layer.identifier}\n"
                    f"⚠ Unsaved changes on: {names}{suffix}"
                )
            else:
                self._showPopupMessage(f"Saved layer: {layer.identifier}")

        except Exception as e:
            self._showPopupMessage(f"Save failed: {e}")
            import traceback

            traceback.print_exc()

    def _deleteAllSelected(self):
        """Delete all selected links and nodes in a single undo block.

        This ensures that pressing Delete once removes everything that is
        selected, and a single Ctrl+Z restores it all. Links incident to a
        deleted node are also removed from USD so their authored connections
        do not persist as dangling links after the node is deactivated.
        """
        linksToDelete = self._collectSelectedLinkData()
        selectedNodes = [
            (nodeId, node) for nodeId, node in self.nodes.items() if node.selected
        ]

        if not linksToDelete and not selectedNodes:
            return

        if selectedNodes:
            selectedNodeIds = {nodeId for nodeId, _ in selectedNodes}
            linksToDelete.extend(
                self._collectIncidentLinkKeys(selectedNodeIds, linksToDelete)
            )

        stage = self.nodeGraph.getStage()
        if not stage:
            return

        self._noticeHandler.setEnabled(False)
        try:
            # _deleteLinksInUsd handles layer targeting internally via
            # _findConnectionAuthoringLayer for each connection.
            actually_deleted_links, deletedLinks = self._deleteLinksInUsd(
                stage, linksToDelete
            )
            # Deactivation is authored to the stage's current edit target.
            deleted_prim_paths, deletedNodes = self._deactivateNodesInUsd(selectedNodes)
        finally:
            self._noticeHandler.setEnabled(True)

        # Push undo command if anything was deleted
        if deletedLinks > 0 or deletedNodes > 0:
            self._pushDeleteSelectedUndo(actually_deleted_links, deleted_prim_paths)

        self.clearLinkSelection()

        if deletedLinks > 0 or deletedNodes > 0:
            self.linksChanged = True
            self.textChanged = True
            self._clearRenderCache()
            if deletedNodes > 0:
                self._rebuildLinks()
            self.update()

            parts = []
            if deletedNodes > 0:
                parts.append(f"{deletedNodes} node(s)")
            if deletedLinks > 0:
                parts.append(f"{deletedLinks} connection(s)")
            msg = "Deleted " + " and ".join(parts)
            self._showPopupMessage(msg)
            Tf.Status(f"[DELETE] {msg}")

    def _collectSelectedLinkData(self):
        """Return serialized connection data for selected links."""
        result = []
        if self._selectedLinks:
            for linkIndex in sorted(self._selectedLinks, reverse=True):
                if 0 <= linkIndex < len(self.links):
                    link = self.links[linkIndex]
                    result.append(
                        self._makeSerializedLinkRecord(
                            link.sourceNodeId,
                            link.sourcePort,
                            link.targetNodeId,
                            link.targetPort,
                            sourcePropertyName=str(
                                getattr(link, "sourcePropertyName", "")
                            ),
                            targetPropertyName=str(
                                getattr(link, "targetPropertyName", "")
                            ),
                        )
                    )
        return result

    def _collectIncidentLinkKeys(self, selectedNodeIds, existingLinkRecords=None):
        """Collect deduplicated link-key tuples for links incident to selectedNodeIds.

        If existingLinkRecords is provided, those are deserialized and seeded
        into the seen set so they are not duplicated in the result.
        """
        seen = set()
        if existingLinkRecords:
            for linkRecord in existingLinkRecords:
                try:
                    record = self._deserializeLinkRecord(linkRecord)
                except (KeyError, IndexError, TypeError):
                    continue
                seen.add(
                    (
                        record["sourceNodeId"],
                        record["sourcePort"],
                        record["targetNodeId"],
                        record["targetPort"],
                        record["sourcePropertyName"],
                        record["targetPropertyName"],
                    )
                )
        result = []
        for link in self.links:
            if (
                link.sourceNodeId in selectedNodeIds
                or link.targetNodeId in selectedNodeIds
            ):
                key = (
                    link.sourceNodeId,
                    link.sourcePort,
                    link.targetNodeId,
                    link.targetPort,
                    str(getattr(link, "sourcePropertyName", "")),
                    str(getattr(link, "targetPropertyName", "")),
                )
                if key not in seen:
                    seen.add(key)
                    result.append(key)
        return result

    @staticmethod
    def _makeSerializedLinkRecord(
        srcId,
        srcPort,
        tgtId,
        tgtPort,
        *,
        sourcePropertyName="",
        targetPropertyName="",
    ):
        return {
            "sourceNodeId": srcId,
            "sourcePort": srcPort,
            "targetNodeId": tgtId,
            "targetPort": tgtPort,
            "sourcePropertyName": sourcePropertyName,
            "targetPropertyName": targetPropertyName,
        }

    @staticmethod
    def _deserializeLinkRecord(linkRecord):
        if isinstance(linkRecord, dict):
            return {
                "sourceNodeId": str(linkRecord.get("sourceNodeId", "")),
                "sourcePort": str(linkRecord.get("sourcePort", "")),
                "targetNodeId": str(linkRecord.get("targetNodeId", "")),
                "targetPort": str(linkRecord.get("targetPort", "")),
                "sourcePropertyName": str(linkRecord.get("sourcePropertyName", "")),
                "targetPropertyName": str(linkRecord.get("targetPropertyName", "")),
            }

        srcId, srcPort, tgtId, tgtPort = linkRecord[:4]
        sourcePropertyName = linkRecord[4] if len(linkRecord) > 4 else ""
        targetPropertyName = linkRecord[5] if len(linkRecord) > 5 else ""
        return {
            "sourceNodeId": srcId,
            "sourcePort": srcPort,
            "targetNodeId": tgtId,
            "targetPort": tgtPort,
            "sourcePropertyName": sourcePropertyName,
            "targetPropertyName": targetPropertyName,
        }

    @classmethod
    def _linkMatchesRecord(cls, link, linkRecord):
        record = cls._deserializeLinkRecord(linkRecord)
        return (
            link.sourceNodeId == record["sourceNodeId"]
            and link.sourcePort == record["sourcePort"]
            and link.targetNodeId == record["targetNodeId"]
            and link.targetPort == record["targetPort"]
            and str(getattr(link, "sourcePropertyName", ""))
            == record["sourcePropertyName"]
            and str(getattr(link, "targetPropertyName", ""))
            == record["targetPropertyName"]
        )

    @staticmethod
    def _expandSdfListOpItems(listOp):
        """Flatten an Sdf list-op into a single list across all four slots."""
        if not listOp:
            return []
        return (
            list(listOp.explicitItems)
            + list(listOp.prependedItems)
            + list(listOp.appendedItems)
            + list(listOp.addedItems)
        )

    @staticmethod
    def _buildAttrSearchPaths(inputPath, outputPath, targetPort, srcPort):
        """Build (propPath, otherPrimPath, portNameSet) tuples for attr scan.

        Enumerates every direction-hint spelling and the bare/literal name on
        both sides, so connections authored on ``in:``/``input:``/``inputs:``
        (or namespaced attrs such as ``rig1:space``) are all discoverable.
        """
        srcPortNames = set(direction_hint_candidate_names(srcPort, is_input=False))
        tgtPortNames = set(direction_hint_candidate_names(targetPort, is_input=True))
        paths = []
        if targetPort:
            for name in direction_hint_candidate_names(targetPort, is_input=True):
                paths.append((inputPath.AppendProperty(name), outputPath, srcPortNames))
        if srcPort:
            for name in direction_hint_candidate_names(srcPort, is_input=False):
                paths.append((outputPath.AppendProperty(name), inputPath, tgtPortNames))
        return paths

    @staticmethod
    def _buildRelSearchEntries(inputPath, outputPath, targetPort, srcPort):
        """Build (relName, ownerPath, targetPrimPath) tuples for relationship scan."""
        entries = []
        if targetPort:
            entries.append((targetPort, inputPath, outputPath))
        if srcPort:
            entries.append((srcPort, outputPath, inputPath))
        return entries

    @classmethod
    def _layerAuthorsAttrConnection(cls, layer, attrPaths):
        """Return True if any attr path in this layer has a matching connection."""
        for propPath, otherPrimPath, portNames in attrPaths:
            attrSpec = layer.GetAttributeAtPath(propPath)
            if not attrSpec:
                continue
            for conn_path in cls._expandSdfListOpItems(attrSpec.connectionPathList):
                if (
                    conn_path.GetPrimPath() == otherPrimPath
                    and conn_path.name in portNames
                ):
                    return True
        return False

    @classmethod
    def _layerAuthorsRelTarget(cls, layer, relEntries):
        """Return True if any relationship in this layer targets the other prim."""
        for relName, ownerPath, targetPrimPath in relEntries:
            propSpec = layer.GetPropertyAtPath(ownerPath.AppendProperty(relName))
            if not propSpec:
                continue
            targetList = getattr(propSpec, "targetPathList", None)
            for target in cls._expandSdfListOpItems(targetList):
                if target == targetPrimPath or target.GetPrimPath() == targetPrimPath:
                    return True
        return False

    def _findConnectionAuthoringLayer(self, inputPrim, targetPort, outputPrim, srcPort):
        """Find which layer in the stack authors the connection.

        Checks both attribute connections and relationship targets.
        Returns the layer identifier (string) or None if not found.
        """
        stage = self.nodeGraph.getStage()
        if not stage:
            return None
        if not inputPrim or not inputPrim.IsValid():
            return None
        if not outputPrim or not outputPrim.IsValid():
            return None

        inputPath = inputPrim.GetPath()
        outputPath = outputPrim.GetPath()
        attrPaths = self._buildAttrSearchPaths(
            inputPath, outputPath, targetPort, srcPort
        )
        relEntries = self._buildRelSearchEntries(
            inputPath, outputPath, targetPort, srcPort
        )

        for layer in stage.GetLayerStack():
            if self._layerAuthorsAttrConnection(layer, attrPaths):
                return layer.identifier
            if self._layerAuthorsRelTarget(layer, relEntries):
                return layer.identifier
        return None

    def _deleteLinksInUsd(self, stage, linksToDelete):
        """Delete connections in USD and remove from local link list.

        Returns (actually_deleted_links, count). Each deleted link record
        includes '_layer_id' indicating which layer it was deleted from.
        Deletes are issued via the authoring layer when one is identified;
        a missing-but-named layer triggers a Tf.Warn via _editOnLayer.
        """
        actually_deleted = []
        count = 0
        for linkRecord in linksToDelete:
            record = self._deserializeLinkRecord(linkRecord)
            srcId = record["sourceNodeId"]
            srcPort = record["sourcePort"]
            tgtId = record["targetNodeId"]
            tgtPort = record["targetPort"]
            inputNode = self.nodes.get(tgtId)
            outputNode = self.nodes.get(srcId)
            if not inputNode or not outputNode:
                continue
            inputPrim = inputNode.getUsdPrim()
            outputPrim = outputNode.getUsdPrim()
            if not inputPrim or not outputPrim:
                continue
            library = self._findLibraryForPrim(inputPrim)
            if not library:
                continue

            layer_id = self._findConnectionAuthoringLayer(
                inputPrim, tgtPort, outputPrim, srcPort
            )
            connection_kwargs = self._buildConnectionKwargs(record)

            fallback = stage.GetEditTarget().GetLayer()
            with self._editOnLayer(stage, layer_id, fallback):
                success = library.delete_connection(
                    stage,
                    outputPrim,
                    srcPort,
                    inputPrim,
                    tgtPort,
                    **connection_kwargs,
                )

            if success:
                record_with_layer = dict(record)
                record_with_layer["_layer_id"] = layer_id
                actually_deleted.append(record_with_layer)
                self._removeLinkFromGraph(
                    srcId,
                    srcPort,
                    tgtId,
                    tgtPort,
                    sourcePropertyName=record["sourcePropertyName"],
                    targetPropertyName=record["targetPropertyName"],
                )
                count += 1
            elif layer_id is None:
                Tf.Warn(
                    f"Failed to delete connection {srcId}.{srcPort} -> "
                    f"{tgtId}.{tgtPort}: authoring layer not found and "
                    f"fallback deletion returned False"
                )
        return actually_deleted, count

    def _deactivateNodesInUsd(self, selectedNodes):
        """Deactivate selected nodes in USD and remove from graph state.

        Returns (deleted_prim_paths, count).
        """
        deleted_prim_paths = []
        count = 0
        for nodeId, node in selectedNodes:
            prim = node.getUsdPrim()
            if not prim or not prim.IsValid():
                continue
            try:
                deleted_prim_paths.append(str(prim.GetPath()))
                prim.SetActive(False)
                del self.nodes[nodeId]
                self._selectedNodes.discard(nodeId)
                self._spatialIndex.removeNode(nodeId)
                count += 1
            except Exception as e:
                Tf.Warn(f"Failed to delete node {nodeId}: {e}")
                import traceback

                traceback.print_exc()
        return deleted_prim_paths, count

    def _pushDeleteSelectedUndo(self, deleted_links, deleted_paths):
        """Push an undo command that restores deleted links and nodes.

        Links and nodes are stored with their layer info so undo/redo
        can operate on the correct layers regardless of the current edit target.
        """
        gv = self
        stage = gv.nodeGraph.getStage()
        if not stage:
            Tf.Warn(
                "Cannot register undo for delete: no stage available. "
                f"{len(deleted_links)} link(s) and {len(deleted_paths)} node(s) "
                "have already been deleted and will not be undoable."
            )
            return

        # Capture layer info for nodes (typically session layer, but be explicit)
        session_layer_id = stage.GetSessionLayer().identifier
        node_data_with_layers = [(path, session_layer_id) for path in deleted_paths]

        # Links already have layer info from _deleteLinksInUsd
        link_data_with_layers = deleted_links

        def _undo(link_data=link_data_with_layers, node_data=node_data_with_layers):
            gv._applyRestoreDeleted(link_data, node_data)
            gv.linksChanged = True
            gv._rebuildLinks()
            gv.update()

        def _redo(link_data=link_data_with_layers, node_data=node_data_with_layers):
            gv._applyDeleteSelected(link_data, node_data)
            gv.linksChanged = True
            gv._rebuildLinks()
            gv.update()

        _push_undo_command("Delete Selected", _redo, _undo)

    @contextmanager
    def _editOnLayer(self, stage, layer_id, fallback_layer):
        """Set the stage edit target to the named layer, restore on exit.

        Falls back to ``fallback_layer`` when ``layer_id`` is empty or no
        longer loadable. Warns when a non-empty layer identifier can no
        longer be resolved so silent fallback is observable.
        """
        layer = Sdf.Layer.Find(layer_id) if layer_id else fallback_layer
        if layer is None:
            if layer_id:
                Tf.Warn(
                    f"Layer {layer_id!r} no longer loadable; falling back to "
                    f"{fallback_layer.identifier}"
                )
            layer = fallback_layer
        prevTarget = stage.GetEditTarget()
        try:
            stage.SetEditTarget(layer)
            yield layer
        finally:
            stage.SetEditTarget(prevTarget)

    @staticmethod
    def _buildConnectionKwargs(record):
        """Build create/delete-connection kwargs from a deserialized record."""
        if record["sourcePropertyName"] or record["targetPropertyName"]:
            return {
                "source_property_name": record["sourcePropertyName"],
                "target_property_name": record["targetPropertyName"],
            }
        return {}

    @staticmethod
    def _groupLinksByLayer(link_data):
        by_layer = defaultdict(list)
        for link_record in link_data:
            by_layer[link_record.get("_layer_id")].append(link_record)
        return by_layer

    @staticmethod
    def _groupNodesByLayer(node_data):
        by_layer = defaultdict(list)
        for path, layer_id in node_data:
            by_layer[layer_id].append(path)
        return by_layer

    def _setNodesActiveOnLayer(self, stage, layer_id, paths, active):
        """Apply SetActive(active) to each path on the resolved layer.

        Fallback (when ``layer_id`` is empty or no longer loadable) is the
        stage's current edit target so undo/redo of legacy records honors the
        user's chosen authoring layer.
        """
        fallback = stage.GetEditTarget().GetLayer()
        with self._editOnLayer(stage, layer_id, fallback):
            for path_str in paths:
                prim = stage.GetPrimAtPath(path_str)
                if prim:
                    prim.SetActive(active)

    def _runLinkOperationOnLayer(
        self, stage, layer_id, link_records, op_name, failure_verb
    ):
        """Call create_connection or delete_connection per record on a layer.

        ``op_name`` must be ``"create_connection"`` or ``"delete_connection"``.
        ``failure_verb`` appears in the ``Tf.Warn`` emitted on op failure.
        Fallback (when ``layer_id`` is empty or no longer loadable) is the
        stage's current edit target.
        """
        fallback = stage.GetEditTarget().GetLayer()
        with self._editOnLayer(stage, layer_id, fallback):
            for linkRecord in link_records:
                record = self._deserializeLinkRecord(linkRecord)
                srcId = record["sourceNodeId"]
                srcPort = record["sourcePort"]
                tgtId = record["targetNodeId"]
                tgtPort = record["targetPort"]
                outputPrim = stage.GetPrimAtPath(srcId)
                inputPrim = stage.GetPrimAtPath(tgtId)
                if not (inputPrim and outputPrim):
                    continue
                library = self._findLibraryForPrim(inputPrim)
                if not library:
                    continue
                op = getattr(library, op_name)
                success = op(
                    stage,
                    outputPrim,
                    srcPort,
                    inputPrim,
                    tgtPort,
                    **self._buildConnectionKwargs(record),
                )
                if not success:
                    Tf.Warn(
                        f"Failed to {failure_verb} connection "
                        f"{srcId}.{srcPort} -> {tgtId}.{tgtPort}"
                    )

    def _applyRestoreDeleted(self, link_data, node_data):
        """Reactivate nodes and recreate connections for undo.

        link_data: list of link records (dicts) with '_layer_id' key
        node_data: list of (path, layer_id) tuples

        Records that carry a layer id replay on that exact layer. Records with
        no recorded layer (e.g. legacy data) fall back to the stage's current
        edit target so the undo still honors the user's authoring choice. All
        edits across all layers are wrapped in a single ``Sdf.ChangeBlock`` so
        the notice handler observes one batched change rather than many.
        """
        stage = self.nodeGraph.getStage()
        if not stage:
            return
        nodes_by_layer = self._groupNodesByLayer(node_data)
        links_by_layer = self._groupLinksByLayer(link_data)
        with Sdf.ChangeBlock():
            for layer_id, paths in nodes_by_layer.items():
                self._setNodesActiveOnLayer(stage, layer_id, paths, True)
            for layer_id, link_records in links_by_layer.items():
                self._runLinkOperationOnLayer(
                    stage, layer_id, link_records, "create_connection", "restore"
                )

    def _applyDeleteSelected(self, link_data, node_data):
        """Delete connections and deactivate nodes for redo.

        Mirrors _applyRestoreDeleted: same layer dispatch, opposite operations
        and reverse order (links removed before nodes deactivate). All edits
        are batched in a single Sdf.ChangeBlock.
        """
        stage = self.nodeGraph.getStage()
        if not stage:
            return
        nodes_by_layer = self._groupNodesByLayer(node_data)
        links_by_layer = self._groupLinksByLayer(link_data)
        with Sdf.ChangeBlock():
            for layer_id, link_records in links_by_layer.items():
                self._runLinkOperationOnLayer(
                    stage, layer_id, link_records, "delete_connection", "delete"
                )
            for layer_id, paths in nodes_by_layer.items():
                self._setNodesActiveOnLayer(stage, layer_id, paths, False)

    def deleteSelectedNodes(self):
        """Delete all selected nodes (and their incident links) from USD.

        Nodes are deactivated via ``prim.SetActive(False)`` on the stage's
        current edit target. Incident links are removed from whichever layer
        authored the connection (determined via
        ``_findConnectionAuthoringLayer``) so they do not persist as dangling
        links after deactivation.
        """
        if not self.nodes:
            return

        selectedNodes = [
            (nodeId, node) for nodeId, node in self.nodes.items() if node.selected
        ]
        if not selectedNodes:
            Tf.Status("[DELETE] No nodes selected to delete")
            return

        Tf.Status(f"[DELETE] Deleting {len(selectedNodes)} selected node(s)")

        stage = self.nodeGraph.getStage()
        if not stage:
            Tf.Warn("Cannot delete nodes: no stage available")
            return

        selectedNodeIds = {nodeId for nodeId, _ in selectedNodes}
        incidentLinks = self._collectIncidentLinkKeys(selectedNodeIds)

        warn_if_non_persistent_edit_target(stage)

        self._noticeHandler.setEnabled(False)
        try:
            # _deleteLinksInUsd handles layer targeting internally via
            # _findConnectionAuthoringLayer for each connection.
            actually_deleted_links, _ = self._deleteLinksInUsd(stage, incidentLinks)
            deleted_prim_paths, deletedCount = self._deactivateNodesInUsd(selectedNodes)
        finally:
            self._noticeHandler.setEnabled(True)

        if deletedCount > 0:
            self._pushDeleteSelectedUndo(actually_deleted_links, deleted_prim_paths)

            # Mark caches as dirty to trigger proper rebuild
            self.linksChanged = True
            self.textChanged = True

            # IMPORTANT: makeCurrent() ensures clearCache() deletes GL resources
            # (VBOs/VAOs) from this widget's OpenGL context, not the stageView's.
            # Without this, glDeleteBuffers/glDeleteVertexArrays may operate on
            # the wrong context, corrupting the 3D viewport's render state.
            self._clearRenderCache()

            self._rebuildLinks()
            self.update()

            self._showPopupMessage(f"Deleted {deletedCount} node(s)")
            Tf.Status(f"[DELETE] Deleted {deletedCount} node(s)")
        else:
            self._showPopupMessage("No nodes deleted")
            Tf.Status("[DELETE] No nodes were deleted")

    def _findLibraryForPrim(self, prim):
        """Find which node library can handle this prim type."""
        if __debug__:
            matches = [
                lib
                for lib in self._nodeLibraries
                if lib.enabled and lib.can_handle_prim(prim)
            ]
            if len(matches) > 1:
                Tf.Warn(
                    f"Multiple libraries claim prim {prim.GetPath()}: "
                    f"{[m.get_name() for m in matches]}"
                )
        for library in self._nodeLibraries:
            if library.enabled and library.can_handle_prim(prim):
                return library
        return None

    def _abortConnection(self, message, reconnect_state=None):
        Tf.Warn(message)
        if self._reconnectingLink:
            self._cancelLinkReconnect()
        elif reconnect_state is not None:
            oldSrcId, oldSrcPort, oldTgtId, oldTgtPort = reconnect_state
            srcNode = self.nodes.get(oldSrcId)
            tgtNode = self.nodes.get(oldTgtId)
            if srcNode and tgtNode:
                self._addLinkToGraph(
                    oldSrcId, oldSrcPort, oldTgtId, oldTgtPort, srcNode, tgtNode
                )

    def _resolveConnectionEndpoints(self, outputNodeId, inputNodeId):
        outputNode = self.nodes.get(outputNodeId)
        inputNode = self.nodes.get(inputNodeId)
        if not outputNode or not inputNode:
            self._abortConnection("Cannot create connection: invalid node IDs")
            return None

        outputPrim = outputNode.getUsdPrim()
        inputPrim = inputNode.getUsdPrim()
        if not outputPrim or not inputPrim:
            self._abortConnection("Cannot create connection: invalid USD prims")
            return None

        library = self._findLibraryForPrim(inputPrim)
        if not library:
            self._abortConnection(
                f"Cannot create connection: no library handles prim type '{inputPrim.GetTypeName()}'"
            )
            return None

        return outputNode, inputNode, outputPrim, inputPrim, library

    def _deleteOldConnection(self, stage, oldSrcId, oldSrcPort, oldTgtId, oldTgtPort):
        oldInputNode = self.nodes.get(oldTgtId)
        oldOutputNode = self.nodes.get(oldSrcId)
        if not oldInputNode or not oldOutputNode:
            return False
        oldInputPrim = oldInputNode.getUsdPrim()
        oldOutputPrim = oldOutputNode.getUsdPrim()
        if not oldInputPrim or not oldOutputPrim:
            return False
        oldLib = self._findLibraryForPrim(oldInputPrim)
        if not oldLib:
            return False
        return oldLib.delete_connection(
            stage, oldOutputPrim, oldSrcPort, oldInputPrim, oldTgtPort
        )

    def _createConnection(
        self,
        outputNodeId,
        outputPort,
        inputNodeId,
        inputPort,
        reconnect_state=None,
        *,
        sourcePropertyName="",
        targetPropertyName="",
    ):
        """Create a connection in USD via the appropriate node library."""
        resolved = self._resolveConnectionEndpoints(outputNodeId, inputNodeId)
        if not resolved:
            self._abortConnection(
                "Failed to resolve connection endpoints", reconnect_state
            )
            return

        outputNode, inputNode, outputPrim, inputPrim, library = resolved

        stage = self.nodeGraph.getStage()
        if not stage:
            Tf.Warn("Cannot create connection: no stage")
            return
        warn_if_non_persistent_edit_target(stage)

        self._noticeHandler.setEnabled(False)
        try:
            connection_kwargs = {}
            if sourcePropertyName or targetPropertyName:
                connection_kwargs = {
                    "source_property_name": sourcePropertyName,
                    "target_property_name": targetPropertyName,
                }
            success = library.create_connection(
                stage,
                outputPrim,
                outputPort,
                inputPrim,
                inputPort,
                **connection_kwargs,
            )

            if not success:
                self._abortConnection(
                    "Failed to create connection in USD", reconnect_state
                )
                return

            if reconnect_state is not None:
                isReconnect = True
                oldSrcId, oldSrcPort, oldTgtId, oldTgtPort = reconnect_state
            else:
                isReconnect = self._reconnectingLink
                oldSrcId = self._reconnectOldSourceNodeId
                oldSrcPort = self._reconnectOldSourcePort
                oldTgtId = self._reconnectOldTargetNodeId
                oldTgtPort = self._reconnectOldTargetPort

            if isReconnect:
                self._clearReconnectState()

                if not self._deleteOldConnection(
                    stage, oldSrcId, oldSrcPort, oldTgtId, oldTgtPort
                ):
                    # Old connection may have been implicitly replaced by
                    # create_connection (e.g. SetConnections on the same
                    # input attribute). Continue with the new connection.
                    Tf.Status(
                        "Reconnect: old connection already removed "
                        f"({oldSrcId}.{oldSrcPort} -> {oldTgtId}.{oldTgtPort})"
                    )

            self._addLinkToGraph(
                outputNodeId,
                outputPort,
                inputNodeId,
                inputPort,
                outputNode,
                inputNode,
                sourcePropertyName=sourcePropertyName,
                targetPropertyName=targetPropertyName,
            )

            gv = self
            if isReconnect:
                redo_fn, undo_fn = _make_reconnect_edits(
                    gv,
                    oldSrcId,
                    oldSrcPort,
                    oldTgtId,
                    oldTgtPort,
                    outputNodeId,
                    outputPort,
                    inputNodeId,
                    inputPort,
                    sourcePropertyName=sourcePropertyName,
                    targetPropertyName=targetPropertyName,
                )
                _push_undo_command("Reconnect", redo_fn, undo_fn)
            else:
                _push_undo_command(
                    "Create Connection",
                    _make_connection_edit(
                        gv,
                        outputNodeId,
                        outputPort,
                        inputNodeId,
                        inputPort,
                        create=True,
                        source_property_name=sourcePropertyName,
                        target_property_name=targetPropertyName,
                    ),
                    _make_connection_edit(
                        gv,
                        outputNodeId,
                        outputPort,
                        inputNodeId,
                        inputPort,
                        create=False,
                        source_property_name=sourcePropertyName,
                        target_property_name=targetPropertyName,
                    ),
                )

        finally:
            # The notice handler is suppressed across the authoring window so
            # _handleUsdChanges cannot drop the cached per-prim link scan that
            # back-reference resolution reads from.  Drop it explicitly here so
            # a later re-add of an absent prim does not resurrect a connection
            # we just authored away.
            self._invalidateBackReferenceLinkDataCache()
            self._noticeHandler.setEnabled(True)
            self.update()

    def _addLinkToGraph(
        self,
        outputNodeId,
        outputPort,
        inputNodeId,
        inputPort,
        outputNode,
        inputNode,
        *,
        sourcePropertyName="",
        targetPropertyName="",
    ):
        """Add a new link to the local graph state (no USD mutation)."""
        link = LinkData()
        link.sourceNodeId = outputNodeId
        link.sourcePort = outputPort
        link.targetNodeId = inputNodeId
        link.targetPort = inputPort
        link.sourcePropertyName = sourcePropertyName
        link.targetPropertyName = targetPropertyName
        relationship_metadata = get_relationship_link_metadata(
            outputNodeId,
            outputPort,
            inputNodeId,
            inputPort,
        )
        link.propertyOwnerNodeId = str(
            relationship_metadata.get("propertyOwnerNodeId", "")
        )
        link.propertyName = str(relationship_metadata.get("propertyName", ""))
        link.is_relationship_link = bool(
            relationship_metadata.get("is_relationship_link", False)
        )

        # USD fallback: the name-based metadata only knows about the
        # hardcoded sources/affects pins.  An unknown-name relationship like
        # material:binding would otherwise drop in as a regular attribute
        # link, so the persistent prim-target arrowhead at the top of the
        # target prim would not draw until the next USD-driven rebuild.
        if not link.is_relationship_link:
            self._classifyRelationshipLinkFromUsd(
                link,
                outputNode,
                inputNode,
                outputPort,
                inputPort,
                sourcePropertyName,
                targetPropertyName,
            )

        self._setupLinkEndpoints(link, outputNode, inputNode, outputPort, inputPort)

        link.data_sourceNodeId = outputNodeId
        link.data_targetNodeId = inputNodeId
        link.is_input_link = False
        link.isDangling = False

        self.links.append(link)

        linkIndex = len(self.links) - 1
        self._spatialIndex.insertLink(linkIndex, self._computeLinkBounds(link))

        self._rebuildConnectionCache()

        outputNode.outputLinks.append(link)
        inputNode.inputLinks.append(link)

        # A link was added in place (no full rebuild); re-mirror to C++ and
        # rebuild port highlights (the new connection changes port colors).
        self._linksNeedSync = True
        self._portHighlightsDirty = True

    @staticmethod
    def _classifyRelationshipLinkFromUsd(
        link,
        outputNode,
        inputNode,
        outputPort,
        inputPort,
        sourcePropertyName,
        targetPropertyName,
    ):
        """Mark ``link`` as a relationship link when the USD prim says so.

        Mutates ``link.is_relationship_link``, ``link.propertyOwnerNodeId``
        and ``link.propertyName`` in place when the source-side port name or
        target-side port name resolves to an authored USD relationship on the
        corresponding prim.  Returns silently if nothing matches.
        """
        for prim_node, property_candidates, owner_id in (
            (
                outputNode,
                (sourcePropertyName, outputPort),
                getattr(outputNode, "id", "") if outputNode is not None else "",
            ),
            (
                inputNode,
                (targetPropertyName, inputPort),
                getattr(inputNode, "id", "") if inputNode is not None else "",
            ),
        ):
            if prim_node is None:
                continue
            prim_getter = getattr(prim_node, "getUsdPrim", None)
            prim = prim_getter() if callable(prim_getter) else None
            if prim is None or not prim.IsValid():
                continue
            for property_name in property_candidates:
                if not property_name:
                    continue
                rel = prim.GetRelationship(property_name)
                if rel and rel.IsValid():
                    link.is_relationship_link = True
                    link.propertyOwnerNodeId = owner_id
                    link.propertyName = property_name
                    return

    def _deleteConnection(
        self,
        outputNodeId,
        outputPort,
        inputNodeId,
        inputPort,
        *,
        sourcePropertyName="",
        targetPropertyName="",
    ):
        """Delete a connection in USD via the appropriate node library."""
        inputNode = self.nodes.get(inputNodeId)
        outputNode = self.nodes.get(outputNodeId)
        if not inputNode or not outputNode:
            return

        inputPrim = inputNode.getUsdPrim()
        outputPrim = outputNode.getUsdPrim()
        if not inputPrim or not outputPrim:
            return

        library = self._findLibraryForPrim(inputPrim)
        if not library:
            Tf.Warn(
                f"Cannot delete connection: no library handles prim type '{inputPrim.GetTypeName()}'"
            )
            return

        self._noticeHandler.setEnabled(False)
        try:
            connection_kwargs = {}
            if sourcePropertyName or targetPropertyName:
                connection_kwargs = {
                    "source_property_name": sourcePropertyName,
                    "target_property_name": targetPropertyName,
                }
            success = library.delete_connection(
                self.nodeGraph.getStage(),
                outputPrim,
                outputPort,
                inputPrim,
                inputPort,
                **connection_kwargs,
            )

            if success:
                self._removeLinkFromGraph(
                    outputNodeId,
                    outputPort,
                    inputNodeId,
                    inputPort,
                    sourcePropertyName=sourcePropertyName,
                    targetPropertyName=targetPropertyName,
                )

                gv = self
                _push_undo_command(
                    "Delete Connection",
                    _make_connection_edit(
                        gv,
                        outputNodeId,
                        outputPort,
                        inputNodeId,
                        inputPort,
                        create=False,
                        source_property_name=sourcePropertyName,
                        target_property_name=targetPropertyName,
                    ),
                    _make_connection_edit(
                        gv,
                        outputNodeId,
                        outputPort,
                        inputNodeId,
                        inputPort,
                        create=True,
                        source_property_name=sourcePropertyName,
                        target_property_name=targetPropertyName,
                    ),
                )
        finally:
            # Mirror _createConnection: drop the back-reference cache while the
            # notice handler is still suppressed, so a later re-add of an
            # absent prim does not resurrect the connection we just deleted.
            self._invalidateBackReferenceLinkDataCache()
            self._noticeHandler.setEnabled(True)

    def _removeLinkFromGraph(
        self,
        outputNodeId,
        outputPort,
        inputNodeId,
        inputPort,
        *,
        sourcePropertyName="",
        targetPropertyName="",
    ):
        """Remove a link from the local graph state (no USD mutation)."""
        linkRecord = self._makeSerializedLinkRecord(
            outputNodeId,
            outputPort,
            inputNodeId,
            inputPort,
            sourcePropertyName=sourcePropertyName,
            targetPropertyName=targetPropertyName,
        )
        linkToRemove = None
        for link in self.links:
            if self._linkMatchesRecord(link, linkRecord):
                linkToRemove = link
                break

        if not linkToRemove:
            return

        linkIndex = self.links.index(linkToRemove)

        outputNode = self.nodes.get(outputNodeId)
        inputNode = self.nodes.get(inputNodeId)
        if outputNode and linkToRemove in outputNode.outputLinks:
            outputNode.outputLinks.remove(linkToRemove)
        if inputNode and linkToRemove in inputNode.inputLinks:
            inputNode.inputLinks.remove(linkToRemove)

        self._spatialIndex.removeLink(linkIndex)
        self.links.remove(linkToRemove)

        # Re-index remaining links in spatial index (indices shifted)
        for i in range(linkIndex, len(self.links)):
            self._spatialIndex.removeLink(i + 1)
            self._spatialIndex.insertLink(i, self._computeLinkBounds(self.links[i]))

        self._rebuildConnectionCache()
        # A link was removed in place (no full rebuild); re-mirror to C++ and
        # rebuild port highlights (the removed connection changes port colors).
        self._linksNeedSync = True
        self._portHighlightsDirty = True
        self.update()

    def _reloadLinksFromUSD(self):
        """Reload links from USD after connection changes.

        This rebuilds the links array by reading USD connections directly,
        without reloading the entire graph. Much more efficient than a full reload.
        """
        # Rebuild links directly from USD
        self._rebuildLinks()

        # Update the view
        self.update()

    def _selectNodesInMarquee(self, nodeIds, marqueeBounds):
        newlySelected = set()
        for nodeId in nodeIds:
            if nodeId in self.nodes:
                node = self.nodes[nodeId]
                nodeBounds = Gf.Range2d(node.position, node.position + node.size)
                if not Gf.Range2d.GetIntersection(marqueeBounds, nodeBounds).IsEmpty():
                    node.selected = True
                    newlySelected.add(node.id)
        return newlySelected

    def _selectLinksInMarquee(self, linkIndices, marqueeBounds):
        candidateLinks = []
        candidateIndices = []
        for linkIndex in linkIndices:
            if linkIndex < len(self.links):
                candidateLinks.append(self.links[linkIndex])
                candidateIndices.append(linkIndex)

        newlySelected = set()
        if candidateLinks:
            hitPositions = self.linkRenderer.findLinksInBounds(
                candidateLinks, marqueeBounds
            )
            for hitPos in hitPositions:
                linkIndex = candidateIndices[hitPos]
                self.links[linkIndex].selected = True
                newlySelected.add(linkIndex)
        return newlySelected

    def _updateMarqueeSelection(self):
        """Update node and link selection live during marquee drag."""
        if not self.marqueeActive:
            return

        # Deselect nodes/links added by the previous frame (not in base set).
        # Must happen before the minMarqueeSize check so shrinking below the
        # threshold doesn't leave stale selections.
        for nodeId in self._marqueePrevSelectedNodes - self._marqueeBaseSelectedNodes:
            if nodeId in self.nodes:
                self.nodes[nodeId].selected = False
        self._selectedNodes = set(self._marqueeBaseSelectedNodes)

        for linkIdx in self._marqueePrevSelectedLinks - self._marqueeBaseSelectedLinks:
            if linkIdx < len(self.links):
                self.links[linkIdx].selected = False
        self._selectedLinks = set(self._marqueeBaseSelectedLinks)
        # Link selection flags change every marquee frame (deselect above and/or
        # select below, including the early-return shrink path); re-mirror to C++.
        self._linksNeedSync = True

        minMarqueeSize = 2.0 / self.zoom
        dx = abs(self.marqueeCurrent.x() - self.marqueeStart.x())
        dy = abs(self.marqueeCurrent.y() - self.marqueeStart.y())
        if dx < minMarqueeSize and dy < minMarqueeSize:
            self._marqueePrevSelectedNodes = set(self._marqueeBaseSelectedNodes)
            self._marqueePrevSelectedLinks = set(self._marqueeBaseSelectedLinks)
            return

        marqueeBounds = Gf.Range2d(
            Gf.Vec2d(
                min(self.marqueeStart.x(), self.marqueeCurrent.x()),
                min(self.marqueeStart.y(), self.marqueeCurrent.y()),
            ),
            Gf.Vec2d(
                max(self.marqueeStart.x(), self.marqueeCurrent.x()),
                max(self.marqueeStart.y(), self.marqueeCurrent.y()),
            ),
        )

        nodeIds, linkIndices = self._spatialIndex.queryRegion(marqueeBounds)

        newNodes = self._selectNodesInMarquee(nodeIds, marqueeBounds)
        self._selectedNodes |= newNodes
        self._marqueePrevSelectedNodes = set(self._selectedNodes)

        newLinks = self._selectLinksInMarquee(linkIndices, marqueeBounds)
        self._selectedLinks |= newLinks
        self._marqueePrevSelectedLinks = set(self._selectedLinks)

    def selectAll(self):
        for node in self.nodes.values():
            node.selected = True
            self._selectedNodes.add(node.id)
        self.update()
        self._syncSelectionToPrimtree()

    def clearSelection(self):
        for node in self.nodes.values():
            node.selected = False
        self._selectedNodes.clear()
        self.update()

    def frameAll(self):
        """Frame all nodes in the graph, ignoring current selection."""
        if not self.nodes:
            return

        # Calculate bounds of all nodes
        sceneBounds = Gf.Range2d()
        for node in self.nodes.values():
            nodeBounds = Gf.Range2d(node.position, node.position + node.size)
            sceneBounds = Gf.Range2d.GetUnion(sceneBounds, nodeBounds)

        if sceneBounds.IsEmpty():
            return

        boundsSize = Gf.Vec2d(sceneBounds.GetSize())
        if boundsSize[0] == 0 or boundsSize[1] == 0:
            return

        # Frame with a comfortable margin (1.1 = 10% padding)
        self._frameNodeBounds(sceneBounds, margin=1.1)
        self.update()

    def frameScene(self):
        if not self.nodes:
            return

        if self._usdviewApi:
            try:
                dataModel = self._usdviewApi.dataModel
                if dataModel and dataModel.selection:
                    selectedPrims = dataModel.selection.getPrims()
                    if selectedPrims:
                        matchedNodes = []
                        for prim in selectedPrims:
                            primPathStr = str(prim.GetPath())
                            if primPathStr in self.nodes:
                                matchedNodes.append(self.nodes[primPathStr])

                        if matchedNodes:
                            self.clearSelection()
                            for node in matchedNodes:
                                node.selected = True
                                self._selectedNodes.add(node.id)
            except Exception:
                pass

        hasSelection = any(node.selected for node in self.nodes.values())
        sceneBounds = Gf.Range2d()
        selectionBounds = Gf.Range2d()

        for node in self.nodes.values():
            nodeBounds = Gf.Range2d(node.position, node.position + node.size)
            sceneBounds = Gf.Range2d.GetUnion(sceneBounds, nodeBounds)
            if node.selected:
                selectionBounds = Gf.Range2d.GetUnion(selectionBounds, nodeBounds)

        bounds = selectionBounds if hasSelection else sceneBounds
        if bounds.IsEmpty():
            return

        boundsSize = Gf.Vec2d(bounds.GetSize())
        if boundsSize[0] == 0 or boundsSize[1] == 0:
            return

        if hasSelection:
            # Expand bounds to 3x so selection occupies ~1/3 of view
            boundsCenter = Gf.Vec2d(
                (bounds.GetMin()[0] + bounds.GetMax()[0]) * 0.5,
                (bounds.GetMin()[1] + bounds.GetMax()[1]) * 0.5,
            )
            expandedSize = boundsSize * 3.0
            bounds = Gf.Range2d(
                boundsCenter - expandedSize * 0.5, boundsCenter + expandedSize * 0.5
            )
            boundsSize = Gf.Vec2d(bounds.GetSize())

        viewSize = Gf.Vec2d(self.width(), self.height())
        scale = Gf.Vec2d(viewSize[0] / boundsSize[0], viewSize[1] / boundsSize[1])
        self.zoom = min(scale[0], scale[1])

        # Center bounds in view
        scaledView = viewSize / self.zoom
        offset = (scaledView - boundsSize) * 0.5
        self.panX = bounds.GetMin()[0] - offset[0]
        self.panY = bounds.GetMin()[1] - offset[1]
        self.update()

    def frameSelectionWithConnections(self):
        if not self.nodes:
            return

        selectedNodeIds = set()
        for nodeId, node in self.nodes.items():
            if node.selected:
                selectedNodeIds.add(nodeId)

        if not selectedNodeIds:
            self.frameScene()
            return

        connectedNodeIds = set()
        for nodeId in selectedNodeIds:
            connectedNodeIds.update(self.getConnectedNodes(nodeId))

        allNodeIds = selectedNodeIds | connectedNodeIds

        bounds = Gf.Range2d()
        for nodeId in allNodeIds:
            if nodeId not in self.nodes:
                continue
            node = self.nodes[nodeId]
            nodeBounds = Gf.Range2d(node.position, node.position + node.size)
            bounds = Gf.Range2d.GetUnion(bounds, nodeBounds)

        if not bounds.IsEmpty():
            self._frameNodeBounds(bounds)

    def _syncLinksIfNeeded(self, graph):
        """Mirror self.links into the held C++ GraphModel snapshot, but only when
        that snapshot is stale.

        syncLinksFromModels copies every LinkData across the C++ boundary, so
        doing it unconditionally every frame is O(links) of pure marshalling on
        the redraw hot path. The snapshot only goes stale when the hovered link
        changes, or when some other path mutates a rendered link field
        (selection, highlight, endpoints, membership) and flags
        self._linksNeedSync. A plain pan/zoom redraw changes neither, so it skips
        the marshal entirely.

        Hover is derived from self._linkUnderCursor and re-applied here (rather
        than every frame) whenever the hovered index changes; it persists on the
        snapshot between frames, so there is no per-frame set/clear sweep.
        """
        if (
            self._linkUnderCursor == self._lastSyncedLinkHover
            and not self._linksNeedSync
        ):
            return
        for i, link in enumerate(self.links):
            link.hovered = i == self._linkUnderCursor
        self._lastSyncedLinkHover = self._linkUnderCursor
        graph.syncLinksFromModels(self.links)
        self._linksNeedSync = False

    def paintLinks(self):
        """Paint all links in the scene."""
        if not self.linkRenderer or not self.links:
            return

        graph = self.nodeGraph
        # Refresh the held C++ link snapshot only when it is stale (hover change
        # or a flagged link-field mutation), then render straight from it — no
        # per-call LinkData marshalling inside the draw. The C++ side filters out
        # dangling links and partitions regular vs relationship (relationshipOnly);
        # the shader insets whole-prim relationship ends for the arrowhead.
        self._syncLinksIfNeeded(graph)

        projection = self._worldSpaceProjectionMatrix()

        # Use cached colors (updated via settings signal, not every frame)
        hoveredColor = self._cachedLinkHoveredColor
        selectedColor = self._cachedLinkSelectedColor
        config = self._renderConfig()

        for drawSelected, dimming in ((True, 0.3), (False, 1.0)):
            self.linkRenderer.renderLinksFromGraph(
                graph,
                projection,
                self.zoom,
                self.panX,
                self.panY,
                self.width(),
                self.height(),
                config,
                relationshipOnly=False,
                dimming=dimming,
                drawSelected=drawSelected,
                hoveredColor=hoveredColor,
                selectedColor=selectedColor,
                baseColor=list(self._cachedLinkAttributeBaseColor[:3]),
                highlightedColor=list(self._cachedLinkHighlightedColor[:3]),
                cacheKey=0,
            )
            self.linkRenderer.renderLinksFromGraph(
                graph,
                projection,
                self.zoom,
                self.panX,
                self.panY,
                self.width(),
                self.height(),
                config,
                relationshipOnly=True,
                dimming=dimming,
                drawSelected=drawSelected,
                hoveredColor=hoveredColor,
                selectedColor=selectedColor,
                baseColor=list(self._cachedLinkRelationshipBaseColor[:3]),
                highlightedColor=list(self._cachedLinkHighlightedColor[:3]),
                cacheKey=1,
            )

        self._renderRelationshipArrowheads(
            projection,
            hoveredColor,
            selectedColor,
        )

    def _relationshipLinkColor(self, link, hoveredColor, selectedColor):
        if getattr(link, "hovered", False):
            return tuple(hoveredColor)
        if getattr(link, "selected", False):
            return tuple(selectedColor)
        c = self._cachedLinkRelationshipBaseColor
        return (c[0], c[1], c[2], 1.0)

    @staticmethod
    def _isPrimTargetRelationshipLink(link):
        return (
            bool(getattr(link, "is_relationship_link", False))
            and not str(getattr(link, "targetPort", ""))
            and not str(getattr(link, "targetPropertyName", ""))
        )

    @staticmethod
    def _relationshipArrowheadPoints(
        link,
        arrow_length,
        arrow_width,
    ):
        tip_x = float(link.end[0])
        tip_y = float(link.end[1])
        if GraphView._isPrimTargetRelationshipLink(link):
            # Tip sits on the node top (link.end); the body extends up to the
            # arrowhead base. The wire ribbon is inset to that base by the shader.
            prev_x = tip_x
            prev_y = tip_y - arrow_length
            return arrowhead_points(
                tip_x,
                tip_y,
                prev_x,
                prev_y,
                length=arrow_length,
                width=arrow_width,
            )

        curve_points = sampleLinkCurve(
            (link.start[0], link.start[1]),
            (link.end[0], link.end[1]),
            20,
        )
        if len(curve_points) < 2:
            return None
        return arrowhead_points(
            float(curve_points[-1][0]),
            float(curve_points[-1][1]),
            float(curve_points[-2][0]),
            float(curve_points[-2][1]),
            length=arrow_length,
            width=arrow_width,
        )

    def _renderRelationshipArrowheads(
        self,
        projection,
        hoveredColor,
        selectedColor,
    ):
        arrowhead_vertices = []
        arrow_length = max(10.0 / max(self.zoom, 1e-6), 2.0)
        arrow_width = max(8.0 / max(self.zoom, 1e-6), 2.0)
        for link in self.links:
            if not bool(getattr(link, "is_relationship_link", False)):
                continue
            if getattr(link, "isDangling", False):
                continue
            arrow_points = GraphView._relationshipArrowheadPoints(
                link,
                arrow_length,
                arrow_width,
            )
            if arrow_points is None:
                continue
            self._appendTriangleVertices(
                arrowhead_vertices,
                arrow_points,
                0.0,
                self._relationshipLinkColor(link, hoveredColor, selectedColor),
            )
        if arrowhead_vertices:
            self._drawNodeVertices(
                arrowhead_vertices,
                projection,
                0.0,
                (0.0, 0.0, 0.0, 0.0),
            )

    def _renderDanglingIndicators(self, danglingLinks, projection):
        """Render circle indicators at the endpoints of dangling links.

        Each dangling link gets a filled circle at the unpopulated end,
        with a hover highlight when the cursor is over it.
        """
        if not self.shaderLibrary:
            return

        shader = self.shaderLibrary.get("basic")
        if not shader:
            return

        INDICATOR_RADIUS = 6.0
        NUM_SEGMENTS = 16

        # Build vertex data for all indicator circles
        vertices = []
        for link in danglingLinks:
            # The indicator goes at the dangling end (link.end)
            cx, cy = link.end[0], link.end[1]

            # Generate triangle fan vertices for the circle
            for i in range(NUM_SEGMENTS):
                angle0 = 2.0 * math.pi * i / NUM_SEGMENTS
                angle1 = 2.0 * math.pi * (i + 1) / NUM_SEGMENTS
                # Triangle: center, edge0, edge1
                vertices.extend([cx, cy, 0.0])
                vertices.extend(
                    [
                        cx + INDICATOR_RADIUS * math.cos(angle0),
                        cy + INDICATOR_RADIUS * math.sin(angle0),
                        0.0,
                    ]
                )
                vertices.extend(
                    [
                        cx + INDICATOR_RADIUS * math.cos(angle1),
                        cy + INDICATOR_RADIUS * math.sin(angle1),
                        0.0,
                    ]
                )

        if not vertices:
            return

        # Determine color based on hover state
        hoveredIdx = self._danglingIndicatorUnderCursor
        hasHover = hoveredIdx >= 0

        try:
            vbo = GL.glGenBuffers(1)
            vao = GL.glGenVertexArrays(1)

            packedData = (ctypes.c_float * len(vertices))(*vertices)

            GL.glBindVertexArray(vao)
            GL.glBindBuffer(GL.GL_ARRAY_BUFFER, vbo)
            GL.glBufferData(
                GL.GL_ARRAY_BUFFER,
                ctypes.sizeof(packedData),
                packedData,
                GL.GL_STREAM_DRAW,
            )

            # Position attribute (location 0, vec3)
            GL.glEnableVertexAttribArray(0)
            GL.glVertexAttribPointer(
                0, 3, GL.GL_FLOAT, GL.GL_FALSE, 12, ctypes.c_void_p(0)
            )

            shader.use()

            matrixData = projection.data()
            matrix = (ctypes.c_float * 16)(*matrixData)
            projLoc = shader.getUniformLocation("uProjection")
            if projLoc >= 0:
                GL.glUniformMatrix4fv(projLoc, 1, GL.GL_FALSE, matrix)

            GL.glEnable(GL.GL_BLEND)
            GL.glBlendFunc(GL.GL_SRC_ALPHA, GL.GL_ONE_MINUS_SRC_ALPHA)

            vertsPerCircle = NUM_SEGMENTS * 3

            # Draw non-hovered indicators in base color
            colorLoc = shader.getUniformLocation("uColor")
            baseColor = (0.55, 0.57, 0.60, 0.85)
            hoverColor = (0.85, 0.90, 0.95, 1.0)

            for i, link in enumerate(danglingLinks):
                linkIdx = self.links.index(link) if hasHover else -1
                isHovered = hasHover and linkIdx == hoveredIdx
                color = hoverColor if isHovered else baseColor
                if colorLoc >= 0:
                    GL.glUniform4f(colorLoc, *color)
                GL.glDrawArrays(GL.GL_TRIANGLES, i * vertsPerCircle, vertsPerCircle)

        except GLError as e:
            Tf.Warn(f"GL error during dangling indicator rendering: {e}")
        finally:
            GL.glBindVertexArray(0)
            GL.glBindBuffer(GL.GL_ARRAY_BUFFER, 0)
            shader.release()
            if vbo is not None:
                GL.glDeleteBuffers(1, [vbo])
            if vao is not None:
                GL.glDeleteVertexArrays(1, [vao])

    def _findDanglingIndicatorUnderCursor(self, worldPos):
        """Find which dangling link's circle indicator is under the cursor.

        Returns the index into self.links of the dangling link whose indicator
        contains worldPos, or -1 if none.
        """
        INDICATOR_RADIUS = 6.0
        HIT_RADIUS = INDICATOR_RADIUS + 4.0
        hitRadiusSq = HIT_RADIUS * HIT_RADIUS

        for i, link in enumerate(self.links):
            if not link.isDangling:
                continue
            cx, cy = link.end[0], link.end[1]
            dx = worldPos[0] - cx
            dy = worldPos[1] - cy
            if dx * dx + dy * dy <= hitRadiusSq:
                return i
        return -1

    def _findArrowheadUnderCursor(self, worldPos):
        """Index into self.links of the relationship link whose arrowhead is
        under worldPos, or -1. Grabbing the arrowhead (the link's target end)
        starts a reconnect; dropping it on empty space disconnects the link.
        """
        arrow_length = max(10.0 / max(self.zoom, 1e-6), 2.0)
        arrow_width = max(8.0 / max(self.zoom, 1e-6), 2.0)
        hit_radius = max(arrow_length, arrow_width)
        hitRadiusSq = hit_radius * hit_radius
        best = -1
        best_distance = None
        for i, link in enumerate(self.links):
            if not bool(getattr(link, "is_relationship_link", False)):
                continue
            if getattr(link, "isDangling", False):
                continue
            dx = worldPos[0] - float(link.end[0])
            dy = worldPos[1] - float(link.end[1])
            distance = dx * dx + dy * dy
            if distance <= hitRadiusSq and (
                best_distance is None or distance < best_distance
            ):
                best_distance = distance
                best = i
        return best

    def _renderTemporaryLink(self):
        """
        Render the temporary link being dragged with color feedback.

        Color indicates connection validity:
        - Green: Valid target port (correct direction)
        - Red: Invalid target (same direction as source)
        - Active color (configurable): Dragging (no port hovered)
        """
        # Determine base color based on validity
        if self._dragLinkValidTarget:
            # Green for valid target
            baseColor = [0.0, 1.0, 0.0]
        elif self._hoveredPort and self._hoveredPort[2] == self._dragLinkSourceIsOutput:
            # Red for invalid target (same direction)
            baseColor = [1.0, 0.0, 0.0]
        else:
            # Active/dragging color (cached, updated via settings signal)
            if is_relationship_pin(str(self._dragLinkSourcePort)):
                baseColor = list(self._cachedLinkRelationshipBaseColor[:3])
            else:
                activeColor = self._cachedLinkActiveColor
                baseColor = [activeColor[0], activeColor[1], activeColor[2]]

        # The free end is the cursor side; the anchored end is the source pin.
        # Snapshot both as plain float tuples so the inset/restore below cannot
        # be aliased by the C++ LinkData accessors.
        temp_link = self._dragLinkTempLink
        is_relationship = bool(getattr(temp_link, "is_relationship_link", False))
        if self._dragLinkSourceIsOutput:
            # Dragging from output, free end is at 'end' (input side)
            free_end_attr = "end"
            endPos = (float(temp_link.end[0]), float(temp_link.end[1]))
            startPos = (float(temp_link.start[0]), float(temp_link.start[1]))
        else:
            # Dragging from input, free end is at 'start' (output side)
            free_end_attr = "start"
            endPos = (float(temp_link.start[0]), float(temp_link.start[1]))
            startPos = (float(temp_link.end[0]), float(temp_link.end[1]))

        arrow_length = max(12.0 / max(self.zoom, 1e-6), 4.0)
        arrow_width = max(10.0 / max(self.zoom, 1e-6), 3.0)
        # Face the arrowhead down only when snapped to a valid relationship
        # target (dropped on a node's top edge); otherwise it faces left/right.
        use_vertical = is_relationship and self._dragLinkValidTarget

        # Render using existing link renderer.
        # Pass as a single-element list with baseColor to override uniform.
        # When snapped to a whole-prim relationship target the noodle must stop
        # at the arrowhead BASE, not overshoot to its tip (the node top). Committed
        # links get that inset from the shader via the per-instance isPrimTarget
        # flag, but the per-call renderLinks path used for the drag preview always
        # passes isPrimTarget=0 -- so inset the free end here by the same
        # arrow_length the arrowhead uses, then restore it before the arrowhead
        # and hit geometry, which key off the true node top.
        projection = self._worldSpaceProjectionMatrix()
        hoveredColor = self._cachedLinkHoveredColor
        selectedColor = self._cachedLinkSelectedColor
        saved_free_end = None
        if use_vertical:
            saved_free_end = endPos
            setattr(temp_link, free_end_attr, (endPos[0], endPos[1] - arrow_length))
        self.linkRenderer.renderLinks(
            [temp_link],
            projection,
            self.zoom,
            self.panX,
            self.panY,
            self.width(),
            self.height(),
            self._renderConfig(),
            dimming=1.0,
            drawSelected=False,
            hoveredColor=hoveredColor,
            selectedColor=selectedColor,
            baseColor=baseColor,
            cacheKey=2,
        )
        if saved_free_end is not None:
            setattr(temp_link, free_end_attr, saved_free_end)

        projList = list(projection.data())

        if is_relationship:
            # Draw triangle for relationship links
            dx = endPos[0] - startPos[0]

            if use_vertical:
                # Match the persistent arrowhead layout
                # (_relationshipArrowheadPoints): tip sits exactly on the top
                # edge of the target prim and the triangle body extends UPWARD.
                # The previous +arrow_length/2
                # offset pushed the tip into the prim and made the arrowhead
                # read as "under" the node during a reroute drag.
                tip_x = endPos[0]
                tip_y = endPos[1]
                prev_x = tip_x
                prev_y = tip_y - arrow_length
            elif dx > 0:
                tip_x = endPos[0] + arrow_length * 0.5
                tip_y = endPos[1]
                prev_x = tip_x - arrow_length
                prev_y = tip_y
            else:
                tip_x = endPos[0] - arrow_length * 0.5
                tip_y = endPos[1]
                prev_x = tip_x + arrow_length
                prev_y = tip_y

            arrow_points = arrowhead_points(
                tip_x,
                tip_y,
                prev_x,
                prev_y,
                length=arrow_length,
                width=arrow_width,
            )
            if len(arrow_points) < 3:
                return

            self._nodeRenderManager.renderEndpointTriangle(
                arrow_points[0][0],
                arrow_points[0][1],
                arrow_points[1][0],
                arrow_points[1][1],
                arrow_points[2][0],
                arrow_points[2][1],
                0.0,
                projList,
                baseColor + [1.0],
            )
        else:
            # Draw circle for regular links via SDF quad (matches port pin visual)
            renderer = self.defaultRenderer
            radius = renderer.getPortWidth() if renderer else 8.0
            strokeWidth = self._cachedPortRingThickness
            strokeColor = [c * 0.7 for c in baseColor] + [1.0]
            self._nodeRenderManager.renderEndpointCircle(
                endPos[0],
                endPos[1],
                radius,
                0.0,
                projList,
                baseColor + [1.0],
                strokeColor,
                strokeWidth,
            )

    def _renderConfig(self):
        """Cached C++ RenderConfig shared by the node-quad and link paths.

        The config depends only on settings, so it is rebuilt lazily after a
        settings change (invalidated to None in _initCachedSettings), never per
        frame or per geometry rebuild.
        """
        if self._cachedRenderConfig is None:
            from .nodeGraph import build_render_config

            self._cachedRenderConfig = build_render_config()
        return self._cachedRenderConfig

    def paintNodes(self):
        if not self.nodes or not self.shaderLibrary or not self.fontAtlas:
            return

        try:
            projection = self._worldSpaceProjectionMatrix()
            projList = list(projection.data())
            graph = self.nodeGraph

            # Node-quad appearance depends on selection (colors, stroke, and the
            # graffi highlight quad), so a selection-set change must regenerate.
            # Detecting it here covers every selection path (click, marquee,
            # selectAll, clearSelection, prim-tree sync) without flagging each.
            current_selection = frozenset(self._selectedNodes)
            if current_selection != getattr(
                self, "_lastNodeQuadSelection", frozenset()
            ):
                self._nodeRenderManager.markNodeQuadDirty(True)
            self._lastNodeQuadSelection = current_selection

            content_changed = self.textChanged
            needs_rebuild = (
                content_changed or self._nodeRenderManager.needsNodeRebuild()
            )

            # A content change can move a node's rows (pins added/removed, fold
            # toggled, font sizing). Run the single C++ layout producer for every
            # node here — the one chokepoint right before the authoritative
            # snapshot — so each node's display pins, row slots, size, and per-pin
            # centers are recomputed together in one pass from the current
            # content. This closes the stale-cache window for every mutation path
            # (creation, fold, USD-notice/invalidateCache, font change) and is
            # what makes the text/pin/stripe vertical drift structurally
            # impossible. Gated on content_changed so selection-only rebuilds stay
            # cheap (no relayout).
            if content_changed and self.fontAtlas:
                calcWidth = (
                    self.textRenderer.calculateTextWidth
                    if self.textRenderer
                    else lambda t, s: s * 5
                )
                for node in self.nodes.values():
                    # Force the lazy pin getters so an invalidated node reloads
                    # its raw content before we lay it out from that content.
                    _ = node.inputPins
                    _ = node.outputPins
                    self.nodeGraph._calculateNodeSize(node, calcWidth, self.fontAtlas)

            # GraphModel.nodes is the authoritative render snapshot; re-sync from
            # the Python models only when geometry will be regenerated, then
            # render from it. Keep sync and render adjacent — the freshness
            # invariant depends on no frame boundary between them.
            if needs_rebuild:
                graph.syncNodesFromModels(graph.nodes)

            config = self._renderConfig()
            self._nodeRenderManager.renderNodesFromGraph(
                graph,
                config,
                self.fontAtlas,
                projList,
                config.nodeCornerRadius,
                self._cachedNodeSelectedStrokeColor,
                content_changed,
            )

        except Exception as e:
            Tf.Warn(f"Error rendering nodes: {e}")
            import traceback

            traceback.print_exc()

    def paintGroupStickers(self):
        if not self.groupStickers or not self.shaderLibrary:
            return

        try:
            projection = self._worldSpaceProjectionMatrix()
            self._groupStickerRenderer.renderStickers(self.groupStickers, projection)

        except Exception as e:
            Tf.Warn(f"Error rendering group stickers: {e}")
            import traceback

            traceback.print_exc()

    def paintText(self):
        if not self.nodes or not self.shaderLibrary or not self.fontAtlas:
            return

        try:
            projection = self._worldSpaceProjectionMatrix()

            # Render node icons in title bars. The icons are generated + drawn
            # entirely in C++ from the authoritative GraphModel snapshot (no
            # per-frame quad walk across the language boundary, and they ride
            # node drags via the shared transform frame). The C++ cull of
            # off-screen icons is always-on, matching the glyph cull below.
            if (
                hasattr(self, "_cppIconRenderer")
                and self._cppIconRenderer.isInitialized
            ):
                self._cppIconRenderer.renderNodeIcons(
                    self.nodeGraph,
                    projection,
                    self.zoom,
                    self.textChanged,
                    self.panX,
                    self.panY,
                    float(self.width()),
                    float(self.height()),
                    True,
                )

            success = self.textRenderer.renderNodeText(
                self.nodeGraph,
                projection,
                self.zoom,
                self.textChanged,
                self.panX,
                self.panY,
                float(self.width()),
                float(self.height()),
                True,
            )

            # Only clear textChanged flag if rendering succeeded
            if success and self.textChanged:
                self.textChanged = False

        except Exception as e:
            Tf.Warn(f"Error rendering text: {e}")
            import traceback

            traceback.print_exc()

    def paintStatusOverlay(self):
        if not self.nodeIdUnderCursor or not self.shaderLibrary or not self.fontAtlas:
            return

        if self.nodeIdUnderCursor not in self.nodes:
            return

        node = self.nodes[self.nodeIdUnderCursor]
        projection = self._screenSpaceProjectionMatrix()

        self.statusOverlay.render(
            node,
            self.textRenderer,
            NodeVertex,
            self._drawNodeVertices,
            projection,
            self.height(),
            MAX_RENDER_DEPTH,
        )

    def paintHotkeyPopup(self):
        if not self.shaderLibrary or not self.fontAtlas:
            return

        projection = self._screenSpaceProjectionMatrix()
        self.hotkeyPopup.render(
            self.textRenderer,
            NodeVertex,
            self._drawNodeVertices,
            projection,
            self.width(),
            self.height(),
            MAX_RENDER_DEPTH,
        )

    def paintNavigationHotkeyPopup(self):
        if not self.shaderLibrary or not self.fontAtlas:
            return

        projection = self._screenSpaceProjectionMatrix()
        self.navigationHotkeyPopup.render(
            self.textRenderer,
            NodeVertex,
            self._drawNodeVertices,
            projection,
            self.width(),
            self.height(),
            MAX_RENDER_DEPTH,
        )

    def paintMarquee(self):
        if not self.marqueeActive or not self.shaderLibrary:
            return

        startPt = Gf.Vec2d(self.marqueeStart.x(), self.marqueeStart.y())
        currentPt = Gf.Vec2d(self.marqueeCurrent.x(), self.marqueeCurrent.y())
        marqueeBounds = Gf.Range2d.GetUnion(
            Gf.Range2d(startPt, startPt), Gf.Range2d(currentPt, currentPt)
        )

        size = marqueeBounds.GetSize()
        if size[0] < 1.0 or size[1] < 1.0:
            return  # Too small to render

        minPt = marqueeBounds.GetMin()
        maxPt = marqueeBounds.GetMax()
        w, h = size[0], size[1]
        depth = MAX_RENDER_DEPTH - 1.0  # Near the front

        r, g, b, a = 100, 150, 255, 80  # Semi-transparent blue

        marqueeVertexData = [
            NodeVertex(
                minPt[0],
                minPt[1],
                depth,
                0,
                0,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                maxPt[0],
                minPt[1],
                depth,
                w,
                0,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                minPt[0],
                maxPt[1],
                depth,
                0,
                h,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                maxPt[0],
                minPt[1],
                depth,
                w,
                0,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                maxPt[0],
                maxPt[1],
                depth,
                w,
                h,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                minPt[0],
                maxPt[1],
                depth,
                0,
                h,
                w,
                h,
                r,
                g,
                b,
                a,
                2.0,
                0,
                0,
                0,
                255,
                0.0,
            ),
        ]

        try:
            projection = self._worldSpaceProjectionMatrix()
            strokeColor = (150 / 255.0, 200 / 255.0, 1.0, 180 / 255.0)
            # Always force upload for marquee since it changes every frame during drag
            self._drawNodeVertices(
                marqueeVertexData,
                projection,
                0.0,
                strokeColor,
                generation=0,
                selectionChanged=True,
            )

        except Exception as e:
            Tf.Warn(f"Error rendering marquee: {e}")
            import traceback

            traceback.print_exc()

    def _frameNodeBounds(self, bounds, margin=1.0):
        """Frame the view to show the given bounds with an optional margin.

        Args:
            bounds: Gf.Range2d containing the bounds to frame
            margin: Scale factor for the view (1.0 = fit exactly, 1.4 = 40% extra space)
        """
        if bounds.IsEmpty():
            return

        boundsSize = Gf.Vec2d(bounds.GetSize())
        if boundsSize[0] == 0 or boundsSize[1] == 0:
            return

        # Apply margin by expanding the bounds
        boundsCenter = Gf.Vec2d(
            (bounds.GetMin()[0] + bounds.GetMax()[0]) * 0.5,
            (bounds.GetMin()[1] + bounds.GetMax()[1]) * 0.5,
        )
        expandedSize = boundsSize * margin
        bounds = Gf.Range2d(
            boundsCenter - expandedSize * 0.5, boundsCenter + expandedSize * 0.5
        )
        boundsSize = Gf.Vec2d(bounds.GetSize())

        viewSize = Gf.Vec2d(self.width(), self.height())
        scale = Gf.Vec2d(viewSize[0] / boundsSize[0], viewSize[1] / boundsSize[1])
        self.zoom = min(scale[0], scale[1])

        # Center bounds in view
        scaledView = viewSize / self.zoom
        offset = (scaledView - boundsSize) * 0.5
        self.panX = bounds.GetMin()[0] - offset[0]
        self.panY = bounds.GetMin()[1] - offset[1]

    def _nodeHasAuthoredPosition(self, node):
        """Return True if the node's prim already has an authored position.

        ``ui:nodegraph:node:pos`` is the editor's per-node placement metadata.
        When it is authored, the position was set deliberately (a previous
        editor session, an upstream tool, or hand-authoring) and must be
        respected when the node is added to the graph instead of being replaced
        with an auto-layout position.
        """
        prim = node.getUsdPrim()
        if not prim or not prim.IsValid():
            return False
        posAttr = prim.GetAttribute("ui:nodegraph:node:pos")
        return bool(posAttr and posAttr.HasValue())

    def _positionNodeInViewport(self, node):
        """Position a node in the center of the current viewport, avoiding overlap.

        A node whose prim already has an authored ``ui:nodegraph:node:pos`` is
        left where it is — we respect the authored position instead of
        overwriting it with an auto-computed placement. Only nodes that have no
        position of their own are auto-placed (which also avoids leaving them
        stacked at the USD default of (0, 0)).
        """
        if self._nodeHasAuthoredPosition(node):
            # Respect the authored position, but read it once so the cached and
            # C++-side positions sync from the authored USD value without
            # re-authoring it. The noodle renderer reads the C++
            # NodeData.position (via getPortPosition) to place link endpoints;
            # the old code reached that sync through the position setter, so
            # skipping the setter here must not skip the sync, or connected
            # noodles draw to a stale (0, 0) endpoint.
            _ = node.position
            return
        scaledW = self.width() / self.zoom
        scaledH = self.height() / self.zoom
        centerX = self.panX + scaledW * 0.5
        centerY = self.panY + scaledH * 0.5

        nodeW = node.size[0] if node.size[0] > 0 else 200
        nodeH = node.size[1] if node.size[1] > 0 else 100

        # Start at viewport center
        x = centerX - nodeW * 0.5
        y = centerY - nodeH * 0.5

        # Nudge to avoid overlap with existing nodes. The cap scales with the node
        # count so the search can always clear a full row instead of giving up
        # after a fixed number of steps and dropping the node onto others.
        padding = 30.0
        for _ in range(len(self.nodes) + 1):
            overlap = False
            for existingNode in self.nodes.values():
                ex, ey = existingNode.position[0], existingNode.position[1]
                ew = existingNode.size[0] if existingNode.size[0] > 0 else 200
                eh = existingNode.size[1] if existingNode.size[1] > 0 else 100
                if (
                    x < ex + ew + padding
                    and x + nodeW + padding > ex
                    and y < ey + eh + padding
                    and y + nodeH + padding > ey
                ):
                    overlap = True
                    # Place to the right of the overlapping node
                    x = ex + ew + padding
                    break
            if not overlap:
                break

        node.position = Gf.Vec2d(x, y)

    def _addPrimAsNode(self, prim, stage, addedNodeIds):
        """Helper to add a single prim as a node to the graph.

        Args:
            prim: USD prim to add
            stage: USD stage
            addedNodeIds: List to append added node IDs to

        Returns:
            True if node was added, False otherwise
        """
        primPath = prim.GetPath()
        primPathStr = str(primPath)

        # Skip if already in graph
        if primPathStr in self.nodes:
            return False

        # Use NodeFactory to create the node
        node = self._nodeFactory.create_node_from_prim(prim, stage)
        if not node:
            # No library can handle this prim type
            return False

        self._populateNodeLinksFromPrim(node, prim)

        # Calculate node size
        if self.fontAtlas:
            self.nodeGraph._calculateNodeSize(
                node, self.textRenderer.calculateTextWidth, self.fontAtlas
            )

        # Placement is deferred to the caller, which grid-places the whole batch
        # at once (sized to the batch and anchored below existing content).
        self.nodes[node.id] = node
        self._nextZOrder += 1
        node.zOrder = self._nextZOrder
        addedNodeIds.append(node.id)
        return True

    def _getContainerChildNodes(self, containerPrim):
        """Get all child prims of a container/blueprint.

        Args:
            containerPrim: A Container or Blueprint prim

        Returns:
            List of child prims
        """
        return list(containerPrim.GetChildren())

    def addNodesFromPrimTreeSelection(self):
        """Add nodes from the USD prim tree selection to the editor.

        When the user presses 'A', this method:
        1. Gets the currently selected prims from the prim tree
        2. If a Container or Blueprint is selected, adds all its child nodes
        3. Otherwise creates NodeModel instances for valid node prims
        4. Adds them to the graph with their incoming/outgoing links
        5. Links to nodes not in the graph become dangling links
        6. If the graph was empty, frames the view to show the added nodes
        """
        if not self._usdviewApi:
            Tf.Warn("Cannot add nodes: usdviewApi not available")
            return

        try:
            dataModel = self._usdviewApi.dataModel
            if not dataModel or not dataModel.selection:
                return

            stage = self._usdviewApi.stage
            if not stage:
                return

            livePrims = list(dataModel.selection.getPrims())
            # If only the root prim is selected (happens when canvas click resets selection),
            # fall back to the last meaningful prim tree selection. Also fall back when
            # the cached selection contains MORE non-root prims than the live API — this
            # guards against cases where the live selection has been narrowed (e.g. by
            # focus changes) but the user's intent was to add the full multi-selection.
            liveNonRoot = [
                p for p in livePrims if p.IsValid() and p.GetPath().pathString != "/"
            ]
            cachedNonRoot = [
                p
                for p in self._lastPrimTreeSelection
                if p.IsValid() and p.GetPath().pathString != "/"
            ]
            if len(cachedNonRoot) > len(liveNonRoot):
                selectedPrims = cachedNonRoot
            elif liveNonRoot:
                selectedPrims = liveNonRoot
            elif cachedNonRoot:
                selectedPrims = cachedNonRoot
            else:
                self._showPopupMessage("No prims selected")
                return

            # Deduplicate by path — getPrims() can list a prim multiple times
            # (e.g. via instance proxies) and we should only add each once.
            seenPaths = set()
            uniquePrims = []
            for p in selectedPrims:
                pathStr = str(p.GetPath())
                if pathStr == "/" or pathStr in seenPaths:
                    continue
                seenPaths.add(pathStr)
                uniquePrims.append(p)
            selectedPrims = uniquePrims
            if not selectedPrims:
                self._showPopupMessage("No valid node prims selected")
                return

            addedCount = 0
            selectedExistingCount = 0
            addedNodeIds = []

            for prim in selectedPrims:
                primType = prim.GetTypeName()

                # Check if this is a container-type prim (Container or Blueprint)
                # If so, add all its child nodes instead of the container itself
                if primType in ["Container", "Blueprint"]:
                    childNodes = self._getContainerChildNodes(prim)
                    if childNodes:
                        for childPrim in childNodes:
                            childPathStr = str(childPrim.GetPath())
                            if childPathStr in self.nodes:
                                self.nodes[childPathStr].selected = True
                                self._selectedNodes.add(childPathStr)
                                selectedExistingCount += 1
                            elif self._addPrimAsNode(childPrim, stage, addedNodeIds):
                                addedCount += 1
                    else:
                        # Empty container - show message
                        self._showPopupMessage(
                            f"Container '{prim.GetName()}' has no nodes"
                        )
                else:
                    primPathStr = str(prim.GetPath())
                    if primPathStr in self.nodes:
                        # Node already in graph — select it so the user can see it
                        self.nodes[primPathStr].selected = True
                        self._selectedNodes.add(primPathStr)
                        selectedExistingCount += 1
                    elif self._addPrimAsNode(prim, stage, addedNodeIds):
                        addedCount += 1

            if addedCount > 0 or selectedExistingCount > 0:
                if addedCount > 0:
                    # Grid-place the newly added nodes below existing content
                    # (cell sized to the batch's largest node).
                    self._gridPlaceNodes(
                        [self.nodes[nid] for nid in addedNodeIds if nid in self.nodes]
                    )
                    self.linksChanged = True
                    self.textChanged = True
                    self._clearRenderCache()
                    self.textRenderer.resetPositionCaches()
                    self._cppIconRenderer.resetPositionCaches()
                    self._nodeRenderManager.resetNodeQuadCaches()
                    self._nodeTransformFrame.reset()

                # Enable prim tree syncing since we now have nodes with valid USD paths
                self.nodeGraph.syncSelectionToPrimTree = True

                # Frame the added nodes (they're gridded below any existing
                # content, so they may be off-screen) with a comfortable margin.
                if addedNodeIds:
                    # Calculate bounds of added nodes
                    addedBounds = Gf.Range2d()
                    for nodeId in addedNodeIds:
                        if nodeId in self.nodes:
                            node = self.nodes[nodeId]
                            nodeBounds = Gf.Range2d(
                                node.position, node.position + node.size
                            )
                            addedBounds = Gf.Range2d.GetUnion(addedBounds, nodeBounds)

                    # Frame with 1.4 margin (40% extra space around nodes)
                    self._frameNodeBounds(addedBounds, margin=1.4)

                self.update()
                if addedCount > 0 and selectedExistingCount > 0:
                    self._showPopupMessage(
                        f"Added {addedCount} node(s), selected {selectedExistingCount} existing"
                    )
                elif addedCount > 0:
                    self._showPopupMessage(f"Added {addedCount} node(s)")
                else:
                    self._showPopupMessage(
                        f"Selected {selectedExistingCount} existing node(s)"
                    )
            else:
                self._showPopupMessage("No valid nodes to add")

        except Exception as e:
            Tf.Warn(f"Error adding nodes from selection: {e}")
            import traceback

            traceback.print_exc()

    def removeSelectedNodes(self):
        """Remove selected nodes from the editor view.

        When the user presses 'D', this method:
        1. Gets the currently selected nodes in the editor
        2. Removes them from the graph (visual only, not from USD)
        3. Updates links - any links to removed nodes become dangling
        """
        if not self._selectedNodes:
            self._showPopupMessage("No nodes selected")
            return

        removedCount = 0
        nodesToRemove = list(self._selectedNodes)

        for nodeId in nodesToRemove:
            if nodeId in self.nodes:
                del self.nodes[nodeId]
                self._selectedNodes.discard(nodeId)
                self._spatialIndex.removeNode(nodeId)
                removedCount += 1

        if removedCount > 0:
            self.linksChanged = True
            self.textChanged = True
            self._clearRenderCache()
            # Rebuild links immediately so surviving prims' connections to the
            # removed prim become dangling links right away. Without this, the
            # rebuild is deferred to the next paint, and any user input that
            # fires before that paint (e.g. a double-click on a now-stale
            # port) sees inconsistent state.
            self._rebuildLinks()
            self.update()
            self._showPopupMessage(f"Removed {removedCount} node(s)")
        else:
            self._showPopupMessage("No nodes removed")

    def _addNewNodeFast(self, prim, stage):
        """Add a newly created node to the graph with minimal overhead.

        This is optimized for newly created nodes that have NO connections yet.
        It skips the full link rebuild and only does the minimal work needed.

        Args:
            prim: The USD prim for the new node
            stage: The USD stage

        Returns:
            str: The node ID if successful, None otherwise
        """
        primPath = prim.GetPath()
        primPathStr = str(primPath)

        # Skip if already in graph
        if primPathStr in self.nodes:
            return None

        # Use NodeFactory to create the node
        node = self._nodeFactory.create_node_from_prim(prim, stage)
        if not node:
            return None

        # New nodes have no connections, so inputLinks and outputLinks are empty
        # No need to process USD attributes for connections

        # Calculate node size
        if self.fontAtlas:
            self.nodeGraph._calculateNodeSize(
                node, self.textRenderer.calculateTextWidth, self.fontAtlas
            )

        # Add to nodes dictionary
        self.nodes[node.id] = node
        self._nextZOrder += 1
        node.zOrder = self._nextZOrder

        # Index the new node for hit-testing (no links / highlights yet).
        self._updateNodeSpatialBounds({node.id})

        return node.id

    def _createNodeFromHotbox(self, node_data, screen_pos):
        """Create a new node using the appropriate library.

        Args:
            node_data: Dict with 'identifier' and 'library' keys from the hotbox
            screen_pos: Screen position tuple (x, y) where hotbox was opened
        """
        if not self._usdviewApi:
            self._showPopupMessage("Cannot create node: no USD stage")
            return

        from pxr import Gf

        stage = self._usdviewApi.stage
        if not stage:
            self._showPopupMessage("Cannot create node: no USD stage")
            return

        library = node_data["library"]
        identifier = node_data["identifier"]

        world_pos = (
            QtCore.QPointF(screen_pos[0], screen_pos[1]) / self.zoom
        ) + QtCore.QPointF(self.panX, self.panY)

        parent_path = self._resolveParentPath(stage)
        if not parent_path:
            self._showPopupMessage("No Blueprint/Container found to add node to")
            return

        self._noticeHandler.setEnabled(False)
        prim_path = None
        try:
            prim_path = library.create_node(
                stage,
                parent_path,
                identifier,
                Gf.Vec2d(world_pos.x(), world_pos.y()),
            )

            if not prim_path:
                self._showPopupMessage(f"Failed to create {identifier}")
                return

            prim = stage.GetPrimAtPath(prim_path)
            if prim and prim.IsValid():
                self._onNodeCreated(prim, prim_path, stage, identifier)
            else:
                self._showPopupMessage("Failed to get created prim")

        finally:
            self._noticeHandler.setEnabled(True)

        if prim_path:
            prim_name = str(prim_path).split("/")[-1]
            self._showPopupMessage(f"Created {prim_name}")

    def _resolveParentPath(self, stage):
        """Resolve the parent container/blueprint path for node creation."""
        from .primAuthoring import PrimAuthor

        parent_path = self._currentPrimPath if self._currentPrimPath else None
        if not parent_path:
            parent_path = PrimAuthor.get_current_container_path(
                stage, self._usdviewApi.dataModel if self._usdviewApi else None
            )
        return parent_path

    @staticmethod
    def _selectedGraphNodes(gv):
        selectedNodeIds = set(getattr(gv, "_selectedNodes", set()))
        return [
            node
            for nodeId, node in gv.nodes.items()
            if nodeId in selectedNodeIds or getattr(node, "selected", False)
        ]

    @staticmethod
    def _computeBackdropBoundsForNodes(nodes, padding):
        minX = None
        minY = None
        maxX = None
        maxY = None

        for node in nodes:
            position = getattr(node, "position", None)
            size = getattr(node, "size", None)
            if position is None or size is None:
                continue

            x0 = float(position[0])
            y0 = float(position[1])
            x1 = x0 + float(size[0])
            y1 = y0 + float(size[1])

            minX = x0 if minX is None else min(minX, x0)
            minY = y0 if minY is None else min(minY, y0)
            maxX = x1 if maxX is None else max(maxX, x1)
            maxY = y1 if maxY is None else max(maxY, y1)

        if minX is None or minY is None or maxX is None or maxY is None:
            return None

        return (
            Gf.Vec2d(minX - padding, minY - padding),
            Gf.Vec2d((maxX - minX) + padding * 2.0, (maxY - minY) + padding * 2.0),
        )

    @staticmethod
    def _stickerPrimPath(sticker):
        prim = getattr(sticker, "_prim", None)
        if not prim:
            return None
        try:
            return str(prim.GetPath())
        except Exception:
            return None

    @staticmethod
    def _removeBackdropSticker(gv, prim_path):
        pathStr = str(prim_path)
        stickers = gv.groupStickers
        stickers[:] = [
            sticker
            for sticker in stickers
            if GraphView._stickerPrimPath(sticker) != pathStr
        ]

    @staticmethod
    def _addBackdropStickerFromPrim(gv, prim):
        GraphView._removeBackdropSticker(gv, prim.GetPath())
        sticker = GroupSticker.from_prim(prim)
        gv.groupStickers.append(sticker)
        return sticker

    @staticmethod
    def _finishBackdropChange(gv):
        renderer = getattr(gv, "_groupStickerRenderer", None)
        if renderer:
            renderer.markDirty()
        gv.update()

    def _resolveBackdropParentPath(self, stage):
        parentPath = self._currentPrimPath if self._currentPrimPath is not None else "/"
        parentPrim = stage.GetPrimAtPath(parentPath)
        if not parentPrim or not parentPrim.IsValid():
            return None
        return parentPath

    def _stageForBackdropCreation(self):
        stage = self.nodeGraph.getStage() if self.nodeGraph else None
        if stage is None and self._usdviewApi:
            stage = self._usdviewApi.stage
        return stage

    def _authorBackdropPrim(self, stage, parentPath, position, size):
        from .primAuthoring import PrimAuthor

        noticeHandler = getattr(self, "_noticeHandler", None)
        try:
            if noticeHandler:
                noticeHandler.setEnabled(False)
            return PrimAuthor.create_backdrop_prim(
                stage,
                parentPath,
                position,
                size,
                description="Backdrop",
                color=Gf.Vec3f(0.196, 0.196, 0.196),
            )
        finally:
            if noticeHandler:
                noticeHandler.setEnabled(True)

    def _pushCreateBackdropUndo(self, stage, primPath):
        gv = self
        createdPath = str(primPath)

        def redo():
            primToActivate = stage.GetPrimAtPath(createdPath)
            if primToActivate:
                primToActivate.SetActive(True)
                if primToActivate.IsValid():
                    GraphView._addBackdropStickerFromPrim(gv, primToActivate)
            GraphView._finishBackdropChange(gv)

        def undo():
            primToDeactivate = stage.GetPrimAtPath(createdPath)
            if primToDeactivate:
                primToDeactivate.SetActive(False)
            GraphView._removeBackdropSticker(gv, createdPath)
            GraphView._finishBackdropChange(gv)

        _push_undo_command("Create Backdrop", redo, undo)

    def createBackdropFromSelection(self):
        selectedNodes = GraphView._selectedGraphNodes(self)
        if not selectedNodes:
            self._showPopupMessage("No nodes selected")
            return

        stage = GraphView._stageForBackdropCreation(self)
        if not stage:
            self._showPopupMessage("Cannot create backdrop: no USD stage")
            return

        parentPath = self._resolveBackdropParentPath(stage)
        if not parentPath:
            self._showPopupMessage("No graph root found for backdrop")
            return

        bounds = GraphView._computeBackdropBoundsForNodes(
            selectedNodes, GraphView._BACKDROP_PADDING
        )
        if bounds is None:
            self._showPopupMessage("Cannot create backdrop: invalid selection bounds")
            return

        position, size = bounds
        primPath = GraphView._authorBackdropPrim(
            self, stage, parentPath, position, size
        )
        if not primPath:
            self._showPopupMessage("Failed to create backdrop")
            return

        prim = stage.GetPrimAtPath(primPath)
        if not prim or not prim.IsValid():
            self._showPopupMessage("Failed to load created backdrop")
            return

        GraphView._addBackdropStickerFromPrim(self, prim)
        GraphView._pushCreateBackdropUndo(self, stage, primPath)
        GraphView._finishBackdropChange(self)
        self._showPopupMessage(f"Created backdrop for {len(selectedNodes)} node(s)")

    def _onNodeCreated(self, prim, prim_path, stage, identifier):
        """Handle post-creation setup: add to graph, select, push undo."""
        newNodeId = self._addNewNodeFast(prim, stage)
        if not newNodeId:
            self._showPopupMessage(f"Failed to add {identifier} to graph")
            return

        self.textChanged = True
        self._nodeRenderManager.invalidateAll()
        self.nodeGraph.syncSelectionToPrimTree = True

        self.clearSelection()
        if newNodeId in self.nodes:
            self.nodes[newNodeId].selected = True
            self._selectedNodes.add(newNodeId)

        gv = self
        created_path = str(prim_path)
        _push_undo_command(
            f"Create Node: {identifier}",
            _make_prim_activation_undo(gv, [created_path], activate=True),
            _make_prim_activation_undo(gv, [created_path], activate=False),
        )

        self.update()
