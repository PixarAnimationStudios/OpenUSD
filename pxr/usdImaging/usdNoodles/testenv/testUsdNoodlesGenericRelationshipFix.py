#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict
"""Tests for T272405910: generic relationship connections authored in USD layer."""

import unittest
from unittest.mock import MagicMock


class TestGenericRelationshipFix(unittest.TestCase):
    """Verify that relationships not named 'sources'/'affects' are authored."""

    def setUp(self):
        from pxr.UsdNoodles.nodeLibs.usdTypedPrimBase import UsdTypedPrimLibraryBase

        class _Lib(UsdTypedPrimLibraryBase):
            def get_name(self):
                return "test"

            def get_families(self):
                return []

            def get_node_types(self, family):
                return []

            def can_handle_prim(self, prim):
                return True

        self.lib = _Lib()

    # ------------------------------------------------------------------ #
    # create_connection                                                    #
    # ------------------------------------------------------------------ #

    def test_create_connection_authors_generic_input_relationship(self):
        """A relationship pin with any name (not 'sources') is authored on target."""
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/Driver"

        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetTargets.return_value = []
        target_relationship.GetPath.return_value = "/Driven.targets"

        # Target prim has a relationship named "targets" (not in old frozenset)
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship
        target_prim.GetAttribute.return_value = invalid_attr

        result = self.lib.create_connection(
            stage, source_prim, "", target_prim, "targets"
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with(["/Driver"])
        # Must not fall through to attribute path
        target_prim.GetAttribute.assert_not_called()

    def test_create_connection_authors_generic_output_relationship(self):
        """A relationship output pin with any name (not 'affects') is authored on source."""
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/ConstraintA"

        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetTargets.return_value = []
        source_relationship.GetPath.return_value = "/ConstraintA.bindMaterial"

        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        source_prim.GetRelationship.return_value = source_relationship
        source_prim.GetAttribute.return_value = invalid_attr

        target_prim = MagicMock()
        # Target prim has no relationship (so source-side path is taken)
        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False
        target_prim.GetRelationship.return_value = invalid_rel
        target_prim.GetPath.return_value = "/Material"

        result = self.lib.create_connection(
            stage, source_prim, "bindMaterial", target_prim, ""
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with(["/Material"])

    def test_create_connection_generic_relationship_appends_to_existing_targets(self):
        """Generic relationship authoring preserves existing targets (read-modify-write)."""
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/NewDriver"

        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetTargets.return_value = ["/ExistingDriver"]
        target_relationship.GetPath.return_value = "/Driven.targets"

        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship
        target_prim.GetAttribute.return_value = invalid_attr

        result = self.lib.create_connection(
            stage, source_prim, "", target_prim, "targets"
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with(
            ["/ExistingDriver", "/NewDriver"]
        )

    def test_create_connection_invalid_relationship_falls_through_to_attribute(self):
        """Prim with no relationship and no attribute on the given name returns False."""
        stage = MagicMock()
        source_prim = MagicMock()
        target_prim = MagicMock()

        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False

        source_prim.GetRelationship.return_value = invalid_rel
        source_prim.GetAttribute.return_value = invalid_attr
        target_prim.GetRelationship.return_value = invalid_rel
        target_prim.GetAttribute.return_value = invalid_attr

        result = self.lib.create_connection(
            stage, source_prim, "nonexistent", target_prim, "nonexistent"
        )

        self.assertFalse(result)

    # ------------------------------------------------------------------ #
    # delete_connection                                                    #
    # ------------------------------------------------------------------ #

    def test_delete_connection_removes_generic_input_relationship_target(self):
        """delete_connection removes a target from a generic input relationship."""
        stage = MagicMock()
        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/Driver"

        target_relationship = MagicMock()
        target_relationship.IsValid.return_value = True
        target_relationship.GetName.return_value = "targets"
        target_relationship.GetTargets.return_value = ["/Driver"]
        target_relationship.GetPath.return_value = "/Driven.targets"

        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = target_relationship

        result = self.lib.delete_connection(
            stage, source_prim, "", target_prim, "targets"
        )

        self.assertTrue(result)
        target_relationship.SetTargets.assert_called_once_with([])

    # ------------------------------------------------------------------ #
    # can_connect                                                          #
    # ------------------------------------------------------------------ #

    def test_can_connect_returns_true_for_generic_relationship_pin(self):
        """can_connect allows connecting to any valid USD relationship."""
        source_prim = MagicMock()
        target_prim = MagicMock()

        valid_rel = MagicMock()
        valid_rel.IsValid.return_value = True
        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False

        # target has a "targets" relationship; source does not
        target_prim.GetRelationship.return_value = valid_rel
        source_prim.GetRelationship.return_value = invalid_rel

        result = self.lib.can_connect(
            source_prim, "out", True, target_prim, "targets", False
        )

        self.assertTrue(result)

    def test_can_connect_returns_false_when_neither_side_is_relationship_or_attr(self):
        """can_connect rejects when both sides have no valid relationship or attribute."""
        source_prim = MagicMock()
        target_prim = MagicMock()

        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False
        invalid_attr = MagicMock()
        invalid_attr.IsValid.return_value = False

        source_prim.GetRelationship.return_value = invalid_rel
        source_prim.GetAttribute.return_value = invalid_attr
        target_prim.GetRelationship.return_value = invalid_rel
        target_prim.GetAttribute.return_value = invalid_attr

        result = self.lib.can_connect(source_prim, "x", True, target_prim, "y", False)

        self.assertFalse(result)

    def test_can_connect_returns_false_for_output_to_output_attribute_pins(self):
        """can_connect rejects output→output when neither side is a relationship."""
        source_prim = MagicMock()
        target_prim = MagicMock()

        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False

        valid_attr = MagicMock()
        valid_attr.IsValid.return_value = True

        source_prim.GetRelationship.return_value = invalid_rel
        source_prim.GetAttribute.return_value = valid_attr
        target_prim.GetRelationship.return_value = invalid_rel
        target_prim.GetAttribute.return_value = valid_attr

        # Both are outputs — direction mismatch → False
        result = self.lib.can_connect(
            source_prim, "out", True, target_prim, "out", True
        )

        self.assertFalse(result)

    # ------------------------------------------------------------------ #
    # create_connection — exception / edge cases                          #
    # ------------------------------------------------------------------ #

    def test_create_connection_returns_false_when_exception_raised(self):
        """create_connection catches exceptions and returns False."""
        stage = MagicMock()
        source_prim = MagicMock()
        target_prim = MagicMock()

        # Source has no valid relationship, so target's GetRelationship raises first.
        source_prim.GetRelationship.return_value.IsValid.return_value = False
        target_prim.GetRelationship.side_effect = RuntimeError("boom")

        result = self.lib.create_connection(
            stage, source_prim, "out", target_prim, "targets"
        )

        self.assertFalse(result)

    def test_create_connection_falls_through_to_attribute_when_no_relationship(self):
        """create_connection uses attribute path when neither side has a relationship."""
        stage = MagicMock()

        invalid_rel = MagicMock()
        invalid_rel.IsValid.return_value = False
        valid_attr = MagicMock()
        valid_attr.IsValid.return_value = True
        valid_attr.GetPath.return_value = "/Source.outputs:value"

        source_prim = MagicMock()
        source_prim.GetRelationship.return_value = invalid_rel
        source_prim.GetAttribute.return_value = valid_attr

        target_valid_attr = MagicMock()
        target_valid_attr.IsValid.return_value = True
        target_valid_attr.GetPath.return_value = "/Target.inputs:value"
        target_prim = MagicMock()
        target_prim.GetRelationship.return_value = invalid_rel
        target_prim.GetAttribute.return_value = target_valid_attr

        result = self.lib.create_connection(
            stage, source_prim, "value", target_prim, "value"
        )

        self.assertTrue(result)
        target_valid_attr.AddConnection.assert_called_once_with("/Source.outputs:value")

    # ------------------------------------------------------------------ #
    # delete_connection — source-side relationship                        #
    # ------------------------------------------------------------------ #

    def test_delete_connection_removes_source_side_relationship_target(self):
        """delete_connection removes a target from a source-side output relationship."""
        stage = MagicMock()

        source_relationship = MagicMock()
        source_relationship.IsValid.return_value = True
        source_relationship.GetName.return_value = "affects"
        source_relationship.GetTargets.return_value = ["/Driven"]
        source_relationship.GetPath.return_value = "/Driver.affects"

        invalid_target_rel = MagicMock()
        invalid_target_rel.IsValid.return_value = False

        source_prim = MagicMock()
        source_prim.GetPath.return_value = "/Driver"
        source_prim.GetRelationship.return_value = source_relationship

        target_prim = MagicMock()
        target_prim.GetPath.return_value = "/Driven"
        target_prim.GetRelationship.return_value = invalid_target_rel

        result = self.lib.delete_connection(
            stage, source_prim, "affects", target_prim, ""
        )

        self.assertTrue(result)
        source_relationship.SetTargets.assert_called_once_with([])

    def test_delete_connection_returns_false_when_exception_raised(self):
        """delete_connection catches exceptions and returns False."""
        stage = MagicMock()
        source_prim = MagicMock()
        target_prim = MagicMock()

        target_prim.GetRelationship.side_effect = RuntimeError("boom")

        result = self.lib.delete_connection(
            stage, source_prim, "out", target_prim, "targets"
        )

        self.assertFalse(result)
