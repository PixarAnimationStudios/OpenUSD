#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for NodeModel.schemaTypeName property.

Verifies lazy loading from prim, setter, cache invalidation, and
no-prim fallback for the schemaTypeName property added to NodeModel.

Uses mock prims to avoid SIGSEGV during interpreter shutdown caused by USD
C++ static destructors when real Usd.Stage objects are created in test
binaries (see testUsdNoodlesDisplayColor.py for the same pattern).
"""

import unittest
from unittest.mock import MagicMock

try:
    from pxr.UsdNoodles.models import NodeModel

    _has_pxr = True
except ImportError:
    _has_pxr = False


def _make_node_model(prim=None):
    """Create a NodeModel without a real USD stage.

    Constructs with no stage/primPath so the constructor skips the
    stage.GetPrimAtPath() call, then injects the mock prim directly.
    """
    model = NodeModel()
    if prim is not None:
        model._prim = prim
    return model


def _mock_prim(type_name="Shader"):
    """Create a mock prim with a configurable GetTypeName() return value."""
    prim = MagicMock()
    prim.GetTypeName.return_value = type_name
    return prim


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestSchemaTypeName(unittest.TestCase):
    """Verify schemaTypeName lazy loading, setter, and cache invalidation."""

    def test_lazy_load_from_prim(self):
        """Accessing schemaTypeName should call prim.GetTypeName() and cache."""
        prim = _mock_prim("Shader")
        model = _make_node_model(prim)

        result = model.schemaTypeName

        self.assertEqual(result, "Shader")
        prim.GetTypeName.assert_called_once()

    def test_lazy_load_caches_result(self):
        """Second access should return cached value without re-reading prim."""
        prim = _mock_prim("Material")
        model = _make_node_model(prim)

        _ = model.schemaTypeName
        _ = model.schemaTypeName

        # GetTypeName should only be called once (first access caches)
        prim.GetTypeName.assert_called_once()

    def test_setter_updates_value(self):
        """Setting schemaTypeName should update the cached value."""
        model = _make_node_model()

        model.schemaTypeName = "NodeGraph"

        self.assertEqual(model.schemaTypeName, "NodeGraph")

    def test_setter_overrides_prim_value(self):
        """Setter should take precedence over prim's GetTypeName."""
        prim = _mock_prim("Shader")
        model = _make_node_model(prim)

        model.schemaTypeName = "Material"

        self.assertEqual(model.schemaTypeName, "Material")
        # Prim should never be queried since setter pre-populated the cache
        prim.GetTypeName.assert_not_called()

    def test_no_prim_returns_empty_string(self):
        """Without a prim, schemaTypeName should return empty string."""
        model = _make_node_model()

        result = model.schemaTypeName

        self.assertEqual(result, "")

    def test_cache_invalidation(self):
        """After invalidateCache(), next access should re-read from prim."""
        prim = _mock_prim("Shader")
        model = _make_node_model(prim)

        # First access caches the value
        first = model.schemaTypeName
        self.assertEqual(first, "Shader")

        # Change what the prim returns
        prim.GetTypeName.return_value = "Material"

        # Invalidate and re-read
        model.invalidateCache()
        second = model.schemaTypeName

        self.assertEqual(second, "Material")
        self.assertEqual(prim.GetTypeName.call_count, 2)

    def test_setter_empty_string(self):
        """Setting schemaTypeName to empty string should work."""
        prim = _mock_prim("Shader")
        model = _make_node_model(prim)

        model.schemaTypeName = ""

        self.assertEqual(model.schemaTypeName, "")

    def test_prim_returns_empty_type_name(self):
        """When prim.GetTypeName() returns empty, should cache and return it."""
        prim = _mock_prim("")
        model = _make_node_model(prim)

        result = model.schemaTypeName

        self.assertEqual(result, "")
        prim.GetTypeName.assert_called_once()
