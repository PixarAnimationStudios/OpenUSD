#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles.core import FontMetrics, GraphModel, RenderConfig
from pxr import Tf


try:
    from pxr.UsdNoodles.render import DefaultNodeRenderer, GraffiNodeRenderer
except ImportError:
    DefaultNodeRenderer = None
    GraffiNodeRenderer = None

# Module-level registry for renderer types
_RENDERER_REGISTRY = None


def _get_renderer_registry():
    from pxr.UsdNoodles.render import DefaultNodeRenderer, GraffiNodeRenderer

    global _RENDERER_REGISTRY
    if _RENDERER_REGISTRY is None:
        _RENDERER_REGISTRY = {
            "default": DefaultNodeRenderer,
            "graffi": GraffiNodeRenderer,
        }
    return _RENDERER_REGISTRY


def createNodeRenderer(uiStyle=None):
    """
    Create a node renderer based on uiStyle or global config.

    Args:
        uiStyle: Optional UI style identifier ("default", "graffi", etc.)
                If None, uses the global config setting.

    Returns:
        GraphNodeRenderer instance
    """
    if uiStyle is None:
        # Fall back to global config
        from .noodlesConfig import NoodlesConfig

        uiStyle = NoodlesConfig.get("nodeRendererType", "default")

    registry = _get_renderer_registry()
    renderer_class = registry.get(uiStyle)

    if renderer_class is None:
        from pxr.UsdNoodles.render import DefaultNodeRenderer

        Tf.Warn(f"Unknown uiStyle '{uiStyle}', using default renderer")
        renderer_class = DefaultNodeRenderer

    if renderer_class is None:
        raise ImportError(
            f"Node renderer for style '{uiStyle}' is not available "
            "(Qt/render bindings not loaded)"
        )

    # Build the renderer with the NoodlesConfig-derived RenderConfig so what it
    # draws (ports, stripes, backgrounds, selection stroke, collapse hit-test)
    # honors the same config as node sizing and text layout. On a default config
    # this equals RenderConfig()'s defaults, so appearance is unchanged; only
    # settings overrides now take effect.
    config = build_render_config()

    # Decide config-aware vs. no-arg up front from the constructor signature so
    # we don't swallow a TypeError raised *inside* a config-aware __init__ as
    # "legacy renderer, fall back to no-arg" — that would silently mask real
    # bugs as default-appearance rendering. boost.python classes don't always
    # expose an inspectable signature; for them, fall through to a narrow
    # try/except gated on a TypeError raised before the body runs.
    import inspect

    try:
        params = inspect.signature(renderer_class).parameters
        accepts_config = any(
            p.kind
            in (
                inspect.Parameter.POSITIONAL_ONLY,
                inspect.Parameter.POSITIONAL_OR_KEYWORD,
                inspect.Parameter.VAR_POSITIONAL,
            )
            for p in params.values()
        )
    except (TypeError, ValueError):
        accepts_config = None  # signature not introspectable (boost.python)

    if accepts_config is True:
        return renderer_class(config)
    if accepts_config is False:
        return renderer_class()
    try:
        return renderer_class(config)
    except TypeError:
        # boost.python raises ArgumentError (a TypeError subclass) when no
        # overload accepts a RenderConfig; legacy plugin renderers may also
        # reject the extra arg. Fall back to the no-arg constructor.
        return renderer_class()


def registerNodeRenderer(uiStyle, renderer_class):
    """
    Register a new renderer type.

    This allows adding new renderer styles dynamically without modifying
    the core code. Useful for plugins or custom node renderers.

    Args:
        uiStyle: UI style identifier (e.g., "compact", "minimal")
        renderer_class: GraphNodeRenderer subclass
    """
    _get_renderer_registry()[uiStyle] = renderer_class


def getRegisteredRendererStyles():
    """
    Get all registered renderer styles.

    Returns:
        List[str]: Available UI style identifiers
    """
    return list(_get_renderer_registry().keys())


def describeNonPersistentEditTarget(layer, stage=None):
    """Return an informational warning string if *layer* won't persist edits.

    Anonymous layers and the stage's session layer are not written to disk
    by a normal save, so opinions authored to them will be lost on reload.
    Authoring is still permitted — this is a heads-up only.

    Returns None when the layer would persist normally.
    """
    if layer is None:
        return None
    if stage is not None and layer == stage.GetSessionLayer():
        return "Authoring to the session layer, changes will not be preserved on save"
    if getattr(layer, "anonymous", False):
        return "Authoring to an anonymous layer, changes may not be preserved on save"
    return None


# Process-wide subscribers for non-persistent edit target warnings.
# The UI layer (GraphView) subscribes its _showPopupMessage at construction
# so any authoring site can call warn_if_non_persistent_edit_target() without
# needing a back-reference to the UI.
_EDIT_TARGET_WARNING_SUBSCRIBERS = []


def subscribe_edit_target_warning(callback):
    """Register ``callback(message)`` to receive non-persistent edit target warnings."""
    _EDIT_TARGET_WARNING_SUBSCRIBERS.append(callback)


def unsubscribe_edit_target_warning(callback):
    if callback in _EDIT_TARGET_WARNING_SUBSCRIBERS:
        _EDIT_TARGET_WARNING_SUBSCRIBERS.remove(callback)


def warn_if_non_persistent_edit_target(stage):
    """Notify subscribers if *stage*'s current edit target won't persist on save.

    Returns the warning message (or None) so callers can also act locally.
    Falls back to ``Tf.Warn`` when no subscribers are registered, matching the
    behavior of ``setEditTargetLayer`` for headless contexts.
    """
    if stage is None:
        return None
    layer = stage.GetEditTarget().GetLayer()
    message = describeNonPersistentEditTarget(layer, stage)
    if not message:
        return None
    if _EDIT_TARGET_WARNING_SUBSCRIBERS:
        for cb in list(_EDIT_TARGET_WARNING_SUBSCRIBERS):
            try:
                cb(message)
            except Exception as e:
                Tf.Warn(f"edit target warning subscriber failed: {e}")
    else:
        Tf.Warn(message)
    return message


def build_render_config():
    """Build a C++ ``RenderConfig`` from the current ``NoodlesConfig`` values.

    This is the single bridge from the user-facing ``NoodlesConfig`` (per-file
    overrides + disk prefs + defaults) to the C++ ``RenderConfig``, and it is
    shared by every part of the render path so they never desync:

    - node *sizing* (``_calculateNodeSize`` -> ``GraphModel.calculateNodeSize``),
    - the node *renderer* (``createNodeRenderer`` -> ports, stripes, backgrounds,
      selection stroke, collapse hit-test),
    - the C++ *text* layout (``TextRenderManager.renderNodeTextFromGraph``).

    Only keys the C++ ``RenderConfig`` actually exposes are bridged; the rest of
    ``NoodlesConfig`` is consumed elsewhere (or not yet wired). Values are coerced
    to the C++ field's native type so e.g. the integer ``nodeBg*`` fields accept a
    numeric setting without boost.python rejecting a float.
    """
    from .noodlesConfig import NoodlesConfig

    config = RenderConfig()
    for key in [
        # Sizing + layout metrics (calculateNodeSize and text layout).
        "nodeTitleFontSize",
        "nodePinFontSize",
        "nodePinTypeFontSize",
        # Hiding pin-type labels shrinks rows, so it feeds calculateNodeSize too.
        "showPinTypeLabels",
        "nodeMarginH",
        "nodeMarginV",
        "nodePortSpacing",
        "nodePortWidth",
        "nodePortRadius",
        "nodeFontSize",
        # Appearance (GraphNodeRenderer: corner, stroke, gradient, type color).
        "nodeRendererType",
        "nodeCornerRadius",
        "selectedNodeStrokeWidth",
        "nodeBgHigh",
        "nodeBgLow",
        "nodeBgAlpha",
        "nodeShadowFactor",
        "nodeTypeSaturation",
        "nodeTypeBrightness",
        # Links (LinkRenderManager: thickness, curve sampling, edge dimming).
        "linkLineWidth",
        "linkSampleRate",
        "linkEdgeDimmingStart",
        "linkEdgeDimmingEnd",
        "linkCutoffAlpha",
    ]:
        val = NoodlesConfig.get(key, None)
        if val is None or not hasattr(config, key):
            continue
        current = getattr(config, key)
        try:
            setattr(config, key, type(current)(val))
        except Exception as exc:
            # Don't let one malformed setting break the whole render path: skip
            # the key and keep the C++ default for this field. (A raw setattr
            # fallback could itself raise an uncaught boost.python error.)
            Tf.Warn(
                f"NoodlesConfig key {key!r}={val!r} could not be applied to "
                f"RenderConfig, using default: {exc}"
            )
    return config


class NodeGraph(GraphModel):
    """USD-aware graph that extends the generic GraphModel."""

    def __init__(self):
        super().__init__()
        self.syncSelectionToPrimTree = False
        self._stage = None
        # Intentionally shadow the C++ GraphModel members: self.nodes is the
        # interaction / hit-test model (a dict of NodeModel). The C++
        # GraphModel.nodes render snapshot is populated separately via
        # syncNodesFromModels (a NodeModel can't be stored in the C++ map — it
        # would slice to NodeData).
        self.nodes = {}
        self.links = []
        self.stickers = []
        self.linksChanged = True
        # Listener callbacks invoked when stage or edit target changes.
        # Plain python lists rather than Qt Signals so this stays usable
        # in headless (non-Qt) tests.
        self._stageChangedCallbacks = []
        self._editTargetChangedCallbacks = []

    @property
    def groupStickers(self):
        return self.stickers

    @groupStickers.setter
    def groupStickers(self, value):
        self.stickers = value

    def setStage(self, stage):
        """Set the USD stage for this graph."""
        self._stage = stage
        for cb in list(self._stageChangedCallbacks):
            try:
                cb(stage)
            except Exception as e:
                Tf.Warn(f"stageChanged callback failed: {e}")

    def getStage(self):
        """Get the USD stage for this graph."""
        return self._stage

    def getSessionLayer(self):
        """Get the session layer from the stage."""
        if self._stage:
            return self._stage.GetSessionLayer()
        return None

    def getEditTargetLayer(self):
        """Get the layer that edits are currently authored to."""
        if self._stage:
            return self._stage.GetEditTarget().GetLayer()
        return None

    def setEditTargetLayer(self, layer):
        """Set the stage edit target to *layer* and notify listeners."""
        if self._stage is None or layer is None:
            return
        self._stage.SetEditTarget(layer)
        warn_if_non_persistent_edit_target(self._stage)
        for cb in list(self._editTargetChangedCallbacks):
            try:
                cb(layer)
            except Exception as e:
                Tf.Warn(f"editTargetChanged callback failed: {e}")

    def addStageChangedCallback(self, callback):
        """Register a callable invoked as ``callback(stage)`` after setStage."""
        self._stageChangedCallbacks.append(callback)

    def removeStageChangedCallback(self, callback):
        if callback in self._stageChangedCallbacks:
            self._stageChangedCallbacks.remove(callback)

    def addEditTargetChangedCallback(self, callback):
        """Register a callable invoked as ``callback(layer)`` after setEditTargetLayer."""
        self._editTargetChangedCallbacks.append(callback)

    def removeEditTargetChangedCallback(self, callback):
        if callback in self._editTargetChangedCallbacks:
            self._editTargetChangedCallbacks.remove(callback)

    def _calculateNodeSize(self, node, calculateTextWidth, fontMetrics):
        """Bridge to the single C++ layout producer (GraphModel.layoutNode).

        Runs the whole node view-model in one atomic pass — display pins + row
        slots + displayRowKinds + size + per-pin centers — from the raw content
        on the struct, so the slot arrays and the centers can never desync (the
        root cause of row drift). Uses NoodlesConfig-derived appearance values.
        """
        config = build_render_config()
        # C++ expects a FontMetrics struct, but callers pass a FontAtlas.
        # Convert by copying the three shared properties.
        if not isinstance(fontMetrics, FontMetrics):
            fm = FontMetrics()
            fm.ascender = fontMetrics.ascender
            fm.descender = fontMetrics.descender
            fm.lineHeight = fontMetrics.lineHeight
            fontMetrics = fm
        self.layoutNode(node, calculateTextWidth, fontMetrics, config)
