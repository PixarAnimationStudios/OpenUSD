#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Kind, Sdf, Tf, Usd

_LAYER_CONTENTS = '''#usda 1.0
def Sphere "Ref1" (
    prepend apiSchemas = ["CollectionAPI:Ref1Collection"]
){
    def Scope "Ref1Child" {
    }
}

def Scope "Ref2" {
}

def Scope "Parent1" {

    int attr1 = 1
    int attr2 = 2
    int attr2.connect = </Parent1.attr3>
    int attr3 = 3

    rel rel1 = </Parent1/Child1>

    def Sphere "Child1" {
    }

    def Sphere "Child2" {
    }
}
def "Parent2" (
    references = </Ref2>
){
}
'''

class TestUsdObjectsChangedNotices(unittest.TestCase):
    def setUp(self):
        self._layer = Sdf.Layer.CreateAnonymous('layer.usda')
        self._layer.ImportFromString(_LAYER_CONTENTS)
        self._stage = Usd.Stage.Open(self._layer)
        self.assertTrue(self._stage)
        self._editor = Usd.NamespaceEditor(self._stage)

        self._parent1 = self._stage.GetPrimAtPath('/Parent1')
        self._parent2 = self._stage.GetPrimAtPath('/Parent2')
        self._child1 = self._stage.GetPrimAtPath('/Parent1/Child1')
        self._child2 = self._stage.GetPrimAtPath('/Parent1/Child2')
        self._attr1 = self._parent1.GetAttribute('attr1')
        self._attr2 = self._parent1.GetAttribute('attr2')
        self._attr3 = self._parent1.GetAttribute('attr3')
        self._rel1 = self._parent1.GetRelationship('rel1')

        self.assertTrue(self._parent1)
        self.assertTrue(self._parent2)
        self.assertTrue(self._child1)
        self.assertTrue(self._child2)
        self.assertTrue(self._attr1)
        self.assertTrue(self._attr2)
        self.assertTrue(self._attr3)
        self.assertTrue(self._rel1)

        # Each ObjectsChanged notice is converted to a dictionary and appended
        # to self._notices. The list of notices is reset before each test.
        self._notices = []
        self._noticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged,
            self._onObjectsChanged,
            self._stage)

    def _onObjectsChanged(self, notice, sender):
        asDict = {}
        resynced = notice.GetResyncedPaths()
        changedInfoOnly = notice.GetChangedInfoOnlyPaths()
        resolvedAssetPathsResynced = notice.GetResolvedAssetPathsResyncedPaths()
        if resynced:
            asDict['Resynced'] = {
                str(path): notice.GetChangedFields(path)
                for path in resynced
            }
        if changedInfoOnly:
            asDict['ChangedInfoOnly'] = {
                str(path): notice.GetChangedFields(path)
                for path in changedInfoOnly
            }
        if resolvedAssetPathsResynced:
            asDict['ResolvedAssetPathsResynced'] = {
                str(path): notice.GetChangedFields(path)
                for path in resolvedAssetPathsResynced
            }
        self._notices.append(asDict)

    def test_ObjectSetMetadata(self):
        self._parent1.SetMetadata('comment', 'test')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1': ['comment']
            }
        })

    def test_ObjectSetMetadataByDictKey(self):
        self._parent1.SetMetadataByDictKey('customData', 'a:b', 42)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1': ['customData']
            }
        })

    def test_StageDefinePrim(self):
        self._stage.DefinePrim('/Parent3', 'Sphere')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent3': [
                    'specifier',
                    'typeName',
                ]
            }
        })

    def test_PrimSetSpecifier(self):
        self._parent1.SetSpecifier(Sdf.SpecifierClass)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['specifier']
            }
        })

    def test_PrimSetTypeName(self):
        self._parent1.SetTypeName('Cylinder')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['typeName']
            }
        })

    def test_PrimClearTypeName(self):
        self._parent1.ClearTypeName()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['typeName']
            }
        })

    def test_PrimSetActive(self):
        self._parent1.SetActive(False)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['active']
            }
        })

    def test_PrimSetPropertyOrder(self):
        self._parent1.SetPropertyOrder(['attr2', 'attr1', 'attr3'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                # XXX:USD-10220: Shouldn't this be ['propertyOrder']?
                '/Parent1': []
            }
        })

    def test_PrimRemoveProperty(self):
        self._parent1.RemoveProperty('attr2')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                # Note: attr2 had connections, but the connectionPaths field
                # is omitted.
                '/Parent1.attr2': [] 
            }
        })

    def test_PrimSetKind(self):
        self._parent1.SetKind(Kind.Tokens.model)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['kind']
            }
        })

    def test_PrimAddAppliedSchema(self):
        self._parent1.AddAppliedSchema('CollectionAPI:myCollection')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['apiSchemas']
            }
        })

    def test_PrimSetChildrenReorder(self):
        self._parent1.SetChildrenReorder(['Child2', 'Child1'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['primOrder']
            }
        })

    def test_PrimChangeReferences(self):
        # In this case, the referenced prim has apiSchemas that are added to
        # the referencing prim. Even though the 'apiSchemas' field will change,
        # only the 'references' field is reported to have changed.
        self._parent1.GetReferences().AddReference(
            self._layer.identifier, '/Ref1')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['references']
            }
        })
        self.assertTrue(self._stage.GetPrimAtPath('/Parent1/Ref1Child'))
        self.assertTrue(self._stage.GetRelationshipAtPath(
            '/Parent1.collection:Ref1Collection:includes'))

    def test_PrimChangeReferencesChangesTypeName(self):
        self.assertEqual(self._parent2.GetTypeName(), 'Scope')
        self._parent2.GetReferences().SetReferences([
            Sdf.Reference(self._layer.identifier, '/Ref1')
        ])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent2': ['references']
            }
        })
        self.assertEqual(self._parent2.GetTypeName(), 'Sphere')

    def test_PropertySetDisplayGroup(self):
        self._attr1.SetDisplayGroup('test')
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['displayGroup']
            }
        })

    def test_PropertySetNestedDisplayGroups(self):
        self._attr1.SetNestedDisplayGroups(['test', 'test2'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['displayGroup']
            }
        })

    def test_PropertySetCustom(self):
        self._attr1.SetCustom(True)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['custom']
            }
        })

    def test_AttributeSetVariability(self):
        self._attr1.SetVariability(Sdf.VariabilityUniform)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['variability']
            }
        })

    def test_AttributeSetTypename(self):
        self._attr1.SetTypeName(Sdf.ValueTypeNames.Double)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['typeName']
            }
        })

    def test_AttributeSet(self):
        self._attr1.Set(42)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['default']
            }
        })

    def test_AttributeAddConnection(self):
        self._attr1.AddConnection(self._attr2.GetPath())
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr1': ['connectionPaths']
            }
        })

    def test_AttributeClearConnections(self):
        self._attr2.ClearConnections()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.attr2': ['connectionPaths']
            }
        })

    def test_RelationshipAddTarget(self):
        self._rel1.AddTarget(self._parent2.GetPath())
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })
    
    def test_RelationshipRemoveTarget(self):
        self._rel1.RemoveTarget(self._child1.GetPath())
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })

        # Removing target that does not exist should not trigger a notice.
        self._rel1.RemoveTarget(self._child1.GetPath())
        self.assertEqual(len(self._notices), 1)

    def test_RelationshipSetTargets(self):
        self._rel1.SetTargets([self._child1.GetPath(), self._child2.GetPath()])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })

    def test_RelationshipClearTargets(self):
        self._rel1.ClearTargets(removeSpec = False)
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })
        
        self._rel1.ClearTargets(removeSpec = True)
        self.assertEqual(len(self._notices), 2)
        self.assertDictEqual(self._notices[1], {
            'Resynced': {
                '/Parent1.rel1': []
            }
        })

    def test_NamespaceEditorDeletePrimAtPath(self):
        self._editor.DeletePrimAtPath('/Parent1')
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': []
            }
        })

    def test_NamespaceEditorMovePrimAtPath(self):
        self._editor.MovePrimAtPath('/Parent2', '/Parent1/Parent3')
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent2': [],
                '/Parent1/Parent3': []
            }
        })

    def test_NamespaceEditorRenamePrim(self):
        self._editor.RenamePrim(self._child1, 'Child3')
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1/Child1': [],
                '/Parent1/Child3': []
            },
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })

    def test_NamespaceEditorReparentPrim(self):
        self._editor.ReparentPrim(self._child1, self._parent2)
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1/Child1': [],
                '/Parent2/Child1': []
            },
            'ChangedInfoOnly': {
                '/Parent1.rel1': ['targetPaths']
            }
        })

    def test_NamespaceEditorDeleteProperty(self):
        self._editor.DeleteProperty(self._attr3)
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1.attr3': []
            },
            'ChangedInfoOnly': {
                '/Parent1.attr2': ['connectionPaths']
            }
        })

    def test_NamespaceEditorMovePropertyAtPath(self):
        self._editor.MovePropertyAtPath(
            self._attr3.GetPath(),
            '/Parent2.newName')
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1.attr3': [],
                '/Parent2.newName': []
            },
            'ChangedInfoOnly': {
                '/Parent1.attr2': ['connectionPaths']
            }
        })

    def test_NamespaceEditorReparentProperty(self):
        self._editor.ReparentProperty(self._attr3, self._parent2)
        self._editor.ApplyEdits()
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1.attr3': [],
                '/Parent2.attr3': []
            },
            'ChangedInfoOnly': {
                '/Parent1.attr2': ['connectionPaths']
            }
        })

    def test_SdfChangeBlockChangeTypeThenDelete(self):
        with Sdf.ChangeBlock():
            parentSpec = self._layer.GetPrimAtPath('/Parent1')
            parentSpec.typeName = 'Cylinder'
            del(self._layer.rootPrims['Parent1'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['typeName']
            }
        })
        self.assertFalse(self._stage.GetPrimAtPath('/Parent1'))

    def test_SdfChangeBlockAppendAPISchemaThenDelete(self):
        with Sdf.ChangeBlock():
            parentSpec = self._layer.GetPrimAtPath('/Parent1')
            schemas = Sdf.TokenListOp.Create(['CollectionAPI:myCollection'])
            parentSpec.SetInfo('apiSchemas', schemas)
            del(self._layer.rootPrims['Parent1'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['apiSchemas']
            }
        })
        self.assertFalse(self._stage.GetPrimAtPath('/Parent1'))

    def test_SdfChangeBlockReorderChildrenThenDelete(self):
        with Sdf.ChangeBlock():
            parentSpec = self._layer.GetPrimAtPath('/Parent1')
            parentSpec.nameChildrenOrder = ['Child2', 'Child1']
            del(self._layer.rootPrims['Parent1'])
        self.assertEqual(len(self._notices), 1)
        self.assertDictEqual(self._notices[0], {
            'Resynced': {
                '/Parent1': ['primOrder']
            }
        })
        self.assertFalse(self._stage.GetPrimAtPath('/Parent1'))


if __name__ == "__main__":
    unittest.main()