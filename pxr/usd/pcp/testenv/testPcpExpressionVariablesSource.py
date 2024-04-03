#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

import unittest

from pxr import Pcp, Sdf

class TestPcpExpressionVariablesSource(unittest.TestCase):
    def test_Basic(self):
        defaultSource = Pcp.ExpressionVariablesSource()
        self.assertEqual(defaultSource, defaultSource)
        self.assertTrue(defaultSource.IsRootLayerStack())
        self.assertIsNone(defaultSource.GetLayerStackIdentifier())

        rootLayer = Sdf.Layer.CreateAnonymous('root')
        rootId = Pcp.LayerStackIdentifier(rootLayer)
        rootSource = Pcp.ExpressionVariablesSource(
            layerStackId=rootId, rootLayerStackId=rootId)
        self.assertEqual(rootSource, defaultSource)
        self.assertTrue(rootSource.IsRootLayerStack())
        self.assertIsNone(rootSource.GetLayerStackIdentifier())
        self.assertEqual(rootSource.ResolveLayerStackIdentifier(rootId), rootId)
        
        refLayer = Sdf.Layer.CreateAnonymous('ref')
        refId = Pcp.LayerStackIdentifier(refLayer)
        refSource = Pcp.ExpressionVariablesSource(
            layerStackId=refId, rootLayerStackId=rootId)
        self.assertNotEqual(refSource, defaultSource)
        self.assertFalse(refSource.IsRootLayerStack())
        self.assertEqual(refSource.GetLayerStackIdentifier(), refId)
        self.assertEqual(refSource.ResolveLayerStackIdentifier(rootId), refId)

if __name__ == "__main__":
    unittest.main()
