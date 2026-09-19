#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
GraphTestHelper — high-level graph operations for headless tests.

Wraps GraphView methods into clean, assertion-friendly APIs for
creating nodes, moving them, connecting pins, and controlling
undo/redo.
"""

from pxr import Gf, Sdf


class GraphTestHelper:
    """Provides clean test APIs for graph manipulation."""

    def __init__(self, harness):
        """
        Args:
            harness: NoodlesTestHarness instance with an initialized GraphView
        """
        self.harness = harness

    @property
    def gv(self):
        return self.harness.graph_view

    def add_node(self, stage, parent_path, node_type_id, position):
        """Create an ExecNode in the USD stage and register it in the GraphView.

        Creates a ``def ExecNode`` prim with the given ``node_type_id``
        set as the ``token id`` attribute. To create Container or Shader
        prims, use ``PrimAuthor.create_node_prim`` directly.

        Args:
            stage: Usd.Stage
            parent_path: str path to parent Blueprint prim
            node_type_id: str logical node type (set as ``token id``)
            position: tuple or Gf.Vec2d (x, y) in display coordinates

        Returns:
            str: node ID (prim path string), or None on failure
        """
        from pxr.UsdNoodles.primAuthoring import PrimAuthor

        pos = Gf.Vec2d(position[0], position[1])
        prim_path = PrimAuthor.create_node_prim(stage, parent_path, "ExecNode", pos)
        if prim_path is None:
            return None

        # Set the node type ID on the prim
        layer = stage.GetEditTarget().GetLayer()
        prim_spec = layer.GetPrimAtPath(prim_path)
        if prim_spec:
            attr_spec = Sdf.AttributeSpec(prim_spec, "id", Sdf.ValueTypeNames.Token)
            if attr_spec:
                attr_spec.default = node_type_id

        prim = stage.GetPrimAtPath(prim_path)
        if not prim or not prim.IsValid():
            return None

        node_id = self.gv._addNewNodeFast(prim, stage)
        return node_id

    def move_node(self, node_id, new_position, with_undo=True):
        """Move a node to a new position.

        Args:
            node_id: str node ID (prim path)
            new_position: tuple or Gf.Vec2d (x, y) in display coordinates
            with_undo: if True, push an undo command for the move
        """
        node = self.gv.nodes.get(node_id)
        if node is None:
            raise KeyError(f"Node not found: {node_id}")

        old_pos = Gf.Vec2d(node.position)
        new_pos = Gf.Vec2d(new_position[0], new_position[1])

        if with_undo:
            from pxr.UsdNoodles._usdNoodles import pushLambdaCommand

            # Write position to USD session layer
            if not hasattr(self.gv, "nodeGraph") or self.gv.nodeGraph is None:
                raise RuntimeError(
                    "GraphView has no loaded graph — call load_blueprint first"
                )
            stage = self.gv.nodeGraph.getStage()
            prim = stage.GetPrimAtPath(node_id)

            def do_move():
                node.position = new_pos
                if prim and prim.IsValid():
                    pos_attr = prim.GetAttribute("ui:nodegraph:node:pos")
                    if pos_attr:
                        scaled = Gf.Vec2f(new_pos[0] / 1000.0, new_pos[1] / 1000.0)
                        pos_attr.Set(scaled)

            def undo_move():
                node.position = old_pos
                if prim and prim.IsValid():
                    pos_attr = prim.GetAttribute("ui:nodegraph:node:pos")
                    if pos_attr:
                        scaled = Gf.Vec2f(old_pos[0] / 1000.0, old_pos[1] / 1000.0)
                        pos_attr.Set(scaled)

            do_move()
            pushLambdaCommand("Move node", do_move, undo_move)
        else:
            node.position = new_pos

    def get_node_position(self, node_id):
        """Get a node's current position.

        Returns:
            Gf.Vec2d: node position in display coordinates
        """
        node = self.gv.nodes.get(node_id)
        if node is None:
            raise KeyError(f"Node not found: {node_id}")
        return Gf.Vec2d(node.position)

    def get_node_count(self):
        """Return the number of nodes in the graph."""
        return len(self.gv.nodes)

    def get_node_ids(self):
        """Return a list of all node IDs."""
        return list(self.gv.nodes.keys())

    def select_node(self, node_id):
        """Select a single node."""
        node = self.gv.nodes.get(node_id)
        if node is None:
            raise KeyError(f"Node not found: {node_id}")
        node.selected = True
        self.gv._selectedNodes.add(node_id)

    def clear_selection(self):
        """Clear all node selections."""
        for node in self.gv.nodes.values():
            node.selected = False
        self.gv._selectedNodes.clear()

    def undo(self):
        """Execute undo via the NoodlesUndoManager."""
        self.gv._performUndo()

    def redo(self):
        """Execute redo via the NoodlesUndoManager."""
        self.gv._performRedo()

    def can_undo(self):
        """Check if undo is available."""
        from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager

        return NoodlesUndoManager.instance().canUndo()

    def can_redo(self):
        """Check if redo is available."""
        from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager

        return NoodlesUndoManager.instance().canRedo()

    def clear_undo_stack(self):
        """Clear the undo/redo stacks."""
        from pxr.UsdNoodles._usdNoodles import NoodlesUndoManager

        NoodlesUndoManager.instance().clear()
