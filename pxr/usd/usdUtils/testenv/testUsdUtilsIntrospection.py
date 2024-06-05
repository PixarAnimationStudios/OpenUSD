#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import UsdUtils
import unittest

class TestUsdUtilsIntrospection(unittest.TestCase):
    def test_UsdUtilsComputeStageStats(self):
        expectedStageStats = {
            'prototypes': {
                'primCountsByType': {'untyped': 4, 'Mesh': 7, 'Xform': 2}, 
                'primCounts': {'activePrimCount': 13, 'inactivePrimCount': 0,
                                'instanceCount': 2, 'pureOverCount': 0, 
                                'totalPrimCount': 13}},
                'usedLayerCount': 1, 
            'primary': {
                'primCountsByType': {'untyped': 3, 'Scope': 2, 'Mesh': 16, 'Xform': 13},
                'primCounts': {'activePrimCount': 32, 'inactivePrimCount': 2,
                            'instanceCount': 6, 'pureOverCount': 1, 
                            'totalPrimCount': 34}}, 
            'modelCount': 8, 
            'instancedModelCount': 1, 
            'totalPrimCount': 47, 
            'totalInstanceCount': 8, 
            'prototypeCount': 4, 
            'assetCount': 1}

        self.assertEqual(UsdUtils.ComputeUsdStageStats('stageStats.usda'),
                         expectedStageStats)

if __name__=="__main__":
    unittest.main()
