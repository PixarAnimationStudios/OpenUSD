#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict

"""
End-to-end tests for external NodeLibrary plugin discovery.

Validates the primary value proposition of the dynamic registry:
an external developer can create a NodeLibrary subclass + plugInfo.json,
register the directory with Plug.Registry, and have their library appear
in NodeLibraryRegistry.discover() with correct priority ordering.

The test writes a minimal plugin (Python module + plugInfo.json) to a
temp directory at runtime, registers it via Plug.Registry().RegisterPlugins(),
and verifies the full discover() pipeline finds it alongside built-in libraries.
"""

import json
import os
import shutil
import sys
import tempfile
import textwrap
import unittest

try:
    from pxr import Plug
    from pxr.UsdNoodles.nodeLibraries import NodeLibrary
    from pxr.UsdNoodles.nodeLibs.registry import NodeLibraryRegistry

    _has_pxr = True
except ImportError:
    _has_pxr = False

# plugInfo.json for the test plugin.
# Declares a single NodeLibrary subclass with priority 75 (above the default
# 100 and the generic UsdPrimLibrary catch-all at 1000).
_EXTERNAL_PLUGIN_INFO = {
    "Plugins": [
        {
            "Type": "python",
            "Name": "e2e_external_library",
            "Info": {
                "Types": {
                    "e2e_external_library.E2EExternalLibrary": {
                        "bases": ["pxr.UsdNoodles.nodeLibraries.NodeLibrary"],
                        "displayName": "E2E External",
                        "priority": 75,
                    }
                }
            },
        }
    ]
}

# Minimal NodeLibrary subclass that implements all abstract methods.
_EXTERNAL_MODULE_SOURCE = textwrap.dedent("""\
    from pxr import Tf
    from pxr.UsdNoodles.nodeLibraries import NodeLibrary


    class E2EExternalLibrary(NodeLibrary):

        def get_name(self):
            return "E2E External"

        def get_families(self):
            return ["E2EFamily"]

        def get_node_types(self, family):
            if family not in self.get_families():
                return []
            return [{"name": "E2ENode", "identifier": "E2ENode", "family": family}]

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


    Tf.Type.Define(E2EExternalLibrary)
""")


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class ExternalPluginE2ETest(unittest.TestCase):
    """End-to-end: external NodeLibrary discovered via plugInfo.json + Plug.Registry.

    Simulates the external developer workflow:
      1. Write a NodeLibrary subclass + call Tf.Type.Define()
      2. Write a plugInfo.json with bases/displayName/priority metadata
      3. Register the directory with Plug.Registry
      4. Call discover() and verify the library appears correctly
    """

    _tmpdir: str = ""

    @classmethod
    def setUpClass(cls) -> None:
        cls._tmpdir = tempfile.mkdtemp(prefix="noodles_e2e_plugin_")

        with open(os.path.join(cls._tmpdir, "plugInfo.json"), "w") as f:
            json.dump(_EXTERNAL_PLUGIN_INFO, f, indent=4)
            f.write("\n")

        with open(os.path.join(cls._tmpdir, "e2e_external_library.py"), "w") as f:
            f.write(_EXTERNAL_MODULE_SOURCE)

        # Make the module importable (USD plugin.Load() will import it)
        if cls._tmpdir not in sys.path:
            sys.path.insert(0, cls._tmpdir)

        # Register with USD's Plug system
        registered = Plug.Registry().RegisterPlugins(cls._tmpdir)
        assert len(registered) > 0, f"No plugins registered from {cls._tmpdir}"

    @classmethod
    def tearDownClass(cls) -> None:
        if cls._tmpdir in sys.path:
            sys.path.remove(cls._tmpdir)
        sys.modules.pop("e2e_external_library", None)

        if cls._tmpdir and os.path.isdir(cls._tmpdir):
            shutil.rmtree(cls._tmpdir, ignore_errors=True)

    def setUp(self) -> None:
        NodeLibraryRegistry.clear_manual()

    def tearDown(self) -> None:
        NodeLibraryRegistry.clear_manual()

    # -- Discovery tests -------------------------------------------------------

    def test_external_library_discovered_via_plug_registry(self) -> None:
        """External plugin should appear in discover() results."""
        libraries = NodeLibraryRegistry().discover()
        names = [lib.get_name() for lib in libraries]
        self.assertIn(
            "E2E External",
            names,
            "External plugin 'E2E External' not found via plugInfo.json discovery",
        )

    def test_external_library_is_node_library_instance(self) -> None:
        """Discovered external library should be a NodeLibrary instance."""
        libraries = NodeLibraryRegistry().discover()
        ext = [lib for lib in libraries if lib.get_name() == "E2E External"]
        self.assertEqual(len(ext), 1)
        self.assertIsInstance(ext[0], NodeLibrary)

    def test_external_library_coexists_with_builtins(self) -> None:
        """External library should appear alongside built-in libraries."""
        libraries = NodeLibraryRegistry().discover()
        names = {lib.get_name() for lib in libraries}
        self.assertIn("USD Prims", names, "Built-in USD Prims library missing")
        self.assertIn("E2E External", names, "External library missing")
        # After collapsing to a single generic built-in (USD Prims, the
        # catch-all at priority 1000), "coexistence" means the catch-all plus
        # the external library — at least two distinct libraries.
        self.assertGreaterEqual(
            len(names),
            2,
            "Expected the USD Prims catch-all plus the external library",
        )

    def test_no_duplicate_from_rediscovery(self) -> None:
        """Multiple discover() calls should not produce duplicates."""
        registry = NodeLibraryRegistry()
        first = [lib.get_name() for lib in registry.discover()]
        second = [lib.get_name() for lib in registry.discover()]

        first_ext_count = first.count("E2E External")
        second_ext_count = second.count("E2E External")
        self.assertEqual(first_ext_count, 1, "Duplicate external library in first call")
        self.assertEqual(
            second_ext_count, 1, "Duplicate external library in second call"
        )

    # -- Priority tests --------------------------------------------------------

    def test_external_library_priority_from_pluginfo(self) -> None:
        """Priority 75 from plugInfo.json should place the external library
        before the generic UsdPrimLibrary catch-all (priority 1000)."""
        libraries = NodeLibraryRegistry().discover()
        names = [lib.get_name() for lib in libraries]

        self.assertIn("E2E External", names, "E2E External library must be discovered")
        self.assertIn(
            "USD Prims", names, "Built-in USD Prims library must be discovered"
        )

        self.assertLess(
            names.index("E2E External"),
            names.index("USD Prims"),
            "E2E External (75) should come before USD Prims (1000)",
        )

    def test_external_library_precedes_catchall(self) -> None:
        """External library (priority 75) precedes the generic UsdPrimLibrary
        catch-all (priority 1000), so it claims its own types first."""
        libraries = NodeLibraryRegistry().discover()
        names = [lib.get_name() for lib in libraries]
        if "E2E External" in names and "USD Prims" in names:
            self.assertLess(
                names.index("E2E External"),
                names.index("USD Prims"),
                "External library should precede the generic catch-all",
            )

    # -- Functional tests ------------------------------------------------------

    def test_external_library_get_families(self) -> None:
        """External library should return its declared families."""
        libraries = NodeLibraryRegistry().discover()
        ext = [lib for lib in libraries if lib.get_name() == "E2E External"]
        self.assertEqual(len(ext), 1)
        self.assertEqual(ext[0].get_families(), ["E2EFamily"])

    def test_external_library_get_node_types(self) -> None:
        """External library should return node types for its families."""
        libraries = NodeLibraryRegistry().discover()
        ext = [lib for lib in libraries if lib.get_name() == "E2E External"]
        self.assertEqual(len(ext), 1)

        node_types = ext[0].get_node_types("E2EFamily")
        self.assertEqual(len(node_types), 1)
        self.assertEqual(node_types[0]["name"], "E2ENode")
        self.assertEqual(node_types[0]["identifier"], "E2ENode")
        self.assertEqual(node_types[0]["family"], "E2EFamily")

    def test_external_library_unknown_family_returns_empty(self) -> None:
        """Querying a non-existent family should return an empty list."""
        libraries = NodeLibraryRegistry().discover()
        ext = [lib for lib in libraries if lib.get_name() == "E2E External"]
        self.assertEqual(len(ext), 1)
        self.assertEqual(ext[0].get_node_types("NonExistentFamily"), [])

    # -- Manual registration does not override plug discovery ------------------

    def test_manual_register_does_not_override_external_plugin(self) -> None:
        """Programmatic registration should not replace a plugInfo.json-discovered library."""
        # Register a fake library with the same class name
        stub = type(
            "E2EExternalLibrary",
            (NodeLibrary,),
            {
                "get_name": lambda self: "FAKE_SHOULD_NOT_APPEAR",
                "get_families": lambda self: [],
                "get_node_types": lambda self, f: [],
                "create_node": lambda self, s, p, i, pos: None,
                "can_handle_prim": lambda self, p: False,
                "create_node_descriptor_from_prim": lambda self, p, s: None,
                "get_prim_ui_style": lambda self, p: None,
                "create_connection": lambda self, s, sp, spo, tp, tpo: False,
                "delete_connection": lambda self, s, sp, spo, tp, tpo: False,
                "can_connect": lambda self, sp, spo, sio, tp, tpo, tio: False,
                "get_links_for_prim": lambda self, p: [],
            },
        )
        NodeLibraryRegistry.register(stub, priority=999)

        libraries = NodeLibraryRegistry().discover()
        ext = [lib for lib in libraries if type(lib).__name__ == "E2EExternalLibrary"]
        self.assertGreater(
            len(ext), 0, "Expected to find E2EExternalLibrary in discover() results"
        )
        self.assertNotEqual(
            ext[0].get_name(),
            "FAKE_SHOULD_NOT_APPEAR",
            "Manual registration should not override plug registry entry",
        )
