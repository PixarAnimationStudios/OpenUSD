#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
Node Library Registry — Dynamic discovery of NodeLibrary plugins.

Uses USD's Plug.Registry to discover NodeLibrary subclasses registered via
plugInfo.json, following the same pattern usdview uses for PluginContainer
discovery (see pxr/usdImaging/usdviewq/plugin.py).

External developers can add custom libraries by:
1. Creating a NodeLibrary subclass
2. Calling Tf.Type.Define(MyLibrary) in their module
3. Adding a plugInfo.json with {"bases": ["pxr.UsdNoodles.nodeLibraries.NodeLibrary"]}
4. Setting PXR_PLUGINPATH_NAME to include their plugin directory
"""

from __future__ import annotations

import importlib
import inspect
import pkgutil

from pxr import Plug, Tf

from ..nodeLibraries import NodeLibrary

# Default priority for libraries without explicit metadata.
# Custom external libraries default to this. The generic UsdPrimLibrary
# catch-all sits far below (priority 1000) so external libraries always
# claim their own types first via first-match-by-priority.
_DEFAULT_PRIORITY: int = 100

# Built-in libraries that should always be available.
# Used as a fallback when plugInfo.json discovery misses any of them.
# Format: (module_path_relative_to_this_package, class_name, default_priority)
#
# UsdPrimLibrary is the generic catch-all (it handles every valid prim type),
# so it is registered at the lowest precedence (priority 1000). Any external
# plugInfo.json library (default priority 100) still claims its own types first.
_BUILTIN_LIBRARIES: list[tuple[str, str, int]] = [
    (".usdPrimLibrary", "UsdPrimLibrary", 1000),
]


class NodeLibraryRegistry:
    """
    Discovers NodeLibrary plugins via USD's Plug system.

    Libraries are found via Plug.Registry.GetAllDerivedTypes() and sorted
    by priority metadata from their plugInfo.json. Lower priority values
    are registered first (higher precedence in first-match-wins dispatch).

    Also supports programmatic registration via register() for quick
    prototyping without a plugInfo.json.
    """

    # Process-global registry for programmatically registered libraries.
    # Entries persist for the lifetime of the Python process, similar to
    # Plug.Registry itself.  Use clear_manual() to reset (primarily for tests).
    #
    # Thread safety: _manual_registry is not protected by a lock. Both
    # register() and discover() must be called from the Qt main thread
    # (the same thread that creates GraphView).  This matches USD's own
    # Plug.Registry, which is also not thread-safe for mutations.
    _manual_registry: dict[str, tuple[type[NodeLibrary], int]] = {}

    @classmethod
    def register(
        cls, library_cls: type[NodeLibrary], priority: int = _DEFAULT_PRIORITY
    ) -> None:
        """Register a library class programmatically (no plugInfo.json needed).

        Args:
            library_cls: A NodeLibrary subclass to register.
            priority: Sort order (lower = higher precedence). Default 100.

        Note:
            Must be called from the Qt main thread (the same thread that
            calls ``discover()``).  Not thread-safe for concurrent access.

            Libraries are keyed by ``__name__``. If two classes share the same
            ``__name__`` the later registration silently wins. Give distinct
            class names to avoid collisions.
        """
        cls._manual_registry[library_cls.__name__] = (library_cls, priority)

    @classmethod
    def clear_manual(cls) -> None:
        """Clear all programmatically registered libraries (for testing)."""
        cls._manual_registry.clear()

    # Namespace packages scanned by discover() for auto-import.
    # Any BUCK target that places a Python package under one of these
    # namespaces will be imported automatically, triggering its __init__.py
    # registration. Add new namespaces here to extend discovery.
    _EXTENSION_NAMESPACES: list[str] = [
        "pxr.UsdNoodles.extensions",
    ]

    def discover(self) -> list[NodeLibrary]:
        """Discover all registered NodeLibrary subclasses and return sorted instances.

        Discovery sources (merged, deduplicated by class name):
        1. Plug.Registry — types declared in plugInfo.json files on PXR_PLUGINPATH_NAME
        2. Extension namespaces — auto-imported packages under pxr.UsdNoodles.extensions
        3. Built-in fallbacks — direct imports for libraries that should always be available
        4. Manual registry — types added via NodeLibraryRegistry.register()

        Built-in fallbacks run before the manual registry so that programmatic
        registration cannot accidentally shadow a built-in library.

        Returns:
            List of NodeLibrary instances sorted by priority (ascending).
        """
        entries: dict[str, tuple[type[NodeLibrary], int]] = {}

        self._discover_from_plug_registry(entries)
        self._discover_from_extension_namespaces()
        self._discover_builtin_fallbacks(entries)
        self._discover_from_manual_registry(entries)

        sorted_entries = sorted(
            entries.values(), key=lambda pair: (pair[1], pair[0].__name__)
        )
        return [cls() for cls, _priority in sorted_entries]

    def _discover_from_plug_registry(
        self, entries: dict[str, tuple[type[NodeLibrary], int]]
    ) -> None:
        """Find libraries via Plug.Registry.GetAllDerivedTypes()."""
        base_tf_type = Tf.Type.Find(NodeLibrary)
        if not base_tf_type:
            Tf.Warn(
                "NodeLibraryRegistry: NodeLibrary TfType not found. "
                "Dynamic library discovery is disabled."
            )
            return

        plug_registry = Plug.Registry()
        derived_types = plug_registry.GetAllDerivedTypes(base_tf_type)

        for tf_type in derived_types:
            plugin = plug_registry.GetPluginForType(tf_type)
            if plugin is None:
                continue

            try:
                plugin.Load()
            except Exception:
                Tf.Warn(
                    f"NodeLibraryRegistry: Failed to load plugin for "
                    f"{tf_type.typeName}. Skipping."
                )
                continue

            python_cls = tf_type.pythonClass
            if python_cls is None or inspect.isabstract(python_cls):
                continue

            priority = self._get_priority(tf_type, plugin)
            entries[python_cls.__name__] = (python_cls, priority)

    def _discover_from_extension_namespaces(self) -> None:
        """Auto-import packages under extension namespaces.

        Walks each namespace in _EXTENSION_NAMESPACES and imports every
        subpackage found. If a subpackage's __init__.py calls
        NodeLibraryRegistry.register(), the library is added to
        _manual_registry and picked up by _discover_from_manual_registry().
        """
        for ns in self._EXTENSION_NAMESPACES:
            try:
                pkg = importlib.import_module(ns)
            except ImportError:
                continue

            pkg_path = getattr(pkg, "__path__", None)
            if pkg_path is None:
                continue

            for _importer, modname, ispkg in pkgutil.iter_modules(pkg_path):
                if not ispkg:
                    continue
                full_name = f"{ns}.{modname}"
                try:
                    importlib.import_module(full_name)
                except Exception:
                    Tf.Warn(
                        f"NodeLibraryRegistry: Failed to auto-import "
                        f"extension package {full_name}. Skipping."
                    )

    def _discover_from_manual_registry(
        self, entries: dict[str, tuple[type[NodeLibrary], int]]
    ) -> None:
        """Merge programmatically registered libraries."""
        for name, (cls, priority) in self._manual_registry.items():
            if name not in entries:
                entries[name] = (cls, priority)

    def _discover_builtin_fallbacks(
        self, entries: dict[str, tuple[type[NodeLibrary], int]]
    ) -> None:
        """Import any built-in libraries not found via Plug.Registry or manual registration."""
        for module_path, class_name, default_priority in _BUILTIN_LIBRARIES:
            if class_name in entries:
                continue
            try:
                mod = importlib.import_module(module_path, package=__package__)
                cls = getattr(mod, class_name)
                entries[class_name] = (cls, default_priority)
                Tf.Warn(
                    f"NodeLibraryRegistry: {class_name} not found via "
                    f"plugInfo.json — loaded via direct import fallback."
                )
            except Exception:
                Tf.Warn(
                    f"NodeLibraryRegistry: {class_name} not found via "
                    f"plugInfo.json and direct import also failed. Skipping."
                )

    @staticmethod
    def _get_priority(tf_type: Tf.Type, plugin: Plug.Plugin) -> int:
        """Read priority from plugInfo.json metadata, defaulting to 100."""
        try:
            metadata = plugin.GetMetadataForType(tf_type)
            if metadata and "priority" in metadata:
                return int(metadata["priority"])
        except (TypeError, ValueError, KeyError):
            Tf.Warn(
                f"NodeLibraryRegistry: Invalid 'priority' value in "
                f"plugInfo.json for {tf_type.typeName}. Using default "
                f"({_DEFAULT_PRIORITY})."
            )
        return _DEFAULT_PRIORITY
