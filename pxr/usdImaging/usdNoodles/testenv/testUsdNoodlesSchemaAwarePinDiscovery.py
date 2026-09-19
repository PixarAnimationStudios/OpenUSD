#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
Tests for schema-aware pin discovery.

Verifies that hotbox-created prims (with only ``typeName`` set — no authored
attributes) still surface their built-in API-schema pins via
``prim.GetPrimDefinition()``, complementing the authored attribute set.
"""

from __future__ import annotations

import builtins
import gc
import os
import subprocess
import sys
import unittest
from unittest.mock import MagicMock, patch

from pxr import Gf, Sdf, Usd

try:
    from pxr.UsdNoodles._schema_pin_names import (
        get_api_schema_pin_groups,
        get_attribute_type_name,
        get_schema_aware_pin_entries,
        get_schema_aware_pin_names,
        get_schema_aware_pin_properties,
        get_schema_aware_property_names,
        populate_descriptor_pins,
        SchemaAwarePinProperty,
    )
    from pxr.UsdNoodles.models import NodeModel
    from pxr.UsdNoodles.nodeFactory import NodeFactory
    from pxr.UsdNoodles.nodeLibs.usdPrimLibrary import UsdPrimLibrary

    _has_module = True
except ImportError:
    _has_module = False


def _probe_real_stage() -> bool:
    """Check whether Usd.Stage.CreateInMemory() works without crashing.

    The noodles test binary can SIGSEGV when creating real USD stages
    (C++ static destructor ordering issue). A subprocess probe detects
    this so integration tests can be skipped.
    """
    try:
        result = subprocess.run(
            [sys.executable, "-c", "from pxr import Usd; Usd.Stage.CreateInMemory()"],
            timeout=30,
            capture_output=True,
            env={
                **os.environ,
                "PXR_PLUGINPATH_NAME": os.environ.get("PXR_PLUGINPATH_NAME", ""),
            },
        )
        return result.returncode == 0
    except Exception:
        return False


_can_create_stage: bool = _has_module and _probe_real_stage()


class _StageTestCase(unittest.TestCase):
    """Base class for tests that create real USD stages."""

    def tearDown(self) -> None:
        if hasattr(self, "stage"):
            self.stage = None
        gc.collect()


@unittest.skipUnless(_has_module, "pxr.UsdNoodles not available")
class SchemaAwareRelationshipSpecTest(unittest.TestCase):
    def test_schema_relationship_specs_are_surfaceable(self) -> None:
        prim_def = MagicMock()
        prim_def.GetPropertyNames.return_value = [
            "inputs:weight",
            "sources",
            "affects",
        ]
        prim_def.GetSchemaAttributeSpec.side_effect = (
            lambda name: object() if name == "inputs:weight" else None
        )
        prim_def.GetSchemaRelationshipSpec.side_effect = (
            lambda name: object() if name in {"sources", "affects"} else None
        )

        prim = MagicMock()
        prim.GetPrimDefinition.return_value = prim_def
        prim.GetAuthoredPropertyNames.return_value = []
        prim.GetAppliedSchemas.return_value = []
        invalid_relationship = MagicMock()
        invalid_relationship.IsValid.return_value = False
        prim.GetRelationship.return_value = invalid_relationship

        properties = get_schema_aware_pin_properties(prim)

        self.assertEqual(
            properties,
            [
                SchemaAwarePinProperty(
                    side="input",
                    pin_name="weight",
                    property_name="inputs:weight",
                    is_relationship=False,
                ),
                SchemaAwarePinProperty(
                    side="output",
                    pin_name="sources",
                    property_name="sources",
                    is_relationship=True,
                ),
                SchemaAwarePinProperty(
                    side="output",
                    pin_name="affects",
                    property_name="affects",
                    is_relationship=True,
                ),
            ],
        )

    def test_relationship_without_targets_is_surfaced(self) -> None:
        """usdNoodles does not filter properties: a relationship with no authored
        targets still becomes an output-side row (it is not skipped)."""
        prim_def = MagicMock()
        prim_def.GetPropertyNames.return_value = ["myRel"]
        prim_def.GetSchemaAttributeSpec.return_value = None
        prim_def.GetSchemaRelationshipSpec.side_effect = (
            lambda name: object() if name == "myRel" else None
        )
        prim = MagicMock()
        prim.GetPrimDefinition.return_value = prim_def
        prim.GetAuthoredPropertyNames.return_value = []
        prim.GetAppliedSchemas.return_value = []
        rel = MagicMock()
        rel.IsValid.return_value = True
        rel.HasAuthoredTargets.return_value = False
        rel.GetTargets.return_value = []
        prim.GetRelationship.return_value = rel

        self.assertEqual(
            get_schema_aware_pin_properties(prim),
            [
                SchemaAwarePinProperty(
                    side="output",
                    pin_name="myRel",
                    property_name="myRel",
                    is_relationship=True,
                ),
            ],
        )


# Expected LightAPI inputs that GeometryLight inherits via built-in apiSchemas.
_LIGHT_API_INPUT_PINS: frozenset[str] = frozenset(
    {
        "color",
        "colorTemperature",
        "diffuse",
        "enableColorTemperature",
        "exposure",
        "intensity",
        "normalize",
        "specular",
    }
)

# Grouped versions of the same pins, as returned by populate_descriptor_pins
# and NodeModel (which apply schema-group prefixing: bare → "Light:<bare>").
_LIGHT_API_GROUPED_INPUT_PINS: frozenset[str] = frozenset(
    f"Light:{p}" for p in _LIGHT_API_INPUT_PINS
)


def _expected_direction_group_display(
    prim: Usd.Prim, *, prefix: str
) -> tuple[list[str], list[int]]:
    display_pins: list[str] = []
    row_kinds: list[int] = []
    saw_header = False
    for attr_name in get_schema_aware_pin_names(prim):
        if attr_name.startswith(f"{prefix}:"):
            if not saw_header:
                display_pins.append(prefix)
                row_kinds.append(2)
                saw_header = True
            display_pins.append(attr_name[len(prefix) + 1 :])
            row_kinds.append(3)
            continue
        if prefix == "inputs" and ":" not in attr_name:
            display_pins.append(attr_name)
            row_kinds.append(0)
    return display_pins, row_kinds


def _assert_row_layout(
    case: unittest.TestCase,
    node: NodeModel,
    *,
    expected_input_slots: list[int],
    expected_output_slots: list[int],
    expected_display_row_kinds: list[int],
) -> None:
    case.assertEqual(list(node.inputRowSlots), expected_input_slots)
    case.assertEqual(list(node.outputRowSlots), expected_output_slots)
    case.assertEqual(list(node.displayRowKinds), expected_display_row_kinds)


def _expected_direction_group_row_layout(
    prim: Usd.Prim,
    *,
    expected_input_pins: list[str],
    expected_output_pins: list[str],
) -> tuple[list[int], list[int], list[int]]:
    input_slots = [-1] * len(expected_input_pins)
    output_slots = [-1] * len(expected_output_pins)
    display_row_kinds: list[int] = []

    input_pin_to_index = {pin: idx for idx, pin in enumerate(expected_input_pins)}
    output_pin_to_index = {pin: idx for idx, pin in enumerate(expected_output_pins)}
    saw_inputs_header = False
    saw_outputs_header = False

    for attr_name in get_schema_aware_pin_names(prim):
        if attr_name.startswith("inputs:"):
            if not saw_inputs_header:
                input_slots[input_pin_to_index["inputs"]] = len(display_row_kinds)
                display_row_kinds.append(2)
                saw_inputs_header = True
            pin_name = attr_name[7:]
            input_slots[input_pin_to_index[pin_name]] = len(display_row_kinds)
            display_row_kinds.append(3)
            continue

        if attr_name.startswith("outputs:"):
            if not saw_outputs_header:
                output_slots[output_pin_to_index["outputs"]] = len(display_row_kinds)
                display_row_kinds.append(2)
                saw_outputs_header = True
            pin_name = attr_name[8:]
            output_slots[output_pin_to_index[pin_name]] = len(display_row_kinds)
            display_row_kinds.append(3)
            continue

        if ":" not in attr_name:
            input_slots[input_pin_to_index[attr_name]] = len(display_row_kinds)
            display_row_kinds.append(0)

    return input_slots, output_slots, display_row_kinds


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class SchemaAwarePinNamesTest(_StageTestCase):
    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()

    def test_geometry_light_has_light_api_pins_without_authoring(self) -> None:
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        names = set(get_schema_aware_pin_names(prim))
        inputs = {n[len("inputs:") :] for n in names if n.startswith("inputs:")}
        self.assertTrue(
            _LIGHT_API_INPUT_PINS.issubset(inputs),
            f"Missing LightAPI pins; got {sorted(inputs)}",
        )

    def test_authored_attribute_merged_without_duplication(self) -> None:
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        prim.CreateAttribute("inputs:color", Sdf.ValueTypeNames.Color3f).Set(
            Gf.Vec3f(0.5, 0.5, 0.5)
        )
        names = list(get_schema_aware_pin_names(prim))
        self.assertEqual(names.count("inputs:color"), 1, f"Duplicated in {names}")

    def test_unknown_type_returns_authored_only(self) -> None:
        prim = self.stage.DefinePrim("/X", "NonExistentMadeUpType")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        names = list(get_schema_aware_pin_names(prim))
        self.assertEqual(names, ["inputs:foo"])

    def test_typeless_prim_returns_authored_only(self) -> None:
        prim = self.stage.DefinePrim("/Untyped")
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)
        names = list(get_schema_aware_pin_names(prim))
        self.assertEqual(names, ["inputs:bar"])

    def test_authored_relationships_preserve_property_order(self) -> None:
        prim = self.stage.DefinePrim("/Constraint")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateRelationship("sources")
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(2.0)
        prim.CreateRelationship("affects")

        entries = get_schema_aware_pin_entries(prim)
        expected_entries: list[tuple[str, str]] = []
        for property_name in get_schema_aware_property_names(prim):
            if property_name.startswith("inputs:"):
                expected_entries.append(("input", property_name[7:]))
            elif property_name.startswith("outputs:"):
                expected_entries.append(("output", property_name[8:]))
            elif property_name == "sources":
                expected_entries.append(("output", property_name))
            elif property_name == "affects":
                expected_entries.append(("output", property_name))
            elif ":" not in property_name:
                expected_entries.append(("input", property_name))

        self.assertEqual(entries, expected_entries)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class AttributeTypeNameTest(_StageTestCase):
    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()

    def test_schema_only_attribute_type_from_spec(self) -> None:
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        # Unauthored schema attribute — falls back to SdfAttributeSpec type.
        t = get_attribute_type_name(prim, "inputs:color")
        self.assertEqual(t, "color3f")

    def test_authored_attribute_type_from_runtime(self) -> None:
        prim = self.stage.DefinePrim("/Node")
        prim.CreateAttribute("inputs:val", Sdf.ValueTypeNames.Float).Set(1.0)
        t = get_attribute_type_name(prim, "inputs:val")
        self.assertEqual(t, "float")

    def test_unknown_attribute_returns_empty(self) -> None:
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        self.assertEqual(get_attribute_type_name(prim, "inputs:nonexistent"), "")


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class NodeFactoryGenericDescriptorTest(_StageTestCase):
    """Tests the Python fallback _generic_descriptor_from_prim."""

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        self.factory = NodeFactory(node_libraries=[])  # empty -> always falls back

    def test_hotbox_geometry_light_yields_light_api_pins(self) -> None:
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        # pyre-ignore[16]: private accessor, intentional white-box test
        descriptor = self.factory._generic_descriptor_from_prim(prim)
        pins = set(descriptor["inputPins"])
        self.assertTrue(
            _LIGHT_API_GROUPED_INPUT_PINS.issubset(pins),
            f"Missing LightAPI pins; got {sorted(pins)}",
        )

    def test_unknown_type_custom_authored_inputs(self) -> None:
        prim = self.stage.DefinePrim("/X", "NonExistentMadeUpType")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("outputs:bar", Sdf.ValueTypeNames.Float)
        # pyre-ignore[16]
        descriptor = self.factory._generic_descriptor_from_prim(prim)
        self.assertIn("foo", descriptor["inputPins"])
        self.assertIn("bar", descriptor["outputPins"])

    def test_suppressed_prim_types_do_not_fall_back_to_generic_nodes(self) -> None:
        for prim_path, type_name in [
            ("/Exec", "ExecNode"),
            ("/RigGraph", "RigGraph"),
            ("/RigGraphContainer", "RigGraphContainer"),
            ("/RigGraphNode", "RigGraphNode"),
        ]:
            prim = self.stage.DefinePrim(prim_path, type_name)
            prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
            prim.CreateAttribute("outputs:bar", Sdf.ValueTypeNames.Float)
            self.assertIsNone(
                self.factory.create_node_from_prim(prim, self.stage),
                f"{type_name} should be filtered out of usdNoodles",
            )

    def test_node_factory_groups_inputs_outputs_namespaces(self) -> None:
        prim = self.stage.DefinePrim("/Node")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(3.0)
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)
        prim.CreateAttribute("outputs:left", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("outputs:right", Sdf.ValueTypeNames.Float)

        node = self.factory.create_node_from_prim(
            prim, self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)
        expected_input_pins, expected_input_row_kinds = (
            _expected_direction_group_display(prim, prefix="inputs")
        )
        expected_output_pins, expected_output_row_kinds = (
            _expected_direction_group_display(prim, prefix="outputs")
        )
        (
            expected_input_slots,
            expected_output_slots,
            expected_display_row_kinds,
        ) = _expected_direction_group_row_layout(
            prim,
            expected_input_pins=expected_input_pins,
            expected_output_pins=expected_output_pins,
        )
        self.assertEqual(node.inputPins, expected_input_pins)
        self.assertEqual(list(node.inputRowKinds), expected_input_row_kinds)
        self.assertEqual(node.outputPins, expected_output_pins)
        self.assertEqual(list(node.outputRowKinds), expected_output_row_kinds)
        _assert_row_layout(
            self,
            node,
            expected_input_slots=expected_input_slots,
            expected_output_slots=expected_output_slots,
            expected_display_row_kinds=expected_display_row_kinds,
        )

    def test_node_factory_preserves_interleaved_property_rows(self) -> None:
        prim = self.stage.DefinePrim("/InterleavedNode")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("outputs:left", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(3.0)
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)
        prim.CreateAttribute("outputs:right", Sdf.ValueTypeNames.Float)

        node = self.factory.create_node_from_prim(
            prim, self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)
        expected_input_pins, _expected_input_row_kinds = (
            _expected_direction_group_display(prim, prefix="inputs")
        )
        expected_output_pins, _expected_output_row_kinds = (
            _expected_direction_group_display(prim, prefix="outputs")
        )
        (
            expected_input_slots,
            expected_output_slots,
            expected_display_row_kinds,
        ) = _expected_direction_group_row_layout(
            prim,
            expected_input_pins=expected_input_pins,
            expected_output_pins=expected_output_pins,
        )
        self.assertEqual(node.inputPins, expected_input_pins)
        self.assertEqual(node.outputPins, expected_output_pins)
        _assert_row_layout(
            self,
            node,
            expected_input_slots=expected_input_slots,
            expected_output_slots=expected_output_slots,
            expected_display_row_kinds=expected_display_row_kinds,
        )

    def test_direction_namespace_header_is_foldable(self) -> None:
        prim = self.stage.DefinePrim("/FoldableNode")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(3.0)
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)

        node = self.factory.create_node_from_prim(
            prim, self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)

        node.toggle_fold("inputs", is_output=False)

        self.assertEqual(node.inputPins, ["inputs", "middle"])
        self.assertEqual(list(node.inputRowKinds), [1, 0])
        self.assertEqual(node.resolve_pin_name("foo"), "inputs")
        self.assertEqual(node.resolve_pin_name("bar"), "inputs")
        _assert_row_layout(
            self,
            node,
            expected_input_slots=[0, 1],
            expected_output_slots=[],
            expected_display_row_kinds=[1, 0],
        )

    def test_generic_descriptor_and_node_model_include_relationship_pins(self) -> None:
        prim = self.stage.DefinePrim("/Constraint")
        prim.CreateAttribute("inputs:weight", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateRelationship("sources")
        prim.CreateRelationship("affects")

        # pyre-ignore[16]
        descriptor = self.factory._generic_descriptor_from_prim(prim)
        self.assertEqual(descriptor["inputPins"], ["weight"])
        # USD returns authored property names in alphabetical order, so the
        # relationship output pin order is not guaranteed to match the order
        # in which relationships were created on the prim.
        self.assertCountEqual(descriptor["outputPins"], ["sources", "affects"])

        node = self.factory.create_node_from_prim(
            prim, self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)
        self.assertFalse(node.is_relationship_pin("sources", False))
        self.assertTrue(node.is_relationship_pin("sources", True))
        self.assertTrue(node.is_relationship_pin("affects", True))

    def test_relationship_pins_are_not_collapsed_into_dual_pins(self) -> None:
        prim = self.stage.DefinePrim("/Constraint")
        prim.CreateRelationship("sources")
        prim.CreateAttribute("outputs:sources", Sdf.ValueTypeNames.Float).Set(1.0)

        node = self.factory.create_node_from_prim(
            prim, self.stage, default_renderer=MagicMock()
        )

        self.assertIsNotNone(node)
        self.assertFalse(node.is_relationship_pin("sources", False))
        self.assertTrue(node.is_relationship_pin("sources", True))
        self.assertNotIn("sources", node._dual_pin_names)
        self.assertIn("sources", node.outputPins)
        self.assertNotIn("sources", node.inputPins)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class NodeModelPropertyGettersTest(_StageTestCase):
    """Verify NodeModel.inputPins/outputPins/inputPinTypes/outputPinTypes
    getters use schema-aware discovery for uncached prims."""

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()

    def test_input_pins_getter_on_geometry_light(self) -> None:
        prim_path = Sdf.Path("/Light")
        self.stage.DefinePrim(prim_path, "GeometryLight")
        # Pass a mock renderer and uiStyle="default" to satisfy NodeData's
        # C++ std::string field and avoid Qt-dependent renderer loading.
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        pins = set(node.inputPins)
        self.assertTrue(
            _LIGHT_API_GROUPED_INPUT_PINS.issubset(pins),
            f"Missing LightAPI pins in NodeModel.inputPins; got {sorted(pins)}",
        )

    def test_input_pin_types_getter_on_geometry_light(self) -> None:
        prim_path = Sdf.Path("/Light")
        self.stage.DefinePrim(prim_path, "GeometryLight")
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        types = node.inputPinTypes
        self.assertEqual(types.get("Light:color"), "color3f")
        self.assertEqual(types.get("Light:intensity"), "float")

    def test_getters_group_inputs_outputs_namespaces(self) -> None:
        prim_path = Sdf.Path("/GroupedNode")
        prim = self.stage.DefinePrim(prim_path)
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(3.0)
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)
        prim.CreateAttribute("outputs:left", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("outputs:right", Sdf.ValueTypeNames.Float)

        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        expected_input_pins, expected_input_row_kinds = (
            _expected_direction_group_display(prim, prefix="inputs")
        )
        expected_output_pins, expected_output_row_kinds = (
            _expected_direction_group_display(prim, prefix="outputs")
        )
        (
            expected_input_slots,
            expected_output_slots,
            expected_display_row_kinds,
        ) = _expected_direction_group_row_layout(
            prim,
            expected_input_pins=expected_input_pins,
            expected_output_pins=expected_output_pins,
        )
        self.assertEqual(node.inputPins, expected_input_pins)
        self.assertEqual(list(node.inputRowKinds), expected_input_row_kinds)
        self.assertEqual(node.outputPins, expected_output_pins)
        self.assertEqual(list(node.outputRowKinds), expected_output_row_kinds)
        _assert_row_layout(
            self,
            node,
            expected_input_slots=expected_input_slots,
            expected_output_slots=expected_output_slots,
            expected_display_row_kinds=expected_display_row_kinds,
        )

    def test_getters_preserve_interleaved_input_output_rows(self) -> None:
        prim_path = Sdf.Path("/InterleavedGroupedNode")
        prim = self.stage.DefinePrim(prim_path)
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("outputs:left", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("middle", Sdf.ValueTypeNames.Float).Set(3.0)
        prim.CreateAttribute("inputs:bar", Sdf.ValueTypeNames.Float).Set(2.0)
        prim.CreateAttribute("outputs:right", Sdf.ValueTypeNames.Float)

        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        expected_input_pins, _expected_input_row_kinds = (
            _expected_direction_group_display(prim, prefix="inputs")
        )
        expected_output_pins, _expected_output_row_kinds = (
            _expected_direction_group_display(prim, prefix="outputs")
        )
        (
            expected_input_slots,
            expected_output_slots,
            expected_display_row_kinds,
        ) = _expected_direction_group_row_layout(
            prim,
            expected_input_pins=expected_input_pins,
            expected_output_pins=expected_output_pins,
        )
        self.assertEqual(node.inputPins, expected_input_pins)
        self.assertEqual(node.outputPins, expected_output_pins)
        _assert_row_layout(
            self,
            node,
            expected_input_slots=expected_input_slots,
            expected_output_slots=expected_output_slots,
            expected_display_row_kinds=expected_display_row_kinds,
        )


@unittest.skipUnless(_has_module, "pxr.UsdNoodles not available")
class NodeModelManualRowLayoutTest(unittest.TestCase):
    def test_manual_node_model_preserves_interleaved_rows(self) -> None:
        node = NodeModel(renderer=MagicMock(), uiStyle="default")
        node.set_ordered_pin_entries(
            [
                ("input", "foo"),
                ("output", "left"),
                ("input", "middle"),
                ("input", "bar"),
                ("output", "right"),
            ]
        )
        node.set_direction_group_pins(
            input_pins={"foo", "bar"},
            output_pins={"left", "right"},
        )
        node.inputPinTypes = {}
        node.outputPinTypes = {}
        node.inputPins = ["foo", "middle", "bar"]
        node.outputPins = ["left", "right"]

        self.assertEqual(node.inputPins, ["inputs", "foo", "middle", "bar"])
        self.assertEqual(node.outputPins, ["outputs", "left", "right"])
        _assert_row_layout(
            self,
            node,
            expected_input_slots=[0, 1, 4, 5],
            expected_output_slots=[2, 3, 6],
            expected_display_row_kinds=[2, 3, 2, 3, 0, 3, 3],
        )


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class NamespacedPinGroupingTest(_StageTestCase):
    """Verify namespaced schema pins are discovered for typed prims."""

    def test_apply_shaping_api_changes_pin_set(self) -> None:
        """ShapingAPI.Apply() changes the pin set returned by the helper.

        UsdLuxShapingAPI is not auto-applied to GeometryLight in open-source
        USD — it's a single-apply API schema users opt into. Verifies both
        (a) that shaping:* pins are absent before Apply, and (b) that they
        appear after Apply — proving the helper reads the per-prim composed
        definition, not just the static type definition.
        """
        from pxr import UsdLux

        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/Light", "GeometryLight")

        names_before = set(get_schema_aware_pin_names(prim))
        shaping_before = [n for n in names_before if n.startswith("inputs:shaping:")]
        self.assertEqual(
            shaping_before,
            [],
            f"Unexpected shaping pins before Apply; got {shaping_before}",
        )

        UsdLux.ShapingAPI.Apply(prim)

        names_after = set(get_schema_aware_pin_names(prim))
        shaping_after = [n for n in names_after if n.startswith("inputs:shaping:")]
        self.assertTrue(
            shaping_after,
            f"ShapingAPI.Apply() did not produce shaping pins via helper; names={sorted(names_after)}",
        )
        self.assertGreater(len(names_after), len(names_before))


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class UsdPrimLibraryCreateNodeTest(_StageTestCase):
    """Verify ``UsdPrimLibrary.create_node`` creates the correct prim type.

    Guards against regression of the Sdr-vs-schema precedence bug: UsdLux
    light types are registered both in Sdr and as concrete schemas in
    open-source USD. The library must create a typed prim, not a Shader
    prim with info:id=<identifier>.
    """

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        self.lib = UsdPrimLibrary()

    def test_geometry_light_identifier_creates_typed_prim_not_shader(self) -> None:
        parent = Sdf.Path("/")
        prim_path = self.lib.create_node(
            self.stage, parent, "GeometryLight", Gf.Vec2d(0, 0)
        )
        self.assertIsNotNone(prim_path, "create_node returned None")
        prim = self.stage.GetPrimAtPath(prim_path)
        self.assertTrue(prim.IsValid())
        self.assertEqual(
            prim.GetTypeName(),
            "GeometryLight",
            f"expected typed GeometryLight, got {prim.GetTypeName()!r} "
            f"(indicates Sdr-over-schema precedence regression)",
        )

    def test_unknown_identifier_returns_none_or_generic(self) -> None:
        parent = Sdf.Path("/")
        # Unknown identifier — not in schema registry, not in Sdr.
        prim_path = self.lib.create_node(
            self.stage, parent, "NonExistentMadeUpType", Gf.Vec2d(0, 0)
        )
        # Acceptable outcomes: None (create failed gracefully) or a prim
        # whose typeName matches the (unknown) identifier. Either is OK;
        # what MUST NOT happen is creating a Shader with info:id.
        if prim_path is not None:
            prim = self.stage.GetPrimAtPath(prim_path)
            self.assertNotEqual(
                prim.GetTypeName(),
                "Shader",
                "create_node created a Shader for an unknown identifier "
                "(should fall through to typed-prim attempt)",
            )


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class MeshPinTest(_StageTestCase):
    """Regression: UsdGeom.Mesh has no inputs:/outputs: schema properties.

    Mesh has bare schema attributes (points, extent, visibility, …) that
    surface as dual pins, but no inputs:/outputs: namespaced properties.
    """

    def test_mesh_has_no_inputs_outputs_pins_via_helper(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/M", "Mesh")
        names = get_schema_aware_pin_names(prim)
        input_pins = [n for n in names if n.startswith("inputs:")]
        output_pins = [n for n in names if n.startswith("outputs:")]
        self.assertEqual(input_pins, [])
        self.assertEqual(output_pins, [])

    def test_mesh_attrs_are_dual_pins_via_nodefactory(self) -> None:
        """Mesh has no in:/out: direction hints, so every schema ATTRIBUTE — bare
        (points, extent, …) and namespaced (e.g. primvars:displayColor) — is a
        dual pin (mirrored on both edges). Schema RELATIONSHIPS (proxyPrim) are
        surfaced as output-only rows: usdNoodles does not filter properties."""
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/M", "Mesh")
        factory = NodeFactory(node_libraries=[])
        # pyre-ignore[16]: private accessor, intentional white-box test
        descriptor = factory._generic_descriptor_from_prim(prim)
        self.assertIn("points", descriptor["inputPins"])
        # Every attribute is dual, so the input pins are a subset of the output
        # pins; the extra output pins are the output-only relationships.
        self.assertTrue(
            set(descriptor["inputPins"]).issubset(set(descriptor["outputPins"]))
        )
        # The proxyPrim relationship surfaces as an output-only row.
        self.assertIn("proxyPrim", descriptor["outputPins"])
        self.assertNotIn("proxyPrim", descriptor["inputPins"])
        # Namespaced schema attrs (primvars:*) are dual too, on both edges.
        self.assertTrue(
            any(":" in pin for pin in descriptor["inputPins"]),
            "Expected namespaced (dual) input pins such as primvars:*",
        )


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class WaddlerNamespacePinTest(_StageTestCase):
    """Generic namespace-aware pins for OpenExec-style rig prims.

    Exercises in:/out: direction hints alongside arbitrary namespaced
    attributes (parentIn:, rig1:) that are kept as input-side pins and grouped
    one level by their first namespace component.
    """

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        self.factory = NodeFactory(node_libraries=[])

    def _make_waddler_prim(self) -> Usd.Prim:
        prim = self.stage.DefinePrim("/Fk")
        prim.CreateAttribute("in:tx", Sdf.ValueTypeNames.Float).Set(0.0)
        prim.CreateAttribute("out:space", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("parentIn:space", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("rig1:space", Sdf.ValueTypeNames.Float)
        return prim

    def test_pin_entries_classify_hints_and_namespaces(self) -> None:
        entries = get_schema_aware_pin_entries(self._make_waddler_prim())
        self.assertIn(("input", "tx"), entries)  # in: -> input
        self.assertIn(("output", "space"), entries)  # out: -> output
        self.assertIn(("input", "parentIn:space"), entries)  # kept-name namespace
        self.assertIn(("input", "rig1:space"), entries)  # kept-name namespace

    def test_generic_descriptor_sides(self) -> None:
        prim = self._make_waddler_prim()
        # pyre-ignore[16]: private accessor, intentional white-box test
        descriptor = self.factory._generic_descriptor_from_prim(prim)
        # in:tx -> input-only "tx"; out:space -> output-only "space"; the
        # non-hint namespaced attrs (parentIn:space, rig1:space) are dual, so
        # they appear on BOTH sides.
        self.assertCountEqual(
            descriptor["inputPins"], ["tx", "parentIn:space", "rig1:space"]
        )
        self.assertCountEqual(
            descriptor["outputPins"], ["space", "parentIn:space", "rig1:space"]
        )

    def test_one_level_namespace_grouping(self) -> None:
        node = self.factory.create_node_from_prim(
            self._make_waddler_prim(), self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)
        input_kinds = dict(zip(node.inputPins, node.inputRowKinds))
        # Direction hint folds under the "inputs" header; arbitrary namespaces
        # fold under their own first-component headers (row kind 2 = unfolded
        # header, 3 = child).
        self.assertEqual(input_kinds.get("inputs"), 2)
        self.assertEqual(input_kinds.get("tx"), 3)
        self.assertEqual(input_kinds.get("parentIn"), 2)
        self.assertEqual(input_kinds.get("parentIn:space"), 3)
        self.assertEqual(input_kinds.get("rig1"), 2)
        self.assertEqual(input_kinds.get("rig1:space"), 3)
        # Every attribute row is mirror-dual: it keeps a single slot on its
        # primary side and mirrors a port onto the opposite edge. The namespaced
        # pins (parentIn:space, rig1:space) and the in:tx hint are input-side rows
        # that mirror an OUTPUT port (_dual_pin_names); the out:space hint is an
        # output-side row that mirrors an INPUT port (_output_dual_pin_names). So
        # the only real output rows are the "outputs" direction group + out:space.
        self.assertEqual(node.outputPins, ["outputs", "space"])
        self.assertEqual(list(node.outputRowKinds), [2, 3])
        self.assertEqual(node._dual_pin_names, {"tx", "parentIn:space", "rig1:space"})
        self.assertEqual(node._output_dual_pin_names, {"space"})

    def test_namespace_header_is_foldable(self) -> None:
        node = self.factory.create_node_from_prim(
            self._make_waddler_prim(), self.stage, default_renderer=MagicMock()
        )
        self.assertIsNotNone(node)
        node.toggle_fold("rig1", is_output=False)
        self.assertIn("rig1", node.inputPins)
        self.assertNotIn("rig1:space", node.inputPins)

    def test_hide_ui_toggle_controls_ui_namespace(self) -> None:
        prim = self.stage.DefinePrim("/UiNode")
        prim.CreateAttribute("ui:custom:flag", Sdf.ValueTypeNames.Bool).Set(True)
        prim.CreateAttribute("in:tx", Sdf.ValueTypeNames.Float).Set(0.0)

        hidden = get_schema_aware_pin_entries(prim, hide_ui=True)
        self.assertIn(("input", "tx"), hidden)
        self.assertNotIn(("input", "ui:custom:flag"), hidden)

        shown = get_schema_aware_pin_entries(prim, hide_ui=False)
        self.assertIn(("input", "ui:custom:flag"), shown)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class NodeModelNamespacedDualGettersTest(_StageTestCase):
    """Namespaced (non-hint) attributes surface as mirror-dual input-side rows
    via the lazy ``NodeModel(stage=, primPath=)`` property getters.

    This is the direct-construction path (used e.g. for dangling-link
    expansion), parallel to the ``NodeFactory`` descriptor path that
    ``WaddlerNamespacePinTest`` covers. Both paths must agree: a prim opened by
    direct NodeModel construction shows its namespaced pins
    (``parentIn:space``, ``rig1:space``) as input-side rows recorded in
    ``_dual_pin_names`` so they mirror an OUTPUT port onto the right edge — not
    as real, separate output rows.
    """

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()

    def _make_waddler_node(self) -> NodeModel:
        prim_path = Sdf.Path("/Fk")
        prim = self.stage.DefinePrim(prim_path)
        prim.CreateAttribute("in:tx", Sdf.ValueTypeNames.Float).Set(0.0)
        prim.CreateAttribute("out:space", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("parentIn:space", Sdf.ValueTypeNames.Float)
        prim.CreateAttribute("rig1:space", Sdf.ValueTypeNames.Float)
        return NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )

    def test_namespaced_dual_pins_mirror_on_input_side_via_getters(self) -> None:
        node = self._make_waddler_node()
        # in:tx -> input row "tx" (folds under the "inputs" header);
        # out:space -> output row "space" (folds under "outputs");
        # parentIn:space / rig1:space carry no direction hint: they are input-side
        # rows that mirror an output port onto the right edge (row kind 2 =
        # unfolded header, 3 = child).
        input_kinds = dict(zip(node.inputPins, node.inputRowKinds))
        self.assertEqual(input_kinds.get("inputs"), 2)
        self.assertEqual(input_kinds.get("tx"), 3)
        self.assertEqual(input_kinds.get("parentIn"), 2)
        self.assertEqual(input_kinds.get("parentIn:space"), 3)
        self.assertEqual(input_kinds.get("rig1"), 2)
        self.assertEqual(input_kinds.get("rig1:space"), 3)
        # Namespaced pins are NOT real output rows; only the out:space hint is.
        self.assertEqual(node.outputPins, ["outputs", "space"])
        self.assertEqual(list(node.outputRowKinds), [2, 3])

    def test_namespaced_dual_marked_mirror_dual_via_getters(self) -> None:
        node = self._make_waddler_node()
        _ = node.inputPins
        # Namespaced (non-hint) attributes are mirror-dual: a single input-side
        # row whose port is mirrored onto the right edge. They are recorded in
        # _dual_pin_names so the render/interaction layer draws + grabs that
        # mirrored output port.
        self.assertIn("rig1:space", node._dual_pin_names)
        self.assertIn("parentIn:space", node._dual_pin_names)
        # The out:space hint is the symmetric case: an output-side row mirrored
        # to the left edge, recorded in _output_dual_pin_names.
        _ = node.outputPins
        self.assertIn("space", node._output_dual_pin_names)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class RelationshipFilteringTest(_StageTestCase):
    """``get_schema_aware_pin_names`` is attribute-only (relationships never
    become *attribute* pins), but relationships ARE surfaced as their own rows
    by ``get_schema_aware_pin_properties`` / ``populate_descriptor_pins`` --
    usdNoodles does not filter properties by connection state."""

    def test_relationships_excluded_from_pin_names(self) -> None:
        # get_schema_aware_pin_names enumerates ATTRIBUTE pins only.
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/M", "Mesh")
        names = get_schema_aware_pin_names(prim)
        self.assertNotIn("proxyPrim", names)

    def test_relationships_surfaced_in_descriptor(self) -> None:
        # No property filtering: the proxyPrim relationship surfaces as an
        # output-side row even with no authored targets.
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/M", "Mesh")
        descriptor = {
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
        }
        populate_descriptor_pins(prim, descriptor)
        self.assertIn("proxyPrim", descriptor["outputPins"])
        self.assertNotIn("proxyPrim", descriptor["inputPins"])


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class NodeModelOutputGettersTest(_StageTestCase):
    """Verify NodeModel.outputPins/outputPinTypes getters."""

    def setUp(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()

    def test_output_pins_getter_with_authored_outputs(self) -> None:
        prim_path = Sdf.Path("/Node")
        prim = self.stage.DefinePrim(prim_path, "Shader")
        prim.CreateAttribute("outputs:surface", Sdf.ValueTypeNames.Token)
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        self.assertIn("surface", node.outputPins)

    def test_output_pin_types_getter_with_authored_outputs(self) -> None:
        prim_path = Sdf.Path("/Node")
        prim = self.stage.DefinePrim(prim_path, "Shader")
        prim.CreateAttribute("outputs:surface", Sdf.ValueTypeNames.Token)
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        self.assertEqual(node.outputPinTypes.get("surface"), "token")

    def test_output_pins_excludes_bare_attributes(self) -> None:
        """Bare attributes are dual-pins rendered in inputPins only."""
        prim_path = Sdf.Path("/Node")
        prim = self.stage.DefinePrim(prim_path, "Shader")
        prim.CreateAttribute("bareAttr", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.CreateAttribute("outputs:real", Sdf.ValueTypeNames.Token)
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        self.assertIn("real", node.outputPins)
        self.assertNotIn("bareAttr", node.outputPins)
        self.assertIn("bareAttr", node.inputPins)


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class PopulateDescriptorPinsTest(_StageTestCase):
    """Tests for the shared populate_descriptor_pins helper."""

    def _make_descriptor(self) -> dict:
        return {
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
        }

    def test_geometry_light_populates_light_api_pins(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        descriptor = self._make_descriptor()
        populate_descriptor_pins(prim, descriptor)
        pins = set(descriptor["inputPins"])
        self.assertTrue(
            _LIGHT_API_GROUPED_INPUT_PINS.issubset(pins),
            f"Missing LightAPI pins; got {sorted(pins)}",
        )
        self.assertEqual(descriptor["inputPinTypes"].get("Light:color"), "color3f")

    def test_port_type_custom_data_takes_precedence(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/N")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.GetAttribute("inputs:foo").SetCustomDataByKey("portType", "CustomFloat")
        descriptor = self._make_descriptor()
        populate_descriptor_pins(prim, descriptor, check_port_type=True)
        self.assertEqual(descriptor["inputPinTypes"].get("foo"), "CustomFloat")

    def test_port_type_ignored_when_not_requested(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/N")
        prim.CreateAttribute("inputs:foo", Sdf.ValueTypeNames.Float).Set(1.0)
        prim.GetAttribute("inputs:foo").SetCustomDataByKey("portType", "CustomFloat")
        descriptor = self._make_descriptor()
        populate_descriptor_pins(prim, descriptor, check_port_type=False)
        self.assertEqual(descriptor["inputPinTypes"].get("foo"), "float")

    def test_bare_attribute_added_to_both_sides(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/N")
        prim.CreateAttribute("val", Sdf.ValueTypeNames.Float).Set(1.0)
        descriptor = self._make_descriptor()
        populate_descriptor_pins(prim, descriptor)
        self.assertIn("val", descriptor["inputPins"])
        self.assertIn("val", descriptor["outputPins"])
        self.assertEqual(descriptor["inputPinTypes"]["val"], "float")
        self.assertEqual(descriptor["outputPinTypes"]["val"], "float")

    def test_schema_grouping_evicts_preexisting_bare_names(self) -> None:
        """C++ GetDescriptorFromPrim adds bare names; populate_descriptor_pins
        must replace them with grouped names, not duplicate them."""
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        descriptor = self._make_descriptor()
        descriptor["inputPins"] = ["color", "intensity"]
        descriptor["inputPinTypes"] = {"color": "color3f", "intensity": "float"}
        populate_descriptor_pins(prim, descriptor)
        bare_light_pins = [
            p for p in descriptor["inputPins"] if p in _LIGHT_API_INPUT_PINS
        ]
        self.assertEqual(
            bare_light_pins,
            [],
            f"Bare LightAPI pin names should be replaced by grouped names; "
            f"found {bare_light_pins}",
        )


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class SchemaGroupAliasMapTest(_StageTestCase):
    """Tests for _get_schema_pin_groups, _build_alias_map, and resolve_pin_name."""

    def _make_light_node(self) -> NodeModel:
        self.stage = Usd.Stage.CreateInMemory()
        prim_path = Sdf.Path("/Light")
        self.stage.DefinePrim(prim_path, "GeometryLight")
        return NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )

    def test_get_schema_pin_groups_returns_light_api_groups(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/Light", "GeometryLight")
        groups = get_api_schema_pin_groups(prim)
        self.assertIn("color", groups)
        self.assertEqual(groups["color"], "Light")
        self.assertIn("intensity", groups)
        self.assertEqual(groups["intensity"], "Light")

    def test_get_schema_pin_groups_empty_for_mesh(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/M", "Mesh")
        groups = get_api_schema_pin_groups(prim)
        self.assertEqual(groups, {})

    def test_build_alias_map_maps_bare_to_prefixed(self) -> None:
        node = self._make_light_node()
        _ = node.inputPins
        pins = node._input_pins
        grouped_pins = [p for p in pins if p.startswith("Light:")]
        self.assertTrue(grouped_pins, f"No grouped pins found in {pins}")
        alias_map = node._build_alias_map(pins)
        for bare, prefixed in alias_map.items():
            self.assertTrue(
                prefixed.startswith("Light:"),
                f"Alias {bare!r} -> {prefixed!r} missing Light: prefix",
            )
            self.assertIn(prefixed, pins)

    def test_build_alias_map_empty_for_mesh(self) -> None:
        self.stage = Usd.Stage.CreateInMemory()
        prim_path = Sdf.Path("/M")
        self.stage.DefinePrim(prim_path, "Mesh")
        node = NodeModel(
            stage=self.stage,
            primPath=prim_path,
            renderer=MagicMock(),
            uiStyle="default",
        )
        _ = node.inputPins
        alias_map = node._build_alias_map(node._input_pins)
        self.assertEqual(alias_map, {})

    def test_resolve_pin_name_translates_bare_to_grouped(self) -> None:
        node = self._make_light_node()
        _ = node.inputPins
        node._input_pin_alias_map = node._build_alias_map(node._input_pins)
        resolved = node.resolve_pin_name("color")
        self.assertEqual(
            resolved,
            "Light:color",
            f"resolve_pin_name('color') should return 'Light:color', got {resolved!r}",
        )

    def test_resolve_pin_name_passes_through_unknown(self) -> None:
        node = self._make_light_node()
        _ = node.inputPins
        node._input_pin_alias_map = node._build_alias_map(node._input_pins)
        resolved = node.resolve_pin_name("nonexistent")
        self.assertEqual(resolved, "nonexistent")

    def test_resolve_pin_name_passes_through_already_prefixed(self) -> None:
        node = self._make_light_node()
        _ = node.inputPins
        node._input_pin_alias_map = node._build_alias_map(node._input_pins)
        resolved = node.resolve_pin_name("Light:color")
        self.assertEqual(resolved, "Light:color")


@unittest.skipUnless(_has_module, "pxr.UsdNoodles not available")
class ApplyAutoApiSchemasImportErrorTest(unittest.TestCase):
    """Verifies that ``_apply_auto_api_schemas`` returns gracefully when the
    optional ``pxr.UsdLux`` module is unavailable (ImportError guard at
    usdPrimLibrary.py lines 224-227).
    """

    def test_returns_silently_when_usdlux_import_fails(self) -> None:
        # Simulate a prim that has LightAPI applied so we get past the early
        # return on line 222 and reach the ``from pxr import UsdLux`` import.
        mock_prim = MagicMock()
        mock_prim.GetAppliedSchemas.return_value = ["LightAPI"]

        real_import = builtins.__import__

        def fake_import(
            name: str,
            globals_: object = None,
            locals_: object = None,
            fromlist: tuple[str, ...] = (),
            level: int = 0,
        ) -> object:
            if name == "pxr" and "UsdLux" in (fromlist or ()):
                raise ImportError("simulated missing pxr.UsdLux")
            return real_import(name, globals_, locals_, fromlist, level)

        with patch("builtins.__import__", side_effect=fake_import):
            # Must not raise — the guard should swallow the ImportError.
            result = UsdPrimLibrary._apply_auto_api_schemas(mock_prim)

        self.assertIsNone(result)
        # Schemas should NOT have been applied because UsdLux was unavailable.
        mock_prim.ApplyAPI.assert_not_called()

    def test_returns_silently_when_no_light_api_applied(self) -> None:
        # Sanity check: prims without LightAPI bail out before the import,
        # so an ImportError patch should never be triggered.
        mock_prim = MagicMock()
        mock_prim.GetAppliedSchemas.return_value = ["MaterialBindingAPI"]

        result = UsdPrimLibrary._apply_auto_api_schemas(mock_prim)

        self.assertIsNone(result)
        mock_prim.ApplyAPI.assert_not_called()


@unittest.skipUnless(
    _can_create_stage, "pxr.UsdNoodles not available or real USD stages crash"
)
class ConnectionEndpointPinSurfacingTest(_StageTestCase):
    """A producer whose output exists only as a connection target still gets a pin.

    Replicates the unregistered-schema case (e.g. the waddler ``Ir*`` rig types):
    the producer prim neither authors nor has a registered schema for
    ``out:space``, so USD never exposes it as an attribute -- it appears only as
    the *target* of the consumer's connection. The producer node must still
    surface a ``space`` output pin so the noodle has a real pin instead of
    attaching to the bare node edge.
    """

    def test_producer_gains_output_pin_for_unauthored_target(self) -> None:
        from pxr.UsdNoodles.pinUtils import collect_links_for_prim

        self.stage = Usd.Stage.CreateInMemory()
        producer = self.stage.DefinePrim("/Producer", "")
        consumer = self.stage.DefinePrim("/Consumer", "")
        space_attr = consumer.CreateAttribute("rig1:space", Sdf.ValueTypeNames.Matrix4d)
        space_attr.AddConnection(producer.GetPath().AppendProperty("out:space"))

        # The producer authored no out:space (and has no schema), so it has no
        # "space" output row to begin with.
        producer_node = NodeModel(stage=self.stage, primPath=producer.GetPath())
        self.assertNotIn("space", producer_node.outputPins)

        # The link, collected from the consumer where the connection is
        # authored, names the producer's "space" output as its source.
        links = collect_links_for_prim(consumer)
        producer_outputs = [
            link["sourcePinName"]
            for link in links
            if link["sourceNodeId"] == str(producer.GetPath())
        ]
        self.assertIn("space", producer_outputs)

        # Surfacing the endpoint adds a real, dual "space" output pin.
        self.assertTrue(
            producer_node.add_connection_endpoint_pins(output_names=producer_outputs)
        )
        self.assertIn("space", producer_node.outputPins)
        self.assertIn("space", producer_node._output_dual_pin_names)
