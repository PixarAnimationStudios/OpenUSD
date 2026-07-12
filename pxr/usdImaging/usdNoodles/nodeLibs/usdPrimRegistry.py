#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
Registry of concrete USD prim types discovered from UsdSchemaRegistry.

Pure-Python replacement for the former C++ ``noodles::UsdPrimRegistry``. It
discovers every concrete typed prim schema known to ``UsdSchemaRegistry`` and
groups them by their plugin family (``usdGeom``, ``usdLux``, ...) so the create
menu can offer every concrete type rather than a fixed allowlist.

Discovery runs once on first access and is cached on the instance.

Attribute introspection and connection reading (``GetDescriptorFromPrim``,
``GetLinksForPrim``, ``GetSchemaAwarePropertyNames``,
``GetAttributeTypeNameString`` on the old C++ class) now live in
``_schema_pin_names.py`` and ``pinUtils.py`` and are the single source of truth
for pin discovery, so only schema-type discovery remains here.
"""

from __future__ import annotations

from dataclasses import dataclass

from pxr import Plug, Tf, Usd


@dataclass(frozen=True)
class PrimTypeInfo:
    """A concrete USD prim type and the plugin family it belongs to.

    Field names mirror the former C++ ``UsdPrimRegistry::PrimTypeInfo`` struct
    so existing callers reading ``.typeName`` / ``.family`` are unchanged.
    """

    typeName: str
    family: str


class UsdPrimRegistry:
    """Discovers concrete USD prim types from ``UsdSchemaRegistry``.

    Types are grouped by plugin family and sorted alphabetically within each
    family. Discovery is performed once on first access and cached on the
    instance.

    Not thread-safe: construct and query from a single thread (the Qt main
    thread), matching USD's own ``Plug.Registry`` / ``Usd.SchemaRegistry``,
    which are also not thread-safe for the queries used here.
    """

    def __init__(self) -> None:
        # None until _discover_types() runs; acts as the once-only guard.
        self._all_types: list[PrimTypeInfo] | None = None
        self._family_index: dict[str, list[int]] = {}

    def _discover_types(self) -> None:
        if self._all_types is not None:
            return

        all_types: list[PrimTypeInfo] = []
        family_index: dict[str, list[int]] = {}

        base_type = Tf.Type.FindByName("UsdSchemaBase")
        plug_registry = Plug.Registry()

        for tf_type in base_type.GetAllDerivedTypes():
            try:
                # IsConcrete excludes abstract and API schemas. When a schema
                # type's plugin is not registered in the current environment,
                # USD posts a Tf error (surfaced in Python as Tf.ErrorException)
                # while resolving its schema kind; skip such types instead of
                # aborting, mirroring the former C++ code, where the same
                # condition simply yielded a non-concrete result.
                if not Usd.SchemaRegistry.IsConcrete(tf_type):
                    continue

                type_name = str(Usd.SchemaRegistry.GetSchemaTypeName(tf_type))
                if not type_name:
                    continue

                # Family from the plugin name, reliable across USD versions.
                plugin = plug_registry.GetPluginForType(tf_type)
                family = plugin.name if plugin is not None else "Other"
            except Tf.ErrorException:
                continue

            index = len(all_types)
            all_types.append(PrimTypeInfo(typeName=type_name, family=family))
            family_index.setdefault(family, []).append(index)

        # Sort indices within each family alphabetically by type name.
        for indices in family_index.values():
            indices.sort(key=lambda i: all_types[i].typeName)

        self._all_types = all_types
        self._family_index = family_index

    def GetFamilies(self) -> list[str]:
        """Return sorted plugin family names (e.g. ``usdGeom``)."""
        self._discover_types()
        return sorted(self._family_index)

    def GetPrimTypes(self, family: str) -> list[PrimTypeInfo]:
        """Return concrete prim types in ``family``, sorted by type name."""
        self._discover_types()
        indices = self._family_index.get(family)
        if not indices:
            return []
        all_types = self._all_types or []
        return [all_types[index] for index in indices]
