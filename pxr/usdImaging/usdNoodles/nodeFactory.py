#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
Node Factory - Central node creation from USD prims using NodeLibraries.

This module provides the single interface for creating NodeModel objects
from USD prims, using the appropriate NodeLibrary to gather port information.
"""

from typing import List, Optional

from pxr import Usd, UsdUI

from ._schema_pin_names import (
    get_api_schema_pin_groups,
    get_schema_aware_pin_entries,
    get_schema_aware_pin_names,
    get_schema_aware_pin_properties,
    populate_descriptor_pins,
)
from .models import NodeModel
from .nodeLibraries import NodeLibrary
from .pinUtils import split_direction_hint

_SUPPRESSED_PRIM_TYPES = frozenset(
    {
        "ExecNode",
        "RigGraph",
        "RigGraphContainer",
        "RigGraphNode",
    }
)


def _is_suppressed_prim(prim: Usd.Prim) -> bool:
    return prim.GetTypeName() in _SUPPRESSED_PRIM_TYPES


def _get_direction_group_pins_from_prim(prim: Usd.Prim) -> tuple[set[str], set[str]]:
    schema_groups = get_api_schema_pin_groups(prim)
    input_pins: set[str] = set()
    output_pins: set[str] = set()

    for attr_name in get_schema_aware_pin_names(prim):
        side, pin_name = split_direction_hint(attr_name)
        if side is None:
            continue
        if ":" not in pin_name and pin_name in schema_groups:
            pin_name = f"{schema_groups[pin_name]}:{pin_name}"
        if side == "input":
            input_pins.add(pin_name)
        else:
            output_pins.add(pin_name)

    return input_pins, output_pins


def _hide_ui_pins() -> bool:
    """Whether ``ui:`` namespace attrs are hidden from pins (config, default on).

    ``NoodlesConfig`` imports Qt (``pxr.Usdviewq.qt``) at module load. Descriptor
    creation must work headless (unit tests, batch tooling), so when that import
    is unavailable fall back to the documented default rather than crashing.
    """
    try:
        from .noodlesConfig import NoodlesConfig
    except ImportError:
        return True

    return bool(NoodlesConfig.get("hideUiNamespacePins", True))


def _get_relationship_pins_from_prim(prim: Usd.Prim) -> tuple[set[str], set[str]]:
    input_pins: set[str] = set()
    output_pins: set[str] = set()

    for pin_property in get_schema_aware_pin_properties(prim):
        if not pin_property.is_relationship:
            continue
        if pin_property.side == "input":
            input_pins.add(pin_property.pin_name)
        elif pin_property.side == "output":
            output_pins.add(pin_property.pin_name)

    return input_pins, output_pins


class NodeFactory:
    """
    Factory for creating NodeModel objects from USD prims.

    Uses NodeLibraries to gather port and type information, ensuring
    consistent node creation across all code paths (hotbox, tree add,
    graph loading, dangling links).

    This factory queries registered node libraries to find one that can handle
    a given prim type, then uses its descriptor to create a NodeModel with
    proper ports, types, and UI style.
    """

    def __init__(self, node_libraries: List[NodeLibrary]):
        """
        Initialize with a list of node libraries.

        Args:
            node_libraries: List of NodeLibrary instances to query
        """
        self.node_libraries = node_libraries

    def create_node_from_prim(
        self, prim: Usd.Prim, stage: Usd.Stage, default_renderer=None
    ) -> Optional[NodeModel]:
        """
        Create a NodeModel from a USD prim.

        Queries all registered node libraries to find one that can handle
        this prim type, then uses its descriptor to create a NodeModel
        with proper ports and UI style.

        This method ensures consistent node creation across all code paths:
        - Hotbox creation (after prim is created)
        - USD tree add (user selects existing prims)
        - Graph loading (loading Blueprint graphs)
        - Dangling link expansion (double-clicking to add missing nodes)

        Args:
            prim: USD prim to create node from
            stage: USD stage
            default_renderer: Default renderer if library doesn't provide one

        Returns:
            NodeModel instance, or None if no library can handle this prim
        """
        descriptor = self._create_descriptor_from_prim(prim, stage)
        if descriptor is None:
            return None

        # Create NodeModel from descriptor
        # Renderer priority: descriptor > default_renderer > createNodeRenderer with uiStyle
        renderer = descriptor.get("renderer") or default_renderer
        ui_style = descriptor.get("uiStyle")

        # If no renderer provided but uiStyle is set, let NodeModel create it
        node = NodeModel(
            stage=stage,
            primPath=descriptor["primPath"],
            renderer=renderer,
            uiStyle=ui_style,
        )
        input_direction_group_pins, output_direction_group_pins = (
            _get_direction_group_pins_from_prim(prim)
        )
        relationship_input_pins, relationship_output_pins = (
            _get_relationship_pins_from_prim(prim)
        )
        node.set_ordered_pin_entries(
            get_schema_aware_pin_entries(prim, hide_ui=_hide_ui_pins())
        )
        node.set_direction_group_pins(
            input_pins=input_direction_group_pins,
            output_pins=output_direction_group_pins,
        )
        node.set_relationship_pins(
            input_pins=relationship_input_pins,
            output_pins=relationship_output_pins,
        )

        # Set port information from descriptor.
        # Types are set first so that _capture_originals_and_rebuild (triggered
        # by the inputPins/outputPins setters) captures the correct type map
        # into _original_input_pin_types / _original_output_pin_types.
        node.inputPinTypes = descriptor.get("inputPinTypes", {})
        node.outputPinTypes = descriptor.get("outputPinTypes", {})
        self._apply_pin_descriptor(node, descriptor)

        # Force lazy-load of name and type to populate C++ NodeData fields
        # before calculateNodeSize is called. The C++ sizing code reads
        # directly from the struct, so we must populate these fields now.
        _ = node.name
        _ = node.type
        _ = node.schemaTypeName

        self._apply_metadata_from_prim(node, prim)

        return node

    def _create_descriptor_from_prim(
        self, prim: Usd.Prim, stage: Usd.Stage
    ) -> Optional[dict]:
        if _is_suppressed_prim(prim):
            return None

        for library in self.node_libraries:
            if library.enabled and library.can_handle_prim(prim):
                descriptor = library.create_node_descriptor_from_prim(prim, stage)
                if descriptor:
                    return descriptor

        # No library can handle this - use generic fallback that reads
        # inputs:/outputs: attributes directly from the prim.
        return self._generic_descriptor_from_prim(prim)

    @staticmethod
    def _apply_pin_descriptor(node: NodeModel, descriptor: dict) -> None:
        input_pins = descriptor.get("inputPins", [])
        output_pins = descriptor.get("outputPins", [])

        # Every non-relationship attribute is dual (both pins on its one row).
        # Each row keeps a single slot on its primary side and mirrors a port
        # onto the opposite edge:
        #   * input-side rows (bare, namespaced, in:/input:/inputs:) live in
        #     ``inputPins`` and mirror an OUTPUT port to the right edge
        #     (``_dual_pin_names``).
        #   * output-side rows (out:/output:/outputs:) live in ``outputPins``
        #     and mirror an INPUT port to the left edge
        #     (``_output_dual_pin_names``).
        # ``populate_descriptor_pins`` lists each bare/namespaced dual in BOTH
        # lists; those are input-side rows, so drop them from the real output
        # rows (keeping only the genuine out:-hint pins) and let the mirror
        # supply their output port. Routing every dual through a single-slot row
        # keeps nested rows aligned — a duplicate real output row would not get a
        # slot assigned, leaving the pin missing/misplaced.
        input_set = set(input_pins)
        output_pins = [p for p in output_pins if p not in input_set]

        node.inputPins = input_pins
        node.outputPins = output_pins
        node._dual_pin_names = {
            pin_name
            for pin_name in input_pins
            if not node.is_relationship_pin(pin_name, False)
        }
        node._output_dual_pin_names = {
            pin_name
            for pin_name in output_pins
            if not node.is_relationship_pin(pin_name, True)
        }

    def _apply_metadata_from_prim(self, node: NodeModel, prim: Usd.Prim) -> None:
        self._apply_icon_from_prim(node, prim)
        self._apply_display_color_from_prim(node, prim)
        self._apply_expansion_state_from_prim(node, prim)

    @staticmethod
    def _apply_icon_from_prim(node: NodeModel, prim: Usd.Prim) -> None:
        try:
            icon_attr = prim.GetAttribute("ui:nodegraph:node:icon")
            if icon_attr.IsValid() and icon_attr.HasAuthoredValue():
                icon_asset = icon_attr.Get()
                icon_path = icon_asset.resolvedPath or icon_asset.path
                if icon_path:
                    node._icon_path = str(icon_path)
        except Exception:
            pass

    @staticmethod
    def _apply_display_color_from_prim(node: NodeModel, prim: Usd.Prim) -> None:
        try:
            color_attr = UsdUI.NodeGraphNodeAPI(prim).GetDisplayColorAttr()
            if color_attr.IsValid() and color_attr.HasAuthoredValue():
                color = color_attr.Get()
                if color is not None:
                    node._display_color = (color[0], color[1], color[2])
                    node.displayColorR = color[0]
                    node.displayColorG = color[1]
                    node.displayColorB = color[2]
        except Exception:
            pass

    @staticmethod
    def _apply_expansion_state_from_prim(node: NodeModel, prim: Usd.Prim) -> None:
        try:
            expansion_attr = UsdUI.NodeGraphNodeAPI(prim).GetExpansionStateAttr()
            if expansion_attr.IsValid() and expansion_attr.HasAuthoredValue():
                state = str(expansion_attr.Get())
                if state in ("closed", "minimized"):
                    node._title_collapsed = True
                    node.titleCollapsed = True
        except Exception:
            pass

    def _generic_descriptor_from_prim(self, prim: Usd.Prim) -> dict:
        """Fallback descriptor for prims that no library recognizes."""
        descriptor = {
            "primPath": prim.GetPath(),
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
            "uiStyle": "default",
            "renderer": None,
        }
        populate_descriptor_pins(
            prim, descriptor, check_port_type=True, hide_ui=_hide_ui_pins()
        )
        return descriptor

    def get_ui_style_for_prim(self, prim: Usd.Prim) -> Optional[str]:
        """
        Get the UI style for a prim.

        Queries registered libraries to determine which renderer style
        should be used for drawing this prim's node.

        Args:
            prim: USD prim

        Returns:
            UI style string (e.g., "default", "graffi"), or None if no library handles this prim
        """
        if _is_suppressed_prim(prim):
            return None

        for library in self.node_libraries:
            if library.enabled and library.can_handle_prim(prim):
                return library.get_prim_ui_style(prim)
        return None
