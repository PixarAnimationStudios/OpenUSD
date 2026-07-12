#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

from OpenGL import GL
from pxr import Gf, Tf

from ..noodlesConfig import NoodlesConfig

# Viewport rectangle minimum size and hit area configuration
# These are implementation details, not user-facing settings
MINIMAP_VP_MIN_SIZE = 8.0  # Minimum visual size for viewport rectangle (pixels)
MINIMAP_VP_HIT_AREA_THRESHOLD = (
    20.0  # Below this size, expand hit area for easier grabbing
)
MINIMAP_VP_HIT_AREA_SIZE = 24.0  # Expanded hit area size when viewport is small

# Viewport stroke colors (not in main config schema yet)
MINIMAP_VP_STROKE_NORMAL = (0.7, 0.25, 0.25, 0.7)  # Viewport border - normal
MINIMAP_VP_STROKE_HOVER = (0.85, 0.4, 0.4, 0.85)  # Viewport border - hovered
MINIMAP_VP_STROKE_DRAG = (
    1.0,
    0.8,
    0.8,
    0.95,
)  # Viewport border - dragging (white glow)


class Minimap:
    def __init__(self, graphView):
        self._graphView = graphView

        # Minimap interaction state
        self._hovered = False
        self._viewportHovered = False
        self._viewportDragging = False
        self._dragOffset = Gf.Vec2d(0, 0)

        # Cache minimap geometry for hit testing (updated each paint)
        self._rect = None  # (x, y, w, h) in screen coords
        self._viewportRect = None  # (x0, y0, x1, y1) in screen coords
        self._scale = 1.0
        self._offset = Gf.Vec2d(0, 0)
        self._paddedGraphMin = Gf.Vec2d(0, 0)

    @property
    def hovered(self):
        return self._hovered

    @property
    def viewportHovered(self):
        return self._viewportHovered

    @property
    def viewportDragging(self):
        return self._viewportDragging

    @viewportDragging.setter
    def viewportDragging(self, value):
        self._viewportDragging = value

    @property
    def dragOffset(self):
        return self._dragOffset

    @dragOffset.setter
    def dragOffset(self, value):
        self._dragOffset = value

    @property
    def scale(self):
        return self._scale

    @property
    def viewportRect(self):
        return self._viewportRect

    def _getGraphBounds(self):
        bounds = Gf.Range2d()
        for node in self._graphView.nodes.values():
            nodeBounds = Gf.Range2d(node.position, node.position + node.size)
            bounds = Gf.Range2d.GetUnion(bounds, nodeBounds)
        return bounds

    def _getViewportBounds(self):
        gv = self._graphView
        scaledWidth = gv.width() / gv.zoom
        scaledHeight = gv.height() / gv.zoom
        minPt = Gf.Vec2d(gv.panX, gv.panY)
        maxPt = Gf.Vec2d(gv.panX + scaledWidth, gv.panY + scaledHeight)
        viewportBounds = Gf.Range2d(minPt, maxPt)

        return viewportBounds

    def isPointInMinimap(self, screenPos):
        if self._rect is None:
            return False

        x, y = screenPos.x(), screenPos.y()
        mx, my, mw, mh = self._rect
        return mx <= x <= mx + mw and my <= y <= my + mh

    def isPointInViewport(self, screenPos):
        if self._viewportRect is None:
            return False

        x, y = screenPos.x(), screenPos.y()
        vx0, vy0, vx1, vy1 = self._viewportRect
        return vx0 <= x <= vx1 and vy0 <= y <= vy1

    def updateHoverState(self, screenPos):
        wasHovered = self._viewportHovered
        self._hovered = self.isPointInMinimap(screenPos)
        self._viewportHovered = self.isPointInViewport(screenPos)

        # Request update if hover state changed
        if wasHovered != self._viewportHovered:
            self._graphView.update()

    def screenToWorld(self, screenPos):
        if self._scale == 0:
            return None

        x, y = screenPos.x(), screenPos.y()
        worldX = self._paddedGraphMin[0] + (x - self._offset[0]) / self._scale
        worldY = self._paddedGraphMin[1] + (y - self._offset[1]) / self._scale
        return Gf.Vec2d(worldX, worldY)

    def getGraphBounds(self):
        return self._getGraphBounds()

    def handleMousePress(self, screenPos):
        if not self.isPointInViewport(screenPos):
            return False

        self._viewportDragging = True

        # Calculate offset from viewport center to click point
        if self._viewportRect:
            vx0, vy0, vx1, vy1 = self._viewportRect
            vpCenterX = (vx0 + vx1) * 0.5
            vpCenterY = (vy0 + vy1) * 0.5
            self._dragOffset = Gf.Vec2d(
                screenPos.x() - vpCenterX, screenPos.y() - vpCenterY
            )

        self._graphView.update()
        return True

    def handleMouseMove(self, screenPos):
        if not self._viewportDragging:
            return False

        if self._scale <= 0:
            return True

        gv = self._graphView

        # Calculate new viewport center based on mouse position (minus drag offset)
        newScreenX = screenPos.x() - self._dragOffset[0]
        newScreenY = screenPos.y() - self._dragOffset[1]

        # Convert screen position to world coordinates
        from pxr.Usdviewq.qt import QtCore

        worldPos = self.screenToWorld(QtCore.QPoint(int(newScreenX), int(newScreenY)))
        if not worldPos:
            return True

        # Calculate viewport size in world coords
        vpWidth = gv.width() / gv.zoom
        vpHeight = gv.height() / gv.zoom

        # Set new pan to center the viewport on the world position
        newPanX = worldPos[0] - vpWidth * 0.5
        newPanY = worldPos[1] - vpHeight * 0.5

        # Clamp to graph bounds
        graphBounds = self._getGraphBounds()
        if not graphBounds.IsEmpty():
            graphMin = graphBounds.GetMin()
            graphMax = graphBounds.GetMax()

            # Add padding to allow some movement beyond graph bounds
            padding = Gf.Vec2d(graphBounds.GetSize()) * 0.1

            # Clamp pan so viewport stays within padded graph bounds
            minPanX = graphMin[0] - padding[0]
            maxPanX = graphMax[0] + padding[0] - vpWidth
            minPanY = graphMin[1] - padding[1]
            maxPanY = graphMax[1] + padding[1] - vpHeight

            gv.panX = max(minPanX, min(newPanX, maxPanX))
            gv.panY = max(minPanY, min(newPanY, maxPanY))
        else:
            gv.panX = newPanX
            gv.panY = newPanY

        gv.update()
        return True

    def handleMouseRelease(self):
        if not self._viewportDragging:
            return False

        self._viewportDragging = False
        self._graphView.update()
        return True

    def paint(self, NodeVertex, MAX_DEPTH):
        gv = self._graphView
        if not gv.nodes or not gv.shaderLibrary:
            return

        # Get config values
        minimapMinWindowWidth = NoodlesConfig.get("minimapMinWindowWidth", 500)
        minimapMinWindowHeight = NoodlesConfig.get("minimapMinWindowHeight", 350)
        minimapMaxHeight = NoodlesConfig.get("minimapMaxHeight", 120.0)
        minimapMinHeight = NoodlesConfig.get("minimapMinHeight", 50.0)
        minimapMargin = NoodlesConfig.get("minimapMargin", 10.0)
        minimapPadding = NoodlesConfig.get("minimapPadding", 4.0)
        minimapMaxHeightPercent = NoodlesConfig.get("minimapMaxHeightPercent", 0.15)
        minimapMaxWidthPercent = NoodlesConfig.get("minimapMaxWidthPercent", 0.5)
        minimapBgColor = NoodlesConfig.get("minimapBgColor", [20, 20, 25, 160])
        minimapBgStroke = NoodlesConfig.get("minimapBgStroke", [0.5, 0.5, 0.5, 0.5])
        minimapViewportNormal = NoodlesConfig.get(
            "minimapViewportNormal", [80, 30, 30, 120]
        )
        minimapViewportHover = NoodlesConfig.get(
            "minimapViewportHover", [110, 45, 45, 150]
        )
        minimapViewportDrag = NoodlesConfig.get(
            "minimapViewportDrag", [140, 60, 60, 180]
        )

        # Check if window is too small to show minimap
        if gv.width() < minimapMinWindowWidth or gv.height() < minimapMinWindowHeight:
            self._rect = None
            self._viewportRect = None
            return

        graphBounds = self._getGraphBounds()
        if graphBounds.IsEmpty():
            return

        viewportBounds = self._getViewportBounds()

        # Calculate graph dimensions with padding
        graphSize = graphBounds.GetSize()
        if graphSize[0] == 0 or graphSize[1] == 0:
            return

        graphPadding = Gf.Vec2d(graphSize[0] * 0.1, graphSize[1] * 0.1)
        paddedGraphMin = graphBounds.GetMin() - graphPadding
        paddedGraphMax = graphBounds.GetMax() + graphPadding
        paddedGraphSize = paddedGraphMax - paddedGraphMin

        # Calculate graph aspect ratio
        graphAspect = paddedGraphSize[0] / paddedGraphSize[1]

        # Calculate minimap height based on window height fraction, clamped to max
        maxHeightFromWindow = gv.height() * minimapMaxHeightPercent
        minimapH = min(minimapMaxHeight, maxHeightFromWindow)

        # Check if we're below minimum height threshold
        if minimapH < minimapMinHeight:
            if maxHeightFromWindow < minimapMinHeight * 0.8:
                self._rect = None
                self._viewportRect = None
                return
            minimapH = minimapMinHeight

        # Calculate width based on graph aspect ratio
        # The graph should fill the minimap exactly (no dead space)
        innerH = minimapH - minimapPadding * 2
        innerW = innerH * graphAspect
        minimapW = innerW + minimapPadding * 2

        # Clamp width to maximum allowed fraction of window
        maxWidthFromWindow = gv.width() * minimapMaxWidthPercent
        if minimapW > maxWidthFromWindow:
            minimapW = maxWidthFromWindow
            innerW = minimapW - minimapPadding * 2
            # Recalculate inner height to maintain aspect ratio
            innerH = innerW / graphAspect
            minimapH = innerH + minimapPadding * 2

        # Minimap position (upper right corner, right-justified, in screen space)
        minimapX = gv.width() - minimapW - minimapMargin
        minimapY = minimapMargin

        # Cache minimap rect for hit testing
        self._rect = (minimapX, minimapY, minimapW, minimapH)

        # Calculate scale - graph should fill the minimap interior exactly
        scale = innerW / paddedGraphSize[0]  # Same as innerH / paddedGraphSize[1]

        # Calculate offset (graph is centered in minimap)
        offsetX = minimapX + minimapPadding
        offsetY = minimapY + minimapPadding

        # Cache scale and offset for coordinate conversion
        self._scale = scale
        self._offset = Gf.Vec2d(offsetX, offsetY)
        self._paddedGraphMin = paddedGraphMin

        try:
            projection = gv._screenSpaceProjectionMatrix()
            depth = MAX_DEPTH - 0.5  # Very front

            # Disable depth testing for overlay
            GL.glDisable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_FALSE)

            # Draw background frame using configured colors
            bgVertexData = []
            bgR, bgG, bgB, bgA = minimapBgColor

            # NodeVertex signature: x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected
            bgVertexData.append(
                NodeVertex(
                    float(minimapX),
                    float(minimapY),
                    depth,
                    0.0,
                    0.0,
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bgVertexData.append(
                NodeVertex(
                    float(minimapX + minimapW),
                    float(minimapY),
                    depth,
                    float(minimapW),
                    0.0,
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bgVertexData.append(
                NodeVertex(
                    float(minimapX),
                    float(minimapY + minimapH),
                    depth,
                    0.0,
                    float(minimapH),
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bgVertexData.append(
                NodeVertex(
                    float(minimapX + minimapW),
                    float(minimapY),
                    depth,
                    float(minimapW),
                    0.0,
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bgVertexData.append(
                NodeVertex(
                    float(minimapX + minimapW),
                    float(minimapY + minimapH),
                    depth,
                    float(minimapW),
                    float(minimapH),
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bgVertexData.append(
                NodeVertex(
                    float(minimapX),
                    float(minimapY + minimapH),
                    depth,
                    0.0,
                    float(minimapH),
                    float(minimapW),
                    float(minimapH),
                    bgR,
                    bgG,
                    bgB,
                    bgA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

            # Use -1 generation to force cache bypass (minimap changes every frame)
            gv._drawNodeVertices(
                bgVertexData, projection, 3.0, tuple(minimapBgStroke), generation=-1
            )

            # Calculate viewport rectangle in minimap space
            vpMin = viewportBounds.GetMin()
            vpMax = viewportBounds.GetMax()

            # Transform viewport bounds to minimap coordinates
            vpX0 = offsetX + (vpMin[0] - paddedGraphMin[0]) * scale
            vpY0 = offsetY + (vpMin[1] - paddedGraphMin[1]) * scale
            vpX1 = offsetX + (vpMax[0] - paddedGraphMin[0]) * scale
            vpY1 = offsetY + (vpMax[1] - paddedGraphMin[1]) * scale

            # Clamp viewport rectangle to minimap bounds
            vpX0 = max(
                minimapX + minimapPadding,
                min(vpX0, minimapX + minimapW - minimapPadding),
            )
            vpY0 = max(
                minimapY + minimapPadding,
                min(vpY0, minimapY + minimapH - minimapPadding),
            )
            vpX1 = max(
                minimapX + minimapPadding,
                min(vpX1, minimapX + minimapW - minimapPadding),
            )
            vpY1 = max(
                minimapY + minimapPadding,
                min(vpY1, minimapY + minimapH - minimapPadding),
            )

            vpW = vpX1 - vpX0
            vpH = vpY1 - vpY0

            # Calculate viewport center for minimum size enforcement and hit area expansion
            vpCenterX = (vpX0 + vpX1) * 0.5
            vpCenterY = (vpY0 + vpY1) * 0.5

            # Enforce minimum visual size for the viewport rectangle
            # This ensures the viewport is always visible even when zoomed in very far
            if vpW < MINIMAP_VP_MIN_SIZE:
                halfMin = MINIMAP_VP_MIN_SIZE * 0.5
                vpX0 = vpCenterX - halfMin
                vpX1 = vpCenterX + halfMin
                vpW = MINIMAP_VP_MIN_SIZE
            if vpH < MINIMAP_VP_MIN_SIZE:
                halfMin = MINIMAP_VP_MIN_SIZE * 0.5
                vpY0 = vpCenterY - halfMin
                vpY1 = vpCenterY + halfMin
                vpH = MINIMAP_VP_MIN_SIZE

            # Cache the visual viewport rect (used for hit testing)
            # When viewport is small, we use an expanded hit area for easier grabbing
            if (
                vpW < MINIMAP_VP_HIT_AREA_THRESHOLD
                or vpH < MINIMAP_VP_HIT_AREA_THRESHOLD
            ):
                # Expand hit area to make it easier to grab
                hitHalfSize = MINIMAP_VP_HIT_AREA_SIZE * 0.5
                hitX0 = vpCenterX - hitHalfSize
                hitY0 = vpCenterY - hitHalfSize
                hitX1 = vpCenterX + hitHalfSize
                hitY1 = vpCenterY + hitHalfSize
                # Clamp hit area to minimap bounds
                hitX0 = max(minimapX, hitX0)
                hitY0 = max(minimapY, hitY0)
                hitX1 = min(minimapX + minimapW, hitX1)
                hitY1 = min(minimapY + minimapH, hitY1)
                self._viewportRect = (hitX0, hitY0, hitX1, hitY1)
            else:
                self._viewportRect = (vpX0, vpY0, vpX1, vpY1)

            # Always draw the viewport now that we have minimum size enforcement
            # Determine viewport colors based on hover/drag state
            if self._viewportDragging:
                vpR, vpG, vpB, vpA = minimapViewportDrag
                vpStrokeColor = MINIMAP_VP_STROKE_DRAG
            elif self._viewportHovered:
                vpR, vpG, vpB, vpA = minimapViewportHover
                vpStrokeColor = MINIMAP_VP_STROKE_HOVER
            else:
                vpR, vpG, vpB, vpA = minimapViewportNormal
                vpStrokeColor = MINIMAP_VP_STROKE_NORMAL

            # Draw viewport frame
            # NodeVertex signature: x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected
            vpVertexData = []
            vpVertexData.append(
                NodeVertex(
                    float(vpX0),
                    float(vpY0),
                    depth,
                    0.0,
                    0.0,
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            vpVertexData.append(
                NodeVertex(
                    float(vpX1),
                    float(vpY0),
                    depth,
                    float(vpW),
                    0.0,
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            vpVertexData.append(
                NodeVertex(
                    float(vpX0),
                    float(vpY1),
                    depth,
                    0.0,
                    float(vpH),
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            vpVertexData.append(
                NodeVertex(
                    float(vpX1),
                    float(vpY0),
                    depth,
                    float(vpW),
                    0.0,
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            vpVertexData.append(
                NodeVertex(
                    float(vpX1),
                    float(vpY1),
                    depth,
                    float(vpW),
                    float(vpH),
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            vpVertexData.append(
                NodeVertex(
                    float(vpX0),
                    float(vpY1),
                    depth,
                    0.0,
                    float(vpH),
                    float(vpW),
                    float(vpH),
                    vpR,
                    vpG,
                    vpB,
                    vpA,
                    1.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

            gv._drawNodeVertices(
                vpVertexData, projection, 0.0, vpStrokeColor, generation=-1
            )

            # Re-enable depth testing
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)

        except Exception as e:
            Tf.Warn(f"Error rendering minimap: {e}")
            import traceback

            traceback.print_exc()
            GL.glEnable(GL.GL_DEPTH_TEST)
