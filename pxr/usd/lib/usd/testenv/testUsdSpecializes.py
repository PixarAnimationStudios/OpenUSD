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
from pxr import Usd, Pcp, Sdf, Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdSpecializes(unittest.TestCase):
    def test_BasicApi(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            specA = stage.DefinePrim("/SpecA")
            concrete = stage.OverridePrim("/Concrete")
            items = None

            assert not concrete.HasAuthoredSpecializes()
            assert concrete.GetSpecializes().AddSpecialize(specA.GetPath())
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 1)
            self.assertEqual(concrete.GetMetadata("specializes").addedItems[0],
                        specA.GetPath())
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)
            # This will be used later in the test.
            items = concrete.GetMetadata("specializes").ApplyOperations([])

            assert concrete.GetSpecializes().RemoveSpecialize(specA.GetPath())
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").deletedItems), 1)
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)

            assert concrete.GetSpecializes().ClearSpecializes()
            assert not concrete.HasAuthoredSpecializes()
            assert not concrete.GetMetadata("specializes")

            # Set the list of added items explicitly.
            assert concrete.GetSpecializes().SetSpecializes(items)
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 1)

    def test_SpecializedPrim(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            specA = stage.CreateClassPrim("/SpecA")
            stage.DefinePrim("/SpecA/Child")

            concrete = stage.DefinePrim("/Concrete")

            assert not concrete.GetChildren() 
            assert concrete.GetSpecializes().AddSpecialize(specA.GetPath())

            self.assertEqual(concrete.GetChildren()[0].GetPath(),
                        concrete.GetPath().AppendChild("Child"))

            assert concrete.GetSpecializes().RemoveSpecialize(specA.GetPath())
            assert len(concrete.GetChildren()) == 0

    def test_SpecializesPathMapping(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt, sessionLayer=None)
            
            # Create test scenegraph 
            stage.DefinePrim("/Ref")
            stage.DefinePrim("/Ref/Class")
            stage.DefinePrim("/Ref/Instance")

            stage.DefinePrim("/Ref2")
            stage.DefinePrim("/Ref2/Class")

            stage.DefinePrim("/Class")

            prim = stage.DefinePrim("/Model")
            prim.GetReferences().AddInternalReference("/Ref")

            classPrim = stage.GetPrimAtPath("/Model/Class")
            instancePrim = stage.GetPrimAtPath("/Model/Instance")
            self.assertEqual(prim.GetChildren(), [classPrim, instancePrim])

            # Set the edit target to point to the referenced prim.
            refNode = prim.GetPrimIndex().rootNode.children[0]
            self.assertEqual(refNode.arcType, Pcp.ArcTypeReference)

            stage.SetEditTarget(
                Usd.EditTarget(refNode.layerStack.layers[0], refNode))

            # Add a specializes path to the instance prim pointing to the 
            # class prim.
            instancePrim.GetSpecializes() \
                        .AddSpecialize("/Model/Class", Usd.ListPositionFront)

            expectedSpecializePaths = Sdf.PathListOp()
            expectedSpecializePaths.prependedItems = [Sdf.Path("/Ref/Class")]

            instancePrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Ref/Instance")
            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Remove the specialize path.
            instancePrim.GetSpecializes().RemoveSpecialize(classPrim.GetPath())

            expectedSpecializePaths = Sdf.PathListOp()
            expectedSpecializePaths.deletedItems = [Sdf.Path("/Ref/Class")]
            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Add a global specialize path.
            instancePrim.GetSpecializes() \
                        .AddSpecialize("/Class", Usd.ListPositionFront)

            expectedSpecializePaths = Sdf.PathListOp()
            expectedSpecializePaths.prependedItems = [Sdf.Path("/Class")]
            expectedSpecializePaths.deletedItems = [Sdf.Path("/Ref/Class")]
            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Remove the global specialize path.
            instancePrim.GetSpecializes().RemoveSpecialize("/Class")

            expectedSpecializePaths = Sdf.PathListOp()
            expectedSpecializePaths.deletedItems = ["/Ref/Class", "/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Try to add a local specialize path pointing to a prim outside the 
            # scope of reference. This should fail because that path will not
            # map across the reference edit target.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetSpecializes() \
                            .AddSpecialize("/Ref2/Class", Usd.ListPositionFront)

            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Remove the local specialize path. This should fail and raise an
            # error again because the path will not map across the reference.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetSpecializes().RemoveSpecialize("/Ref2/Class")

            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)
            
            # Set specialize paths using the SetSpecializes API
            instancePrim.GetSpecializes().SetSpecializes(
                ["/Model/Class", "/Class"])

            expectedSpecializePaths = Sdf.PathListOp()
            expectedSpecializePaths.explicitItems = ["/Ref/Class", "/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

            # Try to set unmappable specialize paths using the 
            # SetSpecializes API, which should fail.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetSpecializes().SetSpecializes(["/Ref2/Class"])

            self.assertEqual(instancePrimSpec.GetInfo("specializes"),
                             expectedSpecializePaths)

    def test_SpecializesPathMappingVariants(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt, sessionLayer=None)

            # Create test scenegraph with variant
            refPrim = stage.DefinePrim("/Root")
            vset = refPrim.GetVariantSet("v")
            vset.AddVariant("x")
            vset.SetVariantSelection("x")
            with vset.GetVariantEditContext():
                stage.DefinePrim("/Root/Class")
                stage.DefinePrim("/Root/Instance")

            # Set edit target inside the variant and add a specializes
            # to another prim in the same variant.
            with vset.GetVariantEditContext():
                instancePrim = stage.GetPrimAtPath("/Root/Instance")
                instancePrim.GetSpecializes().AddSpecialize(
                    "/Root/Class", Usd.ListPositionFront)

            # Check that authored specializes path does *not* include variant
            # selection.
            instancePrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Root{v=x}Instance")
            expectedSpecializes = Sdf.PathListOp()
            expectedSpecializes.prependedItems = [Sdf.Path("/Root/Class")]
            self.assertEqual(instancePrimSpec.GetInfo('specializes'),
                             expectedSpecializes)

if __name__ == '__main__':
    unittest.main()
