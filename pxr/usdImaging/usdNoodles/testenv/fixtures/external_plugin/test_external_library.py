#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#
# pyre-strict

"""
Test fixture: minimal external NodeLibrary plugin.

Used by testUsdNoodlesNodeLibraryRegistry.py to validate that external
plugins discovered via PXR_PLUGINPATH_NAME are loaded correctly.

This module is NOT imported directly by tests — it is loaded at runtime
by USD's Plug.Registry when PXR_PLUGINPATH_NAME includes the directory
containing the accompanying plugInfo.json.
"""

from pxr import Tf
from pxr.UsdNoodles.nodeLibraries import NodeLibrary


class TestExternalLibrary(NodeLibrary):
    """Minimal external node library for testing plugin discovery."""

    def get_name(self):
        return "Test External"

    def get_families(self):
        return ["TestFamily"]

    def get_node_types(self, family):
        return [{"name": "TestNode", "identifier": "TestNode", "family": family}]

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


Tf.Type.Define(TestExternalLibrary)
