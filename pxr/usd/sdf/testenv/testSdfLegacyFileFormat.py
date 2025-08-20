#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Tf
import os
import unittest

class TestSdfLegacyFileFormat(unittest.TestCase):
    def test_ReadLegacySdfFileFormat(self):
        legacyRead = Tf.GetEnvSetting("SDF_FILE_FORMAT_LEGACY_IMPORT")

        layer = Sdf.Layer.CreateAnonymous()
        importStr = """#sdf 1.4.32
            def "Prim" {}
        """

        if legacyRead == "allow" or legacyRead == "warn":
            success = layer.ImportFromString(importStr)
            self.assertTrue(success)
        elif legacyRead == "error":
            with self.assertRaises(Tf.ErrorException):
                layer.ImportFromString(importStr)

if __name__ == "__main__":
    unittest.main()