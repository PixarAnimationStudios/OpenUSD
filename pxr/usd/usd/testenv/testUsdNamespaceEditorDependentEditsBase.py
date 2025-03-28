#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import contextlib, inspect, sys, unittest
from pxr import Sdf, Usd

class TestUsdNamespaceEditorDependentEditsBase(unittest.TestCase):
    '''Base class for testUsdNamespaceEditDependentEditsXXX tests which 
    provides share utilities for verifying outcomes of edits.
    '''

    @contextlib.contextmanager
    def ApplyEdits(self, editor, label, expectWarnings = False):
        '''Context manager for adding a namespace edit on a Usd.NameespaceEditor
        that will automatically verify that it can be applied and will apply it
        verifying a successful apply. It also prints out useful information to 
        help keep test output organized.

        Example Usage:
            editor = Usd.NamespaceEditor(stage)
            with self.ApplyEdits(editor, "Move /Foo to /Bar"):
                self.assertTrue(editor.MovePrimAtPath('/Foo', '/Bar'))
        '''

        # The enclosing test case function name to output with the begin and 
        # end messages.
        testFunctionName = next((x.function for x in inspect.stack() 
                                 if x.function.startswith("test_")), "")
        msg = testFunctionName + " : " + label
        print("\n==== Begin ApplyEdits : {} ====".format(msg))
        print("\n==== Begin ApplyEdits : {} ====".format(msg), file=sys.stderr)

        yield
        
        if expectWarnings:
            print("\n=== EXPECT WARNINGS ===", file=sys.stderr)
        self.assertTrue(editor.CanApplyEdits())
        self.assertTrue(editor.ApplyEdits())       
        if expectWarnings:
            print("\n=== END EXPECTED WARNINGS ===", file=sys.stderr)

        print("==== End ApplyEdits : {} ====".format(msg))
        print("==== End ApplyEdits : {} ====".format(msg), file=sys.stderr)

    def _VerifyPrimContents(self, prim, expectedContentsDict):
        '''Helper that verifies the contents of a USD prim, specifically its
        child prims and properties, match the given expected contents.

        A prims expected contents are expressed as a dictionary of the form
        
        { 
            '.' : ['propName1', 'propName2'],
            'Child1' : {...child prim expected contents...},
            'Child2' : {...child prim expected contents...}
        }
        '''

        # '.' is used to key the list of expected property names for the prim 
        expectedPropertyNameSet = set(expectedContentsDict.get('.', []))

        # Get the actual property names and compare against the expected set.
        propertyNameSet = set(prim.GetPropertyNames())
        self.assertEqual(propertyNameSet, expectedPropertyNameSet,
            "The actual property set {} does not match the expected property "
            "set {} for the prim at path {}".format(
                list(propertyNameSet), 
                list(expectedPropertyNameSet),
                prim.GetPath()))

        # Expected children names are all the expected contents keys except '.'
        expectedChildNameSet = set(
            [k for k in expectedContentsDict.keys() if k != '.'])

        # Get the actual child prim names and compare against the expected set.
        # We specifically want all children so we get unloaded prims and prims
        # that aren't defined (they are just overs)
        childrenNameSet = set(prim.GetAllChildrenNames())
        self.assertEqual(childrenNameSet, expectedChildNameSet,
            "The actual child prim set {} does not match the expected children "
            "set {} for the prim at path {}".format(
                list(childrenNameSet), 
                list(expectedChildNameSet), 
                prim.GetPath()))

        # Verify the expected contents of each expected child prim.
        for childName in expectedChildNameSet:
            childPath = prim.GetPath().AppendChild(childName)
            primChild = prim.GetPrimAtPath(childPath)
            self._VerifyPrimContents(primChild, expectedContentsDict[childName])

    def _VerifyStageContents(self, stage, expectedContentsDict):
        '''Helper that verifies the contents of every USD prim on the given 
        stage, specifically each's child prims and properties, match the given
        expected contents dictionary.'''

        self._VerifyPrimContents(stage.GetPseudoRoot(), expectedContentsDict)
    
    def _GetCompositionFieldsInLayer(self, layer):
        '''Helper that finds all prims specs with composition fields set in the
        given layer and returns a dictionary of prim paths to the fields and 
        their values.

        Example output:
        
        { 
            '/' : { 
                relocates : (('/Prim/Foo', '/Prim/Bar), )
            },
            '/Prim' : {
                references : [Sdf.Refence(layer, '/RefPath')],
                payload : [Sdf.Payload(layer, '/PayloadPath)]
            },
            '/PrimA/ChildA' : {
                inherits : ['/GlobalClass', '/PrimA/LocalClass']
            }
        }
        '''
        
        compositionFields = {}

        # Relocates are only in layer metadata so add them as belonging to the 
        # pseudoroot if there are any.
        if layer.HasRelocates():
            compositionFields ['/'] = {'relocates' : layer.relocates}

        def _GetCompositonFieldsForPrimAtPath(path) :
            if not path.IsPrimPath() and not path.IsPrimVariantSelectionPath():
                return

            def _AddListOpValueForField(fieldName, listOp):
                if not path in compositionFields:
                    compositionFields[str(path)] = {}
                compositionFields[str(path)][fieldName] = listOp.GetAppliedItems()

            prim = layer.GetPrimAtPath(path)
            if prim is None:
                return
            if prim.hasReferences:
                _AddListOpValueForField('references', prim.referenceList)
            if prim.hasPayloads:
                _AddListOpValueForField('payload', prim.payloadList)
            if prim.hasInheritPaths:
                _AddListOpValueForField('inherits', prim.inheritPathList)
            if prim.hasSpecializes:
                _AddListOpValueForField('specializes', prim.specializesList)

        layer.Traverse("/", _GetCompositonFieldsForPrimAtPath)
        return compositionFields

if __name__ == '__main__':
    unittest.main()
