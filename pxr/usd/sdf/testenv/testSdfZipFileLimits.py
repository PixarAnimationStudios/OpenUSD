#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest
import zipfile

from pxr import Sdf, Tf

MAX_ZIPFILE_SIZE = Tf.GetEnvSetting('SDF_MAX_ZIPFILE_SIZE')

class TestSdfZipFileLimits(unittest.TestCase):
    def createFileWithPrims(self, filename, count):
        layer = Sdf.Layer.CreateAnonymous(filename)
        for i in range(count):
            Sdf.CreatePrimInLayer(layer, '/Test{}'.format(i))

        # ensure same output size between windows and linux/mac
        with open(filename, "w", newline="\n") as f:
            f.write(layer.ExportToString())


    def test_addFile(self):
        # create test files and verify their sizes are below / above
        # the max archive size as we would expect
        self.createFileWithPrims("0.usda", 0)
        self.assertLess(os.path.getsize("0.usda"), MAX_ZIPFILE_SIZE)
        self.createFileWithPrims("1.usda", 1)
        self.assertLess(os.path.getsize("1.usda"), MAX_ZIPFILE_SIZE)
        self.createFileWithPrims("11.usda", 11)
        self.assertGreater(os.path.getsize("11.usda"), MAX_ZIPFILE_SIZE)

        # Baseline sanity check. This file should be MAX_ZIPFILE_SIZE bytes
        with Sdf.ZipFileWriter.CreateNew("valid.usdz") as zfw:
            zfw.AddFile("1.usda")
        self.assertTrue(os.path.isfile("valid.usdz"))
        self.assertEqual(os.path.getsize("valid.usdz"), MAX_ZIPFILE_SIZE)

        # Failure Case 1: Adding Files before saving trips the limit
        # 1.usda is small enough to fit in the archive, but 11.usda is large
        # enough that adding it will push the archive over the maximum size
        # before directory data is written
        with Sdf.ZipFileWriter.CreateNew("TooManyFiles.usdz") as zfw:
            zfw.AddFile("1.usda")

        with self.assertRaises(RuntimeError):
            zfw.AddFile("11.usda")

            # prevent context from calling Save and triggering other exceptions
            zfw.Discard()


        # Failure Case 2: Zipfile exceeds limit when writing central directory
        with self.assertRaises(RuntimeError):
            with Sdf.ZipFileWriter.CreateNew("archiveTooBig.usdz") as zfw:
                zfw.AddFile("0.usda")
                zfw.AddFile("1.usda")


if __name__ == "__main__":
    unittest.main()
