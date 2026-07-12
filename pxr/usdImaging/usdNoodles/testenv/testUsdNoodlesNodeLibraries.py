#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Tests for the generic UsdPrimLibrary node library.

Covers connection authoring/deletion (inherited from UsdTypedPrimLibraryBase)
and link collection. The per-schema libraries (AssetData, GraphComposition,
OpenRig) were removed in favor of this single generic catch-all, so all prim
types are now handled by UsdPrimLibrary.
"""

import unittest
from unittest.mock import MagicMock, patch

try:
    from pxr.UsdNoodles.nodeLibs.usdPrimLibrary import UsdPrimLibrary

    _has_libs = True
except ImportError:
    _has_libs = False


def _mock_prim(type_name: str) -> MagicMock:
    """Create a mock Usd.Prim with the given type name."""
    prim = MagicMock()
    prim.GetTypeName.return_value = type_name
    prim.IsValid.return_value = True
    return prim


@unittest.skipUnless(_has_libs, "node library modules not available")
class UsdPrimLibraryConnectionTest(unittest.TestCase):
    """Connection authoring/deletion behavior.

    These methods are inherited from UsdTypedPrimLibraryBase. They were
    previously exercised via OpenRigLibrary; now the generic UsdPrimLibrary
    owns every prim type, so the suite targets it directly.
    """

    def setUp(self) -> None:
        self.lib = UsdPrimLibrary()

    def test_create_connection_authors_sources_relationship(self) -> None:
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/Driver"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        source_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        source_prim.GetRelationship.return_value = invalid_relationship
        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetPath.return_value = "/Constraint.sources"
        target_relationship.GetTargets.return_value = []
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "",
            target_prim,
            "sources",
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with(["/Driver"])
        source_prim.GetAttribute.assert_not_called()
        source_prim.GetRelationship.assert_not_called()

    def test_create_connection_authors_affects_relationship(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.affects"
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/Driven"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "affects",
            target_prim,
            "",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(["/Driven"])
        target_prim.GetAttribute.assert_not_called()
        target_prim.GetRelationship.assert_not_called()

    def test_create_connection_authors_sources_relationship_from_source_pin(
        self,
    ) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.sources"
        source_relationship.GetTargets.return_value = []
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/Driver"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "sources",
            target_prim,
            "",
            source_property_name="sources",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(["/Driver"])

    def test_create_connection_authors_relationship_to_property(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.affects"
        source_relationship.GetTargets.return_value = []
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_attr = MagicMock()
        target_attr.IsValid.return_value = True
        target_attr.GetPath.return_value = "/Driven.inputs:translate"
        target_prim = MagicMock()
        target_prim.GetAttribute.return_value = target_attr
        target_prim.GetRelationship.return_value = MagicMock(
            IsValid=MagicMock(return_value=False)
        )

        result = self.lib.create_connection(
            stage,
            source_prim,
            "affects",
            target_prim,
            "translate",
            target_property_name="inputs:translate",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(
            ["/Driven.inputs:translate"]
        )

    def test_create_connection_authors_affects_relationship_to_prim(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.affects"
        source_relationship.GetTargets.return_value = []
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/Driven"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "affects",
            target_prim,
            "",
            source_property_name="affects",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(["/Driven"])

    def test_create_connection_authors_relationship_to_relationship(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/ConstraintA.sources"
        source_relationship.GetTargets.return_value = []
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetPath.return_value = "/ConstraintB.affects"
        target_prim = MagicMock()
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        target_prim.GetRelationship.return_value = target_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "sources",
            target_prim,
            "affects",
            source_property_name="sources",
            target_property_name="affects",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(["/ConstraintB.affects"])

    def test_create_connection_accumulates_attribute_connections(self) -> None:
        stage = MagicMock()
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        source_attr = MagicMock()
        source_attr.IsValid.return_value = True
        source_attr.GetPath.return_value = "/NodeA.outputs:result"
        source_prim = MagicMock()
        source_prim.GetAttribute.return_value = source_attr
        source_prim.GetRelationship.return_value = invalid_relationship
        target_attr = MagicMock()
        target_attr.IsValid.return_value = True
        target_attr.GetPath.return_value = "/NodeB.inputs:x"
        target_prim = MagicMock()
        target_prim.GetAttribute.return_value = target_attr
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "result",
            target_prim,
            "x",
        )

        self.assertTrue(result)
        target_attr.AddConnection.assert_called_once_with("/NodeA.outputs:result")
        target_attr.ClearConnections.assert_not_called()

    def test_create_connection_appends_to_existing_sources_targets(self) -> None:
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/NewDriver"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        source_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        source_prim.GetRelationship.return_value = invalid_relationship
        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetPath.return_value = "/Constraint.sources"
        target_relationship.GetTargets.return_value = ["/ExistingDriver"]
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "",
            target_prim,
            "sources",
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with(
            ["/ExistingDriver", "/NewDriver"]
        )

    def test_create_connection_appends_to_existing_affects_targets(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.affects"
        source_relationship.GetTargets.return_value = ["/ExistingDriven"]
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/NewDriven"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.create_connection(
            stage,
            source_prim,
            "affects",
            target_prim,
            "",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(
            ["/ExistingDriven", "/NewDriven"]
        )

    def test_delete_connection_removes_relationship_target(self) -> None:
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/Driver"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        source_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        source_prim.GetRelationship.return_value = invalid_relationship
        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetPath.return_value = "/Constraint.sources"
        target_relationship.GetTargets.return_value = ["/Driver"]
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship

        result = self.lib.delete_connection(
            stage,
            source_prim,
            "",
            target_prim,
            "sources",
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with([])
        source_prim.GetAttribute.assert_not_called()
        source_prim.GetRelationship.assert_not_called()

    def test_delete_connection_removes_relationship_property_target(self) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.affects"
        source_relationship.GetTargets.return_value = ["/Driven.inputs:translate"]
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_attr = MagicMock()
        target_attr.IsValid.return_value = True
        target_attr.GetPath.return_value = "/Driven.inputs:translate"
        target_prim = MagicMock()
        target_prim.GetAttribute.return_value = target_attr
        target_prim.GetRelationship.return_value = MagicMock(
            IsValid=MagicMock(return_value=False)
        )

        result = self.lib.delete_connection(
            stage,
            source_prim,
            "affects",
            target_prim,
            "translate",
            target_property_name="inputs:translate",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with([])

    def test_delete_connection_removes_sources_relationship_target_from_source_pin(
        self,
    ) -> None:
        stage = MagicMock()
        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetPath.return_value = "/Constraint.sources"
        source_relationship.GetTargets.return_value = ["/Driver"]
        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = source_relationship
        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/Driver"
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim.GetAttribute.return_value = invalid_attr
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_relationship

        result = self.lib.delete_connection(
            stage,
            source_prim,
            "sources",
            target_prim,
            "",
            source_property_name="sources",
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with([])

    def test_delete_connection_removes_attribute_connection(self) -> None:
        """Attribute-connection deletion exercises _delete_attribute_connection
        and _remove_matching_connections, including matching a connection that
        used an ``out:`` direction-hint spelling on the source attribute."""
        stage = MagicMock()
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False

        # Connection targeting the source prim with an out: hint spelling.
        conn_match = MagicMock()
        conn_match.GetPrimPath.return_value = "/NodeA"
        conn_match.name = "out:result"
        # Connection on the right prim but an unrelated property name.
        conn_wrong_name = MagicMock()
        conn_wrong_name.GetPrimPath.return_value = "/NodeA"
        conn_wrong_name.name = "outputs:other"
        # Connection pointing at a different prim entirely.
        conn_other_prim = MagicMock()
        conn_other_prim.GetPrimPath.return_value = "/Other"
        conn_other_prim.name = "out:result"

        target_attr = MagicMock()
        target_attr.IsValid.return_value = True
        target_attr.GetConnections.return_value = [
            conn_other_prim,
            conn_wrong_name,
            conn_match,
        ]

        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/NodeA"
        source_prim.GetRelationship.return_value = invalid_relationship
        source_prim.GetAttribute.return_value = invalid_attr

        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/NodeB"
        target_prim.GetRelationship.return_value = invalid_relationship
        target_prim.GetAttribute.side_effect = (
            lambda name: target_attr if name == "inputs:x" else invalid_attr
        )

        result = self.lib.delete_connection(
            stage,
            source_prim,
            "result",
            target_prim,
            "x",
        )

        self.assertTrue(result)
        target_attr.RemoveConnection.assert_called_once_with(conn_match)

    def test_resolve_endpoint_path_returns_prim_path_for_empty_endpoint(self) -> None:
        prim = MagicMock()
        prim.GetPath.return_value = "/Driven"

        result = self.lib._resolve_endpoint_path(
            prim,
            "",
            "",
            is_input=True,
        )

        self.assertEqual(result, "/Driven")
        prim.GetAttribute.assert_not_called()
        prim.GetRelationship.assert_not_called()

    def test_relationship_connections_allow_any_property_pin(self) -> None:
        self.assertTrue(
            self.lib.can_connect(
                MagicMock(),
                "affects",
                True,
                MagicMock(),
                "weight",
                False,
            )
        )


@unittest.skipUnless(_has_libs, "node library modules not available")
class LinkCollectionLibraryTest(unittest.TestCase):
    def test_get_links_for_prim_delegates_to_collect_links(self) -> None:
        prim = _mock_prim("Mesh")
        links = [
            {"is_relationship_link": False, "propertyName": "outputs:result"},
            {"is_relationship_link": True, "propertyName": "sources"},
        ]
        library = UsdPrimLibrary()

        with patch(
            "pxr.UsdNoodles.nodeLibs.usdPrimLibrary.collect_links_for_prim",
            return_value=links,
        ):
            result = library.get_links_for_prim(prim)

        self.assertEqual(result, links)

    def test_get_links_for_prim_returns_empty_for_invalid_prim(self) -> None:
        prim = MagicMock()
        prim.IsValid.return_value = False
        library = UsdPrimLibrary()

        self.assertEqual(library.get_links_for_prim(prim), [])


if __name__ == "__main__":
    unittest.main()
