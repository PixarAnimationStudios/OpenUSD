#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


from __future__ import annotations

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

try:
    from pxr import Gf
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.UsdNoodles.nodeGraphBlueprint import (
        _append_relationship_links as append_blueprint_relationship_links,
    )
    from pxr.UsdNoodles.nodeGraphBpContainer import (
        _append_relationship_links as append_container_relationship_links,
    )
    from pxr.UsdNoodles.nodeGraphStage import (
        _append_collected_links as append_stage_collected_links,
    )
    from pxr.UsdNoodles.pinUtils import RELATIONSHIP_LINK_BASE_COLOR
    from pxr.Usdviewq.qt import QtCore

    _has_graph = True
except ImportError as e:
    print(f"relationship graph imports failed: {e}")
    _has_graph = False


class _Projection:
    def data(self):
        return [1.0] * 16


def _fake_node_vertex(*args):
    return SimpleNamespace(x=args[0], y=args[1], z=args[2], raw=args)


def _relationship_link():
    return SimpleNamespace(
        sourceNodeId="/Constraint",
        sourcePort="affects",
        targetNodeId="/Driven",
        targetPort="affects",
        sourcePropertyName="affects",
        targetPropertyName="",
        start=Gf.Vec2d(0.0, 0.0),
        end=Gf.Vec2d(20.0, 0.0),
        hovered=False,
        selected=False,
        is_relationship_link=True,
        isDangling=False,
    )


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class GraphViewRelationshipHelpersTest(unittest.TestCase):
    def test_populate_node_links_from_prim_copies_metadata(self):
        node = SimpleNamespace(inputLinks=["stale"], outputLinks=["stale"])
        view = SimpleNamespace(
            _makeLinkFromUsdData=GraphView._makeLinkFromUsdData,
        )

        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "sources",
                    "targetNodeId": "/Driver",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "",
                },
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "affects",
                    "targetNodeId": "/Driven",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "affects",
                    "is_relationship_link": True,
                    "sourcePropertyName": "affects",
                    "targetPropertyName": "",
                },
            ],
        ):
            GraphView._populateNodeLinksFromPrim(view, node, MagicMock())

        self.assertEqual(len(node.inputLinks), 0)
        self.assertEqual(len(node.outputLinks), 2)
        self.assertEqual(node.outputLinks[0].propertyName, "sources")
        self.assertEqual(node.outputLinks[0].sourcePropertyName, "sources")
        self.assertEqual(node.outputLinks[1].sourcePropertyName, "affects")
        self.assertTrue(node.outputLinks[0].is_relationship_link)

    def test_rebuild_links_uses_shared_population_helper(self):
        prim = MagicMock()
        prim.IsValid.return_value = True
        node = SimpleNamespace(
            id="/Node",
            _prim=prim,
            inputLinks=[],
            outputLinks=[],
            position=Gf.Vec2d(0.0, 0.0),
            size=Gf.Vec2d(10.0, 10.0),
        )
        view = SimpleNamespace(
            links=[],
            nodes={"/Node": node},
            nodeGraph=SimpleNamespace(syncLinksFromModels=MagicMock()),
            _populateNodeLinksFromPrim=MagicMock(),
            _surfaceConnectionEndpointPins=MagicMock(),
            _processNodeLinks=MagicMock(),
            _addBackReferenceDanglingLinks=MagicMock(),
            _rebuildConnectionCache=MagicMock(),
            _spatialIndex=SimpleNamespace(
                clear=MagicMock(),
                insertNode=MagicMock(),
                insertLink=MagicMock(),
            ),
            _computeLinkBounds=MagicMock(return_value=MagicMock()),
            _profiler=MagicMock(),
        )

        GraphView._rebuildLinks(view)

        view._populateNodeLinksFromPrim.assert_called_once_with(node, prim)
        view._surfaceConnectionEndpointPins.assert_called_once()
        view._processNodeLinks.assert_called_once_with(node)
        view._addBackReferenceDanglingLinks.assert_called_once()
        # The freshly-rebuilt links are synced into the C++ View Model before the
        # connection cache is rebuilt, so the cache reflects the current links.
        view.nodeGraph.syncLinksFromModels.assert_called_once_with(view.links)

    def _make_back_ref_view(self, stage, nodes, links=None):
        """Build a SimpleNamespace view shaped for _addBackReferenceDanglingLinks."""
        view = SimpleNamespace(
            links=list(links or []),
            nodes=dict(nodes),
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _backReferenceLinkDataCache=None,
            _setupDanglingLinkEndpoint=MagicMock(),
            _makeLinkFromUsdData=lambda d: SimpleNamespace(
                sourceNodeId=d["sourceNodeId"],
                sourcePort=d.get("sourcePinName", ""),
                targetNodeId=d["targetNodeId"],
                targetPort=d.get("targetPinName", ""),
                sourcePropertyName=d.get("sourcePropertyName", ""),
                targetPropertyName=d.get("targetPropertyName", ""),
                isDangling=False,
            ),
        )
        view._invalidateBackReferenceLinkDataCache = (
            lambda: GraphView._invalidateBackReferenceLinkDataCache(view)
        )
        return view

    def test_add_back_reference_swallows_collect_links_runtime_error(self):
        """RuntimeError from collect_links_for_prim must not surface; loop continues."""
        prim_a = MagicMock()
        prim_a.GetPath.return_value = "/A"
        stage = MagicMock()
        stage.Traverse.return_value = [prim_a]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            side_effect=RuntimeError("usd boom"),
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])
        # Cache populated to {} so we don't re-traverse on the next call.
        self.assertEqual(view._backReferenceLinkDataCache, {})

    def test_add_back_reference_swallows_collect_links_type_error(self):
        prim_a = MagicMock()
        prim_a.GetPath.return_value = "/A"
        stage = MagicMock()
        stage.Traverse.return_value = [prim_a]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            side_effect=TypeError("bad type"),
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])

    def test_add_back_reference_swallows_collect_links_attribute_error(self):
        prim_a = MagicMock()
        prim_a.GetPath.return_value = "/A"
        stage = MagicMock()
        stage.Traverse.return_value = [prim_a]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            side_effect=AttributeError("no attr"),
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])

    def test_add_back_reference_present_is_source_branch(self):
        """Absent prim's link with present-prim source → output-side dangling link."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        present_node = SimpleNamespace(id="/Present")
        view = self._make_back_ref_view(stage, nodes={"/Present": present_node})

        link_data = {
            "sourceNodeId": "/Present",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_data],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(len(view.links), 1)
        added = view.links[0]
        self.assertFalse(added.is_input_link)
        self.assertEqual(added.data_sourceNodeId, "/Present")
        self.assertEqual(added.data_targetNodeId, "/Absent")
        view._setupDanglingLinkEndpoint.assert_called_once_with(
            added, present_node, "out", isOutput=True
        )

    def test_add_back_reference_present_is_target_branch(self):
        """Absent prim's link with present-prim target → input-side dangling link."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        present_node = SimpleNamespace(id="/Present")
        view = self._make_back_ref_view(stage, nodes={"/Present": present_node})

        link_data = {
            "sourceNodeId": "/Absent",
            "sourcePinName": "out",
            "targetNodeId": "/Present",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_data],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(len(view.links), 1)
        added = view.links[0]
        self.assertTrue(added.is_input_link)
        self.assertEqual(added.data_sourceNodeId, "/Absent")
        self.assertEqual(added.data_targetNodeId, "/Present")
        view._setupDanglingLinkEndpoint.assert_called_once_with(
            added, present_node, "inp", isOutput=False
        )

    def test_add_back_reference_skips_when_both_endpoints_absent(self):
        """Links between two absent prims are not surfaced."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        link_data = {
            "sourceNodeId": "/OtherAbsent",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_data],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])

    def test_add_back_reference_dedups_against_existing_link(self):
        """A back-ref matching an already-added link's full key is skipped."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        present_node = SimpleNamespace(id="/Present")
        # Pre-seed an existing forward-direction link with the same key.
        existing = SimpleNamespace(
            sourceNodeId="/Present",
            sourcePort="out",
            targetNodeId="/Absent",
            targetPort="inp",
            sourcePropertyName="outputs:out",
            targetPropertyName="inputs:inp",
        )
        view = self._make_back_ref_view(
            stage, nodes={"/Present": present_node}, links=[existing]
        )

        link_data = {
            "sourceNodeId": "/Present",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_data],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [existing])
        view._setupDanglingLinkEndpoint.assert_not_called()

    def test_add_back_reference_skips_present_prim_at_use_time(self):
        """The use-time loop skips prims in self.nodes; forward pass handled them."""
        present_prim = MagicMock()
        present_prim.GetPath.return_value = "/Present"
        stage = MagicMock()
        stage.Traverse.return_value = [present_prim]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        # Return non-empty data so the present prim's path IS cached, then
        # verify the use-time loop still skips it (no _setupDangling call,
        # no link appended) because membership is checked per call, not per
        # cache build.
        link_data = {
            "sourceNodeId": "/Present",
            "sourcePinName": "out",
            "targetNodeId": "/Other",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_data],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])
        view._setupDanglingLinkEndpoint.assert_not_called()
        self.assertIn("/Present", view._backReferenceLinkDataCache)

    def test_add_back_reference_no_op_when_stage_missing(self):
        view = SimpleNamespace(
            links=[],
            nodes={},
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=None)),
            _backReferenceLinkDataCache=None,
        )
        GraphView._addBackReferenceDanglingLinks(view)
        self.assertEqual(view.links, [])
        # Cache untouched when there's no stage to scan.
        self.assertIsNone(view._backReferenceLinkDataCache)

    def test_add_back_reference_reuses_cache_across_calls(self):
        """Second call must NOT re-traverse the stage (cache is reused)."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        collect = MagicMock(return_value=[])
        with patch("pxr.UsdNoodles.graphView.collect_links_for_prim", collect):
            GraphView._addBackReferenceDanglingLinks(view)
            first_traverse_count = stage.Traverse.call_count
            first_collect_count = collect.call_count
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(stage.Traverse.call_count, first_traverse_count)
        self.assertEqual(collect.call_count, first_collect_count)

    def test_invalidate_back_reference_cache_forces_retraversal(self):
        """After invalidation, the next call must re-traverse."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        with patch("pxr.UsdNoodles.graphView.collect_links_for_prim", return_value=[]):
            GraphView._addBackReferenceDanglingLinks(view)
            GraphView._invalidateBackReferenceLinkDataCache(view)
            self.assertIsNone(view._backReferenceLinkDataCache)
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(stage.Traverse.call_count, 2)

    def test_add_back_reference_shared_absent_prim_to_multiple_present(self):
        """A single absent prim with links to two different present prims
        produces two distinct back-reference dangling links."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        present_a = SimpleNamespace(id="/PresentA")
        present_b = SimpleNamespace(id="/PresentB")
        view = self._make_back_ref_view(
            stage, nodes={"/PresentA": present_a, "/PresentB": present_b}
        )

        link_to_a = {
            "sourceNodeId": "/PresentA",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp_a",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp_a",
        }
        link_to_b = {
            "sourceNodeId": "/PresentB",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp_b",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp_b",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[link_to_a, link_to_b],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(len(view.links), 2)
        present_nodes_used = {
            id(call.args[1]) for call in view._setupDanglingLinkEndpoint.call_args_list
        }
        self.assertEqual(present_nodes_used, {id(present_a), id(present_b)})

    def test_add_back_reference_self_referencing_absent_prim_is_skipped(self):
        """An absent prim whose connection points at itself (both endpoints
        absent) is not surfaced — there is no present-prim port to attach to."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        view = self._make_back_ref_view(stage, nodes={"/Present": SimpleNamespace()})

        self_link = {
            "sourceNodeId": "/Absent",
            "sourcePinName": "out",
            "targetNodeId": "/Absent",
            "targetPinName": "inp",
            "sourcePropertyName": "outputs:out",
            "targetPropertyName": "inputs:inp",
        }
        with patch(
            "pxr.UsdNoodles.graphView.collect_links_for_prim",
            return_value=[self_link],
        ):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [])
        view._setupDanglingLinkEndpoint.assert_not_called()

    def test_add_back_reference_self_referencing_present_prim_skipped_by_forward_pass(
        self,
    ):
        """A present prim that self-references is fully handled by the forward
        pass; the back-ref scan must not double-add. We verify by including a
        present-prim self-link in the existing self.links keyset — the scan
        sees the absent prim's view of any data and must dedup correctly."""
        absent_prim = MagicMock()
        absent_prim.GetPath.return_value = "/Absent"
        stage = MagicMock()
        stage.Traverse.return_value = [absent_prim]
        present = SimpleNamespace(id="/Present")
        # forward pass already added the present prim's self-link
        existing_self = SimpleNamespace(
            sourceNodeId="/Present",
            sourcePort="out",
            targetNodeId="/Present",
            targetPort="inp",
            sourcePropertyName="outputs:out",
            targetPropertyName="inputs:inp",
        )
        view = self._make_back_ref_view(
            stage, nodes={"/Present": present}, links=[existing_self]
        )

        # Absent prim has no links of its own; back-ref must add nothing.
        with patch("pxr.UsdNoodles.graphView.collect_links_for_prim", return_value=[]):
            GraphView._addBackReferenceDanglingLinks(view)

        self.assertEqual(view.links, [existing_self])
        view._setupDanglingLinkEndpoint.assert_not_called()

    def test_create_connection_preserves_relationship_property_metadata(self):
        stage = MagicMock()
        root_layer = MagicMock()
        edit_target = MagicMock()
        edit_target.GetLayer.return_value = root_layer
        stage.GetEditTarget.return_value = edit_target
        stage.GetRootLayer.return_value = root_layer
        library = MagicMock()
        library.create_connection.return_value = True
        output_node = SimpleNamespace(outputLinks=[])
        input_node = SimpleNamespace(inputLinks=[])
        view = SimpleNamespace(
            _resolveConnectionEndpoints=MagicMock(
                return_value=(
                    output_node,
                    input_node,
                    MagicMock(),
                    MagicMock(),
                    library,
                )
            ),
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _noticeHandler=SimpleNamespace(setEnabled=MagicMock()),
            _showPopupMessage=MagicMock(),
            _abortConnection=MagicMock(),
            _deleteOldConnection=MagicMock(return_value=True),
            _addLinkToGraph=MagicMock(),
            _clearReconnectState=MagicMock(),
            _invalidateBackReferenceLinkDataCache=MagicMock(),
            _reconnectingLink=False,
            _reconnectOldSourceNodeId=None,
            _reconnectOldSourcePort=None,
            _reconnectOldTargetNodeId=None,
            _reconnectOldTargetPort=None,
            update=MagicMock(),
        )

        with patch("pxr.UsdNoodles.graphView._push_undo_command"):
            GraphView._createConnection(
                view,
                "/ConstraintA",
                "sources",
                "/ConstraintB",
                "affects",
                sourcePropertyName="sources",
                targetPropertyName="affects",
            )

        library.create_connection.assert_called_once()
        view._addLinkToGraph.assert_called_once_with(
            "/ConstraintA",
            "sources",
            "/ConstraintB",
            "affects",
            output_node,
            input_node,
            sourcePropertyName="sources",
            targetPropertyName="affects",
        )

    def test_reconnect_continues_when_delete_old_returns_false(self):
        """When _deleteOldConnection returns False (implicit replacement by
        create_connection), the reconnect must still succeed: link added,
        undo pushed, no abort."""
        stage = MagicMock()
        root_layer = MagicMock()
        edit_target = MagicMock()
        edit_target.GetLayer.return_value = root_layer
        stage.GetEditTarget.return_value = edit_target
        stage.GetRootLayer.return_value = root_layer
        library = MagicMock()
        library.create_connection.return_value = True
        output_node = SimpleNamespace(outputLinks=[])
        input_node = SimpleNamespace(inputLinks=[])
        view = SimpleNamespace(
            _resolveConnectionEndpoints=MagicMock(
                return_value=(
                    output_node,
                    input_node,
                    MagicMock(),
                    MagicMock(),
                    library,
                )
            ),
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _noticeHandler=SimpleNamespace(setEnabled=MagicMock()),
            _showPopupMessage=MagicMock(),
            _abortConnection=MagicMock(),
            _deleteOldConnection=MagicMock(return_value=False),
            _addLinkToGraph=MagicMock(),
            _clearReconnectState=MagicMock(),
            _invalidateBackReferenceLinkDataCache=MagicMock(),
            _reconnectingLink=False,
            _reconnectOldSourceNodeId=None,
            _reconnectOldSourcePort=None,
            _reconnectOldTargetNodeId=None,
            _reconnectOldTargetPort=None,
            update=MagicMock(),
        )

        with patch("pxr.UsdNoodles.graphView._push_undo_command") as mock_push:
            GraphView._createConnection(
                view,
                "/C",
                "out",
                "/D",
                "inp",
                reconnect_state=("/A", "out", "/B", "inp"),
            )

        view._addLinkToGraph.assert_called_once()
        view._abortConnection.assert_not_called()
        view._clearReconnectState.assert_called_once()
        mock_push.assert_called_once()
        self.assertEqual(mock_push.call_args[0][0], "Reconnect")

    def test_create_connection_invalidates_back_reference_cache(self):
        # Regression: the back-reference link cache is populated from
        # collect_links_for_prim and consulted whenever a node is re-added to
        # the editor.  USD notices are suppressed across _createConnection, so
        # _handleUsdChanges cannot drop the cache for us -- without an
        # explicit invalidation here, removing the involved prims from the
        # editor and re-adding them would resurrect the stale connection.
        stage = MagicMock()
        root_layer = MagicMock()
        edit_target = MagicMock()
        edit_target.GetLayer.return_value = root_layer
        stage.GetEditTarget.return_value = edit_target
        stage.GetRootLayer.return_value = root_layer
        library = MagicMock()
        library.create_connection.return_value = True
        output_node = SimpleNamespace(outputLinks=[])
        input_node = SimpleNamespace(inputLinks=[])
        view = SimpleNamespace(
            _resolveConnectionEndpoints=MagicMock(
                return_value=(
                    output_node,
                    input_node,
                    MagicMock(),
                    MagicMock(),
                    library,
                )
            ),
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _noticeHandler=SimpleNamespace(setEnabled=MagicMock()),
            _abortConnection=MagicMock(),
            _addLinkToGraph=MagicMock(),
            _clearReconnectState=MagicMock(),
            _invalidateBackReferenceLinkDataCache=MagicMock(),
            _reconnectingLink=False,
            _reconnectOldSourceNodeId=None,
            _reconnectOldSourcePort=None,
            _reconnectOldTargetNodeId=None,
            _reconnectOldTargetPort=None,
            update=MagicMock(),
        )

        with patch("pxr.UsdNoodles.graphView._push_undo_command"):
            GraphView._createConnection(
                view,
                "/A",
                "material:binding",
                "/B",
                "",
                sourcePropertyName="material:binding",
                targetPropertyName="",
            )

        view._invalidateBackReferenceLinkDataCache.assert_called_once()

    def test_delete_connection_invalidates_back_reference_cache(self):
        # Regression for the user's reported bug: remove a relationship in the
        # editor, remove the prims from the editor, then re-add them.  Without
        # invalidating the back-reference cache here, the cached
        # collect_links_for_prim snapshot still reports the deleted target and
        # the editor resurrects the connection on re-add.
        library = MagicMock()
        library.delete_connection.return_value = True
        output_prim = MagicMock()
        output_prim.IsValid.return_value = True
        input_prim = MagicMock()
        input_prim.IsValid.return_value = True
        output_node = SimpleNamespace(
            outputLinks=[],
            getUsdPrim=MagicMock(return_value=output_prim),
        )
        input_node = SimpleNamespace(
            inputLinks=[],
            getUsdPrim=MagicMock(return_value=input_prim),
        )
        stage = MagicMock()
        view = SimpleNamespace(
            nodes={"/A": output_node, "/B": input_node},
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _findLibraryForPrim=MagicMock(return_value=library),
            _noticeHandler=SimpleNamespace(setEnabled=MagicMock()),
            _removeLinkFromGraph=MagicMock(),
            _invalidateBackReferenceLinkDataCache=MagicMock(),
        )

        with patch("pxr.UsdNoodles.graphView._push_undo_command"):
            GraphView._deleteConnection(
                view,
                "/A",
                "material:binding",
                "/B",
                "",
                sourcePropertyName="material:binding",
                targetPropertyName="",
            )

        library.delete_connection.assert_called_once()
        view._invalidateBackReferenceLinkDataCache.assert_called_once()

    def test_add_node_from_dangling_link_uses_population_helper(self):
        prim = MagicMock()
        prim.IsValid.return_value = True
        prim.GetTypeName.return_value = "RigJoint"
        prim.GetName.return_value = "Missing"
        node = SimpleNamespace(id="/Missing", inputLinks=[], outputLinks=[], zOrder=0)
        stage = MagicMock()
        stage.GetPrimAtPath.return_value = prim
        view = SimpleNamespace(
            _usdviewApi=SimpleNamespace(stage=stage),
            _showPopupMessage=MagicMock(),
            _nodeFactory=SimpleNamespace(
                create_node_from_prim=MagicMock(return_value=node),
            ),
            _populateNodeLinksFromPrim=MagicMock(),
            _positionNodeInViewport=MagicMock(),
            _nextZOrder=5,
            fontAtlas=None,
            nodeGraph=SimpleNamespace(
                _calculateNodeSize=MagicMock(),
                syncSelectionToPrimTree=False,
            ),
            textRenderer=SimpleNamespace(calculateTextWidth=MagicMock()),
            nodes={},
            _clearRenderCache=MagicMock(),
            update=MagicMock(),
            linksChanged=False,
            textChanged=False,
        )
        link = SimpleNamespace(
            isDangling=True,
            data_sourceNodeId="/Missing",
            data_targetNodeId="",
        )

        result = GraphView._addNodeFromDanglingLink(view, link)

        self.assertTrue(result)
        view._populateNodeLinksFromPrim.assert_called_once_with(node, prim)
        self.assertIn("/Missing", view.nodes)
        # Match the _addPrimAsNode setup: positioning and z-order are
        # required for the renderer to compute pin positions reliably,
        # which in turn lets _rebuildLinks resolve the noodle endpoints
        # on the next paint instead of leaving them at the dangling stub.
        view._positionNodeInViewport.assert_called_once_with(node)
        self.assertEqual(view._nextZOrder, 6)
        self.assertEqual(node.zOrder, 6)

    def test_collect_sibling_dangling_links_groups_by_present_source_port(self):
        # A USD relationship with three targets becomes three dangling
        # LinkData objects that all share (sourceNodeId, sourcePort) on the
        # present side.  Expansion needs every sibling, not just the first
        # one a port-hit search returns.
        seed = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat1",
            targetPort="",
            targetPropertyName="",
        )
        sibling_b = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat2",
            targetPort="",
            targetPropertyName="",
        )
        sibling_c = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat3",
            targetPort="",
            targetPropertyName="",
        )
        # A dangling link on a DIFFERENT relationship on the same prim must
        # not be folded into this expansion.
        unrelated = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="proxyPrim",
            sourcePropertyName="proxyPrim",
            targetNodeId="/Proxy",
            targetPort="",
            targetPropertyName="",
        )
        non_dangling = SimpleNamespace(
            isDangling=False,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat4",
            targetPort="",
            targetPropertyName="",
        )
        view = SimpleNamespace(
            links=[non_dangling, seed, sibling_b, unrelated, sibling_c],
            nodes={"/Mesh": SimpleNamespace()},
        )

        siblings = GraphView._collectSiblingDanglingLinks(view, seed)

        self.assertEqual(siblings, [seed, sibling_b, sibling_c])

    def test_add_nodes_from_dangling_link_expands_all_relationship_targets(self):
        # Regression: expanding a relationship with multiple targets must
        # bring in every missing target, not just the first one
        # ``_findDanglingLinkAtPort`` happens to return.
        present_node = SimpleNamespace()
        seed = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat1",
            targetPort="",
            targetPropertyName="",
        )
        sibling_b = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat2",
            targetPort="",
            targetPropertyName="",
        )
        sibling_c = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Mesh",
            sourcePort="material:binding",
            sourcePropertyName="material:binding",
            targetNodeId="/Mat3",
            targetPort="",
            targetPropertyName="",
        )
        view = SimpleNamespace(
            links=[seed, sibling_b, sibling_c],
            nodes={"/Mesh": present_node},
            _addNodeFromDanglingLink=MagicMock(return_value=True),
            _collectSiblingDanglingLinks=lambda link: GraphView._collectSiblingDanglingLinks(
                view, link
            ),
        )

        result = GraphView._addNodesFromDanglingLink(view, seed)

        self.assertTrue(result)
        self.assertEqual(view._addNodeFromDanglingLink.call_count, 3)
        called_links = [
            call.args[0] for call in view._addNodeFromDanglingLink.call_args_list
        ]
        self.assertEqual(called_links, [seed, sibling_b, sibling_c])

    def test_find_dangling_link_at_port_hits_present_endpoint(self):
        renderer = SimpleNamespace(getPortWidth=MagicMock(return_value=8.0))
        node = SimpleNamespace(renderer=renderer)
        present_link = SimpleNamespace(
            isDangling=True,
            sourceNodeId="/Missing",
            sourcePort="out",
            targetNodeId="/Present",
            targetPort="inp",
        )
        non_dangling_link = SimpleNamespace(
            isDangling=False,
            sourceNodeId="/Present",
            sourcePort="out",
            targetNodeId="/Other",
            targetPort="inp",
        )
        view = SimpleNamespace(
            links=[non_dangling_link, present_link],
            nodes={"/Present": node},
            defaultRenderer=renderer,
            _cachedPortHitRadiusMultiplier=1.5,
            _getPortPosition=MagicMock(return_value=Gf.Vec2d(100.0, 50.0)),
        )

        idx = GraphView._findDanglingLinkAtPort(view, Gf.Vec2d(101.0, 51.0))
        self.assertEqual(idx, 1)
        view._getPortPosition.assert_called_with(node, "inp", False)

        miss_idx = GraphView._findDanglingLinkAtPort(view, Gf.Vec2d(200.0, 200.0))
        self.assertEqual(miss_idx, -1)

    def test_find_dangling_link_at_port_skips_when_both_endpoints_missing(self):
        view = SimpleNamespace(
            links=[
                SimpleNamespace(
                    isDangling=True,
                    sourceNodeId="/MissingA",
                    sourcePort="out",
                    targetNodeId="/MissingB",
                    targetPort="inp",
                ),
            ],
            nodes={},
            defaultRenderer=None,
            _cachedPortHitRadiusMultiplier=1.5,
            _getPortPosition=MagicMock(),
        )

        self.assertEqual(
            GraphView._findDanglingLinkAtPort(view, Gf.Vec2d(0.0, 0.0)),
            -1,
        )

    def test_add_prim_as_node_uses_population_helper(self):
        prim = MagicMock()
        prim.GetPath.return_value = "/Node"
        node = SimpleNamespace(
            id="/Node",
            inputLinks=[],
            outputLinks=[],
            position=Gf.Vec2d(0.0, 0.0),
            size=Gf.Vec2d(20.0, 10.0),
            zOrder=0,
        )
        view = SimpleNamespace(
            nodes={},
            _nodeFactory=SimpleNamespace(
                create_node_from_prim=MagicMock(return_value=node),
            ),
            _populateNodeLinksFromPrim=MagicMock(),
            fontAtlas=None,
            nodeGraph=SimpleNamespace(_calculateNodeSize=MagicMock()),
            textRenderer=SimpleNamespace(calculateTextWidth=MagicMock()),
            _positionNodeInViewport=MagicMock(),
            _nextZOrder=0,
        )
        added_node_ids = []

        result = GraphView._addPrimAsNode(view, prim, MagicMock(), added_node_ids)

        self.assertTrue(result)
        view._populateNodeLinksFromPrim.assert_called_once_with(node, prim)
        self.assertEqual(added_node_ids, ["/Node"])

    def test_add_nodes_from_prim_tree_selection_processes_all_selected(self):
        def _make_prim(path):
            sdf_path = MagicMock()
            sdf_path.pathString = path
            sdf_path.__str__ = MagicMock(return_value=path)
            prim = MagicMock()
            prim.IsValid.return_value = True
            prim.GetPath.return_value = sdf_path
            prim.GetTypeName.return_value = "Shader"
            prim.GetName.return_value = path.rsplit("/", 1)[-1]
            return prim, sdf_path

        driver_prim, _ = _make_prim("/Scope/DriverNode")
        driven_prim, _ = _make_prim("/Scope/DrivenNode")

        added_paths = []

        def fake_add(prim, _stage, addedNodeIds):
            path = str(prim.GetPath())
            added_paths.append(path)
            view.nodes[path] = SimpleNamespace(
                id=path,
                selected=False,
                position=Gf.Vec2d(0.0, 0.0),
                size=Gf.Vec2d(10.0, 10.0),
            )
            addedNodeIds.append(path)
            return True

        selection = SimpleNamespace(
            getPrims=MagicMock(return_value=[driver_prim, driven_prim]),
        )
        dataModel = SimpleNamespace(selection=selection)
        view = SimpleNamespace(
            _usdviewApi=SimpleNamespace(dataModel=dataModel, stage=MagicMock()),
            _lastPrimTreeSelection=[],
            _showPopupMessage=MagicMock(),
            nodes={},
            _selectedNodes=set(),
            _addPrimAsNode=MagicMock(side_effect=fake_add),
            _getContainerChildNodes=MagicMock(return_value=[]),
            nodeGraph=SimpleNamespace(syncSelectionToPrimTree=False),
            _clearRenderCache=MagicMock(),
            textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
            _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
            # addNodesFromPrimTreeSelection grid-places the freshly added nodes
            # and resets the text + icon position caches.
            _gridPlaceNodes=MagicMock(return_value=[]),
            _cppIconRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _frameNodeBounds=MagicMock(),
            update=MagicMock(),
            linksChanged=False,
            textChanged=False,
        )

        GraphView.addNodesFromPrimTreeSelection(view)

        self.assertEqual(added_paths, ["/Scope/DriverNode", "/Scope/DrivenNode"])
        view._showPopupMessage.assert_called_with("Added 2 node(s)")

    def test_add_nodes_from_prim_tree_selection_deduplicates(self):
        sdf_path = MagicMock()
        sdf_path.pathString = "/Scope/Node"
        sdf_path.__str__ = MagicMock(return_value="/Scope/Node")
        prim = MagicMock()
        prim.IsValid.return_value = True
        prim.GetPath.return_value = sdf_path
        prim.GetTypeName.return_value = "Shader"

        added_paths = []

        def fake_add(p, _stage, addedNodeIds):
            path = str(p.GetPath())
            added_paths.append(path)
            view.nodes[path] = SimpleNamespace(
                id=path,
                selected=False,
                position=Gf.Vec2d(0.0, 0.0),
                size=Gf.Vec2d(10.0, 10.0),
            )
            addedNodeIds.append(path)
            return True

        selection = SimpleNamespace(
            getPrims=MagicMock(return_value=[prim, prim]),
        )
        dataModel = SimpleNamespace(selection=selection)
        view = SimpleNamespace(
            _usdviewApi=SimpleNamespace(dataModel=dataModel, stage=MagicMock()),
            _lastPrimTreeSelection=[],
            _showPopupMessage=MagicMock(),
            nodes={},
            _selectedNodes=set(),
            _addPrimAsNode=MagicMock(side_effect=fake_add),
            _getContainerChildNodes=MagicMock(return_value=[]),
            nodeGraph=SimpleNamespace(syncSelectionToPrimTree=False),
            _clearRenderCache=MagicMock(),
            textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
            _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
            _frameNodeBounds=MagicMock(),
            update=MagicMock(),
            linksChanged=False,
            textChanged=False,
        )

        GraphView.addNodesFromPrimTreeSelection(view)

        self.assertEqual(added_paths, ["/Scope/Node"])

    def test_add_nodes_from_prim_tree_selection_uses_cached_when_richer(self):
        def _make_prim(path):
            sdf_path = MagicMock()
            sdf_path.pathString = path
            sdf_path.__str__ = MagicMock(return_value=path)
            prim = MagicMock()
            prim.IsValid.return_value = True
            prim.GetPath.return_value = sdf_path
            prim.GetTypeName.return_value = "Shader"
            prim.GetName.return_value = path.rsplit("/", 1)[-1]
            return prim

        driver_prim = _make_prim("/Scope/DriverNode")
        driven_prim = _make_prim("/Scope/DrivenNode")

        added_paths = []

        def fake_add(p, _stage, addedNodeIds):
            path = str(p.GetPath())
            added_paths.append(path)
            view.nodes[path] = SimpleNamespace(
                id=path,
                selected=False,
                position=Gf.Vec2d(0.0, 0.0),
                size=Gf.Vec2d(10.0, 10.0),
            )
            addedNodeIds.append(path)
            return True

        # Live API returns only one prim (e.g. focus narrowed),
        # but the cached selection captured both.
        selection = SimpleNamespace(
            getPrims=MagicMock(return_value=[driver_prim]),
        )
        dataModel = SimpleNamespace(selection=selection)
        view = SimpleNamespace(
            _usdviewApi=SimpleNamespace(dataModel=dataModel, stage=MagicMock()),
            _lastPrimTreeSelection=[driver_prim, driven_prim],
            _showPopupMessage=MagicMock(),
            nodes={},
            _selectedNodes=set(),
            _addPrimAsNode=MagicMock(side_effect=fake_add),
            _getContainerChildNodes=MagicMock(return_value=[]),
            nodeGraph=SimpleNamespace(syncSelectionToPrimTree=False),
            _clearRenderCache=MagicMock(),
            textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
            _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
            _frameNodeBounds=MagicMock(),
            update=MagicMock(),
            linksChanged=False,
            textChanged=False,
        )

        GraphView.addNodesFromPrimTreeSelection(view)

        self.assertEqual(
            sorted(added_paths),
            ["/Scope/DrivenNode", "/Scope/DriverNode"],
        )

    def test_update_link_drag_rejects_regular_to_relationship_hover(self):
        temp_link = SimpleNamespace(start=None, end=None)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="translate",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=True,
            _dragLinkSyntheticTarget=("old", "sources", False),
            _dragLinkResolvedTarget=("old", "sources", False, "sources"),
            _hoveredPort=("constraint", "sources", True),
            nodes={"constraint": SimpleNamespace()},
            _getPortPosition=MagicMock(return_value=Gf.Vec2d(5.0, 6.0)),
            nodeIdUnderCursor="",
            _dragLinkSourceNode="/Driver",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(0, 0))

        self.assertFalse(view._dragLinkValidTarget)
        self.assertIsNone(view._dragLinkSyntheticTarget)
        self.assertIsNone(view._dragLinkResolvedTarget)

    def test_update_link_drag_creates_prim_target_for_relationship_pin(self):
        temp_link = SimpleNamespace(start=None, end=None, is_relationship_link=True)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="sources",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=None,
            nodes={
                "driver": SimpleNamespace(
                    position=Gf.Vec2d(10.0, 12.0),
                    size=Gf.Vec2d(30.0, 40.0),
                )
            },
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            nodeIdUnderCursor="driver",
            _dragLinkSourceNode="/Constraint",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(1, 2))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertEqual(view._dragLinkSyntheticTarget, ("driver", "", False, ""))
        self.assertEqual(temp_link.end, Gf.Vec2d(25.0, 12.0))

    def _make_temp_link_render_view(self, temp_link, valid_target):
        """Fake self for _renderTemporaryLink; captures the noodle's free end
        as it is passed to renderLinks so the prim-target inset is observable."""
        captured = {}

        def _capture_render(links, *args, **kwargs):
            captured["free_end"] = (float(links[0].end[0]), float(links[0].end[1]))

        return (
            SimpleNamespace(
                _dragLinkValidTarget=valid_target,
                _hoveredPort=None,
                _dragLinkSourceIsOutput=True,
                _dragLinkSourcePort="myRel",
                _dragLinkTempLink=temp_link,
                _cachedLinkRelationshipBaseColor=[1.0, 0.5, 0.5, 1.0],
                _cachedLinkActiveColor=[0.0, 0.0, 1.0, 1.0],
                _cachedLinkHoveredColor=[1.0, 1.0, 0.0],
                _cachedLinkSelectedColor=[1.0, 1.0, 0.3],
                _cachedPortRingThickness=1.0,
                zoom=1.0,
                panX=0.0,
                panY=0.0,
                width=MagicMock(return_value=800),
                height=MagicMock(return_value=600),
                _worldSpaceProjectionMatrix=MagicMock(
                    return_value=SimpleNamespace(
                        data=MagicMock(return_value=[0.0] * 16)
                    )
                ),
                _renderConfig=MagicMock(return_value=MagicMock()),
                linkRenderer=SimpleNamespace(
                    renderLinks=MagicMock(side_effect=_capture_render)
                ),
                _nodeRenderManager=SimpleNamespace(
                    renderEndpointTriangle=MagicMock(),
                    renderEndpointCircle=MagicMock(),
                ),
                defaultRenderer=SimpleNamespace(
                    getPortWidth=MagicMock(return_value=8.0)
                ),
            ),
            captured,
        )

    def test_render_temporary_link_insets_prim_target_noodle_to_arrow_base(self):
        """A snapped whole-prim relationship drag draws the preview noodle to the
        arrowhead base (node_top.y - arrow_length), not the tip, then restores the
        true node top so the arrowhead/hit geometry stay anchored to it. The
        committed path gets this inset from the shader (isPrimTarget); the per-call
        renderLinks path used for the preview does not, so it is applied here."""
        node_top = Gf.Vec2d(25.0, 100.0)
        temp_link = SimpleNamespace(
            start=Gf.Vec2d(0.0, 0.0),
            end=Gf.Vec2d(node_top[0], node_top[1]),
            is_relationship_link=True,
        )
        view, captured = self._make_temp_link_render_view(temp_link, valid_target=True)

        GraphView._renderTemporaryLink(view)

        arrow_length = max(12.0 / max(view.zoom, 1e-6), 4.0)
        self.assertEqual(
            captured["free_end"], (node_top[0], node_top[1] - arrow_length)
        )
        self.assertEqual((temp_link.end[0], temp_link.end[1]), (25.0, 100.0))

    def test_render_temporary_link_does_not_inset_when_unsnapped(self):
        """With no valid target the preview noodle follows the cursor to its full
        end (the arrowhead faces sideways, not down)."""
        cursor = Gf.Vec2d(40.0, 70.0)
        temp_link = SimpleNamespace(
            start=Gf.Vec2d(0.0, 0.0),
            end=Gf.Vec2d(cursor[0], cursor[1]),
            is_relationship_link=True,
        )
        view, captured = self._make_temp_link_render_view(temp_link, valid_target=False)

        GraphView._renderTemporaryLink(view)

        self.assertEqual(captured["free_end"], (40.0, 70.0))
        self.assertEqual((temp_link.end[0], temp_link.end[1]), (40.0, 70.0))

    def test_update_link_drag_allows_self_prim_target_for_relationship_pin(self):
        temp_link = SimpleNamespace(start=None, end=None, is_relationship_link=True)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="sources",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=None,
            nodes={
                "/Constraint": SimpleNamespace(
                    position=Gf.Vec2d(10.0, 12.0),
                    size=Gf.Vec2d(30.0, 40.0),
                )
            },
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            nodeIdUnderCursor="/Constraint",
            _dragLinkSourceNode="/Constraint",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(1, 2))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertEqual(view._dragLinkSyntheticTarget, ("/Constraint", "", False, ""))
        self.assertEqual(temp_link.end, Gf.Vec2d(25.0, 12.0))

    def test_update_link_drag_prefers_property_target_over_prim_target(self):
        temp_link = SimpleNamespace(
            start=Gf.Vec2d(40.0, 50.0),
            end=Gf.Vec2d(40.0, 50.0),
            sourceNodeId="/Constraint",
            sourcePort="sources",
            targetNodeId="",
            targetPort="",
            sourcePropertyName="sources",
            targetPropertyName="",
            is_relationship_link=True,
        )
        target_node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(60.0, 90.0),
        )
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="sources",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=("/Constraint", "sourceTranslations", False),
            nodes={"/Constraint": target_node},
            _resolvePortPropertyName=MagicMock(
                return_value="inputs:sourceTranslations"
            ),
            _getLinkEndpointPosition=MagicMock(return_value=Gf.Vec2d(10.0, 72.0)),
            nodeIdUnderCursor="/Constraint",
            _dragLinkSourceNode="/Constraint",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(10, 72))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertIsNone(view._dragLinkSyntheticTarget)
        self.assertEqual(
            view._dragLinkResolvedTarget,
            ("/Constraint", "sourceTranslations", False, "inputs:sourceTranslations"),
        )
        self.assertEqual(temp_link.targetNodeId, "/Constraint")
        self.assertEqual(temp_link.targetPort, "sourceTranslations")
        self.assertEqual(temp_link.targetPropertyName, "inputs:sourceTranslations")
        self.assertEqual(temp_link.end, Gf.Vec2d(10.0, 72.0))

    def test_update_link_drag_recovers_prim_target_when_border_misses_node_hit(self):
        temp_link = SimpleNamespace(start=None, end=None, is_relationship_link=True)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="affects",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=None,
            nodes={
                "driver": SimpleNamespace(
                    position=Gf.Vec2d(10.0, 12.0),
                    size=Gf.Vec2d(30.0, 40.0),
                )
            },
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _getNodeIdAtWorldPos=MagicMock(return_value="driver"),
            nodeIdUnderCursor="",
            _dragLinkSourceNode="/Constraint",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(25, 12))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertEqual(view._dragLinkSyntheticTarget, ("driver", "", False, ""))
        self.assertEqual(temp_link.end, Gf.Vec2d(25.0, 12.0))
        view._getNodeIdAtWorldPos.assert_called_once()

    def test_update_link_drag_accepts_any_property_for_relationship_pin(self):
        temp_link = SimpleNamespace(start=None, end=None, is_relationship_link=True)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="sources",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=("driver", "translate", False),
            nodes={"driver": SimpleNamespace()},
            _resolvePortPropertyName=MagicMock(return_value="inputs:translate"),
            _getLinkEndpointPosition=MagicMock(return_value=Gf.Vec2d(7.0, 9.0)),
            nodeIdUnderCursor="driver",
            _dragLinkSourceNode="/Constraint",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(1, 2))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertEqual(
            view._dragLinkResolvedTarget,
            ("driver", "translate", False, "inputs:translate"),
        )
        self.assertEqual(temp_link.end, Gf.Vec2d(7.0, 9.0))

    def test_update_link_drag_accepts_visible_relationship_pin_as_target(self):
        temp_link = SimpleNamespace(start=None, end=None, is_relationship_link=True)
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            _dragLinkCursorPos=None,
            _dragLinkSourceIsOutput=True,
            _dragLinkSourcePort="sources",
            _dragLinkTempLink=temp_link,
            _dragLinkValidTarget=False,
            _dragLinkSyntheticTarget=None,
            _dragLinkResolvedTarget=None,
            _hoveredPort=("constraintB", "affects", True),
            nodes={"constraintB": SimpleNamespace()},
            _resolvePortPropertyName=MagicMock(return_value="affects"),
            _getLinkEndpointPosition=MagicMock(return_value=Gf.Vec2d(11.0, 13.0)),
            nodeIdUnderCursor="constraintB",
            _dragLinkSourceNode="/ConstraintA",
        )

        GraphView._updateLinkDrag(view, QtCore.QPoint(1, 2))

        self.assertTrue(view._dragLinkValidTarget)
        self.assertEqual(
            view._dragLinkResolvedTarget,
            ("constraintB", "affects", False, "affects"),
        )
        self.assertEqual(temp_link.end, Gf.Vec2d(11.0, 13.0))
        self.assertTrue(temp_link.is_relationship_link)
        self.assertEqual(temp_link.targetPropertyName, "affects")

    def test_start_link_drag_marks_temp_link_as_relationship(self):
        temp_link = SimpleNamespace(
            sourceNodeId="",
            targetNodeId="",
            sourcePort="",
            targetPort="",
            sourcePropertyName="",
            targetPropertyName="",
            is_relationship_link=False,
            start=None,
            end=None,
        )
        node = SimpleNamespace(is_relationship_pin=MagicMock(return_value=True))
        view = SimpleNamespace(
            _draggingLink=False,
            _dragLinkSourceNode=None,
            _dragLinkSourcePort=None,
            _dragLinkSourceIsOutput=None,
            _dragLinkSyntheticTarget=("old", "", False),
            _dragLinkResolvedTarget=("old", "", False, ""),
            _dragLinkTempLink=temp_link,
            nodes={"/Constraint": node},
            _getPortPosition=MagicMock(return_value=Gf.Vec2d(7.0, 9.0)),
        )

        GraphView._startLinkDrag(view, "/Constraint", "sources", True)

        self.assertTrue(view._draggingLink)
        self.assertTrue(temp_link.is_relationship_link)
        self.assertEqual(temp_link.sourcePropertyName, "sources")
        self.assertEqual(temp_link.sourcePort, "sources")
        self.assertEqual(temp_link.start, Gf.Vec2d(7.0, 9.0))

    def test_add_link_to_graph_sets_relationship_state_before_endpoint_setup(self):
        setup_states = []
        output_node = SimpleNamespace(outputLinks=[])
        input_node = SimpleNamespace(inputLinks=[])
        view = SimpleNamespace(
            _setupLinkEndpoints=MagicMock(
                side_effect=lambda link, *_args: setup_states.append(
                    (
                        getattr(link, "is_relationship_link", False),
                        str(getattr(link, "sourcePropertyName", "")),
                        str(getattr(link, "targetPropertyName", "")),
                    )
                )
            ),
            links=[],
            _spatialIndex=SimpleNamespace(insertLink=MagicMock()),
            _computeLinkBounds=MagicMock(return_value=MagicMock()),
            _rebuildConnectionCache=MagicMock(),
            _classifyRelationshipLinkFromUsd=staticmethod(
                GraphView._classifyRelationshipLinkFromUsd
            ),
        )

        GraphView._addLinkToGraph(
            view,
            "/ConstraintA",
            "sources",
            "/ConstraintB",
            "affects",
            output_node,
            input_node,
            sourcePropertyName="sources",
            targetPropertyName="affects",
        )

        self.assertEqual(setup_states, [(True, "sources", "affects")])
        self.assertEqual(len(view.links), 1)
        self.assertTrue(view.links[0].is_relationship_link)
        self.assertEqual(view.links[0].propertyName, "sources")

    def test_add_link_to_graph_marks_unknown_relationship_via_usd_fallback(self):
        # Regression: after a reroute drop, an unknown-name relationship like
        # material:binding was being classified as a regular attribute link
        # because get_relationship_link_metadata only recognises the hardcoded
        # sources/affects fast-path names.  That left the persistent prim-target
        # arrowhead unrendered until the next USD-driven _rebuildLinks fired,
        # so the user saw the noodle "ending under the prim" with no arrow on
        # top.  _addLinkToGraph now falls back to a live prim.GetRelationship
        # probe so the link is correctly classified at insert time.
        rel = SimpleNamespace(IsValid=MagicMock(return_value=True))
        source_prim = MagicMock()
        source_prim.IsValid.return_value = True
        source_prim.GetRelationship.return_value = rel
        target_prim = MagicMock()
        target_prim.IsValid.return_value = False
        output_node = SimpleNamespace(
            id="/MeshBody",
            outputLinks=[],
            getUsdPrim=MagicMock(return_value=source_prim),
        )
        input_node = SimpleNamespace(
            id="/Material",
            inputLinks=[],
            getUsdPrim=MagicMock(return_value=target_prim),
        )
        setup_states = []
        view = SimpleNamespace(
            _setupLinkEndpoints=MagicMock(
                side_effect=lambda link, *_args: setup_states.append(
                    getattr(link, "is_relationship_link", False)
                )
            ),
            links=[],
            _spatialIndex=SimpleNamespace(insertLink=MagicMock()),
            _computeLinkBounds=MagicMock(return_value=MagicMock()),
            _rebuildConnectionCache=MagicMock(),
            _classifyRelationshipLinkFromUsd=staticmethod(
                GraphView._classifyRelationshipLinkFromUsd
            ),
        )

        GraphView._addLinkToGraph(
            view,
            "/MeshBody",
            "material:binding",
            "/Material",
            "",
            output_node,
            input_node,
            sourcePropertyName="material:binding",
            targetPropertyName="",
        )

        source_prim.GetRelationship.assert_called_with("material:binding")
        self.assertEqual(setup_states, [True])
        self.assertEqual(len(view.links), 1)
        link = view.links[0]
        self.assertTrue(link.is_relationship_link)
        self.assertEqual(link.propertyOwnerNodeId, "/MeshBody")
        self.assertEqual(link.propertyName, "material:binding")

    def test_update_hovered_port_accepts_relationship_row_hit_during_drag(self):
        renderer = SimpleNamespace(
            getPortAtPoint=MagicMock(return_value={"found": False}),
            getPortWidth=MagicMock(return_value=8.0),
            getPortMarginV=MagicMock(return_value=4.0),
            getTitleFontSize=MagicMock(return_value=12.0),
            getPortFontSize=MagicMock(return_value=10.0),
            getPortTypeFontSize=MagicMock(return_value=8.0),
            getPortSpacing=MagicMock(return_value=6.0),
        )
        node = SimpleNamespace(
            renderer=renderer,
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(120.0, 80.0),
            # Cached layout _getRowHitMetrics reads: title=12*1.0+4*2=20, row=10+8+6=24.
            layoutPortStartY=20.0,
            layoutPortLineHeight=24.0,
            outputPins=["affects"],
            _output_row_kinds=[0],
            _output_row_slots=[0],
            # C++ view-model accessor names the migrated readers consume (the
            # _*_row_kinds/_*_row_slots mirrors above are kept so the pre-rename
            # readers lower in the stack still pass).
            outputRowKinds=[0],
            outputRowSlots=[0],
            _title_collapsed=False,
            is_relationship_pin=lambda pin_name, is_output: (
                pin_name == "affects" and is_output
            ),
        )
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            defaultRenderer=None,
            fontAtlas=SimpleNamespace(ascender=0.8, descender=-0.2, lineHeight=1.0),
            _cachedPortHitRadiusMultiplier=1.5,
            _draggingLink=True,
            _dragLinkSourcePort="sources",
            _hoveredPort=None,
            nodeIdUnderCursor="constraintB",
            nodes={"constraintB": node},
            _spatialIndex=SimpleNamespace(
                queryRegion=MagicMock(return_value=(["constraintB"], []))
            ),
            _setHoveredPort=lambda new_hover: GraphView._setHoveredPort(
                view, new_hover
            ),
            _nodeRenderManager=SimpleNamespace(
                portAtPoint=MagicMock(return_value={"found": False})
            ),
            nodeGraph=SimpleNamespace(),
            _relationshipDragRowHover=lambda world_pos_arg, port_extent_arg: (
                GraphView._relationshipDragRowHover(
                    view,
                    world_pos_arg,
                    port_extent_arg,
                )
            ),
            _dragLinkTempLink=SimpleNamespace(is_relationship_link=True),
            _findRelationshipRowHover=lambda node_arg, node_id_arg, world_pos_arg: (
                GraphView._findRelationshipRowHover(
                    view,
                    node_arg,
                    node_id_arg,
                    world_pos_arg,
                )
            ),
            _getRowHitMetrics=lambda n, r: GraphView._getRowHitMetrics(view, n, r),
            _getPortPosition=MagicMock(return_value=Gf.Vec2d(130.0, 56.0)),
        )

        changed = GraphView._updateHoveredPort(view, QtCore.QPoint(60, 50))

        self.assertTrue(changed)
        self.assertEqual(view._hoveredPort, ("constraintB", "affects", True))

    def test_update_hovered_port_accepts_property_row_hit_during_relationship_drag(
        self,
    ):
        renderer = SimpleNamespace(
            getPortAtPoint=MagicMock(return_value={"found": False}),
            getPortWidth=MagicMock(return_value=8.0),
            getPortMarginV=MagicMock(return_value=4.0),
            getTitleFontSize=MagicMock(return_value=12.0),
            getPortFontSize=MagicMock(return_value=10.0),
            getPortTypeFontSize=MagicMock(return_value=8.0),
            getPortSpacing=MagicMock(return_value=6.0),
        )
        node = SimpleNamespace(
            renderer=renderer,
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(120.0, 100.0),
            # Cached layout _getRowHitMetrics reads: title=12*1.0+4*2=20, row=10+8+6=24.
            layoutPortStartY=20.0,
            layoutPortLineHeight=24.0,
            inputPins=["inputs", "sourceTranslations"],
            outputPins=["affects"],
            _input_row_kinds=[2, 3],
            _input_row_slots=[0, 1],
            _output_row_kinds=[0],
            _output_row_slots=[2],
            # C++ view-model accessor names the migrated readers consume.
            inputRowKinds=[2, 3],
            inputRowSlots=[0, 1],
            outputRowKinds=[0],
            outputRowSlots=[2],
            _title_collapsed=False,
            is_relationship_pin=lambda pin_name, is_output: (
                pin_name == "affects" and is_output
            ),
        )
        view = SimpleNamespace(
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            defaultRenderer=None,
            fontAtlas=SimpleNamespace(ascender=0.8, descender=-0.2, lineHeight=1.0),
            _cachedPortHitRadiusMultiplier=1.5,
            _draggingLink=True,
            _dragLinkSourcePort="sources",
            _hoveredPort=None,
            nodeIdUnderCursor="constraintB",
            nodes={"constraintB": node},
            _spatialIndex=SimpleNamespace(
                queryRegion=MagicMock(return_value=(["constraintB"], []))
            ),
            _setHoveredPort=lambda new_hover: GraphView._setHoveredPort(
                view, new_hover
            ),
            _nodeRenderManager=SimpleNamespace(
                portAtPoint=MagicMock(return_value={"found": False})
            ),
            nodeGraph=SimpleNamespace(),
            _relationshipDragRowHover=lambda world_pos_arg, port_extent_arg: (
                GraphView._relationshipDragRowHover(
                    view,
                    world_pos_arg,
                    port_extent_arg,
                )
            ),
            _dragLinkTempLink=SimpleNamespace(is_relationship_link=True),
            _findRelationshipRowHover=MagicMock(return_value=None),
            _findPropertyRowHover=lambda node_arg, node_id_arg, world_pos_arg: (
                GraphView._findPropertyRowHover(
                    view,
                    node_arg,
                    node_id_arg,
                    world_pos_arg,
                )
            ),
            _getRowHitMetrics=lambda n, r: GraphView._getRowHitMetrics(view, n, r),
            _getPortPosition=MagicMock(
                side_effect=lambda _node_arg, pin_name, is_output: (
                    Gf.Vec2d(10.0, 75.0)
                    if pin_name == "sourceTranslations" and not is_output
                    else (
                        Gf.Vec2d(130.0, 93.0)
                        if pin_name == "affects" and is_output
                        else None
                    )
                )
            ),
        )

        changed = GraphView._updateHoveredPort(view, QtCore.QPoint(40, 75))

        self.assertTrue(changed)
        self.assertEqual(
            view._hoveredPort,
            ("constraintB", "sourceTranslations", False),
        )

    def test_get_link_endpoint_position_uses_top_center_for_prim_target(self):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(30.0, 40.0),
        )
        view = SimpleNamespace(
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _resolveLinkDisplayPinName=MagicMock(return_value=""),
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _getPortPosition=MagicMock(),
        )

        position = GraphView._getLinkEndpointPosition(view, node, "", False, "")

        self.assertEqual(position, Gf.Vec2d(25.0, 20.0))

    def test_get_link_endpoint_position_mirrors_property_row_across_node(self):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(40.0, 30.0),
        )
        view = SimpleNamespace(
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _resolveLinkDisplayPinName=MagicMock(return_value="translate"),
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _mirrorPortPosition=GraphView._mirrorPortPosition,
            _getPortPosition=MagicMock(return_value=Gf.Vec2d(10.0, 35.0)),
        )

        position = GraphView._getLinkEndpointPosition(
            view,
            node,
            "translate",
            True,
            "inputs:translate",
        )

        self.assertEqual(position, Gf.Vec2d(50.0, 35.0))

    def test_get_link_endpoint_position_uses_property_side_when_folded_names_overlap(
        self,
    ):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(40.0, 30.0),
            _prim=None,
        )
        node.resolve_pin_name = MagicMock(
            side_effect=lambda pin_name, is_output=None: (
                "outputs" if is_output else "inputs"
            )
        )
        view = SimpleNamespace(
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _mirrorPortPosition=GraphView._mirrorPortPosition,
            _resolveLinkDisplayPinName=lambda n, p, prop, o: (
                GraphView._resolveLinkDisplayPinName(view, n, p, prop, o)
            ),
            _getPortPosition=MagicMock(
                side_effect=lambda _node_arg, pin_name, is_output: (
                    Gf.Vec2d(50.0, 35.0)
                    if pin_name == "outputs" and is_output
                    else None
                )
            ),
        )

        position = GraphView._getLinkEndpointPosition(
            view,
            node,
            "translation",
            False,
            "outputs:translation",
        )

        self.assertEqual(position, Gf.Vec2d(10.0, 35.0))
        node.resolve_pin_name.assert_called_once_with("translation", is_output=True)

    def test_get_port_position_delegates_to_resolve_port_position(self):
        # _getPortPosition is now a thin shim over the C++ View Model's
        # GraphNodeRenderer.resolvePortPosition (which owns the synthetic / dual /
        # normal geometry; covered by the C++ ResolvePortPositionTest gtests).
        renderer = SimpleNamespace(
            resolvePortPosition=MagicMock(return_value=Gf.Vec2d(50.0, 35.0))
        )
        node = SimpleNamespace(renderer=renderer)
        view = SimpleNamespace(defaultRenderer=None)

        position = GraphView._getPortPosition(view, node, "translation", True)

        self.assertEqual(position, Gf.Vec2d(50.0, 35.0))
        renderer.resolvePortPosition.assert_called_once_with(node, "translation", True)

    def test_get_port_position_falls_back_to_default_renderer(self):
        default_renderer = SimpleNamespace(
            resolvePortPosition=MagicMock(return_value=Gf.Vec2d(1.0, 2.0))
        )
        node = SimpleNamespace(renderer=None)
        view = SimpleNamespace(defaultRenderer=default_renderer)

        position = GraphView._getPortPosition(view, node, "p", False)

        self.assertEqual(position, Gf.Vec2d(1.0, 2.0))
        default_renderer.resolvePortPosition.assert_called_once_with(node, "p", False)

    def test_get_port_position_returns_none_without_a_renderer(self):
        node = SimpleNamespace(renderer=None)
        view = SimpleNamespace(defaultRenderer=None)

        self.assertIsNone(GraphView._getPortPosition(view, node, "x", True))

    def test_get_link_endpoint_position_uses_visible_output_side_for_relationship_source(
        self,
    ):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(40.0, 30.0),
            renderer=SimpleNamespace(
                getPortPosition=MagicMock(return_value=Gf.Vec2d(50.0, 35.0))
            ),
            outputPins=["affects"],
            inputPins=[],
            _dual_pin_names=set(),
        )
        view = SimpleNamespace(
            defaultRenderer=None,
            fontAtlas=None,
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _resolveLinkDisplayPinName=MagicMock(return_value="affects"),
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _mirrorPortPosition=GraphView._mirrorPortPosition,
            _getVisiblePortPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getVisiblePortPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getRelationshipRowEndpointPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getRelationshipRowEndpointPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _nodeHasVisiblePort=lambda node_arg, pin_name, is_output: (
                GraphView._nodeHasVisiblePort(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getPortPosition=MagicMock(),
        )

        position = GraphView._getLinkEndpointPosition(
            view,
            node,
            "affects",
            True,
            "affects",
            preferPropertySide=True,
        )

        self.assertEqual(position, Gf.Vec2d(50.0, 35.0))
        view._getPortPosition.assert_not_called()

    def test_get_link_endpoint_position_uses_left_row_anchor_for_relationship_target(
        self,
    ):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(40.0, 30.0),
            renderer=SimpleNamespace(
                getPortPosition=MagicMock(return_value=Gf.Vec2d(50.0, 35.0))
            ),
            outputPins=["sources"],
            inputPins=[],
            _dual_pin_names=set(),
        )
        view = SimpleNamespace(
            defaultRenderer=None,
            fontAtlas=None,
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _resolveLinkDisplayPinName=MagicMock(return_value="sources"),
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _mirrorPortPosition=GraphView._mirrorPortPosition,
            _getVisiblePortPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getVisiblePortPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getRelationshipRowEndpointPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getRelationshipRowEndpointPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _nodeHasVisiblePort=lambda node_arg, pin_name, is_output: (
                GraphView._nodeHasVisiblePort(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getPortPosition=MagicMock(),
        )

        position = GraphView._getLinkEndpointPosition(
            view,
            node,
            "sources",
            False,
            "sources",
            preferPropertySide=True,
        )

        self.assertEqual(position, Gf.Vec2d(10.0, 35.0))
        view._getPortPosition.assert_not_called()

    def test_get_link_endpoint_position_uses_collapsed_parent_row_for_relationship_target(
        self,
    ):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 20.0),
            size=Gf.Vec2d(40.0, 30.0),
            renderer=SimpleNamespace(
                getPortPosition=MagicMock(return_value=Gf.Vec2d(50.0, 35.0))
            ),
            outputPins=["Group"],
            inputPins=[],
            _dual_pin_names=set(),
        )
        view = SimpleNamespace(
            defaultRenderer=None,
            fontAtlas=None,
            _getNodeTopCenterPosition=GraphView._getNodeTopCenterPosition,
            _resolveLinkDisplayPinName=MagicMock(return_value="Group"),
            _getPropertyOutputSide=GraphView._getPropertyOutputSide,
            _mirrorPortPosition=GraphView._mirrorPortPosition,
            _getVisiblePortPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getVisiblePortPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getRelationshipRowEndpointPosition=lambda node_arg, pin_name, is_output: (
                GraphView._getRelationshipRowEndpointPosition(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _nodeHasVisiblePort=lambda node_arg, pin_name, is_output: (
                GraphView._nodeHasVisiblePort(
                    view,
                    node_arg,
                    pin_name,
                    is_output,
                )
            ),
            _getPortPosition=MagicMock(),
        )

        position = GraphView._getLinkEndpointPosition(
            view,
            node,
            "sources",
            False,
            "sources",
            preferPropertySide=True,
        )

        self.assertEqual(position, Gf.Vec2d(10.0, 35.0))
        view._getPortPosition.assert_not_called()

    def test_complete_link_drag_creates_relationship_property_link_without_replacement(
        self,
    ):
        view = SimpleNamespace(
            _dragLinkTempLink=SimpleNamespace(is_relationship_link=True),
            _dragLinkSourcePort="affects",
            _dragLinkSourceIsOutput=True,
            _dragLinkSourceNode="/Constraint",
            _dragLinkValidTarget=True,
            _dragLinkResolvedTarget=(
                "/Driven",
                "translate",
                False,
                "inputs:translate",
            ),
            _dragLinkSyntheticTarget=None,
            _hoveredPort=("ignored", "ignored", False),
            _findLinkToInput=MagicMock(),
            _createConnection=MagicMock(),
            _hasExactLink=MagicMock(return_value=False),
            update=MagicMock(),
            nodes={},
            _reconnectingLink=None,
        )

        GraphView._completeLinkDrag(view)

        view._findLinkToInput.assert_not_called()
        view._createConnection.assert_called_once_with(
            "/Constraint",
            "affects",
            "/Driven",
            "translate",
            sourcePropertyName="affects",
            targetPropertyName="inputs:translate",
        )
        self.assertIsNone(view._dragLinkResolvedTarget)

    def test_complete_link_drag_creates_self_relationship_prim_link(self) -> None:
        view = SimpleNamespace(
            _dragLinkTempLink=SimpleNamespace(is_relationship_link=True),
            _dragLinkSourcePort="sources",
            _dragLinkSourceIsOutput=True,
            _dragLinkSourceNode="/Constraint",
            _dragLinkValidTarget=True,
            _dragLinkResolvedTarget=None,
            _dragLinkSyntheticTarget=("/Constraint", "", False, ""),
            _hoveredPort=None,
            _findLinkToInput=MagicMock(),
            _createConnection=MagicMock(),
            _hasExactLink=MagicMock(return_value=False),
            update=MagicMock(),
            nodes={},
            _reconnectingLink=None,
        )

        GraphView._completeLinkDrag(view)

        view._findLinkToInput.assert_not_called()
        view._createConnection.assert_called_once_with(
            "/Constraint",
            "sources",
            "/Constraint",
            "",
            sourcePropertyName="sources",
            targetPropertyName="",
        )
        self.assertIsNone(view._dragLinkSyntheticTarget)

    def test_find_link_to_input_skips_sources_but_keeps_regular_inputs(self):
        regular_link = SimpleNamespace(targetNodeId="/Node", targetPort="weight")
        sources_link = SimpleNamespace(targetNodeId="/Node", targetPort="sources")
        view = SimpleNamespace(links=[sources_link, regular_link])

        self.assertIsNone(GraphView._findLinkToInput(view, "/Node", "sources"))
        self.assertIs(GraphView._findLinkToInput(view, "/Node", "weight"), regular_link)

    def test_render_relationship_arrowheads_draws_triangle_vertices(self):
        link = _relationship_link()
        view = SimpleNamespace(
            zoom=1.0,
            links=[link],
            _cachedLinkRelationshipBaseColor=RELATIONSHIP_LINK_BASE_COLOR + (1.0,),
            _appendTriangleVertices=lambda v, pts, d, c: (
                GraphView._appendTriangleVertices(None, v, pts, d, c)
            ),
            _drawNodeVertices=MagicMock(),
        )
        view._relationshipLinkColor = lambda rel_link, hovered, selected: (
            GraphView._relationshipLinkColor(view, rel_link, hovered, selected)
        )

        with patch(
            "pxr.UsdNoodles.graphView.NodeVertex", side_effect=_fake_node_vertex
        ):
            GraphView._renderRelationshipArrowheads(
                view,
                _Projection(),
                (1.0, 1.0, 0.0, 1.0),
                (1.0, 1.0, 0.3, 1.0),
            )

        drawn_vertices = view._drawNodeVertices.call_args.args[0]
        self.assertEqual(len(drawn_vertices), 3)

    def test_navigate_along_relationship_link_jumps_to_target_when_near_source(self):
        link = _relationship_link()
        # Source-side click — should navigate to the (off-screen) target.
        worldPos = Gf.Vec2d(0.0, 0.0)
        view = SimpleNamespace(
            nodes={"/Constraint": object(), "/Driven": object()},
            _jumpToTargetNodeInternal=MagicMock(),
            _jumpToSourceNodeInternal=MagicMock(),
        )

        result = GraphView._navigateAlongRelationshipLink(view, link, worldPos)

        self.assertTrue(result)
        view._jumpToTargetNodeInternal.assert_called_once_with(link, "/Driven")
        view._jumpToSourceNodeInternal.assert_not_called()

    def test_navigate_along_relationship_link_jumps_to_source_when_near_target(self):
        link = _relationship_link()
        # Target-side click — should navigate back to the source.
        worldPos = Gf.Vec2d(20.0, 0.0)
        view = SimpleNamespace(
            nodes={"/Constraint": object(), "/Driven": object()},
            _jumpToTargetNodeInternal=MagicMock(),
            _jumpToSourceNodeInternal=MagicMock(),
        )

        result = GraphView._navigateAlongRelationshipLink(view, link, worldPos)

        self.assertTrue(result)
        view._jumpToSourceNodeInternal.assert_called_once_with(link, "/Constraint")
        view._jumpToTargetNodeInternal.assert_not_called()

    def test_navigate_along_relationship_link_falls_back_when_preferred_missing(self):
        link = _relationship_link()
        # Click is near source so target is preferred; but target node is not
        # present in self.nodes, so we should fall back to the source jump.
        worldPos = Gf.Vec2d(0.0, 0.0)
        view = SimpleNamespace(
            nodes={"/Constraint": object()},
            _jumpToTargetNodeInternal=MagicMock(),
            _jumpToSourceNodeInternal=MagicMock(),
        )

        result = GraphView._navigateAlongRelationshipLink(view, link, worldPos)

        self.assertTrue(result)
        view._jumpToSourceNodeInternal.assert_called_once_with(link, "/Constraint")
        view._jumpToTargetNodeInternal.assert_not_called()

    def test_navigate_along_relationship_link_returns_false_when_no_nodes_known(self):
        link = _relationship_link()
        view = SimpleNamespace(
            nodes={},
            _jumpToTargetNodeInternal=MagicMock(),
            _jumpToSourceNodeInternal=MagicMock(),
        )

        result = GraphView._navigateAlongRelationshipLink(
            view, link, Gf.Vec2d(10.0, 0.0)
        )

        self.assertFalse(result)
        view._jumpToTargetNodeInternal.assert_not_called()
        view._jumpToSourceNodeInternal.assert_not_called()

    def test_render_relationship_arrowheads_skips_dangling_links(self):
        """Dangling relationship links should NOT draw an arrowhead.

        When the prim on the other end of a relationship is absent from the
        graph, the link is marked dangling. Only the circle indicator should
        be drawn; the noodle body and the arrowhead must both be suppressed
        so the user sees a stub instead of a connection.
        """
        link = _relationship_link()
        link.isDangling = True
        view = SimpleNamespace(
            zoom=1.0,
            links=[link],
            _cachedLinkRelationshipBaseColor=RELATIONSHIP_LINK_BASE_COLOR + (1.0,),
            _appendTriangleVertices=lambda v, pts, d, c: (
                GraphView._appendTriangleVertices(None, v, pts, d, c)
            ),
            _drawNodeVertices=MagicMock(),
        )
        view._relationshipLinkColor = lambda rel_link, hovered, selected: (
            GraphView._relationshipLinkColor(view, rel_link, hovered, selected)
        )

        with patch(
            "pxr.UsdNoodles.graphView.NodeVertex", side_effect=_fake_node_vertex
        ):
            GraphView._renderRelationshipArrowheads(
                view,
                _Projection(),
                (1.0, 1.0, 0.0, 1.0),
                (1.0, 1.0, 0.3, 1.0),
            )

        view._drawNodeVertices.assert_not_called()

    def test_relationship_arrowhead_points_down_for_prim_target(self):
        link = _relationship_link()
        link.targetPort = ""
        link.targetPropertyName = ""
        link.end = Gf.Vec2d(30.0, 18.0)

        tip, base_a, base_b = GraphView._relationshipArrowheadPoints(
            link,
            10.0,
            8.0,
        )

        self.assertEqual(tip, (30.0, 18.0))
        self.assertLess(base_a[1], tip[1])
        self.assertLess(base_b[1], tip[1])
        self.assertLess(base_a[0], tip[0])
        self.assertGreater(base_b[0], tip[0])

    def test_prim_target_relationship_link_detected_for_shader_inset(self):
        # The end-shorten that used to live in _relationshipRenderEnd now happens
        # in the vertex shader, gated on the prim-target predicate (mirrored by
        # the C++ isPrimTarget). Verify the predicate still identifies a whole-prim
        # relationship target and excludes port/property targets.
        link = _relationship_link()
        link.targetPort = ""
        link.targetPropertyName = ""
        self.assertTrue(GraphView._isPrimTargetRelationshipLink(link))

        link.targetPort = "in"
        self.assertFalse(GraphView._isPrimTargetRelationshipLink(link))

    def test_paint_links_leaves_prim_target_end_unmutated(self):
        # The prim-target end inset moved into the vertex shader, so paintLinks no
        # longer mutates link.end (the old Python prim_target_tips hack). Verify
        # the end is left untouched and rendering goes through the held-vector path
        # (syncLinksFromModels + renderLinksFromGraph), not the per-call renderLinks.
        relationship_link = _relationship_link()
        relationship_link.targetPort = ""
        relationship_link.targetPropertyName = ""
        relationship_link.end = Gf.Vec2d(30.0, 18.0)

        render_from_graph = MagicMock()
        view = SimpleNamespace(
            linkRenderer=SimpleNamespace(renderLinksFromGraph=render_from_graph),
            nodeGraph=SimpleNamespace(syncLinksFromModels=MagicMock()),
            links=[relationship_link],
            _linkUnderCursor=-1,
            _lastSyncedLinkHover=-1,
            _linksNeedSync=True,
            _worldSpaceProjectionMatrix=lambda: _Projection(),
            _cachedLinkHoveredColor=(1.0, 1.0, 0.0, 1.0),
            _cachedLinkSelectedColor=(1.0, 1.0, 0.3, 1.0),
            _cachedLinkAttributeBaseColor=(0.5, 0.5, 0.5, 1.0),
            _cachedLinkRelationshipBaseColor=RELATIONSHIP_LINK_BASE_COLOR + (1.0,),
            _cachedLinkHighlightedColor=(1.0, 1.0, 1.0, 1.0),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=lambda: 100,
            height=lambda: 80,
            _renderRelationshipArrowheads=MagicMock(),
            _renderConfig=lambda: None,
        )
        view._syncLinksIfNeeded = lambda graph: GraphView._syncLinksIfNeeded(
            view, graph
        )

        GraphView.paintLinks(view)

        # The wire ribbon's inset is now shader-side; link.end stays the node top.
        self.assertEqual(relationship_link.end, Gf.Vec2d(30.0, 18.0))
        view.nodeGraph.syncLinksFromModels.assert_called_once_with([relationship_link])
        self.assertTrue(render_from_graph.called)

    def test_paint_links_batches_relationship_links_with_red_base_color(self):
        relationship_link = _relationship_link()
        regular_link = SimpleNamespace(
            sourceNodeId="/A",
            sourcePort="out",
            targetNodeId="/B",
            targetPort="in",
            start=Gf.Vec2d(0.0, 0.0),
            end=Gf.Vec2d(10.0, 10.0),
            hovered=False,
            selected=False,
            is_relationship_link=False,
            isDangling=False,
        )
        render_from_graph = MagicMock()
        view = SimpleNamespace(
            linkRenderer=SimpleNamespace(renderLinksFromGraph=render_from_graph),
            nodeGraph=SimpleNamespace(syncLinksFromModels=MagicMock()),
            links=[regular_link, relationship_link],
            _linkUnderCursor=-1,
            _lastSyncedLinkHover=-1,
            _linksNeedSync=True,
            _worldSpaceProjectionMatrix=lambda: _Projection(),
            _cachedLinkHoveredColor=(1.0, 1.0, 0.0, 1.0),
            _cachedLinkSelectedColor=(1.0, 1.0, 0.3, 1.0),
            _cachedLinkAttributeBaseColor=(0.5, 0.5, 0.5, 1.0),
            _cachedLinkRelationshipBaseColor=RELATIONSHIP_LINK_BASE_COLOR + (1.0,),
            _cachedLinkHighlightedColor=(1.0, 1.0, 1.0, 1.0),
            _renderRelationshipArrowheads=MagicMock(),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            width=lambda: 100,
            height=lambda: 60,
            _renderConfig=lambda: None,
        )
        view._syncLinksIfNeeded = lambda graph: GraphView._syncLinksIfNeeded(
            view, graph
        )

        GraphView.paintLinks(view)

        # The C++ side partitions the held vector; the relationship pass is keyed
        # by relationshipOnly=True with the relationship base color, once per
        # drawSelected pass.
        relationship_calls = [
            call
            for call in render_from_graph.call_args_list
            if call.kwargs.get("relationshipOnly") is True
            and call.kwargs.get("baseColor") == list(RELATIONSHIP_LINK_BASE_COLOR)
        ]
        self.assertEqual(len(relationship_calls), 2)
        view.nodeGraph.syncLinksFromModels.assert_called_once_with(
            [regular_link, relationship_link]
        )
        view._renderRelationshipArrowheads.assert_called_once()

    def test_has_exact_link_returns_true_for_duplicate(self):
        existing = SimpleNamespace(
            sourceNodeId="/Constraint",
            sourcePort="sources",
            targetNodeId="/Driven",
            targetPort="",
            sourcePropertyName="sources",
            targetPropertyName="",
        )
        view = SimpleNamespace(links=[existing])

        self.assertTrue(
            GraphView._hasExactLink(
                view, "/Constraint", "sources", "/Driven", "", "sources", ""
            )
        )

    def test_has_exact_link_returns_false_for_different_property(self):
        existing = SimpleNamespace(
            sourceNodeId="/Constraint",
            sourcePort="sources",
            targetNodeId="/Driven",
            targetPort="",
            sourcePropertyName="sources",
            targetPropertyName="",
        )
        view = SimpleNamespace(links=[existing])

        self.assertFalse(
            GraphView._hasExactLink(
                view, "/Constraint", "sources", "/Driven", "", "OTHER_PROP", ""
            )
        )

    def test_has_exact_link_returns_false_for_empty_links(self):
        view = SimpleNamespace(links=[])

        self.assertFalse(GraphView._hasExactLink(view, "/A", "out", "/B", "in"))

    def test_complete_link_drag_rejects_duplicate_relationship_link(self):
        view = SimpleNamespace(
            _dragLinkSourcePort="affects",
            _dragLinkSourceIsOutput=True,
            _dragLinkSourceNode="/Constraint",
            _dragLinkValidTarget=True,
            _dragLinkResolvedTarget=(
                "/Driven",
                "translate",
                False,
                "inputs:translate",
            ),
            _dragLinkSyntheticTarget=None,
            _hoveredPort=("ignored", "ignored", False),
            _findLinkToInput=MagicMock(),
            _createConnection=MagicMock(),
            _hasExactLink=MagicMock(return_value=True),
            update=MagicMock(),
            nodes={},
            _reconnectingLink=False,
        )

        GraphView._completeLinkDrag(view)

        view._createConnection.assert_not_called()

    def test_complete_link_drag_cancels_reconnect_on_duplicate(self):
        view = SimpleNamespace(
            _dragLinkSourcePort="affects",
            _dragLinkSourceIsOutput=True,
            _dragLinkSourceNode="/Constraint",
            _dragLinkValidTarget=True,
            _dragLinkResolvedTarget=(
                "/Driven",
                "translate",
                False,
                "inputs:translate",
            ),
            _dragLinkSyntheticTarget=None,
            _hoveredPort=("ignored", "ignored", False),
            _findLinkToInput=MagicMock(),
            _createConnection=MagicMock(),
            _hasExactLink=MagicMock(return_value=True),
            _cancelLinkReconnect=MagicMock(),
            update=MagicMock(),
            nodes={},
            _reconnectingLink=True,
            _reconnectOldSourceNodeId="/OldConstraint",
            _reconnectOldSourcePort="old_port",
            _reconnectOldTargetNodeId="/OldDriven",
            _reconnectOldTargetPort="old_target",
        )

        GraphView._completeLinkDrag(view)

        view._createConnection.assert_not_called()
        view._cancelLinkReconnect.assert_called_once()

    def test_cancel_link_reconnect_restores_property_names(self):
        srcNode = SimpleNamespace()
        tgtNode = SimpleNamespace()
        view = SimpleNamespace(
            _reconnectingLink=True,
            _reconnectOldSourceNodeId="/Constraint",
            _reconnectOldSourcePort="sources",
            _reconnectOldTargetNodeId="/Driven",
            _reconnectOldTargetPort="",
            _reconnectOldSourcePropertyName="sources",
            _reconnectOldTargetPropertyName="",
            _clearReconnectState=MagicMock(),
            nodes={"/Constraint": srcNode, "/Driven": tgtNode},
            _addLinkToGraph=MagicMock(),
        )

        GraphView._cancelLinkReconnect(view)

        view._addLinkToGraph.assert_called_once_with(
            "/Constraint",
            "sources",
            "/Driven",
            "",
            srcNode,
            tgtNode,
            sourcePropertyName="sources",
            targetPropertyName="",
        )

    def test_start_link_reconnect_saves_property_names(self):
        link = _relationship_link()
        view = SimpleNamespace(
            links=[link],
            nodes={"/Constraint": SimpleNamespace()},
            _reconnectingLink=False,
            _reconnectOldSourceNodeId=None,
            _reconnectOldSourcePort=None,
            _reconnectOldTargetNodeId=None,
            _reconnectOldTargetPort=None,
            _reconnectOldSourcePropertyName="",
            _reconnectOldTargetPropertyName="",
            _removeLinkFromGraph=MagicMock(),
            _startLinkDrag=MagicMock(),
            _cancelLinkReconnect=MagicMock(),
            _dragLinkTempLink=SimpleNamespace(start=None, end=None),
        )

        GraphView._startLinkReconnect(view, 0, (10.0, 5.0))

        self.assertEqual(view._reconnectOldSourcePropertyName, "affects")
        self.assertEqual(view._reconnectOldTargetPropertyName, "")

    def _render_temp_link_triangle_view(self, *, end_pos, start_pos, valid_target):
        """Build a minimal view mock for _renderTemporaryLink triangle path."""
        temp_link = SimpleNamespace(
            start=start_pos,
            end=end_pos,
            is_relationship_link=True,
        )
        render_triangle = MagicMock()
        return (
            SimpleNamespace(
                _dragLinkValidTarget=valid_target,
                _hoveredPort=None,
                _dragLinkSourceIsOutput=True,
                _dragLinkSourcePort="affects",
                _dragLinkTempLink=temp_link,
                _cachedLinkHoveredColor=(1.0, 1.0, 0.0, 1.0),
                _cachedLinkSelectedColor=(1.0, 1.0, 0.3, 1.0),
                _cachedLinkActiveColor=(0.5, 0.5, 0.5, 1.0),
                _cachedLinkRelationshipBaseColor=(1.0, 0.5, 0.5, 1.0),
                linkRenderer=SimpleNamespace(renderLinks=MagicMock()),
                nodes={},
                zoom=1.0,
                panX=0.0,
                panY=0.0,
                width=lambda: 100,
                height=lambda: 80,
                _worldSpaceProjectionMatrix=lambda: _Projection(),
                _nodeRenderManager=SimpleNamespace(
                    renderEndpointTriangle=render_triangle
                ),
                defaultRenderer=SimpleNamespace(
                    getPortWidth=MagicMock(return_value=8.0)
                ),
                _cachedPortRingThickness=2.0,
                _renderConfig=lambda: None,
            ),
            render_triangle,
        )

    def test_render_temporary_link_triangle_faces_down_when_valid(self):
        view, render_tri = self._render_temp_link_triangle_view(
            end_pos=Gf.Vec2d(50.0, 30.0),
            start_pos=Gf.Vec2d(10.0, 10.0),
            valid_target=True,
        )
        GraphView._renderTemporaryLink(view)
        render_tri.assert_called_once()
        args = render_tri.call_args[0]
        tip_y = args[1]
        base1_y = args[3]
        base2_y = args[5]
        self.assertGreater(tip_y, base1_y)
        self.assertGreater(tip_y, base2_y)

    def test_render_temporary_link_triangle_tip_sits_on_prim_top_edge(self):
        # Regression: previously the snap-target arrow was offset by
        # arrow_length/2 DOWN from endPos, which put the tip inside the
        # prim's body and made the arrowhead read as "under" the node
        # while rerouting a relationship link. The tip must land exactly on
        # the prim's top edge so the drag visual matches the persistent
        # arrow drawn after the drop completes.
        view, render_tri = self._render_temp_link_triangle_view(
            end_pos=Gf.Vec2d(50.0, 30.0),
            start_pos=Gf.Vec2d(10.0, 10.0),
            valid_target=True,
        )
        GraphView._renderTemporaryLink(view)
        args = render_tri.call_args[0]
        tip_x = args[0]
        tip_y = args[1]
        self.assertEqual(tip_x, 50.0)
        self.assertEqual(tip_y, 30.0)

    def test_render_temporary_link_triangle_faces_right_when_dx_positive(self):
        view, render_tri = self._render_temp_link_triangle_view(
            end_pos=Gf.Vec2d(50.0, 30.0),
            start_pos=Gf.Vec2d(10.0, 30.0),
            valid_target=False,
        )
        GraphView._renderTemporaryLink(view)
        render_tri.assert_called_once()
        args = render_tri.call_args[0]
        tip_x = args[0]
        base1_x = args[2]
        base2_x = args[4]
        self.assertGreater(tip_x, base1_x)
        self.assertGreater(tip_x, base2_x)

    def test_render_temporary_link_triangle_faces_left_when_dx_negative(self):
        view, render_tri = self._render_temp_link_triangle_view(
            end_pos=Gf.Vec2d(10.0, 30.0),
            start_pos=Gf.Vec2d(50.0, 30.0),
            valid_target=False,
        )
        GraphView._renderTemporaryLink(view)
        render_tri.assert_called_once()
        args = render_tri.call_args[0]
        tip_x = args[0]
        base1_x = args[2]
        base2_x = args[4]
        self.assertLess(tip_x, base1_x)
        self.assertLess(tip_x, base2_x)

    def test_find_connection_authoring_layer_returns_first_match_when_two_layers_author(
        self,
    ):
        layer_a = SimpleNamespace(identifier="/root.usda")
        layer_b = SimpleNamespace(identifier="/overlay.usda")
        stage = MagicMock()
        stage.GetLayerStack.return_value = [layer_a, layer_b]
        input_prim = MagicMock()
        input_prim.IsValid.return_value = True
        input_prim.GetPath.return_value = "/NodeB"
        output_prim = MagicMock()
        output_prim.IsValid.return_value = True
        output_prim.GetPath.return_value = "/NodeA"
        attr_paths = [("/NodeB.inputs:x", "/NodeA")]
        view = SimpleNamespace(
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _buildAttrSearchPaths=MagicMock(return_value=attr_paths),
            _buildRelSearchEntries=MagicMock(return_value=[]),
            _layerAuthorsAttrConnection=MagicMock(return_value=True),
            _layerAuthorsRelTarget=MagicMock(return_value=False),
        )

        result = GraphView._findConnectionAuthoringLayer(
            view, input_prim, "x", output_prim, "result"
        )

        self.assertEqual(result, "/root.usda")
        view._layerAuthorsAttrConnection.assert_called_once_with(layer_a, attr_paths)


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class NodeGraphRelationshipHelpersTest(unittest.TestCase):
    def test_stage_helper_copies_relationship_link_metadata(self):
        node = SimpleNamespace(inputLinks=[], outputLinks=[])

        with patch(
            "pxr.UsdNoodles.nodeGraphStage.collect_links_for_prim",
            return_value=[
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "sources",
                    "targetNodeId": "/Driver",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "",
                }
            ],
        ):
            append_stage_collected_links(node, MagicMock())

        self.assertEqual(node.outputLinks[0].propertyOwnerNodeId, "/Constraint")
        self.assertEqual(node.outputLinks[0].sourcePropertyName, "sources")
        self.assertTrue(node.outputLinks[0].is_relationship_link)

    def test_blueprint_helper_filters_root_relationship_links(self):
        node = SimpleNamespace(inputLinks=[], outputLinks=[])

        with patch(
            "pxr.UsdNoodles.nodeGraphBlueprint.collect_links_for_prim",
            return_value=[
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "sources",
                    "targetNodeId": "/Blueprint",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "",
                },
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "affects",
                    "targetNodeId": "/Driven",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "affects",
                    "is_relationship_link": True,
                    "sourcePropertyName": "affects",
                    "targetPropertyName": "",
                },
            ],
        ):
            append_blueprint_relationship_links(node, MagicMock(), "/Blueprint")

        self.assertEqual(len(node.inputLinks), 0)
        self.assertEqual(len(node.outputLinks), 1)
        self.assertEqual(node.outputLinks[0].propertyName, "affects")

    def test_container_helper_filters_root_relationship_links(self):
        node = SimpleNamespace(inputLinks=[], outputLinks=[])

        with patch(
            "pxr.UsdNoodles.nodeGraphBpContainer.collect_links_for_prim",
            return_value=[
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "sources",
                    "targetNodeId": "/Container",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "",
                },
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "affects",
                    "targetNodeId": "/Driven",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "affects",
                    "is_relationship_link": True,
                    "sourcePropertyName": "affects",
                    "targetPropertyName": "",
                },
            ],
        ):
            append_container_relationship_links(node, MagicMock(), "/Container")

        self.assertEqual(len(node.inputLinks), 0)
        self.assertEqual(len(node.outputLinks), 1)
        self.assertEqual(node.outputLinks[0].targetNodeId, "/Driven")


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class LinkClickDispatchTest(unittest.TestCase):
    """Verify isDangling controls whether click selects or starts reconnect."""

    def _make_link(self, *, isDangling):
        return SimpleNamespace(
            isDangling=isDangling,
            selected=False,
            hovered=False,
            highlighted=False,
            sourceNodeId="/A",
            sourcePort="out",
            targetNodeId="/B",
            targetPort="in",
            start=Gf.Vec2d(0.0, 0.0),
            end=Gf.Vec2d(20.0, 0.0),
            is_relationship_link=False,
            sourcePropertyName="out",
            targetPropertyName="in",
            data_sourceNodeId="/A",
            data_targetNodeId="/B",
            is_input_link=False,
        )

    def _make_view(self, link):
        return SimpleNamespace(
            setFocus=MagicMock(),
            lastMousePos=None,
            ctrlPressedOnMouseDown=False,
            mousePosOnMouseDown=None,
            nodeCreationHotbox=SimpleNamespace(is_visible=False),
            minimap=SimpleNamespace(handleMousePress=MagicMock(return_value=False)),
            # mousePressEvent now gates the minimap branch on this; production
            # sets it in _initCachedSettings (default True), which this
            # unbound-call fixture never runs.
            _cachedShowMinimap=True,
            _updateNodeUnderCursor=MagicMock(),
            _reconcileMovedNodes=MagicMock(),
            spacePressed=False,
            nodeIdUnderCursor=None,
            nodes={},
            _hoveredPort=None,
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            links=[link],
            linkRenderer=SimpleNamespace(
                findLinkUnderCursor=MagicMock(return_value=0),
            ),
            _cachedLinkLineWidth=12.0,
            _spatialIndex=None,
            _selectedNodes=set(),
            _selectedLinks=set(),
            clearSelection=MagicMock(),
            clearLinkSelection=MagicMock(),
            _selectLink=MagicMock(),
            _startLinkReconnect=MagicMock(),
            # No relationship arrowhead under the cursor (these are data links),
            # so the click falls through to the regular link-dispatch path.
            _findArrowheadUnderCursor=MagicMock(return_value=-1),
            _toggleLinkSelection=MagicMock(),
            _updateLinkSelectionFromNodes=MagicMock(),
            _syncLinkSelectionToPrimtree=MagicMock(),
            marqueeActive=False,
            marqueeStart=None,
            marqueeCurrent=None,
            update=MagicMock(),
            draggingNodes=False,
            _nextZOrder=0,
            textChanged=False,
            linksChanged=False,
        )

    def _make_event(self):
        event = MagicMock()
        event.buttons.return_value = QtCore.Qt.MouseButton.LeftButton
        event.position.return_value.toPoint.return_value = QtCore.QPoint(50, 50)
        event.modifiers.return_value = QtCore.Qt.KeyboardModifier(0)
        return event

    def test_click_dangling_link_selects(self):
        link = self._make_link(isDangling=True)
        view = self._make_view(link)
        GraphView.mousePressEvent(view, self._make_event())

        view._selectLink.assert_called_once_with(0, addToSelection=False)
        view._startLinkReconnect.assert_not_called()

    def test_click_regular_link_starts_reconnect(self):
        link = self._make_link(isDangling=False)
        view = self._make_view(link)
        GraphView.mousePressEvent(view, self._make_event())

        view._startLinkReconnect.assert_called_once()
        view._selectLink.assert_not_called()


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class RowHitMetricsSchemaTypeTest(unittest.TestCase):
    """_getRowHitMetrics reserves the schema-type subtitle line (Bug 2).

    Drives the real GraphView method unbound (like LinkClickDispatchTest above)
    so the production schema-offset branch is exercised, not just the shim
    mirror in testUsdNoodlesPinDragInteractions. After REQ 1 every USD-prim node
    carries a schemaTypeName; the renderer reserves an extra line for the
    subtitle, so the first row's hit band must shift down to stay aligned with
    the rendered pins/noodles.
    """

    def _renderer(self):
        return SimpleNamespace(
            getPortMarginV=MagicMock(return_value=4.0),
            getTitleFontSize=MagicMock(return_value=14.0),
            getPortFontSize=MagicMock(return_value=10.0),
            getPortTypeFontSize=MagicMock(return_value=8.0),
            getPortSpacing=MagicMock(return_value=2.0),
            getPortWidth=MagicMock(return_value=8.0),
        )

    def _view(self):
        return SimpleNamespace(
            fontAtlas=SimpleNamespace(ascender=0.8, descender=-0.2, lineHeight=1.2),
        )

    def _node(self, schemaTypeName):
        # The migrated _getRowHitMetrics reads the cached layout (layoutPortStartY
        # / layoutPortLineHeight) instead of computing from the renderer. layoutNode
        # bakes the title + (optional schema subtitle) into layoutPortStartY, so a
        # typed node's band starts below a bare node's; model that here. (The
        # schema-subtitle offset itself is unit-tested in NoodlesCoreTest.) Values
        # mirror the renderer mocks so this also holds on the pre-migration
        # renderer-computed path in the lower diffs.
        return SimpleNamespace(
            position=Gf.Vec2d(0.0, 0.0),
            size=Gf.Vec2d(100.0, 200.0),
            schemaTypeName=schemaTypeName,
            layoutPortLineHeight=10.0 * 1.2 + 8.0 * 1.2 + 2.0,
            layoutPortStartY=(
                14.0 * 1.0
                + 4.0 * 2.0
                + 4.0
                + (14.0 * 0.65 * 1.2 if schemaTypeName else 0.0)
            ),
        )

    def test_schema_type_shifts_first_row_band_down(self):
        view = self._view()
        renderer = self._renderer()
        typed_start, typed_h = GraphView._getRowHitMetrics(
            view, self._node("IrFkController"), renderer
        )
        bare_start, bare_h = GraphView._getRowHitMetrics(view, self._node(""), renderer)
        # Bare baseline: pos.y + (titleFont*(asc-desc) + marginV*2) + marginV
        #             = 0 + (14*1.0 + 8) + 4 = 26.0
        self.assertAlmostEqual(bare_start, 26.0)
        # A schemaTypeName adds a positive subtitle line, pushing the band down.
        self.assertGreater(typed_start, bare_start)
        # Row height itself is unchanged; only the start offset moves.
        self.assertAlmostEqual(typed_h, bare_h)
        self.assertAlmostEqual(bare_h, 10.0 * 1.2 + 8.0 * 1.2 + 2.0)


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class RowEdgeDragStartTest(unittest.TestCase):
    """Pressing a row's outer-tenth edge band starts a connection.

    Drives the real GraphView._findRowEdgeDragStart end-to-end (it calls the
    real _pinAtRowEdge -> _rowEdgePinCandidates / _getRowHitMetrics), unlike the
    shim mirror in testUsdNoodlesPinDragInteractions. It covers REQ 4 for
    pin-less rows: REQ 3 hides a pin glyph until it is connected or dragged, so
    on a freshly added node the row carries no glyph, yet a press in the outer
    tenth of either side must still start a drag (aim near the node edge, not the
    inward-offset label). The middle 80% of the row stays a node-body drag
    handle. Only driving the real method exercises the real schema-subtitle row
    offset together with the edge-band resolution.
    """

    def _renderer(self):
        return SimpleNamespace(
            getPortMarginV=MagicMock(return_value=4.0),
            getTitleFontSize=MagicMock(return_value=14.0),
            getPortFontSize=MagicMock(return_value=10.0),
            getPortTypeFontSize=MagicMock(return_value=8.0),
            getPortSpacing=MagicMock(return_value=2.0),
            getPortWidth=MagicMock(return_value=8.0),
        )

    def _node(self):
        # Typed node (schemaTypeName set, like every REQ 1 USD-prim node), one
        # input row and one output row, both at slot 0. No glyphs are drawn
        # (REQ 3) since neither pin is connected.
        return SimpleNamespace(
            position=Gf.Vec2d(0.0, 0.0),
            size=Gf.Vec2d(100.0, 200.0),
            schemaTypeName="IrFkController",
            renderer=None,
            inputPins=["a"],
            outputPins=["x"],
            _input_row_slots=[0],
            _output_row_slots=[0],
            _input_row_kinds=[0],
            _output_row_kinds=[0],
            # Cached layout the migrated _getRowHitMetrics reads at PR7+ (typed
            # node: title 14*1.0 + marginV*2 + marginV + schema subtitle
            # 14*0.65*1.2 = 36.92; row = portFont*1.2 + portTypeFont*1.2 +
            # spacing = 23.6). y=45 lands in the first row band [36.92, 60.52];
            # y=5 is above it.
            layoutPortStartY=14.0 * 1.0 + 4.0 * 2.0 + 4.0 + 14.0 * 0.65 * 1.2,
            layoutPortLineHeight=10.0 * 1.2 + 8.0 * 1.2 + 2.0,
            # C++ view-model accessor names the migrated _rowEdgePinCandidates /
            # _getRowHitMetrics read (slot==index==0 here, so the toy node still
            # passes; grouped nodes where slot != index exercise the real path).
            inputRowSlots=[0],
            outputRowSlots=[0],
            inputRowKinds=[0],
            outputRowKinds=[0],
            _dual_pin_names=set(),
            _title_collapsed=False,
        )

    def _view(self):
        view = SimpleNamespace(
            fontAtlas=SimpleNamespace(ascender=0.8, descender=-0.2, lineHeight=1.2),
            defaultRenderer=self._renderer(),
        )
        # Bind the real production methods so the full resolution stack runs.
        view._getRowHitMetrics = GraphView._getRowHitMetrics.__get__(view)
        view._rowEdgePinCandidates = GraphView._rowEdgePinCandidates.__get__(view)
        view._pinAtRowEdge = GraphView._pinAtRowEdge.__get__(view)
        view._findRowEdgeDragStart = GraphView._findRowEdgeDragStart.__get__(view)
        # No glyph is drawn (REQ 3), so there is no port position to snap to;
        # this exercises the band-center fallback inside _pinAtRowEdge.
        view._getPortPosition = lambda node, pin, is_output: None
        return view

    def test_edge_press_on_typed_node_row_starts_drag(self):
        # Band = 10% of the 100px node = 10px on each side. A press in the outer
        # tenth resolves to the row's pin even though no glyph is drawn (REQ 3);
        # the side is the one pressed. y=45 lands in the first row band
        # regardless of the schema-subtitle offset.
        view = self._view()
        node = self._node()
        self.assertEqual(
            view._findRowEdgeDragStart(node, Gf.Vec2d(3.0, 45.0)), ("a", False)
        )
        self.assertEqual(
            view._findRowEdgeDragStart(node, Gf.Vec2d(97.0, 45.0)), ("x", True)
        )

    def test_middle_press_falls_through_to_node_drag(self):
        # The middle 80% of the row is a node-body drag handle, so a press there
        # returns None and the node moves instead of starting a connection.
        view = self._view()
        self.assertIsNone(
            view._findRowEdgeDragStart(self._node(), Gf.Vec2d(50.0, 45.0))
        )

    def test_press_above_first_row_falls_through(self):
        # In the left edge band horizontally, but above the first row band
        # vertically -> no pin -> None (falls through to node selection/drag).
        view = self._view()
        self.assertIsNone(view._findRowEdgeDragStart(self._node(), Gf.Vec2d(3.0, 5.0)))


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class ChangeBlockBatchingTest(unittest.TestCase):
    """Verify _applyRestoreDeleted and _applyDeleteSelected use Sdf.ChangeBlock."""

    def _make_view(self):
        stage = MagicMock()
        return SimpleNamespace(
            nodeGraph=SimpleNamespace(getStage=MagicMock(return_value=stage)),
            _groupNodesByLayer=MagicMock(return_value={}),
            _groupLinksByLayer=MagicMock(return_value={}),
            _setNodesActiveOnLayer=MagicMock(),
            _runLinkOperationOnLayer=MagicMock(),
        )

    @patch("pxr.Sdf.ChangeBlock")
    def test_applyRestoreDeleted_uses_change_block(self, mock_block):
        view = self._make_view()
        GraphView._applyRestoreDeleted(view, [], [])
        mock_block.assert_called_once()
        mock_block.return_value.__enter__.assert_called_once()

    @patch("pxr.Sdf.ChangeBlock")
    def test_applyDeleteSelected_uses_change_block(self, mock_block):
        view = self._make_view()
        GraphView._applyDeleteSelected(view, [], [])
        mock_block.assert_called_once()
        mock_block.return_value.__enter__.assert_called_once()


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class SurfaceConnectionEndpointPinsTest(unittest.TestCase):
    """_surfaceConnectionEndpointPins materializes a pin for every endpoint.

    A connection can reference a property that is not an authored/declared
    attribute on its prim (e.g. a typed prim whose schema is not registered, so
    its outputs exist only as the targets of other prims' connections). Such an
    endpoint has no pin row, so the noodle attaches to the bare node edge. This
    pass surfaces each non-relationship endpoint as a dual pin on its node.
    """

    def _link(self, src_id, src_port, tgt_id, tgt_port, *, is_rel=False):
        return SimpleNamespace(
            sourceNodeId=src_id,
            sourcePort=src_port,
            targetNodeId=tgt_id,
            targetPort=tgt_port,
            is_relationship_link=is_rel,
        )

    def _node(self, node_id, *, input_links=None, output_links=None, add_returns=True):
        calls = []

        def add(input_names=(), output_names=()):
            calls.append((list(input_names), list(output_names)))
            return add_returns

        return SimpleNamespace(
            id=node_id,
            inputLinks=list(input_links or []),
            outputLinks=list(output_links or []),
            add_connection_endpoint_pins=add,
            added_calls=calls,
        )

    def _view(self, nodes, *, font_atlas=True):
        return SimpleNamespace(
            nodes={n.id: n for n in nodes},
            fontAtlas=object() if font_atlas else None,
            textRenderer=SimpleNamespace(calculateTextWidth=lambda *a, **k: 0.0),
            nodeGraph=SimpleNamespace(_calculateNodeSize=MagicMock()),
        )

    def test_surfaces_producer_output_and_consumer_input(self):
        # The link is stored on the consumer; it names the producer as source.
        link = self._link("/FK1", "space", "/Switch1", "rig1:space")
        fk1 = self._node("/FK1")
        switch1 = self._node("/Switch1", input_links=[link])
        view = self._view([fk1, switch1])

        GraphView._surfaceConnectionEndpointPins(view)

        # Producer FK1 gains the output endpoint "space".
        self.assertEqual(fk1.added_calls, [([], ["space"])])
        # Consumer Switch1 gains the input endpoint "rig1:space".
        self.assertEqual(switch1.added_calls, [(["rig1:space"], [])])

    def test_relationship_links_are_skipped(self):
        link = self._link("/A", "affects", "/B", "x", is_rel=True)
        node_a = self._node("/A", output_links=[link])
        node_b = self._node("/B")
        view = self._view([node_a, node_b])

        GraphView._surfaceConnectionEndpointPins(view)

        self.assertEqual(node_a.added_calls, [])
        self.assertEqual(node_b.added_calls, [])

    def test_endpoint_to_absent_node_is_ignored(self):
        # Source node not in the graph: only the present endpoint is surfaced.
        link = self._link("/Absent", "out", "/B", "in")
        node_b = self._node("/B", input_links=[link])
        view = self._view([node_b])

        GraphView._surfaceConnectionEndpointPins(view)

        self.assertEqual(node_b.added_calls, [(["in"], [])])

    def test_recomputes_node_size_when_pins_added(self):
        link = self._link("/FK1", "space", "/Switch1", "rig1:space")
        fk1 = self._node("/FK1")
        switch1 = self._node("/Switch1", input_links=[link])
        view = self._view([fk1, switch1])

        GraphView._surfaceConnectionEndpointPins(view)

        self.assertEqual(view.nodeGraph._calculateNodeSize.call_count, 2)

    def test_no_size_recompute_when_nothing_added(self):
        link = self._link("/FK1", "space", "/Switch1", "rig1:space")
        fk1 = self._node("/FK1", add_returns=False)
        switch1 = self._node("/Switch1", input_links=[link], add_returns=False)
        view = self._view([fk1, switch1])

        GraphView._surfaceConnectionEndpointPins(view)

        view.nodeGraph._calculateNodeSize.assert_not_called()

    def test_no_font_atlas_skips_size_recompute_but_still_surfaces(self):
        link = self._link("/FK1", "space", "/Switch1", "rig1:space")
        fk1 = self._node("/FK1")
        switch1 = self._node("/Switch1", input_links=[link])
        view = self._view([fk1, switch1], font_atlas=False)

        GraphView._surfaceConnectionEndpointPins(view)

        view.nodeGraph._calculateNodeSize.assert_not_called()
        self.assertEqual(fk1.added_calls, [([], ["space"])])
