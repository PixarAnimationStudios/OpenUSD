#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles.render import TextRenderManager
from OpenGL import GL
from OpenGL.error import GLError
from pxr import Tf


class TextRenderer:
    """Thin Python facade over the C++ ``TextRenderManager``.

    Per-node graph text is generated, held, and drawn entirely in C++ via
    ``renderNodeText`` -> ``TextRenderManager.renderNodeTextFromGraph`` (which
    reads the authoritative ``GraphModel.nodes`` snapshot — no per-frame vertex
    marshalling across the language boundary). The per-string
    helpers (``calculateTextWidth`` / ``generateTextVertices`` /
    ``drawTextVertices``) remain for screen-space widget text such as hotkey
    popups, the status overlay, selectable lists, and text inputs.
    """

    def __init__(self):
        self._cpp = TextRenderManager()
        self.font_atlas = None
        self.shaderLibrary = None
        self._initialized = False

    def initialize(self, shaderLibrary, font_atlas):
        self.shaderLibrary = shaderLibrary
        self.font_atlas = font_atlas
        self._cpp.initialize(shaderLibrary, font_atlas)
        self._initialized = True

    @property
    def isInitialized(self):
        return (
            self._initialized
            and self.font_atlas is not None
            and self.shaderLibrary is not None
        )

    # --- Per-string text (screen-space widgets) -----------------------------

    def calculateTextWidth(self, text, font_size):
        if not self.font_atlas:
            return len(text) * font_size * 0.5
        return self._cpp.calculateTextWidth(text, font_size)

    def generateTextVertices(
        self, text, cursor_x, cursor_y, depth, scale, vertex_data, node_index=0.0
    ):
        if not self.font_atlas:
            return cursor_x, 0
        return self._cpp.generateTextVertices(
            text, cursor_x, cursor_y, depth, scale, vertex_data, node_index
        )

    def drawTextVertices(
        self, vertex_data, projection, color=(1.0, 1.0, 1.0, 1.0), disable_depth=False
    ):
        if not vertex_data:
            return

        try:
            if disable_depth:
                GL.glDisable(GL.GL_DEPTH_TEST)
                GL.glDepthMask(GL.GL_FALSE)

            proj = list(projection.data())
            self._cpp.renderText(vertex_data, proj, list(color))
        except GLError as e:
            Tf.Warn(f"GL error during text rendering: {e}")
        finally:
            if disable_depth:
                GL.glEnable(GL.GL_DEPTH_TEST)
                GL.glDepthMask(GL.GL_TRUE)

    # --- Node graph text (C++-owned) ----------------------------------------

    def setTransformFrame(self, frame):
        """Share the node-local coordinate frame with the C++ text path.

        The same frame is shared with the node-quad renderer, so text and quads
        sample one transform texture and move together on a drag.
        """
        self._cpp.setTransformFrame(frame)

    def resetPositionCaches(self):
        self._cpp.resetPositionCaches()

    def markNodeTextDirty(self, needs_rebuild=False):
        self._cpp.markNodeTextDirty(needs_rebuild)

    def patchNodeTextDepth(self, node):
        """Patch one node's text depth in the C++-held buffer (bring-to-front on
        selection) without re-laying out every node's text.

        If the node's slice isn't in the held buffer yet (e.g. a selection in the
        window between a reload and the next full text rebuild), fall back to
        flagging a rebuild so the new depth is still applied on the next frame.
        """
        if not self._cpp.patchNodeTextDepth(node.id, node.zOrder):
            # Positional: the boost.python binding declares the arg as
            # ``needsRebuild`` (camelCase); a snake_case kwarg would raise.
            self._cpp.markNodeTextDirty(True)

    def renderNodeText(
        self,
        graph,
        projection,
        zoom,
        text_changed,
        pan_x=0.0,
        pan_y=0.0,
        viewport_width=0.0,
        viewport_height=0.0,
        cull_offscreen=False,
    ):
        if not graph.nodes or not self.shaderLibrary or not self.font_atlas:
            return False

        # Text shares the single RenderConfig bridge (build_render_config) with
        # node sizing and the node renderer (ports, stripes, backgrounds,
        # collapse hit-test), so all three stay in lockstep with NoodlesConfig.
        from .nodeGraph import build_render_config

        proj = list(projection.data())
        try:
            # GraphModel.nodes is the authoritative C++ render snapshot; re-sync
            # it from the Python models only when a re-layout will read it (the
            # same gate renderNodeTextFull uses internally), then render from it.
            # Keep the sync and render adjacent — the freshness invariant
            # depends on no frame boundary between them.
            if text_changed or self._cpp.needsTextRebuild():
                graph.syncNodesFromModels(graph.nodes)
            return self._cpp.renderNodeTextFromGraph(
                graph,
                proj,
                zoom,
                text_changed,
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
        self._cpp.cleanup()
