#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# pyre-strict

"""
Import chain tests for pxr.UsdNoodles.

Validates that the core UsdNoodles modules are importable in headless
environments without PySide6/Qt. This guards against regressions where
a module-level import pulls in Qt transitively.
"""

import unittest


class UsdNoodlesCoreImportTest(unittest.TestCase):
    """Verify core pxr.UsdNoodles modules load without Qt."""

    def test_import_pxr_tf(self) -> None:
        from pxr import Tf  # noqa: F401

    def test_import_pxr_plug(self) -> None:
        from pxr import Plug  # noqa: F401

    def test_import_pxr_usd(self) -> None:
        from pxr import Usd  # noqa: F401

    def test_import_usd_noodles_package(self) -> None:
        from pxr import UsdNoodles  # noqa: F401

    def test_import_node_libraries_module(self) -> None:
        """nodeLibraries.py (the ABC) should import without Qt."""
        from pxr.UsdNoodles.nodeLibraries import NodeLibrary  # noqa: F401

    def test_import_nodelibs_package(self) -> None:
        """nodeLibs/__init__.py should import without Qt (lazy NoodlesConfig)."""
        from pxr.UsdNoodles import nodeLibs  # noqa: F401

    def test_import_registry(self) -> None:
        """registry.py should import without Qt."""
        from pxr.UsdNoodles.nodeLibs.registry import NodeLibraryRegistry  # noqa: F401

    def test_node_library_tf_type_registered(self) -> None:
        """NodeLibrary ABC should be registered as a TfType."""
        from pxr import Tf
        from pxr.UsdNoodles.nodeLibraries import NodeLibrary

        tf_type = Tf.Type.Find(NodeLibrary)
        self.assertTrue(tf_type, "NodeLibrary TfType not registered")


if __name__ == "__main__":
    unittest.main()
