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
       nodeGraphNode = UsdUI.NodeGraphNodeAPI(rootPrim)
       assert nodeGraphNode
       
       # Test Position
       posAttr = nodeGraphNode.GetPosAttr()
       assert not posAttr
       posAttr = nodeGraphNode.CreatePosAttr()
       assert posAttr, "Failed creating pos attribute"
       assert posAttr.GetTypeName() == 'float2', \
           "Type of position attribute should be 'float2', not %s" % posAttr.GetTypeName()
       posAttr.Set(Gf.Vec2f(3, 2))
       
       # Test Stacking Order
       stackingOrderAttr = nodeGraphNode.GetStackingOrderAttr()
       assert not stackingOrderAttr
       stackingOrderAttr = nodeGraphNode.CreateStackingOrderAttr()
       assert stackingOrderAttr, "Failed creating stacking order attribute"
       assert stackingOrderAttr.GetTypeName() == 'int', \
           "Type of position attribute should be 'int', not %s" % \
           stackingOrderAttr.GetTypeName()
       stackingOrderAttr.Set(100)
       
       # Test Display Color
       displayColorAttr = nodeGraphNode.GetDisplayColorAttr()
       assert not displayColorAttr
       displayColorAttr = nodeGraphNode.CreateDisplayColorAttr()
       assert displayColorAttr, "Failed creating display color attribute"
       assert displayColorAttr.GetTypeName() == 'color3f', \
           "Type of position attribute should be 'color3f', not %s" % \
           displayColorAttr.GetTypeName()
       displayColorAttr.Set(Gf.Vec3f(1, 0, 0))

       # Test Size
       sizeAttr = nodeGraphNode.GetSizeAttr()
       assert not sizeAttr
       sizeAttr = nodeGraphNode.CreateSizeAttr()
       assert sizeAttr, "Failed creating size attribute"
       assert sizeAttr.GetTypeName() == 'float2', \
           "Type of size attribute should be 'float2', not %s" % sizeAttr.GetTypeName()
       sizeAttr.Set(Gf.Vec2f(300, 400))
       
       stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
