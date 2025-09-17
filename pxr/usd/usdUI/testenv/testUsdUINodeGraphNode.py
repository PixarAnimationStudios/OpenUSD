#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd, UsdUI, Gf
import unittest

class TestUsdUINodeGraphNode(unittest.TestCase):
    def test_Basic(self):
       fileName = "nodeGraph.usda"
       stage = Usd.Stage.CreateNew(fileName)
       
       # Create this prim first, since it's the "entrypoint" to the layer, and
       # we want it to appear at the top
       rootPrim = stage.DefinePrim("/ANode")
       
       # Test Node
       nodeGraphNode = UsdUI.NodeGraphNodeAPI.Apply(rootPrim)
       assert(nodeGraphNode)
       
       # Test Position
       posAttr = nodeGraphNode.GetPosAttr()
       assert posAttr
       posAttr = nodeGraphNode.CreatePosAttr()
       assert posAttr, "Failed creating pos attribute"
       assert posAttr.GetTypeName() == 'float2', \
           "Type of position attribute should be 'float2', not %s" % posAttr.GetTypeName()
       posAttr.Set(Gf.Vec2f(3, 2))
       
       # Test Stacking Order
       stackingOrderAttr = nodeGraphNode.GetStackingOrderAttr()
       assert stackingOrderAttr
       stackingOrderAttr = nodeGraphNode.CreateStackingOrderAttr()
       assert stackingOrderAttr, "Failed creating stacking order attribute"
       assert stackingOrderAttr.GetTypeName() == 'int', \
           "Type of position attribute should be 'int', not %s" % \
           stackingOrderAttr.GetTypeName()
       stackingOrderAttr.Set(100)
       
       # Test Display Color
       displayColorAttr = nodeGraphNode.GetDisplayColorAttr()
       assert displayColorAttr
       displayColorAttr = nodeGraphNode.CreateDisplayColorAttr()
       assert displayColorAttr, "Failed creating display color attribute"
       assert displayColorAttr.GetTypeName() == 'color3f', \
           "Type of position attribute should be 'color3f', not %s" % \
           displayColorAttr.GetTypeName()
       displayColorAttr.Set(Gf.Vec3f(1, 0, 0))

       # Test Size
       sizeAttr = nodeGraphNode.GetSizeAttr()
       assert sizeAttr
       sizeAttr = nodeGraphNode.CreateSizeAttr()
       assert sizeAttr, "Failed creating size attribute"
       assert sizeAttr.GetTypeName() == 'float2', \
           "Type of size attribute should be 'float2', not %s" % sizeAttr.GetTypeName()
       sizeAttr.Set(Gf.Vec2f(300, 400))
       
       stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
