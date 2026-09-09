#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

from pxr import Gf, Tf

from ..nodeGraph import warn_if_non_persistent_edit_target

try:
    from pxr import UsdUI
except ImportError:
    UsdUI = None

# Default drawing configuration for group stickers
DEFAULT_CORNER_RADIUS = 25.0
DEFAULT_INNER_STROKE = 10.0
DEFAULT_ALPHA = 255
DEFAULT_STROKE_COLOR = (0.078, 0.196, 1.0, 0.392)  # (20, 50, 255, 100) normalized
DEFAULT_COLOR = Gf.Vec4f(0.196, 0.196, 0.196, 1.0)


class GroupSticker:
    """(name, position, size, color, cornerRadius, innerStroke, alpha, strokeColor)"""

    # Scale factor between USD coordinate space and screen/display coordinates.
    # Matches the positionScale used in models.py for node positions.
    _POSITION_SCALE = 1000.0

    def __init__(
        self,
        name="",
        position=None,
        size=None,
        color=None,
        cornerRadius=None,
        innerStroke=None,
        alpha=None,
        strokeColor=None,
        prim=None,
    ):
        self._prim = prim
        self._name = name
        self._position = position if position is not None else Gf.Vec2d(0, 0)
        self._size = size if size is not None else Gf.Vec2d(100, 100)
        self.color = color if color is not None else Gf.Vec4f(DEFAULT_COLOR)
        self.cornerRadius = (
            cornerRadius if cornerRadius is not None else DEFAULT_CORNER_RADIUS
        )
        self.innerStroke = (
            innerStroke if innerStroke is not None else DEFAULT_INNER_STROKE
        )
        self.alpha = alpha if alpha is not None else DEFAULT_ALPHA
        self.strokeColor = (
            strokeColor if strokeColor is not None else DEFAULT_STROKE_COLOR
        )

    # -- USD-backed property: name -----------------------------------------

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value
        if self._prim and UsdUI is not None:
            self._writeNameToUsd(value)

    def _writeNameToUsd(self, value):
        """Write name/description to the stage's current edit target."""
        stage = self._prim.GetStage()
        if not stage:
            return
        warn_if_non_persistent_edit_target(stage)
        backdrop = UsdUI.Backdrop(self._prim)
        descAttr = backdrop.GetDescriptionAttr()
        if not descAttr:
            descAttr = backdrop.CreateDescriptionAttr()
        if descAttr:
            descAttr.Set(value)

    # -- USD-backed property: position -------------------------------------

    @property
    def position(self):
        return self._position

    @position.setter
    def position(self, value):
        self._position = value
        if self._prim and UsdUI is not None:
            self._writePositionToUsd(value)

    def _writePositionToUsd(self, value):
        """Write position to the stage's current edit target."""
        stage = self._prim.GetStage()
        if not stage:
            return
        warn_if_non_persistent_edit_target(stage)
        api = UsdUI.NodeGraphNodeAPI(self._prim)
        posAttr = api.GetPosAttr()
        if not posAttr:
            posAttr = api.CreatePosAttr()
        if posAttr:
            scale = 1.0 / self._POSITION_SCALE
            posAttr.Set(Gf.Vec2f(value[0] * scale, value[1] * scale))

    # -- USD-backed property: size -----------------------------------------

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value
        if self._prim and UsdUI is not None:
            self._writeSizeToUsd(value)

    def _writeSizeToUsd(self, value):
        """Write size to the stage's current edit target."""
        stage = self._prim.GetStage()
        if not stage:
            return
        warn_if_non_persistent_edit_target(stage)
        api = UsdUI.NodeGraphNodeAPI(self._prim)
        sizeAttr = api.GetSizeAttr()
        if not sizeAttr:
            sizeAttr = api.CreateSizeAttr()
        if sizeAttr:
            scale = 1.0 / self._POSITION_SCALE
            sizeAttr.Set(Gf.Vec2f(value[0] * scale, value[1] * scale))

    # -- from_prim classmethod ---------------------------------------------

    @classmethod
    def from_prim(cls, prim):
        """Create a GroupSticker from a UsdUI.Backdrop prim.

        Reads position, size, label, color and non-standard properties
        (cornerRadius, alpha, strokeColor, innerStroke) from the prim.
        """
        scale = cls._POSITION_SCALE

        # Position
        position = Gf.Vec2d(0, 0)
        api = UsdUI.NodeGraphNodeAPI(prim)
        posAttr = api.GetPosAttr()
        if posAttr and posAttr.HasAuthoredValue():
            pos = posAttr.Get()
            position = Gf.Vec2d(pos[0] * scale, pos[1] * scale)

        # Size
        size = Gf.Vec2d(100, 100)
        sizeAttr = api.GetSizeAttr()
        if sizeAttr and sizeAttr.HasAuthoredValue():
            sz = sizeAttr.Get()
            size = Gf.Vec2d(sz[0] * scale, sz[1] * scale)

        # Label / description
        name = ""
        backdrop = UsdUI.Backdrop(prim)
        descAttr = backdrop.GetDescriptionAttr()
        if descAttr and descAttr.HasAuthoredValue():
            name = str(descAttr.Get())

        # Color (UsdUI displayColor is Vec3f 0-1, GroupSticker uses Vec4f 0-1)
        color = Gf.Vec4f(DEFAULT_COLOR)
        colorAttr = api.GetDisplayColorAttr()
        if colorAttr and colorAttr.HasAuthoredValue():
            c = colorAttr.Get()
            color = Gf.Vec4f(c[0], c[1], c[2], 1.0)

        # Non-standard properties from customData
        cornerRadius = prim.GetCustomDataByKey("cornerRadius")
        if cornerRadius is None:
            cornerRadius = DEFAULT_CORNER_RADIUS

        alpha = prim.GetCustomDataByKey("alpha")
        if alpha is None:
            alpha = DEFAULT_ALPHA

        innerStroke = prim.GetCustomDataByKey("innerStroke")
        if innerStroke is None:
            innerStroke = DEFAULT_INNER_STROKE

        strokeColorData = prim.GetCustomDataByKey("strokeColor")
        if strokeColorData is not None and len(strokeColorData) == 4:
            strokeColor = tuple(strokeColorData)
        else:
            strokeColor = DEFAULT_STROKE_COLOR

        return cls(
            name=name,
            position=position,
            size=size,
            color=color,
            cornerRadius=cornerRadius,
            innerStroke=innerStroke,
            alpha=alpha,
            strokeColor=strokeColor,
            prim=prim,
        )

    def getColorRGB(self):
        return (
            int(self.color[0] * 255),
            int(self.color[1] * 255),
            int(self.color[2] * 255),
        )

    def getBounds(self):
        return Gf.Range2d(self.position, self.position + self.size)

    def containsPoint(self, point):
        return self.getBounds().Contains(point)

    def generateVertexData(self, NodeVertex, depth):
        w = self.size[0]
        h = self.size[1]
        x0 = self.position[0]
        y0 = self.position[1]
        x1 = x0 + w
        y1 = y0 + h

        red, grn, blu = self.getColorRGB()
        alpha = self.alpha
        stroke = self.innerStroke

        # NodeVertex signature: x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected
        vertices = [
            NodeVertex(
                float(x0),
                float(y0),
                depth,
                0.0,
                0.0,
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                float(x1),
                float(y0),
                depth,
                float(w),
                0.0,
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                float(x0),
                float(y1),
                depth,
                0.0,
                float(h),
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                float(x1),
                float(y0),
                depth,
                float(w),
                0.0,
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                float(x1),
                float(y1),
                depth,
                float(w),
                float(h),
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
            NodeVertex(
                float(x0),
                float(y1),
                depth,
                0.0,
                float(h),
                float(w),
                float(h),
                red,
                grn,
                blu,
                alpha,
                stroke,
                0,
                0,
                0,
                255,
                0.0,
            ),
        ]

        return vertices


class GroupStickerRenderer:
    def __init__(self):
        self._cpp = None

    def initialize(self, shaderLibrary, cpp_renderer=None):
        self._cpp = cpp_renderer

    def cleanup(self):
        if self._cpp:
            self._cpp.cleanup()
            self._cpp = None

    def render(self, stickers, graphView, NodeVertex):
        if not stickers or not graphView.shaderLibrary:
            return False

        try:
            stickerVertexData = []
            depth = 0.0

            for sticker in stickers:
                vertices = sticker.generateVertexData(NodeVertex, depth)
                stickerVertexData.extend(vertices)
                depth += 1.0

            if not stickerVertexData:
                return False

            projection = graphView._worldSpaceProjectionMatrix()

            if stickers:
                cornerRadius = stickers[0].cornerRadius
                strokeColor = stickers[0].strokeColor
            else:
                cornerRadius = DEFAULT_CORNER_RADIUS
                strokeColor = DEFAULT_STROKE_COLOR

            graphView._drawNodeVertices(
                stickerVertexData, projection, cornerRadius, strokeColor, generation=0
            )
            return True

        except Exception as e:
            Tf.Warn(f"Error rendering group stickers: {e}")
            import traceback

            traceback.print_exc()
            return False
