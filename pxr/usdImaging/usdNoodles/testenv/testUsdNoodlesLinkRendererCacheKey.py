#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from __future__ import annotations

import unittest
from unittest.mock import MagicMock, patch

# Match sibling testUsdNoodlesLinkRenderer.py's try/except + skipUnless
# pattern. A missing module here is a BUCK target misconfiguration; the
# canonical signal is the build dep failing to load, not a runtime
# ImportError split across two files in the same target. Hard-importing
# while the sibling silently skips would emit inconsistent CI signals
# for the same regression class.
try:
    from pxr.UsdNoodles.render import LinkRenderManager as _RealLinkRenderManager
    from pxr.UsdNoodles import linkRenderer as link_renderer_module

    _has_link_renderer = True
except ImportError as e:
    print(f"link renderer imports failed: {e}")
    _RealLinkRenderManager = None
    link_renderer_module = None
    _has_link_renderer = False


class _FakeProjection:
    """Minimal stand-in for a Gf.Matrix4d-like projection — only `.data()` is used by the wrappers."""

    def __init__(self, values):
        self._values = list(values)

    def data(self):
        return self._values


def _total_arity(call_args):
    """Total parameters that reached the binding, regardless of positional/keyword form."""
    return len(call_args.args) + len(call_args.kwargs)


def _get_param(call_args, name, positional_index):
    """Read a parameter from a mock call, accepting positional OR keyword forwarding.

    The contract under test is "the parameter reaches the binding"; whether
    the wrapper forwards positionally or as a keyword is a private
    implementation detail that a future refactor is free to change without
    breaking the dirty-gate optimization.
    """
    if name in call_args.kwargs:
        return call_args.kwargs[name]
    if positional_index < len(call_args.args):
        return call_args.args[positional_index]
    raise AssertionError(
        f"parameter {name!r} not found in call (positional index "
        f"{positional_index} out of range; kwargs keys = "
        f"{sorted(call_args.kwargs)})"
    )


@unittest.skipUnless(_has_link_renderer, "Link renderer modules not available")
class LinkRendererCacheKeyForwardingTest(unittest.TestCase):
    """Pin the cacheKey hand-off from LinkRenderer (Python) to LinkRenderManager (C++).

    Covers both Python wrappers that forward cacheKey to the binding:

    - `renderLinks` — drag-temp link path (graphView paintLinks cacheKey=2)
    - `renderLinksFromGraph` — production paintLinks-regular (cacheKey=0)
      and paintLinks-relationship (cacheKey=1) paths

    The dirty-gate optimization in LinkRenderManager keys its instance VBOs
    by cacheKey to disambiguate independent call sites that share the
    manager within a frame. If a future refactor drops the kwarg, swaps
    positional args around it, or silently consumes-but-fails-to-forward
    it, every call site collapses onto a shared bucket per
    (drawSelected, LOD) and overwrites the others' cached bytes — turning
    every gate check into a miss without any diagnostic.
    """

    # renderLinks: 15 params; cacheKey at index 14, drawSelected at index 9.
    _RENDER_LINKS_ARITY = 15
    _RL_CACHE_KEY_POS = 14
    _RL_DRAW_SELECTED_POS = 9

    # renderLinksFromGraph: 16 params (relationshipOnly inserted at index
    # 10 between drawSelected and the color args); cacheKey at index 15.
    _RENDER_LINKS_FROM_GRAPH_ARITY = 16
    _RLFG_CACHE_KEY_POS = 15
    _RLFG_DRAW_SELECTED_POS = 9
    _RLFG_RELATIONSHIP_ONLY_POS = 10

    def setUp(self):
        # Hold the patch for the entire test. Using `patch.object` as a
        # short-lived context manager would restore the real binding before
        # the test body runs.
        self._patcher = patch.object(link_renderer_module, "LinkRenderManager")
        self._mock_cls = self._patcher.start()
        self.addCleanup(self._patcher.stop)
        # spec= against the real class catches ATTRIBUTE drift (a removed
        # method on the binding raises AttributeError on access). It does
        # NOT catch call-signature drift: boost.python wrappers are not
        # introspectable Python functions, so MagicMock cannot validate
        # arity or argument names against them. Call shape is pinned
        # explicitly via the per-method _assert_*_call_shape helpers.
        self._mock_instance = MagicMock(spec=_RealLinkRenderManager)
        self._mock_cls.return_value = self._mock_instance

    def _make_renderer(self):
        # Use the public `initialize()` rather than poking the private
        # `_initialized` flag. The patched binding turns the inner
        # initialize() call into a no-op mock; coupling tests to the
        # private flag would mask a regression where initialize() forgets
        # to set it.
        renderer = link_renderer_module.LinkRenderer()
        renderer.initialize(shaderLibrary=object())
        return renderer

    def _common_args(self):
        # 16 floats — matches the binding's len(pyProjection) >= 16 guard.
        proj = _FakeProjection([1.0] * 16)
        # Non-empty links — the wrapper short-circuits on empty links.
        links = [object()]
        return links, proj

    def _assert_renderLinks_call_shape(
        self, call_args, expected_cacheKey, expected_drawSelected
    ):
        """Pin arity, cacheKey, and drawSelected; accept positional OR keyword forwarding."""
        self.assertEqual(
            _total_arity(call_args),
            self._RENDER_LINKS_ARITY,
            f"renderLinks must take exactly {self._RENDER_LINKS_ARITY} "
            f"params; got {_total_arity(call_args)}",
        )
        self.assertEqual(
            _get_param(call_args, "cacheKey", self._RL_CACHE_KEY_POS),
            expected_cacheKey,
        )
        self.assertEqual(
            _get_param(call_args, "drawSelected", self._RL_DRAW_SELECTED_POS),
            expected_drawSelected,
        )

    def _assert_renderLinksFromGraph_call_shape(
        self,
        call_args,
        expected_cacheKey,
        expected_drawSelected,
        expected_relationshipOnly,
    ):
        self.assertEqual(
            _total_arity(call_args),
            self._RENDER_LINKS_FROM_GRAPH_ARITY,
            f"renderLinksFromGraph must take exactly "
            f"{self._RENDER_LINKS_FROM_GRAPH_ARITY} params; got "
            f"{_total_arity(call_args)}",
        )
        self.assertEqual(
            _get_param(call_args, "cacheKey", self._RLFG_CACHE_KEY_POS),
            expected_cacheKey,
        )
        self.assertEqual(
            _get_param(call_args, "drawSelected", self._RLFG_DRAW_SELECTED_POS),
            expected_drawSelected,
        )
        self.assertEqual(
            _get_param(
                call_args,
                "relationshipOnly",
                self._RLFG_RELATIONSHIP_ONLY_POS,
            ),
            expected_relationshipOnly,
        )

    # --- renderLinks: drag-temp wrapper (graphView paintLinks cacheKey=2) ---

    def test_renderLinks_cacheKey_default_is_zero(self):
        renderer = self._make_renderer()
        links, proj = self._common_args()

        renderer.renderLinks(
            links=links,
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
        )

        self.assertEqual(self._mock_instance.renderLinks.call_count, 1)
        self._assert_renderLinks_call_shape(
            self._mock_instance.renderLinks.call_args,
            expected_cacheKey=0,
            expected_drawSelected=False,
        )

    def test_renderLinks_cacheKey_explicit_value_forwarded(self):
        renderer = self._make_renderer()
        links, proj = self._common_args()

        renderer.renderLinks(
            links=links,
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            cacheKey=7,
        )

        self._assert_renderLinks_call_shape(
            self._mock_instance.renderLinks.call_args,
            expected_cacheKey=7,
            expected_drawSelected=False,
        )

    def test_renderLinks_drawSelected_round_trips_distinctly_from_cacheKey(self):
        # internalKey = cacheKey * 2 + drawSelected. Both halves must reach
        # the binding distinctly; a refactor that swapped their positions
        # would silently collapse independent buckets together.
        renderer = self._make_renderer()
        links, proj = self._common_args()

        renderer.renderLinks(
            links=links,
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            drawSelected=True,
            cacheKey=4,
        )

        self._assert_renderLinks_call_shape(
            self._mock_instance.renderLinks.call_args,
            expected_cacheKey=4,
            expected_drawSelected=True,
        )

    def test_renderLinks_empty_links_short_circuits_before_binding_call(self):
        renderer = self._make_renderer()
        _, proj = self._common_args()

        renderer.renderLinks(
            links=[],
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            cacheKey=5,
        )

        self._mock_instance.renderLinks.assert_not_called()

    def test_renderLinks_uninitialized_renderer_short_circuits(self):
        # Construction without initialize() must not forward to the binding,
        # regardless of cacheKey.
        renderer = link_renderer_module.LinkRenderer()
        # Intentionally NOT calling initialize().

        links, proj = self._common_args()
        renderer.renderLinks(
            links=links,
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            cacheKey=3,
        )

        self._mock_instance.renderLinks.assert_not_called()

    # --- renderLinksFromGraph: production paintLinks-regular/relationship paths ---

    def test_renderLinksFromGraph_cacheKey_default_is_zero(self):
        renderer = self._make_renderer()
        _, proj = self._common_args()

        renderer.renderLinksFromGraph(
            graph=object(),
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            relationshipOnly=False,
        )

        self.assertEqual(self._mock_instance.renderLinksFromGraph.call_count, 1)
        self._assert_renderLinksFromGraph_call_shape(
            self._mock_instance.renderLinksFromGraph.call_args,
            expected_cacheKey=0,
            expected_drawSelected=False,
            expected_relationshipOnly=False,
        )

    def test_renderLinksFromGraph_paintLinks_call_shape_round_trips(self):
        # Models the production paintLinks call shape:
        #   paintLinks-regular:      relationshipOnly=False, cacheKey=0
        #   paintLinks-relationship: relationshipOnly=True,  cacheKey=1
        # Each must round-trip distinctly to keep the dirty-gate buckets
        # independent. A refactor that collapsed cacheKey forwarding here
        # would silently overwrite one bucket from the other on every frame.
        renderer = self._make_renderer()
        _, proj = self._common_args()

        for relationshipOnly, cacheKey in ((False, 0), (True, 1)):
            renderer.renderLinksFromGraph(
                graph=object(),
                projection=proj,
                zoom=1.0,
                panX=0.0,
                panY=0.0,
                width=800,
                height=600,
                config=object(),
                relationshipOnly=relationshipOnly,
                cacheKey=cacheKey,
            )

        calls = self._mock_instance.renderLinksFromGraph.call_args_list
        self.assertEqual(len(calls), 2)
        forwarded = [
            (
                _get_param(
                    c,
                    "relationshipOnly",
                    self._RLFG_RELATIONSHIP_ONLY_POS,
                ),
                _get_param(c, "cacheKey", self._RLFG_CACHE_KEY_POS),
            )
            for c in calls
        ]
        self.assertEqual(forwarded, [(False, 0), (True, 1)])
        # Confirm arity is stable across both calls.
        for c in calls:
            self.assertEqual(_total_arity(c), self._RENDER_LINKS_FROM_GRAPH_ARITY)

    def test_renderLinksFromGraph_drawSelected_round_trips(self):
        # internalKey = cacheKey * 2 + drawSelected on the C++ side; both
        # halves must reach renderLinksFromGraph's binding distinctly.
        renderer = self._make_renderer()
        _, proj = self._common_args()

        renderer.renderLinksFromGraph(
            graph=object(),
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            relationshipOnly=False,
            drawSelected=True,
            cacheKey=6,
        )

        self._assert_renderLinksFromGraph_call_shape(
            self._mock_instance.renderLinksFromGraph.call_args,
            expected_cacheKey=6,
            expected_drawSelected=True,
            expected_relationshipOnly=False,
        )

    def test_renderLinksFromGraph_uninitialized_renderer_short_circuits(self):
        renderer = link_renderer_module.LinkRenderer()
        _, proj = self._common_args()

        renderer.renderLinksFromGraph(
            graph=object(),
            projection=proj,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=800,
            height=600,
            config=object(),
            relationshipOnly=False,
            cacheKey=2,
        )

        self._mock_instance.renderLinksFromGraph.assert_not_called()
