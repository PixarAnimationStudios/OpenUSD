#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict
from __future__ import annotations

"""
USD Typed Prim Library Base Class.

Provides default implementations for creating, describing, and connecting
standard USD typed prims. Libraries for specific prim domains (
Graph Composition, Asset Data, generic USD Prims) inherit from this base
and only need to define type discovery and ownership.
"""

from pxr import Gf, Sdf, Tf, Usd

from .._schema_pin_names import populate_descriptor_pins
from ..nodeLibraries import NodeLibrary
from ..pinUtils import collect_links_for_prim, direction_hint_candidate_names


def _remove_matching_connections(
    attr: Usd.Attribute | None,
    other_prim_path: Sdf.Path,
    port_name: str,
) -> bool:
    """Remove authored connections on `attr` that target `other_prim_path` with
    a property name matching `port_name` in any spelling: bare, every input hint
    (``inputs:``/``input:``/``in:``), every output hint
    (``outputs:``/``output:``/``out:``), or the literal namespaced name.

    Returns True if any connection was removed.
    """
    if attr is None or not attr.IsValid():
        return False
    variants = (
        *direction_hint_candidate_names(port_name, is_input=True),
        *direction_hint_candidate_names(port_name, is_input=False),
    )
    removed_any = False
    for conn_path in list(attr.GetConnections()):
        if conn_path.GetPrimPath() != other_prim_path:
            continue
        if conn_path.name in variants:
            attr.RemoveConnection(conn_path)
            removed_any = True
    return removed_any


def _delete_relationship_target(
    rel_owner_prim,
    rel_name_hint: str,
    rel_name_default: str,
    endpoint_path: Sdf.Path,
    log_prefix: str,
) -> bool:
    """Remove one target from a relationship; True if removed, False if not authored."""
    rel = rel_owner_prim.GetRelationship(rel_name_hint or rel_name_default)
    if not (rel and rel.IsValid()):
        return False
    targets = list(rel.GetTargets())
    if endpoint_path not in targets:
        return False
    targets.remove(endpoint_path)
    rel.SetTargets(targets)
    Tf.Status(
        f"{log_prefix}: Removed relationship target "
        f"{endpoint_path} from {rel.GetPath()}"
    )
    return True


def _delete_attribute_connection(
    source_prim,
    source_port: str,
    target_prim,
    target_port: str,
    log_prefix: str,
) -> bool:
    """Strip authored connections from every hint-prefixed and bare attribute variant."""
    target_attrs = [
        target_prim.GetAttribute(name)
        for name in direction_hint_candidate_names(target_port, is_input=True)
    ]
    source_attrs = [
        source_prim.GetAttribute(name)
        for name in direction_hint_candidate_names(source_port, is_input=False)
    ]
    source_prim_path = source_prim.GetPath()
    target_prim_path = target_prim.GetPath()
    removed = False
    for tattr in target_attrs:
        if _remove_matching_connections(tattr, source_prim_path, source_port):
            removed = True
    for sattr in source_attrs:
        if _remove_matching_connections(sattr, target_prim_path, target_port):
            removed = True
    if removed:
        Tf.Status(
            f"{log_prefix}: Deleted connection "
            f"{source_prim_path}.{source_port} -> "
            f"{target_prim_path}.{target_port}"
        )
    return removed


class UsdTypedPrimLibraryBase(NodeLibrary):
    """
    Base class for libraries that handle standard USD typed prims.

    Provides default implementations for node creation, descriptor generation,
    connection management, and link discovery. Subclasses define which prim
    types they own via get_families/get_node_types/can_handle_prim.
    """

    def create_node(
        self,
        stage: Usd.Stage,
        parent_path: Sdf.Path,
        identifier: str,
        position: Gf.Vec2d,
    ) -> Sdf.Path | None:
        """Create a new USD typed prim node."""
        try:
            from ..primAuthoring import PrimAuthor

            parent_path_str = str(parent_path)
            unique_name = PrimAuthor.generate_unique_prim_name(
                stage, parent_path_str, identifier
            )

            if parent_path_str.endswith("/"):
                prim_path = Sdf.Path(f"{parent_path_str}{unique_name}")
            else:
                prim_path = Sdf.Path(f"{parent_path_str}/{unique_name}")

            layer = stage.GetEditTarget().GetLayer()
            if not layer:
                Tf.Warn("No edit target layer found")
                return None

            prim_spec = Sdf.CreatePrimInLayer(layer, prim_path)
            if not prim_spec:
                Tf.Warn(f"Failed to create prim spec at {prim_path}")
                return None

            prim_spec.specifier = Sdf.SpecifierDef
            prim_spec.typeName = identifier

            position_scale = 1.0 / 1000.0
            scaled_pos = Gf.Vec2f(
                position[0] * position_scale, position[1] * position_scale
            )

            pos_attr_spec = Sdf.AttributeSpec(
                prim_spec, "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
            if pos_attr_spec:
                pos_attr_spec.default = scaled_pos

            Tf.Status(f"Created USD prim '{identifier}' at {prim_path}")
            return prim_path

        except (Tf.ErrorException, RuntimeError, TypeError) as e:
            Tf.Warn(f"Error creating USD prim node: {e}")
            return None

    def create_node_descriptor_from_prim(
        self, prim: Usd.Prim, stage: Usd.Stage
    ) -> dict | None:
        """Create a node descriptor by reading prim attributes."""
        if not self.can_handle_prim(prim):
            return None

        # Lazy import: NoodlesConfig pulls in noodlesSettings → pxr.Usdviewq → Qt.
        # Deferring the import keeps nodeLibs importable in headless environments.
        from ..noodlesConfig import NoodlesConfig

        descriptor = {
            "primPath": prim.GetPath(),
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
            "uiStyle": NoodlesConfig.get("nodeRendererType", "default"),
            "renderer": None,
        }
        populate_descriptor_pins(prim, descriptor)
        return descriptor

    def get_prim_ui_style(self, prim: Usd.Prim) -> str | None:
        """Return renderer style for handled prims."""
        if self.can_handle_prim(prim):
            from ..noodlesConfig import NoodlesConfig

            return NoodlesConfig.get("nodeRendererType", "default")
        return None

    def create_connection(
        self,
        stage,
        source_prim,
        source_port,
        target_prim,
        target_port,
        *,
        source_property_name="",
        target_property_name="",
    ):
        """Create connection using input-side authoring convention.

        Relationship targets use read-modify-write with SetTargets to avoid
        Sdf list op conflicts between AddTarget and RemoveTarget that break
        undo (deletedItems from RemoveTarget persist and suppress targets
        re-added via AddTarget's prependedItems).

        Only writes to relationships that already exist on the prim (either
        from the USD schema or prior authoring). Auto-creation is intentionally
        omitted: the graph editor guarantees relationship pins are backed by a
        real USD relationship before a connection is attempted.
        """
        try:
            source_rel_name = source_property_name or source_port
            if source_rel_name:
                source_rel = source_prim.GetRelationship(source_rel_name)
                if source_rel and source_rel.IsValid():
                    relationship_target = self._resolve_endpoint_path(
                        target_prim,
                        target_port,
                        target_property_name,
                        is_input=True,
                    )
                    targets = list(source_rel.GetTargets())
                    if relationship_target not in targets:
                        targets.append(relationship_target)
                    source_rel.SetTargets(targets)
                    Tf.Status(
                        f"{self.get_name()}: Added relationship target "
                        f"{source_rel.GetPath()} -> {relationship_target}"
                    )
                    return True

            target_rel_name = target_property_name or target_port
            if target_rel_name:
                target_rel = target_prim.GetRelationship(target_rel_name)
                if target_rel and target_rel.IsValid():
                    relationship_target = self._resolve_endpoint_path(
                        source_prim,
                        source_port,
                        source_property_name,
                        is_input=False,
                    )
                    targets = list(target_rel.GetTargets())
                    if relationship_target not in targets:
                        targets.append(relationship_target)
                    target_rel.SetTargets(targets)
                    Tf.Status(
                        f"{self.get_name()}: Added relationship target "
                        f"{relationship_target} -> {target_rel.GetPath()}"
                    )
                    return True

            target_attr = self._resolve_attr(target_prim, target_port, is_input=True)
            source_attr = self._resolve_attr(source_prim, source_port, is_input=False)

            if target_attr and source_attr:
                target_attr.AddConnection(source_attr.GetPath())
                Tf.Status(
                    f"{self.get_name()}: Created connection "
                    f"{source_attr.GetPath()} -> {target_attr.GetPath()}"
                )
                return True
            return False

        except (Tf.ErrorException, RuntimeError, TypeError) as e:
            Tf.Warn(f"{self.get_name()}: Failed to create connection: {e}")
            return False

    def delete_connection(
        self,
        stage,
        source_prim,
        source_port,
        target_prim,
        target_port,
        *,
        source_property_name="",
        target_property_name="",
    ):
        """Delete connection across authoring side and prefix-variant ambiguity.

        USD attribute paths in connection lists are matched literally, but the
        authoring side (input vs output) and prefix form (e.g. ``outputs:foo``
        vs bare ``foo``) varies between scenes. Enumerate authored connections
        on both prefixed and bare attribute variants on each side, removing
        any whose path matches the other prim with a port-name variant of the
        requested port.
        """
        try:
            source_rel_name = source_property_name or source_port
            if source_rel_name:
                source_rel = source_prim.GetRelationship(source_rel_name)
                if source_rel and source_rel.IsValid():
                    endpoint = self._resolve_endpoint_path(
                        target_prim, target_port, target_property_name, is_input=True
                    )
                    return _delete_relationship_target(
                        source_prim,
                        source_property_name,
                        source_port,
                        endpoint,
                        self.get_name(),
                    )

            target_rel_name = target_property_name or target_port
            if target_rel_name:
                target_rel = target_prim.GetRelationship(target_rel_name)
                if target_rel and target_rel.IsValid():
                    endpoint = self._resolve_endpoint_path(
                        source_prim, source_port, source_property_name, is_input=False
                    )
                    return _delete_relationship_target(
                        target_prim,
                        target_property_name,
                        target_port,
                        endpoint,
                        self.get_name(),
                    )

            return _delete_attribute_connection(
                source_prim, source_port, target_prim, target_port, self.get_name()
            )

        except (Tf.ErrorException, RuntimeError, TypeError) as e:
            Tf.Warn(f"{self.get_name()}: Failed to delete connection: {e}")
            return False

    def can_connect(
        self,
        source_prim,
        source_port,
        source_is_output,
        target_prim,
        target_port,
        target_is_output,
    ):
        """Validate connection direction and attribute existence."""
        target_rel = target_prim.GetRelationship(target_port)
        source_rel = source_prim.GetRelationship(source_port)
        if (target_rel and target_rel.IsValid()) or (
            source_rel and source_rel.IsValid()
        ):
            # USD relationships are directionless; the graph editor enforces
            # input/output sides before calling can_connect, so direction is
            # not re-validated here.
            return bool(source_port or target_port)

        if source_is_output == target_is_output:
            return False

        source_attr = self._resolve_attr(
            source_prim, source_port, is_input=not source_is_output
        )
        target_attr = self._resolve_attr(
            target_prim, target_port, is_input=not target_is_output
        )
        return source_attr is not None and target_attr is not None

    def get_links_for_prim(self, prim: Usd.Prim) -> list[dict]:
        """Get all connections for a prim as link data."""
        if not self.can_handle_prim(prim):
            return []
        return collect_links_for_prim(prim)

    @staticmethod
    def _resolve_attr(prim, port_name, is_input):
        """Resolve a port name to a USD attribute.

        Tries each direction-hint spelling for the side (``inputs:``/``input:``/
        ``in:`` for inputs, ``outputs:``/``output:``/``out:`` for outputs) and
        finally the bare/literal name (which also covers namespaced pins such as
        ``rig1:space`` whose full name is kept).
        """
        if not port_name:
            return None
        for name in direction_hint_candidate_names(port_name, is_input=is_input):
            attr = prim.GetAttribute(name)
            if attr.IsValid():
                return attr
        return None

    @classmethod
    def _resolve_endpoint_path(
        cls,
        prim,
        port_name,
        property_name,
        *,
        is_input,
    ):
        if not property_name and not port_name:
            return prim.GetPath()

        resolved_property_name = property_name
        if not resolved_property_name:
            attr = cls._resolve_attr(prim, port_name, is_input)
            if attr and attr.IsValid():
                resolved_property_name = attr.GetName()

        if not resolved_property_name:
            relationship = prim.GetRelationship(port_name)
            if relationship and relationship.IsValid():
                resolved_property_name = relationship.GetName()

        if not resolved_property_name:
            return prim.GetPath()

        attr = prim.GetAttribute(resolved_property_name)
        if attr and attr.IsValid():
            return attr.GetPath()

        relationship = prim.GetRelationship(resolved_property_name)
        if relationship and relationship.IsValid():
            return relationship.GetPath()

        return prim.GetPath().AppendProperty(resolved_property_name)


# Register UsdTypedPrimLibraryBase as a TfType so that subclasses declared in
# plugInfo.json with this base can be discovered via Plug.Registry.
Tf.Type.Define(UsdTypedPrimLibraryBase)
