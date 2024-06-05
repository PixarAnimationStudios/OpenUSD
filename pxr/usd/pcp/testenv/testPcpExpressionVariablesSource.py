#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
