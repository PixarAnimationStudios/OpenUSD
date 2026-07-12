#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for dual-pin (bare attribute) support in usdNoodles.

Attributes without inputs:/outputs: namespace should appear as both input
and output pins and allow connection authoring for both directions.
"""

import unittest
from unittest.mock import MagicMock


def _mock_attr(
    name,
    is_custom=True,
    type_name="float",
    connections=None,
    port_type=None,
    has_authored_connections=False,
):
    """Create a mock USD attribute."""
    attr = MagicMock()
    attr.GetName.return_value = name
    attr.IsCustom.return_value = is_custom

    # Namespace: everything before the last ':'
    if ":" in name:
        ns = name.rsplit(":", 1)[0]
    else:
        ns = ""
    attr.GetNamespace.return_value = ns

    mock_type = MagicMock()
    mock_type.__str__ = lambda self: type_name
    mock_type.__bool__ = lambda self: bool(type_name)
    attr.GetTypeName.return_value = mock_type

    attr.GetCustomDataByKey.return_value = port_type
    attr.GetConnections.return_value = connections or []
    attr.HasAuthoredConnections.return_value = has_authored_connections
    attr.IsValid.return_value = True

    return attr


def _mock_target_path(path):
    target_path = MagicMock()
    target_path.IsPrimPath.return_value = True
    target_path.IsPrimPropertyPath.return_value = False
    target_path.__str__.return_value = path
    return target_path


def _mock_property_target_path(path, property_name, *, use_name_attr_only=False):
    if use_name_attr_only:

        class _NameOnlyPropertyTargetPath:
            def __init__(self, prim_path, prop_name):
                self._prim_path = prim_path
                self.name = prop_name

            def IsPrimPath(self):
                return False

            def IsPrimPropertyPath(self):
                return True

            def GetPrimPath(self):
                return self._prim_path

            def __str__(self):
                return f"{self._prim_path}.{self.name}"

        return _NameOnlyPropertyTargetPath(path, property_name)

    target_path = MagicMock()
    target_path.IsPrimPath.return_value = False
    target_path.IsPrimPropertyPath.return_value = True
    target_path.GetPrimPath.return_value = path
    target_path.GetNameToken.return_value = property_name
    return target_path


def _mock_relationship(name, targets=None):
    relationship = MagicMock()
    relationship.GetName.return_value = name
    relationship.GetTargets.return_value = targets or []
    relationship.IsValid.return_value = True
    return relationship


def _mock_prim(path="/Node", attrs=None, relationships=None):
    prim = MagicMock()
    prim.GetPath.return_value = path
    prim.GetAttributes.return_value = attrs or []
    prim.GetRelationships.return_value = relationships or []
    prim.GetPrimDefinition.return_value = None
    prim.GetAppliedSchemas.return_value = []
    prim.GetRelationship.return_value = MagicMock(IsValid=MagicMock(return_value=False))
    return prim


class TestClassifyAttribute(unittest.TestCase):
    """Tests for pinUtils.classify_attribute()."""

    def test_input_prefix(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("inputs:color")
        result = classify_attribute(attr)
        self.assertIsNotNone(result)
        pin_name, is_input, is_output, is_bare = result
        self.assertEqual(pin_name, "color")
        self.assertTrue(is_input)
        self.assertFalse(is_output)
        self.assertFalse(is_bare)

    def test_output_prefix(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("outputs:result")
        result = classify_attribute(attr)
        self.assertIsNotNone(result)
        pin_name, is_input, is_output, is_bare = result
        self.assertEqual(pin_name, "result")
        self.assertFalse(is_input)
        self.assertTrue(is_output)
        self.assertFalse(is_bare)

    def test_bare_custom_attribute(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("myValue", is_custom=True)
        result = classify_attribute(attr)
        self.assertIsNotNone(result)
        pin_name, is_input, is_output, is_bare = result
        self.assertEqual(pin_name, "myValue")
        self.assertTrue(is_input)
        self.assertTrue(is_output)
        self.assertTrue(is_bare)

    def test_bare_schema_attribute_is_dual_pin(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("points", is_custom=False)
        result = classify_attribute(attr)
        self.assertIsNotNone(result)
        pin_name, is_input, is_output, is_bare = result
        self.assertEqual(pin_name, "points")
        self.assertTrue(is_input)
        self.assertTrue(is_output)
        self.assertTrue(is_bare)

    def test_in_hint_is_input(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("in:tx"))
        self.assertEqual(result, ("tx", True, False, False))

    def test_input_hint_is_input(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("input:scale"))
        self.assertEqual(result, ("scale", True, False, False))

    def test_out_hint_is_output(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("out:space"))
        self.assertEqual(result, ("space", False, True, False))

    def test_output_hint_is_output(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("output:result"))
        self.assertEqual(result, ("result", False, True, False))

    def test_namespaced_attribute_is_dual_pin(self):
        # Any other namespaced attribute (no direction hint) is dual, like a
        # bare attribute: it keeps its full name so the UI nests it one level
        # under a foldable namespace header and renders a pin on both edges.
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("xformOp:translate", is_custom=True))
        self.assertEqual(result, ("xformOp:translate", True, True, True))

    def test_primvars_namespace_is_dual_pin(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("primvars:st", is_custom=True))
        self.assertEqual(result, ("primvars:st", True, True, True))

    def test_rig_namespace_is_dual_pin(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        result = classify_attribute(_mock_attr("rig1:space", is_custom=True))
        self.assertEqual(result, ("rig1:space", True, True, True))

    def test_ui_namespace_hidden_by_default(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("ui:nodegraph:node:pos", is_custom=True)
        self.assertIsNone(classify_attribute(attr))

    def test_ui_namespace_shown_when_hide_ui_false(self):
        from pxr.UsdNoodles.pinUtils import classify_attribute

        attr = _mock_attr("ui:nodegraph:node:pos", is_custom=True)
        result = classify_attribute(attr, hide_ui=False)
        # A shown ui: attr follows the generic namespaced rule: a dual pin.
        self.assertEqual(result, ("ui:nodegraph:node:pos", True, True, True))


class TestDeduplicateLinks(unittest.TestCase):
    """Tests for pinUtils.deduplicate_links()."""

    def test_no_duplicates(self):
        from pxr.UsdNoodles.pinUtils import deduplicate_links

        links = [
            {
                "sourceNodeId": "/A",
                "sourcePinName": "out",
                "targetNodeId": "/B",
                "targetPinName": "in",
            },
            {
                "sourceNodeId": "/C",
                "sourcePinName": "out",
                "targetNodeId": "/D",
                "targetPinName": "in",
            },
        ]
        result = deduplicate_links(links)
        self.assertEqual(len(result), 2)

    def test_removes_duplicates(self):
        from pxr.UsdNoodles.pinUtils import deduplicate_links

        link = {
            "sourceNodeId": "/A",
            "sourcePinName": "value",
            "targetNodeId": "/B",
            "targetPinName": "value",
        }
        links = [link, dict(link)]  # duplicate
        result = deduplicate_links(links)
        self.assertEqual(len(result), 1)

    def test_preserves_order(self):
        from pxr.UsdNoodles.pinUtils import deduplicate_links

        link1 = {
            "sourceNodeId": "/A",
            "sourcePinName": "x",
            "targetNodeId": "/B",
            "targetPinName": "y",
        }
        link2 = {
            "sourceNodeId": "/C",
            "sourcePinName": "x",
            "targetNodeId": "/D",
            "targetPinName": "y",
        }
        links = [link1, link2, dict(link1)]  # link1 duplicate at end
        result = deduplicate_links(links)
        self.assertEqual(len(result), 2)
        self.assertEqual(result[0]["sourceNodeId"], "/A")
        self.assertEqual(result[1]["sourceNodeId"], "/C")

    def test_keeps_relationship_links_with_different_property_targets(self):
        from pxr.UsdNoodles.pinUtils import deduplicate_links

        links = [
            {
                "sourceNodeId": "/Constraint",
                "sourcePinName": "affects",
                "targetNodeId": "/Driven",
                "targetPinName": "value",
                "sourcePropertyName": "affects",
                "targetPropertyName": "inputs:value",
            },
            {
                "sourceNodeId": "/Constraint",
                "sourcePinName": "affects",
                "targetNodeId": "/Driven",
                "targetPinName": "value",
                "sourcePropertyName": "affects",
                "targetPropertyName": "outputs:value",
            },
        ]

        self.assertEqual(len(deduplicate_links(links)), 2)


class TestInferBareLinkDirection(unittest.TestCase):
    """Tests for pinUtils.infer_bare_link_direction()."""

    def test_target_has_inputs_prefix(self):
        from pxr.UsdNoodles.pinUtils import infer_bare_link_direction

        # Target is inputs: -> bare attr is output side
        result = infer_bare_link_direction("inputs:foo")
        self.assertFalse(result)

    def test_target_has_outputs_prefix(self):
        from pxr.UsdNoodles.pinUtils import infer_bare_link_direction

        # Target is outputs: -> bare attr is input side
        result = infer_bare_link_direction("outputs:foo")
        self.assertTrue(result)

    def test_bare_to_bare_defaults_to_input(self):
        from pxr.UsdNoodles.pinUtils import infer_bare_link_direction

        # bare-to-bare -> default UsdShade convention (input/target side)
        result = infer_bare_link_direction("value")
        self.assertTrue(result)

    def test_target_has_in_hint(self):
        from pxr.UsdNoodles.pinUtils import infer_bare_link_direction

        # Target is in: -> bare attr is output side
        self.assertFalse(infer_bare_link_direction("in:foo"))

    def test_target_has_out_hint(self):
        from pxr.UsdNoodles.pinUtils import infer_bare_link_direction

        # Target is out: -> bare attr is input side
        self.assertTrue(infer_bare_link_direction("out:foo"))


class TestDirectionHintHelpers(unittest.TestCase):
    """Tests for pinUtils direction-hint helpers."""

    def test_split_direction_hint_inputs(self):
        from pxr.UsdNoodles.pinUtils import split_direction_hint

        self.assertEqual(split_direction_hint("inputs:foo"), ("input", "foo"))
        self.assertEqual(split_direction_hint("input:foo"), ("input", "foo"))
        self.assertEqual(split_direction_hint("in:foo"), ("input", "foo"))

    def test_split_direction_hint_outputs(self):
        from pxr.UsdNoodles.pinUtils import split_direction_hint

        self.assertEqual(split_direction_hint("outputs:bar"), ("output", "bar"))
        self.assertEqual(split_direction_hint("output:bar"), ("output", "bar"))
        self.assertEqual(split_direction_hint("out:bar"), ("output", "bar"))

    def test_split_direction_hint_none_for_other_namespaces(self):
        from pxr.UsdNoodles.pinUtils import split_direction_hint

        self.assertEqual(split_direction_hint("rig1:space"), (None, "rig1:space"))
        self.assertEqual(split_direction_hint("bare"), (None, "bare"))

    def test_split_direction_hint_requires_exact_namespace_token(self):
        # "internal:" must NOT match "in:"; the trailing ':' makes each hint an
        # exact namespace token rather than a bare string prefix.
        from pxr.UsdNoodles.pinUtils import split_direction_hint

        self.assertEqual(split_direction_hint("internal:foo"), (None, "internal:foo"))
        self.assertEqual(split_direction_hint("outer:x"), (None, "outer:x"))

    def test_is_hidden_namespace_pin(self):
        from pxr.UsdNoodles.pinUtils import is_hidden_namespace_pin

        self.assertTrue(is_hidden_namespace_pin("ui:foo", hide_ui=True))
        self.assertFalse(is_hidden_namespace_pin("ui:foo", hide_ui=False))
        self.assertFalse(is_hidden_namespace_pin("rig1:space", hide_ui=True))

    def test_direction_hint_candidate_names_input(self):
        from pxr.UsdNoodles.pinUtils import direction_hint_candidate_names

        self.assertEqual(
            direction_hint_candidate_names("tx", is_input=True),
            ["inputs:tx", "input:tx", "in:tx", "tx"],
        )

    def test_direction_hint_candidate_names_output(self):
        from pxr.UsdNoodles.pinUtils import direction_hint_candidate_names

        self.assertEqual(
            direction_hint_candidate_names("space", is_input=False),
            ["outputs:space", "output:space", "out:space", "space"],
        )

    def test_direction_hint_candidate_names_namespaced_is_literal_only(self):
        # A name that already contains a ':' (kept-name namespaced pin) only
        # resolves to its literal form; prefixed variants would be nonsensical.
        from pxr.UsdNoodles.pinUtils import direction_hint_candidate_names

        self.assertEqual(
            direction_hint_candidate_names("rig1:space", is_input=True),
            ["rig1:space"],
        )
        self.assertEqual(
            direction_hint_candidate_names("rig1:space", is_input=False),
            ["rig1:space"],
        )


class TestAttributeLinks(unittest.TestCase):
    def test_collect_links_for_prim_surfaces_output_attribute_property_metadata(self):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        prim = _mock_prim(
            path="/Driver",
            attrs=[
                _mock_attr(
                    "outputs:worldMatrix",
                    connections=[
                        _mock_property_target_path("/Driven", "inputs:space:translate")
                    ],
                )
            ],
        )

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/Driver",
                    "sourcePinName": "worldMatrix",
                    "targetNodeId": "/Driven",
                    "targetPinName": "space:translate",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Driver",
                    "propertyName": "outputs:worldMatrix",
                    "is_relationship_link": False,
                    "sourcePropertyName": "outputs:worldMatrix",
                    "targetPropertyName": "inputs:space:translate",
                }
            ],
        )

    def test_collect_links_for_prim_surfaces_input_attribute_property_metadata(self):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        prim = _mock_prim(
            path="/Driven",
            attrs=[
                _mock_attr(
                    "inputs:space:translate",
                    connections=[
                        _mock_property_target_path("/Driver", "outputs:worldMatrix")
                    ],
                )
            ],
        )

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/Driver",
                    "sourcePinName": "worldMatrix",
                    "targetNodeId": "/Driven",
                    "targetPinName": "space:translate",
                    "is_input_link": True,
                    "propertyOwnerNodeId": "/Driven",
                    "propertyName": "inputs:space:translate",
                    "is_relationship_link": False,
                    "sourcePropertyName": "outputs:worldMatrix",
                    "targetPropertyName": "inputs:space:translate",
                }
            ],
        )


class TestClassifyRelationship(unittest.TestCase):
    def test_sources_is_output_pin(self):
        from pxr.UsdNoodles.pinUtils import classify_relationship

        result = classify_relationship(_mock_relationship("sources"))
        self.assertEqual(result, ("sources", False, True))

    def test_affects_is_output_pin(self):
        from pxr.UsdNoodles.pinUtils import classify_relationship

        result = classify_relationship(_mock_relationship("affects"))
        self.assertEqual(result, ("affects", False, True))

    def test_unknown_relationship_defaults_to_output_pin(self):
        # USD relationships are outgoing references by default; any
        # relationship not declared in the back-compat input/output sets
        # (e.g. material:binding, skel:skeleton) is treated as an
        # output-side pin so it renders on the right edge of the prim.
        from pxr.UsdNoodles.pinUtils import classify_relationship

        result = classify_relationship(_mock_relationship("material:binding"))
        self.assertEqual(result, ("material:binding", False, True))

        result = classify_relationship(_mock_relationship("targets"))
        self.assertEqual(result, ("targets", False, True))


class TestRelationshipLinks(unittest.TestCase):
    def test_collect_links_for_prim_includes_relationship_targets(self):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        prim = _mock_prim(
            path="/Constraint",
            relationships=[
                _mock_relationship("sources", [_mock_target_path("/Driver")]),
                _mock_relationship("affects", [_mock_target_path("/Driven")]),
            ],
        )

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
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
        )

    def test_collect_links_for_prim_includes_unknown_relationship_as_output(self):
        # material:binding is a USD relationship not enumerated in the
        # back-compat input/output sets, but it must still produce a
        # link so the row renders filled and supports dangling-target
        # navigation.
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        prim = _mock_prim(
            path="/Body",
            relationships=[
                _mock_relationship(
                    "material:binding", [_mock_target_path("/Materials/Body/Mat")]
                ),
            ],
        )

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/Body",
                    "sourcePinName": "material:binding",
                    "targetNodeId": "/Materials/Body/Mat",
                    "targetPinName": "",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Body",
                    "propertyName": "material:binding",
                    "is_relationship_link": True,
                    "sourcePropertyName": "material:binding",
                    "targetPropertyName": "",
                },
            ],
        )

    def test_collect_links_for_prim_surfaces_property_target_metadata(self):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        stage = MagicMock()
        target_prim = _mock_prim("/Driver")
        target_prim.GetRelationship.return_value = MagicMock(
            IsValid=MagicMock(return_value=False)
        )
        stage.GetPrimAtPath.return_value = target_prim

        prim = _mock_prim(
            path="/Constraint",
            relationships=[
                _mock_relationship(
                    "sources",
                    [_mock_property_target_path("/Driver", "outputs:worldMatrix")],
                ),
            ],
        )
        prim.GetStage.return_value = stage

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/Constraint",
                    "sourcePinName": "sources",
                    "targetNodeId": "/Driver",
                    "targetPinName": "worldMatrix",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/Constraint",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "outputs:worldMatrix",
                }
            ],
        )

    def test_collect_links_for_prim_reads_relationship_target_name_without_getnametoken(
        self,
    ):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        stage = MagicMock()
        target_prim = _mock_prim("/ConstraintB")
        target_prim.GetRelationship.return_value = MagicMock(
            IsValid=MagicMock(return_value=True)
        )
        stage.GetPrimAtPath.return_value = target_prim

        prim = _mock_prim(
            path="/ConstraintA",
            relationships=[
                _mock_relationship(
                    "sources",
                    [
                        _mock_property_target_path(
                            "/ConstraintB",
                            "affects",
                            use_name_attr_only=True,
                        )
                    ],
                ),
            ],
        )
        prim.GetStage.return_value = stage

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/ConstraintA",
                    "sourcePinName": "sources",
                    "targetNodeId": "/ConstraintB",
                    "targetPinName": "affects",
                    "is_input_link": False,
                    "propertyOwnerNodeId": "/ConstraintA",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "affects",
                }
            ],
        )

    def test_collect_links_for_prim_keeps_sources_to_sources_direction(self):
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        stage = MagicMock()
        target_prim = _mock_prim("/ConstraintSrc")
        target_prim.GetRelationship.return_value = MagicMock(
            IsValid=MagicMock(return_value=True)
        )
        stage.GetPrimAtPath.return_value = target_prim

        prim = _mock_prim(
            path="/ConstraintDst",
            relationships=[
                _mock_relationship(
                    "sources",
                    [
                        _mock_property_target_path(
                            "/ConstraintSrc",
                            "sources",
                        )
                    ],
                ),
            ],
        )
        prim.GetStage.return_value = stage

        result = collect_links_for_prim(prim)

        self.assertEqual(
            result,
            [
                {
                    "sourceNodeId": "/ConstraintSrc",
                    "sourcePinName": "sources",
                    "targetNodeId": "/ConstraintDst",
                    "targetPinName": "sources",
                    "is_input_link": True,
                    "propertyOwnerNodeId": "/ConstraintDst",
                    "propertyName": "sources",
                    "is_relationship_link": True,
                    "sourcePropertyName": "sources",
                    "targetPropertyName": "sources",
                }
            ],
        )

    def test_collect_links_for_prim_respects_cleared_targets_on_strongest_layer(
        self,
    ):
        # Regression: removing a relationship in the editor authors
        # `SetTargets([])` on the edit target.  Composition then yields an
        # empty target list, even if a weaker referenced/payload layer still
        # has the original targets.  The previous fallback walked
        # `GetPropertyStack` and "recovered" the weaker targets, so removing
        # the source/target prims from the editor and re-adding them caused
        # the deleted relationship to reappear with its old connection --
        # contradicting USD composition and what the user just authored.
        # `GetTargets` is the source of truth and an explicit clear is
        # honoured here just like USD itself sees it.
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        relationship = _mock_relationship("sources")
        relationship.GetTargets.return_value = []
        relationship.HasAuthoredTargets.return_value = True
        prim = _mock_prim(path="/Constraint", relationships=[relationship])
        stage = MagicMock()
        target_prim = MagicMock()
        target_prim.IsValid.return_value = False
        stage.GetPrimAtPath.return_value = target_prim
        prim.GetStage.return_value = stage

        result = collect_links_for_prim(prim)

        self.assertEqual(result, [])

    def test_get_relationship_link_metadata_marks_property_owner(self):
        from pxr.UsdNoodles.pinUtils import get_relationship_link_metadata

        metadata = get_relationship_link_metadata(
            "/Constraint",
            "affects",
            "/Driven",
            "affects",
        )

        self.assertEqual(metadata["propertyOwnerNodeId"], "/Constraint")
        self.assertEqual(metadata["propertyName"], "affects")
        self.assertTrue(metadata["is_relationship_link"])


class TestRelationshipGeometry(unittest.TestCase):
    def test_triangle_points_face_input_side_left(self):
        from pxr.UsdNoodles.pinUtils import triangle_points

        tip, base_top, _base_bottom = triangle_points(
            10.0,
            20.0,
            4.0,
            is_output=False,
        )

        self.assertLess(tip[0], base_top[0])

    def test_inset_triangle_points_stays_inside_triangle(self):
        from pxr.UsdNoodles.pinUtils import inset_triangle_points, triangle_points

        outer = triangle_points(10.0, 20.0, 6.0, is_output=True)
        inner = inset_triangle_points(outer, 0.25)

        outer_min_x = min(point[0] for point in outer)
        outer_max_x = max(point[0] for point in outer)
        inner_min_x = min(point[0] for point in inner)
        inner_max_x = max(point[0] for point in inner)
        self.assertGreater(inner_min_x, outer_min_x)
        self.assertLess(inner_max_x, outer_max_x)

    def test_arrowhead_points_point_toward_tip(self):
        from pxr.UsdNoodles.pinUtils import arrowhead_points

        tip, base_a, base_b = arrowhead_points(
            10.0,
            5.0,
            2.0,
            5.0,
            length=4.0,
            width=6.0,
        )

        self.assertGreater(tip[0], base_a[0])
        self.assertGreater(tip[0], base_b[0])
