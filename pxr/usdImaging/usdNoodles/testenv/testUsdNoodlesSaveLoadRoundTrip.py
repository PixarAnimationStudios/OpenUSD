#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Round-trip test: author a Noodles-shaped graph, save the layer, reload it,
verify what was authored persists.

Regression guard for the class of bug where Noodles-authored graph state
doesn't survive save-reload — specifically the historical failure mode
where connections authored via the editor were not persisted to USD on
Cmd+S (fixed in an earlier landed change).

Tests use pure pxr.Usd / pxr.Sdf (no GraphView / GL imports) so they run
in headless CI. They verify the USD-level contract Noodles depends on:

  1. Prim types (e.g. ``ExecNode``, ``Blueprint``) round-trip
  2. The ``ui:nodegraph:node:pos`` attribute (Noodles' node-position
     convention, authored by ``primAuthoring.PrimAuthor.create_node_prim``)
     round-trips
  3. Attribute connections between nodes (authored via
     ``Sdf.AttributeSpec.connectionPathList``) round-trip

If any of these regress at the USD layer, Noodles' persistence breaks.
"""

import os
import tempfile
import unittest

from pxr import Gf, Sdf, Usd


POS_ATTR_NAME = "ui:nodegraph:node:pos"


class SaveLoadRoundTripTest(unittest.TestCase):
    """Author → Save → Reload → Verify — the persistence contract."""

    def _author_stage(self, layer_path):
        """Author a small Blueprint-scoped graph with two ExecNode prims,
        node positions on the ``ui:nodegraph:node:pos`` attribute, and one
        attribute connection from Node1's output to Node2's input.

        Saves the root layer and returns the stage.
        """
        stage = Usd.Stage.CreateNew(layer_path)
        layer = stage.GetRootLayer()

        # Blueprint container (matches Noodles graph-root convention)
        bp_spec = Sdf.CreatePrimInLayer(layer, "/Root/BP")
        bp_spec.specifier = Sdf.SpecifierDef
        bp_spec.typeName = "Blueprint"

        # Node 1 with position + output attribute
        node1_spec = Sdf.CreatePrimInLayer(layer, "/Root/BP/Node1")
        node1_spec.specifier = Sdf.SpecifierDef
        node1_spec.typeName = "ExecNode"

        pos1_spec = Sdf.AttributeSpec(
            node1_spec, POS_ATTR_NAME, Sdf.ValueTypeNames.Float2
        )
        pos1_spec.default = Gf.Vec2f(1.0, 2.0)

        out_spec = Sdf.AttributeSpec(
            node1_spec, "outputs:result", Sdf.ValueTypeNames.Float
        )
        out_spec.default = 0.0

        # Node 2 with position + input attribute connected to Node 1's output
        node2_spec = Sdf.CreatePrimInLayer(layer, "/Root/BP/Node2")
        node2_spec.specifier = Sdf.SpecifierDef
        node2_spec.typeName = "ExecNode"

        pos2_spec = Sdf.AttributeSpec(
            node2_spec, POS_ATTR_NAME, Sdf.ValueTypeNames.Float2
        )
        pos2_spec.default = Gf.Vec2f(3.0, 4.0)

        in_spec = Sdf.AttributeSpec(
            node2_spec, "inputs:value", Sdf.ValueTypeNames.Float
        )
        in_spec.default = 0.0
        in_spec.connectionPathList.explicitItems = [
            Sdf.Path("/Root/BP/Node1.outputs:result")
        ]

        layer.Save()
        return stage

    def test_prim_types_survive_reload(self):
        """Type names authored via Sdf specs persist across save+reload."""
        with tempfile.TemporaryDirectory() as td:
            path = os.path.join(td, "types.usda")
            self._author_stage(path)

            stage = Usd.Stage.Open(path)
            for prim_path, expected_type in [
                ("/Root/BP", "Blueprint"),
                ("/Root/BP/Node1", "ExecNode"),
                ("/Root/BP/Node2", "ExecNode"),
            ]:
                prim = stage.GetPrimAtPath(prim_path)
                self.assertTrue(prim.IsValid(), f"Missing prim: {prim_path}")
                self.assertEqual(
                    prim.GetTypeName(),
                    expected_type,
                    f"Wrong type on {prim_path}",
                )

    def test_positions_survive_reload(self):
        """Node positions (`ui:nodegraph:node:pos`) survive save+reload.

        This is the direct USD contract exercised by
        ``primAuthoring.PrimAuthor.create_node_prim``.
        """
        with tempfile.TemporaryDirectory() as td:
            path = os.path.join(td, "positions.usda")
            self._author_stage(path)

            stage = Usd.Stage.Open(path)
            for prim_path, expected_pos in [
                ("/Root/BP/Node1", Gf.Vec2f(1.0, 2.0)),
                ("/Root/BP/Node2", Gf.Vec2f(3.0, 4.0)),
            ]:
                prim = stage.GetPrimAtPath(prim_path)
                self.assertTrue(prim.IsValid(), f"Missing prim: {prim_path}")
                attr = prim.GetAttribute(POS_ATTR_NAME)
                self.assertTrue(
                    attr.IsValid(),
                    f"{POS_ATTR_NAME} missing on {prim_path}",
                )
                self.assertEqual(
                    attr.Get(),
                    expected_pos,
                    f"Position drift on {prim_path}: authored "
                    f"{expected_pos} but reload got {attr.Get()}",
                )

    def test_connections_survive_reload(self):
        """Attribute connections between nodes survive save+reload.

        Regression guard for the class of bug where connections authored
        via the Noodles editor were not persisted to USD on save (see
        earlier landed fix around Cmd+S connection persistence).
        """
        with tempfile.TemporaryDirectory() as td:
            path = os.path.join(td, "connections.usda")
            self._author_stage(path)

            stage = Usd.Stage.Open(path)
            node2 = stage.GetPrimAtPath("/Root/BP/Node2")
            self.assertTrue(node2.IsValid())

            in_attr = node2.GetAttribute("inputs:value")
            self.assertTrue(
                in_attr.IsValid(),
                "inputs:value attribute missing after reload",
            )

            sources = in_attr.GetConnections()
            self.assertEqual(
                len(sources),
                1,
                f"Expected exactly one connection after reload, got {len(sources)}",
            )
            self.assertEqual(
                str(sources[0]),
                "/Root/BP/Node1.outputs:result",
                "Connection target drift after reload",
            )

    def test_multiple_save_reload_cycles_are_idempotent(self):
        """Multiple save+reload cycles converge to the same state — no drift."""
        with tempfile.TemporaryDirectory() as td:
            path = os.path.join(td, "idempotent.usda")
            self._author_stage(path)

            for _ in range(3):
                stage = Usd.Stage.Open(path)
                # Round-trip each cycle: re-save without modification, reopen
                stage.GetRootLayer().Save()
                del stage

            stage_final = Usd.Stage.Open(path)
            node1 = stage_final.GetPrimAtPath("/Root/BP/Node1")
            self.assertTrue(node1.IsValid())
            self.assertEqual(
                node1.GetAttribute(POS_ATTR_NAME).Get(),
                Gf.Vec2f(1.0, 2.0),
            )

            node2 = stage_final.GetPrimAtPath("/Root/BP/Node2")
            self.assertTrue(node2.IsValid())
            in_attr = node2.GetAttribute("inputs:value")
            self.assertEqual(
                str(in_attr.GetConnections()[0]),
                "/Root/BP/Node1.outputs:result",
            )


if __name__ == "__main__":
    unittest.main()
