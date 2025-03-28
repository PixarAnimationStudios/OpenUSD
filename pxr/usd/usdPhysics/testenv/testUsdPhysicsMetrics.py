#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Tf, Usd, UsdPhysics

class TestUsdPhysicsMetrics(unittest.TestCase):

    def test_kilogramsPerUnit(self):
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage)

        self.assertEqual(UsdPhysics.GetStageKilogramsPerUnit(stage),
                         UsdPhysics.MassUnits.kilograms)
        self.assertFalse(UsdPhysics.StageHasAuthoredKilogramsPerUnit(stage))

        self.assertTrue(UsdPhysics.SetStageKilogramsPerUnit(stage,
                        UsdPhysics.MassUnits.grams))
        self.assertTrue(UsdPhysics.StageHasAuthoredKilogramsPerUnit(stage))
        authored = UsdPhysics.GetStageKilogramsPerUnit(stage)
        self.assertTrue(UsdPhysics.MassUnitsAre(authored, UsdPhysics.MassUnits.grams))


if __name__ == "__main__":
    unittest.main()
