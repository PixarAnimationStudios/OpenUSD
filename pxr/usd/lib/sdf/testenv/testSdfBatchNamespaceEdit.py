#!/pxrpythonsubst

from Mentor.Runtime import *

from pxr import Sdf

verbose = False
#verbose = True

# Assert failures end test
SetAssertMode(MTR_EXIT_TEST)

print 'Test constructors'

testEdits = [
    Sdf.NamespaceEdit('/C', '/D'),
    Sdf.NamespaceEdit('/B', '/C'),
    Sdf.NamespaceEdit('/D', '/B'),
    Sdf.NamespaceEdit('/X', '')
]
edit = Sdf.BatchNamespaceEdit()
AssertEqual(edit.edits, [])
edit2 = Sdf.BatchNamespaceEdit(edit)
AssertEqual(edit2.edits, [])
edit = Sdf.BatchNamespaceEdit(testEdits)
AssertEqual(edit.edits, testEdits)
edit2 = Sdf.BatchNamespaceEdit(edit)
AssertEqual(edit2.edits, testEdits)

print '\nTest repr'

edit = Sdf.BatchNamespaceEdit()
AssertEqual(edit.edits, eval(repr(edit)).edits)
AssertEqual(edit2.edits, eval(repr(edit2)).edits)

print '\nTest Add()'

edit.Add(Sdf.NamespaceEdit('/C', '/D'))
edit.Add('/B', '/C')
edit.Add(edit2.edits[-2])
edit.Add('/X', Sdf.Path.emptyPath)
AssertEqual(edit.edits, edit2.edits)

print '\nTest Process()'

layer = Sdf.Layer.FindOrOpen(FindDataFile('testSdfBatchNamespaceEdit.testenv/test.usda'))
final = Sdf.Layer.FindOrOpen(FindDataFile('testSdfBatchNamespaceEdit.testenv/final.usda'))

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

AssertEqual(edit.Process(hasObject, canEdit), (True, edit.edits))

# Try it again.  Since nothing actually changed in layer this should work.
AssertEqual(edit.Process(hasObject, canEdit), (True, edit.edits))

# Try an additional edit.  This one will not be returned by Process() because
# it's redundant.
edit.Add('/E/F', Sdf.Path.emptyPath)# Remove implicitly removed prim
AssertEqual(edit.Process(hasObject, canEdit)[0], True)

#
# Try lots of things that don't work.
#

def CheckFail(result, description):
    if verbose:
        if result[0]:
            print "Unexpected success: %s" % description
        else:
            print "%40s: [%s]" % (description, '.'.join([str(x) for x in result[1]]))
    AssertFalse(result[0])

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

AssertTrue(layer.CanApply(edit))
layer.Apply(edit)
AssertEqual(layer.ExportToString(), final.ExportToString())

print '\nTest SUCCEEDED'
