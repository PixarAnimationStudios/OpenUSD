#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict

"""
Tests for NodeLibraryRegistry — dynamic library discovery via Plug.Registry.

Verifies that the registry discovers built-in libraries, respects priority
ordering, and supports programmatic registration.
"""

import unittest

try:
    from pxr import Plug, Tf
    from pxr.UsdNoodles.nodeLibraries import NodeLibrary
    from pxr.UsdNoodles.nodeLibs.registry import NodeLibraryRegistry

    _has_pxr = True
except ImportError:
    _has_pxr = False


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class NodeLibraryTfTypeTest(unittest.TestCase):
    """Verify NodeLibrary is registered as a discoverable TfType base."""

    def test_node_library_tf_type_exists(self) -> None:
        tf_type = Tf.Type.Find(NodeLibrary)
        self.assertTrue(tf_type, "NodeLibrary TfType not found")

    def test_derived_types_discoverable(self) -> None:
        base_type = Tf.Type.Find(NodeLibrary)
        derived = Plug.Registry.GetAllDerivedTypes(base_type)
        self.assertGreater(len(derived), 0, "No derived NodeLibrary types found")


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class NodeLibraryRegistryDiscoverTest(unittest.TestCase):
    """Verify the registry discovers and sorts libraries correctly."""

    def test_discover_returns_node_library_instances(self) -> None:
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        for lib in libraries:
            self.assertIsInstance(lib, NodeLibrary)

    def test_discover_finds_built_in_libraries(self) -> None:
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = {lib.get_name() for lib in libraries}
        self.assertIn("USD Prims", names, "Built-in library 'USD Prims' not discovered")
        self.assertGreaterEqual(
            len(libraries), 1, "Expected at least 1 built-in library"
        )

    def test_discover_returns_priority_sorted(self) -> None:
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        # USD Prims is the generic catch-all at the lowest precedence
        # (priority 1000), so it sorts last.
        if "USD Prims" in names:
            self.assertEqual(
                names[-1],
                "USD Prims",
                "USD Prims (priority 1000) should be the lowest-precedence entry",
            )

    def test_discover_no_duplicates(self) -> None:
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertEqual(len(names), len(set(names)), f"Duplicate libraries: {names}")


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class NodeLibraryRegistryManualTest(unittest.TestCase):
    """Verify programmatic registration via register()."""

    def setUp(self) -> None:
        NodeLibraryRegistry.clear_manual()

    def tearDown(self) -> None:
        NodeLibraryRegistry.clear_manual()

    def _make_stub_library(self, name: str):
        """Create a minimal concrete NodeLibrary subclass for testing."""

        class _StubLibrary(NodeLibrary):
            _name = name

            def get_name(self):
                return self._name

            def get_families(self):
                return []

            def get_node_types(self, family):
                return []

            def create_node(self, stage, parent_path, identifier, position):
                return None

            def can_handle_prim(self, prim):
                return False

            def create_node_descriptor_from_prim(self, prim, stage):
                return None

            def get_prim_ui_style(self, prim):
                return None

            def create_connection(
                self, stage, source_prim, source_port, target_prim, target_port
            ):
                return False

            def delete_connection(
                self, stage, source_prim, source_port, target_prim, target_port
            ):
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
                return False

            def get_links_for_prim(self, prim):
                return []

        # Give each stub a unique class name for the registry
        _StubLibrary.__name__ = f"_Stub_{name.replace(' ', '_')}"
        _StubLibrary.__qualname__ = _StubLibrary.__name__
        return _StubLibrary

    def test_register_and_discover(self) -> None:
        TestLib = self._make_stub_library("_TestManualLib")
        NodeLibraryRegistry.register(TestLib, priority=5)
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertIn("_TestManualLib", names)

    def test_manual_priority_respected(self) -> None:
        HighPriorityLib = self._make_stub_library("_HighPriority")
        NodeLibraryRegistry.register(HighPriorityLib, priority=1)
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertEqual(names[0], "_HighPriority", "Priority 1 should be first")

    def test_clear_manual(self) -> None:
        TempLib = self._make_stub_library("_TempLib")
        NodeLibraryRegistry.register(TempLib)
        NodeLibraryRegistry.clear_manual()
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertNotIn("_TempLib", names)

    def test_manual_does_not_override_builtin(self) -> None:
        """Programmatic registration should not override built-in libraries."""
        FakeUsdPrims = self._make_stub_library("SHOULD_NOT_APPEAR")
        FakeUsdPrims.__name__ = "UsdPrimLibrary"
        NodeLibraryRegistry.register(FakeUsdPrims, priority=999)
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        usd_prim_libs = [
            lib for lib in libraries if type(lib).__name__ == "UsdPrimLibrary"
        ]
        self.assertTrue(usd_prim_libs, "UsdPrimLibrary should still be present")
        self.assertNotEqual(
            usd_prim_libs[0].get_name(),
            "SHOULD_NOT_APPEAR",
            "Manual registration should not override built-in libraries",
        )

    def test_register_without_pluginfo(self) -> None:
        """Programmatic registration works for libraries not in plugInfo.json."""
        CustomLib = self._make_stub_library("Custom Nodes")
        NodeLibraryRegistry.register(CustomLib, priority=75)
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertIn("Custom Nodes", names)
        if "USD Prims" in names and "Custom Nodes" in names:
            self.assertLess(
                names.index("Custom Nodes"),
                names.index("USD Prims"),
                "Custom (75) should come before USD Prims (1000)",
            )


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class NodeLibraryRegistryPlugInfoTest(unittest.TestCase):
    """Verify plugInfo.json metadata is read correctly."""

    def test_built_in_libraries_are_all_discovered(self) -> None:
        """All built-in libraries should be present in discover() results."""
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = [lib.get_name() for lib in libraries]
        self.assertIn("USD Prims", names)

    def test_usd_prims_is_lowest_precedence(self) -> None:
        """UsdPrimLibrary is the generic catch-all at the highest priority
        number (lowest precedence), so it sorts last."""
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        if libraries:
            self.assertEqual(
                libraries[-1].get_name(),
                "USD Prims",
                "UsdPrimLibrary should be last (priority 1000)",
            )

    def test_discover_is_idempotent(self) -> None:
        """Multiple discover() calls should return the same results."""
        registry = NodeLibraryRegistry()
        first = [lib.get_name() for lib in registry.discover()]
        second = [lib.get_name() for lib in registry.discover()]
        self.assertEqual(first, second, "discover() should be idempotent")


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class NodeLibraryRegistryBuiltinFallbackTest(unittest.TestCase):
    """Verify built-in libraries are loaded via fallback when plugInfo.json misses them."""

    def test_fallback_populates_missing_builtins(self) -> None:
        """_discover_builtin_fallbacks should import libraries not already in entries."""
        registry = NodeLibraryRegistry()
        entries: dict = {}
        registry._discover_builtin_fallbacks(entries)
        self.assertIn("UsdPrimLibrary", entries)
        for name, (cls, _priority) in entries.items():
            self.assertTrue(
                issubclass(cls, NodeLibrary),
                f"{name} is not a NodeLibrary subclass",
            )

    def test_fallback_skips_already_discovered(self) -> None:
        """_discover_builtin_fallbacks should not overwrite entries already present."""
        registry = NodeLibraryRegistry()
        sentinel_priority = 999
        from pxr.UsdNoodles.nodeLibs.usdPrimLibrary import UsdPrimLibrary

        entries: dict = {
            "UsdPrimLibrary": (UsdPrimLibrary, sentinel_priority),
        }
        registry._discover_builtin_fallbacks(entries)
        self.assertEqual(
            entries["UsdPrimLibrary"][1],
            sentinel_priority,
            "Fallback should not overwrite already-discovered libraries",
        )

    def test_fallback_uses_correct_default_priorities(self) -> None:
        """Fallback-loaded libraries should use the default priorities from the table."""
        registry = NodeLibraryRegistry()
        entries: dict = {}
        registry._discover_builtin_fallbacks(entries)
        if "UsdPrimLibrary" in entries:
            self.assertEqual(entries["UsdPrimLibrary"][1], 1000)

    def test_discover_includes_builtins_regardless_of_plug_registry(self) -> None:
        """Full discover() should always include built-in libraries."""
        registry = NodeLibraryRegistry()
        libraries = registry.discover()
        names = {lib.get_name() for lib in libraries}
        self.assertIn("USD Prims", names)


if __name__ == "__main__":
    unittest.main()
