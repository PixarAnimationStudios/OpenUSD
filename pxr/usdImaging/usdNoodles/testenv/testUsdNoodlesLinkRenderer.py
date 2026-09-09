#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


from __future__ import annotations

import unittest

try:
    from types import SimpleNamespace

    from pxr.UsdNoodles.spatial import SpatialIndex
    from pxr import Gf
    from pxr.UsdNoodles.linkRenderer import (
        computeLinkCurveBounds,
        LinkRenderer,
        sampleLinkCurve as _sampleLinkCurve,
    )

    _has_link_renderer = True
except ImportError as e:
    print(f"link renderer imports failed: {e}")
    _has_link_renderer = False


def _make_picker():
    # Build a LinkRenderer without running __init__ (which constructs the C++
    # render manager). Picking only needs the curve cache, so this keeps the test
    # free of any GL/render-manager setup.
    renderer = LinkRenderer.__new__(LinkRenderer)
    renderer._curveSegCache = {}
    return renderer


def _segment_delta(points):
    return (
        float(points[-1][0] - points[-2][0]),
        float(points[-1][1] - points[-2][1]),
    )


@unittest.skipUnless(_has_link_renderer, "Link renderer modules not available")
class LinkRendererCurveTest(unittest.TestCase):
    def test_prim_target_curve_approaches_end_vertically(self):
        start = (10.0, 30.0)
        end = (110.0, 20.0)

        default_curve = _sampleLinkCurve(start, end, 20)
        prim_target_curve = _sampleLinkCurve(
            start,
            end,
            20,
            vertical_end_tangent=True,
        )

        default_dx, default_dy = _segment_delta(default_curve)
        prim_dx, prim_dy = _segment_delta(prim_target_curve)

        self.assertEqual(default_curve[-1], Gf.Vec2d(*end))
        self.assertEqual(prim_target_curve[-1], Gf.Vec2d(*end))
        self.assertGreater(abs(default_dx), abs(default_dy))
        self.assertGreater(abs(prim_dy), abs(prim_dx))
        self.assertLess(abs(prim_dx), abs(default_dx))

    def test_prim_target_curve_approaches_end_vertically_for_backward_links(self):
        start = (110.0, 30.0)
        end = (20.0, 20.0)

        prim_target_curve = _sampleLinkCurve(
            start,
            end,
            20,
            vertical_end_tangent=True,
        )

        prim_dx, prim_dy = _segment_delta(prim_target_curve)

        self.assertEqual(prim_target_curve[-1], Gf.Vec2d(*end))
        self.assertGreater(abs(prim_dy), abs(prim_dx))


@unittest.skipUnless(_has_link_renderer, "Link renderer modules not available")
class LinkRendererBoundsTest(unittest.TestCase):
    def test_curve_bounds_cover_backward_bow(self):
        # The backward S-bow reaches x ~ 543.75, right of both endpoints (max
        # 500), so the curve bounds must extend past the straight endpoint box.
        link = SimpleNamespace(start=Gf.Vec2d(500.0, 0.0), end=Gf.Vec2d(0.0, 300.0))
        bounds = computeLinkCurveBounds(link)
        self.assertGreater(bounds.GetMax()[0], 500.0)
        self.assertTrue(bounds.Contains(Gf.Vec2d(500.0, 0.0)))
        self.assertTrue(bounds.Contains(Gf.Vec2d(0.0, 300.0)))

    def test_curve_bounds_margin_applied(self):
        link = SimpleNamespace(start=Gf.Vec2d(0.0, 0.0), end=Gf.Vec2d(200.0, 20.0))
        tight = computeLinkCurveBounds(link)
        padded = computeLinkCurveBounds(link, margin=20.0)
        self.assertAlmostEqual(padded.GetMin()[0], tight.GetMin()[0] - 20.0)
        self.assertAlmostEqual(padded.GetMin()[1], tight.GetMin()[1] - 20.0)
        self.assertAlmostEqual(padded.GetMax()[0], tight.GetMax()[0] + 20.0)
        self.assertAlmostEqual(padded.GetMax()[1], tight.GetMax()[1] + 20.0)


@unittest.skipUnless(_has_link_renderer, "Link renderer modules not available")
class LinkRendererPickTest(unittest.TestCase):
    @staticmethod
    def _link(start, end):
        return SimpleNamespace(start=Gf.Vec2d(*start), end=Gf.Vec2d(*end))

    # Cursor on the S-bow of a backward link with start=(500, 0), end=(0, 300).
    # The load-bearing property is that this point is OUTSIDE the straight
    # endpoint bbox (x > max(start.x, end.x) = 500), so a hit here can only
    # come from curve-aware picking, not from a chord test. The exact literal
    # (543.75, 60.9375) is a hand-picked sample on the shape produced by the
    # current buildLinkCurveShape/kLinkCurveHalfHeight/kLinkTransitionSpan
    # constants -- if those are retuned, this literal may fall outside the
    # ribbon and the two tests below will need a matching update.
    _BACKWARD_BOW_CURSOR = Gf.Vec2d(543.75, 60.9375)

    def test_broad_phase_finds_backward_link_on_bow(self):
        # Backward link whose S-bow bulges to x ~ 543, outside the straight
        # endpoint box. The cursor sits on the bulge (a true curve point); the
        # curve-inclusive index bounds plus the adaptive narrow-phase must catch
        # it even though the spatial query is keyed on bounds, not the chord.
        link = self._link((500.0, 0.0), (0.0, 300.0))
        # Pin the load-bearing "outside straight bbox" property so a
        # shape-constant retune that pulls the cursor back inside the chord
        # box surfaces as a clear failure here rather than as a silent -1 in
        # the picker.
        self.assertGreater(
            self._BACKWARD_BOW_CURSOR[0], max(link.start[0], link.end[0])
        )
        index = SpatialIndex(maxDepth=4, maxItems=20)
        index.insertLink(0, computeLinkCurveBounds(link, margin=20.0))

        hit = _make_picker().findLinkUnderCursor(
            self._BACKWARD_BOW_CURSOR, [link], 1.0, 12.0, spatialIndex=index
        )
        self.assertEqual(hit, 0)

    def test_broad_phase_misses_when_far(self):
        link = self._link((500.0, 0.0), (0.0, 300.0))
        index = SpatialIndex(maxDepth=4, maxItems=20)
        index.insertLink(0, computeLinkCurveBounds(link, margin=20.0))

        hit = _make_picker().findLinkUnderCursor(
            Gf.Vec2d(2000.0, 2000.0), [link], 1.0, 12.0, spatialIndex=index
        )
        self.assertEqual(hit, -1)

    def test_cache_reused_until_geometry_changes(self):
        link = self._link((0.0, 0.0), (200.0, 20.0))
        picker = _make_picker()
        segs1 = picker._cachedLinkSegments(link, 0)
        segs2 = picker._cachedLinkSegments(link, 0)
        self.assertIs(segs1, segs2)  # cached object reused, not re-sampled
        link.end = Gf.Vec2d(300.0, 20.0)  # geometry changed -> cache invalidated
        segs3 = picker._cachedLinkSegments(link, 0)
        self.assertIsNot(segs1, segs3)

    def test_no_spatial_index_falls_back_to_all_links(self):
        # When no spatial index is supplied, `_candidateLinkIndices` must fall
        # back to iterating every link so legacy non-indexed callers keep
        # working. Exercised end-to-end: the cursor sits on the S-bow of a
        # backward link (outside its straight endpoint box), and picking must
        # still find the link -- proving the fallback returned this link's
        # index despite no spatial pre-filtering.
        link = self._link((500.0, 0.0), (0.0, 300.0))
        # Same "outside straight bbox" pin as the broad-phase test above.
        self.assertGreater(
            self._BACKWARD_BOW_CURSOR[0], max(link.start[0], link.end[0])
        )
        hit = _make_picker().findLinkUnderCursor(
            self._BACKWARD_BOW_CURSOR, [link], 1.0, 12.0, spatialIndex=None
        )
        self.assertEqual(hit, 0)

    def test_candidate_indices_none_index_returns_full_range(self):
        # Pins the None path: with spatialIndex=None the method must return
        # `range(linkCount)` verbatim -- not an empty list, an int, or a
        # generator producing the same values. This does NOT discriminate a
        # truthiness-refactor (`if not spatialIndex:`) against the current
        # `is None`, since both are True for None; the docstring is scoped to
        # the None-input shape guarantee only.
        picker = _make_picker()
        # Positional to match production call at linkRenderer.py:231-233 so a
        # rename of a private parameter doesn't break the test spuriously.
        result = picker._candidateLinkIndices(Gf.Vec2d(0.0, 0.0), 5, 1.0, None)
        self.assertIsInstance(result, range)
        self.assertEqual(list(result), [0, 1, 2, 3, 4])

    def test_pick_radius_matches_visible_half_width_zoomed_in(self):
        # Zoomed in, the pick radius equals the drawn ribbon's visible half-width:
        # 0.5 * linkLineWidth (the 0.5 SDF cutoff). At linkLineWidth 12, zoom 1
        # that is 6 world units, so 5.9 world hits and 6.1 misses -- exact WYSIWYG.
        link = self._link((0.0, 0.0), (200.0, 0.0))
        near = _make_picker().findLinkUnderCursor(
            Gf.Vec2d(100.0, 5.9), [link], 1.0, 12.0
        )
        far = _make_picker().findLinkUnderCursor(
            Gf.Vec2d(100.0, 6.1), [link], 1.0, 12.0
        )
        self.assertEqual(near, 0)
        self.assertEqual(far, -1)

    def test_pick_radius_floored_when_zoomed_out(self):
        # Zoomed out, the visible half-width (0.5*12*0.1 = 0.6 px) is below the
        # 2px floor, so the radius is 2px = 20 world at zoom 0.1: 19 world hits,
        # 21 misses.
        link = self._link((0.0, 0.0), (200.0, 0.0))
        near = _make_picker().findLinkUnderCursor(
            Gf.Vec2d(100.0, 19.0), [link], 0.1, 12.0
        )
        far = _make_picker().findLinkUnderCursor(
            Gf.Vec2d(100.0, 21.0), [link], 0.1, 12.0
        )
        self.assertEqual(near, 0)
        self.assertEqual(far, -1)
