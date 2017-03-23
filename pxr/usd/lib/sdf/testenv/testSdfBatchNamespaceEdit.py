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

from pxr import Sdf
import unittest, os

verbose = False
#verbose = True

class TestSdfBatchNamespaceEdit(unittest.TestCase):
    def test_Basic(self):
        print 'Test constructors'

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

        print '\nTest repr'

        edit = Sdf.BatchNamespaceEdit()
        self.assertEqual(edit.edits, eval(repr(edit)).edits)
        self.assertEqual(edit2.edits, eval(repr(edit2)).edits)

        print '\nTest Add()'

        edit.Add(Sdf.NamespaceEdit('/C', '/D'))
        edit.Add('/B', '/C')
        edit.Add(edit2.edits[-2])
        edit.Add('/X', Sdf.Path.emptyPath)
        self.assertEqual(edit.edits, edit2.edits)

        print '\nTest Process()'

        layer = Sdf.Layer.FindOrOpen('testSdfBatchNamespaceEdit.testenv/test.usda')
        final = Sdf.Layer.FindOrOpen('testSdfBatchNamespaceEdit.testenv/final.usda')
        self.assertTrue(layer is not None)
        self.assertTrue(final is not None)

        #
        # Try things that work.
        #

        # hasObject() must succeed when path is a target.  Sd does not have a
        # relationship target object.
        def Has(layer, path):
            if path.IsTargetPath():
                x = layer.GetObjectAtPath(path.GetParentPath())
                if x:
                    return x.targetAttributes[path.targetPath].IsValid()
                return False
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

        edit.Add('/P.j[/S.j].n', '/P.k[/P.j].n')    # Reparent relational attribute
        edit.Add('/S', '/T')                        # Rename prim used in targets
        edit.Add('/P.j[/T.j].m', '/P.j[/T.j].o')    # Rename relational attribute
        edit.Add('/P.k[/P.j].n', '/P.k[/P.j].o')    # Rename relational attribute
        edit.Add('/P.j[/T.j].p', Sdf.Path.emptyPath)# Remove relational attribute

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

        def CheckFail(result, description):
            if verbose:
                if result[0]:
                    print "Unexpected success: %s" % description
                else:
                    print "%40s: [%s]" % (description, '.'.join([str(x) for x in result[1]]))
            self.assertFalse(result[0])

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
        edit.Add('/S', '/T')                    # Edit rel attr on edited target
        edit.Add('/P.j[/T.j].m', '/P.j[/T.j].o')
        CheckFail(edit.Process(hasObject, canEdit, False), "Using an edited target")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D')                    # hasObject fails.
        CheckFail(edit.Process(lambda p: False, canEdit), "hasObject === False")

        edit = Sdf.BatchNamespaceEdit()
        edit.Add('/C', '/D')                    # canEdit fails.
        CheckFail(edit.Process(hasObject, lambda e: (False, "Can't edit")), "canEdit fails")

        print '\nTest live edits on layer'
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

        edit.Add('/P.j[/S.j].n', '/P.k[/P.j].n')    # Reparent relational attribute
        edit.Add('/P.j[/S.j].m', '/P.j[/S.j].o')    # Rename relational attribute
        edit.Add('/P.k[/P.j].n', '/P.k[/P.j].o')    # Rename relational attribute
        edit.Add('/P.j[/S.j].p', Sdf.Path.emptyPath)# Remove relational attribute

        edit.Add('/V{v=one}U', '/V{v=two}W/U')      # Variant prim reparent/rename
        edit.Add('/V{v=two}W', Sdf.Path.emptyPath)  # Variant prim remove
        edit.Add('/V{v=one}.u', '/V{v=two}.u')      # Variant property reparent/rename
        edit.Add('/V{v=two}.w', Sdf.Path.emptyPath) # Variant property remove

        self.assertTrue(layer.CanApply(edit))
        layer.Apply(edit)
        self.assertEqual(layer.ExportToString(), final.ExportToString())

        print '\nTest SUCCEEDED'

if __name__ == "__main__":
    unittest.main()
