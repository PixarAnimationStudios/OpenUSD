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

import sys, unittest
from pxr import Sdf,Pcp,Usd,Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdReferences(unittest.TestCase):
    def test_API(self):
        for fmt in allFormats:
            s1 = Usd.Stage.CreateInMemory('API-s1.'+fmt)
            s2 = Usd.Stage.CreateInMemory('API-s2.'+fmt)
            srcPrim = s1.OverridePrim('/src')
            trgPrimInternal = s1.OverridePrim('/trg_internal')
            srcPrimSpec = s1.GetRootLayer().GetPrimAtPath('/src')    
            trgPrim = s2.OverridePrim('/trg')
            s2.GetRootLayer().defaultPrim = 'trg'

            # Identifier only.
            srcPrim.GetReferences().AddReference(s2.GetRootLayer().identifier)
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference(s2.GetRootLayer().identifier))
            srcPrim.GetReferences().ClearReferences()

            # Identifier and layerOffset.
            srcPrim.GetReferences().AddReference(s2.GetRootLayer().identifier,
                                        Sdf.LayerOffset(1.25, 2.0))
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference(s2.GetRootLayer().identifier,
                                      layerOffset=Sdf.LayerOffset(1.25, 2.0)))
            srcPrim.GetReferences().ClearReferences()

            # Identifier and primPath.
            srcPrim.GetReferences().AddReference(s2.GetRootLayer().identifier, '/trg')
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference(s2.GetRootLayer().identifier,
                                      primPath='/trg'))
            srcPrim.GetReferences().ClearReferences()

            # Identifier and primPath and layerOffset.
            srcPrim.GetReferences().AddReference(s2.GetRootLayer().identifier, '/trg',
                                        Sdf.LayerOffset(1.25, 2.0))
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference(s2.GetRootLayer().identifier, primPath='/trg',
                                      layerOffset=Sdf.LayerOffset(1.25, 2.0)))
            srcPrim.GetReferences().ClearReferences()

            # primPath only.
            srcPrim.GetReferences().AddInternalReference('/trg_internal')
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference('', primPath='/trg_internal'))
            srcPrim.GetReferences().ClearReferences()

            # primPath and layer offset.
            srcPrim.GetReferences().AddInternalReference(
                '/trg_internal', layerOffset=Sdf.LayerOffset(1.25, 2.0))
            self.assertEqual(srcPrimSpec.referenceList.GetAddedOrExplicitItems()[0],
                        Sdf.Reference('', primPath='/trg_internal',
                                      layerOffset=Sdf.LayerOffset(1.25, 2.0)))
            srcPrim.GetReferences().ClearReferences()

    def test_DefaultPrimBasics(self):
        # create a layer, set DefaultPrim, then reference it.
        for fmt in allFormats:
            targLyr = Sdf.Layer.CreateAnonymous('DefaultPrimBasics.'+fmt)

            def makePrim(name, attrDefault):
                primSpec = Sdf.CreatePrimInLayer(targLyr, name)
                primSpec.specifier = Sdf.SpecifierDef
                attr = Sdf.AttributeSpec(
                    primSpec, 'attr', Sdf.ValueTypeNames.Double)
                attr.default = attrDefault

            makePrim('target1', 1.234)
            makePrim('target2', 2.345)

            targLyr.defaultPrim = 'target1'

            # create a new layer and reference the first.
            srcLyr = Sdf.Layer.CreateAnonymous('DefaultPrimBasics-new.'+fmt)
            srcPrimSpec = Sdf.CreatePrimInLayer(srcLyr, '/source')

            # create a stage with srcLyr.
            stage = Usd.Stage.Open(srcLyr)
            prim = stage.GetPrimAtPath('/source')
            self.assertTrue(prim)

            # author a prim-path-less reference to the targetLyr.
            prim.GetReferences().AddReference(targLyr.identifier)

            # should now pick up 'attr' from across the reference.
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)


    def test_DefaultPrimChangeProcessing(self):
        for fmt in allFormats:
            # create a layer, set DefaultPrim, then reference it.
            targLyr = Sdf.Layer.CreateAnonymous('DefaultPrimChangeProcessing.'+fmt)

            def makePrim(name, attrDefault):
                primSpec = Sdf.CreatePrimInLayer(targLyr, name)
                primSpec.specifier = Sdf.SpecifierDef
                attr = Sdf.AttributeSpec(
                    primSpec, 'attr', Sdf.ValueTypeNames.Double)
                attr.default = attrDefault

            makePrim('target1', 1.234)
            makePrim('target2', 2.345)

            targLyr.defaultPrim = 'target1'

            # create a new layer and reference the first.
            srcLyr = Sdf.Layer.CreateAnonymous(
                'DefaultPrimChangeProcessing-new.'+fmt)
            srcPrimSpec = Sdf.CreatePrimInLayer(srcLyr, '/source')
            srcPrimSpec.referenceList.Add(Sdf.Reference(targLyr.identifier))

            # create a stage with srcLyr.
            stage = Usd.Stage.Open(srcLyr)

            prim = stage.GetPrimAtPath('/source')
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

            # Clear defaultPrim.  This should issue a composition
            # error, and fail to pick up the attribute from the referenced prim.
            targLyr.ClearDefaultPrim()
            self.assertTrue(prim)
            self.assertFalse(prim.GetAttribute('attr'))

            # Change defaultPrim to other target.  This should pick up the reference
            # again, but to the new prim with the new attribute value.
            targLyr.defaultPrim = 'target2'
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 2.345)


    def test_InternalReferences(self):
        for fmt in allFormats:
            targLyr = Sdf.Layer.CreateAnonymous('InternalReferences.'+fmt)

            def makePrim(name, attrDefault):
                primSpec = Sdf.CreatePrimInLayer(targLyr, name)
                primSpec.specifier = Sdf.SpecifierDef
                attr = Sdf.AttributeSpec(
                    primSpec, 'attr', Sdf.ValueTypeNames.Double)
                attr.default = attrDefault

            makePrim('target1', 1.234)
            makePrim('target2', 2.345)

            targLyr.defaultPrim = 'target1'

            stage = Usd.Stage.Open(targLyr)
            prim = stage.DefinePrim('/ref1')
            prim.GetReferences().AddInternalReference('/target2')
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 2.345)

            prim.GetReferences().ClearReferences()
            self.assertTrue(prim)
            self.assertFalse(prim.GetAttribute('attr'))

            prim.GetReferences().AddInternalReference('/target1')
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

    def test_SubrootReferences(self):
        for fmt in allFormats:
            refLayer = Sdf.Layer.CreateAnonymous('SubrootReferences.'+fmt)
            
            def makePrim(name, attrDefault):
                primSpec = Sdf.CreatePrimInLayer(refLayer, name)
                primSpec.specifier = Sdf.SpecifierDef
                attr = Sdf.AttributeSpec(
                    primSpec, 'attr', Sdf.ValueTypeNames.Double)
                attr.default = attrDefault

            makePrim('/target1/child', 1.234)

            stage = Usd.Stage.CreateInMemory()
            prim = stage.DefinePrim('/subroot_ref1')
            prim.GetReferences().AddReference(
                refLayer.identifier, '/target1/child')
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

            prim = stage.DefinePrim('/subroot_ref2')
            prim.GetReferences().AddReference(
                Sdf.Reference(refLayer.identifier, '/target1/child'))
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

            stage = Usd.Stage.Open(refLayer)
            prim = stage.DefinePrim('/subroot_ref3')
            prim.GetReferences().AddInternalReference('/target1/child')
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

    def test_PrependVsAppend(self):
        for fmt in allFormats:
            layer = Sdf.Layer.CreateAnonymous('PrependVsAppend.'+fmt)

            def makePrim(name, attrDefault):
                primSpec = Sdf.CreatePrimInLayer(layer, name)
                primSpec.specifier = Sdf.SpecifierDef
                attr = Sdf.AttributeSpec(
                    primSpec, 'attr', Sdf.ValueTypeNames.Double)
                attr.default = attrDefault

            makePrim('target1', 1.234)
            makePrim('target2', 2.345)
            stage = Usd.Stage.Open(layer)
            prim = stage.DefinePrim('/ref')

            # Prepend target1, and then prepend target2:
            # target2 ends up stronger.
            prim.GetReferences().AddInternalReference('/target1',
                position = Usd.ListPositionFront)
            prim.GetReferences().AddInternalReference('/target2',
                position = Usd.ListPositionFront)
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 2.345)

            prim.GetReferences().ClearReferences()
            self.assertTrue(prim)
            self.assertFalse(prim.GetAttribute('attr'))

            # Append target1, and then append target2:
            # target1 ends up stronger.
            prim.GetReferences().AddInternalReference('/target1',
                position = Usd.ListPositionBack)
            prim.GetReferences().AddInternalReference('/target2',
                position = Usd.ListPositionBack)
            self.assertTrue(prim)
            self.assertEqual(prim.GetAttribute('attr').Get(), 1.234)

    def test_InternalReferenceMapping(self):
        for fmt in allFormats:
            # Create test scenegraph
            refLayer = Sdf.Layer.CreateAnonymous('InternalRefMapping_ref.'+fmt)
            refSpec = Sdf.PrimSpec(refLayer, 'Ref', Sdf.SpecifierDef)
            subRefSpec = Sdf.PrimSpec(refSpec, 'SubrootRef', Sdf.SpecifierDef)
            childRefSpec = Sdf.PrimSpec(refSpec, 'Child', Sdf.SpecifierDef)
            refSpec2 = Sdf.PrimSpec(refLayer, 'Ref2', Sdf.SpecifierDef)
            childRefSpec2 = Sdf.PrimSpec(refSpec2, 'Child', Sdf.SpecifierDef)

            stage = Usd.Stage.CreateInMemory('InternalRefMapping.'+fmt)
            prim = stage.DefinePrim('/Root')
            prim.GetReferences().AddReference(refLayer.identifier, refSpec.path)
            prim = stage.GetPrimAtPath('/Root')

            # Set the edit target to point to the referenced prim
            refNode = prim.GetPrimIndex().rootNode.children[0]
            self.assertEqual(refNode.arcType, Pcp.ArcTypeReference)
            stage.SetEditTarget(Usd.EditTarget(refLayer, refNode))

            # Add an internal sub-root reference to the child of the
            # referenced prim. 
            childPrim = stage.GetPrimAtPath('/Root/Child')
            childPrim.GetReferences().AddInternalReference(
                '/Root/SubrootRef', position=Usd.ListPositionFront)

            expectedRefs = Sdf.ReferenceListOp()
            expectedRefs.prependedItems = [
                Sdf.Reference(primPath='/Ref/SubrootRef')
            ]
            self.assertEqual(childRefSpec.GetInfo('references'), expectedRefs)

            # Remove the internal sub-root reference.
            childPrim.GetReferences().RemoveReference(
                Sdf.Reference(primPath='/Root/SubrootRef'))
            expectedRefs = Sdf.ReferenceListOp()
            expectedRefs.deletedItems = [
                Sdf.Reference(primPath='/Ref/SubrootRef')
            ]
            self.assertEqual(childRefSpec.GetInfo('references'), expectedRefs)

            # Add an internal reference.
            childPrim.GetReferences().AddInternalReference(
                '/Ref2', position=Usd.ListPositionFront)
            expectedRefs = Sdf.ReferenceListOp()
            expectedRefs.deletedItems = [
                Sdf.Reference(primPath='/Ref/SubrootRef')
            ]
            expectedRefs.prependedItems = [
                Sdf.Reference(primPath='/Ref2')
            ]
            self.assertEqual(childRefSpec.GetInfo('references'), expectedRefs)

            # Remove internal reference.
            childPrim.GetReferences().RemoveReference(
                Sdf.Reference(primPath='/Ref2'))
            expectedRefs = Sdf.ReferenceListOp()
            expectedRefs.deletedItems = [
                Sdf.Reference(primPath='/Ref/SubrootRef'),
                Sdf.Reference(primPath='/Ref2')
            ]
            self.assertEqual(childRefSpec.GetInfo('references'), expectedRefs)

            # Try to set an unmappable sub-root reference.
            with self.assertRaises(Tf.ErrorException):
                childPrim.GetReferences().AddInternalReference('/Ref2/Child')
            self.assertEqual(childRefSpec.GetInfo('references'), expectedRefs)

    def test_InternalReferenceMappingVariants(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt, sessionLayer=None)

            # Create test scenegraph with variant
            refPrim = stage.DefinePrim("/Root")
            vset = refPrim.GetVariantSet("v")
            vset.AddVariant("x")
            vset.SetVariantSelection("x")
            with vset.GetVariantEditContext():
                stage.DefinePrim("/Root/SubrootRef")
                stage.DefinePrim("/Root/Child")
            
            # Set edit target inside the variant and add a internal sub-root
            # reference to another prim in the same variant.
            with vset.GetVariantEditContext():
                instancePrim = stage.GetPrimAtPath("/Root/Child")
                instancePrim.GetReferences().AddInternalReference(
                    "/Root/SubrootRef", position=Usd.ListPositionFront)

            # Check that authored reference does *not* include variant
            # selection.
            childPrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Root{v=x}Child")
            expectedRefs = Sdf.ReferenceListOp()
            expectedRefs.prependedItems = [
                Sdf.Reference(primPath="/Root/SubrootRef")
            ]
            self.assertEqual(childPrimSpec.GetInfo('references'), expectedRefs)
            
if __name__ == "__main__":
    unittest.main()
