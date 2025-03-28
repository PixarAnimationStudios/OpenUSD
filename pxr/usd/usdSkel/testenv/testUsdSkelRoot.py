#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Usd, UsdSkel, UsdGeom
import unittest


class TestUsdSkelRoot(unittest.TestCase):

    def test_ComputeExtentPlugin(self):
        """Tests plugin for computing extents on a UsdSkelRoot."""

        testFile = "root.usda"
        stage = Usd.Stage.Open(testFile)

        boundable = UsdGeom.Boundable(stage.GetPrimAtPath("/Root"))

        for time in range(int(stage.GetStartTimeCode()),
                          int(stage.GetEndTimeCode())+1):
            UsdGeom.Boundable.ComputeExtentFromPlugins(boundable, time)

        stage.GetRootLayer().Export("root.computedExtents.usda")


if __name__ == "__main__":
    unittest.main()
