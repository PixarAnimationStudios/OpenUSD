#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
Schema-aware pin-name discovery.

Reads ``prim.GetPrimDefinition()`` — the per-prim composed definition
including all built-in and runtime-applied API schemas — so pin discovery
works for hotbox-created prims that have only ``typeName`` set.
"""

from __future__ import annotations

from dataclasses import dataclass

from pxr import Tf, Usd

from .pinUtils import (
    is_hidden_namespace_pin,
    is_relationship_input_pin,
    is_relationship_output_pin,
    split_direction_hint,
)


@dataclass(frozen=True)
class SchemaAwarePinProperty:
    side: str
    pin_name: str
    property_name: str
    is_relationship: bool


def get_schema_aware_pin_names(prim: Usd.Prim) -> list[str]:
    """Return deduplicated attribute names ordered schema-first, authored-second."""
    seen: set[str] = set()
    ordered: list[str] = []

    prim_def = prim.GetPrimDefinition()
    if prim_def is not None:
        for name in prim_def.GetPropertyNames():
            name = str(name)
            # Skip relationships — only attributes become pins.
            if prim_def.GetSchemaAttributeSpec(name) is None:
                continue
            if name not in seen:
                seen.add(name)
                ordered.append(name)

    for attr in prim.GetAuthoredAttributes():
        name = attr.GetName()
        if name not in seen:
            seen.add(name)
            ordered.append(name)

    return ordered


def get_schema_aware_property_names(prim: Usd.Prim) -> list[str]:
    """Return deduplicated property names ordered schema-first, authored-second."""
    seen: set[str] = set()
    ordered: list[str] = []

    prim_def = prim.GetPrimDefinition()
    if prim_def is not None:
        for name in prim_def.GetPropertyNames():
            name = str(name)
            if (
                prim_def.GetSchemaAttributeSpec(name) is None
                and prim_def.GetSchemaRelationshipSpec(name) is None
            ):
                continue
            if name not in seen:
                seen.add(name)
                ordered.append(name)

    for name in prim.GetAuthoredPropertyNames():
        name = str(name)
        if name not in seen:
            seen.add(name)
            ordered.append(name)

    return ordered


def get_attribute_type_name(prim: Usd.Prim, attr_name: str) -> str:
    """Return the SdfValueTypeName string, or empty string."""
    attr = prim.GetAttribute(attr_name)
    if attr and attr.IsValid():
        t = attr.GetTypeName()
        if t:
            return str(t)

    prim_def = prim.GetPrimDefinition()
    if prim_def is None:
        return ""

    spec = prim_def.GetSchemaAttributeSpec(attr_name)
    if spec is None:
        return ""
    t = spec.typeName
    return str(t) if t else ""


def get_api_schema_pin_groups(prim: Usd.Prim) -> dict[str, str]:
    """Map bare pin names to their source Applied API schema for display grouping.

    Returns ``{bare_pin_name: schema_display_name}`` for ``inputs:`` /
    ``outputs:`` pins from applied API schemas that have no sub-namespace
    after stripping the prefix.  Pins that already contain a sub-namespace
    (like ``inputs:shaping:focus``) group naturally via the ``:`` separator
    and are excluded here.
    """
    pin_to_schema: dict[str, str] = {}

    try:
        applied = prim.GetAppliedSchemas()
    except (Tf.ErrorException, RuntimeError):
        return pin_to_schema
    if not applied:
        return pin_to_schema
    schema_reg = Usd.SchemaRegistry()

    for schema_name in applied:
        api_def = schema_reg.FindAppliedAPIPrimDefinition(str(schema_name))
        if api_def is None:
            continue
        display = str(schema_name)
        if display.endswith("API"):
            display = display[:-3]
        for prop_name in api_def.GetPropertyNames():
            if api_def.GetSchemaAttributeSpec(prop_name) is None:
                continue
            side, bare = split_direction_hint(str(prop_name))
            if side is None:
                continue
            if ":" not in bare and bare not in pin_to_schema:
                pin_to_schema[bare] = display

    return pin_to_schema


def _normalize_display_pin_name(
    attr_name: str,
    schema_groups: dict[str, str],
    *,
    hide_ui: bool = True,
) -> tuple[str, str] | None:
    side, stripped = split_direction_hint(attr_name)
    if side is not None:
        pin_name = stripped
        if ":" not in pin_name and pin_name in schema_groups:
            pin_name = f"{schema_groups[pin_name]}:{pin_name}"
        return (side, pin_name)
    if ":" not in attr_name:
        return ("input", attr_name)
    if is_hidden_namespace_pin(attr_name, hide_ui=hide_ui):
        return None
    # Any other namespaced attribute becomes an input-side pin that keeps its
    # full name, so namespace grouping nests it one level under a foldable header.
    return ("input", attr_name)


def _is_relationship_property(prim: Usd.Prim, property_name: str) -> bool:
    prim_def = prim.GetPrimDefinition()
    if prim_def is not None:
        if prim_def.GetSchemaAttributeSpec(property_name) is not None:
            return False
        if prim_def.GetSchemaRelationshipSpec(property_name) is not None:
            return True

    relationship = prim.GetRelationship(property_name)
    return bool(relationship and relationship.IsValid())


def get_schema_aware_pin_properties(
    prim: Usd.Prim, *, hide_ui: bool = True
) -> list[SchemaAwarePinProperty]:
    """Return normalized visible pin properties in composed USD property order.

    When a relationship and an attribute would normalize to the same display
    key (for example a ``sources`` relationship alongside an ``outputs:sources``
    attribute), the relationship takes precedence so the UI exposes the
    relationship pin rather than collapsing it into a dual attribute pin.
    """
    schema_groups = get_api_schema_pin_groups(prim)
    ordered_properties: list[SchemaAwarePinProperty] = []
    seen: dict[tuple[str, str], int] = {}

    for property_name in get_schema_aware_property_names(prim):
        if _is_relationship_property(prim, property_name):
            if is_relationship_output_pin(property_name):
                side = "output"
            elif is_relationship_input_pin(property_name):
                side = "input"
            else:
                # USD relationships are outgoing references by default, so
                # unknown relationships (e.g. material:binding, proxyPrim)
                # render as output-side pins on the right edge of the prim.
                # Every relationship is surfaced as a row regardless of whether
                # it has authored targets -- usdNoodles does not filter
                # properties (only ``ui:`` editor metadata is hidden).
                side = "output"
            pin_property = SchemaAwarePinProperty(
                side=side,
                pin_name=property_name,
                property_name=property_name,
                is_relationship=True,
            )
        else:
            entry = _normalize_display_pin_name(
                property_name, schema_groups, hide_ui=hide_ui
            )
            if entry is None:
                continue
            pin_property = SchemaAwarePinProperty(
                side=entry[0],
                pin_name=entry[1],
                property_name=property_name,
                is_relationship=False,
            )

        key = (pin_property.side, pin_property.pin_name)
        if key in seen:
            existing_index = seen[key]
            existing = ordered_properties[existing_index]
            # Promote a relationship pin over an attribute pin that maps to
            # the same display key so callers see the relationship metadata.
            if pin_property.is_relationship and not existing.is_relationship:
                ordered_properties[existing_index] = pin_property
            continue
        seen[key] = len(ordered_properties)
        ordered_properties.append(pin_property)

    return ordered_properties


def get_schema_aware_pin_entries(
    prim: Usd.Prim, *, hide_ui: bool = True
) -> list[tuple[str, str]]:
    """Return normalized visible pin entries in composed USD property order.

    Entries are ``("input" | "output", pin_name)`` pairs after schema display
    grouping is applied. Bare attributes are treated as input-side rows because
    usdNoodles renders them as dual pins from the single visible row.
    """
    return [
        (pin_property.side, pin_property.pin_name)
        for pin_property in get_schema_aware_pin_properties(prim, hide_ui=hide_ui)
    ]


def get_display_pin_name_for_property_name(
    prim: Usd.Prim,
    property_name: str,
    *,
    hide_ui: bool = True,
) -> str | None:
    """Return the normalized visible pin name for a USD property."""
    if _is_relationship_property(prim, property_name):
        # Every relationship is a row regardless of authored targets; only
        # ``ui:`` editor metadata is filtered (handled for attributes below).
        return property_name

    entry = _normalize_display_pin_name(
        property_name,
        get_api_schema_pin_groups(prim),
        hide_ui=hide_ui,
    )
    if entry is None:
        return None
    return entry[1]


def resolve_property_name_for_display_pin(
    prim: Usd.Prim,
    pin_name: str,
    side: str,
) -> str | None:
    """Resolve a visible pin name back to its exact USD property name."""
    for pin_property in get_schema_aware_pin_properties(prim):
        if pin_property.pin_name != pin_name or pin_property.side != side:
            continue
        return pin_property.property_name

    if ":" in pin_name:
        return None

    attr = prim.GetAttribute(pin_name)
    if attr and attr.IsValid():
        return pin_name
    return None


def _resolve_pin_type(
    prim: Usd.Prim,
    attr_name: str,
    check_port_type: bool,
) -> str:
    if check_port_type:
        attr = prim.GetAttribute(attr_name)
        if attr and attr.IsValid():
            port_type = attr.GetCustomDataByKey("portType")
            if port_type:
                return port_type
    return get_attribute_type_name(prim, attr_name)


def _add_pin(
    pin_name: str,
    type_str: str,
    pins: list[str],
    pin_set: set[str],
    pin_types: dict[str, str],
    schema_groups: dict[str, str],
) -> None:
    """Add a single pin to a direction list, applying schema grouping.

    ``pin_set`` mirrors ``pins`` for O(1) membership checks, avoiding
    quadratic behavior on prims with many pins.  ``pins`` is preserved as
    the public ordered list on the descriptor.
    """
    bare_alias = pin_name
    if ":" not in pin_name and pin_name in schema_groups:
        pin_name = f"{schema_groups[pin_name]}:{pin_name}"
    elif ":" in pin_name:
        prefix, bare = pin_name.split(":", 1)
        if schema_groups.get(bare) == prefix:
            bare_alias = bare

    if bare_alias != pin_name and bare_alias in pin_set:
        pins.remove(bare_alias)
        pin_set.discard(bare_alias)
        pin_types.pop(bare_alias, None)
    if pin_name not in pin_set:
        pins.append(pin_name)
        pin_set.add(pin_name)
    if type_str:
        pin_types[pin_name] = type_str


def populate_descriptor_pins(
    prim: Usd.Prim,
    descriptor: dict,
    *,
    check_port_type: bool = False,
    hide_ui: bool = True,
) -> None:
    """Populate descriptor input/output pins from schema + authored attributes."""
    schema_groups = get_api_schema_pin_groups(prim)
    input_pins: list[str] = descriptor["inputPins"]
    output_pins: list[str] = descriptor["outputPins"]
    # Parallel sets give O(1) membership lookups in _add_pin; without them,
    # `pin_name in pins` is O(n) and the loop becomes O(n^2) per prim.
    input_set: set[str] = set(input_pins)
    output_set: set[str] = set(output_pins)
    for pin_property in get_schema_aware_pin_properties(prim, hide_ui=hide_ui):
        property_name = pin_property.property_name
        type_str = (
            ""
            if pin_property.is_relationship
            else _resolve_pin_type(prim, property_name, check_port_type)
        )
        # Bare attributes and any non-hint namespaced attribute (e.g.
        # ``rig1:space``) are dual: a pin on both edges. Only explicit
        # direction-hint attributes (``in:``/``out:``/...) are single-sided.
        is_dual = (
            not pin_property.is_relationship
            and split_direction_hint(property_name)[0] is None
        )
        if is_dual:
            _add_pin(
                pin_property.pin_name,
                type_str,
                input_pins,
                input_set,
                descriptor["inputPinTypes"],
                schema_groups,
            )
            _add_pin(
                pin_property.pin_name,
                type_str,
                output_pins,
                output_set,
                descriptor["outputPinTypes"],
                schema_groups,
            )
        elif pin_property.side == "input":
            _add_pin(
                pin_property.pin_name,
                type_str,
                input_pins,
                input_set,
                descriptor["inputPinTypes"],
                schema_groups,
            )
        elif pin_property.side == "output":
            _add_pin(
                pin_property.pin_name,
                type_str,
                output_pins,
                output_set,
                descriptor["outputPinTypes"],
                schema_groups,
            )
