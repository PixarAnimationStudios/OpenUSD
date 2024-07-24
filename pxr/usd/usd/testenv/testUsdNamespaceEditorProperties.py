#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Sdf, Usd, Tf, Plug

class TestUsdNamespaceEditorProperties(unittest.TestCase):

    # Verifies the fields and property stack of the property at path 
    # /C.C_Root_Attr when the basic stage is loaded. The fields we verify here
    # should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageCRootAttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.String)
        self.assertIsNone(prop.GetMetadata("displayGroup"))
        self.assertEqual(prop.Get(), "foo")
        self.assertEqual(prop.GetPropertyStack(), [
            Sdf.Find('basic/root.usda', prop.GetPath())
            ])
            
    # Verifies the fields and property stack of the property at path 
    # /C.C_Sub1_Attr when the basic stage is loaded. The fields we verify here
    # should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageCSub1AttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.Int)
        self.assertEqual(prop.GetMetadata("displayGroup"), "Sub1")
        self.assertEqual(prop.Get(), 3)
        self.assertEqual(prop.GetPropertyStack(), [
            Sdf.Find('basic/root.usda', prop.GetPath()), 
            Sdf.Find('basic/sub_1.usda', prop.GetPath())
            ])

    # Verifies the fields and property stack of the property at path 
    # /C.C_Sub2_Attr when the basic stage is loaded. The fields we verify here
    # should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageCSub2AttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.Double)
        self.assertEqual(prop.GetMetadata("displayGroup"), "Sub2")
        self.assertEqual(prop.Get(), 3.0)
        self.assertEqual(prop.GetPropertyStack(), [
            Sdf.Find('basic/root.usda', prop.GetPath()), 
            Sdf.Find('basic/sub_1.usda', prop.GetPath()),
            Sdf.Find('basic/sub_2.usda', prop.GetPath())
            ])

    # Verifies the fields and property stack of the property at path 
    # /C/B.B_Root_Attr when the basic stage is loaded. The fields we verify here
    # should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageBRootAttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.String)
        self.assertIsNone(prop.GetMetadata("displayGroup"))
        self.assertEqual(prop.Get(), "bar")
        self.assertEqual(prop.GetPropertyStack(),
            [Sdf.Find('basic/root.usda', prop.GetPath())]
        )
            
    # Verifies the fields and property stack of the property at path 
    # /C/B.B_Sub1_Attr when the basic stage is loaded. The fields we verify here
    # should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageBSub1AttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.Int)
        self.assertEqual(prop.GetMetadata("displayGroup"), "Sub1")
        self.assertEqual(prop.Get(), 2)
        self.assertEqual(prop.GetPropertyStack(), [
            Sdf.Find('basic/root.usda', prop.GetPath()), 
            Sdf.Find('basic/sub_1.usda', prop.GetPath())
            ])

    # Verifies the fields and property stack of the property at path 
    # /C/B/A.A_Root_Attr when the basic stage is loaded. The fields we verify 
    # here should not change when the property is reparented and/or renamed. 
    def _VerifyBasicStageARootAttrValues(self, prop):
        self.assertTrue(prop)
        self.assertEqual(prop.GetTypeName(), Sdf.ValueTypeNames.String)
        self.assertIsNone(prop.GetMetadata("displayGroup"))
        self.assertEqual(prop.Get(), "baz")
        self.assertEqual(prop.GetPropertyStack(),
            [Sdf.Find('basic/root.usda', prop.GetPath())]
        )
        
    # Verifies the original prim stacks (i.e. at stage load time) of the three 
    # prims on the basic stage that hold the properties used in the basic 
    # property namespace editing test cases. Most property operations will not
    # change these prim stacks with the exception of some reparent operations.
    def _VerifyOriginalBasicStagePrimStacks(self, stage):
        self.assertEqual(stage.GetPrimAtPath('/C/B/A').GetPrimStack(), [
            Sdf.Find('basic/root.usda', '/C/B/A')
            ])
        self.assertEqual(stage.GetPrimAtPath('/C/B').GetPrimStack(), [
            Sdf.Find('basic/root.usda', '/C/B'),
            Sdf.Find('basic/sub_1.usda', '/C/B')
            ])
        self.assertEqual(stage.GetPrimAtPath('/C').GetPrimStack(), [
            Sdf.Find('basic/root.usda', '/C'),
            Sdf.Find('basic/sub_1.usda', '/C'),
            Sdf.Find('basic/sub_2.usda', '/C')
            ])

    # Verifies that the result is false and provides the expected message.
    def _VerifyFalseResult(self, result, expectedMessage):
        self.assertFalse(result)
        self.assertEqual(result.whyNot, expectedMessage)

    # Setup function for many of the basic test cases. Loads the "basic" stage
    # and gets and returns the needed layers and initial prims from this stage.
    # Also verifies the initial state of the stage and its prim and properties
    # match the expected unedited state of the stage.
    def _SetupBasicStage(self):
        # Open the root layer
        stage = Usd.Stage.Open("basic/root.usda")
               
        # The stage defines three prims.
        primA = stage.GetPrimAtPath("/C/B/A")
        primB = stage.GetPrimAtPath("/C/B")
        primC = stage.GetPrimAtPath("/C")
        self.assertTrue(primA)
        self.assertTrue(primB)
        self.assertTrue(primC)
        
        # Verify prim /C's three properties
        self.assertEqual(primC.GetPropertyNames(), 
            ['C_Root_Attr', 'C_Sub1_Attr', 'C_Sub2_Attr'])
        self._VerifyBasicStageCRootAttrValues(primC.GetProperties()[0])
        self._VerifyBasicStageCSub1AttrValues(primC.GetProperties()[1])
        self._VerifyBasicStageCSub2AttrValues(primC.GetProperties()[2])

        # Verify prim /C/B's two properties
        self.assertEqual(primB.GetPropertyNames(), ['B_Root_Attr', 'B_Sub1_Attr'])       
        self._VerifyBasicStageBRootAttrValues(primB.GetProperties()[0])
        self._VerifyBasicStageBSub1AttrValues(primB.GetProperties()[1])
        
        # Verify prim /C/B/A's single property
        self.assertEqual(primA.GetPropertyNames(), ['A_Root_Attr'])       
        self._VerifyBasicStageARootAttrValues(primA.GetProperties()[0])
        
        return (stage, primA, primB, primC)

    def test_BasicDeleteProperty(self):
        """Test basic USD property deletion through the UsdNamespaceEditor API.
        """

        # This function allows the same tests to be performed using 
        # DeleteProperty and DeletePropertyAtPath.
        def _TestDeleteProperty(self, useDeletePropertyAtPath):
            
            # Open the basic stage and verify it's starting state.
            stage, primA, primB, primC = self._SetupBasicStage()
        
            editor = Usd.NamespaceEditor(stage)
            
            # Verify the prim stacks for the stage's prims in their pre-edited 
            # state
            self._VerifyOriginalBasicStagePrimStacks(stage)

            # Paths to each property on the stage. We will perform delete on 
            # each of them.
            propPaths = [
                Sdf.Path('/C/B/A.A_Root_Attr'),
                Sdf.Path('/C/B.B_Root_Attr'), 
                Sdf.Path('/C/B.B_Sub1_Attr'),
                Sdf.Path('/C.C_Root_Attr'), 
                Sdf.Path('/C.C_Sub1_Attr'), 
                Sdf.Path('/C.C_Sub2_Attr')
                ]

            for propPath in propPaths:
                # Get the property; it must exist.
                prop = stage.GetPropertyAtPath(propPath)
                self.assertTrue(prop)

                # Verify the property can be deleted and delete it.                
                if useDeletePropertyAtPath:
                    self.assertTrue(editor.DeletePropertyAtPath(propPath))
                else:
                    self.assertTrue(editor.DeleteProperty(prop))
                self.assertTrue(editor.CanApplyEdits())
                self.assertTrue(editor.ApplyEdits())

                # Verify the property no longer exists.
                self.assertFalse(prop)
                self.assertFalse(stage.GetPropertyAtPath(propPath))

            # Verify the prim stacks for the stage's prims are still the same
            # after all the delete operations
            self._VerifyOriginalBasicStagePrimStacks(stage)

        # Run delete tests using both DeleteProperty and DeletePropertyAtPath
        _TestDeleteProperty(self, useDeletePropertyAtPath=False)
        _TestDeleteProperty(self, useDeletePropertyAtPath=True)

    def test_BasicRenameProperty(self):
        """Test basic USD property renaming through the UsdNamespaceEditor API.
        """

        # This function allows the same tests to be performed using 
        # RenameProperty and MovePropertyAtPath.
        def _TestRenameProperty(self, useMovePropertyAtPath):
            
            # Open the basic stage and verify it's starting state.
            stage, primA, primB, primC = self._SetupBasicStage()
        
            # Create a namespace editor for the stage
            editor = Usd.NamespaceEditor(stage)
            
            # Paths to each property on the stage along with the new name to 
            # rename it to and the verify function to call on the property to 
            # verify that it was fully moved to the new name.
            propPaths = [
                (Sdf.Path('/C/B/A.A_Root_Attr'), 
                    'New_A_Root_Attr', 
                    self._VerifyBasicStageARootAttrValues),
                (Sdf.Path('/C/B.B_Root_Attr'), 
                    'New_B_Root_Attr', 
                    self._VerifyBasicStageBRootAttrValues),
                (Sdf.Path('/C/B.B_Sub1_Attr'), 
                    'New_B_Sub1_Attr', 
                    self._VerifyBasicStageBSub1AttrValues),
                (Sdf.Path('/C.C_Root_Attr'), 
                    'New_C_Root_Attr', 
                    self._VerifyBasicStageCRootAttrValues),
                (Sdf.Path('/C.C_Sub1_Attr'), 
                    'New_C_Sub1_Attr', 
                    self._VerifyBasicStageCSub1AttrValues),
                (Sdf.Path('/C.C_Sub2_Attr'), 
                    'New_C_Sub2_Attr', 
                    self._VerifyBasicStageCSub2AttrValues)
                ]

            # Verify the prim stacks for the stage's prims in their pre-edited 
            # state
            self._VerifyOriginalBasicStagePrimStacks(stage)
            
            for propPath, newName, verifyValuesFunc in propPaths:
                # Get the property; it must exist.
                prop = stage.GetPropertyAtPath(propPath)
                self.assertTrue(prop)

                # Get the expected path of the property after the rename.                
                newPropPath = prop.GetPrimPath().AppendProperty(newName)

                # Verify the property can be renamed and rename it.                
                if useMovePropertyAtPath:
                    self.assertTrue(editor.MovePropertyAtPath(
                        propPath, newPropPath))
                else:
                    self.assertTrue(editor.RenameProperty(prop, newName))
                self.assertTrue(editor.CanApplyEdits())
                self.assertTrue(editor.ApplyEdits())

                # Verify the original property no longer exists.
                self.assertFalse(prop)
                self.assertFalse(stage.GetPropertyAtPath(propPath))

                # Verify the property exists at the renamed path and has the
                # same values as the original property
                self.assertTrue(stage.GetPropertyAtPath(newPropPath))
                verifyValuesFunc(stage.GetPropertyAtPath(newPropPath))

            # Verify the prim stacks for the stage's prims are still the same
            # after all the rename operations
            self._VerifyOriginalBasicStagePrimStacks(stage)

        # Run rename tests using both RenameProperty and MovePropertyAtPath
        _TestRenameProperty(self, useMovePropertyAtPath=False)
        _TestRenameProperty(self, useMovePropertyAtPath=True)

    def test_BasicReparentProperty(self):
        """Test basic USD property reparenting through the UsdNamespaceEditor 
        API without renaming the properties.
        """

        # This function allows the same tests to be performed using 
        # ReparentProperty and MovePropertyAtPath.
        def _TestReparentProperty(self, useMovePropertyAtPath):
            
            # Open the basic stage and verify it's starting state.
            stage, primA, primB, primC = self._SetupBasicStage()
        
            editor = Usd.NamespaceEditor(stage)
            
            # Paths to each property on the stage along with the new parent prim
            # to reparent it to and the verify function to call on the property
            # to verify that it was fully moved to the new name.
            propPaths = [
                (Sdf.Path('/C/B/A.A_Root_Attr'), 
                    primC, self._VerifyBasicStageARootAttrValues),
                (Sdf.Path('/C/B.B_Root_Attr'), 
                    primA, self._VerifyBasicStageBRootAttrValues),
                (Sdf.Path('/C/B.B_Sub1_Attr'), 
                    primA, self._VerifyBasicStageBSub1AttrValues),
                (Sdf.Path('/C.C_Root_Attr'), 
                    primA, self._VerifyBasicStageCRootAttrValues),
                (Sdf.Path('/C.C_Sub1_Attr'), 
                    primA, self._VerifyBasicStageCSub1AttrValues),
                (Sdf.Path('/C.C_Sub2_Attr'), 
                    primA, self._VerifyBasicStageCSub2AttrValues)
                ]

            # Verify the prim stacks for the stage's prims in their pre-edited 
            # state
            self._VerifyOriginalBasicStagePrimStacks(stage)
            
            def _PerformReparent(propPaths):
                for propPath, newParent, verifyValuesFunc in propPaths:
                    # Get the property; it must exist.
                    prop = stage.GetPropertyAtPath(propPath)
                    self.assertTrue(prop)
                    
                    # Get the expected path of the property after the reparent.                
                    newPropPath = newParent.GetPath().AppendProperty(prop.GetName())

                    # Verify the property can be reparented and reparent it.                
                    if useMovePropertyAtPath:
                        self.assertTrue(editor.MovePropertyAtPath(
                            propPath, newPropPath))
                    else:
                        self.assertTrue(
                            editor.ReparentProperty(prop, newParent))
                    self.assertTrue(editor.CanApplyEdits())
                    self.assertTrue(editor.ApplyEdits())

                    # Verify the original property no longer exists.
                    self.assertFalse(prop)
                    self.assertFalse(stage.GetPropertyAtPath(propPath))

                    # Verify the property exists at the reparented path and has
                    # the same values as the original property
                    self.assertTrue(stage.GetPropertyAtPath(newPropPath))
                    verifyValuesFunc(stage.GetPropertyAtPath(newPropPath))

            _PerformReparent(propPaths)

            # Verify the prim stacks for the stage's prims have change after the
            # reparent, in particular new overs have been added for /C/B and
            # /C/B/A on the sublayers in order to hold some of the moved specs
            # for the reparented properties.
            self.assertEqual(primA.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C/B/A'),
                Sdf.Find('basic/sub_1.usda', '/C/B/A'),
                Sdf.Find('basic/sub_2.usda', '/C/B/A')
                ])
            self.assertEqual(primB.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C/B'),
                Sdf.Find('basic/sub_1.usda', '/C/B'),
                Sdf.Find('basic/sub_2.usda', '/C/B')
                ])
            self.assertEqual(primC.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C'),
                Sdf.Find('basic/sub_1.usda', '/C'),
                Sdf.Find('basic/sub_2.usda', '/C')
                ])

            # Paths to each of the moved properties at their moved paths along 
            # with the original prim they were reparented from. Performing the 
            # reparent with this set will return the stage back to its state
            # before the edits.
            propPaths = [
                (Sdf.Path('/C.A_Root_Attr'), 
                    primA, self._VerifyBasicStageARootAttrValues),
                (Sdf.Path('/C/B/A.B_Root_Attr'), 
                    primB, self._VerifyBasicStageBRootAttrValues),
                (Sdf.Path('/C/B/A.B_Sub1_Attr'), 
                    primB, self._VerifyBasicStageBSub1AttrValues),
                (Sdf.Path('/C/B/A.C_Root_Attr'), 
                    primC, self._VerifyBasicStageCRootAttrValues),
                (Sdf.Path('/C/B/A.C_Sub1_Attr'), 
                    primC, self._VerifyBasicStageCSub1AttrValues),
                (Sdf.Path('/C/B/A.C_Sub2_Attr'), 
                    primC, self._VerifyBasicStageCSub2AttrValues)
                ]

            # Perform and verify the "undo" reparent operations
            _PerformReparent(propPaths)

            # Verify the prim stacks for the stage's prims are back in their 
            # pre-edited state as the reparent will removed unneeded overs that
            # left behind after moving a property.
            self._VerifyOriginalBasicStagePrimStacks(stage)

        # Run reparent tests using both ReparentProperty and MovePropertyAtPath
        _TestReparentProperty(self, useMovePropertyAtPath=True)
        _TestReparentProperty(self, useMovePropertyAtPath=False)

    def test_BasicReparentAndRenameProperty(self):
        """Test basic USD property reparenting through the UsdNamespaceEditor 
        API using the overload that renames the property as well.
        """

        # This function allows the same tests to be performed using 
        # ReparentProperty and MovePropertyAtPath.
        def _TestReparentAndRenameProperty(self, useMovePropertyAtPath):
            
            # Open the basic stage and verify it's starting state.
            stage, primA, primB, primC = self._SetupBasicStage()
        
            editor = Usd.NamespaceEditor(stage)
            
            # Paths to each property on the stage along with the new parent prim
            # and new name to reparent and rename it to and the verify function 
            # to call on the property to verify that it was fully moved its new
            # path
            propPaths = [
                (Sdf.Path('/C/B/A.A_Root_Attr'), 
                    primC, 'New_A_Root_Attr', 
                    self._VerifyBasicStageARootAttrValues),
                (Sdf.Path('/C/B.B_Root_Attr'), 
                    primA, 'New_B_Root_Attr', 
                    self._VerifyBasicStageBRootAttrValues),
                (Sdf.Path('/C/B.B_Sub1_Attr'), 
                    primA, 'New_B_Sub1_Attr', 
                    self._VerifyBasicStageBSub1AttrValues),
                (Sdf.Path('/C.C_Root_Attr'), 
                    primA, 'New_C_Root_Attr', 
                    self._VerifyBasicStageCRootAttrValues),
                (Sdf.Path('/C.C_Sub1_Attr'), 
                    primA, 'New_C_Sub1_Attr', 
                    self._VerifyBasicStageCSub1AttrValues),
                (Sdf.Path('/C.C_Sub2_Attr'), 
                    primA, 'New_C_Sub2_Attr', 
                    self._VerifyBasicStageCSub2AttrValues)
                ]

            # Verify the prim stacks for the stage's prims in their pre-edited 
            # state
            self._VerifyOriginalBasicStagePrimStacks(stage)
            
            def _PerformReparentAndRename(propPaths):
                for propPath, newParent, newName, verifyValuesFunc in propPaths:
                    # Get the property; it must exist.
                    prop = stage.GetPropertyAtPath(propPath)
                    self.assertTrue(prop)
                    
                    # Get the expected path of the property after the reparent
                    # and rename.                
                    newPropPath = newParent.GetPath().AppendProperty(newName)

                    # Verify the property can be reparented and reparent it.                
                    if useMovePropertyAtPath:
                        self.assertTrue(editor.MovePropertyAtPath(
                            propPath, newPropPath))
                    else:
                        self.assertTrue(editor.ReparentProperty(
                            prop, newParent, newName))
                    self.assertTrue(editor.CanApplyEdits())
                    self.assertTrue(editor.ApplyEdits())

                    # Verify the original property no longer exists.
                    self.assertFalse(prop)
                    self.assertFalse(stage.GetPropertyAtPath(propPath))

                    # Verify the property exists at the new path and has
                    # the same values as the original property
                    self.assertTrue(stage.GetPropertyAtPath(newPropPath))
                    verifyValuesFunc(stage.GetPropertyAtPath(newPropPath))

            _PerformReparentAndRename(propPaths)

            # Verify the prim stacks for the stage's prims have change after the
            # reparent, in particular new overs have been added for /C/B and
            # /C/B/A on the sublayers in order to hold some of the moved specs
            # for the reparented properties.
            self.assertEqual(primA.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C/B/A'),
                Sdf.Find('basic/sub_1.usda', '/C/B/A'),
                Sdf.Find('basic/sub_2.usda', '/C/B/A')
                ])
            self.assertEqual(primB.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C/B'),
                Sdf.Find('basic/sub_1.usda', '/C/B'),
                Sdf.Find('basic/sub_2.usda', '/C/B')
                ])
            self.assertEqual(primC.GetPrimStack(), [
                Sdf.Find('basic/root.usda', '/C'),
                Sdf.Find('basic/sub_1.usda', '/C'),
                Sdf.Find('basic/sub_2.usda', '/C')
                ])

            # Paths to each of the moved properties at their moved paths along 
            # with the original prim and name they were reparented and renamed
            # from. Performing the reparent with this set will return the stage
            # back to its state before the edits.
            propPaths = [
                (Sdf.Path('/C.New_A_Root_Attr'), 
                    primA, 'A_Root_Attr', 
                    self._VerifyBasicStageARootAttrValues),
                (Sdf.Path('/C/B/A.New_B_Root_Attr'), 
                    primB, 'B_Root_Attr', 
                    self._VerifyBasicStageBRootAttrValues),
                (Sdf.Path('/C/B/A.New_B_Sub1_Attr'), 
                    primB, 'B_Sub1_Attr', 
                    self._VerifyBasicStageBSub1AttrValues),
                (Sdf.Path('/C/B/A.New_C_Root_Attr'), 
                    primC, 'C_Root_Attr', 
                    self._VerifyBasicStageCRootAttrValues),
                (Sdf.Path('/C/B/A.New_C_Sub1_Attr'), 
                    primC, 'C_Sub1_Attr', 
                    self._VerifyBasicStageCSub1AttrValues),
                (Sdf.Path('/C/B/A.New_C_Sub2_Attr'), 
                    primC, 'C_Sub2_Attr', 
                    self._VerifyBasicStageCSub2AttrValues)
                ]

            # Perform and verify the "undo" reparent operations
            _PerformReparentAndRename(propPaths)

            # Verify the prim stacks for the stage's prims are back in their 
            # pre-edited state as the reparent will removed unneeded overs that
            # left behind after moving a property.
            self._VerifyOriginalBasicStagePrimStacks(stage)

        # Run reparent and rename tests using both ReparentProperty and 
        # MovePropertyAtPath
        _TestReparentAndRenameProperty(self, useMovePropertyAtPath=True)
        _TestReparentAndRenameProperty(self, useMovePropertyAtPath=False)

    def test_BasicCanEditProperty(self):
        """Tests the basic usage of the CanApplyEdits in cases where namespace
        editing should fail. Also tests that calling ApplyEdits in cases where
        CanApplyEdits returns false will not change any layer content.
        """

        # Load the basic stage and get its prims 
        stage, primA, primB, primC = self._SetupBasicStage()

        editor = Usd.NamespaceEditor(stage)

        # Helper to verify that none of the layers used by the stage have 
        # changed after an expected failed edit.
        def _VerifyNoLayersHaveChanged():
            # The basic stage should have 4 layers: the session layer, the root
            # layer, and its two sublayers.
            stageLayers = stage.GetUsedLayers()
            self.assertEqual(len(stageLayers), 4)
            for layer in stageLayers:
                self.assertFalse(layer.dirty)

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
        def _VerifyCannotApplyDeleteProperty(property, expectedMessage):
            self.assertTrue(editor.DeleteProperty(property))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyRenameProperty(property, newName, expectedMessage):
            self.assertTrue(editor.RenameProperty(property, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyReparentProperty(property, newParent, expectedMessage):
            self.assertTrue(editor.ReparentProperty(property, newParent))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotReparentAndRenameProperty(
                property, newParent, newName, expectedMessage):
            self.assertTrue(editor.ReparentProperty(property, newParent, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyDeletePropertyAtPath(path, expectedMessage):
            self.assertTrue(editor.DeletePropertyAtPath(path))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyMovePropertyAtPath(path, newPath, expectedMessage):
            self.assertTrue(editor.MovePropertyAtPath(path, newPath))
            _VerifyCannotApplyEdits(expectedMessage)

        # First verify we can't apply edits if we haven't added any yet.
        _VerifyCannotApplyEdits("There are no valid edits to perform")

        # Test invalid edit operations that can't even be added because they
        # do not produce valid absolute property paths.
            
        # Test valid SdfPaths that are not absolute property paths. These 
        # non-property paths will emit a coding error if provided to any of the
        # add property edit at path functions.
        invalidPrimPaths = [
            "/C", # prim path
            "/C{x=y}.Attr", # property path with ancestor variant selection
            "/", # pseudo-root
            ".Attr", # relative property path
            "C.Attr", # relative property path
            "../B.Attr", # relative property path
            "../.Attr", # relative property path
            "C", # relative prim path
            "/C/B.B_Rel[/Foo].attr" # target path
        ]
        for pathString in invalidPrimPaths:
            # Verify it is a valid SdfPath
            path = Sdf.Path(pathString)
            self.assertFalse(path.isEmpty)

            # Cannot use path for delete.
            with self.assertRaises(Tf.ErrorException):
                editor.DeletePropertyAtPath(path)
            # Cannot use path for source or destination of move
            with self.assertRaises(Tf.ErrorException):
                editor.MovePropertyAtPath(path, "/Foo.foo")
            with self.assertRaises(Tf.ErrorException):
                editor.MovePropertyAtPath("/C.foo", path)

        # Test fully invalid SdfPaths that will also emit a coding error if 
        #provided to any of the add prim edit at path functions.
        invalidPaths = [
            "",
            "//A",
            "/C.123",
            "/C.$",
            "/Foo:Bar.Attr",
            "/.foo"
        ]
        for pathString in invalidPaths:
            # Verify it is a valid SdfPath
            path = Sdf.Path(pathString)
            self.assertTrue(path.isEmpty)
            
            # Cannot use path for delete.
            with self.assertRaises(Tf.ErrorException):
                editor.DeletePropertyAtPath(path)
            # Cannot use path for source or destination of move
            with self.assertRaises(Tf.ErrorException):
                editor.MovePropertyAtPath(path, "/Foo.foo")
            with self.assertRaises(Tf.ErrorException):
                editor.MovePropertyAtPath("/C.foo", path)

        # GetPropertyAtPath('/Bogus.bogus') does not return a valid property 
        # because /Bogus is not a valid property. We cannot add edits for this 
        # invalid property.
        invalidProperty = stage.GetPropertyAtPath("/Bogus.foo")
        self.assertFalse(invalidProperty)
        self.assertEqual(invalidProperty.GetPath(), Sdf.Path())
        with self.assertRaises(Tf.ErrorException):
            editor.DeleteProperty(invalidProperty)
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(invalidProperty, "NewA")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(invalidProperty, primC)

        # GetPropertyAtPath('/C.bogus') also does not return a valid property.
        # However, because /C is a valid prim the UsdProperty is still able to
        # return a valid property path so we can add edits for this property.
        # The edits cannot be applied though. 
        invalidProperty = stage.GetPropertyAtPath("/C.Bogus")
        self.assertFalse(invalidProperty)
        self.assertEqual(invalidProperty.GetPath(), Sdf.Path("/C.Bogus"))
        _VerifyCannotApplyDeleteProperty(invalidProperty, 
            "The property to edit is not a valid property")
        _VerifyCannotApplyRenameProperty(invalidProperty, "New_bogus", 
            "The property to edit is not a valid property")
        _VerifyCannotApplyReparentProperty(invalidProperty, primA, 
            "The property to edit is not a valid property")

        # We cannot add edits for the empty property.
        invalidProperty = Usd.Property()
        self.assertFalse(invalidProperty)
        with self.assertRaises(Tf.ErrorException):
            editor.DeleteProperty(invalidProperty)
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(invalidProperty, "NewA")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(invalidProperty, primC)

        # We also can't add edits that try to reparent a valid property to 
        # to an invalid prim
        validProperty = stage.GetPropertyAtPath("/C.C_Root_Attr")
        self.assertTrue(validProperty)
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, Usd.Prim())

        # We cannot add rename edits with invalid property names.

        # Empty name
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(validProperty, "")
        # Invalid property name
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(validProperty, "/A")
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(validProperty, "C.foo")
        with self.assertRaises(Tf.ErrorException):
            editor.RenameProperty(validProperty, "C{x=y}")

        # We cannot add reparent edits with a new name that isn't a valid 
        # property name.

        # Empty name when reparenting with a new name
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, primA, "")
        # Invalid property name when reparenting with a new name
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, primA, "/A")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, primA, "C.foo")
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, primA, "C{x=y}")

        # At this point the last edit we attempted to add failed so there are no
        # edits to perform        
        _VerifyCannotApplyEdits("There are no valid edits to perform")

        # /C.bogus is not a prim on the stage. We cannot edit a prim at this path.
        _VerifyCannotApplyDeletePropertyAtPath("/C.bogus", 
            "The property to edit is not a valid property")
        _VerifyCannotApplyMovePropertyAtPath("/C.bogus", "/C.New_bogus", 
            "The property to edit is not a valid property")
        _VerifyCannotApplyMovePropertyAtPath("/C.bogus", "/C/B.bogus", 
            "The property to edit is not a valid property")
            
        # Test invalid rename destinations.

        self.assertTrue(validProperty)

        # Renames fail when an object already exists at the renamed path.
        _VerifyCannotApplyRenameProperty(validProperty, "C_Sub1_Attr", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePropertyAtPath("/C.C_Root_Attr", "/C.C_Sub1_Attr", 
            "An object already exists at the new path")

        _VerifyCannotApplyRenameProperty(validProperty, "C_Root_Attr", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePropertyAtPath("/C.C_Root_Attr", "/C.C_Root_Attr", 
            "An object already exists at the new path")

        # Test invalid reparent destinations.

        validProperty = primA.GetProperty("A_Root_Attr")
        self.assertTrue(validProperty)

        # Properties cannot be reparented to the pseusdo-root
        _VerifyCannotApplyReparentProperty(validProperty, stage.GetPseudoRoot(), 
            "The new parent prim for a property cannot be the pseudo-root")

        # New parent prim does not exist
        with self.assertRaises(Tf.ErrorException):
            editor.ReparentProperty(validProperty, stage.GetPrimAtPath("/D"))
        _VerifyCannotApplyMovePropertyAtPath(
            "/C/B/A.A_Root_Attr", "/D.A_Root_Attr", 
            "The new parent prim is not a valid prim")

        # New parent prim is the same parent prim.
        _VerifyCannotApplyReparentProperty(validProperty, primA, 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePropertyAtPath(
            "/C/B/A.A_Root_Attr", "/C/B/A.A_Root_Attr", 
            "An object already exists at the new path")

        # Reparents fail when an object already exists at the reparented path.
        # These reparent cases rename the prim to cause the path collision.
        _VerifyCannotReparentAndRenameProperty(validProperty, primB, "B_Root_Attr", 
            "An object already exists at the new path")
        _VerifyCannotApplyMovePropertyAtPath(
            "/C/B/A.A_Root_Attr", "/C/B.B_Root_Attr", 
            "An object already exists at the new path")

        subLayer1 = Sdf.Layer.Find("basic/sub_1.usda")
        self.assertTrue(subLayer1)

        # Open one of the sublayers as a stage to test that the stage of the
        # prim doesn't matter when adding edits.
        subLayerStage = Usd.Stage.Open(subLayer1)
        # Create another namespace editor for the sublayer stage.
        subLayerEditor = Usd.NamespaceEditor(subLayerStage)
        # The sublayer stage has a property at /C.C_Sub1_Attr but does not have
        # a prim at /C/B/A unlike the root stage.
        self.assertTrue(subLayerStage.GetPropertyAtPath("/C.C_Sub1_Attr"))
        self.assertFalse(subLayerStage.GetPrimAtPath("/C/B/A"))

        # It's valid to add reparent operations to both editors using UsdObjects
        # from either stage as we only use the prims to determine the paths to 
        # objects to edit, not to determine the objects themselves.
        self.assertTrue(editor.ReparentProperty(
            subLayerStage.GetPropertyAtPath("/C.C_Sub1_Attr"), 
            stage.GetPrimAtPath("/C/B/A")))
        self.assertTrue(subLayerEditor.ReparentProperty(
            subLayerStage.GetPropertyAtPath("/C.C_Sub1_Attr"), 
            stage.GetPrimAtPath("/C/B/A")))
        # But only the root stage can apply this reparent edit as /C/B/A is only
        # a valid parent prim on the root stage and not on the sublayer stage.
        self.assertTrue(editor.CanApplyEdits())     
        self._VerifyFalseResult(subLayerEditor.CanApplyEdits(),
            "The new parent prim is not a valid prim")     

        # Make sublayer1 uneditable to test can edit functions when not all 
        # contributing specs in a layer stack can be edited.
        subLayer1.SetPermissionToEdit(False)

        # Name, path of properties on the basic stage that have specs in 
        # sublayer1. These properties will not be editable.
        propsWithSpecsInSub1 = [
            ('C_Sub1_Attr', '/C.C_Sub1_Attr'),
            ('C_Sub2_Attr', '/C.C_Sub2_Attr'),
            ('B_Sub1_Attr', '/C/B.B_Sub1_Attr')]
            
        for propName, propPath in propsWithSpecsInSub1:
            propToEdit = stage.GetPropertyAtPath(propPath)
            self.assertTrue(propToEdit)

            # Get the current working directory as it would be in the layer 
            # identifier regardless of platform
            def getFormattedCwd():
                drive, tail = os.path.splitdrive(os.getcwd())
                return drive + tail.replace('\\', '/')

            uneditableLayerMessage = (
                "The spec @{cwd}/basic/sub_1.usda@<{propPath}> cannot be "
                "edited because the layer is not editable".format(
                    cwd=getFormattedCwd(), propPath=propPath))

            # Cannot delete
            _VerifyCannotApplyDeleteProperty(propToEdit,
                uneditableLayerMessage)
            _VerifyCannotApplyDeletePropertyAtPath(propPath,
                uneditableLayerMessage)

            # Cannot rename
            _VerifyCannotApplyRenameProperty(propToEdit, "New_Attr", 
                uneditableLayerMessage)
            _VerifyCannotApplyMovePropertyAtPath(propPath, 
                Sdf.Path(propPath).ReplaceName("New_Attr"), 
                uneditableLayerMessage)
            
            # Cannot reparent
            _VerifyCannotApplyReparentProperty(propToEdit, primA, 
                uneditableLayerMessage)
            _VerifyCannotApplyMovePropertyAtPath(propPath, 
                primA.GetPath().AppendProperty(propName), 
                uneditableLayerMessage)

            # Cannot reparent and rename
            _VerifyCannotReparentAndRenameProperty(propToEdit, primA, 'New_Attr',
                uneditableLayerMessage)
            _VerifyCannotApplyMovePropertyAtPath(propPath, 
                primA.GetPath().AppendProperty('New_Attr'), 
                uneditableLayerMessage)

        # Name, path, and reparent target of properties on the basic stage that 
        # do NOT have specs in sublayer1. These properties are still editable
        # even though sublayer1 can't be edited.
        propsWithoutSpecsInSub1 = [
            ('C_Root_Attr', '/C.C_Root_Attr', '/C/B/A'),
            ('B_Root_Attr', '/C/B.B_Root_Attr', '/C'),
            ('A_Root_Attr', '/C/B/A.A_Root_Attr', '/C/B')]

        for propName, propPath, newParentPath in propsWithoutSpecsInSub1:

            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)

            # Helper for verifying that the following edits to the properties 
            # that aren't affected by sublayer1 were performed followed by 
            # resetting the stage for the next edit.
            def _VerifyPropertyWasEditedAndReset(propPath, newPropPath = None):
                self.assertTrue(editor.CanApplyEdits())
                self.assertTrue(editor.ApplyEdits())

                # The successful edits on these properties will affect the root 
                # layer but won't affect sublayer1
                self.assertTrue(stage.GetRootLayer().dirty)
                self.assertFalse(subLayer1.dirty)

                # No object will exist at the original path but for a move edit, 
                # the property will exist at a new path.
                self.assertFalse(stage.GetObjectAtPath(propPath))
                if newPropPath:
                    self.assertTrue(stage.GetPropertyAtPath(newPropPath))

                # Reset by reloading the root layer and get the property back
                # again
                stage.GetRootLayer().Reload()
                nonlocal prop
                primA = stage.GetPropertyAtPath(propPath)
                self.assertTrue(prop)

            # Can delete the property
            self.assertTrue(editor.DeleteProperty(prop))
            _VerifyPropertyWasEditedAndReset(propPath)

            newPropName = propName + "_New"
            newParentPrim = stage.GetPrimAtPath(newParentPath)

            # Can rename the property
            newPropPath = propPath + "_New"
            self.assertTrue(editor.RenameProperty(prop, newPropName))
            _VerifyPropertyWasEditedAndReset(propPath, newPropPath)

            # Can reparent property without rename
            newPropPath = newParentPrim.GetPath().AppendProperty(propName)
            self.assertTrue(editor.ReparentProperty(prop, newParentPrim))
            _VerifyPropertyWasEditedAndReset(propPath, newPropPath)

            # Can reparent property with rename
            newPropPath = newParentPrim.GetPath().AppendProperty(newPropName)
            self.assertTrue(
                editor.ReparentProperty(prop, newParentPrim, newPropName))
            _VerifyPropertyWasEditedAndReset(propPath, newPropPath)
        
        subLayer1.SetPermissionToEdit(True)

    def test_CanEditBuiltinProperties(self):
        """Tests that we can't namespace edit properties that are built-in 
        properties of a prim's schema type.
        """

        # Open the basic stage for this test.
        rootLayer = Sdf.Layer.FindOrOpen("basic/root.usda")
        stage = Usd.Stage.Open(rootLayer)

        editor = Usd.NamespaceEditor(stage)

        # The basic stage a root prim named "PrimWithCollection" that has an
        # instance of CollectionAPI named "foo" applied. Note that we use 
        # CollectionAPI as its the only schema entirely defined in libUsd that 
        # can be used to apply built-in properties to prim
        primWithCollectionPath = Sdf.Path('/PrimWithCollection')
        primWithCollection = stage.GetPrimAtPath(primWithCollectionPath)
        self.assertTrue(primWithCollection)
        self.assertTrue(primWithCollection.HasAPI(Usd.CollectionAPI, "foo"))

        # Helper for verifying that a built-in property cannot be namespace 
        # edited.
        def _VerifyCannotEditBuiltinProperty(prop):
            self.assertTrue(prop)

            # The same error message will occur for any operation that would
            # only fail because the property is built-in.
            expectedMessage = \
                "The property to edit is a built-in property of its prim"

            # Cannot delete
            self.assertTrue(editor.DeleteProperty(prop))
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()

            # Cannot rename
            newName = "NewName"    
            self.assertTrue(editor.RenameProperty(prop, newName))
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()

            # Cannot reparent
            newParent = stage.GetPrimAtPath("/C")
            self.assertTrue(editor.ReparentProperty(prop, newParent))
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()

            # The basic stage should have 4 layers: the session layer, the root
            # layer, and its two sublayers. After all the failed edits, none of
            # the stage's layers should have been edited
            stageLayers = stage.GetUsedLayers()
            self.assertEqual(len(stageLayers), 4)
            for layer in stageLayers:
                self.assertFalse(layer.dirty)

        # Helper for verifying that a custom property (not built-in) at the
        # given path can be edited
        def _VerifyCanEditCustomPropertyAtPath(propPath):
            # Get and verify the property at path
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)
            self.assertTrue(prop.IsAuthored())

            # Verify we can delete the property
            self.assertTrue(editor.DeleteProperty(prop))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())

            # Verify the property is deleted then reset
            self.assertFalse(stage.GetPropertyAtPath(propPath))
            stage.Reload()

            # Get and verify the property at path
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)
            self.assertTrue(prop.IsAuthored())

            # Verify we can renanme the property
            newName = "NewName"    
            self.assertTrue(editor.RenameProperty(prop, newName))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())

            # Verify the property is moved then reset
            self.assertFalse(stage.GetPropertyAtPath(propPath))
            self.assertTrue(stage.GetPropertyAtPath(propPath.ReplaceName(newName)))
            stage.Reload()

            # Get and verify the property at path
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)
            self.assertTrue(prop.IsAuthored())
            
            # Verify we can renparent the property
            newParent = stage.GetPrimAtPath("/C")
            self.assertTrue(editor.ReparentProperty(prop, newParent))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())

            # Verify the property is moved then reset
            self.assertFalse(stage.GetPropertyAtPath(propPath))
            self.assertTrue(newParent.GetProperty(propPath.name))
            stage.Reload()

        # The following built-in properties are provided only by the prim's 
        # applied schema and have no authored opinions on the stage at all. 
        # These properties can not be edited.
        builtinFallbackProps = [
            'collection:foo', 
            'collection:foo:excludes', 
            'collection:foo:expansionRule', 
            'collection:foo:membershipExpression']
        for propName in builtinFallbackProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            prop = stage.GetPropertyAtPath(propPath)
            self.assertFalse(prop.IsAuthored())
            _VerifyCannotEditBuiltinProperty(prop)

        # These built-in properties DO have authored opinions on the stage, but
        # since they're also built-in properties of the applied schema, they
        # still cannot be namespace edited.
        builtinAuthoredProps = [
            'collection:foo:includeRoot', 
            'collection:foo:includes']
        for propName in builtinAuthoredProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop.IsAuthored())
            _VerifyCannotEditBuiltinProperty(prop)

        # These properties are not built-in to the API schema and only exist
        # as opinions on the stage. These properties CAN be edited.
        customProps = ['nonCollectionAttr']
        for propName in customProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            _VerifyCanEditCustomPropertyAtPath(propPath)

        # Reopen the stage but with a session layer that removes the 
        # CollectionAPI:foo applied schema from "PrimWithCollection"
        sessionLayer = Sdf.Layer.FindOrOpen("basic/session.usda")    
        stage = Usd.Stage.Open(rootLayer, sessionLayer)

        # We have to recreate the namespace editor with the newly opened stage
        # as the previous editor was attached to the stage without the session 
        # layer.
        editor = Usd.NamespaceEditor(stage)

        # Verify the prim no longer has the API schema applied.
        primWithCollection = stage.GetPrimAtPath(primWithCollectionPath)
        self.assertTrue(primWithCollection)
        self.assertFalse(primWithCollection.HasAPI(Usd.CollectionAPI, "foo"))

        # The built-in only properties now no longer exist because there were
        # no opinions on the stage.
        for propName in builtinFallbackProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            prop = stage.GetPropertyAtPath(propPath)
            self.assertFalse(prop)

        # The built-in properties that had authored opinions on the stage are 
        # no longer built-in and can now be edited as custom properties.
        for propName in builtinAuthoredProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            _VerifyCanEditCustomPropertyAtPath(propPath)

        # The custom properties that were never built-ins can still be edited.
        for propName in customProps:
            propPath = primWithCollectionPath.AppendProperty(propName)
            _VerifyCanEditCustomPropertyAtPath(propPath)

    def test_EditPropertiesWithInstancing(self):
        """Tests namespace edit operations on properties with native instancing.
        """

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

            return (stage, instance1, instance2, nonInstancePrim, prototypePrim)

        # Helper to verify that CanApplyEdits returns false with the expected
        # message and that ApplyEdits fails and doesn't edit the stage.
        def _VerifyCannotApplyEdits(expectedMessage):
            self._VerifyFalseResult(editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()
            self.assertFalse(stage.GetRootLayer().dirty)

        # Helpers to verify that a valid [Delete|Rename|Reparent]Property 
        # operation can be added but cannot be applied to the editor's stage for
        # the expected reason.
        def _VerifyCannotApplyDeleteProperty(prop, expectedMessage):
            self.assertTrue(editor.DeleteProperty(prop))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyRenameProperty(prop, newName, expectedMessage):
            self.assertTrue(editor.RenameProperty(prop, newName))
            _VerifyCannotApplyEdits(expectedMessage)

        def _VerifyCannotApplyReparentProperty(prop, newParent, expectedMessage):
            self.assertTrue(editor.ReparentProperty(prop, newParent))
            _VerifyCannotApplyEdits(expectedMessage)

        # Helper to verify that the prim at that path can be successfully
        # deleted. Resets the stage to be unedited afterward.
        def _VerifyCanDeletePropertyAtPath(propPath):
            # Verify the property actually exists first
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)

            # Verify that we can delete the property, delete it, and make sure
            # it no longer exists on the stage.
            self.assertTrue(editor.DeleteProperty(prop))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPropertyAtPath(propPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the property at that path can be successfully
        # moved to a new path. Resets the stage to be unedited afterward.
        def _VerifyCanMovePropertyAtPath(propPath, newPropPath):
            # Verify the property actually exists first, and make sure the 
            # new property does not exist.
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)
            self.assertFalse(stage.GetPropertyAtPath(newPropPath))

            # Verify that we can move the property, move it, and make sure it
            # no longer exists at the old path but does exist at the new path.
            self.assertTrue(
                editor.MovePropertyAtPath(propPath, newPropPath))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPropertyAtPath(propPath))
            self.assertTrue(stage.GetPropertyAtPath(newPropPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper for verifying a property that is invalid to edit because of
        # necessitating relocates as opposed to any requirements related to
        # instancing.
        def _VerifyCannotEditPropertyWithoutRelocates(prop):
            expectedMessage = "The property to edit requires authoring " \
                "relocates since it composes opinions introduced by " \
                "ancestral composition arcs; authoring relocates is not " \
                "supported for properties"
            _VerifyCannotApplyDeleteProperty(prop, expectedMessage)
            _VerifyCannotApplyRenameProperty(prop, "New_Attr", expectedMessage)
            _VerifyCannotApplyReparentProperty(prop, basicRootPrim, 
                                               expectedMessage)

        # Open the stage and get the prims to test.
        stage, instance1, instance2, nonInstancePrim, prototypePrim = \
            _SetupInstancingStage()

        # Simple root prim to be target for some reparent cases.
        basicRootPrim = stage.GetPrimAtPath("/BasicRootPrim")
        self.assertTrue(basicRootPrim)

        editor = Usd.NamespaceEditor(stage)

        # First verify that we can't edit ReferencePrim_Attr from any of 
        # instance and non-instance prims without relocates as it is defined
        # across a reference.
        _VerifyCannotEditPropertyWithoutRelocates(
            instance1.GetProperty("ReferencePrim_Attr"))
        _VerifyCannotEditPropertyWithoutRelocates(
            instance2.GetProperty("ReferencePrim_Attr"))
        _VerifyCannotEditPropertyWithoutRelocates(
            nonInstancePrim.GetProperty("ReferencePrim_Attr"))

        # We can delete local properties of both the instance and non-instance 
        # prims as long as they are not defined across the reference (i.e. 
        # would need relocates).
        _VerifyCanDeletePropertyAtPath("/Instance1.Instance1_Attr")
        _VerifyCanDeletePropertyAtPath("/Instance2.Instance2_Attr")
        _VerifyCanDeletePropertyAtPath("/NonInstancePrim.NonInstance_Attr")

        # Like with delete, we can rename these same instance and non-instance 
        # properties (as long as the new name is valid).
        _VerifyCanMovePropertyAtPath(
            "/Instance1.Instance1_Attr", "/Instance1.NewAttr")
        _VerifyCanMovePropertyAtPath(
            "/Instance2.Instance2_Attr", "/Instance2.NewAttr")
        _VerifyCanMovePropertyAtPath(
            "/NonInstancePrim.NonInstance_Attr", "/NonInstancePrim.NewAttr")

        # We can also reparent these properties...
        # ...from an instance to another instance
        _VerifyCanMovePropertyAtPath(
            "/Instance1.Instance1_Attr", "/Instance2.Instance1_Attr")

        # ...from an instance to a non-instance prim
        _VerifyCanMovePropertyAtPath(
            "/Instance2.Instance2_Attr", "/NonInstancePrim.Instance2_Attr")

        # ...from a non-instance prim to an instance prim.
        _VerifyCanMovePropertyAtPath(
            "/NonInstancePrim.NonInstance_Attr", "/Instance1.NonInstance_Attr")

        # We can't delete, rename, or reparent any of the properties of the 
        # instance prim's children as they are proxies from the prototype. 
        prop = instance1.GetChild("B").GetProperty("B_Attr")
        _VerifyCannotApplyDeleteProperty(prop, 
            "The property to edit belongs to an instance prototype proxy")
        _VerifyCannotApplyRenameProperty(prop, "New_Attr", 
            "The property to edit belongs to an instance prototype proxy")
        _VerifyCannotApplyReparentProperty(prop, basicRootPrim, 
            "The property to edit belongs to an instance prototype proxy")

        prop = instance2.GetChild("B").GetProperty("B_Attr")
        _VerifyCannotApplyDeleteProperty(prop, 
            "The property to edit belongs to an instance prototype proxy")
        _VerifyCannotApplyRenameProperty(prop, "New_Attr", 
            "The property to edit belongs to an instance prototype proxy")
        _VerifyCannotApplyReparentProperty(prop, basicRootPrim, 
            "The property to edit belongs to an instance prototype proxy")

        # We also can't edit the same child prim properties of the non-instance 
        # prim but it's because of relocates as they're still defined across a 
        # reference.
        _VerifyCannotEditPropertyWithoutRelocates(
            nonInstancePrim.GetChild("B").GetProperty("B_Attr"))

        # We also can't reparent an otherwise valid property under the child of
        # an instance prim as instance proxies can't have local property 
        # opinions.
        newParentPrim = instance1.GetChild("B")
        _VerifyCannotApplyReparentProperty(
            instance1.GetProperty("Instance1_Attr"), newParentPrim, 
            "The new parent prim is a prototype proxy descendant of an "
            "instance prim")
        _VerifyCannotApplyReparentProperty(
            instance2.GetProperty("Instance2_Attr"), newParentPrim, 
            "The new parent prim is a prototype proxy descendant of an "
            "instance prim")
        _VerifyCannotApplyReparentProperty(
            nonInstancePrim.GetProperty("NonInstance_Attr"), newParentPrim, 
            "The new parent prim is a prototype proxy descendant of an "
            "instance prim")

        # But these same properties can still be reparented under the child
        # of the non-instance prim as there is no such local property opinion
        # restriction.
        _VerifyCanMovePropertyAtPath(
            "/Instance1.Instance1_Attr", "/NonInstancePrim/B.Instance1_Attr")
        _VerifyCanMovePropertyAtPath(
            "/Instance2.Instance2_Attr", "/NonInstancePrim/B.Instance2_Attr")
        _VerifyCanMovePropertyAtPath(
            "/NonInstancePrim.NonInstance_Attr", 
            "/NonInstancePrim/B.NonInstance_Attr")

        # No editing operations can be performed with a prototype prim or its 
        # children ever. The prototype prim itself can't have its own properties
        self.assertFalse(prototypePrim.GetProperties())

        protoTypeChildProp = prototypePrim.GetChild("B").GetProperty("B_Attr")
        _VerifyCannotApplyDeleteProperty(protoTypeChildProp, 
            "The property to edit belongs to a prototype prim")
        _VerifyCannotApplyRenameProperty(protoTypeChildProp, "New_Attr", 
            "The property to edit belongs to a prototype prim")
        _VerifyCannotApplyReparentProperty(protoTypeChildProp, basicRootPrim, 
            "The property to edit belongs to a prototype prim")

        _VerifyCannotApplyReparentProperty(
            instance1.GetProperty("Instance1_Attr"), prototypePrim, 
            "The new parent prim belongs to a prototype prim")
        _VerifyCannotApplyReparentProperty(
            nonInstancePrim.GetProperty("NonInstance_Attr"), prototypePrim, 
            "The new parent prim belongs to a prototype prim")
        _VerifyCannotApplyReparentProperty(
            nonInstancePrim.GetProperty("NonInstance_Attr"), 
            prototypePrim.GetChild("B"), 
            "The new parent prim belongs to a prototype prim")

    def test_EditPropertiesWithCompositionArcs(self):
        """Tests namespace edit operations on properties with specs that 
        contribute opinions across composition arcs.
        """

        # This stage has few variety of composition arcs on the root prims with
        # properties defined both the local layer stack and across these arcs.
        stage = Usd.Stage.Open("composition_arcs/root.usda")
        self.assertTrue(stage)

        editor = Usd.NamespaceEditor(stage)

        # Helper functions for testing properties that we expect to be able to 
        # successfully edit.

        # Helper to verify that the property at that path can be successfully
        # deleted. Resets the stage to be unedited afterward.
        def _VerifyCanDeletePropertyAtPath(propPath):
            # Verify the property actually exists first
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)

            # Verify that we can delete the property, delete it, and make sure
            # it no longer exists on the stage.
            self.assertTrue(editor.DeleteProperty(prop))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPropertyAtPath(propPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the property at that path can be successfully
        # moved to a new path. Resets the stage to be unedited afterward.
        def _VerifyCanMovePropertyAtPath(propPath, newPropPath):
            # Verify the property actually exists first, and make sure the 
            # new property does not exist.
            prop = stage.GetPropertyAtPath(propPath)
            self.assertTrue(prop)
            self.assertFalse(stage.GetPropertyAtPath(newPropPath))

            # Verify that we can move the property, move it, and make sure it
            # no longer exists at the old path but does exist at the new path.
            self.assertTrue(
                editor.MovePropertyAtPath(propPath, newPropPath))
            self.assertTrue(editor.CanApplyEdits())
            self.assertTrue(editor.ApplyEdits())
            self.assertFalse(stage.GetPropertyAtPath(propPath))
            self.assertTrue(stage.GetPropertyAtPath(newPropPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the prim can be both deleted and moved.
        def _VerifyCanEditPropertyAtPath(propPath):
            propPath = Sdf.Path(propPath)
            renamedPropPath = propPath.ReplaceName("NewPropName")
            reparentedPropPath = \
                Sdf.Path("/BasicRootPrim").AppendProperty(propPath.name)
            _VerifyCanDeletePropertyAtPath(propPath)
            _VerifyCanMovePropertyAtPath(propPath, renamedPropPath)
            _VerifyCanMovePropertyAtPath(propPath, reparentedPropPath)

        # Helper functions for testing prims that we expect to NOT be able to 
        # successfully edit.

        # Helper for extra verification that delete shouldn't work. This 
        # function finds all the specs for the property in the stage's root 
        # layer stack and deletes these specs from the layer. This is what 
        # delete operation would do if it were performed without first checking 
        # that it would succeed. We can call this to prove that deleting the 
        # root layer stack specs is not sufficient for deleting the prim from 
        # the stage.
        def _VerifyDeletingRootLayerStackSpecsDoesNotDeleteProperty(propPath):
            for layer in stage.GetLayerStack():
                spec = layer.GetPropertyAtPath(propPath)
                if spec:
                    del spec.owner.properties[spec.path.name]
                # No spec should exist for the property in this layer because it
                # either didn't exist to begin with or we just deleted it.
                self.assertFalse(layer.GetPropertyAtPath(propPath))

            # Verify the stage still has a property at the path after deleting 
            # all root layer stack specs
            self.assertTrue(stage.GetPropertyAtPath(propPath))

            # Reset the stage for the next case.
            stage.Reload()

        # Helper to verify that the property at that path cannot be deleted and 
        # fails with the expected error message.
        def _VerifyCannotApplyDeletePropertyAtPath(propPath, expectedMessage):
            # Verify the property actually exists first
            self.assertTrue(stage.GetPropertyAtPath(propPath))

            # Verify that we cannot delete the property.
            self.assertTrue(editor.DeletePropertyAtPath(propPath))
            self._VerifyFalseResult(
                editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()

            # Verify that the deleting root layer stack specs would indeed not
            # delete the property.
            _VerifyDeletingRootLayerStackSpecsDoesNotDeleteProperty(propPath)

        # Helper to verify that the property at that path cannot be moved and 
        # fails with the expected error message.
        def _VerifyCannotApplyMovePropertyAtPath(
                propPath, newPropPath, expectedMessage):
            # Verify the property actually exists first
            self.assertTrue(stage.GetPropertyAtPath(propPath))

            # Verify that we cannot move the property.
            self.assertTrue(editor.MovePropertyAtPath(propPath, newPropPath))
            self._VerifyFalseResult(
                editor.CanApplyEdits(), expectedMessage)
            with self.assertRaises(Tf.ErrorException):
                editor.ApplyEdits()

            # No need to verify that moving specs in the root layer stack would
            # not move the property; the delete case is sufficient for proving 
            # this.

        # Helper to verify that the property cannot be deleted nor moved.
        def _VerifyCannotEditPropertyAtPath(propPath):
            propPath = Sdf.Path(propPath)
            renamedPropPath = propPath.ReplaceName("NewPropName")
            reparentedPropPath = \
                Sdf.Path("/BasicRootPrim").AppendProperty(propPath.name)

            expectedMessage = "The property to edit requires authoring " \
                "relocates since it composes opinions introduced by " \
                "ancestral composition arcs; authoring relocates is not " \
                "supported for properties"
            _VerifyCannotApplyDeletePropertyAtPath(propPath, expectedMessage)
            _VerifyCannotApplyMovePropertyAtPath(propPath, renamedPropPath, 
                                                 expectedMessage)
            _VerifyCannotApplyMovePropertyAtPath(propPath, reparentedPropPath, 
                                                 expectedMessage)

        # /PrimWithReference has a direct reference to @ref.usda@</ReferencePrim>

        # Can still edit property Root_PrimWithReference_Attr which only has
        # specs in the root layer stack.
        _VerifyCanEditPropertyAtPath(
            "/PrimWithReference.Root_PrimWithReference_Attr")

        # Cannot edit Ref_ReferencePrim_Attr which has specs in referenced prim
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithReference.Ref_ReferencePrim_Attr")

        # Cannot edit Ref_RefClass_Attr which has specs in a class prim that is
        # inherited by the referenced prim.
        _VerifyCannotEditPropertyAtPath("/PrimWithReference.Ref_RefClass_Attr")

        # Cannot edit ClassChild.ClassChild.Ref_ClassChild_Attr which has specs
        # in the child prim of the class inherited by the referenced prim.
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithReference/ClassChild.Ref_ClassChild_Attr")

        # Cannot edit ClassChild.ClassChild.Root_ClassChild_Attr even though the
        # specs are only in the root layer as they are defined in a prim that 
        # is an implied inherit propagated from the referenced prim..
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithReference/ClassChild.Root_ClassChild_Attr")

        # Can still edit property Root_B_Attr of the child prim "B" as it only 
        # has specs in the root layer stack.
        _VerifyCanEditPropertyAtPath("/PrimWithReference/B.Root_B_Attr")

        # Cannot edit Ref_B_Attr of child prim "B" which has specs in the 
        # ancestrally referenced prim.
        _VerifyCannotEditPropertyAtPath("/PrimWithReference/B.Ref_B_Attr")

        # Cannot edit Ref_A_Attr of grandchild prim "B/A" which has specs in the 
        # ancestrally referenced prim.
        _VerifyCannotEditPropertyAtPath("/PrimWithReference/B/A.Ref_A_Attr")

        # /PrimWithSubrootReference has a direct reference to 
        # @ref.usda@</ReferencePrim/B> which is a subroot reference.

        # Can still edit property Root_B_Attr as it only has specs in the root 
        # layer stack.
        _VerifyCanEditPropertyAtPath("/PrimWithSubrootReference.Root_B_Attr")

        # Cannot edit Ref_B_Attr which has specs in referenced prim
        _VerifyCannotEditPropertyAtPath("/PrimWithSubrootReference.Ref_B_Attr")

        # Can still edit property Root_A_Attr of the child prim "A" as it only 
        # has specs in the root layer stack.
        _VerifyCanEditPropertyAtPath("/PrimWithSubrootReference/A.Root_A_Attr")

        # Cannot edit Ref_A_Attr of child prim "A" which has specs in the 
        # ancestrally referenced prim.
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithSubrootReference/A.Ref_A_Attr")

        # /PrimWithVariant has a variant selection of {v=one} of which this 
        # variant has a reference to @ref.usda@</ReferencePrim>

        # Can edit Root_OutsideTheVariant_Attr which only has specs in the local
        # layer stack and they are outside the variant.
        _VerifyCanEditPropertyAtPath(
            "/PrimWithVariant.Root_OutsideTheVariant_Attr")

        # Cannot edit Root_VariantOne_Attr even though it only has specs in the
        # local layer stack as these specs are defined inside the variant 
        # itself.
        _VerifyCannotEditPropertyAtPath("/PrimWithVariant.Root_VariantOne_Attr")

        # Cannot edit the attributes that has specs defined inside the reference
        # or the class it inherits.
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithVariant.Ref_ReferencePrim_Attr")
        _VerifyCannotEditPropertyAtPath("/PrimWithVariant.Ref_RefClass_Attr")
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithVariant/ClassChild.Root_ClassChild_Attr")
        _VerifyCannotEditPropertyAtPath(
            "/PrimWithVariant/ClassChild.Ref_ClassChild_Attr")

if __name__ == '__main__':
    unittest.main()
