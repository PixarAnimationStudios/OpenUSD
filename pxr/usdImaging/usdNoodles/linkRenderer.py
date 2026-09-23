#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles import _usdNoodles as _noodles
from pxr.UsdNoodles.render import LinkRenderManager
from pxr import Gf

_NUM_CURVE_SAMPLES = 20

# Max chord deviation (world units) for the cached, curvature-adaptive pick
# curve. Smaller is more faithful; the curve is sampled once per geometry change
# and reused across pan/zoom, so a tight value is cheap.
_PICK_CURVE_FLATNESS = 0.5

# Fraction of linkLineWidth at which the drawn ribbon's visible edge sits. The
# link shader renders the solid ribbon out to its 0.5 SDF cutoff, which lies at
# half the world thickness (uThickness == linkLineWidth), excluding the AA
# padding. Picking uses this so the grab radius matches the drawn edge exactly
# when zoomed in. Keep in sync with link_poly_vert.glsl / link_poly_frag.glsl.
_LINK_VISIBLE_HALF_WIDTH_FRACTION = 0.5

# Minimum link pick radius in screen pixels. When zoomed out the visible ribbon
# shrinks below a few pixels, so the pick radius is floored here (in screen space,
# converted to world via / zoom) to keep thin/zoomed-out links clickable. This
# clamps the PICK band only; the visual link width is intentionally left unchanged.
_MIN_LINK_PICK_TOLERANCE_PX = 2.0


def _prim_target_arrow_length(zoom):
    return max(10.0 / max(zoom, 1e-6), 2.0)


def _rendered_prim_target_end(end, zoom):
    return (end[0], end[1] - _prim_target_arrow_length(zoom))


def sampleLinkCurve(start, end, num_samples, vertical_end_tangent=False):
    """Sample the shaped link "noodle" curve into world-space points.

    Thin Python entry over the C++ ``LinkGeometry::sampleLinkCurve``, which
    mirrors the GPU vertex shader so hit-testing and arrowheads match what is
    drawn. ``start`` and ``end`` may be any (x, y)-indexable pair; returns a list
    of ``Gf.Vec2d``.
    """
    return _noodles.LinkGeometry.sampleLinkCurve(
        Gf.Vec2d(start[0], start[1]),
        Gf.Vec2d(end[0], end[1]),
        num_samples,
        vertical_end_tangent,
    )


def sampleLinkCurveAdaptive(
    start, end, vertical_end_tangent=False, flatness=_PICK_CURVE_FLATNESS
):
    """Curvature-adaptive sampling of the shaped link curve.

    Delegates to the C++ ``LinkGeometry::sampleLinkCurveAdaptive``: dense points
    on the backward S-bow, few on near-straight links, to ``flatness`` world
    units. Zoom-independent, so the result can be cached and reused across
    pan/zoom. Returns a list of ``Gf.Vec2d``.
    """
    return _noodles.LinkGeometry.sampleLinkCurveAdaptive(
        Gf.Vec2d(start[0], start[1]),
        Gf.Vec2d(end[0], end[1]),
        vertical_end_tangent,
        flatness,
    )


def computeLinkCurveBounds(link, num_samples=_NUM_CURVE_SAMPLES, margin=0.0):
    """Conservative world-space bounds of a link's shaped curve, plus ``margin``.

    Covers the backward S-bow that extends past the straight start/end box, so a
    spatial broad-phase keyed on these bounds will not miss backward links. Uses
    the raw (un-inset) end so the bounds are zoom-independent (the prim-target
    arrow inset only shrinks the curve). Returns a ``Gf.Range2d``.
    """
    vertical = _uses_prim_target_relationship_curve(link)
    bounds = _noodles.LinkGeometry.computeLinkCurveBounds(
        Gf.Vec2d(link.start[0], link.start[1]),
        Gf.Vec2d(link.end[0], link.end[1]),
        num_samples,
        vertical,
    )
    if not margin:
        return bounds
    return Gf.Range2d(
        Gf.Vec2d(bounds.GetMin()[0] - margin, bounds.GetMin()[1] - margin),
        Gf.Vec2d(bounds.GetMax()[0] + margin, bounds.GetMax()[1] + margin),
    )


class LinkRenderer:
    def __init__(self):
        self._cpp = LinkRenderManager()
        self._initialized = False
        # Per-link cache of the curvature-adaptive pick polyline, keyed by link
        # index and validated by the link's geometry. Built lazily on hover and
        # reused across pan/zoom until the link's endpoints change.
        self._curveSegCache = {}

    def initialize(self, shaderLibrary):
        self._cpp.initialize(shaderLibrary)
        self._initialized = True

    def renderLinks(
        self,
        links,
        projection,
        zoom,
        panX,
        panY,
        width,
        height,
        config,
        dimming=1.0,
        drawSelected=False,
        hoveredColor=None,
        selectedColor=None,
        baseColor=None,
        highlightedColor=None,
        cacheKey=0,
    ):
        if not links or not self._initialized:
            return

        proj = list(projection.data())

        # Default colors if not provided
        if hoveredColor is None:
            hoveredColor = [1.0, 1.0, 0.0]
        if selectedColor is None:
            selectedColor = [1.0, 1.0, 0.3]
        if baseColor is None:
            baseColor = []  # Empty list signals use default in C++
        if highlightedColor is None:
            highlightedColor = []

        self._cpp.renderLinks(
            links,
            proj,
            config,
            zoom,
            panX,
            panY,
            float(width),
            float(height),
            dimming,
            drawSelected,
            baseColor,
            selectedColor,
            hoveredColor,
            highlightedColor,
            cacheKey,
        )

    def renderLinksFromGraph(
        self,
        graph,
        projection,
        zoom,
        panX,
        panY,
        width,
        height,
        config,
        relationshipOnly,
        dimming=1.0,
        drawSelected=False,
        hoveredColor=None,
        selectedColor=None,
        baseColor=None,
        highlightedColor=None,
        cacheKey=0,
    ):
        """Render links from the held C++ GraphModel.links snapshot (no per-frame
        Python->C++ marshalling). ``graph`` is the NodeGraph (a GraphModel)."""
        if not self._initialized:
            return

        proj = list(projection.data())

        # Default colors if not provided
        if hoveredColor is None:
            hoveredColor = [1.0, 1.0, 0.0]
        if selectedColor is None:
            selectedColor = [1.0, 1.0, 0.3]
        if baseColor is None:
            baseColor = []  # Empty list signals use default in C++
        if highlightedColor is None:
            highlightedColor = []

        self._cpp.renderLinksFromGraph(
            graph,
            proj,
            config,
            zoom,
            panX,
            panY,
            float(width),
            float(height),
            dimming,
            drawSelected,
            relationshipOnly,
            baseColor,
            selectedColor,
            hoveredColor,
            highlightedColor,
            cacheKey,
        )

    def findLinkUnderCursor(
        self, worldPos, links, zoom, linkLineWidth, spatialIndex=None, debug=False
    ):
        if not links:
            return -1

        # Pick radius matches the drawn ribbon. Its visible edge is the shader's
        # 0.5 SDF cutoff, at 0.5 * linkLineWidth in world space (zoom-independent,
        # AA padding excluded) -- so when zoomed in the grab band is exactly the
        # visible ribbon (WYSIWYG). When zoomed out the visible ribbon shrinks
        # below a few pixels, so the radius is floored at a minimum on-screen
        # value (converted to world via / zoom).
        worldTolerance = max(
            _LINK_VISIBLE_HALF_WIDTH_FRACTION * linkLineWidth,
            _MIN_LINK_PICK_TOLERANCE_PX / zoom,
        )

        # Broad-phase: only narrow-phase the few links whose curve bounds are
        # near the cursor, instead of sampling every link in the scene.
        candidateIndices = self._candidateLinkIndices(
            worldPos, len(links), worldTolerance, spatialIndex
        )

        segmentEndpoints = []
        segmentToLink = []
        for linkIdx in candidateIndices:
            for seg in self._cachedLinkSegments(links[linkIdx], linkIdx):
                segmentEndpoints.append(seg)
                segmentToLink.append(linkIdx)

        if not segmentEndpoints:
            return -1

        hitSegment = _noodles.LinkGeometry.findLinkUnderCursor(
            Gf.Vec2d(worldPos[0], worldPos[1]), segmentEndpoints, worldTolerance
        )

        if hitSegment >= 0:
            return segmentToLink[hitSegment]
        return -1

    def _candidateLinkIndices(self, worldPos, linkCount, worldTolerance, spatialIndex):
        """Broad-phase: link indices whose curve bounds are near the cursor.

        Queries the spatial index (a quadtree of curve-inclusive AABBs) so only
        the handful of links near the cursor are narrow-phased. Falls back to all
        links when no index is supplied (keeps non-indexed callers working).
        """
        if spatialIndex is None:
            return range(linkCount)
        queryBox = Gf.Range2d(
            Gf.Vec2d(worldPos[0] - worldTolerance, worldPos[1] - worldTolerance),
            Gf.Vec2d(worldPos[0] + worldTolerance, worldPos[1] + worldTolerance),
        )
        _, linkIndices = spatialIndex.queryRegion(queryBox)
        return [i for i in linkIndices if 0 <= i < linkCount]

    def _cachedLinkSegments(self, link, linkIdx):
        """Curvature-adaptive pick segments for a link, cached by its geometry.

        The polyline is zoom-independent, so it is sampled once and reused across
        pan/zoom; it is re-sampled only when the link's endpoints (or prim-target
        flag) change. The prim-target arrow inset is intentionally omitted here so
        the cache stays zoom-independent -- it only shortens the curve at the tip
        by a few world units (well within the hit tolerance), and treating the
        arrowhead region as pickable is desirable.
        """
        vertical = _uses_prim_target_relationship_curve(link)
        geomKey = (link.start[0], link.start[1], link.end[0], link.end[1], vertical)
        cached = self._curveSegCache.get(linkIdx)
        if cached is not None and cached[0] == geomKey:
            return cached[1]
        points = sampleLinkCurveAdaptive(
            (link.start[0], link.start[1]), (link.end[0], link.end[1]), vertical
        )
        segs = [(points[j], points[j + 1]) for j in range(len(points) - 1)]
        self._curveSegCache[linkIdx] = (geomKey, segs)
        return segs

    def findLinksInBounds(self, links, bounds):
        if not links:
            return []

        linkEndpoints = [
            (Gf.Vec2d(link.start[0], link.start[1]), Gf.Vec2d(link.end[0], link.end[1]))
            for link in links
        ]

        return _noodles.LinkGeometry.findLinksInBounds(linkEndpoints, bounds)

    def cleanup(self):
        self._cpp.cleanup()


def _uses_prim_target_relationship_curve(link):
    return (
        bool(getattr(link, "is_relationship_link", False))
        and bool(str(getattr(link, "targetNodeId", "")))
        and not str(getattr(link, "targetPort", ""))
        and not str(getattr(link, "targetPropertyName", ""))
    )
