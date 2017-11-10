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

class TestUsdInherits(unittest.TestCase):
    def test_BasicApi(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            classA = stage.CreateClassPrim("/ClassA")
            concrete = stage.OverridePrim("/Concrete")
            items = None

            assert not concrete.HasAuthoredInherits()
            assert concrete.GetInherits().AddInherit(classA.GetPath())
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 1)
            self.assertEqual(concrete.GetMetadata("inheritPaths").prependedItems[0],
                        classA.GetPath())
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)
            # This will be used later in the test.
            items = concrete.GetMetadata("inheritPaths").prependedItems

            assert concrete.GetInherits().RemoveInherit(classA.GetPath())
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 1)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)

            assert concrete.GetInherits().ClearInherits()
            assert not concrete.HasAuthoredInherits()
            assert not concrete.GetMetadata("inheritPaths")

            # Set the list of added items explicitly.
            assert concrete.GetInherits().SetInherits(items)
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 1)


    def test_InheritedPrim(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            classA = stage.CreateClassPrim("/ClassA")
            stage.DefinePrim("/ClassA/Child")

            concrete = stage.DefinePrim("/Concrete")

            assert not concrete.GetChildren() 
            assert concrete.GetInherits().AddInherit(classA.GetPath())

            self.assertEqual(concrete.GetChildren()[0].GetPath(),
                        concrete.GetPath().AppendChild("Child"))

            assert concrete.GetInherits().RemoveInherit(classA.GetPath())
            assert len(concrete.GetChildren()) == 0

    def test_InheritPathMapping(self):
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

            # Add an inherit path to the instance prim pointing to the 
            # class prim.
            instancePrim.GetInherits() \
                        .AddInherit("/Model/Class", Usd.ListPositionFront)

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.prependedItems = [Sdf.Path("/Ref/Class")]

            instancePrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Ref/Instance")
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Remove the inherit path.
            instancePrim.GetInherits().RemoveInherit(classPrim.GetPath())

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.deletedItems = [Sdf.Path("/Ref/Class")]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Add a global inherit path.
            instancePrim.GetInherits() \
                        .AddInherit("/Class", Usd.ListPositionFront)

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.prependedItems = [Sdf.Path("/Class")]
            expectedInheritPaths.deletedItems = [Sdf.Path("/Ref/Class")]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Remove the global inherit path.
            instancePrim.GetInherits().RemoveInherit("/Class")

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.deletedItems = ["/Ref/Class", "/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Try to add a local inherit path pointing to a prim outside the 
            # scope of reference. This should fail because that path will not
            # map across the reference edit target.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetInherits() \
                            .AddInherit("/Ref2/Class", Usd.ListPositionFront)

            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Remove the local inherit path. This should fail and raise an
            # error again because the path will not map across the reference.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetInherits().RemoveInherit("/Ref2/Class")

            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)
            
            # Set inherit paths using the SetInherits API
            instancePrim.GetInherits().SetInherits(
                ["/Model/Class", "/Class"])

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.explicitItems = ["/Ref/Class", "/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Try to set unmappable inherit paths using the SetInherits API,
            # which should fail.
            with self.assertRaises(Tf.ErrorException):
                instancePrim.GetInherits().SetInherits(["/Ref2/Class"])

            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

    def test_InheritPathMappingVariants(self):
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

            # Set edit target inside the variant and add an inherit
            # to another prim in the same variant.
            with vset.GetVariantEditContext():
                instancePrim = stage.GetPrimAtPath("/Root/Instance")
                instancePrim.GetInherits().AddInherit(
                    "/Root/Class", Usd.ListPositionFront)

            # Check that authored inherit path does *not* include variant
            # selection.
            instancePrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Root{v=x}Instance")
            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.prependedItems = [Sdf.Path("/Root/Class")]
            self.assertEqual(instancePrimSpec.GetInfo('inheritPaths'),
                             expectedInheritPaths)

if __name__ == '__main__':
    unittest.main()
