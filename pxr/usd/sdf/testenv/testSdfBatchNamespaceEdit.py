#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf
import unittest, os

verbose = False
#verbose = True

class TestSdfBatchNamespaceEdit(unittest.TestCase):
    def CheckFail(self, result, description):
        """
        Verify that the returned result of edit.Process(...) returns False with 
        the given description.
        """
        if verbose:
            if result[0]:
                print("Unexpected success: %s" % description)
            else:
                print("%40s: [%s]" % (description, '.'.join([str(x) for x in result[1]])))
        self.assertFalse(result[0])

    def AssertCanApply(self, layer, edit):
        """
        Assert that layer.CanApply(edit) returns true.

        Simply calling self.assertTrue(layer.CanApply(edit)) doesn't work 
        because CanApply returns a tuple on failure (bool, reason), which 
        evaluates to True as a bool.
        """
        canApply = layer.CanApply(edit)
        if isinstance(canApply, bool):
            self.assertTrue(canApply)
        else:
            self.assertTrue(*canApply)

    def test_EqualityOperators(self):
        self.assertEqual(Sdf.NamespaceEdit('/A', '/B'), Sdf.NamespaceEdit('/A', '/B'))
        self.assertNotEqual(Sdf.NamespaceEdit('/B', '/A'), Sdf.NamespaceEdit('/A', '/B'))

        # Validate the equality operators by varying a single field from a prototype edit
        # at a time
        prototype = Sdf.NamespaceEditDetail(
            Sdf.NamespaceEditDetail.Okay,
            Sdf.NamespaceEdit('/A', '/B'),
            "reason"
        )
        self.assertEqual(prototype, Sdf.NamespaceEditDetail(
            prototype.result,
            prototype.edit,
            prototype.reason))
        self.assertNotEqual(prototype, Sdf.NamespaceEditDetail(
            Sdf.NamespaceEditDetail.Unbatched,
            prototype.edit,
            prototype.reason))
        self.assertNotEqual(prototype, Sdf.NamespaceEditDetail(
            prototype.result,
            Sdf.NamespaceEdit('/C', '/D'),
            prototype.reason))
        self.assertNotEqual(prototype, Sdf.NamespaceEditDetail(
            prototype.result,
            prototype.edit,
            "a different reason"))


    def test_Basic(self):
        print('Test constructors')

        testEdits = [
            Sdf.NamespaceEdit('/C', '/D'),
            Sdf.NamespaceEdit('/B', '/C'),
            Sdf.NamespaceEdit('/D', '/B'),
            Sdf.NamespaceEdit('/X', '')
        ]
        edit = Sdf.BatchNamespaceEdit()
        self.assertEqual(edit.edits, [])
        edit2 = Sdf.BatchNamespaceEdit(edit)
        self.assertEqual(edit2.edits, [])
        edit = Sdf.BatchNamespaceEdit(testEdits)
        self.assertEqual(edit.edits, testEdits)
        edit2 = Sdf.BatchNamespaceEdit(edit)
        self.assertEqual(edit2.edits, testEdits)

        print('\nTest repr')

        edit = Sdf.BatchNamespaceEdit()
        self.assertEqual(edit.edits, eval(repr(edit)).edits)
        self.assertEqual(edit2.edits, eval(repr(edit2)).edits)

        print('\nTest Add()')

        edit.Add(Sdf.NamespaceEdit('/C', '/D'))
        edit.Add('/B', '/C')
        edit.Add(edit2.edits[-2])
        edit.Add('/X', Sdf.Path.emptyPath)
        self.assertEqual(edit.edits, edit2.edits)

        print('\nTest Process()')

        layer = Sdf.Layer.FindOrOpen('testSdfBatchNamespaceEdit.testenv/test.sdf')
        final = Sdf.Layer.FindOrOpen('testSdfBatchNamespaceEdit.testenv/final.sdf')
        self.assertTrue(layer is not None)
        self.assertTrue(final is not None)

        #
        # Try things that work.
        #
        def Has(layer, path):
            return bool(layer.GetObjectAtPath(path))

        # Try everything.
        hasObject = lambda p: Has(layer, p)
        canEdit   = lambda e: True
        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D')                # Prim renames
        edit.Add('/B', '/C')
        edit.Add('/D', '/B')
        edit.Add('/G', '/E/G')              # Prim reparents
        edit.Add('/H', '/E/F/H')
        edit.Add('/I', '/E/H')              # Prim reparent/rename
        edit.Add('/J', '/L/J')              # Prim reparent
        edit.Add('/L/J/K', '/K')            # Prim reparent from under a reparented prim
        edit.Add('/X', Sdf.Path.emptyPath)  # Prim remove
        edit.Add('/E', Sdf.Path.emptyPath)  # Prim with descendants remove

        edit.Add('/P.c', '/P.d')            # Prim property renames
        edit.Add('/P.b', '/P.c')
        edit.Add('/P.d', '/P.b')
        edit.Add('/P.g', '/Q.g')            # Prim property reparents
        edit.Add('/P.h', '/Q/R.h')
        edit.Add('/P.i', '/Q.h')            # Prim property reparent/rename
        edit.Add('/P.x', Sdf.Path.emptyPath)# Prim property remove

        edit.Add('/S', '/T')                        # Rename prim used in targets

        edit.Add('/V{v=one}U', '/V{v=two}W/U')      # Variant prim reparent/rename
        edit.Add('/V{v=two}W', Sdf.Path.emptyPath)  # Variant prim remove
        edit.Add('/V{v=one}.u', '/V{v=two}.u')      # Variant property reparent/rename
        edit.Add('/V{v=two}.w', Sdf.Path.emptyPath) # Variant property remove

        self.assertEqual(edit.Process(hasObject, canEdit), (True, edit.edits))

        # Try it again.  Since nothing actually changed in layer this should work.
        self.assertEqual(edit.Process(hasObject, canEdit), (True, edit.edits))

        # Try an additional edit.  This one will not be returned by Process() because
        # it's redundant.
        edit.Add('/E/F', Sdf.Path.emptyPath)# Remove implicitly removed prim
        self.assertEqual(edit.Process(hasObject, canEdit)[0], True)

        #
        # Try lots of things that don't work.
        #
        CheckFail = lambda result, description: \
            self.CheckFail(result, description)

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/Y', '/Z')                    # Unknown object.
        CheckFail(edit.Process(hasObject, canEdit), "Unknown object")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/X', '/B')                    # Object exists at new path.
        CheckFail(edit.Process(hasObject, canEdit), "Object exists at new path")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/X', '/B/Y/Z')                # Object doesn't exist for parent.
        CheckFail(edit.Process(hasObject, canEdit), "Parent object doesn't exist")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/E', '/E/F/E')                # Object would be its own ancestor.
        CheckFail(edit.Process(hasObject, canEdit), "Object would be its own ancestor")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/G', '/E/G')
        edit.Add('/H', '/E/G')                  # Object will already exist.
        CheckFail(edit.Process(hasObject, canEdit), "Object will already exist")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/', '/')                      # Unsupported path.
        CheckFail(edit.Process(hasObject, canEdit), "Unsupported path: root")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/X.a.mapper[/B]', '/Z')       # Unsupported path.
        CheckFail(edit.Process(hasObject, canEdit), "Unsupported path: mapper")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/X.a.mapper[/B].c', '/Z')     # Unsupported path.
        CheckFail(edit.Process(hasObject, canEdit), "Unsupported path: mapper arg")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/X.a.expression', '/Z')       # Unsupported path.
        CheckFail(edit.Process(hasObject, canEdit), "Unsupported path: expression")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/Z', '/Z.z')                  # Path type mismatch.
        CheckFail(edit.Process(hasObject, canEdit), "Path type mismatch")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/Z', '/Z.z[/Z].z')            # Path type mismatch.
        CheckFail(edit.Process(hasObject, canEdit), "Path type mismatch")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/Z.z', '/Z')                  # Path type mismatch.
        CheckFail(edit.Process(hasObject, canEdit), "Path type mismatch")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D')                    # hasObject fails.
        CheckFail(edit.Process(lambda p: False, canEdit), "hasObject === False")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D')                    # canEdit fails.
        CheckFail(edit.Process(hasObject, lambda e: (False, "Can't edit")), "canEdit fails")

        print('\nTest live edits on layer')
        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D', Sdf.NamespaceEdit.same) # Prim renames
        edit.Add('/B', '/C', Sdf.NamespaceEdit.same)
        edit.Add('/D', '/B', Sdf.NamespaceEdit.same)
        edit.Add('/G', '/E/G')              # Prim reparents
        edit.Add('/H', '/E/F/H')
        edit.Add('/I', '/E/H')              # Prim reparent/rename
        edit.Add('/J', '/L/J')              # Prim reparent
        edit.Add('/L/J/K', '/K')            # Prim reparent from under a reparented prim
        edit.Add('/X', Sdf.Path.emptyPath)  # Prim remove
        edit.Add('/E', Sdf.Path.emptyPath)  # Prim with descendants remove
        edit.Add('/E/F', Sdf.Path.emptyPath)# Remove implicitly removed prim

        edit.Add('/P.c', '/P.d')            # Prim property renames
        edit.Add('/P.b', '/P.c')
        edit.Add('/P.d', '/P.b')
        edit.Add('/P.g', '/Q.g')            # Prim property reparents
        edit.Add('/P.h', '/Q/R.h')
        edit.Add('/P.i', '/Q.h')            # Prim property reparent/rename
        edit.Add('/P.x', Sdf.Path.emptyPath)# Prim property remove

        edit.Add('/V{v=one}U', '/V{v=two}W/U')      # Variant prim reparent/rename
        edit.Add('/V{v=two}W', Sdf.Path.emptyPath)  # Variant prim remove
        edit.Add('/V{v=one}.u', '/V{v=two}.u')      # Variant property reparent/rename
        edit.Add('/V{v=two}.w', Sdf.Path.emptyPath) # Variant property remove

        self.AssertCanApply(layer, edit)
        layer.Apply(edit)
        self.assertEqual(layer.ExportToString(), final.ExportToString())

        print('\nTest SUCCEEDED')

    def test_DescendantMoveDeadspace(self):
        """
        Regression test: If a prim has deadspace under it (i.e. a child was 
        moved), then that deadspace should move with the prim if the prim gets 
        moved.
        """
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString("""#sdf 1.4.32
            def Prim "A" {
                custom double a
                def Prim "B" {
                    custom double b
                }
            }""")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add(Sdf.NamespaceEdit("/A/B", "/_B"))
        edit.Add(Sdf.NamespaceEdit("/A", "/_A"))
        edit.Add(Sdf.NamespaceEdit("/_B", "/_A/B"))

        self.AssertCanApply(layer, edit)
        self.assertTrue(layer.Apply(edit))

        # </A> -> </_A>
        self.assertTrue(layer.GetPropertyAtPath("/_A.a"))

        # </A/B> -> </_B> -> </_A/B>
        self.assertTrue(layer.GetPropertyAtPath("/_A/B.b"))

    def test_DescendantPreserveDeadspace(self):
        """
        Regression test: If a child of a parent was moved or removed, and then 
        the parent prim was moved but then moved back, then the edit operation
        should know that the child's former path is still empty ("deadspace").
        """
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString("""#sdf 1.4.32
            def Prim "A" {
                custom double a
                def Prim "B" {
                    custom double b
                }
            }
            def Prim "C" {
                custom double c
            }""")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add(Sdf.NamespaceEdit("/A/B", "/_B"))
        edit.Add(Sdf.NamespaceEdit("/A", "/_A"))
        edit.Add(Sdf.NamespaceEdit("/_A", "/A"))
        edit.Add(Sdf.NamespaceEdit("/C", "/A/B"))

        self.AssertCanApply(layer, edit)
        self.assertTrue(layer.Apply(edit))

        # </A/B> -> </_B>
        self.assertTrue(layer.GetPropertyAtPath("/_B.b"))

        # </A> -> </_A> -> </A>
        self.assertTrue(layer.GetPropertyAtPath("/A.a"))

        # </C> -> </A/B>
        self.assertTrue(layer.GetPropertyAtPath("/A/B.c"))

    def test_ParentRemoved(self):
        """
        Regression test: Detect that a path is not available if that path was
        moved followed by an edit to its ancestor. Before, it would (correctly) 
        fail but with the incorrect reason, "Cannot reparent object under 
        itself." Instead, it should fail with "New parent was removed."
        """
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString("""#sdf 1.4.32
            def Prim "A" {
                def Prim "B" {}
            }
            def Prim "C" {}""")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add(Sdf.NamespaceEdit("/A/B", "/_B"))
        edit.Add(Sdf.NamespaceEdit("/A", "/_A"))
        edit.Add(Sdf.NamespaceEdit("/_B", "/_A/B/C"))

        hasObject = lambda path: bool(layer.GetObjectAtPath(path))
        canEdit = lambda edit: True
        self.CheckFail(edit.Process(hasObject, canEdit), 
            "New parent was removed")

if __name__ == "__main__":
    unittest.main()
