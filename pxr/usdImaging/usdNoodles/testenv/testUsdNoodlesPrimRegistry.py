#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict

"""
Unit tests for the pure-Python UsdPrimRegistry (nodeLibs/usdPrimRegistry.py).

Covers schema-type discovery against the real UsdSchemaRegistry as well as the
discovery branches (concrete filter, empty type-name skip, "Other" family
fallback, and per-family sorting) driven with mocked pxr registries so they
run deterministically without a USD stage.
"""

import unittest
from dataclasses import FrozenInstanceError
from unittest.mock import MagicMock, patch

try:
    from pxr.UsdNoodles.nodeLibs.usdPrimRegistry import PrimTypeInfo, UsdPrimRegistry

    _has_pxr = True
except ImportError:
    _has_pxr = False


_MODULE = "pxr.UsdNoodles.nodeLibs.usdPrimRegistry"


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class PrimTypeInfoTest(unittest.TestCase):
    """Verify the PrimTypeInfo value type."""

    def test_fields(self) -> None:
        info = PrimTypeInfo(typeName="Mesh", family="usdGeom")
        self.assertEqual(info.typeName, "Mesh")
        self.assertEqual(info.family, "usdGeom")

    def test_is_frozen(self) -> None:
        info = PrimTypeInfo(typeName="Mesh", family="usdGeom")
        with self.assertRaises(FrozenInstanceError):
            info.typeName = "Sphere"  # frozen dataclass


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class UsdPrimRegistryRealDiscoveryTest(unittest.TestCase):
    """Exercise discovery against the real pxr registries.

    This verifies the actual pxr API is called correctly (something the mocked
    tests above cannot catch). The pytest binary does not register every schema
    plugin, so schema-kind lookups post Tf errors for those types and discovery
    skips them; these tests therefore assert only structural invariants that
    hold whether or not any concrete type is discoverable. Content correctness
    is covered by UsdPrimRegistryDiscoveryBranchTest.
    """

    def test_discovery_does_not_raise(self) -> None:
        # Missing schema plugins must be tolerated, not fatal.
        self.assertIsInstance(UsdPrimRegistry().GetFamilies(), list)

    def test_families_are_sorted(self) -> None:
        families = UsdPrimRegistry().GetFamilies()
        self.assertEqual(families, sorted(families))

    def test_prim_types_sorted_and_tagged(self) -> None:
        registry = UsdPrimRegistry()
        for family in registry.GetFamilies():
            types = registry.GetPrimTypes(family)
            names = [info.typeName for info in types]
            self.assertEqual(
                names, sorted(names), f"Types in family '{family}' should be sorted"
            )
            for info in types:
                self.assertEqual(info.family, family)
                self.assertTrue(info.typeName)

    def test_unknown_family_returns_empty(self) -> None:
        self.assertEqual(UsdPrimRegistry().GetPrimTypes("NoSuchFamily"), [])

    def test_discovery_is_cached(self) -> None:
        """Repeated queries return equal results."""
        registry = UsdPrimRegistry()
        self.assertEqual(registry.GetFamilies(), registry.GetFamilies())


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class UsdPrimRegistryDiscoveryBranchTest(unittest.TestCase):
    """Drive _discover_types() with mocked pxr registries for branch coverage."""

    def _run_discovery(
        self,
        derived_types: list,
        is_concrete: dict,
        type_names: dict,
        plugins: dict,
    ) -> UsdPrimRegistry:
        """Build a registry whose discovery sees the given synthetic schema types.

        `is_concrete`/`type_names`/`plugins` are keyed by the sentinel objects in
        `derived_types`; a plugin value of None exercises the "Other" fallback.
        """
        base_type = MagicMock()
        base_type.GetAllDerivedTypes.return_value = derived_types

        mock_tf = MagicMock()
        mock_tf.Type.FindByName.return_value = base_type

        mock_usd = MagicMock()
        mock_usd.SchemaRegistry.IsConcrete.side_effect = lambda t: is_concrete[t]
        mock_usd.SchemaRegistry.GetSchemaTypeName.side_effect = lambda t: type_names[t]

        mock_plug_registry = MagicMock()
        mock_plug_registry.GetPluginForType.side_effect = lambda t: plugins[t]
        mock_plug = MagicMock()
        mock_plug.Registry.return_value = mock_plug_registry

        registry = UsdPrimRegistry()
        with (
            patch(f"{_MODULE}.Tf", mock_tf),
            patch(f"{_MODULE}.Usd", mock_usd),
            patch(f"{_MODULE}.Plug", mock_plug),
        ):
            registry._discover_types()
        return registry

    @staticmethod
    def _plugin(name: str) -> MagicMock:
        plugin = MagicMock()
        plugin.name = name
        return plugin

    def test_skips_non_concrete_and_empty_type_names(self) -> None:
        concrete, abstract, unnamed = object(), object(), object()
        geom = self._plugin("usdGeom")
        registry = self._run_discovery(
            derived_types=[concrete, abstract, unnamed],
            is_concrete={concrete: True, abstract: False, unnamed: True},
            type_names={concrete: "Mesh", abstract: "Imageable", unnamed: ""},
            plugins={concrete: geom, abstract: geom, unnamed: geom},
        )
        self.assertEqual(registry.GetFamilies(), ["usdGeom"])
        types = registry.GetPrimTypes("usdGeom")
        self.assertEqual([t.typeName for t in types], ["Mesh"])

    def test_plugin_none_uses_other_family(self) -> None:
        orphan = object()
        registry = self._run_discovery(
            derived_types=[orphan],
            is_concrete={orphan: True},
            type_names={orphan: "MysteryPrim"},
            plugins={orphan: None},
        )
        self.assertEqual(registry.GetFamilies(), ["Other"])
        self.assertEqual(
            registry.GetPrimTypes("Other"),
            [PrimTypeInfo(typeName="MysteryPrim", family="Other")],
        )

    def test_types_sorted_within_family(self) -> None:
        sphere, cube, mesh = object(), object(), object()
        geom = self._plugin("usdGeom")
        registry = self._run_discovery(
            derived_types=[sphere, cube, mesh],
            is_concrete={sphere: True, cube: True, mesh: True},
            type_names={sphere: "Sphere", cube: "Cube", mesh: "Mesh"},
            plugins={sphere: geom, cube: geom, mesh: geom},
        )
        self.assertEqual(
            [t.typeName for t in registry.GetPrimTypes("usdGeom")],
            ["Cube", "Mesh", "Sphere"],
        )

    def test_families_grouped_and_sorted(self) -> None:
        mesh, light = object(), object()
        registry = self._run_discovery(
            derived_types=[mesh, light],
            is_concrete={mesh: True, light: True},
            type_names={mesh: "Mesh", light: "DistantLight"},
            plugins={mesh: self._plugin("usdGeom"), light: self._plugin("usdLux")},
        )
        self.assertEqual(registry.GetFamilies(), ["usdGeom", "usdLux"])
        self.assertEqual(
            [t.typeName for t in registry.GetPrimTypes("usdLux")], ["DistantLight"]
        )

    def test_discovery_runs_only_once(self) -> None:
        mesh = object()
        base_type = MagicMock()
        base_type.GetAllDerivedTypes.return_value = [mesh]
        mock_tf = MagicMock()
        mock_tf.Type.FindByName.return_value = base_type
        mock_usd = MagicMock()
        mock_usd.SchemaRegistry.IsConcrete.return_value = True
        mock_usd.SchemaRegistry.GetSchemaTypeName.return_value = "Mesh"
        mock_plug = MagicMock()
        mock_plug.Registry.return_value.GetPluginForType.return_value = self._plugin(
            "usdGeom"
        )

        registry = UsdPrimRegistry()
        with (
            patch(f"{_MODULE}.Tf", mock_tf),
            patch(f"{_MODULE}.Usd", mock_usd),
            patch(f"{_MODULE}.Plug", mock_plug),
        ):
            registry.GetFamilies()
            registry.GetPrimTypes("usdGeom")
            registry.GetFamilies()

        # FindByName is the entry point of _discover_types; it must run once.
        mock_tf.Type.FindByName.assert_called_once_with("UsdSchemaBase")


if __name__ == "__main__":
    unittest.main()
