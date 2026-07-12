#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict
from __future__ import annotations

"""Utilities for classifying USD attributes as node pins and deduplicating links.

Centralizes the logic for determining whether a USD attribute should appear as
an input pin, output pin, or both (dual pin for non-namespaced attributes).
"""

import math
from typing import Any, cast


RELATIONSHIP_LINK_BASE_COLOR: tuple[float, float, float] = (1.0, 0.5, 0.5)
_RELATIONSHIP_INPUT_PINS: frozenset[str] = frozenset({"sources"})
# Relationship properties are displayed as outgoing authoring pins in the UI.
# Keep `sources` in the input set for backwards-compatible target-side edits.
_RELATIONSHIP_OUTPUT_PINS: frozenset[str] = frozenset({"sources", "affects"})
# Relationship authoring should preserve all authored targets for both
# constraint source/affect lists when users add links interactively.
_RELATIONSHIP_MULTI_TARGET_INPUT_PINS: frozenset[str] = frozenset(
    {"sources", "affects"}
)

# Direction-hint namespaces. The trailing ':' makes each prefix an exact
# namespace-token match: "internal:foo".startswith("in:") is False because the
# character after "in" is "t", not ":". The three spellings per side are
# mutually unambiguous for the same reason, so tuple order is irrelevant.
_INPUT_HINT_PREFIXES: tuple[str, ...] = ("inputs:", "input:", "in:")
_OUTPUT_HINT_PREFIXES: tuple[str, ...] = ("outputs:", "output:", "out:")

# Namespaces hidden from pins by default: editor-written UI metadata such as
# ``ui:nodegraph:node:pos`` / ``:icon`` / ``:expansionState`` (consumed
# specially by the editor, not data pins). Gated by a ``hide_ui`` flag.
_HIDDEN_PIN_NAMESPACES: tuple[str, ...] = ("ui:",)


def split_direction_hint(name: str) -> tuple[str | None, str]:
    """Split a property name into its direction hint and the remaining name.

    Returns ``("input" | "output" | None, stripped_name)``.  ``None`` means the
    name carries no direction hint (it is bare or uses some other namespace), in
    which case ``stripped_name`` is the original name unchanged.
    """
    for prefix in _INPUT_HINT_PREFIXES:
        if name.startswith(prefix):
            return ("input", name[len(prefix) :])
    for prefix in _OUTPUT_HINT_PREFIXES:
        if name.startswith(prefix):
            return ("output", name[len(prefix) :])
    return (None, name)


def is_hidden_namespace_pin(name: str, *, hide_ui: bool) -> bool:
    """True when ``name`` is in a namespace hidden from pins (when ``hide_ui``)."""
    return hide_ui and name.startswith(_HIDDEN_PIN_NAMESPACES)


def direction_hint_candidate_names(port_name: str, *, is_input: bool) -> list[str]:
    """Ordered USD attribute-name candidates for a pin's ``port_name`` on a side.

    Each direction-hint prefix for the side is applied (``inputs:``/``input:``/
    ``in:`` for inputs, ``outputs:``/``output:``/``out:`` for outputs), followed
    by the bare ``port_name`` itself. The bare form also resolves namespaced
    pins whose full name was kept (e.g. ``rig1:space``); for those the prefixed
    forms are skipped, since ``inputs:rig1:space`` is never a real attribute.
    """
    prefixes = _INPUT_HINT_PREFIXES if is_input else _OUTPUT_HINT_PREFIXES
    candidates: list[str] = []
    if ":" not in port_name:
        candidates.extend(f"{prefix}{port_name}" for prefix in prefixes)
    candidates.append(port_name)
    return candidates


def classify_attribute(
    attr: Any, *, hide_ui: bool = True
) -> tuple[str, bool, bool, bool] | None:
    """Classify a USD attribute for pin enumeration.

    Returns a tuple (pin_name, is_input, is_output, is_bare) or None if the
    attribute should not be a pin.

    Classification rules:
      - ``in:X`` / ``input:X`` / ``inputs:X``    -> input pin named ``X``
      - ``out:X`` / ``output:X`` / ``outputs:X`` -> output pin named ``X``
      - bare ``X`` (no namespace) -> dual pin named ``X`` (both input and
        output)
      - any other namespaced attribute (``rig1:space``, ``xformOp:translate``,
        ...) -> dual pin (both input and output) keeping its full namespaced
        name, so the UI nests it under a foldable namespace header and renders
        a pin on both edges
      - hidden namespaces (``ui:``) when ``hide_ui`` -> None (skip)

    NOTE: direction-hint pins (``in:``/``out:``/...) are single-sided here for
    link discovery and USD identity. They are nonetheless rendered with a port
    on BOTH edges (a mirrored affordance) via the node's
    ``_dual_pin_names`` / ``_output_dual_pin_names`` sets — see ``models.py`` and
    the graph-view render/interaction paths. This keeps the UsdShade
    ``inputs:``/``outputs:`` convention (distinct, same-name-coexisting pins)
    and authored attribute identity intact while still letting every row be
    grabbed from either edge.
    """
    attr_name: str = attr.GetName()

    side, stripped = split_direction_hint(attr_name)
    if side == "input":
        return (stripped, True, False, False)
    if side == "output":
        return (stripped, False, True, False)

    # Bare attribute: no namespace (no ':') — both input and output.
    if ":" not in attr_name:
        return (attr_name, True, True, True)

    if is_hidden_namespace_pin(attr_name, hide_ui=hide_ui):
        return None

    # Any other namespaced attribute is dual (both input and output) like a
    # bare attribute: it keeps its full name so namespace grouping nests it one
    # level under a foldable header, and its link direction is inferred
    # per-connection from the target (the is_bare=True flag drives inference).
    return (attr_name, True, True, True)


def classify_relationship(rel: Any) -> tuple[str, bool, bool] | None:
    """Classify a USD relationship for pin enumeration.

    USD relationships are outgoing references by default; the
    ``_RELATIONSHIP_INPUT_PINS`` set declares the small back-compat
    exception list of relationship names that author as input-side pins.
    """
    rel_name: str = rel.GetName()

    if rel_name in _RELATIONSHIP_OUTPUT_PINS:
        return (rel_name, False, True)

    if rel_name in _RELATIONSHIP_INPUT_PINS:
        return (rel_name, True, False)

    return (rel_name, False, True)


def is_relationship_input_pin(pin_name: str) -> bool:
    return pin_name in _RELATIONSHIP_INPUT_PINS


def is_relationship_output_pin(pin_name: str) -> bool:
    return pin_name in _RELATIONSHIP_OUTPUT_PINS


def is_relationship_pin(pin_name: str) -> bool:
    return is_relationship_input_pin(pin_name) or is_relationship_output_pin(pin_name)


def is_multi_target_relationship_input_pin(pin_name: str) -> bool:
    return pin_name in _RELATIONSHIP_MULTI_TARGET_INPUT_PINS


def deduplicate_links(links: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Remove duplicate link dicts, preserving insertion order.

    Two links are duplicates when they share the same
    (sourceNodeId, sourcePort, targetNodeId, targetPort) tuple.
    """
    seen: set[tuple[str, str, str, str, str, str]] = set()
    result: list[dict[str, Any]] = []
    for link in links:
        key = (
            link["sourceNodeId"],
            link["sourcePinName"],
            link["targetNodeId"],
            link["targetPinName"],
            str(link.get("sourcePropertyName", "")),
            str(link.get("targetPropertyName", "")),
        )
        if key not in seen:
            seen.add(key)
            result.append(link)
    return result


def infer_bare_link_direction(target_prop_name: str) -> bool:
    """Infer link direction when the source attribute is bare (no namespace).

    Returns ``True`` if the bare attribute should be treated as an input
    (target side of the connection), ``False`` if it should be treated as an
    output (source side).

    Canonical direction rule:
      - Target is an input pin (``in:``/``input:``/``inputs:``) -> bare attr is
        on the output side (False)
      - Target is an output pin or bare -> bare attr is on the input side
        (True). The bare-to-bare fallback follows the UsdShade convention.
    """
    return split_direction_hint(target_prop_name)[0] != "input"


def _strip_pin_prefix(prop_name: str) -> str:
    """Strip a direction-hint prefix (``in:``/``inputs:``/``out:``/...) if present."""
    return split_direction_hint(prop_name)[1]


def _get_path_property_name(path_obj: Any) -> str:
    get_name_token = getattr(path_obj, "GetNameToken", None)
    if callable(get_name_token):
        try:
            return str(get_name_token())
        except (AttributeError, RuntimeError, TypeError):
            pass

    get_name = getattr(path_obj, "GetName", None)
    if callable(get_name):
        try:
            return str(get_name())
        except (AttributeError, RuntimeError, TypeError):
            pass

    name_token = getattr(path_obj, "nameToken", None)
    if name_token is not None:
        return str(name_token)

    name = getattr(path_obj, "name", None)
    if name is not None:
        return str(name)

    path_str = str(path_obj)
    if "." in path_str:
        return path_str.rsplit(".", 1)[-1]
    return ""


def _get_relationship_authored_targets(rel: Any) -> list[Any]:
    """Return the composed target list, trusting USD composition.

    Previously this function fell back to walking ``GetPropertyStack`` when
    ``GetTargets`` was empty but ``HasAuthoredTargets`` was true, so it could
    "recover" targets authored on a weaker layer.  That contradicts USD
    composition: when the user clears a relationship on the edit target
    (``SetTargets([])``), the strongest opinion is the empty list, and
    surfacing the referenced/payload layer's targets reintroduces the link
    the user just deleted -- so removing the prim from the editor and
    re-adding it brings the deleted connection back.  ``GetTargets`` is the
    source of truth; nothing else.
    """
    try:
        return list(rel.GetTargets())
    except (AttributeError, RuntimeError, TypeError):
        return []


def make_link_for_connection(
    prim_path_str: str,
    pin_name: str,
    property_name: str,
    is_input: bool,
    is_bare: bool,
    conn_path: Any,
) -> dict[str, Any] | None:
    """Build a link dict from a single USD attribute connection.

    Returns ``None`` if ``conn_path`` is not a prim-property path. For bare
    (dual-pin) attributes, the direction is inferred from the target property.
    """
    if not conn_path.IsPrimPropertyPath():
        return None

    target_prim_path = str(conn_path.GetPrimPath())
    target_prop_name = _get_path_property_name(conn_path)
    target_pin_name = _strip_pin_prefix(target_prop_name)

    if is_bare:
        is_input = infer_bare_link_direction(target_prop_name)

    if is_input:
        return {
            "sourceNodeId": target_prim_path,
            "sourcePinName": target_pin_name,
            "targetNodeId": prim_path_str,
            "targetPinName": pin_name,
            "is_input_link": True,
            "propertyOwnerNodeId": prim_path_str,
            "propertyName": property_name,
            "is_relationship_link": False,
            "sourcePropertyName": target_prop_name,
            "targetPropertyName": property_name,
        }
    return {
        "sourceNodeId": prim_path_str,
        "sourcePinName": pin_name,
        "targetNodeId": target_prim_path,
        "targetPinName": target_pin_name,
        "is_input_link": False,
        "propertyOwnerNodeId": prim_path_str,
        "propertyName": property_name,
        "is_relationship_link": False,
        "sourcePropertyName": property_name,
        "targetPropertyName": target_prop_name,
    }


def make_link_for_relationship(
    prim: Any,
    relationship_name: str,
    target_path: Any,
) -> dict[str, Any] | None:
    """Build a link dict from a supported USD relationship target."""
    prim_path_str = str(prim.GetPath())
    target_pin_name = ""
    target_property_name = ""

    if target_path.IsPrimPropertyPath():
        target_prim_path = str(target_path.GetPrimPath())
        target_property_name = _get_path_property_name(target_path)
        try:
            from ._schema_pin_names import get_display_pin_name_for_property_name

            target_prim = prim.GetStage().GetPrimAtPath(target_path.GetPrimPath())
            if target_prim and target_prim.IsValid():
                display_pin_name = get_display_pin_name_for_property_name(
                    target_prim,
                    target_property_name,
                )
                if display_pin_name is not None:
                    target_pin_name = display_pin_name
        except (AttributeError, ImportError, RuntimeError, TypeError):
            target_pin_name = ""

        if not target_pin_name:
            target_pin_name = _strip_pin_prefix(target_property_name)
    elif target_path.IsPrimPath():
        target_prim_path = str(target_path)
    else:
        return None

    # `sources` is stored on the target/input prim when authoring a
    # relationship-to-relationship link into another `sources` row. During
    # rebuild, preserve that authored direction instead of treating the current
    # prim as the visible output side and flipping the noodle.
    if is_relationship_input_pin(relationship_name) and is_relationship_input_pin(
        target_property_name
    ):
        return {
            "sourceNodeId": target_prim_path,
            "sourcePinName": target_pin_name,
            "targetNodeId": prim_path_str,
            "targetPinName": relationship_name,
            "is_input_link": True,
            "propertyOwnerNodeId": prim_path_str,
            "propertyName": relationship_name,
            "is_relationship_link": True,
            "sourcePropertyName": target_property_name,
            "targetPropertyName": relationship_name,
        }

    if is_relationship_output_pin(relationship_name):
        return {
            "sourceNodeId": prim_path_str,
            "sourcePinName": relationship_name,
            "targetNodeId": target_prim_path,
            "targetPinName": target_pin_name,
            "is_input_link": False,
            "propertyOwnerNodeId": prim_path_str,
            "propertyName": relationship_name,
            "is_relationship_link": True,
            "sourcePropertyName": relationship_name,
            "targetPropertyName": target_property_name,
        }

    if is_relationship_input_pin(relationship_name):
        return {
            "sourceNodeId": target_prim_path,
            "sourcePinName": target_pin_name,
            "targetNodeId": prim_path_str,
            "targetPinName": relationship_name,
            "is_input_link": True,
            "propertyOwnerNodeId": prim_path_str,
            "propertyName": relationship_name,
            "is_relationship_link": True,
            "sourcePropertyName": target_property_name,
            "targetPropertyName": relationship_name,
        }

    # Default: unknown relationships are outgoing references; emit an
    # output-side link so e.g. material:binding draws from the right
    # edge of the source prim to its target.
    return {
        "sourceNodeId": prim_path_str,
        "sourcePinName": relationship_name,
        "targetNodeId": target_prim_path,
        "targetPinName": target_pin_name,
        "is_input_link": False,
        "propertyOwnerNodeId": prim_path_str,
        "propertyName": relationship_name,
        "is_relationship_link": True,
        "sourcePropertyName": relationship_name,
        "targetPropertyName": target_property_name,
    }


def collect_links_for_prim(prim: Any) -> list[dict[str, Any]]:
    """Collect attribute and supported relationship links for a prim."""
    links: list[dict[str, Any]] = []
    prim_path_str = str(prim.GetPath())

    for attr in prim.GetAttributes():
        cls = classify_attribute(attr)
        if cls is None:
            continue
        pin_name, is_input, _is_output, is_bare = cls
        property_name = attr.GetName()

        for conn_path in attr.GetConnections():
            link = make_link_for_connection(
                prim_path_str,
                pin_name,
                property_name,
                is_input,
                is_bare,
                conn_path,
            )
            if link is not None:
                links.append(link)

    for rel in prim.GetRelationships():
        cls = classify_relationship(rel)
        if cls is None:
            continue
        relationship_name, _is_input, _is_output = cls

        for target_path in _get_relationship_authored_targets(rel):
            link = make_link_for_relationship(
                prim,
                relationship_name,
                target_path,
            )
            if link is not None:
                links.append(link)

    return deduplicate_links(links)


def get_relationship_link_metadata(
    source_node_id: str,
    source_port: str,
    target_node_id: str,
    target_port: str,
) -> dict[str, Any]:
    """Return selection metadata for relationship-backed links."""
    if is_relationship_output_pin(source_port):
        return {
            "propertyOwnerNodeId": source_node_id,
            "propertyName": source_port,
            "is_relationship_link": True,
        }
    if is_relationship_input_pin(target_port):
        return {
            "propertyOwnerNodeId": target_node_id,
            "propertyName": target_port,
            "is_relationship_link": True,
        }
    return {}


def triangle_points(
    center_x: float,
    center_y: float,
    radius: float,
    *,
    is_output: bool,
) -> tuple[tuple[float, float], tuple[float, float], tuple[float, float]]:
    """Return the 3 points for a left- or right-facing isosceles triangle."""
    base_x = center_x - radius if is_output else center_x + radius
    tip_x = center_x + radius if is_output else center_x - radius
    top_y = center_y - radius
    bottom_y = center_y + radius
    return ((tip_x, center_y), (base_x, top_y), (base_x, bottom_y))


def inset_triangle_points(
    points: tuple[tuple[float, float], tuple[float, float], tuple[float, float]],
    inset_ratio: float,
) -> tuple[tuple[float, float], tuple[float, float], tuple[float, float]]:
    """Shrink triangle points toward their centroid by ``inset_ratio``."""
    centroid_x = sum(point[0] for point in points) / 3.0
    centroid_y = sum(point[1] for point in points) / 3.0

    inset_points = []
    for point_x, point_y in points:
        inset_points.append(
            (
                point_x + (centroid_x - point_x) * inset_ratio,
                point_y + (centroid_y - point_y) * inset_ratio,
            )
        )
    return cast(
        tuple[tuple[float, float], tuple[float, float], tuple[float, float]],
        tuple(inset_points),
    )


def arrowhead_points(
    tip_x: float,
    tip_y: float,
    prev_x: float,
    prev_y: float,
    *,
    length: float,
    width: float,
) -> tuple[tuple[float, float], tuple[float, float], tuple[float, float]]:
    """Return a filled arrowhead triangle pointing from ``prev`` to ``tip``."""
    dir_x = tip_x - prev_x
    dir_y = tip_y - prev_y
    magnitude = math.hypot(dir_x, dir_y)
    if magnitude <= 1e-6:
        unit_x, unit_y = 1.0, 0.0
    else:
        unit_x = dir_x / magnitude
        unit_y = dir_y / magnitude

    perp_x = -unit_y
    perp_y = unit_x
    base_center_x = tip_x - unit_x * length
    base_center_y = tip_y - unit_y * length
    half_width = width * 0.5

    return (
        (tip_x, tip_y),
        (base_center_x + perp_x * half_width, base_center_y + perp_y * half_width),
        (base_center_x - perp_x * half_width, base_center_y - perp_y * half_width),
    )
