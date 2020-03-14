#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

from pxr import UsdUtils
import unittest

class TestUsdUtilsIntrospection(unittest.TestCase):
    def test_UsdUtilsComputeStageStats(self):
        expectedStageStats = {
            'masters': {
                'primCountsByType': {'untyped': 4, 'Mesh': 7, 'Xform': 2}, 
                'primCounts': {'activePrimCount': 13, 'inactivePrimCount': 0,
                                'instanceCount': 2, 'pureOverCount': 4, 
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
            'masterCount': 4, 
            'assetCount': 1}

        self.assertEqual(UsdUtils.ComputeUsdStageStats('stageStats.usda'),
                         expectedStageStats)

if __name__=="__main__":
    unittest.main()
