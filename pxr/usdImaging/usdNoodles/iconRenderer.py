#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""Icon renderer for node title bar icons.

Node title + row icons are generated, held, and drawn entirely in C++ via the
``CppIconRenderer`` facade over ``IconRenderManager``.
"""

try:
    from pxr.UsdNoodles.render import IconRenderManager
except ImportError:
    # The C++ extension may be unavailable (e.g. in a pure-Python context or a
    # sandboxed test). The C++-backed CppIconRenderer facade degrades to a no-op
    # in that case (see its __init__).
    IconRenderManager = None


class CppIconRenderer:
    """Thin Python facade over the C++ ``IconRenderManager``.

    Mirrors ``TextRenderer`` in textRenderer.py: per-node title + row icons are
    generated, held, and drawn entirely in C++ via ``renderNodeIcons`` ->
    ``IconRenderManager.renderIconsFromGraph`` (which reads the authoritative
    ``GraphModel.nodes`` snapshot — no per-frame quad marshalling across the
    language boundary). Sharing the ``NodeTransformFrame`` with the text and
    node-quad paths means icons ride node drags from the same transform texture.

    If the C++ extension is unavailable, construction degrades to a no-op
    (``isInitialized`` stays False) so the editor still opens.
    """

    def __init__(self):
        self._cpp = IconRenderManager() if IconRenderManager is not None else None
        self.shaderLibrary = None
        self._initialized = False

    def initialize(self, shaderLibrary, assets_path):
        if self._cpp is None:
            return
        self.shaderLibrary = shaderLibrary
        self._cpp.initialize(shaderLibrary, str(assets_path))
        self._initialized = True

    @property
    def isInitialized(self):
        return self._initialized and self.shaderLibrary is not None

    def setTransformFrame(self, frame):
        """Share the node-local coordinate frame with the C++ icon path.

        The same frame is shared with the text + node-quad renderers, so icons,
        text, and quads sample one transform texture and move together on a drag.
        """
        if self._cpp is not None:
            self._cpp.setTransformFrame(frame)

    def markIconsDirty(self):
        if self._cpp is not None:
            self._cpp.markIconsDirty()

    def resetPositionCaches(self):
        """Drop the per-node icon vertex cache (baked at the transform-frame base)
        so icons re-bake at the re-baselined base after a NodeTransformFrame reset.
        Peer of TextRenderer.resetPositionCaches."""
        if self._cpp is not None:
            self._cpp.resetPositionCaches()

    def renderNodeIcons(
        self,
        graph,
        projection,
        zoom,
        content_changed,
        pan_x=0.0,
        pan_y=0.0,
        viewport_width=0.0,
        viewport_height=0.0,
        cull_offscreen=False,
    ):
        if self._cpp is None or not graph.nodes or not self.shaderLibrary:
            return False

        # Icons share the single RenderConfig bridge (build_render_config) with
        # node sizing, text, and the node renderer so they stay in lockstep with
        # NoodlesConfig.
        from .nodeGraph import build_render_config

        proj = list(projection.data())
        try:
            # GraphModel.nodes is the authoritative C++ render snapshot; re-sync
            # it from the Python models whenever a rebuild will read it (the same
            # gate TextRenderer uses), then render from it. The C++ rebuild fires
            # on contentChanged OR markIconsDirty (needsIconRebuild), so gating
            # the sync on contentChanged alone would let a dirty-only frame
            # rebuild from a stale snapshot. Keep the sync and render adjacent —
            # the freshness invariant depends on no frame boundary between them.
            if content_changed or self._cpp.needsIconRebuild():
                graph.syncNodesFromModels(graph.nodes)
            return self._cpp.renderIconsFromGraph(
                graph,
                proj,
                zoom,
                content_changed,
                build_render_config(),
                pan_x,
                pan_y,
                viewport_width,
                viewport_height,
                cull_offscreen,
            )
        except Exception:
            import traceback

            traceback.print_exc()
            return False

    def cleanup(self):
        if self._cpp is not None:
            self._cpp.cleanup()
