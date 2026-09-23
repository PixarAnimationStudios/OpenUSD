#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles import _usdNoodles as _noodles
from pxr.UsdNoodles.core import NodeData
from pxr import Gf, Sdf

from ._schema_pin_names import (
    get_api_schema_pin_groups,
    get_attribute_type_name,
    get_schema_aware_pin_entries,
    get_schema_aware_pin_properties,
)
from .nodeGraph import warn_if_non_persistent_edit_target
from .pinUtils import split_direction_hint

# Grab the C++ def_readwrite descriptors from NodeData so we can write to
# the underlying C++ struct fields even though NodeModel overrides them
# with Python @property.  This keeps C++ methods (generateVertices,
# calculateNodeSize, etc.) in sync with the Python-level values.
_cpp_position = NodeData.__dict__["position"]
_cpp_name = NodeData.__dict__["name"]
_cpp_type = NodeData.__dict__["type"]
_cpp_schemaTypeName = NodeData.__dict__["schemaTypeName"]
_cpp_selected = NodeData.__dict__["selected"]
_cpp_inputPins = NodeData.__dict__["inputPins"]
_cpp_outputPins = NodeData.__dict__["outputPins"]
_cpp_inputPinTypes = NodeData.__dict__["inputPinTypes"]
_cpp_outputPinTypes = NodeData.__dict__["outputPinTypes"]
_cpp_inputRowKinds = NodeData.__dict__["inputRowKinds"]
_cpp_outputRowKinds = NodeData.__dict__["outputRowKinds"]
_cpp_inputRowSlots = NodeData.__dict__["inputRowSlots"]
_cpp_outputRowSlots = NodeData.__dict__["outputRowSlots"]
_cpp_displayRowKinds = NodeData.__dict__["displayRowKinds"]
_cpp_relationshipInputPins = NodeData.__dict__["relationshipInputPins"]
_cpp_relationshipOutputPins = NodeData.__dict__["relationshipOutputPins"]
# Raw USD-derived CONTENT mirrored to C++ so the C++ layout producer can derive
# display pins / slots / size from it (see _sync_content_to_cpp).
_cpp_originalInputPins = NodeData.__dict__["originalInputPins"]
_cpp_originalOutputPins = NodeData.__dict__["originalOutputPins"]
_cpp_originalInputPinTypes = NodeData.__dict__["originalInputPinTypes"]
_cpp_originalOutputPinTypes = NodeData.__dict__["originalOutputPinTypes"]
_cpp_foldState = NodeData.__dict__["foldState"]
_cpp_orderedPinEntries = NodeData.__dict__["orderedPinEntries"]
_cpp_inputDirectionGroupPins = NodeData.__dict__["inputDirectionGroupPins"]
_cpp_outputDirectionGroupPins = NodeData.__dict__["outputDirectionGroupPins"]
_cpp_dualPinNames = NodeData.__dict__["dualPinNames"]
_cpp_outputDualPinNames = NodeData.__dict__["outputDualPinNames"]
_cpp_titleIconPath = NodeData.__dict__["titleIconPath"]


def _sync_cpp_map(descriptor, instance, py_dict):
    """Sync a Python dict into a C++ StringMap (unordered_map<string,string>).

    Boost::python's map_indexing_suite doesn't register a from_python
    converter for plain dicts, so we can't assign directly.  Instead we
    get the proxy and update key-by-key.  Stale entries for pins no longer
    in py_dict are left in place — rendering only looks up types for
    currently visible pins by name, so extra entries are harmless.
    """
    cpp_map = descriptor.__get__(instance)
    for key, val in py_dict.items():
        cpp_map[key] = val


def _sync_cpp_vector(descriptor, instance, py_list):
    descriptor.__set__(instance, list(py_list))


class NodeModel(NodeData):
    """
    USD-aware data model for a node in the graph.

    Inherits pure data fields from NodeData and adds USD prim-backed
    property accessors. Position edits are written to the session layer
    by default. During drag operations, position is cached locally
    and only written to USD on drag completion.
    """

    def __init__(self, stage=None, primPath=None, renderer=None, uiStyle=None):
        # NodeData C++ class takes no arguments in __init__
        super().__init__()
        # Set id attribute after construction
        self.id = str(primPath) if primPath else ""

        # USD references - if provided, this is a USD-backed node
        self._stage = stage
        self._primPath = primPath
        self._prim = stage.GetPrimAtPath(primPath) if stage and primPath else None

        # Drag state for batched position writes
        self._dragging = False
        self._dragPosition = None

        # Use None as sentinel to distinguish "not loaded" from "loaded but empty"
        self._name = None
        self._type = None
        self._schemaTypeName = None
        self._position = None
        self._input_pins = None
        self._output_pins = None
        self._input_pin_types = None
        self._output_pin_types = None

        # Python-side link storage (C++ inputLinks/outputLinks is vector<int>,
        # but the Python code stores LinkData objects here)
        self._input_links = []
        self._output_links = []

        # Fold/unfold state for namespaced pin groups
        self._fold_state = {}  # {"In": True, "Out": False}  True=folded
        self._original_input_pins = None
        self._original_output_pins = None
        self._original_input_pin_types = None
        self._original_output_pin_types = None
        # Row kinds / slots / displayRowKinds and the folded child->header maps
        # live solely on the C++ struct now (computed by the C++ layout producer
        # and read via node.inputRowSlots / node.foldedInputPinMap, etc.).
        # Dual-pin names: input-side pins (bare, namespaced, and in:/input:/
        # inputs: direction hints) that ALSO expose a mirrored OUTPUT port on the
        # right edge of their row.
        self._dual_pin_names = set()
        # Output-dual pin names: output-side pins (out:/output:/outputs: hints)
        # that ALSO expose a mirrored INPUT port on the left edge of their row.
        # Symmetric counterpart to _dual_pin_names so every attribute row is dual.
        self._output_dual_pin_names = set()
        # Schema-group alias maps: USD bare name → prefixed display name
        self._input_pin_alias_map: dict[str, str] = {}
        self._output_pin_alias_map: dict[str, str] = {}
        self._schema_pin_groups_cache: dict[str, str] | None = None
        self._ordered_pin_entries: list[tuple[str, str]] | None = None
        # Pins that originated from authored inputs:/outputs: namespaces.
        self._input_direction_group_pins: set[str] = set()
        self._output_direction_group_pins: set[str] = set()
        self._relationship_input_pins: set[str] = set()
        self._relationship_output_pins: set[str] = set()

        # Title-level collapse: hides all property rows, shows aggregate port
        self._title_collapsed = False
        self._icon_path = None  # Path to node icon image (from ui_node_icon metadata)
        self._display_color = (
            None  # Display color tuple (R, G, B) from UsdUINodeGraphNodeAPI
        )
        _sync_cpp_vector(_cpp_inputRowKinds, self, [])
        _sync_cpp_vector(_cpp_outputRowKinds, self, [])
        _sync_cpp_vector(_cpp_inputRowSlots, self, [])
        _sync_cpp_vector(_cpp_outputRowSlots, self, [])
        _sync_cpp_vector(_cpp_displayRowKinds, self, [])

        # UI style identifier for selecting renderer type
        self.uiStyle = uiStyle if uiStyle is not None else ""

        # Renderer for custom node appearance
        if renderer is None and uiStyle is not None:
            from .nodeGraph import createNodeRenderer

            self.renderer = createNodeRenderer(uiStyle)
        else:
            self.renderer = renderer

    @property
    def name(self):
        """Get node name from USD or cache."""
        if self._name is not None:
            return self._name
        if self._prim:
            self._name = self._prim.GetName()
            _cpp_name.__set__(self, self._name)
            return self._name
        return ""

    @name.setter
    def name(self, value):
        """Set cached name (USD prims can't be renamed after creation)."""
        self._name = value
        _cpp_name.__set__(self, value)

    @property
    def type(self):
        """Get node type from USD info:implementationSource or cache."""
        if self._type is not None:
            return self._type
        if self._prim:
            implSourceAttr = self._prim.GetAttribute("info:implementationSource")
            if implSourceAttr and implSourceAttr.HasValue():
                self._type = str(implSourceAttr.Get())
            else:
                self._type = self._prim.GetTypeName()
            _cpp_type.__set__(self, self._type)
            return self._type
        return ""

    @type.setter
    def type(self, value):
        """Set node type (writes to USD if available)."""
        self._type = value
        _cpp_type.__set__(self, value)
        if self._prim:
            implSourceAttr = self._prim.GetAttribute("info:implementationSource")
            if not implSourceAttr:
                implSourceAttr = self._prim.CreateAttribute(
                    "info:implementationSource", Sdf.ValueTypeNames.String
                )
            if implSourceAttr:
                implSourceAttr.Set(value)

    @property
    def schemaTypeName(self):
        """Get the USD schema type name (prim.GetTypeName()) for display."""
        if self._schemaTypeName is not None:
            return self._schemaTypeName
        if self._prim:
            self._schemaTypeName = self._prim.GetTypeName()
            _cpp_schemaTypeName.__set__(self, self._schemaTypeName)
            return self._schemaTypeName
        return ""

    @schemaTypeName.setter
    def schemaTypeName(self, value):
        """Set cached schema type name."""
        self._schemaTypeName = value
        _cpp_schemaTypeName.__set__(self, value)

    @property
    def position(self):
        """Get node position from USD ui:nodegraph:node:pos or cache.

        During drag operations, returns the cached drag position for performance.
        Otherwise reads directly from USD.
        """
        # During drag, return the cached drag position
        if self._dragging and self._dragPosition is not None:
            return self._dragPosition

        # If we have a cached position (from previous read), use it
        if self._position is not None:
            return self._position

        # Read from USD
        if self._prim:
            posAttr = self._prim.GetAttribute("ui:nodegraph:node:pos")
            if posAttr and posAttr.HasValue():
                pos = posAttr.Get()
                # USD stores in small units, scale up for display
                positionScale = 1000.0
                self._position = Gf.Vec2d(
                    pos[0] * positionScale, pos[1] * positionScale
                )
            else:
                self._position = Gf.Vec2d(0, 0)
            _cpp_position.__set__(self, self._position)
            return self._position
        return Gf.Vec2d(0, 0)

    @position.setter
    def position(self, value):
        """Set node position.

        During drag operations, only updates the cached drag position.
        Otherwise writes to the stage's current edit target immediately.
        """
        _cpp_position.__set__(self, value)
        if self._dragging:
            # During drag, just update the cached drag position
            self._dragPosition = value
        else:
            self._writePositionToUsd(value)
            self._position = value

    def setDisplayPosition(self, value):
        """Set the node's position for rendering only, without authoring USD.

        Updates just the cached position and the embedded C++ ``NodeData`` base
        that the render snapshot reads; unlike the ``position`` setter it does not
        write ``ui:nodegraph:node:pos``. Used by auto-layout so placement can
        never fail on a prim that won't accept the attribute (and opening a stage
        never dirties it). A later drag still authors through the normal setter.
        """
        _cpp_position.__set__(self, value)
        self._position = value

    def _writePositionToUsdRaw(self, value):
        """Write position attribute to the stage's current edit target.

        Args:
            value: Gf.Vec2d position in display coordinates.
        """
        if not self._prim or not self._prim.IsValid():
            return
        # Pseudo-root "/" has no path name component and does not support attributes.
        if not self._prim.GetPath().name:
            return
        # Prims inside a USD instance are instance proxies; USD forbids authoring
        # to them ("authoring to an instance proxy is not allowed"). Skip the USD
        # write rather than raising — the display position still updates, and
        # bulk authoring (auto-layout, save) must not abort on instanced nodes.
        if self._prim.IsInstanceProxy():
            return

        posAttr = self._prim.GetAttribute("ui:nodegraph:node:pos")
        if not posAttr or not posAttr.IsValid():
            posAttr = self._prim.CreateAttribute(
                "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
        if posAttr and posAttr.IsValid():
            positionScale = 1.0 / 1000.0
            posAttr.Set(Gf.Vec2f(value[0] * positionScale, value[1] * positionScale))

    def _writePositionToUsd(self, value):
        """Write position to the stage's current edit target.

        Args:
            value: Gf.Vec2d position in display coordinates.
        """
        if self._prim and self._prim.IsValid():
            warn_if_non_persistent_edit_target(self._prim.GetStage())
        self._writePositionToUsdRaw(value)

    def _write_expansion_state_to_usd(self):
        """Write expansion state to the stage's current edit target."""
        if not self._prim:
            return
        from pxr import UsdUI

        warn_if_non_persistent_edit_target(self._prim.GetStage())
        api = UsdUI.NodeGraphNodeAPI(self._prim)
        attr = api.GetExpansionStateAttr()
        if not attr or not attr.IsValid():
            attr = api.CreateExpansionStateAttr()
        if attr:
            state = "closed" if self._title_collapsed else "open"
            attr.Set(state)

    def beginDrag(self):
        """Begin a drag operation - cache position for performance.

        Call this when starting to drag the node. Position updates during
        the drag will be cached locally and only written to USD on endDrag().
        """
        self._dragging = True
        # Cache current position as the starting drag position
        self._dragPosition = self.position

    def endDrag(self):
        """End a drag operation - write final position to USD.

        Call this when the drag is complete. The final position is written
        to the stage's current edit target.
        """
        if self._dragging and self._dragPosition is not None:
            self._dragging = False
            # Write the final drag position to USD
            self._writePositionToUsd(self._dragPosition)
            # Update cache with final position
            self._position = self._dragPosition
            self._dragPosition = None
        else:
            self._dragging = False
            self._dragPosition = None

    def cancelDrag(self):
        """Cancel a drag operation - revert to original position.

        Call this to abort a drag without saving changes.
        """
        self._dragging = False
        self._dragPosition = None
        # _position still holds the original position

    @property
    def inputPins(self):
        """Get input pins from USD inputs: attributes or cache.

        Uses schema-aware discovery so prims with only ``typeName`` set
        (hotbox-created) still surface their built-in API-schema pins.
        """
        if self._input_pins is not None:
            return self._input_pins
        if self._prim:
            self._ensure_ordered_pin_entries()
            raw_pins = []
            raw_types = {}
            self._dual_pin_names = set()
            prefixed_pins = set()
            relationship_pins = set()
            for pin_property in get_schema_aware_pin_properties(self._prim):
                if pin_property.side != "input":
                    continue
                attrName = pin_property.property_name
                pin_name = pin_property.pin_name
                # Every non-relationship attribute is mirror-dual: an input-side
                # row whose port is also mirrored onto the right edge, so every
                # such row exposes both an input and an output pin and is
                # grabbable from either edge. This covers bare attributes,
                # namespaced attributes (e.g. ``rig1:space``), AND ``in:``/
                # ``input:``/``inputs:`` direction-hint attributes alike. This
                # matches ``classify_attribute`` in pinUtils. Output-side hint
                # rows (``out:``/...) get their mirror on the opposite edge via
                # ``_output_dual_pin_names`` (see the ``outputPins`` getter).
                is_dual = not pin_property.is_relationship
                if split_direction_hint(attrName)[0] == "input":
                    prefixed_pins.add(pin_name)
                if pin_property.is_relationship:
                    relationship_pins.add(pin_name)
                if pin_name not in raw_pins:
                    raw_pins.append(pin_name)
                    if is_dual:
                        self._dual_pin_names.add(pin_name)
                    if not pin_property.is_relationship:
                        attr = self._prim.GetAttribute(attrName)
                        portType = (
                            attr.GetCustomDataByKey("portType")
                            if attr and attr.IsValid()
                            else None
                        )
                        if portType:
                            raw_types[pin_name] = portType
                        else:
                            type_str = get_attribute_type_name(self._prim, attrName)
                            if type_str:
                                raw_types[pin_name] = type_str
            self._input_direction_group_pins = prefixed_pins
            self._set_relationship_input_pins(relationship_pins)
            self._input_pins = raw_pins
            if self._input_pin_types is None:
                self._input_pin_types = raw_types
            self._capture_originals_and_rebuild(is_output=False)
            return self._input_pins
        return []

    @inputPins.setter
    def inputPins(self, value):
        """Set cached input pins (USD attributes created via separate API)."""
        self._input_pins = value
        _cpp_inputPins.__set__(self, list(value))
        self._capture_originals_and_rebuild(is_output=False)

    @property
    def outputPins(self):
        """Get output pins from USD outputs: attributes or cache.

        Uses schema-aware discovery so prims with only ``typeName`` set
        (hotbox-created) still surface their built-in API-schema pins. Bare and
        namespaced non-hint attributes are excluded here: they are mirror-dual
        rows rendered on the input side with a port mirrored onto the right edge
        (see ``inputPins`` and ``_dual_pin_names``). Only explicit
        ``out:``/``output:``/``outputs:`` attributes are real output rows; each
        such non-relationship row is itself dual, recorded in
        ``_output_dual_pin_names`` so it ALSO exposes a mirrored INPUT port on
        its left edge.
        """
        if self._output_pins is not None:
            return self._output_pins
        if self._prim:
            self._ensure_ordered_pin_entries()
            raw_pins = []
            raw_types = {}
            prefixed_pins = set()
            relationship_pins = set()
            self._output_dual_pin_names = set()
            for pin_property in get_schema_aware_pin_properties(self._prim):
                if pin_property.side != "output":
                    continue
                attrName = pin_property.property_name
                pin_name = pin_property.pin_name
                if split_direction_hint(attrName)[0] == "output":
                    prefixed_pins.add(pin_name)
                if pin_property.is_relationship:
                    relationship_pins.add(pin_name)
                if pin_name not in raw_pins:
                    raw_pins.append(pin_name)
                    if not pin_property.is_relationship:
                        # Every output-side attribute row is dual: it mirrors an
                        # INPUT port onto its left edge (see the render/interaction
                        # paths keyed on ``_output_dual_pin_names``).
                        self._output_dual_pin_names.add(pin_name)
                        attr = self._prim.GetAttribute(attrName)
                        portType = (
                            attr.GetCustomDataByKey("portType")
                            if attr and attr.IsValid()
                            else None
                        )
                        if portType:
                            raw_types[pin_name] = portType
                        else:
                            type_str = get_attribute_type_name(self._prim, attrName)
                            if type_str:
                                raw_types[pin_name] = type_str
            self._output_direction_group_pins = prefixed_pins
            self._set_relationship_output_pins(relationship_pins)
            if self._output_pin_types is None:
                self._output_pin_types = raw_types
            self._output_pins = raw_pins
            self._capture_originals_and_rebuild(is_output=True)
            return self._output_pins
        return []

    @outputPins.setter
    def outputPins(self, value):
        """Set cached output pins (USD attributes created via separate API)."""
        self._output_pins = value
        _cpp_outputPins.__set__(self, list(value))
        self._capture_originals_and_rebuild(is_output=True)

    @property
    def inputPinTypes(self):
        """Get input pin types from USD metadata or cache.

        Uses schema-aware discovery so built-in API-schema attrs contribute
        their types (via SdfAttributeSpec) when no runtime attribute value
        is authored.
        """
        if self._input_pin_types is not None:
            return self._input_pin_types
        if self._prim:
            self._input_pin_types = {}
            for pin_property in get_schema_aware_pin_properties(self._prim):
                if pin_property.side != "input" or pin_property.is_relationship:
                    continue
                attrName = pin_property.property_name
                pinName = pin_property.pin_name
                if pinName in self._input_pin_types:
                    continue
                attr = self._prim.GetAttribute(attrName)
                portType = (
                    attr.GetCustomDataByKey("portType")
                    if attr and attr.IsValid()
                    else None
                )
                if portType:
                    self._input_pin_types[pinName] = portType
                    continue
                type_str = get_attribute_type_name(self._prim, attrName)
                if type_str:
                    self._input_pin_types[pinName] = type_str
            return self._input_pin_types
        return {}

    @inputPinTypes.setter
    def inputPinTypes(self, value):
        """Set cached input pin types."""
        self._input_pin_types = value
        _sync_cpp_map(_cpp_inputPinTypes, self, value)

    @property
    def outputPinTypes(self):
        """Get output pin types from USD metadata or cache.

        Uses schema-aware discovery so built-in API-schema attrs contribute
        their types (via SdfAttributeSpec) when no runtime attribute value
        is authored.
        """
        if self._output_pin_types is not None:
            return self._output_pin_types
        if self._prim:
            self._output_pin_types = {}
            for pin_property in get_schema_aware_pin_properties(self._prim):
                if pin_property.side != "output" or pin_property.is_relationship:
                    continue
                attrName = pin_property.property_name
                pinName = pin_property.pin_name
                if pinName in self._output_pin_types:
                    continue
                attr = self._prim.GetAttribute(attrName)
                portType = (
                    attr.GetCustomDataByKey("portType")
                    if attr and attr.IsValid()
                    else None
                )
                if portType:
                    self._output_pin_types[pinName] = portType
                    continue
                type_str = get_attribute_type_name(self._prim, attrName)
                if type_str:
                    self._output_pin_types[pinName] = type_str
            return self._output_pin_types
        return {}

    @outputPinTypes.setter
    def outputPinTypes(self, value):
        """Set cached output pin types."""
        self._output_pin_types = value
        _sync_cpp_map(_cpp_outputPinTypes, self, value)

    @property
    def inputLinks(self):
        """Get input links (Python-side LinkData list)."""
        return self._input_links

    @inputLinks.setter
    def inputLinks(self, value):
        """Set input links."""
        self._input_links = value

    @property
    def outputLinks(self):
        """Get output links (Python-side LinkData list)."""
        return self._output_links

    @outputLinks.setter
    def outputLinks(self, value):
        """Set output links."""
        self._output_links = value

    def getUsdPrim(self):
        """Get the underlying USD prim, if this is a USD-backed node."""
        return self._prim

    def invalidateCache(self):
        """Invalidate all cached USD data, forcing a re-read on next access."""
        self._name = None
        self._type = None
        self._schemaTypeName = None
        self._position = None
        self._input_pins = None
        self._output_pins = None
        self._input_pin_types = None
        self._output_pin_types = None
        self._input_links = []
        self._output_links = []
        # Reset fold originals so they get recaptured on next access
        self._original_input_pins = None
        self._original_output_pins = None
        self._original_input_pin_types = None
        self._original_output_pin_types = None
        self._input_pin_alias_map = {}
        self._output_pin_alias_map = {}
        self._ordered_pin_entries = None
        # Clear the C++ struct's row kinds / slots / displayRowKinds so a render
        # between invalidation and the next layout can't show stale rows; the
        # folded maps are part of that struct state and clear with them.
        _sync_cpp_vector(_cpp_inputRowKinds, self, [])
        _sync_cpp_vector(_cpp_outputRowKinds, self, [])
        _sync_cpp_vector(_cpp_inputRowSlots, self, [])
        _sync_cpp_vector(_cpp_outputRowSlots, self, [])
        _sync_cpp_vector(_cpp_displayRowKinds, self, [])
        self.foldedInputPinMap = {}
        self.foldedOutputPinMap = {}
        self._schema_pin_groups_cache = None
        self._input_direction_group_pins = set()
        self._output_direction_group_pins = set()
        self._set_relationship_input_pins(())
        self._set_relationship_output_pins(())
        self._dual_pin_names = set()
        self._output_dual_pin_names = set()
        # Clear the mirrored content on the C++ struct too (slots above are
        # already cleared); the next pin-getter access repopulates it.
        self._sync_content_to_cpp()

    def _get_schema_pin_groups(self):
        if self._schema_pin_groups_cache is None:
            if self._prim:
                self._schema_pin_groups_cache = get_api_schema_pin_groups(self._prim)
            else:
                self._schema_pin_groups_cache = {}
        return self._schema_pin_groups_cache

    def _build_alias_map(self, pins):
        """Build reverse alias map: unprefixed USD name -> prefixed display name."""
        schema_groups = self._get_schema_pin_groups()
        if not schema_groups:
            return {}
        alias_map = {}
        pin_set = set(pins)
        for bare, schema_display in schema_groups.items():
            prefixed = f"{schema_display}:{bare}"
            if prefixed in pin_set:
                alias_map[bare] = prefixed
        return alias_map

    def set_direction_group_pins(self, *, input_pins=None, output_pins=None) -> None:
        """Record pins that originated from authored inputs:/outputs: namespaces."""
        if input_pins is not None:
            self._input_direction_group_pins = set(input_pins)
        if output_pins is not None:
            self._output_direction_group_pins = set(output_pins)

    def _set_relationship_input_pins(self, pins):
        """Set the input relationship-pin set and mirror it to the C++ NodeData."""
        self._relationship_input_pins = set(pins)
        _sync_cpp_vector(
            _cpp_relationshipInputPins, self, list(self._relationship_input_pins)
        )

    def _set_relationship_output_pins(self, pins):
        """Set the output relationship-pin set and mirror it to the C++ NodeData."""
        self._relationship_output_pins = set(pins)
        _sync_cpp_vector(
            _cpp_relationshipOutputPins, self, list(self._relationship_output_pins)
        )

    def set_relationship_pins(self, *, input_pins=None, output_pins=None) -> None:
        """Record pins backed by USD relationships instead of attributes."""
        if input_pins is not None:
            self._set_relationship_input_pins(input_pins)
        if output_pins is not None:
            self._set_relationship_output_pins(output_pins)

    def is_relationship_pin(self, pin_name: str, is_output: bool) -> bool:
        relationship_pins = (
            self._relationship_output_pins
            if is_output
            else self._relationship_input_pins
        )
        return pin_name in relationship_pins

    def has_visible_port(self, pin_name: str, is_output: bool) -> bool:
        if is_output:
            return (
                pin_name in (self._output_pins or [])
                or pin_name in self._dual_pin_names
            )
        return (
            pin_name in (self._input_pins or [])
            or pin_name in self._output_dual_pin_names
        )

    def _missing_endpoint_pins(self, names, *, is_output: bool) -> list[str]:
        """Filter connection-endpoint names down to those needing a new pin row.

        Drops empties, duplicates, relationship pins (which use synthetic
        ports), and endpoints that already have a visible port on this side.
        """
        missing: list[str] = []
        seen: set[str] = set()
        for name in names:
            if not name or name in seen:
                continue
            seen.add(name)
            if self.is_relationship_pin(name, is_output):
                continue
            if self.has_visible_port(name, is_output):
                continue
            missing.append(name)
        return missing

    def add_connection_endpoint_pins(self, input_names=(), output_names=()) -> bool:
        """Surface connection endpoints as pins even when the referenced
        property is not an authored or schema-declared attribute on the prim.

        Connections can reference properties that do not exist as attributes on
        a prim -- for example a typed prim whose schema is not registered, so
        outputs like ``out:space`` or ``avars:tx`` appear only as the *targets*
        of other prims' connections. Without a pin row the noodle attaches to
        the bare node edge with no pin. Each newly surfaced endpoint becomes a
        dual row (mirrored on the opposite edge) routed through the same
        grouping/layout machinery as discovered pins, so every property is
        treated uniformly regardless of schema availability.

        ``input_names`` become input-side rows, ``output_names`` output-side
        rows. Returns True if any pin was added. Idempotent: endpoints that are
        already visible are skipped.
        """
        # Build base pin lists / ordered entries first so we augment, not reset.
        _ = self.inputPins
        _ = self.outputPins
        ordered = self._ensure_ordered_pin_entries()

        add_inputs = self._missing_endpoint_pins(input_names, is_output=False)
        add_outputs = self._missing_endpoint_pins(output_names, is_output=True)
        if not add_inputs and not add_outputs:
            return False

        if add_inputs:
            if self._original_input_pins is None:
                self._original_input_pins = list(self._input_pins or [])
            for pin_name in add_inputs:
                if pin_name not in self._original_input_pins:
                    self._original_input_pins.append(pin_name)
                # Mirror onto the right (output) edge so the row is dual.
                self._dual_pin_names.add(pin_name)
                ordered.append(("input", pin_name))

        if add_outputs:
            if self._original_output_pins is None:
                self._original_output_pins = list(self._output_pins or [])
            for pin_name in add_outputs:
                if pin_name not in self._original_output_pins:
                    self._original_output_pins.append(pin_name)
                # Mirror onto the left (input) edge so the row is dual.
                self._output_dual_pin_names.add(pin_name)
                ordered.append(("output", pin_name))

        self._ordered_pin_entries = ordered
        if add_inputs:
            self._rebuild_display_pins(is_output=False)
        if add_outputs:
            self._rebuild_display_pins(is_output=True)
        return True

    def set_ordered_pin_entries(self, entries: list[tuple[str, str]]) -> None:
        """Record authored property order after USD schema pin normalization."""
        self._ordered_pin_entries = list(entries)
        if self._input_pins is not None or self._output_pins is not None:
            self._rebuild_display_pins()

    def _ensure_ordered_pin_entries(self) -> list[tuple[str, str]]:
        if self._ordered_pin_entries is None and self._prim:
            self._ordered_pin_entries = get_schema_aware_pin_entries(self._prim)
        return list(self._ordered_pin_entries or [])

    def _sync_content_to_cpp(self):
        """Mirror raw USD-derived CONTENT onto the embedded C++ NodeData.

        These are the inputs the centralized C++ layout producer derives display
        pins, row slots, size, and per-pin centers from. Python remains the USD
        data source; C++ owns the layout. Original pin-type maps use the
        update-key-by-key helper (no dict->StringMap converter); stale entries
        are harmless since lookups are by current pin name.
        """
        _sync_cpp_vector(_cpp_originalInputPins, self, self._original_input_pins or [])
        _sync_cpp_vector(
            _cpp_originalOutputPins, self, self._original_output_pins or []
        )
        _sync_cpp_map(
            _cpp_originalInputPinTypes, self, self._original_input_pin_types or {}
        )
        _sync_cpp_map(
            _cpp_originalOutputPinTypes, self, self._original_output_pin_types or {}
        )
        _cpp_foldState.__set__(self, dict(self._fold_state or {}))
        _cpp_orderedPinEntries.__set__(self, list(self._ordered_pin_entries or []))
        _sync_cpp_vector(
            _cpp_inputDirectionGroupPins,
            self,
            self._input_direction_group_pins or set(),
        )
        _sync_cpp_vector(
            _cpp_outputDirectionGroupPins,
            self,
            self._output_direction_group_pins or set(),
        )
        _sync_cpp_vector(_cpp_dualPinNames, self, self._dual_pin_names or set())
        _sync_cpp_vector(
            _cpp_outputDualPinNames, self, self._output_dual_pin_names or set()
        )
        # Title-bar icon path (empty string when unset; the C++ field is a
        # std::string and cannot take None). The C++ icon producer reads this.
        _cpp_titleIconPath.__set__(self, self._icon_path or "")

    def _rebuild_display_pins(self, is_output=False):
        """Re-derive the display pins / row kinds / slots / folded maps in C++.

        The grouping/fold/slot LOGIC lives in C++ now: sync the raw content onto
        the struct, run the C++ topology passes (buildDisplayPins +
        assignRowSlots — both text-metric free, so no font atlas is needed), then
        mirror the results back into the Python attrs that interaction/link code
        still reads. The ``is_output`` arg is ignored: the C++ producer always
        lays out both sides together (it is idempotent, so re-running is cheap).
        """
        if self._original_input_pins is None and self._original_output_pins is None:
            return
        self._sync_content_to_cpp()
        _noodles.buildDisplayPins(self)
        _noodles.assignRowSlots(self)
        self._read_layout_back_from_cpp()

    def _read_layout_back_from_cpp(self):
        """Mirror the C++ display pins + pin types back into the Python caches.

        The row slots / kinds / folded maps live solely on the C++ struct now;
        readers go straight to those accessors (node.inputRowSlots, etc.). Only
        the display pin LIST and pin-type maps are mirrored, because the
        std::vector<string>/map fields have no C++->Python read converter and
        Python's lazy getters + tooltips need them. Each side is mirrored only
        once its originals are loaded, so an unloaded side's lazy getter is not
        pre-empted by an empty list.
        """
        if self._original_input_pins is not None:
            self._input_pins = list(self.displayInputPins)
            in_types = self._original_input_pin_types or {}
            self._input_pin_types = {
                p: in_types[p] for p in self._input_pins if p in in_types
            }
            _sync_cpp_map(_cpp_inputPinTypes, self, self._input_pin_types)
        if self._original_output_pins is not None:
            self._output_pins = list(self.displayOutputPins)
            out_types = self._original_output_pin_types or {}
            self._output_pin_types = {
                p: out_types[p] for p in self._output_pins if p in out_types
            }
            _sync_cpp_map(_cpp_outputPinTypes, self, self._output_pin_types)

    def _capture_originals_and_rebuild(self, is_output=False):
        """Capture original pins on first call, then rebuild display pins.

        Uses the inputPinTypes / outputPinTypes property getters (not the raw
        attributes) so that types are lazy-loaded from USD when this is called
        after invalidateCache, giving _original_*_pin_types accurate data even
        when types haven't been explicitly set yet.
        """
        if is_output:
            if self._original_output_pins is None:
                self._original_output_pins = list(self._output_pins or [])
                self._original_output_pin_types = dict(self.outputPinTypes)
                self._output_pin_alias_map = self._build_alias_map(
                    self._original_output_pins
                )
            self._rebuild_display_pins(is_output=True)
        else:
            if self._original_input_pins is None:
                self._original_input_pins = list(self._input_pins or [])
                self._original_input_pin_types = dict(self.inputPinTypes)
                self._input_pin_alias_map = self._build_alias_map(
                    self._original_input_pins
                )
            self._rebuild_display_pins(is_output=False)

    def toggle_fold(self, prefix, is_output=False):
        """Toggle fold state for a group prefix, rebuild display pins."""
        current = self._fold_state.get(prefix, False)
        self._fold_state[prefix] = not current
        self._rebuild_display_pins(is_output=is_output)

    def toggle_title_fold(self):
        """Toggle title-level collapse: hides/shows all property rows."""
        self._title_collapsed = not self._title_collapsed
        self.titleCollapsed = self._title_collapsed
        self._write_expansion_state_to_usd()

    @property
    def is_title_collapsed(self):
        return self._title_collapsed

    def get_pin_documentation(self, pin_name):
        """Get the documentation string for a pin from its USD attribute."""
        if not self._prim:
            return ""
        prefixes = ("inputs:", "input:", "in:", "outputs:", "output:", "out:", "")
        for prefix in prefixes:
            attr = self._prim.GetAttribute(prefix + pin_name)
            if attr and attr.IsValid():
                doc = attr.GetDocumentation()
                if doc:
                    return doc
        return ""

    def resolve_pin_name(self, pin_name, is_output: bool | None = None):
        """Resolve a pin name through alias and folded maps.

        First translates bare USD pin names to schema-prefixed display names,
        then resolves folded children to their parent header.

        When ``is_output`` is provided, resolution stays on that side's alias
        and folded maps. This avoids collapsing an ``outputs:*`` endpoint
        through an ``inputs`` header (or vice versa) when both sides share the
        same bare child name such as ``translation``.
        """

        def _resolve_for_side(name: str, *, output: bool) -> str:
            alias_map = (
                self._output_pin_alias_map if output else self._input_pin_alias_map
            )
            # Folded child -> header maps are produced by the C++ layout pass.
            # The C++ accessor already returns a fresh dict, so no dict() copy.
            folded_map = self.foldedOutputPinMap if output else self.foldedInputPinMap
            aliased_name = alias_map.get(name)
            if aliased_name is not None:
                name = aliased_name
            return folded_map.get(name, name)

        if is_output is True:
            return _resolve_for_side(pin_name, output=True)
        if is_output is False:
            return _resolve_for_side(pin_name, output=False)

        aliased = self._input_pin_alias_map.get(pin_name)
        if aliased is None:
            aliased = self._output_pin_alias_map.get(pin_name)
        if aliased is not None:
            pin_name = aliased
        resolved = self.foldedInputPinMap.get(pin_name)
        if resolved is not None:
            return resolved
        return self.foldedOutputPinMap.get(pin_name, pin_name)
