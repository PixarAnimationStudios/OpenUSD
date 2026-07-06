#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf, Usd, Tf
from testUsdNamespaceEditorDependentEditsBase \
      import TestUsdNamespaceEditorDependentEditsBase

class TestUsdNamespaceEditorDependentEditsTargetPathFixup(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency target-path fixup when a prim or
    property is renamed, reparented, or deleted on the edited stage.'''

    # Verifies the expected layer contents of a layer in relation to the 
    # prim and property hierarchies and the PathListOp field values of the
    # properties' connectionPaths or target fields (for attributes and 
    # relationships respectively).
    #
    # The format of the expected contents is a nested dictionary 
    # representing the full prim hierarchy as demonstrated in the example:
    #
    # {
    #     "/Root" : {
    #         ".rootAttr" : Sdf.PathListOp(),
    #         "/A" : {
    #             ".fooAttr" : Sdf.PathListOp.CreateExplicit([])
    #         },
    #         "/B" : {
    #             ".barAttr" : Sdf.PathListOp.CreateExplicit([
    #                 "/Root/A.fooAttr"]),
    #             ".bazRel" : Sdf.PathListOp.Create(
    #                 deletedItems = ["/Root/A"],
    #                 prependedItems = ["/Root/B"])
    #         }
    #     }
    # }
    #
    # These expected contents indicate that a layer is expected to have a root 
    # prim at path /Root.
    # The prim /Root is expected to have prim children (indicated by 
    # preceding '/') "A" and "B" (paths /Root/A and /Root/B respectively).
    # /Root is also expected to have a property child (indicated by preceding 
    # '.') rootAttr (path /Root.rootAttr)
    # /Root/A must have just the property /Root/A.fooAttr
    # /Root/B must have just the properties /Root/B.barAttr and /Root/B.bazRel
    # Each propery must have its connectionPaths or targets field (depending 
    # on the property spec's actual type) return the matching Sdf.PathListOp
    # when the field is queried from the layer.
    #
    def _VerifyLayerContents(self, layer, expectedContentsDict):
        # Verify the expected contents dictionary is a valid format first for
        # aid in debugging test failures.
        self._VerifyExpectedLayerContentsFormat(expectedContentsDict)

        # Verify the expected number of root prims
        self.assertEqual(len(expectedContentsDict), len(layer.rootPrims),
            "The expected number of root prims in {} doesn't match the "
            "number of root prims {} on layer {}".format(
                list(expectedContentsDict.keys()),
                list(layer.rootPrims.keys()),
                layer.identifier))
        
        # Verify the expected contents of eache root prim.
        for rootPrimPath, expectedChild in expectedContentsDict.items():
            self._VerifyPrimContents(layer, rootPrimPath, expectedChild)

    # Validates expectedContents has the correct shape so test writing bugs
    # show up as setup errors rather than assertion failures.
    def _VerifyExpectedLayerContentsFormat(self, expectedContents,
                                           pathKey=None):

        self.assertTrue(isinstance(expectedContents, dict),
            "Expected contents for prim key '{}' is not a dictionary"
            .format(pathKey or "None"))
        for k, v in expectedContents.items():
            if k.startswith('.'):
                self.assertTrue(isinstance(v, Sdf.PathListOp),
                    "Value '{}' for property key '{}' is invalid; it must "
                    "be an Sdf.PathListOp".format(str(v), k))
            elif k.startswith('/'):
                self._VerifyExpectedLayerContentsFormat(v, k)
            else:
                self.assertTrue(False,
                    "Invalid expected contents dictionary key '{}'; it "
                    "must start with '/' for prim children or '.' for "
                    "property children".format(k))

    # Verifies the contents of a prim in the layer has the expected contents
    # indicated by the expectedContentsDict
    def _VerifyPrimContents(self, layer, path, expectedContentsDict):
        # Prim must exist
        prim = layer.GetPrimAtPath(path)
        self.assertTrue(prim,
            "Expected to find prim at path {} in layer {}".format(
                path, layer.identifier))

        # Prim must have the same number of prim and property children as 
        # indicated in the expected contents.
        self.assertEqual(
            len(expectedContentsDict),
            len(prim.properties) + len(prim.nameChildren),
            "The expected number of prims and properties in {} doesn't "
            "match the combined number of child properties {} and prims "
            "{} on the prim at {} in layer {}".format(
                list(expectedContentsDict.keys()),
                list(prim.properties.keys()),
                list(prim.nameChildren.keys()),
                path, layer.identifier))

        # Verify each expected child.
        for childName, expectedChildContent in expectedContentsDict.items():
            # Create the full path of the child object which may be a prim or
            # a property
            childPath = Sdf.Path(str(path) + childName)

            # Verify the contents match for the appropriate child type.
            if childPath.IsPrimPropertyPath():
                self._VerifyPropertyContents(
                    layer, childPath, expectedChildContent)
            else:
                self._VerifyPrimContents(
                    layer, childPath, expectedChildContent)

    # Verifies the contents of a property in the layer has the expected 
    # connections or relationship targets listOp value.
    def _VerifyPropertyContents(self, layer, path, expectedListOpValue):
        # Property must exist
        prop = layer.GetPropertyAtPath(path)
        self.assertTrue(prop,
            "Expected to find property at path {} in layer {}".format(
                path, layer.identifier))

        if isinstance(prop, Sdf.AttributeSpec):
            # Is attribute, connectionPaths field must match listOp
            listOpValue = prop.GetInfo(Sdf.AttributeSpec.ConnectionPathsKey)
            self.assertEqual(listOpValue, expectedListOpValue,
                "Attribute at {} has connectionPaths value '{}' which "
                "does not match the expected value '{}'".format(
                    path, listOpValue, expectedListOpValue))
        elif isinstance(prop, Sdf.RelationshipSpec):
            # Is relationship, targets field must match listOp
            listOpValue = prop.GetInfo(Sdf.RelationshipSpec.TargetsKey)
            self.assertEqual(listOpValue, expectedListOpValue,
                "Relationship at {} has targets value '{}' which does "
                "not match the expected value '{}'".format(
                    path, listOpValue, expectedListOpValue))
        else:
            # Invalid property type, always fail.
            self.fail("Property at {} is not a valid property type".format(path))
            
    # Calls CanApplyEdits and ApplyEdits on the given editor and verifies both
    # succeed. If expectedObjectsChangedRenamedProperties is provided, this also 
    # verifies that listening to the ObjectsChanged notice will send a notice 
    # holding the expected renamed properties specified.
    def _ApplyEditWithVerification(self, editor, 
            expectedObjectsChangedRenamedProperties = None, expectedWarnings=[]):
        # receivedObjectsChanged is used for sanity checking that the notice
        # handler was indeed called as expected.
        receivedObjectsChanged = False
        def _OnObjectsChangedVerifyRenamedPropertiesNotices(notice, sender):
            nonlocal receivedObjectsChanged
            receivedObjectsChanged = True

            # Compare the notice's renamed properties with the expected
            # properties or verify that renamed properties is empty if we don't
            # expect renamed properties
            if expectedObjectsChangedRenamedProperties is None:
                self.assertEqual(notice.GetRenamedProperties(), [])
            else:
                stageName = notice.GetStage().GetRootLayer().GetDisplayName()
                self.assertEqual(notice.GetRenamedProperties(),
                                 expectedObjectsChangedRenamedProperties[stageName])
            self._OnObjectsChanged(notice, sender)

        # Register the ObjectsChange listener; we revoke it after applying the
        # edits
        self.resyncedObjectsPerStage = {}
        propsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, 
            _OnObjectsChangedVerifyRenamedPropertiesNotices)
        primsChanged = Tf.Notice.RegisterGlobally(
            Usd.Notice.ObjectsChanged, self._OnObjectsChanged)

        try:
            # Verify CanApply and Apply
            self.assertTrue(editor.CanApplyEdits())
            self.assertEqual(len(editor.CanApplyEdits().warnings), 
                             len(expectedWarnings))
            for warn, expectedWarn in zip(editor.CanApplyEdits().warnings, 
                                          expectedWarnings):
                self.assertTrue(expectedWarn in warn)
            self.assertTrue(editor.ApplyEdits())
            # Sanity check on the notice listener being called.
            self.assertTrue(receivedObjectsChanged)
            
        finally:
            propsChanged.Revoke()
            primsChanged.Revoke()

    def test_BasicReferences(self):
        '''Tests target-path fixup after downstream dependency namespace edits 
        across a reference.'''

        # Layer 1 defines /Ref/Child (prim to edit) and /Ref/Child.attr 
        # (property to edit), /Ref/RefSibling (reparent destination), and 
        # /Local with local rel and connection references to /Ref/Child. It also
        # has an internal ref to /Ref, which /Local targets as well.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }
                                
            def "RefSibling" {
             }

            def "Local" {
                custom rel rel 
                append rel rel = [</Ref/Child>, </Ref/Child.attr>]
                prepend rel rel = [</InternalRef/Child>, </InternalRef/Child.attr>]
                                
                custom int conn
                append int conn.connect = </Ref/Child.attr>
                prepend int conn.connect = </InternalRef/Child.attr>
            }
                                
            def "InternalRef"  (
                references = </Ref>
            ) {
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 references /Ref from /Prim and defines /Other
        # with rel and connection references pointing at /Prim/Child.attr.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Prim" (
                references = @''' + layer1.identifier + '''@</Ref>
            ) {
            }

            def "Other" {
                custom rel rel = [</Prim/Child>, </Prim/Child.attr>]
                custom int conn
                int conn.connect = </Prim/Child.attr>
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as a dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child', '/InternalRef/Child.attr'],
                    appendedItems = ['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child.attr'],
                    appendedItems = ['/Ref/Child.attr']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })
        
        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor, '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))


        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child', '/InternalRef/Child.renamed'],
                    appendedItems = ['/Ref/Child', '/Ref/Child.renamed']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child.renamed'],
                    appendedItems = ['/Ref/Child.renamed']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.renamed']),
            },
        })
        
        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))


        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Renamed', '/InternalRef/Renamed.renamed'],
                    appendedItems = ['/Ref/Renamed', '/Ref/Renamed.renamed']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Renamed.renamed'],
                    appendedItems = ['/Ref/Renamed.renamed']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed', '/Prim/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed.renamed']),
            },
        })
        
        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        # Target paths containing /Ref/Renamed.renamed directly can be 
        # updated to the new path. However, those targeting it across a 
        # reference (internal or not) no longer get the new .moved attribute
        # composed in, so target paths containing /InternalRef/Renamed.renamed
        # (layer 1) or /Prim/Renamed.renamed (layer 2) have that path removed 
        # instead.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {},
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp()
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Renamed'],
                    appendedItems = ['/Ref/Renamed', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = ['/RefSibling.moved']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        
        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        # For the same reason as above, target paths containing 
        # /InternalRef/Renamed (layer 1) or /Prim/Renamed (layer 2) have that 
        # path removed, while target paths containing /Ref/Renamed are 
        # updated to /RefSibling/Moved.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp(),
                '/Moved': {},
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = ['/RefSibling/Moved', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = ['/RefSibling.moved']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        
        # Reparent: /RefSibling/Moved -> /Ref/Child
        with self.ApplyEdits(editor,
                "Reparent /RefSibling/Moved -> /Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/RefSibling/Moved', '/Ref/Child'))

        
        # Reparent: /RefSibling.moved -> /Ref/Child.attr
        with self.ApplyEdits(editor,
                "Reparent /RefSibling.moved -> /Ref/Child.attr"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/RefSibling.moved', '/Ref/Child.attr'))

        # The prim/property structure returns to its original state with one 
        # notable exception: the listops that don't directly target /Ref/Child 
        # are NOT restored from being emptied.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = ['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = ['/Ref/Child.attr']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        
        # Reinitialize to reset the list ops.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)
        
        # Verify the initial layer contents have been restored.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child', '/InternalRef/Child.attr'],
                    appendedItems = ['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child.attr'],
                    appendedItems = ['/Ref/Child.attr']),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })
        
        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor,
                "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))
                
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/InternalRef/Child'],
                    appendedItems = ['/Ref/Child']),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = []),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        
        # Delete: /Ref/Child
        with self.ApplyEdits(editor,
                "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath(
                '/Ref/Child'))
                
        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = []),
                '.conn': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = []),
            },
            '/InternalRef': {
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit([]),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

    def test_BasicPayloads(self):
        '''Test target path fixup after downstream dependency namespace edits 
        across a payload. This also verifies that target path fixup on a 
        dependent stage is skipped when the payload is unloaded.
        '''

        # Layer 1 defines /Ref/Child (prim to edit) and /Ref/Child.attr
        # (property to edit), /RefSibling (reparent destination), and
        # /Local with local rel and connection references to /Ref/Child.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }

            def "RefSibling" {
             }

            def "Local" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]

                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 payloads /Ref from /Prim and defines /Other
        # with rel and connection references pointing at /Prim/Child*.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Prim" (
                payload = @''' + layer1.identifier + '''@</Ref>
            ) {
            }

            def "Other" {
                custom rel rel = [</Prim/Child>, </Prim/Child.attr>]
                custom int conn
                int conn.connect = </Prim/Child.attr>
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an additional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Child.attr']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor, '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))


        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Child', '/Ref/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Child.renamed']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.renamed']),
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))


        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Renamed', '/Ref/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Renamed.renamed']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed', '/Prim/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed.renamed']),
            },
        })

        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        # Target path targeting /Ref/Renamed.renamed directly can be 
        # updated to the new path. However, those targeting it across a 
        # payload no longer get the new .moved attribute composed in, so target  
        # paths containing /Prim/Renamed.renamed (layer 2) have that path 
        # removed instead.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {},
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp()
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Renamed', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        # For the same reason as above, target paths containing 
        # /Prim/Renamed (layer 2) have that path removed, while target paths 
        # containing /Ref/Renamed are updated to /RefSibling/Moved.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp(),
                '/Moved': {},
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/RefSibling/Moved', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Reinitialize to reset the list ops for the next test case.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Verify the layers have been returned to their initial state.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Child.attr']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor,
                "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Delete: /Ref/Child
        with self.ApplyEdits(editor,
                "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath(
                '/Ref/Child'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit([]),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Reset both layers. 
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Unload /Prim on stage 2 so the payload contents are not composed into 
        # the dependent stage's namespace. When /Prim is unloaded on stage 2,
        # /Prim/Child does not compose into the stage's namespace, so
        # the dependent-stage target-path fixup cannot reach the
        # /Other.rel and /Other.conn entries that point at /Prim/Child*
        # paths. Layer 1's own rel and connection on /Local are still
        # fixed up because the rename happens on stage 1.
        stage2.Unload('/Prim')

        # Rename: /Ref/Child -> /Ref/Renamed (with /Prim unloaded)
        with self.ApplyEdits(editor,
                "Rename /Ref/Child -> /Ref/Renamed (/Prim unloaded)"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Layer 1 is fixed up as usual since the rename is on stage 1.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Renamed', '/Ref/Renamed.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Renamed.attr']),
            },
        })
        # Layer 2 targets are NOT fixed up because /Prim is unloaded on
        # stage 2 and /Prim/Child does not compose to anything.
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })

        # Reload /Prim and observe the broken state: /Other.rel and
        # /Other.conn still point at /Prim/Child paths even though the
        # payload now composes /Prim/Renamed.
        stage2.Load('/Prim')
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })

    def test_BasicSublayers(self):
        '''Tests path expression fixup after downstream dependency namespace 
        edits across sublayers.

        This test sets up a four stage hierarchy and registers ONLY stage3 
        as a dependent of the stage 1 editor to show that:

          - layer1 is fixed up directly because it's the edit target.
          - layer2 and layer3Sub are fixed up because they sit in
            stage3's layer stack (transitively or directly).
          - layer3 is fixed up because stage3 is registered as a dependent stage.
          - layer4 is NOT fixed up: stage4 was never registered as a
            dependent stage even though layer4 sublayers layer3.
        '''

        # Layer 1 is the edited base layer. It defines /Ref/Child (prim to
        # edit), /Ref/Child.attr (property to edit), /RefSibling
        # (reparent destination), and /Local with rel and connection
        # references targeting /Ref/Child.attr.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }

            def "RefSibling" {
            }

            def "Local" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]

                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 sublayers layer1. It defines /Other2 with rel and
        # connection references targeting /Ref/Child.attr.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            (
                subLayers = [@''' + layer1.identifier + '''@]
            )

            def "Other2" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]
                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Layer 3 Sub will be a sublayer of layer3. It defines /Other3Sub
        # with rel and connection references targeting /Ref/Child.attr.
        layer3Sub = Sdf.Layer.CreateAnonymous("layer3-sub.usda")
        layer3SubImportString = ('''#usda 1.0
            def "Other3Sub" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]
                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer3Sub.ImportFromString(layer3SubImportString)

        # Layer 3 sublayers layer2 (which sublayers layer1) and
        # layer3Sub. It defines /Other3 with rel and connection references
        # targeting /Ref/Child.attr.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3ImportString = ('''#usda 1.0
            (
                subLayers = [
                    @''' + layer2.identifier + '''@,
                    @''' + layer3Sub.identifier + '''@
                ]
            )

            def "Other3" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]
                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer3.ImportFromString(layer3ImportString)

        # Layer 4 sublayers layer3. It defines /Other4 with rel and
        # connection references targeting /Ref/Child.attr. Stage 4 is
        # intentionally not registered as a dependent, so layer 4 stays
        # untouched throughout the test.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4ImportString = ('''#usda 1.0
            (
                subLayers = [@''' + layer3.identifier + '''@]
            )

            def "Other4" {
                custom rel rel = [</Ref/Child>, </Ref/Child.attr>]
                custom int conn
                int conn.connect = </Ref/Child.attr>
            }
        ''')
        layer4.ImportFromString(layer4ImportString)

        # Open all four layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)
        stage3 = Usd.Stage.Open(layer3, Usd.Stage.LoadAll)
        stage4 = Usd.Stage.Open(layer4, Usd.Stage.LoadAll)

        # Create an editor for stage1 and add ONLY stage3 as a
        # dependent stage. Stage 2 and stage 4 are intentionally left
        # unregistered.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage3)

        # Every layer in stage3's layer stack other than layer1
        # holds its /OtherN prim with the same rel and conn listops, so
        # they all get verified together against the same expected
        # values at each step of the test.
        def _VerifyOtherLayerListOps(expectedRel, expectedConn):
            self._VerifyLayerContents(layer2, {
                '/Other2': {
                    '.rel': expectedRel,
                    '.conn': expectedConn,
                },
            })
            self._VerifyLayerContents(layer3Sub, {
                '/Other3Sub': {
                    '.rel': expectedRel,
                    '.conn': expectedConn,
                },
            })
            self._VerifyLayerContents(layer3, {
                '/Other3': {
                    '.rel': expectedRel,
                    '.conn': expectedConn,
                },
            })

        # Verify layer4 keeps its original target paths --
        # stage4 was not registered as a dependent stage so /Other4 is
        # never updated regardless of what edits are applied via
        # stage1.
        def _VerifyLayer4Unchanged():
            self._VerifyLayerContents(layer4, {
                '/Other4': {
                    '.rel': Sdf.PathListOp.CreateExplicit(
                        ['/Ref/Child', '/Ref/Child.attr']),
                    '.conn': Sdf.PathListOp.CreateExplicit(
                        ['/Ref/Child.attr']),
                },
            })

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Child', '/Ref/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Ref/Child.attr']),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(['/Ref/Child', '/Ref/Child.attr']),
            Sdf.PathListOp.CreateExplicit(['/Ref/Child.attr']))
        _VerifyLayer4Unchanged()

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor, '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Child', '/Ref/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Child.renamed']),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(
                ['/Ref/Child', '/Ref/Child.renamed']),
            Sdf.PathListOp.CreateExplicit(['/Ref/Child.renamed']))
        _VerifyLayer4Unchanged()

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Renamed', '/Ref/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Renamed.renamed']),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(
                ['/Ref/Renamed', '/Ref/Renamed.renamed']),
            Sdf.PathListOp.CreateExplicit(['/Ref/Renamed.renamed']))
        _VerifyLayer4Unchanged()

        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {},
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp()
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Ref/Renamed', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(
                ['/Ref/Renamed', '/RefSibling.moved']),
            Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']))
        _VerifyLayer4Unchanged()

        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
                '.moved': Sdf.PathListOp(),
                '/Moved': {},
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/RefSibling/Moved', '/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(
                ['/RefSibling/Moved', '/RefSibling.moved']),
            Sdf.PathListOp.CreateExplicit(['/RefSibling.moved']))
        _VerifyLayer4Unchanged()

        # Reinitialize all layers so the deletes start from the
        # original list ops without the previous reparent edits.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)
        layer3Sub.ImportFromString(layer3SubImportString)
        layer3.ImportFromString(layer3ImportString)
        layer4.ImportFromString(layer4ImportString)

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor,
                "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Ref/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(['/Ref/Child']),
            Sdf.PathListOp.CreateExplicit())
        _VerifyLayer4Unchanged()

        # Delete: /Ref/Child
        with self.ApplyEdits(editor,
                "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath(
                '/Ref/Child'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
            },
            '/RefSibling': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        _VerifyOtherLayerListOps(
            Sdf.PathListOp.CreateExplicit(),
            Sdf.PathListOp.CreateExplicit())
        _VerifyLayer4Unchanged()

    def _RunTestBasicGlobalClassArcs(self, classArcType):
        """Helper for testing downstream dependency namespace edits and 
        subsequent target path fixup across global class arcs and their 
        implied class specs. classArcType can be either 'inherits' or 
        'specializes'"""

        # Layer 1 defines /Class/Child (prim to edit) and /Class/Child.attr
        # (property to edit), /ClassSibling (reparent destination), /Prim
        # which inherits or specializes /Class (so the global class arc
        # propagates as an implied class arc to any layer that references
        # /Prim), and /Local with rel and connection references targeting
        # /Prim/Child.attr (the path translated through /Prim's
        # inherits/specializes arc).
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Class" {
                def "Child" {
                    custom int attr
                }
            }

            def "ClassSibling" {
            }

            def "Prim" (
                ''' + classArcType + ''' = </Class>
            ) {
            }

            def "Local" {
                custom rel rel = [</Prim/Child>, </Prim/Child.attr>]

                custom int conn
                int conn.connect = </Prim/Child.attr>
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 references /Prim from layer1 as /Prim2 so the global
        # class arc on /Prim propagates as an implied class arc to /Class
        # in layer2's namespace. /Other has rel and connection references
        # targeting /Prim2/Child.attr (the path translated through the
        # reference plus inherited/specialized class arc).
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Prim2" (
                references = @''' + layer1.identifier + '''@</Prim>
            ) {
            }

            over "Class" {
                over "Child" {
                    custom int attr
                }
            }

            def "Other" {
                custom rel rel 
                append rel rel = [</Prim2/Child>, </Prim2/Child.attr>]
                prepend rel rel = [</Class/Child>, </Class/Child.attr>]
                custom int conn
                int conn.connect = [</Prim2/Child.attr>, </Class/Child.attr>]
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an additional
        # dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/ClassSibling': {
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(['/Prim/Child.attr']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Class/Child', '/Class/Child.attr'],
                    appendedItems = ['/Prim2/Child', '/Prim2/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim2/Child.attr', '/Class/Child.attr']),
            },
        })

        # Rename: /Class/Child.attr -> /Class/Child.renamed
        with self.ApplyEdits(editor,
                '/Class/Child.attr -> /Class/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Class/Child.attr', '/Class/Child.renamed'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
            '/ClassSibling': {
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child', '/Prim/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child.renamed']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Class/Child', '/Class/Child.renamed'],
                    appendedItems = ['/Prim2/Child', '/Prim2/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim2/Child.renamed', '/Class/Child.renamed']),
            },
        })

        # Rename: /Class/Child -> /Class/Renamed
        with self.ApplyEdits(editor, '/Class/Child -> /Class/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/Child', '/Class/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
            '/ClassSibling': {
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Renamed', '/Prim/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Renamed.renamed']),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Class/Renamed', '/Class/Renamed.renamed'],
                    appendedItems = ['/Prim2/Renamed', '/Prim2/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim2/Renamed.renamed', '/Class/Renamed.renamed']),
            },
        })

        # Reparent: /Class/Renamed.renamed -> /ClassSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Class/Renamed.renamed -> /ClassSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Class/Renamed.renamed', '/ClassSibling.moved'))

        # /ClassSibling is not inherited or specialized by /Prim, so the
        # property is no longer reachable through /Prim's class arc and
        # /Prim/Renamed.renamed drops out of /Local's listops. /Prim/Renamed
        # is still reachable via the class arc since /Class/Renamed
        # still exists with its property removed.
        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Renamed': {},
            },
            '/ClassSibling': {
                '.moved': Sdf.PathListOp()
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Renamed': {},
            },
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Class/Renamed'],
                    appendedItems = ['/Prim2/Renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Reparent: /Class/Renamed -> /ClassSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Class/Renamed -> /ClassSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/Renamed', '/ClassSibling/Moved'))

        # The prim itself is moved out from under /Class so /Prim/Renamed is
        # no longer reachable via the class arc and drops out of /Local's
        # listops as well.
        self._VerifyLayerContents(layer1, {
            '/Class': {
            },
            '/ClassSibling': {
                '.moved': Sdf.PathListOp(),
                '/Moved': {},
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = []),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Reinitialize to reset the list ops for the next test case.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Delete: /Class/Child.attr
        with self.ApplyEdits(editor,
                "Delete /Class/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Class/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Child': {},
            },
            '/ClassSibling': {
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(['/Prim/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {},
            },
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Class/Child'],
                    appendedItems = ['/Prim2/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

        # Delete: /Class/Child
        with self.ApplyEdits(editor,
                "Delete /Class/Child"):
            self.assertTrue(editor.DeletePrimAtPath(
                '/Class/Child'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
            },
            '/ClassSibling': {
            },
            '/Prim': {
            },
            '/Local': {
                '.rel': Sdf.PathListOp.CreateExplicit(),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = [],
                    appendedItems = []),
                '.conn': Sdf.PathListOp.CreateExplicit(),
            },
        })

    def test_BasicInherits(self):
        self._RunTestBasicGlobalClassArcs("inherits")

    def test_BasicSpecializes(self):
        self._RunTestBasicGlobalClassArcs("specializes")

    def test_BasicVariants(self):
        '''Tests target path fixup from downstream dependency namespace edits 
        across a reference contained within a variant both when the variant is 
        selected and when it is not.
        '''

        # Layer 1 defines /Ref/Child (prim to edit) and /Ref/Child.attr
        # (property to edit).
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 defines /Variant with a variant set "v" whose "selected"
        # variant references /Ref from layer1, and /Prim which references
        # /Variant. The session layer below selects v="selected" on /Prim only
        # (not on /Variant), so on stage2:
        #   - /Prim picks up variantSet "v" via the reference, has the
        #     selection from the session layer, composes the variant body, and
        #     therefore composes /Ref via the reference inside the variant.
        #     /Prim/Child exists on stage2 and depends on /Ref/Child in layer1.
        #   - /Variant has variantSet "v" but no selection, so the variant body
        #     never activates for /Variant itself. /Variant/Child does NOT
        #     compose on stage2 and has no dependency on anything in layer1.
        # /Other's rel and conn list both target /Prim/Child* and /Variant/Child*
        # paths; only the /Prim/Child* paths have composed dependencies and so
        # only those get fixed up when /Ref/Child or /Ref/Child.attr are edited
        # on stage1.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Variant" (
                variantSets = ["v"]
            ) {
                variantSet "v" = {
                    "selected" (
                        references = @''' + layer1.identifier + '''@</Ref>
                    ) {
                    }
                }
            }

            def "Prim" (
                references = </Variant>
            ) {
            }

            def "Other" {
                custom rel rel 
                append rel rel = [</Prim/Child>, </Prim/Child.attr>]
                prepend rel rel = [</Variant/Child>, </Variant/Child.attr>]
                custom int conn
                int conn.connect = [</Prim/Child.attr>, </Variant/Child.attr>]
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Session layer authors the variant selection on /Prim so that the
        # reference inside the variant composes when stage2 is opened.
        sessionLayer = Sdf.Layer.CreateAnonymous("session.usda")
        sessionLayer.ImportFromString('''#usda 1.0
            over "Prim" (
                variants = {
                    string v = "selected"
                }
            ) {
            }
        ''')

        # Open both layers as stages. Stage 2 is opened with the session layer
        # to provide the variant selection.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, sessionLayer)

        # Create an editor for stage 1 with stage 2 as an additional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child.attr', '/Variant/Child.attr']),
            },
        })

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        # Only /Prim/Child.attr is fixed up. /Variant/Child.attr has no
        # composed dependency because the variant selection is only authored
        # on /Prim, and not on /Variant and therefore is left alone.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Child', '/Prim/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child.renamed', '/Variant/Child.attr']),
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Only /Prim/Child is fixed up for the same reason as above.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Renamed', '/Prim/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Renamed.renamed', '/Variant/Child.attr']),
            },
        })

        # Reinitialize to reset the list ops for the next test case.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor, "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath('/Ref/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Variant/Child.attr']),
            },
        })

        # Delete: /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = []),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Variant/Child.attr']),
            },
        })

        # Reinitialize layers and mute the session layer so /Prim has no
        # variant selection on stage2. Now, /Prim/Child no longer exists on 
        # stage2, so any paths referring to it or its children will not be 
        # fixed up by the namespace editor.
        with Sdf.ChangeBlock():
            layer1.ImportFromString(layer1ImportString)
            layer2.ImportFromString(layer2ImportString)
        stage2.MuteLayer(sessionLayer.identifier)

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor,
                'Muted: /Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        # Now that there is no variant selection, layer1 is updated, but all the
        # target paths on /Other are unchanged.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child.attr', '/Variant/Child.attr']),
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        # layer1 is updated; layer2 /Other rel/conn remain unchanged.
        with self.ApplyEdits(editor, 'Muted: /Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Now that there is no variant selection, layer1 is updated, but all the
        # target paths on /Other are unchanged.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Variant/Child', '/Variant/Child.attr'],
                    appendedItems = ['/Prim/Child', '/Prim/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Child.attr', '/Variant/Child.attr']),
            },
        })

    def test_BasicRelocates(self):
        '''Tests target path fixup after downstream dependency namespace edits 
        across a reference where a child of the referencing prim is then 
        relocated.'''

        # Layer 1 defines /World/Ref/Child (the prim to edit) and
        # /World/Ref/Child.attr (the property to edit). /World/RefSibling
        # is the reparent destination outside the /Ref subtree.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "World" {
                def "Ref" {
                    def "Child" {
                        custom int attr
                    }
                }
                def "RefSibling" {
                }
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 defines /Prim which references /World, plus a relocates 
        # that moves /Prim/Ref to /Relocated. 
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            (
                relocates = {
                    </Prim/Ref> : </Relocated>,
                }
            )

            def "Prim" (
                references = @''' + layer1.identifier + '''@</World>
            ) {
            }

            def "Relocated" {
            }

            def "Other" {
                custom rel rel
                append rel rel = [</Relocated/Child>, </Relocated/Child.attr>]
                prepend rel rel = [</Prim/Ref/Child>, </Prim/Ref/Child.attr>]
                custom int conn
                int conn.connect = [</Relocated/Child.attr>, </Prim/Ref/Child.attr>]
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Open both layers as stages.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, Usd.Stage.LoadAll)

        # Create an editor for stage 1 with stage 2 as an additional dependent
        # stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {'.attr': Sdf.PathListOp()},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Child', '/Relocated/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Relocated/Child.attr', '/Prim/Ref/Child.attr']),
            },
        })

        # Rename: /World/Ref/Child.attr -> /World/Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/World/Ref/Child.attr -> /World/Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/World/Ref/Child.attr', '/World/Ref/Child.renamed'))

        # On stage2 the relocated property /Relocated/Child.attr renames to
        # /Relocated/Child.renamed; the appended rel entry and the first
        # conn entry are fixed up. /Prim/Ref/Child.attr in /Other has no
        # composed dependency on /World/Ref/Child.attr and stays as-is.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {'.renamed': Sdf.PathListOp()},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Child', '/Relocated/Child.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Relocated/Child.renamed', '/Prim/Ref/Child.attr']),
            },
        })

        # Rename: /World/Ref/Child -> /World/Ref/Renamed
        with self.ApplyEdits(editor,
                '/World/Ref/Child -> /World/Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref/Child', '/World/Ref/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Renamed': {'.renamed': Sdf.PathListOp()},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Renamed', '/Relocated/Renamed.renamed']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Relocated/Renamed.renamed', '/Prim/Ref/Child.attr']),
            },
        })

        # Reparent: /World/Ref/Renamed.renamed -> /World/RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /World/Ref/Renamed.renamed -> "
                "/World/RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/World/Ref/Renamed.renamed', '/World/RefSibling.moved'))
 
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Renamed': {},
                },
                '/RefSibling': {'.moved': Sdf.PathListOp()},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Renamed', '/Prim/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/RefSibling.moved', '/Prim/Ref/Child.attr']),
            },
        })

        # Reparent: /World/Ref/Renamed -> /World/RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /World/Ref/Renamed -> /World/RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/Ref/Renamed', '/World/RefSibling/Moved'))

        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {},
                '/RefSibling': {
                    '.moved': Sdf.PathListOp(),
                    '/Moved': {},
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Prim/RefSibling/Moved', '/Prim/RefSibling.moved']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/RefSibling.moved', '/Prim/Ref/Child.attr']),
            },
        })

        # Edit: Move /World/RefSibling/Moved back to its original path 
        # /World/Ref/Child.
        with self.ApplyEdits(editor,
                "Reparent /World/RefSibling/Moved -> /World/Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/RefSibling/Moved', '/World/Ref/Child'))

        # Edit: Move /World/RefSibling/.moved back to its original path 
        # /World/Ref/Child.attr.
        with self.ApplyEdits(editor,
                "Reparent /World/RefSibling.moved -> /World/Ref/Child.attr"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/World/RefSibling.moved', '/World/Ref/Child.attr'))
        
        # Verify the layers are back to their initial contents.
        # Unlike the references test, no target paths were stripped through
        # the reparent sequence: each move kept a valid composed path
        # on stage 2, so reparenting back fully restores both layers.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {'.attr': Sdf.PathListOp()},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Child', '/Relocated/Child.attr']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Relocated/Child.attr', '/Prim/Ref/Child.attr']),
            },
        })

        # Delete: /World/Ref/Child.attr
        with self.ApplyEdits(editor, 'Delete /World/Ref/Child.attr'):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/World/Ref/Child.attr'))

        # On stage2 the property at /Relocated/Child.attr is removed; the
        # /Relocated/Child.attr entries drop from rel and conn.
        # /Prim/Ref/Child.attr is untouched as before.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = ['/Relocated/Child']),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Ref/Child.attr']),
            },
        })

        # Delete: /World/Ref/Child
        with self.ApplyEdits(editor, 'Delete /World/Ref/Child'):
            self.assertTrue(editor.DeletePrimAtPath('/World/Ref/Child'))

        # On stage2 the relocated prim /Relocated/Child is removed; the
        # remaining /Relocated/Child entry drops from rel.
        # /Prim/Ref/Child* entries are untouched.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {},
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                '.rel': Sdf.PathListOp.Create(
                    prependedItems = ['/Prim/Ref/Child', '/Prim/Ref/Child.attr'],
                    appendedItems = []),
                '.conn': Sdf.PathListOp.CreateExplicit(
                    ['/Prim/Ref/Child.attr']),
            },
        })



if __name__ == '__main__':
    unittest.main()
