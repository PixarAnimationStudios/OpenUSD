#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Extra flag-set coverage for the per-frame link snapshot sync gate.

D109600612 gated the per-frame `graph.syncLinksFromModels(self.links)` marshal
behind `_linksNeedSync` / `_lastSyncedLinkHover`, and set `_linksNeedSync=True`
at every in-place link-field mutation site.  `testUsdNoodlesAutoLayout.py`
(`TestLinkSnapshotSyncGating`) already covers `_syncLinksIfNeeded` itself plus
the four simplest choke points (`clearLinkSelection`, `_selectLink`,
`_toggleLinkSelection`, `_updateLinksForMovedNodes`).

This file covers the remaining testable choke points, all via the same
`SimpleNamespace + GraphView.method(view, ...)` idiom used elsewhere in the
tests directory:

1. `_updateLinkSelectionFromNodes` (highlight sweep) — flag must be set BEFORE
   the `NODES_ONLY` early-return, otherwise a mode-switch that clears highlight
   flags on the Python-side snapshot without re-mirroring leaves C++ rendering
   stale highlights until the next unrelated mutation.
2. `_finishConnectionReplacement` (reconnect red-flash CLEAR) — flag must be
   set when the red highlight is cleared, otherwise the C++ snapshot renders
   the flash indefinitely.
3. `_addLinkToGraph` (incremental add) — flag must be set for the frame that
   picks up the new link; before D109600612 the unconditional per-frame
   marshal covered this, now it is explicit.
4. `_removeLinkFromGraph` (incremental remove) — symmetric to add.
5. `_updateMarqueeSelection` — flag must be set even on the shrink
   early-return path, because the deselect sweep above the early-return
   already mutated `link.selected` on the Python snapshot.

Two flag sites are intentionally NOT covered here (see
`metadata.untestable_paths` in the coverage-track findings): the reconnect
red-flash SET inside the 130+ line `_completeLinkDrag`, and the CLEAR inside
the 90+ line `_rebuildLinks`, both of which have too many collaborators to
exercise cheaply from a `SimpleNamespace` fake.
"""

from __future__ import annotations

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

try:
    from pxr.UsdNoodles.core import LinkSelectionMode
    from pxr.UsdNoodles.graphView import GraphView

    _has_graph = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"GraphView import failed: {e}")
    _has_graph = False


def _profiler_stub():
    """Minimal stub satisfying the _ProfileSection(self._profiler, "...") CM.

    _ProfileSection only calls `_profiler.recordTime(name, elapsed_ms)` in
    __exit__; a MagicMock swallows the call cleanly and lets the with-block
    exit normally.
    """
    return MagicMock()


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestUpdateLinkSelectionFromNodesFlagsResync(unittest.TestCase):
    """`_updateLinkSelectionFromNodes` mutates `link.highlighted` on every link
    (clears then optionally sets); the flag must be set so the C++ snapshot
    picks up the new highlight state on the next paint."""

    def test_flag_set_on_nodes_only_early_return(self):
        # The clearing loop runs BEFORE the NODES_ONLY early-return, so the
        # highlight flags on the Python snapshot were already mutated. The
        # gate MUST fire even though no highlight was subsequently set.
        link = SimpleNamespace(highlighted=True)
        view = SimpleNamespace(
            links=[link],
            _profiler=_profiler_stub(),
            _linksNeedSync=False,
            _linkSelectionMode=LinkSelectionMode.NODES_ONLY,
            nodes={},
        )
        GraphView._updateLinkSelectionFromNodes(view)
        self.assertFalse(link.highlighted)  # cleared by the sweep
        self.assertTrue(view._linksNeedSync)  # flag set despite early return

    def test_flag_set_when_highlight_reapplied(self):
        # Non-NODES_ONLY path: the sweep clears then re-highlights based on
        # node selection. The flag must be set exactly once (idempotent on
        # multiple calls is fine, but never left False).
        link = SimpleNamespace(
            highlighted=False,
            data_sourceNodeId="A",
            data_targetNodeId="B",
        )
        selected_node = SimpleNamespace(selected=True, id="A")
        unselected_node = SimpleNamespace(selected=False, id="B")
        view = SimpleNamespace(
            links=[link],
            _profiler=_profiler_stub(),
            _linksNeedSync=False,
            _linkSelectionMode=LinkSelectionMode.WITH_OUTPUTS,
            nodes={"A": selected_node, "B": unselected_node},
        )
        GraphView._updateLinkSelectionFromNodes(view)
        self.assertTrue(link.highlighted)  # source-selected under WITH_OUTPUTS
        self.assertTrue(view._linksNeedSync)


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestFinishConnectionReplacementFlagsResync(unittest.TestCase):
    """The red-flash CLEAR path in `_finishConnectionReplacement` mutates
    `oldLink.highlighted` and `oldLink.highlightColor` on the Python snapshot;
    without a re-sync the C++ render would keep drawing the flash until the
    next unrelated mutation."""

    def test_clear_flag_set_on_reconnect_flash_clear(self):
        old_link = SimpleNamespace(
            highlighted=True,
            highlightColor=(1.0, 0.0, 0.0, 1.0),
        )
        view = SimpleNamespace(
            _linksNeedSync=False,
            _createConnection=MagicMock(),
            _rebuildLinks=MagicMock(),
            linksChanged=False,
        )
        GraphView._finishConnectionReplacement(
            view,
            old_link,
            outputNodeId="out",
            outputPortName="p",
            inputNodeId="in",
            inputPortName="q",
        )
        self.assertFalse(old_link.highlighted)
        self.assertIsNone(old_link.highlightColor)
        self.assertTrue(view._linksNeedSync)
        # Sanity: the follow-up wiring still runs so we know we exercised the
        # real code path, not just the two setter lines above.
        view._createConnection.assert_called_once()
        view._rebuildLinks.assert_called_once()


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestAddLinkToGraphFlagsResync(unittest.TestCase):
    """The incremental add path (`_addLinkToGraph`) previously relied on the
    unconditional per-frame marshal. Post-gate it MUST set the flag itself."""

    def test_add_link_flags_resync(self):
        output_node = SimpleNamespace(outputLinks=[], inputLinks=[], id="src")
        input_node = SimpleNamespace(outputLinks=[], inputLinks=[], id="dst")
        view = SimpleNamespace(
            links=[],
            _linksNeedSync=False,
            _spatialIndex=SimpleNamespace(insertLink=MagicMock()),
            _computeLinkBounds=lambda link: None,
            _rebuildConnectionCache=MagicMock(),
            _setupLinkEndpoints=MagicMock(),
            _classifyRelationshipLinkFromUsd=lambda *a, **kw: None,
        )
        # _addLinkToGraph constructs a real LinkData() and calls
        # get_relationship_link_metadata; both live in noodles.core /
        # graphView module scope and work under the test harness (the
        # existing TestLinkSnapshotSyncGating exercises the same import
        # chain). If the underlying imports fail, the class-level skip
        # decorator will skip this test entirely.
        GraphView._addLinkToGraph(
            view,
            outputNodeId="src",
            outputPort="out",
            inputNodeId="dst",
            inputPort="in",
            outputNode=output_node,
            inputNode=input_node,
        )
        self.assertEqual(len(view.links), 1)  # link was appended
        self.assertTrue(view._linksNeedSync)


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestRemoveLinkFromGraphFlagsResync(unittest.TestCase):
    """Symmetric to add — incremental remove must flag a re-sync."""

    def test_remove_link_flags_resync(self):
        link = SimpleNamespace(
            sourceNodeId="src",
            sourcePort="out",
            targetNodeId="dst",
            targetPort="in",
        )
        output_node = SimpleNamespace(outputLinks=[link], inputLinks=[])
        input_node = SimpleNamespace(outputLinks=[], inputLinks=[link])
        view = SimpleNamespace(
            links=[link],
            nodes={"src": output_node, "dst": input_node},
            _linksNeedSync=False,
            _spatialIndex=SimpleNamespace(
                removeLink=MagicMock(),
                insertLink=MagicMock(),
            ),
            _computeLinkBounds=lambda link: None,
            _rebuildConnectionCache=MagicMock(),
            _makeSerializedLinkRecord=lambda *a, **kw: "record-key",
            _linkMatchesRecord=lambda link, record: True,
            update=MagicMock(),
        )
        GraphView._removeLinkFromGraph(
            view,
            outputNodeId="src",
            outputPort="out",
            inputNodeId="dst",
            inputPort="in",
        )
        self.assertEqual(view.links, [])  # link was removed
        self.assertTrue(view._linksNeedSync)

    def test_early_return_no_match_does_not_flag(self):
        # When _linkMatchesRecord returns False for every candidate, the
        # function returns before touching self.links. It also (correctly)
        # does not flag a resync — nothing on the snapshot changed. This
        # pins the early-return behavior so a future refactor cannot start
        # setting the flag unconditionally at method entry.
        link = SimpleNamespace(sourceNodeId="src")
        view = SimpleNamespace(
            links=[link],
            nodes={},
            _linksNeedSync=False,
            _makeSerializedLinkRecord=lambda *a, **kw: "record-key",
            _linkMatchesRecord=lambda link, record: False,
        )
        GraphView._removeLinkFromGraph(
            view,
            outputNodeId="src",
            outputPort="out",
            inputNodeId="dst",
            inputPort="in",
        )
        self.assertEqual(len(view.links), 1)  # nothing removed
        self.assertFalse(view._linksNeedSync)  # nothing changed -> no flag


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestUpdateMarqueeSelectionFlagsResync(unittest.TestCase):
    """`_updateMarqueeSelection` clears link-selected flags on the Python
    snapshot before the shrink-early-return; the flag MUST be set even on
    that early-return path, otherwise a shrink below `minMarqueeSize` after
    a previous select leaves C++ rendering the stale selection."""

    def test_flag_set_on_shrink_early_return(self):
        # Zoom=1 -> minMarqueeSize=2.0; marqueeStart == marqueeCurrent gives
        # dx=dy=0 which falls into the shrink early-return branch. The
        # deselect sweep above still ran (self.links[0].selected was cleared
        # because 0 is in _marqueePrevSelectedLinks but not in the base),
        # so the flag MUST be set.
        link = SimpleNamespace(selected=True)
        point = SimpleNamespace(x=lambda: 0.0, y=lambda: 0.0)
        view = SimpleNamespace(
            marqueeActive=True,
            links=[link],
            nodes={},
            zoom=1.0,
            marqueeStart=point,
            marqueeCurrent=point,
            _selectedNodes=set(),
            _selectedLinks={0},
            _marqueePrevSelectedNodes=set(),
            _marqueePrevSelectedLinks={0},
            _marqueeBaseSelectedNodes=set(),
            _marqueeBaseSelectedLinks=set(),
            _linksNeedSync=False,
        )
        GraphView._updateMarqueeSelection(view)
        self.assertFalse(link.selected)  # sweep cleared it
        self.assertTrue(view._linksNeedSync)  # flag set despite early return

    def test_early_return_when_marquee_inactive(self):
        # The marqueeActive=False guard runs before the deselect sweep, so
        # nothing on the snapshot changed and the flag stays False. Pins
        # the guard so a future refactor cannot start unconditionally
        # flagging on every mouse-move.
        view = SimpleNamespace(
            marqueeActive=False,
            _linksNeedSync=False,
        )
        GraphView._updateMarqueeSelection(view)
        self.assertFalse(view._linksNeedSync)
