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

import unittest

class TestUsdBugs(unittest.TestCase):
    def test_153956(self):
        from pixar import Sdf

        # Create a crate-backed .usd file and populate it with an
        # attribute connection. These files do not store specs for
        # targets/connections, so there will be an entry in the
        # connectionChildren list but no corresponding spec.
        layer = Sdf.Layer.CreateAnonymous(".usd")
        primSpec = Sdf.CreatePrimInLayer(layer, "/Test")
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Float)

        # -- Adding item to prependedItems list..."
        attrSpec.connectionPathList.prependedItems.append("/Test.prependedItem")

        # Transfer the contents of the crate-backed .usd file into an
        # memory-backed .usda file. These file *do* store specs for
        # targets/connections.
        newLayer = Sdf.Layer.CreateAnonymous(".usda")
        newLayer.TransferContent(layer)

        primSpec = newLayer.GetPrimAtPath("/Test")
        attrSpec = primSpec.properties["attr"]

        # Adding an item to the explicitItems list changes to listOp to
        # explicit mode, but does not clear any existing connectionChildren.
        attrSpec.connectionPathList.explicitItems.append("/Test.explicitItem")

        # Prior to the fix, this caused a failed verify b/c an entry exists in
        # the connectionChildren list for which there is no corresponding spec.
        primSpec.name = "Test2"

    def test_141718(self):
        from pxr import Sdf
        crateLayer = Sdf.Layer.CreateAnonymous('.usdc')
        prim = Sdf.CreatePrimInLayer(crateLayer, '/Prim')
        rel = Sdf.RelationshipSpec(prim, 'myRel', custom=False)
        rel.targetPathList.explicitItems.append('/Prim2')
        asciiLayer = Sdf.Layer.CreateAnonymous('.usda')
        asciiLayer.TransferContent(crateLayer)
        p = asciiLayer.GetPrimAtPath('/Prim')
        p.RemoveProperty(p.relationships['myRel'])

        
if __name__ == '__main__':
    unittest.main()
