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

            # Set the list of added items to explicitly empty. The metadata will
            # still exist as an explicitly empty list op.
            assert concrete.GetInherits().SetInherits([])
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)

            # Clear the inherits. Still empty but no longer explicit.
            assert concrete.GetInherits().ClearInherits()
            assert not concrete.HasAuthoredInherits()
            assert not concrete.GetMetadata("inheritPaths")

            # Set the list of added items to explicitly empty again from cleared
            # verifying that it is indeed set to explicit.
            assert concrete.GetInherits().SetInherits([])
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)


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

            self.assertEqual(concrete.GetInherits().GetAllDirectInherits(),
                             ['/ClassA'])

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
                        .AddInherit("/Model/Class", Usd.ListPositionFrontOfPrependList)

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
                        .AddInherit("/Class", Usd.ListPositionFrontOfPrependList)

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

            # Add a local inherit path pointing to a prim outside the 
            # scope of reference.  This is allowed, because unlike
            # external references, internal references do not
            # encapsulate namespace.
            instancePrim.GetInherits() \
                        .AddInherit("/Ref2/Class", Usd.ListPositionFrontOfPrependList)

            expectedInheritPaths.prependedItems = ["/Ref2/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Remove the local inherit path.
            instancePrim.GetInherits().RemoveInherit("/Ref2/Class")

            expectedInheritPaths.deletedItems = ["/Ref/Class", "/Class", "/Ref2/Class"]
            expectedInheritPaths.prependedItems = []
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)
            
            # Set inherit paths using the SetInherits API
            instancePrim.GetInherits().SetInherits(
                ["/Model/Class", "/Class"])

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.explicitItems = ["/Ref/Class", "/Class"]
            self.assertEqual(instancePrimSpec.GetInfo("inheritPaths"),
                             expectedInheritPaths)

            # Try to set inherit paths using the SetInherits API.
            instancePrim.GetInherits().SetInherits(["/Ref2/Class"])

            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.explicitItems = ["/Ref2/Class"]
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
                    "/Root/Class", Usd.ListPositionFrontOfPrependList)

            # Check that authored inherit path does *not* include variant
            # selection.
            instancePrimSpec = \
                stage.GetRootLayer().GetPrimAtPath("/Root{v=x}Instance")
            expectedInheritPaths = Sdf.PathListOp()
            expectedInheritPaths.prependedItems = [Sdf.Path("/Root/Class")]
            self.assertEqual(instancePrimSpec.GetInfo('inheritPaths'),
                             expectedInheritPaths)

    def test_GetAllDirectInherits(self):
        for fmt in allFormats:
            # Layer to hold specs for arcs that will be introduced via 
            # reference.
            refLayer = Sdf.Layer.CreateAnonymous('r.'+fmt)
            refLayer.ImportFromString("""#usda 1.0
                # Target of a reference arc in the root's /Parent
                over "PR" (
                    prepend specializes = </PRS>
                ) {
                    over "Child" (
                        prepend inherits = [
                            </PRCI>,
                            </PRCI_NOSPEC>,
                            </PR/Sibling>
                        ]
                    ) {}
    
                    over "Sibling" {}
                }
    
                # Target of the specializes arc in PR
                over "PRS" (
                    prepend inherits = [
                        </PRSI>,
                        </PRSI_NOSPEC>
                    ]
                ) {}
    
                # Target of a reference arc in an inherited class of root's 
                # /Parent    
                over "PIR" (
                    prepend inherits = [</PIRI>, </PIRI_NOSPEC>]
                ) {}
            
                # Target of a reference arc of root's /Parent/Child
                over "CR" (
                    prepend inherits = [</CRI>, </CRI_NOSPEC>]
                ) {}
                """)

            # Create a simple prim hierarchy, /Parent/Child, then add some arcs.
            rootLayer = Sdf.Layer.CreateAnonymous('r.'+fmt)
            rootLayer.ImportFromString("""#usda 1.0
                def "Parent" (
                    inherits = </PI>
                    references = @__REF_LAYER__@</PR>
                    specializes = </PS>
                ) {
                    def "Child" (
                        inherits = </CI>
                        references = @__REF_LAYER__@</CR>
                    ) {}
                }

                # Inherited by /Parent
                over "PI" (
                    prepend references = @__REF_LAYER__@</PIR>
                    prepend specializes = </PIS>
                ) {}

                # Target of specializes in PI which is inherited by /Parent
                over "PIS" (
                    prepend inherits = </PISI>
                ) {}

                # Target of inherits in PIS 
                over "PISI" {}

                # Target of specializes in /Parent
                over "PS" (
                    prepend inherits = </PSI>
                ) {}

                # Target of inherits in PS which is specialized by /Parent
                over "PSI" {}

                # Inherited by /Parent/Child
                over "CI" {}

                # Specs for implied inherit targets introduced in by references
                # to the refLayer.
                over "PRCI" {}
                over "PRSI" {}
                over "CRI" {}
                over "PIRI" {
                    over "Child" {}
                }
                """.replace("__REF_LAYER__", refLayer.identifier))

            stage = Usd.Stage.Open(rootLayer, sessionLayer=None)

            parent = stage.GetPrimAtPath('/Parent')
            child = stage.GetPrimAtPath('/Parent/Child')

            # Now check that the direct inherits are what we expect.
            self.assertEqual(parent.GetInherits().GetAllDirectInherits(),
                [Sdf.Path(path) for path in [
                    '/PI', '/PIRI', '/PIRI_NOSPEC', '/PISI', '/PRSI',
                    '/PRSI_NOSPEC', '/PSI']])

            self.assertEqual(child.GetInherits().GetAllDirectInherits(),
                [Sdf.Path(path) for path in [
                    '/CI', '/CRI', '/CRI_NOSPEC', '/PRCI', '/PRCI_NOSPEC',
                    '/Parent/Sibling', '/PISI/Sibling', '/PSI/Sibling',
                ]])

    def test_ListPosition(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt, sessionLayer=None)

            prim = stage.DefinePrim('/prim')
            for c in 'abcde':
                stage.DefinePrim('/'+c)

            inh = prim.GetInherits()

            # Default behavior: ListPositionBackOfPrependList
            self.assertEqual(inh.GetAllDirectInherits(), [])
            inh.AddInherit('/a')
            self.assertEqual(inh.GetAllDirectInherits(), ['/a'])
            inh.AddInherit('/b')
            self.assertEqual(inh.GetAllDirectInherits(), ['/a', '/b'])

            # ListPositionFrontOfPrependList
            inh.AddInherit('/c', Usd.ListPositionFrontOfPrependList)
            self.assertEqual(inh.GetAllDirectInherits(),
                    ['/c', '/a', '/b'])

            # Adding a redundant entry moves it to the requested position
            inh.AddInherit('/a', Usd.ListPositionFrontOfPrependList)
            self.assertEqual(inh.GetAllDirectInherits(),
                    ['/a', '/c', '/b'])

            # ListPositionBackOfAppendList
            inh.AddInherit('/d', Usd.ListPositionBackOfAppendList)
            self.assertEqual(inh.GetAllDirectInherits(),
                    ['/a', '/c', '/b', '/d'])

            # ListPositionFrontOfAppendList
            inh.AddInherit('/e', Usd.ListPositionFrontOfAppendList)
            self.assertEqual(inh.GetAllDirectInherits(),
                    ['/a', '/c', '/b', '/e', '/d'])

if __name__ == '__main__':
    unittest.main()
