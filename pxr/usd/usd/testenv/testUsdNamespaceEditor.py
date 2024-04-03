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

import sys, os, unittest
from pxr import Sdf, Usd, Tf, Plug

class TestUsdNamespaceEditor(unittest.TestCase):

    # Verifies children, properties, and metadata of the prim named "A" when
    # the basic test stage is loaded.
    def _VerifyBasicStagePrimAValues(self, prim):
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), 'PrimTypeA')
        self.assertTrue(prim.GetChild("A_Root_Prim"))
        self.assertFalse(prim.GetChild("A_Sub1_Prim"))
        self.assertFalse(prim.GetChild("A_Sub2_Prim"))
        self.assertEqual(prim.GetPropertyNames(), ['A_Root_Attr'])
        self.assertEqual(prim.GetMetadata("documentation"), "Root")
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(),
            ["A_Root_API"])
            
    # Verifies children (other than "A"), properties, and metadata of the prim 
    # named "B" when the basic test stage is loaded. This function specifically
    # doesn't check for a child named "A" so that this function can still 
    # used after namespace operations in the following tests that will move "A".
    def _VerifyBasicStagePrimBValues(self, prim):
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), 'PrimTypeB')
        self.assertTrue(prim.GetChild("B_Root_Prim"))
        self.assertTrue(prim.GetChild("B_Sub1_Prim"))
        self.assertFalse(prim.GetChild("B_Sub2_Prim"))
        self.assertEqual(prim.GetPropertyNames(), 
            ['B_Root_Attr', 'B_Sub1_Attr'])
        self.assertEqual(prim.GetMetadata("documentation"), "Root")
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["B_Root_API", "B_Sub1_API"])
    
    # Verifies children (other than "B"), properties, and metadata of the prim 
    # named "B" when the basic test stage is loaded. This function specifically
    # doesn't check for a child named "B" so that this function can still 
    # used after namespace operations in the following tests that will move "B".
    def _VerifyBasicStagePrimCValues(self, prim):
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), 'PrimTypeC')
        self.assertTrue(prim.GetChild("C_Root_Prim"))
        self.assertTrue(prim.GetChild("C_Sub1_Prim"))
        self.assertTrue(prim.GetChild("C_Sub2_Prim"))
        self.assertEqual(prim.GetPropertyNames(), 
            ['C_Root_Attr', 'C_Sub1_Attr', 'C_Sub2_Attr'])
        self.assertEqual(prim.GetMetadata("documentation"), "Root")
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["C_Root_API", "C_Sub1_API", "C_Sub2_API"])

    # Verifies that the result is false and provides the expected message.
    def _VerifyFalseResult(self, result, expectedMessage):
        self.assertFalse(result)
        self.assertEqual(result.whyNot, expectedMessage)

    # Setup function for many of the basic test cases. Loads the "basic" stage
    # and gets and returns the needed layers and initial prims from this stage.
    # Also verifies the initial state of the stage and its prims match the 
    # expected unedited state of the stage.
    def _SetupBasicStage(self):
        # Open the root layer
        stage = Usd.Stage.Open("basic/root.usda")
        self.assertTrue(stage)
        
        # Get the layers that the stage should have opened.
        rootLayer = Sdf.Layer.Find("basic/root.usda")
        subLayer1 = Sdf.Layer.Find("basic/sub_1.usda")
        subLayer2 = Sdf.Layer.Find("basic/sub_2.usda")
        self.assertTrue(rootLayer)
        self.assertTrue(subLayer1)
        self.assertTrue(subLayer2)
        
        # The stage defines three prims.
        primA = stage.GetPrimAtPath("/C/B/A")
        primB = stage.GetPrimAtPath("/C/B")
        primC = stage.GetPrimAtPath("/C")
        self.assertTrue(primA)
        self.assertTrue(primB)
        self.assertTrue(primC)
    
        # Verify the expected data of these prims.
        self._VerifyBasicStagePrimAValues(primA)
        self._VerifyBasicStagePrimBValues(primB)
        self._VerifyBasicStagePrimCValues(primC)
    
        # Assert the expected specs comprising these prims
        # /A is only defined in the root layer
        self.assertEqual(primA.GetPrimStack(),
            [rootLayer.GetPrimAtPath("/C/B/A")])
        # /B is defined in the root and sub_1 layers
        self.assertEqual(primB.GetPrimStack(),
            [rootLayer.GetPrimAtPath("/C/B"),
            subLayer1.GetPrimAtPath("/C/B")])
        # /C is defined in the root and both sub layers
        self.assertEqual(primC.GetPrimStack(),
            [rootLayer.GetPrimAtPath("/C"),
            subLayer1.GetPrimAtPath("/C"),
            subLayer2.GetPrimAtPath("/C")])
        
        return (stage, rootLayer, subLayer1, subLayer2, primA, primB, primC)

    def test_BasicDeletePrim(self):
        """Test basic USD prim deletion through the UsdNamespaceEditor API.
        """

        # This function allows the same tests to be performed using DeletePrim 
        # and DeletePrimAtPath.
        def _TestDeletePrim(useDeletePrimAtPath):
            
            # Open the basic stage and the expected layers and prims that we're
            # working with.
            stage, rootLayer, subLayer1, subLayer2, primA, primB, primC = \
                self._SetupBasicStage()
        
            editor = Usd.NamespaceEditor(stage)
            
            # Delete "/C/B/A"
            self.assertTrue(primA)
            if useDeletePrimAtPath:
                self.assertTrue(editor.DeletePrimAtPath("/C/B/A"))
            else:
                self.assertTrue(editor.DeletePrim(primA))
            self.assertTrue(primA)

            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primA)
            self.assertFalse(stage.GetPrimAtPath("/C/B/A"))
            
            # Delete "/C/B"
            self.assertTrue(primB)
            if useDeletePrimAtPath:
                self.assertTrue(editor.DeletePrimAtPath("/C/B"))
            else:
                self.assertTrue(editor.DeletePrim(primB))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primB)
            self.assertFalse(stage.GetPrimAtPath("/C/B"))
            
            # Delete "/C"
            self.assertTrue(primC)
            if useDeletePrimAtPath:
                self.assertTrue(editor.DeletePrimAtPath("/C"))
            else:
                self.assertTrue(editor.DeletePrim(primC))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            self.assertFalse(stage.GetPrimAtPath("/C"))

            # Reset the stage for the next case.
            stage.Reload()
            primC = stage.GetPrimAtPath("/C")
            primB = stage.GetPrimAtPath("/C/B")
            primA = stage.GetPrimAtPath("/C/B/A")
            self.assertTrue(primC)
            self.assertTrue(primB)
            self.assertTrue(primA)

            # Delete "/C" again. It is deleted along with its children
            if useDeletePrimAtPath:
                self.assertTrue(editor.DeletePrimAtPath("/C"))
            else:
                self.assertTrue(editor.DeletePrim(primC))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            self.assertFalse(primB)
            self.assertFalse(primA)
            self.assertFalse(stage.GetPrimAtPath("/C"))
            self.assertFalse(stage.GetPrimAtPath("/C/B"))
            self.assertFalse(stage.GetPrimAtPath("/C/B/A"))

        # Run delete tests using both DeletePrim and DeletePrimAtPath
        _TestDeletePrim(useDeletePrimAtPath=False)
        _TestDeletePrim(useDeletePrimAtPath=True)

    def test_BasicRenamePrim(self):
        """Test basic USD prim renaming through the UsdNamespaceEditor API.
        """

        # This function allows the same tests to be performed using RenamePrim 
        # and MovePrimAtPath.
        def _TestRenamePrim(useMovePrimAtPath):

            # Open the basic stage and the expected layers and prims that we're
            # working with.
            stage, rootLayer, subLayer1, subLayer2, primA, primB, primC = \
                self._SetupBasicStage()
                
            # Before rename, verify there are no prims at the paths we intend to
            # rename to.
            self.assertFalse(stage.GetPrimAtPath("/C/B/NewA"))
            self.assertFalse(stage.GetPrimAtPath("/C/NewB"))
            self.assertFalse(stage.GetPrimAtPath("/NewC"))
            
            editor = Usd.NamespaceEditor(stage)
            
            # Rename "/C/B/A" to "/C/B/NewA"
            if useMovePrimAtPath:
                self.assertTrue(
                    editor.MovePrimAtPath("/C/B/A", "/C/B/NewA"))
            else:        
                self.assertTrue(editor.RenamePrim(primA, "NewA"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/C/B/NewA")
            self._VerifyBasicStagePrimAValues(primA)
            
            # Assert the expected specs comprising NewA
            # /C/B/A is renamed to /C/B/NewA in the in only the root layer which
            # is the only layer where it was defined
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B/NewA")])
            
            # Rename "/C/B" to "/C/NewB"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/C/B", "/C/NewB"))
            else:        
                self.assertTrue(editor.RenamePrim(primB, "NewB"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primB)
            primB = stage.GetPrimAtPath("/C/NewB")
            self._VerifyBasicStagePrimBValues(primB)
            
            # Assert the expected specs comprising NewB
            # /C/B is renamed to /C/NewB in both the root and sub_1 layers where
            # it had specs defined.
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/NewB"),
                subLayer1.GetPrimAtPath("/C/NewB")])
            
            # NewA is still a child of /C/NewB after the rename of /C/B and its
            # spec in the root layer is moved to the new path.
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/C/NewB/NewA")
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/NewB/NewA")])
            
            # Rename "/C" to "/NewC"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/C", "/NewC"))
            else:        
                self.assertTrue(editor.RenamePrim(primC, "NewC"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            primC = stage.GetPrimAtPath("/NewC")
            self._VerifyBasicStagePrimCValues(primC)
        
            # Assert the expected specs comprising NewB
            # /C is renamed to /NewC in the root and both sublayers where
            # it had specs defined.
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewC"),
                subLayer1.GetPrimAtPath("/NewC"),
                subLayer2.GetPrimAtPath("/NewC")])
            
            # NewA and NewB are still descendants of /NewC after the rename of 
            # /C and all their specs are moved to the new path.
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/NewC/NewB/NewA")
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewC/NewB/NewA")])
            
            self.assertFalse(primB)
            primB = stage.GetPrimAtPath("/NewC/NewB")
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewC/NewB"),
                subLayer1.GetPrimAtPath("/NewC/NewB")])
            
        # Run rename tests using both RenamePrim and RenamePrimAtPath
        _TestRenamePrim(useMovePrimAtPath=False)
        _TestRenamePrim(useMovePrimAtPath=True)

    def test_BasicReparentPrim(self):
        """Test basic USD prim reparenting through the UsdNamespaceEditor API
        without renaming the prims.
        """

        # This function allows the same tests to be performed using ReparentPrim 
        # and MovePrimAtPath.
        def _TestReparentPrim(useMovePrimAtPath):
            
            # Open the basic stage and the expected layers and prims that we're
            # working with.
            stage, rootLayer, subLayer1, subLayer2, primA, primB, primC = \
                self._SetupBasicStage()
        
            # Before reparent, verify there are no prims at the paths we intend
            # to reparent to.
            self.assertFalse(stage.GetPrimAtPath("/A"))
            self.assertFalse(stage.GetPrimAtPath("/A/B"))
            self.assertFalse(stage.GetPrimAtPath("/A/B/C"))
            
            editor = Usd.NamespaceEditor(stage)
            
            # Reparent "/C/B/A" to be "/A"
            if useMovePrimAtPath:
                self.assertTrue(
                    editor.MovePrimAtPath("/C/B/A", "/A"))
            else:
                self.assertTrue(
                    editor.ReparentPrim(primA, stage.GetPseudoRoot()))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/A")
            self._VerifyBasicStagePrimAValues(primA)
            
            # Assert the expected specs comprising the moved prims.
            # /C/B/A is moved to /A in the in the root layer only as that's the
            # only layer it was originally defined in.
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A")])
            
            # Assert that primB and primC are still valid and their prim specs
            # have not been moved in the layers they are defined.
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B"),
                subLayer1.GetPrimAtPath("/C/B")])
                
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])
            
            # Reparent "/C/B" to be "/A/B"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/C/B", "/A/B"))
            else:
                self.assertTrue(editor.ReparentPrim(primB, primA))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primB)
            primB = stage.GetPrimAtPath("/A/B")
            self._VerifyBasicStagePrimBValues(primB)
            
            # Assert the expected specs comprising the moved prims.
            # /C/B is moved to /A/B in the in the root and sub_1 layers as those
            # are the layers where its original specs were defined.
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A/B"),
                subLayer1.GetPrimAtPath("/A/B")])
            
            # Assert that primA is still valid but now has an additional prim
            # spec sub_1 which is needed as a parent for the moved /A/B spec.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A"),
                subLayer1.GetPrimAtPath("/A")])
                
            # Assert that primC is still valid and has unchanged prim specs
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])
            
            # Reparent "/C" to be "/A/B/C"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/C", "/A/B/C"))
            else:
                self.assertTrue(editor.ReparentPrim(primC, primB))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            primC = stage.GetPrimAtPath("/A/B/C")
            self._VerifyBasicStagePrimCValues(primC)
            
            # Assert the expected specs comprising the moved prims.
            # /C is moved to /A/B/C in the root and both sub layers
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A/B/C"),
                subLayer1.GetPrimAtPath("/A/B/C"),
                subLayer2.GetPrimAtPath("/A/B/C")])
            
            # Assert that primA is still valid but now has an additional prim 
            # spec in both sublayers which is needed as a parent for the moved 
            # /A/B and /A/B/C specs.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A"),
                subLayer1.GetPrimAtPath("/A"),
                subLayer2.GetPrimAtPath("/A")])
            
            # Assert that primB is still valid but now has an additional prim 
            # spec in sub_2 which is needed as a parent for the moved 
            # /A/B/C spec.
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A/B"),
                subLayer1.GetPrimAtPath("/A/B"),
                subLayer2.GetPrimAtPath("/A/B")])

            # Reverse the reparent operations. This is to verify the behavior 
            # regarding the overs that were created to allow child specs to be 
            # moved in all layers.

            # Reparent "/A/B/C" back to being "/C"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/A/B/C", "/C"))
            else:
                self.assertTrue(
                    editor.ReparentPrim(primC, stage.GetPseudoRoot()))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            primC = stage.GetPrimAtPath("/C")
            self._VerifyBasicStagePrimCValues(primC)

            # Assert the expected specs comprising /A/B/C
            # is moved back to /C in the root and both sub layers
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])

            # Assert that primA is still valid but now the "inert" prim spec in 
            # sub_2 has been removed since it is no longer needed as an ancestor
            # for /A/B/C.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A"),
                subLayer1.GetPrimAtPath("/A")])
            
            # Assert that primB is still valid but now the "inert" prim spec in 
            # sub_2 has been removed since it is no longer needed as a parent 
            # for /A/B/C.
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A/B"),
                subLayer1.GetPrimAtPath("/A/B")])

            # Reparent "/A/B" back to being "/C/B"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/A/B", "/C/B"))
            else:
                self.assertTrue(editor.ReparentPrim(primB, primC))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primB)
            primB = stage.GetPrimAtPath("/C/B")
            self._VerifyBasicStagePrimBValues(primB)
            
            # Assert the expected specs comprising 
            # /A/B are moved back to /C/B in the in the root and sub_1 layers
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B"),
                subLayer1.GetPrimAtPath("/C/B")])
            
            # Assert that primA is still valid but now the "inert" prim spec in
            # sub_1 has also been removed since it is no longer needed as an 
            # ancestor for /A/B.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/A")])
                
            # Assert that primC is still valid and has unchanged prim specs
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])

            # Reparent "/A" back to being "/C/B/A"
            if useMovePrimAtPath:
                self.assertTrue(editor.MovePrimAtPath("/A", "/C/B/A"))
            else:
                self.assertTrue(editor.ReparentPrim(primA, primB))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/C/B/A")
            self._VerifyBasicStagePrimAValues(primA)
            
            # Assert the expected specs comprising 
            # /A is moved back to /C/B/A in the in the root layer
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B/A")])
            
            # Verify that primB and primC are still valid and have unchanged 
            # prim specs
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B"),
                subLayer1.GetPrimAtPath("/C/B")])
                
            # Assert that primC is still valid and has unchanged prim specs
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])

        # Run reparent tests using both ReparentPrim and MovePrimAtPath
        _TestReparentPrim(useMovePrimAtPath=False)
        _TestReparentPrim(useMovePrimAtPath=True)

    def test_BasicReparentAndRenamePrim(self):
        """Test basic USD prim reparenting through the UsdNamespaceEditor API
        using the overload that renames the prim as well.
        """

        # This function allows the same tests to be performed using ReparentPrim 
        # and MovePrimAtPath.
        def _TestReparentAndRenamePrim(useMovePrimAtPath):
            
            # Open the basic stage and the expected layers and prims that we're
            # working with.
            stage, rootLayer, subLayer1, subLayer2, primA, primB, primC = \
                self._SetupBasicStage()
        
            # Before rename, verify there are no prims at the paths we intend to
            # reparent to.
            self.assertFalse(stage.GetPrimAtPath("/NewA"))
            self.assertFalse(stage.GetPrimAtPath("/NewA/NewB"))
            self.assertFalse(stage.GetPrimAtPath("/NewA/NewB/NewC"))
            
            editor = Usd.NamespaceEditor(stage)
            
            # Reparent and rename "/C/B/A" to be "/NewA"
            if useMovePrimAtPath:
                self.assertTrue(
                    editor.MovePrimAtPath("/C/B/A", "/NewA"))
            else:
                self.assertTrue(
                    editor.ReparentPrim(primA, stage.GetPseudoRoot(), "NewA"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primA)
            primA = stage.GetPrimAtPath("/NewA")
            self._VerifyBasicStagePrimAValues(primA)
            
            # Assert the expected specs comprising the moved prims.
            # /C/B/A is moved to /NewA in the in the root layer only as that's 
            # the only layer it was originally defined in.
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA")])
            
            # Assert that primB and primC are still valid and their prim specs
            # have not been moved in the layers they are defined.
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C/B"),
                subLayer1.GetPrimAtPath("/C/B")])
                
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])
            
            # Reparent and rename "/C/B" to be "/NewA/NewB"
            if useMovePrimAtPath:
                self.assertTrue(
                    editor.MovePrimAtPath("/C/B", "/NewA/NewB"))
            else:
                self.assertTrue(editor.ReparentPrim(primB, primA, "NewB"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primB)
            primB = stage.GetPrimAtPath("/NewA/NewB")
            self._VerifyBasicStagePrimBValues(primB)
            
            # Assert the expected specs comprising the moved prims.
            # /C/B is moved to /NewA/NewB in the in the root and sub_1 layers as
            # those are the layers where its original specs were defined.
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA/NewB"),
                subLayer1.GetPrimAtPath("/NewA/NewB")])
            
            # Assert that primA is still valid but now has an additional prim 
            # spec sub_1 which is needed as a parent for the moved /NewA/NewB 
            # spec.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA"),
                subLayer1.GetPrimAtPath("/NewA")])
                
            # Assert that primC is still valid and has unchanged prim specs
            self._VerifyBasicStagePrimCValues(primC)
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/C"),
                subLayer1.GetPrimAtPath("/C"),
                subLayer2.GetPrimAtPath("/C")])
            
            # Reparent and rename "/C" to be "/NewA/NewB/NewC"
            if useMovePrimAtPath:
                self.assertTrue(
                    editor.MovePrimAtPath("/C", "/NewA/NewB/NewC"))
            else:
                self.assertTrue(editor.ReparentPrim(primC, primB, "NewC"))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(primC)
            primC = stage.GetPrimAtPath("/NewA/NewB/NewC")
            self._VerifyBasicStagePrimCValues(primC)
            
            # Assert the expected specs comprising these prims
            # /C is moved to /NewA/NewB/NewC in the root and both sub layers
            self.assertEqual(primC.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA/NewB/NewC"),
                subLayer1.GetPrimAtPath("/NewA/NewB/NewC"),
                subLayer2.GetPrimAtPath("/NewA/NewB/NewC")])
            
            # Assert that primA is still valid but now has an additional prim 
            # spec in both sublayers which is needed as a parent for the moved 
            # /NewA/NewB and /NewA/NewB/NewC specs.
            self._VerifyBasicStagePrimAValues(primA)
            self.assertEqual(primA.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA"),
                subLayer1.GetPrimAtPath("/NewA"),
                subLayer2.GetPrimAtPath("/NewA")])
            
            # Assert that primB is still valid but now has an additional prim 
            # spec in both sub_2 which is needed as a parent for the moved 
            # /NewA/NewB/NewC spec.
            self._VerifyBasicStagePrimBValues(primB)
            self.assertEqual(primB.GetPrimStack(),
                [rootLayer.GetPrimAtPath("/NewA/NewB"),
                subLayer1.GetPrimAtPath("/NewA/NewB"),
                subLayer2.GetPrimAtPath("/NewA/NewB")])

        # Run reparent and rename tests using both ReparentPrim and 
        # MovePrimAtPath
        _TestReparentAndRenamePrim(useMovePrimAtPath=False)
        _TestReparentAndRenamePrim(useMovePrimAtPath=True)

    def test_BasicCanEditPrim(self):
        """Tests the basic usage of the CanApplyEdits in cases where namespace
        editing should fail. Also tests that calling ApplyEdits in cases where
        CanApplyEdits returns false will not change any layer content.
        """

        # Load the basic stage and get its layers and prims 
        stage, rootLayer, subLayer1, subLayer2, primA, primB, primC = \
            self._SetupBasicStage()

        editor = Usd.NamespaceEditor(stage)

        # Helper to verify that none of the stage's layers have changed after
        # an expected failed edit.
        def _VerifyNoLayersHaveChanged():
            self.assertFalse(rootLayer.dirty)
            self.assertFalse(subLayer1.dirty)
            self.assertFalse(subLayer2.dirty)

        # Helper to verify that CanApplyEdits returns false with the expected
        # message and that ApplyEdits fails and doesn't edit any layers.
        def _VerifyCannotApplyEdits(expectedMessage):
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()
            _VerifyNoLayersHaveChanged()

        # Helpers to verify that a valid edit operation (of the function's 
        # particular type) can be added but cannot be applied to the editor's 
        # stage for the expected reason.
        def _VerifyCannotApplyDeletePrim(prim, expectedMessage):
            self.assertTrue(editor.DeletePrim(prim))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyRenamePrim(prim, newName, expectedMessage):
            self.assertTrue(editor.RenamePrim(prim, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyReparentPrim(prim, newParent, expectedMessage):
            self.assertTrue(editor.ReparentPrim(prim, newParent))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyReparentAndRenamePrim(
                prim, newParent, newName, expectedMessage):
            self.assertTrue(editor.ReparentPrim(prim, newParent, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyDeletePrimAtPath(path, expectedMessage):
            self.assertTrue(editor.DeletePrimAtPath(path))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyMovePrimAtPath(path, newPath, expectedMessage):
            self.assertTrue(editor.MovePrimAtPath(path, newPath))
            _VerifyCannotApplyEdits(expectedMessage)
        
        # First verify we can't apply edits if we haven't added any yet.
        _VerifyCannotApplyEdits("There are no valid edits to perform")

        # Test invalid edit operations that can't even be added because they
        # do not produce valid absolute prim paths.
            
        # Test valid SdfPaths that are not absolute prim paths. These non-prim 
        # paths will emit a coding error if provided to any of the add prim
        # edit at path functions.
        invalidPrimPaths = [
            "/C.C_Root_Attr", # property
            "/C{x=y}", # variant selection
            "/C{x=y}B", # prim path with ancestor variant selection
            "/", # pseudo-root
            "C", # relative prim path
            ".", # relative prim path
            "..", # relative prim path
            "../B", # relative prim path
            ".C_Root_Attr", # relative property path
            "/C/B.B_Rel[/Foo].attr" # target path
        ]
        for pathString in invalidPrimPaths:
            # Verify it is a valid SdfPath
            path = Sdf.Path(pathString)
            self.assertFalse(path.isEmpty)
            
            # Cannot use path for delete.
            with self.assertRaises(Tf.ErrorException):
                editor.DeletePrimAtPath(path)
            # Cannot use path for source or destination of move
            with self.assertRaises(Tf.ErrorException):
                editor.MovePrimAtPath(path, "/Foo")
            with self.assertRaises(Tf.ErrorException):
                editor.MovePrimAtPath("/C", path)

        # Test fully invalid SdfPaths that will also emit a coding error if 
        #provided to any of the add prim edit at path functions.
        invalidPaths = [
            "",
            "//A",
            "123",
            "$",
            "/Foo:Bar",
            "/.foo"
        ]
        for pathString in invalidPaths:
            # Verify it is a valid SdfPath
            path = Sdf.Path(pathString)
            self.assertTrue(path.isEmpty)
            
            # Cannot use path for delete.
            with self.assertRaises(Tf.ErrorException):
                editor.DeletePrimAtPath(path)
            # Cannot use path for source or destination of move
            with self.assertRaises(Tf.ErrorException):
                editor.MovePrimAtPath(path, "/Foo")
            with self.assertRaises(Tf.ErrorException):
                editor.MovePrimAtPath("/C", path)
        
        # GetPrimAtPath('/A') does not return a valid prim. We cannot add edits for
        # this invalid prim.
        invalidPrim = stage.GetPrimAtPath("/Bogus")
        self.assertFalse(invalidPrim)
        with self.assertRaises(Tf.ErrorException):
            editor.DeletePrim(invalidPrim)
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(invalidPrim, "NewA")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(invalidPrim, primC)
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primC, invalidPrim)

        # We cannot add edits for the empty prim.
        invalidPrim = Usd.Prim()
        self.assertFalse(invalidPrim)
        with self.assertRaises(Tf.ErrorException):
            editor.DeletePrim(invalidPrim)
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(invalidPrim, "NewA")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(invalidPrim, primC)
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primC, invalidPrim)

        # We cannot add rename edits with invalid prim names.

        # Empty name
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(primA, "")
        # Invalid prim name
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(primA, "/A")
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(primA, "C.foo")
        with self.assertRaises(Tf.ErrorException):
            editor.RenamePrim(primA, "C{x=y}")

        # We cannot add reparent edits with a new name that isn't a valid prim name.

        # Empty name when reparenting with a new name
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primA, primC, "")
        # Invalid prim name when reparenting with a new name
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primA, primC, "/A")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primA, primC, "C.foo")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primA, primC, "C{x=y}")

        # At this point we've only failed to add any edits so we still cannot apply
        # edits since there are no edits to perform        
        _VerifyCannotApplyEdits("There are no valid edits to perform")

        # /A is not a prim on the stage. We cannot edit a prim at this path.
        _VerifyCannotApplyDeletePrimAtPath("/A", 
            "The prim to edit is not a valid prim")
        _VerifyCannotApplyMovePrimAtPath("/A", "/NewA", 
            "The prim to edit is not a valid prim")
        _VerifyCannotApplyMovePrimAtPath("/A", "/C/A", 
            "The prim to edit is not a valid prim")

        # Renames fail when an object already exists at the renamed path.
        _VerifyCannotApplyRenamePrim(primB, "C_Root_Prim", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePrimAtPath("/C/B", "/C/C_Root_Prim", 
            "An object already exists at the new path")

        _VerifyCannotApplyRenamePrim(primA, "B_Sub1_Prim", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePrimAtPath("/C/B/A", "/C/B/B_Sub1_Prim", 
            "An object already exists at the new path")

        _VerifyCannotApplyRenamePrim(primC, "C", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePrimAtPath("/C", "/C", 
            "An object already exists at the new path")

        # Test invalid reparent destinations.

        # New parent prim does not exist
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentPrim(primA, stage.GetPrimAtPath("/D"))
        _VerifyCannotApplyMovePrimAtPath("/C/B/A", "/D/A", 
            "The new parent prim is not a valid prim")

        # New parent prim is the prim itself
        _VerifyCannotApplyReparentPrim(primA, primA, 
            "The new parent prim is the same as the prim to move")
        _VerifyCannotApplyMovePrimAtPath("/C/B/A", "/C/B/A/A", 
            "The new parent prim is the same as the prim to move")

        # New parent prim is a descendant of the prim itself
        _VerifyCannotApplyReparentPrim(primB, primA, 
            "The new parent prim is a descendant of the prim to move")
        _VerifyCannotApplyMovePrimAtPath("/C/B", "/C/B/A/B", 
            "The new parent prim is a descendant of the prim to move")

        # Reparents fail when an object already exists at the reparented path.
        # These reparent cases rename the prim to cause the path collision.
        _VerifyCannotApplyReparentAndRenamePrim(primA, primB, "B_Root_Prim", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePrimAtPath("/C/B/A", "/C/B/B_Root_Prim", 
            "An object already exists at the new path")
        _VerifyCannotApplyReparentAndRenamePrim(primA, primC, "C_Sub1_Prim", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePrimAtPath("/C/B/A", "/C/C_Sub1_Prim", 
            "An object already exists at the new path")

        # Open one of the sublayers as a stage to test that the stage of the
        # prim doesn't matter when adding edits.
        subLayerStage = Usd.Stage.Open(subLayer1)
        # Create another namespace editor for the sublayer stage.
        subLayerEditor = Usd.NamespaceEditor(subLayerStage)
        # The sublayer stage has a prim at /C but does not have a prim at /C/B/A
        # unlike the root stage.
        self.assertTrue(subLayerStage.GetPrimAtPath("/C"))
        self.assertFalse(subLayerStage.GetPrimAtPath("/C/B/A"))
        
        # It's valid to add reparent operations to both editors using UsdPrims
        # from either stage as we only use the prims to determine the paths to objects
        # to edit, not to determine the objects themselves.
        self.assertTrue(editor.ReparentPrim(
            stage.GetPrimAtPath("/C/B/A"), 
            subLayerStage.GetPrimAtPath("/C")))
        self.assertTrue(subLayerEditor.ReparentPrim(
            stage.GetPrimAtPath("/C/B/A"), 
            subLayerStage.GetPrimAtPath("/C")))
        # But only the root stage can apply this reparent edit as /C/B/A is only a valid
        # prim on the root stage and not on the sublayer stage.
        self.assertTrue(editor.CanApplyEdits())     
        self._VerifyFalseResult(subLayerEditor.CanApplyEdits(),
            "The prim to edit is not a valid prim")     

        # Make sublayer1 uneditable. Prims /C and /C/B cannot be edited because
        # they have specs on sublayer1. Prim /C/B/A can still be edited since it 
        # has no specs on sublayer1.
        subLayer1.SetPermissionToEdit(False)

        # Get the current working directory as it would be in the layer 
        # identifier regardless of platform
        def getFormattedCwd():
            drive, tail = os.path.splitdrive(os.getcwd())
            return drive.lower() + tail.replace('\\', '/')

        # Cannot delete or rename /C (there are no valid reparent targets for 
        # /C on this stage currently regardless of layer permission)
        expectedMsg = ("The spec @{}/basic/sub_1.usda@</C> cannot be edited "
            "because the layer is not editable").format(getFormattedCwd())
        _VerifyCannotApplyDeletePrim(primC, expectedMsg)
        _VerifyCannotApplyDeletePrimAtPath("/C", expectedMsg)
        _VerifyCannotApplyRenamePrim(primC, "NewC", expectedMsg)
        _VerifyCannotApplyMovePrimAtPath("/C", "/NewC", expectedMsg)

        # Cannot delete, rename, or reparent /C/B
        expectedMsg = ("The spec @{}/basic/sub_1.usda@</C/B> cannot be edited "
            "because the layer is not editable").format(getFormattedCwd())
        _VerifyCannotApplyDeletePrim(primB, expectedMsg)
        _VerifyCannotApplyDeletePrimAtPath("/C/B", expectedMsg)
        _VerifyCannotApplyRenamePrim(primB, "NewB", expectedMsg)
        _VerifyCannotApplyMovePrimAtPath("/C/B", "/C/NewB", expectedMsg)
        _VerifyCannotApplyReparentPrim(primB, stage.GetPseudoRoot(), expectedMsg)
        _VerifyCannotApplyMovePrimAtPath("/C/B", "/B", expectedMsg)
        _VerifyCannotApplyReparentAndRenamePrim(primB, stage.GetPseudoRoot(), "NewB", 
            expectedMsg)
        _VerifyCannotApplyMovePrimAtPath("/C/B", "/NewB", expectedMsg)

        # Simple helper for verifying that the edits to primA were performed
        # followed by resetting the stage for the next edit.
        def _VerifyPrimAWasEditedAndReset(newPathAfterMove = None):
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())

            # The edit will have changed the root layer (where /C/B/A's spec) is
            # defined but will not have touched the sublayers.
            self.assertTrue(rootLayer.dirty)
            self.assertFalse(subLayer1.dirty)
            self.assertFalse(subLayer2.dirty)

            # Prim /C/B/A will no longer exist on the stage, but if it was 
            # moved or renamed, the prim at this path will exist.
            self.assertFalse(stage.GetPrimAtPath("/C/B/A"))
            if newPathAfterMove:
                self.assertTrue(stage.GetPrimAtPath(newPathAfterMove))

            # Reload the root layer to reset and get primA again.
            rootLayer.Reload()
            nonlocal primA
            primA = stage.GetPrimAtPath("/C/B/A")
            self.assertTrue(primA)
            
        # All valid edits for /C/B/A are still allowed since there are no 
        # specs on sublayer1 for prim /C/B/A. We test them all here.

        # Can delete primA
        self.assertTrue(editor.DeletePrim(primA))
        _VerifyPrimAWasEditedAndReset()

        # Can rename primA
        self.assertTrue(editor.RenamePrim(primA, "NewA"))
        _VerifyPrimAWasEditedAndReset(newPathAfterMove = "/C/B/NewA")

        # Can reparent primA to root.
        self.assertTrue(editor.ReparentPrim(primA, stage.GetPseudoRoot()))
        _VerifyPrimAWasEditedAndReset(newPathAfterMove = "/A")

        # Can reparent primA to root and rename it..
        self.assertTrue(
            editor.ReparentPrim(primA, stage.GetPseudoRoot(), "NewA"))
        _VerifyPrimAWasEditedAndReset(newPathAfterMove = "/NewA")

        # Can reparent primA under primC even though primC cannnot be namespace
        # edited.
        self.assertTrue(editor.ReparentPrim(primA, primC))
        _VerifyPrimAWasEditedAndReset(newPathAfterMove = "/C/A")

        # Can reparent and rename primA under primC even though primC cannnot be
        # namespace edited.
        self.assertTrue(editor.ReparentPrim(primA, primC, "NewA"))
        _VerifyPrimAWasEditedAndReset(newPathAfterMove = "/C/NewA")

        subLayer1.SetPermissionToEdit(True)

    def test_EditPrimsWithInstancing(self):
        """Tests namespace edit operations on prims with native instancing.
        """

        # Helper for verifying the contents of the prims expected in this test
        # which should be the same for all intanced and non-instanced prims in
        # this particular test case.
        def _VerifyInstancingStagePrimValues(prim):
            # Verify the prim's type and property names
            self.assertTrue(prim)
            self.assertEqual(prim.GetTypeName(), 'PrimTypeC')
            self.assertEqual(
                prim.GetPrimTypeInfo().GetAppliedAPISchemas(), ["C_Ref_API"])
            self.assertEqual(prim.GetPropertyNames(), ['C_Ref_Attr'])

            # Verify the prim has a child named "B" with expected type and
            # property names.
            childB = prim.GetChild("B")
            self.assertTrue(childB)
            self.assertEqual(childB.GetTypeName(), 'PrimTypeB')
            self.assertEqual(
                childB.GetPrimTypeInfo().GetAppliedAPISchemas(), ["B_Ref_API"])
            self.assertEqual(childB.GetPropertyNames(), ['B_Ref_Attr'])

            # Verify the prim's child "B" has a child named "A" with expected 
            # type and property names.
            childA = childB.GetChild("A")
            self.assertTrue(childA)
            self.assertEqual(childA.GetTypeName(), 'PrimTypeA')
            self.assertEqual(
                childA.GetPrimTypeInfo().GetAppliedAPISchemas(), ["A_Ref_API"])
            self.assertEqual(childA.GetPropertyNames(), ['A_Ref_Attr'])

        # Opens the stage for this test case, verifies the expected prims and
        # returns the stage and these prims to test.
        def _SetupInstancingStage():

            stage = Usd.Stage.Open("instancing/root.usda")
            self.assertTrue(stage)
            
            # The stage two instanced prims that have the same prototype.
            instance1 = stage.GetPrimAtPath("/Instance1")
            self.assertTrue(instance1)
            self.assertTrue(instance1.IsInstance())

            instance2 = stage.GetPrimAtPath("/Instance2")
            self.assertTrue(instance2)
            self.assertTrue(instance2.IsInstance())

            prototypePrim = instance1.GetPrototype()
            self.assertTrue(prototypePrim)
            self.assertEqual(instance1.GetPrototype(), prototypePrim)
            self.assertEqual(instance2.GetPrototype(), prototypePrim)

            # The stage also defines a single prim that is not instanceable but
            # uses the same reference as the instanceable prims.
            nonInstancePrim = stage.GetPrimAtPath("/NonInstancePrim")
            self.assertTrue(nonInstancePrim)
            self.assertFalse(nonInstancePrim.IsInstance())
            self.assertFalse(nonInstancePrim.GetPrototype())

            # Verify all the three of these prims have the same content.
            _VerifyInstancingStagePrimValues(instance1)
            _VerifyInstancingStagePrimValues(instance2)
            _VerifyInstancingStagePrimValues(nonInstancePrim)

            return (stage, instance1, instance2, nonInstancePrim, prototypePrim)

        # Helper to verify that CanApplyEdits returns false with the expected
        # message and that ApplyEdits fails and doesn't edit the stage.
        def _VerifyCannotApplyEdits(expectedMessage):
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()
            self.assertFalse(stage.GetRootLayer().dirty)

        # Helpers to verify that a valid [Delete|Rename|Reparent]Prim 
        # operation can be added but cannot be applied to the editor's stage for
        # the expected reason.
        def _VerifyCannotApplyDeletePrim(prim, expectedMessage):
            self.assertTrue(editor.DeletePrim(prim))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyRenamePrim(prim, newName, expectedMessage):
            self.assertTrue(editor.RenamePrim(prim, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyReparentPrim(prim, newParent, expectedMessage):
            self.assertTrue(editor.ReparentPrim(prim, newParent))
            _VerifyCannotApplyEdits(expectedMessage)

        # Helper to verify that the prim at that path can be successfully
        # deleted. Resets the stage to be unedited afterward and returns
        # the original prim.
        def _VerifyCanDeletePrimAtPath(primPath):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we can delete the prim, delete it, and make sure it
            # no longer exists on the stage.
            self.assertTrue(editor.DeletePrim(prim))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPrimAtPath(primPath))

            # Reset the stage for the next case.
            stage.Reload()
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)
            return prim

        # Helper to verify that the prim at that path can be successfully moved
        # to a new path. Resets the stage to be unedited afterward and returns
        # the original prim.
        def _VerifyCanMovePrimAtPath(primPath, newPrimPath):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we can move the prim, move it, and make sure it
            # no longer exists at the old path but does exist at the new path.
            self.assertTrue(editor.MovePrimAtPath(primPath, newPrimPath))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPrimAtPath(primPath))
            self.assertTrue(stage.GetPrimAtPath(newPrimPath))

            # Reset the stage for the next case.
            stage.Reload()
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)
            return prim

        # Open the stage and get the prims to test.
        stage, instance1, instance2, nonInstancePrim, prototypePrim = \
            _SetupInstancingStage()

        editor = Usd.NamespaceEditor(stage)
        
        # We can delete any of the instance and non-instance prims.
        instance1 = _VerifyCanDeletePrimAtPath("/Instance1")
        instance2 = _VerifyCanDeletePrimAtPath("/Instance2")
        nonInstancePrim = _VerifyCanDeletePrimAtPath("/NonInstancePrim")

        # But we can't delete any of the children of the instance prims as they
        # are proxies from the prototype. We also can't delete a child of the 
        # non-instance prim but for a different reason as it is defined across a
        # reference and we don't yet support deactivation as delete.
        _VerifyCannotApplyDeletePrim(instance1.GetChild("B"), 
            "The prim to edit is a prototype proxy descendant of an instance "
            "prim")
        _VerifyCannotApplyDeletePrim(instance2.GetChild("B"), 
            "The prim to edit is a prototype proxy descendant of an instance "
            "prim")
        _VerifyCannotApplyDeletePrim(nonInstancePrim.GetChild("B"), 
            "The prim to delete must be deactivated rather than deleted since "
            "it composes opinions introduced by ancestral composition arcs; "
            "deletion via deactivation is not yet supported")

        # Like with delete, we can rename any of the instance and non-instance 
        # prims (as long as the new name is valid).
        instance1 = _VerifyCanMovePrimAtPath("/Instance1", "/NewInstance1")
        instance2 = _VerifyCanMovePrimAtPath("/Instance2", "/NewInstance2")
        nonInstancePrim = _VerifyCanMovePrimAtPath(
            "/NonInstancePrim", "/NewNonInstancePrim")

        # And just like with delete, we can't rename any of the children of the 
        # instance prims because they are proxies into the prototype. The 
        # non-instanced prim can't be renamed because relocates aren't supported
        # yet.
        _VerifyCannotApplyRenamePrim(instance1.GetChild("B"), "NewB", 
            "The prim to edit is a prototype proxy descendant of an instance "
            "prim")
        _VerifyCannotApplyRenamePrim(instance2.GetChild("B"), "NewB", 
            "The prim to edit is a prototype proxy descendant of an instance "
            "prim")
        _VerifyCannotApplyRenamePrim(nonInstancePrim.GetChild("B"), "NewB", 
            "The prim to move requires authoring relocates since it composes "
            "opinions introduced by ancestral composition arcs; authoring "
            "relocates is not yet supported")

        # We can reparent an instance prim under a non-instance prim
        instance1 = _VerifyCanMovePrimAtPath(
            "/Instance1", "/NonInstancePrim/Instance1")

        # We can reparent and rename an instance prim under a non-instance prim
        instance2 = _VerifyCanMovePrimAtPath(
            "/Instance2", "/NonInstancePrim/NewInstance2")

        # We can also reparent an instance prim under a child of a non-instance
        # prim
        instance1 = _VerifyCanMovePrimAtPath(
            "/Instance1", "/NonInstancePrim/B/Instance1")

        # As well as reparent and rename an instance prim under a child of a
        # non-instance prim
        instance2 = _VerifyCanMovePrimAtPath(
            "/Instance2", "/NonInstancePrim/B/NewInstance2")

        # But we cannot reparent any prims under an another instance prim 
        # regardless of whether they're instanced on non-instanced
        _VerifyCannotApplyReparentPrim(instance1, instance2, 
            "The new parent prim is an instance prim whose children are "
            "provided exclusively by its prototype")
        _VerifyCannotApplyReparentPrim(nonInstancePrim, instance2, 
            "The new parent prim is an instance prim whose children are "
            "provided exclusively by its prototype")

        # No editing operations can be performed with a prototype prim or its 
        # children ever
        _VerifyCannotApplyDeletePrim(prototypePrim, 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyDeletePrim(prototypePrim.GetChild("B"), 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyRenamePrim(prototypePrim, "NewPrototype", 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyRenamePrim(prototypePrim.GetChild("B"), "NewB", 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyReparentPrim(prototypePrim, nonInstancePrim, 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyReparentPrim(prototypePrim.GetChild("B"), nonInstancePrim, 
            "The prim to edit belongs to a prototype prim")
        _VerifyCannotApplyReparentPrim(nonInstancePrim, prototypePrim, 
            "The new parent prim belongs to a prototype prim")
        _VerifyCannotApplyReparentPrim(nonInstancePrim, prototypePrim.GetChild("B"), 
            "The new parent prim belongs to a prototype prim")

    def test_EditPrimsWithCompositionArcs(self):
        """Tests namespace edit operations on prims with specs that contribute
        opinions across composition arcs.
        """

        # This stage has few variety of composition arcs on the root prims.
        stage = Usd.Stage.Open("composition_arcs/root.usda")
        self.assertTrue(stage)

        editor = Usd.NamespaceEditor(stage)

        # Helper functions for testing prims that we expect to be able to 
        # successfully edit.

        # Helper to verify that the prim at that path can be successfully
        # deleted.
        def _VerifyCanDeletePrimAtPath(primPath):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we can delete the prim, delete it, and make sure it
            # no longer exists on the stage.
            self.assertTrue(editor.DeletePrim(prim))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPrimAtPath(primPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the prim at that path can be successfully moved
        # to a new path.
        def _VerifyCanMovePrimAtPath(primPath, newPrimPath):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we can move the prim, move it, and make sure it
            # no longer exists at the old path but does exist at the new path.
            self.assertTrue(editor.MovePrimAtPath(primPath, newPrimPath))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPrimAtPath(primPath))
            self.assertTrue(stage.GetPrimAtPath(newPrimPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the prim can be both deleted and moved.
        def _VerifyCanEditPrimAtPath(primPath):
            _VerifyCanDeletePrimAtPath(primPath)
            _VerifyCanMovePrimAtPath(primPath, "/Foo")

        # Helper functions for testing prims that we expect to NOT be able to 
        # successfully edit.

        # Helper for extra verification that delete shouldn't work. This 
        # function finds all the specs for the prim in the stage's root layer
        # stack and deletes these specs from the layer. This is what delete 
        # operation would do if it were performed without first checking that 
        # it would succeed. We can call this to prove that deleting the root 
        # layer stack specs is not sufficient for deleting the prim from the 
        # stage.
        def _VerifyDeletingRootLayerStackSpecsDoesNotDeletePrim(primPath):
            for layer in stage.GetLayerStack():
                spec = layer.GetPrimAtPath(primPath)
                if spec:
                    del spec.realNameParent.nameChildren[spec.path.name]
                # No spec should exist for the prim in this layer because it
                # either didn't exist to begin with or we just deleted it.
                self.assertFalse(layer.GetPrimAtPath(primPath))

            # Verify the stage still has a prim at the path after deleting all
            # root layer stack specs
            self.assertTrue(stage.GetPrimAtPath(primPath))

            # Reset the stage for the next case.
            stage.Reload()

        def _VerifyCannotApplyEdits(expectedMessage):
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()
            self.assertFalse(stage.GetRootLayer().dirty)

        # Helper to verify that the prim at that path cannot be deleted and 
        # fails with the expected error message.
        def _VerifyCannotApplyDeletePrimAtPath(primPath, expectedMessage):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we cannot delete the prim.
            self.assertTrue(editor.DeletePrim(prim))
            _VerifyCannotApplyEdits(expectedMessage)

            # Verify that the deleting root layer stack specs would indeed not
            # delete the prim.
            _VerifyDeletingRootLayerStackSpecsDoesNotDeletePrim(primPath)

        # Helper to verify that the prim at that path cannot be moved and fails
        # with the expected error message.
        def _VerifyCannotApplyMovePrimAtPath(primPath, newPrimPath, expectedMessage):
            # Verify the prim actually exists first
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)

            # Verify that we cannot move the prim.
            self.assertTrue(editor.MovePrimAtPath(primPath, newPrimPath))
            _VerifyCannotApplyEdits(expectedMessage)

            # No need to verify that moving specs in the root layer stack would
            # not move the prim; the delete case is sufficient for proving this.

        # Helper to verify that the prim cannot be deleted nor moved.
        def _VerifyCannotEditPrimAtPath(primPath):
            _VerifyCannotApplyDeletePrimAtPath(primPath,
                "The prim to delete must be deactivated rather than deleted "
                "since it composes opinions introduced by ancestral "
                "composition arcs; deletion via deactivation is not yet "
                "supported")
            _VerifyCannotApplyMovePrimAtPath(primPath, "/Foo",
                "The prim to move requires authoring relocates since it "
                "composes opinions introduced by ancestral composition arcs; "
                "authoring relocates is not yet supported")

        # A prim with a direct reference to another prim can be edited.
        _VerifyCanEditPrimAtPath("/PrimWithReference")

        # But children of the prim which were defined across the reference 
        # (i.e. brought in by an ancestral reference) cannot be edited without
        # relocates.
        _VerifyCannotEditPrimAtPath("/PrimWithReference/ClassChild")
        _VerifyCannotEditPrimAtPath("/PrimWithReference/B")
        _VerifyCannotEditPrimAtPath("/PrimWithReference/B/A")

        # But a child prim that was added entirely in the root layer stack, even
        # though its parent is defined across the reference, can still be edited
        # since there are no specs across the ancestral reference.
        _VerifyCanEditPrimAtPath("/PrimWithReference/B/B_Root_Child")

        # A prim with a subroot prim reference behaves the same way as a prim
        # with a root prim reference.
        _VerifyCanEditPrimAtPath("/PrimWithSubrootReference")
        _VerifyCannotEditPrimAtPath("/PrimWithSubrootReference/A")
        _VerifyCanEditPrimAtPath("/PrimWithSubrootReference/A/A_Root_Child")

        # A prim with a variant selection can be edited given the variant is 
        # still a direct arc.
        _VerifyCanEditPrimAtPath("/PrimWithVariant")

        # But children of the prim which were defined across the reference 
        # within the selected variant cannot be edited without relocates.
        _VerifyCannotEditPrimAtPath("/PrimWithVariant/ClassChild")
        _VerifyCannotEditPrimAtPath("/PrimWithVariant/B")
        _VerifyCannotEditPrimAtPath("/PrimWithVariant/B/A")

        # But also a child defined fully in the root layer stack but inside
        # the variant self still cannot be edited without relocates or edit 
        # target support as it has specs across an ancestral variant arc.
        _VerifyCannotEditPrimAtPath("/PrimWithVariant/V1_Root_Child")

if __name__ == '__main__':
    unittest.main()
