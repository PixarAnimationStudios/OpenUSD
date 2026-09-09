#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Regression tests for D109369586 — auto-grid layout for nodes at USD default
(0, 0), display-only position setter, and node-count-scaled overlap-avoidance
loop.

Covers three behaviors that shipped with only single-node happy-path
coverage:

1. ``GraphView._gridPlaceNodes`` multi-row/multi-column layout:
     * anchor at world origin on a fresh load,
     * anchor below the bounding box of already-placed nodes when adding
       into an arranged graph,
     * uniform column width sized to the batch's widest node,
     * per-row height sized to THAT row's tallest node (a single tall node
       must not inflate every row's vertical spacing — this is called out
       explicitly in the docstring and diff summary).
2. ``NodeModel.setDisplayPosition`` display-only contract: it must update
   the cached ``_position`` and the C++-side ``NodeData`` position WITHOUT
   authoring ``ui:nodegraph:node:pos`` (the whole point of the setter is
   that auto-placement never dirties a stage on open, and never fails on
   a prim that won't accept the attribute).
3. ``GraphView._positionNodeInViewport`` overlap-avoidance loop cap: the
   change from a fixed ``range(50)`` to ``range(len(self.nodes) + 1)``
   must actually let the search clear more than 50 positions when the
   graph has >50 nodes (the fixed cap was the bug being fixed).

These are pure-Python behaviors on plain-data objects — no Application,
JobSystem, or AppContext fixture (see bot_generated_regression_tests.md).
"""

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

try:
    from pxr import Gf
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.UsdNoodles.models import _cpp_position, NodeModel

    _has_pxr = True
except ImportError:
    _has_pxr = False


def _grid_node(node_id, position=(0.0, 0.0), size=(100.0, 50.0)):
    """A minimal node stand-in that satisfies _gridPlaceNodes.

    _gridPlaceNodes reads .id, .position, .size and calls
    .setDisplayPosition(vec). setDisplayPosition mirrors the assigned vector
    back into .position so the mock stays behaviourally equivalent to the
    real NodeModel setter for any read-after-write path — matches the
    contract that TestSetDisplayPositionNoUsdAuthor asserts on the real
    NodeModel in this same file.
    """
    node = SimpleNamespace(id=node_id, position=position, size=size)
    node.setDisplayPosition = MagicMock(
        side_effect=lambda v: setattr(node, "position", (v[0], v[1]))
    )
    return node


def _grid_view(nodes_by_id):
    return SimpleNamespace(
        nodes=dict(nodes_by_id),
        _AUTO_GRID_COLUMNS=GraphView._AUTO_GRID_COLUMNS,
        _AUTO_GRID_GAP=GraphView._AUTO_GRID_GAP,
        # Semantically-correct mock: nodes at (0, 0) are "unauthored" (placeable),
        # nodes at any other position are "authored" (already placed). The landed
        # graphView.py:2356-2373 classifies by `position == (0,0)` directly and
        # never calls _nodeHasAuthoredPosition, so this is a no-op today. It
        # future-proofs against the follow-up (D110290587) that gates the
        # placeable predicate on _nodeHasAuthoredPosition: with position-based
        # side_effect, an existing (10, 200) node correctly reports authored=True
        # so the anchor-below-existing branch still exercises real behavior.
        _nodeHasAuthoredPosition=MagicMock(
            side_effect=lambda n: n.position != (0.0, 0.0)
        ),
    )


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestGridPlaceMultiRow(unittest.TestCase):
    """Verify _gridPlaceNodes wraps into rows and sizes columns/rows correctly."""

    def test_batch_larger_than_column_count_wraps_to_next_row(self):
        """With more than _AUTO_GRID_COLUMNS placeable nodes, the (COLS+1)th
        node must land on the second row at column 0 (x back to anchorX),
        not at column COLS on the first row. Guards the row-wrap arithmetic."""
        cols = GraphView._AUTO_GRID_COLUMNS
        # cols+1 nodes so exactly one wraps to row 2.
        nodes = [_grid_node(f"/N{i}") for i in range(cols + 1)]
        view = _grid_view({n.id: n for n in nodes})

        placed = GraphView._gridPlaceNodes(view, nodes)

        self.assertEqual(len(placed), cols + 1)
        # First node sits at the world-origin anchor.
        first_call_arg = nodes[0].setDisplayPosition.call_args[0][0]
        self.assertEqual((first_call_arg[0], first_call_arg[1]), (0.0, 0.0))
        # Node at index cols wrapped to row 2 → x resets to anchorX (0.0),
        # y is one row-height + gap below the top row.
        wrap_arg = nodes[cols].setDisplayPosition.call_args[0][0]
        self.assertEqual(wrap_arg[0], 0.0)
        # First row is 50 tall (all nodes 50), plus one _AUTO_GRID_GAP.
        self.assertEqual(wrap_arg[1], 50.0 + GraphView._AUTO_GRID_GAP)

    def test_uniform_column_width_uses_widest_node_in_batch(self):
        """Column stride is (widest node in batch) + gap, applied to EVERY
        column. Here narrow sits in column 0 (x=0.0) and wide sits in column
        1 at x=cellW=(max width + _AUTO_GRID_GAP), NOT at (narrow width +
        gap) as a per-node stride would produce. Guards the cellW =
        max(size[0]) contract."""
        narrow = _grid_node("/Narrow", size=(50.0, 50.0))
        wide = _grid_node("/Wide", size=(300.0, 50.0))
        view = _grid_view({"/Narrow": narrow, "/Wide": wide})

        GraphView._gridPlaceNodes(view, [narrow, wide])

        narrow_x = narrow.setDisplayPosition.call_args[0][0][0]
        wide_x = wide.setDisplayPosition.call_args[0][0][0]
        # Column stride = max(width) + _AUTO_GRID_GAP.
        self.assertEqual(narrow_x, 0.0)
        self.assertEqual(wide_x, 300.0 + GraphView._AUTO_GRID_GAP)

    def test_per_row_height_uses_tallest_node_in_that_row_only(self):
        """A tall node in row 2 must NOT retroactively push row 2's own y
        down — its y is anchored by row 1's height, which is all-shorts.
        This distinguishes per-row height sizing from a batch-max mutation:
          per-row       : row2.y = max(row1)+_AUTO_GRID_GAP (short-based)
          batch-max     : row2.y = max(batch)+_AUTO_GRID_GAP (tall-based)
        Row 1 is all shorts (height 50); row 2 contains a tall node (500).
        Pinning row 2's y to the short-based value catches the mutation
        where the y-step uses the batch-wide max instead of the previous
        row's.
        """
        cols = GraphView._AUTO_GRID_COLUMNS
        # Row 1: cols short (50) nodes.
        row1 = [_grid_node(f"/R1S{i}", size=(100.0, 50.0)) for i in range(cols)]
        # Row 2 col 0: one tall (500) node; the rest short.
        row2_tall = _grid_node("/R2Tall", size=(100.0, 500.0))
        batch = [*row1, row2_tall]
        view = _grid_view({n.id: n for n in batch})

        GraphView._gridPlaceNodes(view, batch)

        row2_tall_y = row2_tall.setDisplayPosition.call_args[0][0][1]
        # y = max(row1)+_AUTO_GRID_GAP. If per-row height sizing broke to
        # batch-wide max instead, this would be 500+_AUTO_GRID_GAP.
        self.assertEqual(row2_tall_y, 50.0 + GraphView._AUTO_GRID_GAP)

    def test_anchors_below_existing_placed_nodes(self):
        """When the graph already has non-origin nodes, the new batch anchors
        just below their bounding box (min x, max (y+height) + gap), not at
        world origin — the diff's 'never lands on top of existing content'
        promise."""
        existing = _grid_node("/Existing", position=(10.0, 200.0), size=(100.0, 30.0))
        new_node = _grid_node("/New")  # at (0, 0), placeable
        view = _grid_view({existing.id: existing, new_node.id: new_node})

        GraphView._gridPlaceNodes(view, [new_node])

        anchor = new_node.setDisplayPosition.call_args[0][0]
        # anchorX = min of existing.x = 10; anchorY = max(y+h) + _AUTO_GRID_GAP.
        self.assertEqual(anchor[0], 10.0)
        self.assertEqual(anchor[1], 230.0 + GraphView._AUTO_GRID_GAP)

    def test_empty_placeable_returns_early_without_placement(self):
        """If no node in the batch is at (0, 0), _gridPlaceNodes returns [] and
        touches no setter. Guards the early-out that keeps arranged graphs
        stable on subsequent load/add."""
        authored = _grid_node("/Authored", position=(50.0, 60.0))
        view = _grid_view({authored.id: authored})

        placed = GraphView._gridPlaceNodes(view, [authored])

        self.assertEqual(placed, [])
        authored.setDisplayPosition.assert_not_called()


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestSetDisplayPositionNoUsdAuthor(unittest.TestCase):
    """NodeModel.setDisplayPosition must update the cache and the C++-side
    position but MUST NOT write ui:nodegraph:node:pos — the entire point of
    the setter is that auto-layout can't dirty a stage on open and can't
    fail on a prim that rejects the attribute (e.g. instance proxies)."""

    def _make_node(self):
        node = NodeModel()  # No stage/prim → pure-Python path, no USD writes.
        node._position = None
        return node

    def test_updates_python_cache(self):
        node = self._make_node()
        node.setDisplayPosition(Gf.Vec2d(42.0, 99.0))
        self.assertEqual((node._position[0], node._position[1]), (42.0, 99.0))

    def test_does_not_call_write_position_to_usd(self):
        """The regular position setter writes to USD; setDisplayPosition must
        not. Spy on _writePositionToUsd and assert it stays untouched."""
        node = self._make_node()
        node._writePositionToUsd = MagicMock()
        node._writePositionToUsdRaw = MagicMock()
        node.setDisplayPosition(Gf.Vec2d(1.0, 2.0))
        node._writePositionToUsd.assert_not_called()
        node._writePositionToUsdRaw.assert_not_called()

    def test_reads_back_through_position_property_when_no_prim(self):
        """After setDisplayPosition, reading .position must return the value
        just set (cache hit path), not fall through to a USD read. Guards
        against a future refactor that clears _position or bypasses the
        cache."""
        node = self._make_node()
        node.setDisplayPosition(Gf.Vec2d(7.0, 8.0))
        pos = node.position
        self.assertEqual((pos[0], pos[1]), (7.0, 8.0))

    def test_updates_cpp_side_node_data_position(self):
        """setDisplayPosition must forward the value to the C++-side NodeData
        base so the render snapshot mirrors the Python cache. A regression
        that drops the descriptor call would pass every test that only
        asserts on ``_position`` but silently break connected-noodle
        rendering (the C++ side keeps the stale position).

        Reads through the module-level ``_cpp_position`` descriptor exported
        by ``pxr.UsdNoodles.models`` (the same ``NodeData.__dict__['position']``
        that ``models.py`` binds at import): MagicMock does not reliably
        support the descriptor-protocol ``__set__`` assertion pattern, and
        reading the C++ side directly verifies the observable contract
        instead of the mechanism. The named-import form avoids coupling
        the test to ``NodeModel.__mro__[1]`` positional order, which would
        silently break under a future change to the class hierarchy."""
        node = self._make_node()
        node.setDisplayPosition(Gf.Vec2d(11.0, 22.0))
        cpp_value = _cpp_position.__get__(node)
        self.assertEqual((cpp_value[0], cpp_value[1]), (11.0, 22.0))


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestPositionInViewportOverlapLoopScales(unittest.TestCase):
    """The overlap-avoidance loop cap changed from range(50) to
    range(len(self.nodes) + 1). Verify the loop can actually clear an
    overlap that requires more than 50 steps — the fixed-50 cap was the
    bug being fixed and was called out as untested in the pre-land review
    (comment 4281972028783604)."""

    # Chosen to exceed the pre-fix range(50) cap by a small margin so the
    # regression is exercised but the test still runs quickly. This is a
    # test-local constant unrelated to GraphView._AUTO_GRID_COLUMNS: the
    # overlap loop under test is _positionNodeInViewport, not the grid
    # layout, so its coverage must not couple to a UI knob that lives in
    # the grid-placement code path.
    N_EXISTING_FOR_OVERLAP_TEST = 55

    def _make_view_with_row_of_nodes(self, n):
        """Build a view whose viewport centers at (400, 300) and has n
        existing nodes lined up horizontally starting at the drop point,
        so the overlap-nudge has to shift the new node past all n.

        The nudge step per iteration is (existing.width + padding) =
        100 + 30 = 130 in x. With n existing nodes, the loop needs at
        least n iterations to clear them.
        """
        # New node size 100x50; viewport 800x600 centered → drop at (350, 275).
        drop_x, drop_y = 350.0, 275.0
        step = 100.0 + 30.0  # width + padding
        existing_nodes = {}
        for i in range(n):
            ex = drop_x + i * step
            existing_nodes[f"/E{i}"] = SimpleNamespace(
                position=(ex, drop_y), size=(100.0, 50.0)
            )

        view = SimpleNamespace(
            nodes=existing_nodes,
            width=MagicMock(return_value=800),
            height=MagicMock(return_value=600),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _nodeHasAuthoredPosition=MagicMock(return_value=False),
            # Defensive: the follow-up geometry cap for _positionNodeInViewport
            # reads self._AUTO_GRID_COLUMNS. Python evaluates both min() args,
            # so the mock must expose it even for landed-state tests.
            _AUTO_GRID_COLUMNS=GraphView._AUTO_GRID_COLUMNS,
            _AUTO_GRID_GAP=GraphView._AUTO_GRID_GAP,
        )
        return view

    def test_loop_clears_overlap_beyond_50_nodes(self):
        """With enough existing nodes lined up that each new candidate x still
        overlaps, the fixed range(50) cap would leave the new node on top
        of the 50th existing node. The scaled cap must clear them all.
        """
        n_existing = self.N_EXISTING_FOR_OVERLAP_TEST
        view = self._make_view_with_row_of_nodes(n_existing)
        new_node = SimpleNamespace(size=(100.0, 50.0), position=(0.0, 0.0))

        GraphView._positionNodeInViewport(view, new_node)

        # After clearing, new_node.x must be to the right of the last
        # existing node. Last existing x = 350 + (n-1)*130.
        last_existing_right = 350.0 + (n_existing - 1) * 130.0 + 100.0
        self.assertGreaterEqual(new_node.position[0], last_existing_right)

    def test_single_non_overlapping_existing_node_iterates_at_least_once(self):
        """With one existing non-overlapping node the loop MUST execute at
        least one iteration to inspect it before settling. Distinguishes
        ``range(0)`` (no iterations — new node dropped without ever reading
        the existing node's position) from ``range(len(self.nodes) + 1)``
        (at least one iteration — reads existing node's position, finds no
        overlap, settles at viewport-center).

        `_nodeHasAuthoredPosition` cannot be used as the iteration witness:
        `_positionNodeInViewport` calls it once at the top on the *new*
        node (see graphView.py `_positionNodeInViewport`), never on existing
        nodes inside the loop. The observable per-iteration signal is
        `existingNode.position` — read twice per iteration on line
        `ex, ey = existingNode.position[0], existingNode.position[1]`. A
        recording `.position` property counts those reads and is the only
        witness that actually distinguishes `range(0)` from `range(1+)`."""

        class _RecordingPositionNode:
            """SimpleNamespace-like stand-in that counts reads of `.position`.

            `.size` remains a plain attribute; only `.position` reads are
            witnessed, so the counter reflects only overlap-loop iterations
            (each iteration reads `.position` on every existing node)."""

            def __init__(self, pos, size):
                self._pos = pos
                self.size = size
                self.position_reads = 0

            @property
            def position(self):
                self.position_reads += 1
                return self._pos

        far_existing = _RecordingPositionNode(
            pos=(10_000.0, 10_000.0), size=(100.0, 50.0)
        )
        view = SimpleNamespace(
            nodes={"/Far": far_existing},
            width=MagicMock(return_value=800),
            height=MagicMock(return_value=600),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _nodeHasAuthoredPosition=MagicMock(return_value=False),
            # Defensive: follow-up geometry cap reads _AUTO_GRID_COLUMNS
            # eagerly via min(); expose it for the single-node case.
            _AUTO_GRID_COLUMNS=GraphView._AUTO_GRID_COLUMNS,
            _AUTO_GRID_GAP=GraphView._AUTO_GRID_GAP,
        )
        new_node = SimpleNamespace(size=(100.0, 50.0), position=(0.0, 0.0))

        GraphView._positionNodeInViewport(view, new_node)

        # (800/2 - 50, 600/2 - 25) = (350, 275) — the far-away existing
        # node does not overlap so the drop point is unchanged.
        self.assertEqual(new_node.position[0], 350.0)
        self.assertEqual(new_node.position[1], 275.0)
        # Each loop iteration reads existingNode.position twice (once for
        # ex, once for ey). >=2 reads proves the loop ran at least once
        # over the existing node — a `range(0)` regression would leave
        # this counter at 0.
        self.assertGreaterEqual(
            far_existing.position_reads,
            2,
            "overlap loop did not iterate over existing nodes — "
            "likely a range(0) regression",
        )
