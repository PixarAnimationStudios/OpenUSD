#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
USD Prim Node Library Implementation.

Unified library for all standard USD prim types, including UsdShade shaders.
Discovers concrete typed prim schemas from UsdSchemaRegistry at runtime via the
C++ UsdPrimRegistry, and queries the Sdr (Shader Definition Registry) for
shader-specific node types and port introspection.

Standard USD plugin families (usdGeom, usdLux, usdMedia, usdRender, usdShade,
usdSkel, usdUI, usdVol, usdProc, usdPhysics) plus any other concrete typed prim
are all handled here: this is the generic, lowest-precedence catch-all. External
plugin libraries (discovered via plugInfo.json) still claim their own types first.
"""

from __future__ import annotations

from pxr import Gf, Sdf, Tf, Usd

from .._schema_pin_names import populate_descriptor_pins
from ..pinUtils import collect_links_for_prim
from .usdTypedPrimBase import UsdTypedPrimLibraryBase


def _get_all_shader_nodes(registry: object) -> list:
    """Get all shader node objects from the Sdr registry.

    Uses GetShaderNodesByFamily() which is the standard Sdr.Registry API
    for enumerating all registered shader nodes.

    Returns:
        List of Sdr shader node objects.
    """
    if hasattr(registry, "GetShaderNodesByFamily"):
        try:
            nodes = registry.GetShaderNodesByFamily()
            if nodes:
                return list(nodes)
        except TypeError:
            # Some versions require an explicit family argument
            pass

    return []


def _read_noodles_config(key: str, default: object) -> object:
    """Read a NoodlesConfig value, tolerating headless environments.

    ``NoodlesConfig`` imports Qt (``pxr.Usdviewq.qt``) at module load. Node and
    descriptor creation must work without a GUI (unit tests, batch tooling), so
    when that import is unavailable fall back to the caller's default.
    """
    try:
        from ..noodlesConfig import NoodlesConfig
    except ImportError:
        return default

    return NoodlesConfig.get(key, default)


class UsdPrimLibrary(UsdTypedPrimLibraryBase):
    """
    Unified node library for standard USD prim types and UsdShade shaders.

    Discovers standard USD concrete typed prim schemas from UsdSchemaRegistry
    at runtime via the C++ UsdPrimRegistry. For Shader prims, additionally
    queries the Sdr (Shader Definition Registry) for port definitions and
    shader identifiers.

    Uses the C++ backend for performance-critical operations on non-shader
    prims (schema enumeration, connection reading). For shader prims, falls
    back to Sdr-based introspection for richer port discovery.
    """

    def __init__(self, enabled: bool = True) -> None:
        super().__init__(enabled)
        self._registry: object | None = None
        self._discovery_failed: bool = False
        self._sdr_registry: object | None = None
        self._cached_sdr_families: list[str] | None = None
        self._cached_sdr_nodes: dict[str, list[dict[str, str]]] = {}

    def _get_registry(self) -> object | None:
        """Lazy-load the C++ UsdPrimRegistry."""
        if self._registry is None and not self._discovery_failed:
            try:
                from pxr.UsdNoodles._usdNoodles import UsdPrimRegistry

                self._registry = UsdPrimRegistry()
            except ImportError as e:
                Tf.Warn(
                    f"UsdPrimLibrary: Failed to import UsdPrimRegistry: {e}. "
                    "C++ prim discovery will be disabled."
                )
                self._discovery_failed = True
        return self._registry

    def _get_sdr_registry(self) -> object:
        """Lazy-load the Sdr (Shader Definition Registry)."""
        if self._sdr_registry is None:
            from pxr import Sdr

            self._sdr_registry = Sdr.Registry()
        return self._sdr_registry

    def get_name(self) -> str:
        return "USD Prims"

    def get_families(self) -> list[str]:
        """Return C++ UsdPrimRegistry families merged with Sdr shader families."""
        families: set[str] = set()

        # C++ families (usdGeom, usdLux, usdShade, etc.)
        registry = self._get_registry()
        if registry is not None:
            families.update(registry.GetFamilies())

        # Sdr shader families (UsdPreviewSurface, MaterialX, etc.)
        try:
            sdr = self._get_sdr_registry()
            for sdr_node in _get_all_shader_nodes(sdr):
                source_type = sdr_node.GetSourceType()
                if source_type:
                    families.add(source_type)
        except Exception:
            pass

        return sorted(families)

    def get_node_types(self, family: str) -> list[dict[str, str]]:
        """Return node types for a family, from C++ registry or Sdr."""
        # Try C++ registry first (covers standard USD typed prims)
        registry = self._get_registry()
        if registry is not None:
            cpp_types = [
                {
                    "name": t.typeName,
                    "identifier": t.typeName,
                    "family": t.family,
                }
                for t in registry.GetPrimTypes(family)
            ]
            if cpp_types:
                return cpp_types

        # Try Sdr (covers shader source-type families)
        if family in self._cached_sdr_nodes:
            return self._cached_sdr_nodes[family]

        try:
            sdr = self._get_sdr_registry()
            node_types = []
            for sdr_node in _get_all_shader_nodes(sdr):
                if sdr_node.GetSourceType() != family:
                    continue
                node_name = sdr_node.GetName()
                identifier = sdr_node.GetIdentifier()
                if not identifier:
                    identifier = node_name
                node_types.append(
                    {"name": node_name, "identifier": identifier, "family": family}
                )
            self._cached_sdr_nodes[family] = node_types
            return node_types
        except Exception:
            return []

    def can_handle_prim(self, prim: Usd.Prim) -> bool:
        # Generic catch-all: handle every valid prim. Registered at the lowest
        # precedence so any external plugInfo.json library still claims its own
        # types first (first-match-by-priority in the factory/registry).
        return bool(prim and prim.IsValid())

    def create_node(
        self,
        stage: Usd.Stage,
        parent_path: Sdf.Path,
        identifier: str,
        position: Gf.Vec2d,
    ) -> Sdf.Path | None:
        """Create a USD typed prim or shader node.

        Concrete schema types (e.g. ``GeometryLight``, ``DomeLight``) take
        precedence over Sdr shader lookup. Several UsdLux light types are
        registered both as concrete typed schemas AND in the shader
        definition registry; without this precedence, the hotbox would
        create a ``Shader`` prim with ``info:id=GeometryLight`` instead of
        a proper typed ``GeometryLight``, causing pin discovery to see a
        shader with no schema-declared inputs.

        For identifiers that are *not* concrete schemas but *are* registered
        in Sdr (real shaders like ``UsdPreviewSurface``), create a
        ``UsdShade.Shader`` prim with the proper ``info:id`` attribute.
        Otherwise fall back to generic typed-prim creation.
        """
        schema_reg = Usd.SchemaRegistry()
        if schema_reg.FindConcretePrimDefinition(identifier) is not None:
            prim_path = super().create_node(stage, parent_path, identifier, position)
            if prim_path:
                self._apply_auto_api_schemas(stage.GetPrimAtPath(prim_path))
            return prim_path

        # Not a concrete typed schema — check Sdr for shader identifiers.
        try:
            sdr = self._get_sdr_registry()
            sdr_node = sdr.GetShaderNodeByIdentifier(identifier)
            if sdr_node:
                return self._create_shader_node(
                    stage, parent_path, identifier, position, sdr_node
                )
        except Exception:
            pass

        # Fall back to generic typed prim creation (may still succeed for
        # types the schema registry discovers lazily).
        return super().create_node(stage, parent_path, identifier, position)

    @staticmethod
    def _apply_auto_api_schemas(prim: Usd.Prim) -> None:
        """Apply companion API schemas that the prim type expects.

        In USD 24.11, ShadowAPI and ShapingAPI are standalone schemas — they
        don't declare ``apiSchemaAutoApplyTo`` and are not built-in on light
        types. We apply them explicitly so light nodes surface shadow and
        shaping pins. Extend the list below for other prim families.
        """
        applied = {str(s) for s in prim.GetAppliedSchemas()}
        if not any("LightAPI" in s for s in applied):
            return

        try:
            from pxr import UsdLux
        except ImportError:
            return

        for schema_cls in (UsdLux.ShadowAPI, UsdLux.ShapingAPI):
            try:
                schema_cls.Apply(prim)
            except (AttributeError, RuntimeError):
                pass

    def _create_shader_node(
        self,
        stage: Usd.Stage,
        parent_path: Sdf.Path,
        identifier: str,
        position: Gf.Vec2d,
        sdr_node: object,
    ) -> Sdf.Path | None:
        """Create a UsdShade.Shader prim with info:id from Sdr."""
        try:
            from ..primAuthoring import PrimAuthor

            shader_id = sdr_node.GetIdentifier()
            shader_name = sdr_node.GetName()

            unique_name = PrimAuthor.generate_unique_prim_name(
                stage, parent_path, shader_name
            )

            parent_str = str(parent_path)
            if parent_str.endswith("/"):
                shader_path = Sdf.Path(f"{parent_str}{unique_name}")
            else:
                shader_path = Sdf.Path(f"{parent_str}/{unique_name}")

            layer = stage.GetEditTarget().GetLayer()
            if not layer:
                Tf.Warn("No edit target layer found")
                return None

            prim_spec = Sdf.CreatePrimInLayer(layer, shader_path)
            if not prim_spec:
                Tf.Warn(f"Failed to create prim spec at {shader_path}")
                return None

            prim_spec.typeName = "Shader"
            prim_spec.specifier = Sdf.SpecifierDef

            id_attr_spec = Sdf.AttributeSpec(
                prim_spec, "info:id", Sdf.ValueTypeNames.Token
            )
            if id_attr_spec:
                id_attr_spec.default = shader_id

            position_scale = 1.0 / 1000.0
            scaled_pos = Gf.Vec2f(
                position[0] * position_scale, position[1] * position_scale
            )

            pos_attr_spec = Sdf.AttributeSpec(
                prim_spec, "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
            if pos_attr_spec:
                pos_attr_spec.default = scaled_pos

            Tf.Status(
                f"Created shader '{shader_name}' (id: {shader_id}) at {shader_path}"
            )
            return shader_path

        except (Tf.ErrorException, RuntimeError, TypeError) as e:
            Tf.Warn(f"Error creating shader node: {e}")
            return None

    def create_node_descriptor_from_prim(
        self, prim: Usd.Prim, stage: Usd.Stage
    ) -> dict[str, object] | None:
        """Create a node descriptor: Sdr for Shader prims, generic Python otherwise."""
        if not self.can_handle_prim(prim):
            return None

        if prim.GetTypeName() == "Shader":
            return self._shader_descriptor_from_prim(prim)

        # Non-shader: generic Python pin discovery, the single source of truth
        # shared with the renderer's get_schema_aware_pin_properties path.
        descriptor: dict[str, object] = {
            "primPath": prim.GetPath(),
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
            "uiStyle": _read_noodles_config("nodeRendererType", "default"),
            "renderer": None,
        }
        self._populate_descriptor_from_attributes(prim, descriptor)
        return descriptor

    def _shader_descriptor_from_prim(self, prim: Usd.Prim) -> dict[str, object]:
        """Build a descriptor for a Shader prim, querying Sdr for port definitions."""
        descriptor: dict[str, object] = {
            "primPath": prim.GetPath(),
            "inputPins": [],
            "outputPins": [],
            "inputPinTypes": {},
            "outputPinTypes": {},
            "uiStyle": _read_noodles_config("nodeRendererType", "default"),
            "renderer": None,
        }

        # Try Sdr-based introspection for richer port discovery
        shader_id_attr = prim.GetAttribute("info:id")
        if shader_id_attr and shader_id_attr.HasValue():
            shader_id = shader_id_attr.Get()
            try:
                sdr = self._get_sdr_registry()
                sdr_node = sdr.GetShaderNodeByIdentifier(shader_id)
                if sdr_node:
                    self._populate_descriptor_from_sdr(sdr_node, descriptor)
                    if descriptor["inputPins"] or descriptor["outputPins"]:
                        return descriptor
            except Exception as e:
                Tf.Warn(f"Failed to query Sdr for shader '{shader_id}': {e}")

        # Fall back to reading from USD attributes
        self._populate_descriptor_from_attributes(prim, descriptor)
        return descriptor

    @staticmethod
    def _populate_descriptor_from_sdr(
        sdr_node: object, descriptor: dict[str, object]
    ) -> None:
        """Populate descriptor input/output pins from Sdr shader definition."""
        UsdPrimLibrary._collect_sdr_ports(
            sdr_node,
            "Input",
            descriptor["inputPins"],
            descriptor["inputPinTypes"],
        )
        UsdPrimLibrary._collect_sdr_ports(
            sdr_node,
            "Output",
            descriptor["outputPins"],
            descriptor["outputPinTypes"],
        )

    @staticmethod
    def _collect_sdr_ports(
        sdr_node: object,
        direction: str,
        pins: list[str],
        pin_types: dict[str, str],
    ) -> None:
        """Collect port names and types from an Sdr node for a given direction."""
        try:
            names = getattr(sdr_node, f"Get{direction}Names")()
        except AttributeError:
            return

        getter = getattr(sdr_node, f"GetShader{direction}", None)
        if getter is None:
            return

        for name in names:
            try:
                sdr_prop = getter(name)
                if not sdr_prop:
                    continue
                pins.append(name)
                type_name = UsdPrimLibrary._extract_sdf_type_name(sdr_prop)
                if type_name:
                    pin_types[name] = type_name
            except Exception:
                pass

    @staticmethod
    def _extract_sdf_type_name(sdr_prop: object) -> str | None:
        """Extract the SdfType name string from an Sdr property, or None."""
        type_tuple = sdr_prop.GetTypeAsSdfType()
        if type_tuple and len(type_tuple) > 0 and type_tuple[0]:
            return str(type_tuple[0])
        return None

    @staticmethod
    def _populate_descriptor_from_attributes(
        prim: Usd.Prim, descriptor: dict[str, object]
    ) -> None:
        """Populate descriptor input/output pins from schema + authored attributes."""
        populate_descriptor_pins(
            prim,
            descriptor,
            check_port_type=True,
            hide_ui=bool(_read_noodles_config("hideUiNamespacePins", True)),
        )

    def get_links_for_prim(self, prim: Usd.Prim) -> list[dict[str, object]]:
        """Get all links via generic Python discovery (single source of truth)."""
        if not self.can_handle_prim(prim):
            return []
        return collect_links_for_prim(prim)


Tf.Type.Define(UsdPrimLibrary)
