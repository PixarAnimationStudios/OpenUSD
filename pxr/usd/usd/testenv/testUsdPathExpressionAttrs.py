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

from pxr import Usd, Vt, Sdf, Tf
import unittest

class TestUsdPathExpressionAttrs(unittest.TestCase):

    def test_BasicResolution(self):
        # Superlayer
        supLayer = Sdf.Layer.CreateAnonymous()
        supPrimSpec = Sdf.CreatePrimInLayer(supLayer, '/prim')
        supAttrSpec = Sdf.AttributeSpec(
            supPrimSpec, 'expr', Sdf.ValueTypeNames.PathExpression)
        supAttrSpec.default = Sdf.PathExpression('super %_')
        # Authoring makes absolute, as with relationships/connections.
        self.assertEqual(supAttrSpec.default,
                         Sdf.PathExpression('/prim/super %_'))

        # Sublayer
        subLayer = Sdf.Layer.CreateAnonymous()
        subPrimSpec = Sdf.CreatePrimInLayer(subLayer, '/prim')
        subAttrSpec = Sdf.AttributeSpec(
            subPrimSpec, 'expr', Sdf.ValueTypeNames.PathExpression)
        subAttrSpec.default = Sdf.PathExpression('suber')
        # Authoring makes absolute, as with relationships/connections.
        self.assertEqual(subAttrSpec.default,
                         Sdf.PathExpression('/prim/suber'))

        # Put 'em together.
        supLayer.subLayerPaths.append(subLayer.identifier)

        stage = Usd.Stage.Open(supLayer)

        # The expression should compose as expected, for a call to Get() at
        # default time.
        self.assertEqual(stage.GetAttributeAtPath('/prim.expr').Get(),
                         Sdf.PathExpression('/prim/super /prim/suber'))

    def test_BasicPathTranslation(self):
        refTargLayer = Sdf.Layer.CreateAnonymous()
        refPrimSpec = Sdf.CreatePrimInLayer(refTargLayer, '/prim')
        refAttrSpec = Sdf.AttributeSpec(
            refPrimSpec, 'expr', Sdf.ValueTypeNames.PathExpression)
        refAttrSpec.default = Sdf.PathExpression('refChild')
        # Authoring makes absolute, as with relationships/connections.
        self.assertEqual(refAttrSpec.default,
                         Sdf.PathExpression('/prim/refChild'))

        srcLayer = Sdf.Layer.CreateAnonymous()
        srcPrimSpec = Sdf.CreatePrimInLayer(srcLayer, '/srcPrim')
        srcAttrSpec = Sdf.AttributeSpec(
            srcPrimSpec, 'expr', Sdf.ValueTypeNames.PathExpression)
        srcAttrSpec.default = Sdf.PathExpression('srcChild %_')
        # Authoring makes absolute, as with relationships/connections.
        self.assertEqual(srcAttrSpec.default,
                         Sdf.PathExpression('/srcPrim/srcChild %_'))

        # Put 'em together.
        srcPrimSpec.referenceList.Append(
            Sdf.Reference(refTargLayer.identifier, refPrimSpec.path))

        stage = Usd.Stage.Open(srcLayer)

        # The expression should path-translate and compose as expected for a
        # call to Get() at default time.
        self.assertEqual(stage.GetAttributeAtPath('/srcPrim.expr').Get(),
                         Sdf.PathExpression(
                             '/srcPrim/srcChild /srcPrim/refChild'))

    def test_BasicPathTranslation2(self):
        # Test that `//` leading exprs translate across references *and* across
        # the prototype-to-instance path mapping, which uses a different
        # mechanism from PcpMapFunction.
        stage = Usd.Stage.CreateInMemory()
        stage2 = Usd.Stage.CreateInMemory()

        src = stage.DefinePrim('/src')
        dst = stage2.DefinePrim('/dst')

        dstCapi = Usd.CollectionAPI.Apply(dst, 'testRef')
        dstCapi.GetMembershipExpressionAttr().Set(Sdf.PathExpression('//'))

        src.GetReferences().AddReference(
            stage2.GetRootLayer().identifier, '/dst')

        srcCapi = Usd.CollectionAPI.Get(src, 'testRef')
        self.assertEqual(srcCapi.GetMembershipExpressionAttr().Get(),
                         Sdf.PathExpression('//'))

        
if __name__ == '__main__':
    unittest.main()
