#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
NoodlesSettings - Configuration settings for the usdNoodles node graph editor.

This module provides a StateSource-based settings system that integrates with
usdviewq's configuration framework. Settings are organized into logical categories
and automatically persisted to disk.

Usage:
    # Initialize (typically done in GraphView.__init__)
    settings = NoodlesSettingsDataModel(parentStateSource)

    # Access settings
    fontSize = settings.nodeTitleFontSize
    settings.nodeTitleFontSize = 28.0

    # Query by category
    nodeSettings = settings.getSettingsByCategory("nodes")
"""

from pxr.Usdviewq.qt import QtCore, QtGui
from pxr.Usdviewq.settings import StateSource


def _render_config_defaults():
    """Read defaults from the C++ RenderConfig struct (single source of truth).
    Returns None if the binding is unavailable (e.g. during early import)."""
    try:
        from pxr.UsdNoodles.core import RenderConfig

        return RenderConfig()
    except ImportError:
        return None


_CPP_DEFAULTS = _render_config_defaults()


def _d(attr, fallback):
    """Get a default from C++ RenderConfig, falling back to a hardcoded value."""
    if _CPP_DEFAULTS is not None:
        return getattr(_CPP_DEFAULTS, attr, fallback)
    return fallback


def _validateKeySequence(value):
    """
    Validate that a string can be converted to a valid QKeySequence.

    Args:
        value: String representation of a key sequence (e.g., "Ctrl+S", "F", "Shift+Alt+K")

    Returns:
        True if the value is a valid key sequence or empty string, False otherwise
    """
    if not isinstance(value, str):
        return False
    if not value:  # Allow empty string (no shortcut)
        return True

    seq = QtGui.QKeySequence(value)
    return not seq.isEmpty()


class NoodlesSettingsDataModel(StateSource, QtCore.QObject):
    """
    Main settings model using usdviewq's StateSource pattern.
    Organizes all configurable parameters into logical categories.
    Uses a data-driven approach for maintainability.

    Settings are automatically persisted to ~/.usdview/state.json and can be
    overridden by USD-embedded configuration.
    """

    # Table-driven settings definition
    # Format: dict of {category: [(name, default, validator, description), ...]}
    SETTINGS_SCHEMA = {
        "fonts": [
            # Global font settings. Future: support for multiple font face definitions
            # For now, font sizes are in their respective categories (nodes, statusOverlay)
        ],
        "nodes": [
            # Node chrome sizes, in graph world-space units (1 unit = 1 px at
            # the default 1.0 camera zoom).
            (
                "nodeTitleFontSize",
                _d("nodeTitleFontSize", 48.0),
                lambda v: v > 0,
                "Font size for node titles",
            ),
            (
                "nodePinFontSize",
                _d("nodePinFontSize", 36.0),
                lambda v: v > 0,
                "Font size for node pins",
            ),
            (
                "nodePinTypeFontSize",
                _d("nodePinTypeFontSize", 26.0),
                lambda v: v > 0,
                "Font size for pin type labels",
            ),
            (
                "showPinTypeLabels",
                True,
                lambda v: isinstance(v, bool),
                "Show the '(type)' label under each pin name (off shrinks rows)",
            ),
            (
                "nodeMarginH",
                _d("nodeMarginH", 30.0),
                lambda v: v >= 0,
                "Horizontal margin inside nodes",
            ),
            (
                "nodeMarginV",
                _d("nodeMarginV", 20.0),
                lambda v: v >= 0,
                "Vertical margin inside nodes",
            ),
            (
                "nodePortWidth",
                _d("nodePortWidth", 40.0),
                lambda v: v > 0,
                "Width of port circle indicators",
            ),
            (
                "nodePortSpacing",
                _d("nodePortSpacing", 12.0),
                lambda v: v >= 0,
                "Vertical spacing between ports",
            ),
            (
                "portRingThickness",
                2.0,
                lambda v: v >= 0,
                "Ring thickness at port outer edge (0 to disable)",
            ),
            (
                "portConnectedFillColor",
                [0.7, 0.8, 0.0, 1.0],
                lambda v: len(v) == 4,
                "Port fill color when connected (RGBA 0-1)",
            ),
            (
                "portDisconnectedFillColor",
                [0.235, 0.235, 0.235, 1.0],
                lambda v: len(v) == 4,
                "Port fill color when disconnected (RGBA 0-1)",
            ),
            (
                "portConnectedRingColor",
                [1.0, 1.0, 1.0, 0.0],
                lambda v: len(v) == 4,
                "Port ring color when connected (RGBA 0-1)",
            ),
            (
                "portDisconnectedRingColor",
                [0.7, 0.8, 0.0, 1.0],
                lambda v: len(v) == 4,
                "Port ring color when disconnected (RGBA 0-1)",
            ),
            (
                "nodeCornerRadius",
                _d("nodeCornerRadius", 14.0),
                lambda v: v >= 0,
                "Corner radius for node rectangles",
            ),
            (
                "nodeBgHigh",
                _d("nodeBgHigh", 90),
                lambda v: 0 <= v <= 255,
                "Node background high value",
            ),
            (
                "nodeBgLow",
                _d("nodeBgLow", 86),
                lambda v: 0 <= v <= 255,
                "Node background low value",
            ),
            (
                "nodeBgAlpha",
                _d("nodeBgAlpha", 245),
                lambda v: 0 <= v <= 255,
                "Node background alpha",
            ),
            (
                "nodeShadowFactor",
                _d("nodeShadowFactor", 0.9),
                lambda v: 0 <= v <= 1,
                "Node shadow intensity factor",
            ),
            (
                "nodeTypeSaturation",
                _d("nodeTypeSaturation", 0.35),
                lambda v: 0 <= v <= 1,
                "Node type color saturation",
            ),
            (
                "nodeTypeBrightness",
                _d("nodeTypeBrightness", 0.5),
                lambda v: 0 <= v <= 1,
                "Node type color brightness",
            ),
            (
                "selectedNodeStrokeColor",
                [1.0, 1.0, 0.117, 1.0],
                lambda v: len(v) == 4,
                "Selected node stroke color (RGBA)",
            ),
            (
                "selectedNodeStrokeWidth",
                _d("selectedNodeStrokeWidth", 10.0),
                lambda v: v > 0,
                "Selected node stroke width",
            ),
            (
                "nodeRendererType",
                "default",
                lambda v: v in ["default", "graffi"],
                "Node renderer type (default or graffi)",
            ),
        ],
        "canvas": [
            (
                "backgroundClearColor",
                [0.03, 0.04, 0.05, 1.0],
                lambda v: len(v) == 4,
                "Background clear color (RGBA)",
            ),
            (
                "marqueeFillColor",
                [100, 150, 255, 80],
                lambda v: len(v) == 4,
                "Marquee selection fill color (RGBA)",
            ),
            (
                "marqueeStrokeColor",
                [150 / 255, 200 / 255, 1.0, 180 / 255],
                lambda v: len(v) == 4,
                "Marquee selection stroke color (RGBA)",
            ),
        ],
        "statusOverlay": [
            (
                "statusOverlayNameSize",
                16.0,
                lambda v: v > 0,
                "Font size for status overlay name",
            ),
            (
                "statusOverlayTypeSize",
                12.0,
                lambda v: v > 0,
                "Font size for status overlay type",
            ),
        ],
        "minimap": [
            (
                "showMinimap",
                True,
                lambda v: isinstance(v, bool),
                "Show the minimap overlay",
            ),
            (
                "minimapBgColor",
                [20, 20, 25, 160],
                lambda v: len(v) == 4,
                "Minimap background color (RGBA)",
            ),
            (
                "minimapBgStroke",
                [0.5, 0.5, 0.5, 0.5],
                lambda v: len(v) == 4,
                "Minimap background stroke (RGBA)",
            ),
            (
                "minimapViewportNormal",
                [80, 30, 30, 120],
                lambda v: len(v) == 4,
                "Viewport indicator color (RGBA)",
            ),
            (
                "minimapViewportHover",
                [110, 45, 45, 150],
                lambda v: len(v) == 4,
                "Viewport hover color (RGBA)",
            ),
            (
                "minimapViewportDrag",
                [140, 60, 60, 180],
                lambda v: len(v) == 4,
                "Viewport drag color (RGBA)",
            ),
            (
                "minimapMaxHeight",
                120.0,
                lambda v: v > 0,
                "Maximum minimap height in pixels",
            ),
            (
                "minimapMinHeight",
                50.0,
                lambda v: v > 0,
                "Minimum minimap height in pixels",
            ),
            (
                "minimapMargin",
                10.0,
                lambda v: v >= 0,
                "Minimap margin from window edge",
            ),
            ("minimapPadding", 4.0, lambda v: v >= 0, "Minimap internal padding"),
            (
                "minimapMaxHeightPercent",
                0.15,
                lambda v: 0 < v <= 1,
                "Max minimap height as % of window",
            ),
            (
                "minimapMaxWidthPercent",
                0.5,
                lambda v: 0 < v <= 1,
                "Max minimap width as % of window",
            ),
            (
                "minimapMinWindowWidth",
                500,
                lambda v: v > 0,
                "Minimum window width for minimap",
            ),
            (
                "minimapMinWindowHeight",
                350,
                lambda v: v > 0,
                "Minimum window height for minimap",
            ),
        ],
        "links": [
            (
                "linkLineWidth",
                _d("linkLineWidth", 12.0),
                lambda v: v > 0,
                "Link line width in pixels",
            ),
            (
                "linkSampleRate",
                _d("linkSampleRate", 6.0),
                lambda v: v > 0,
                "Link curve sample rate",
            ),
            (
                "linkEdgeDimmingStart",
                _d("linkEdgeDimmingStart", 0.7),
                lambda v: 0 <= v <= 1,
                "Screen edge dimming start threshold",
            ),
            (
                "linkEdgeDimmingEnd",
                _d("linkEdgeDimmingEnd", 0.5),
                lambda v: 0 <= v <= 1,
                "Screen edge dimming end threshold",
            ),
            (
                "linkCutoffAlpha",
                _d("linkCutoffAlpha", 0.93),
                lambda v: 0 <= v <= 1,
                "Maximum alpha for link cutoff",
            ),
            (
                "linkHoveredColor",
                [1.0, 1.0, 0.0, 1.0],
                lambda v: len(v) == 4,
                "Link color when hovered (RGB 0-1, matches port hover yellow)",
            ),
            (
                "linkSelectedColor",
                [1.0, 1.0, 0.3, 1.0],
                lambda v: len(v) == 4,
                "Link color when selected (RGB 0-1)",
            ),
            (
                "linkActiveColor",
                [0.0, 0.8, 1.0, 1.0],
                lambda v: len(v) == 4,
                "Link color when dragging (RGB 0-1, cyan)",
            ),
            (
                "linkHighlightedColor",
                [0.31, 0.78, 0.47, 1.0],
                lambda v: len(v) == 4,
                "Link color for active-flow highlighting (RGB 0-1, Presto green)",
            ),
            (
                "linkAttributeBaseColor",
                [0.31, 0.55, 0.86, 1.0],
                lambda v: len(v) == 4,
                "Base color for attribute connections (RGB 0-1, Presto blue)",
            ),
            (
                "linkRelationshipBaseColor",
                [0.78, 0.24, 0.24, 1.0],
                lambda v: len(v) == 4,
                "Base color for relationship connections (RGB 0-1, Presto red)",
            ),
            (
                "nodeSelectedStrokeColor",
                [1.0, 1.0, 0.3, 1.0],
                lambda v: len(v) == 4,
                "Node inner stroke color when selected (RGBA 0-1)",
            ),
        ],
        "behavior": [
            (
                "panAnimationSpeed",
                18.0,
                lambda v: v > 0,
                "Pan animation speed (higher = faster)",
            ),
            (
                "zoomFactorPerNotch",
                1.15,
                lambda v: v > 1.0,
                "Zoom factor per mouse wheel notch",
            ),
            ("minZoom", 0.001, lambda v: v > 0, "Minimum zoom level"),
            ("maxZoom", 100.0, lambda v: v > 0, "Maximum zoom level"),
            (
                "portHitRadiusMultiplier",
                2.75,
                lambda v: v > 0,
                "Multiplier on port radius for circular hit detection",
            ),
            (
                "frameInPrimTreeOnSelect",
                True,
                lambda v: isinstance(v, bool),
                "Frame selection in prim tree",
            ),
            (
                "hideUiNamespacePins",
                True,
                lambda v: isinstance(v, bool),
                "Hide ui: namespace attributes (editor metadata) from node pins",
            ),
            ("linkDimmingSpeed", 1.0, lambda v: v > 0, "Link dimming animation speed"),
        ],
        "performance": [
            (
                "msaaSampleCount",
                4,
                lambda v: v in [0, 2, 4, 8, 16],
                "MSAA sample count",
            ),
            (
                "linkLodThresholds",
                [[80, 160], [40, 80], [20, 40], [10, 20], [5, 10]],
                lambda v: isinstance(v, list),
                "Link LOD distance thresholds",
            ),
            (
                "linkCurveSamplesHit",
                30,
                lambda v: v > 0,
                "Link curve samples for hit detection",
            ),
            (
                "linkCurveSamplesMarquee",
                20,
                lambda v: v > 0,
                "Link curve samples for marquee",
            ),
            (
                "textCutoffStart",
                5.0,
                lambda v: v > 0,
                "Text rendering cutoff start zoom",
            ),
            ("textCutoffEnd", 2.0, lambda v: v > 0, "Text rendering cutoff end zoom"),
            (
                "enableRenderProfiling",
                False,
                lambda v: isinstance(v, bool),
                "Enable render loop profiling",
            ),
            (
                "profilingPrintInterval",
                60,
                lambda v: v > 0,
                "Frames between profiling stat prints",
            ),
        ],
        "layout": [
            (
                "frameScenePadding",
                0.2,
                lambda v: 0 <= v <= 1,
                "Padding around graph when framing",
            ),
            (
                "selectionExpandFactor",
                3.0,
                lambda v: v >= 1,
                "Selection bounds expand factor",
            ),
            (
                "connectionPadding",
                0.1,
                lambda v: 0 <= v <= 1,
                "Connection padding factor",
            ),
        ],
        "libraries": [
            (
                "enabledLibraries",
                ["USD Prims"],
                lambda v: isinstance(v, list),
                "List of enabled node library display names",
            ),
        ],
        "shortcuts": [
            # View shortcuts
            (
                "shortcutToggleLinks",
                "Ctrl+Shift+L",
                _validateKeySequence,
                "Toggle links visibility",
            ),
            (
                "shortcutToggleNodes",
                "Ctrl+Shift+N",
                _validateKeySequence,
                "Toggle nodes visibility",
            ),
            (
                "shortcutToggleText",
                "Ctrl+Shift+T",
                _validateKeySequence,
                "Toggle text visibility",
            ),
            (
                "shortcutToggleLinkDimming",
                "G",
                _validateKeySequence,
                "Toggle link dimming",
            ),
            # Selection shortcuts
            ("shortcutSelectAll", "Ctrl+A", _validateKeySequence, "Select all nodes"),
            (
                "shortcutClearSelection",
                "Escape",
                _validateKeySequence,
                "Clear selection",
            ),
            # Navigation shortcuts
            (
                "shortcutFrameAll",
                "Ctrl+Shift+F",
                _validateKeySequence,
                "Frame all nodes",
            ),
            (
                "shortcutFrameSelection",
                "F",
                _validateKeySequence,
                "Frame selected nodes",
            ),
            (
                "shortcutFrameWithConnections",
                "Shift+F",
                _validateKeySequence,
                "Frame selection with connections",
            ),
            (
                "shortcutNavigateBackward",
                "[",
                _validateKeySequence,
                "Navigate backward (to source)",
            ),
            (
                "shortcutNavigateForward",
                "]",
                _validateKeySequence,
                "Navigate forward (to target)",
            ),
            # Editing shortcuts
            (
                "shortcutRemoveFromGraph",
                "D",
                _validateKeySequence,
                "Remove selected nodes from graph",
            ),
            (
                "shortcutAddFromPrimTree",
                "A",
                _validateKeySequence,
                "Add nodes from prim tree selection",
            ),
            # Window shortcuts (global)
            (
                "shortcutNewEditor",
                "Shift+N",
                _validateKeySequence,
                "Open new Noodles editor",
            ),
            ("shortcutShowEditor", "N", _validateKeySequence, "Show Noodles editor"),
        ],
    }

    # Qt signal for reactive updates
    signalSettingChanged = QtCore.Signal()

    def __init__(self, parent):
        """
        Initialize the settings data model.

        Args:
            parent: Parent StateSource object (typically the main Settings instance)
        """
        StateSource.__init__(self, parent, "noodles")
        QtCore.QObject.__init__(self)

        # Upgrade settings saved by older versions before reading them.
        self._migrateLegacyGlobalNodeScale()

        # Create stateProperty instances from schema
        self._propertyCache = {}
        self._settingsByCategory = {}

        for category, settings in self.SETTINGS_SCHEMA.items():
            self._settingsByCategory[category] = []
            for name, default, validator, description in settings:
                # Create state property with validation
                propType = type(default)
                value = self.stateProperty(
                    name, default, propType=propType, validator=validator
                )

                # Cache the value with private name
                privateName = f"_{name}"
                setattr(self, privateName, value)
                self._propertyCache[name] = (privateName, category, description)
                self._settingsByCategory[category].append(name)

        # Create dynamic property accessors
        self._createPropertyAccessors()

    # Node-size settings folded by a saved ``globalNodeScale`` during migration.
    _SCALE_FOLDED_SIZE_KEYS = (
        "nodeTitleFontSize",
        "nodePinFontSize",
        "nodePinTypeFontSize",
        "nodeMarginH",
        "nodeMarginV",
        "nodePortSpacing",
        "nodePortWidth",
        "nodeCornerRadius",
        "selectedNodeStrokeWidth",
    )

    def _migrateLegacyGlobalNodeScale(self):
        """Fold a saved ``globalNodeScale`` into the per-element node sizes.

        If the saved settings contain a ``globalNodeScale`` value, multiply each
        saved node-size value by it (preserving on-screen size) and drop the
        key. Must run before any ``stateProperty`` is created so the folded
        values are the ones loaded into the properties.
        """
        state = self._getState()
        if not state or "globalNodeScale" not in state:
            return
        scale = state.pop("globalNodeScale")
        if not isinstance(scale, (int, float)) or scale <= 0:
            return
        for key in self._SCALE_FOLDED_SIZE_KEYS:
            value = state.get(key)
            if isinstance(value, (int, float)):
                state[key] = value * scale

    def _createPropertyAccessors(self):
        """
        Dynamically create property getters/setters for all settings.

        This creates Python properties for each setting that emit signals
        when modified, enabling reactive updates in the UI.
        """
        for name, (privateName, _category, _description) in self._propertyCache.items():
            # Use closure to capture current name/privateName
            def make_property(pname):
                def getter(self):
                    return getattr(self, pname)

                def setter(self, value):
                    setattr(self, pname, value)
                    self.signalSettingChanged.emit()

                return property(getter, setter)

            # Add property to class
            setattr(self.__class__, name, make_property(privateName))

    def onSaveState(self, state):
        """
        Marshal all properties to dict for JSON serialization.

        Called by the StateSource framework when saving settings to disk.

        Args:
            state: Dictionary to populate with current settings
        """
        for name, (privateName, _category, _description) in self._propertyCache.items():
            value = getattr(self, privateName)
            # Convert tuples to lists for JSON compatibility
            if isinstance(value, tuple):
                value = list(value)
            state[name] = value

    def getSettingsByCategory(self, category):
        """
        Get all settings in a specific category.

        Args:
            category: Category name (e.g., "nodes", "links", "behavior")

        Returns:
            List of tuples: [(name, current_value, description), ...]
        """
        if category not in self._settingsByCategory:
            return []

        # Get description from schema
        result = []
        for name, _default, _validator, description in self.SETTINGS_SCHEMA.get(
            category, []
        ):
            result.append((name, getattr(self, name), description))
        return result

    def getCategories(self):
        """
        Get list of all setting categories.

        Returns:
            List of category names, sorted alphabetically
        """
        return sorted(self.SETTINGS_SCHEMA.keys())

    def getSettingInfo(self, name):
        """
        Get metadata about a specific setting.

        Args:
            name: Setting name (e.g., "nodeTitleFontSize")

        Returns:
            Dict with keys: name, default, category, description, type
            or None if setting not found
        """
        if name not in self._propertyCache:
            return None

        privateName, category, description = self._propertyCache[name]

        # Find default value from schema
        for n, default, _validator, _desc in self.SETTINGS_SCHEMA.get(category, []):
            if n == name:
                return {
                    "name": name,
                    "default": default,
                    "category": category,
                    "description": description,
                    "type": type(default).__name__,
                }
        return None


class NoodlesUIState(StateSource):
    """
    UI layout state (similar to UIStateProxySource in usdviewq).
    Stores window layout, panel visibility, zoom/pan state, etc.

    This state is automatically saved and restored across sessions.
    """

    def __init__(self, graphView, parent):
        """
        Initialize UI state tracking.

        Args:
            graphView: GraphView instance to track state for
            parent: Parent StateSource object
        """
        StateSource.__init__(self, parent, "ui")
        self._graphView = graphView

        # Initial zoom/pan state
        self._zoom = self.stateProperty("zoom", default=1.0)
        self._panX = self.stateProperty("panX", default=0.0)
        self._panY = self.stateProperty("panY", default=0.0)

        # Visibility toggles
        self._showMinimap = self.stateProperty("showMinimap", default=True)
        self._showStatusOverlay = self.stateProperty("showStatusOverlay", default=True)
        self._showGrid = self.stateProperty("showGrid", default=False)  # Future feature

    def onSaveState(self, state):
        """
        Capture current UI state from GraphView for serialization.

        Args:
            state: Dictionary to populate with current UI state
        """
        # Capture current state from GraphView
        state["zoom"] = self._graphView.zoom
        state["panX"] = self._graphView.panX
        state["panY"] = self._graphView.panY
        state["showMinimap"] = self._showMinimap
        state["showStatusOverlay"] = self._showStatusOverlay
        state["showGrid"] = self._showGrid

    def restoreToGraphView(self):
        """
        Apply saved UI state back to GraphView.

        This should be called after loading settings to restore the
        previous session's view state.
        """
        self._graphView.zoom = self._zoom
        self._graphView.panX = self._panX
        self._graphView.panY = self._panY
        # Visibility flags can be queried by GraphView as needed

    @property
    def showMinimap(self):
        """Whether the minimap is visible."""
        return self._showMinimap

    @showMinimap.setter
    def showMinimap(self, value):
        self._showMinimap = value

    @property
    def showStatusOverlay(self):
        """Whether the status overlay is visible."""
        return self._showStatusOverlay

    @showStatusOverlay.setter
    def showStatusOverlay(self, value):
        self._showStatusOverlay = value

    @property
    def showGrid(self):
        """Whether the grid is visible (future feature)."""
        return self._showGrid

    @showGrid.setter
    def showGrid(self, value):
        self._showGrid = value
