#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
