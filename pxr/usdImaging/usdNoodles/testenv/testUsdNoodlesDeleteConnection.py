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

try:
    from pxr import Sdf, Tf
    from pxr.UsdNoodles.nodeLibs.usdPrimLibrary import UsdPrimLibrary

    _has_libs = True
except ImportError:
    _has_libs = False


@unittest.skipUnless(_has_libs, "node library modules not available")
class DeleteConnectionTest(unittest.TestCase):
    """Exercise UsdTypedPrimLibraryBase.delete_connection through
    the relationship-target and attribute-connection paths."""

    def setUp(self):
        self.lib = UsdPrimLibrary()
        self._invalid = MagicMock()
        self._invalid.IsValid.return_value = False

    def _mock_prim(self, path_str, *, attrs=None, rels=None):
        invalid = self._invalid
        prim = MagicMock()
        prim.GetPath.return_value = Sdf.Path(path_str)
        attr_map = attrs or {}
        rel_map = rels or {}
        prim.GetAttribute = MagicMock(side_effect=lambda n: attr_map.get(n, invalid))
        prim.GetRelationship = MagicMock(side_effect=lambda n: rel_map.get(n, invalid))
        return prim

    def _valid_attr(self, name, path, connections=None):
        attr = MagicMock()
        attr.IsValid.return_value = True
        attr.GetName.return_value = name
        attr.GetPath.return_value = Sdf.Path(path)
        attr.GetConnections.return_value = list(connections or [])
        return attr

    def _valid_rel(self, path, targets=None):
        rel = MagicMock()
        rel.IsValid.return_value = True
        rel.GetPath.return_value = Sdf.Path(path)
        rel.GetTargets.return_value = list(targets or [])
        return rel

    # --- relationship input pin ---

    def test_delete_relationship_input_removes_target(self):
        endpoint = Sdf.Path("/Source.outputs:value")
        src_attr = self._valid_attr("outputs:value", "/Source.outputs:value")
        source = self._mock_prim("/Source", attrs={"outputs:value": src_attr})

        other = Sdf.Path("/Other")
        rel = self._valid_rel("/Target.sources", targets=[endpoint, other])
        target = self._mock_prim("/Target", rels={"sources": rel})

        result = self.lib.delete_connection(
            MagicMock(), source, "value", target, "sources"
        )

        self.assertTrue(result)
        rel.SetTargets.assert_called_once_with([other])

    def test_delete_relationship_input_returns_false_when_target_missing(self):
        src_attr = self._valid_attr("outputs:value", "/Source.outputs:value")
        source = self._mock_prim("/Source", attrs={"outputs:value": src_attr})

        rel = self._valid_rel("/Target.sources", targets=[Sdf.Path("/Unrelated")])
        target = self._mock_prim("/Target", rels={"sources": rel})

        result = self.lib.delete_connection(
            MagicMock(), source, "value", target, "sources"
        )

        self.assertFalse(result)
        rel.SetTargets.assert_not_called()

    # --- relationship output pin ---

    def test_delete_relationship_output_removes_target(self):
        endpoint = Sdf.Path("/Target.inputs:color")
        tgt_attr = self._valid_attr("inputs:color", "/Target.inputs:color")
        target = self._mock_prim("/Target", attrs={"inputs:color": tgt_attr})

        rel = self._valid_rel("/Source.affects", targets=[endpoint])
        source = self._mock_prim("/Source", rels={"affects": rel})

        result = self.lib.delete_connection(
            MagicMock(), source, "affects", target, "color"
        )

        self.assertTrue(result)
        rel.SetTargets.assert_called_once_with([])

    # --- attribute connection ---

    def test_delete_attribute_connection_removes_both_sides(self):
        src_conn = Sdf.Path("/Source.outputs:value")
        tgt_conn = Sdf.Path("/Target.inputs:color")

        tgt_attr = self._valid_attr(
            "inputs:color", "/Target.inputs:color", connections=[src_conn]
        )
        target = self._mock_prim("/Target", attrs={"inputs:color": tgt_attr})

        src_attr = self._valid_attr(
            "outputs:value", "/Source.outputs:value", connections=[tgt_conn]
        )
        source = self._mock_prim("/Source", attrs={"outputs:value": src_attr})

        result = self.lib.delete_connection(
            MagicMock(), source, "value", target, "color"
        )

        self.assertTrue(result)
        tgt_attr.RemoveConnection.assert_called_once_with(src_conn)
        src_attr.RemoveConnection.assert_called_once_with(tgt_conn)

    def test_delete_attribute_connection_returns_false_when_no_match(self):
        tgt_attr = self._valid_attr(
            "inputs:color", "/Target.inputs:color", connections=[]
        )
        target = self._mock_prim("/Target", attrs={"inputs:color": tgt_attr})
        source = self._mock_prim("/Source")

        result = self.lib.delete_connection(
            MagicMock(), source, "value", target, "color"
        )

        self.assertFalse(result)
        tgt_attr.RemoveConnection.assert_not_called()

    # --- error handling ---

    def test_delete_connection_catches_runtime_error(self):
        source = MagicMock()
        source.GetAttribute = MagicMock(side_effect=RuntimeError("boom"))
        target = self._mock_prim("/Target")

        result = self.lib.delete_connection(
            MagicMock(), source, "value", target, "color"
        )

        self.assertFalse(result)

    # --- log_prefix wiring ---

    def test_delete_relationship_emits_status_with_library_name(self):
        endpoint = Sdf.Path("/Source.outputs:value")
        src_attr = self._valid_attr("outputs:value", "/Source.outputs:value")
        source = self._mock_prim("/Source", attrs={"outputs:value": src_attr})

        rel = self._valid_rel("/Target.sources", targets=[endpoint])
        target = self._mock_prim("/Target", rels={"sources": rel})

        with patch.object(Tf, "Status") as mock_status:
            self.lib.delete_connection(MagicMock(), source, "value", target, "sources")
            mock_status.assert_called_once()
            self.assertIn("USD Prims", mock_status.call_args[0][0])

    def test_delete_attribute_emits_status_with_library_name(self):
        src_conn = Sdf.Path("/Source.outputs:value")
        tgt_attr = self._valid_attr(
            "inputs:color", "/Target.inputs:color", connections=[src_conn]
        )
        target = self._mock_prim("/Target", attrs={"inputs:color": tgt_attr})
        source = self._mock_prim("/Source")

        with patch.object(Tf, "Status") as mock_status:
            self.lib.delete_connection(MagicMock(), source, "value", target, "color")
            mock_status.assert_called_once()
            self.assertIn("USD Prims", mock_status.call_args[0][0])
