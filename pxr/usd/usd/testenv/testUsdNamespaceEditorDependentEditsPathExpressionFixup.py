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

class TestUsdNamespaceEditorDependentEditsPathExpressionFixup(
    TestUsdNamespaceEditorDependentEditsBase):
    '''Tests downstream dependency path-expression fixup when a prim or
    property is renamed, reparented, or deleted on the edited stage.
    '''

    # Verifies the expected layer contents of a layer in relation to the
    # prim and property hierarchies, the pathExpression-valued default
    # field on attributes, and the pathExpression-valued customData
    # entries on prim and property specs.
    #
    # The format of the expected contents is a nested dictionary
    # representing the full prim hierarchy as demonstrated in the example:
    #
    # {
    #     "/Root" : {
    #         "customData" : {
    #             "primExpr" : Sdf.PathExpression("/Root/A"),
    #         },
    #         "/A" : {
    #             ".expr" : {
    #                 "default" : Sdf.PathExpression("/Root/A//Mesh*"),
    #                 "customData" : {
    #                     "propExpr" : Sdf.PathExpression("/Root/A.attr"),
    #                 },
    #             },
    #             ".attr" : None,
    #         },
    #     }
    # }
    #
    # Keys starting with '/' indicate prim children.
    # Keys starting with '.' indicate property children. A property's
    #   expected value is either:
    #     - None, or
    #     - a dict that may contain a 'default' key (Sdf.PathExpression,
    #       Sdf.PathExpressionArray, or None meaning "no default
    #       authored") and a 'customData' key (a dict of customData
    #       name -> pathExpression-valued data; absent means "no
    #       customData authored").
    # The key 'customData' on a prim's contents dict indicates
    #   that the prim spec must carry the given customData entries.
    #
    def _VerifyLayerContents(self, layer, expectedContentsDict):
        self._VerifyExpectedLayerContentsFormat(expectedContentsDict)

        self.assertEqual(len(expectedContentsDict), len(layer.rootPrims),
            "The expected number of root prims in {} doesn't match the "
            "number of root prims {} on layer {}".format(
                list(expectedContentsDict.keys()),
                list(layer.rootPrims.keys()),
                layer.identifier))

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
            if k == 'customData':
                self.assertTrue(isinstance(v, dict),
                    "Value for 'customData' on prim key '{}' is not a "
                    "dictionary".format(pathKey or "None"))
            elif k.startswith('.'):
                self.assertTrue(v is None or isinstance(v, dict),
                    "Value '{}' for property key '{}' is invalid; it "
                    "must be None or a dict with optional 'default' "
                    "and 'customData' entries".format(str(v), k))
                if isinstance(v, dict):
                    for vk in v.keys():
                        self.assertIn(vk, ('default', 'customData'),
                            "Invalid key '{}' in property '{}' expected "
                            "dict; only 'default' and 'customData' are "
                            "allowed".format(vk, k))
            elif k.startswith('/'):
                self._VerifyExpectedLayerContentsFormat(v, k)
            else:
                self.assertTrue(False,
                    "Invalid expected contents dictionary key '{}'; it "
                    "must start with '/' for prim children, '.' for "
                    "property children, or be 'customData' for the "
                    "prim's customData".format(k))

    # Verifies the contents of a prim in the layer has the expected contents
    # indicated by the expectedContentsDict
    def _VerifyPrimContents(self, layer, path, expectedContentsDict):
        prim = layer.GetPrimAtPath(path)
        self.assertTrue(prim,
            "Expected to find prim at path {} in layer {}".format(
                path, layer.identifier))

        # Separate the prim's expected customData from its children
        expectedCustomData = expectedContentsDict.get('customData', None)
        expectedChildren = {k: v for k, v in expectedContentsDict.items()
                            if k != 'customData'}

        # Verify the prim spec's customData match expectations. Absent
        # entry means we require the spec to have no customData authored.
        self._VerifyCustomData(layer, path, expectedCustomData)

        # Prim must have the same number of prim and property children as
        # indicated in the expected child entries.
        self.assertEqual(
            len(expectedChildren),
            len(prim.properties) + len(prim.nameChildren),
            "The expected number of prims and properties in {} doesn't "
            "match the combined number of child properties {} and prims "
            "{} on the prim at {} in layer {}".format(
                list(expectedChildren.keys()),
                list(prim.properties.keys()),
                list(prim.nameChildren.keys()),
                path, layer.identifier))

        for childName, expectedChildContent in expectedChildren.items():
            childPath = Sdf.Path(str(path) + childName)
            if childPath.IsPrimPropertyPath():
                self._VerifyPropertyContents(
                    layer, childPath, expectedChildContent)
            else:
                self._VerifyPrimContents(
                    layer, childPath, expectedChildContent)

    # Verifies the contents of a property in the layer has the expected
    # default and customData values for path-expression-bearing fields.
    def _VerifyPropertyContents(self, layer, path, expectedValue):
        prop = layer.GetPropertyAtPath(path)
        self.assertTrue(prop,
            "Expected to find property at path {} in layer {}".format(
                path, layer.identifier))

        if expectedValue is None:
            return

        # Verify default if specified.
        if 'default' in expectedValue:
            expectedDefault = expectedValue['default']
            self.assertTrue(isinstance(prop, Sdf.AttributeSpec),
                "Property at {} in layer {} is not an attribute but a "
                "default value was specified".format(path, layer.identifier))
            actualDefault = prop.default
            self.assertEqual(actualDefault, expectedDefault,
                "Attribute at {} in layer {} has default value '{}' "
                "which does not match the expected value '{}'".format(
                    path, layer.identifier,
                    str(actualDefault), str(expectedDefault)))
        else:
            if isinstance(prop, Sdf.AttributeSpec):
                self.assertFalse(prop.HasInfo("default"),
                    "Attribute at {} in layer {} unexpectedly has a "
                    "default value authored".format(path, layer.identifier))

        # Verify customData.
        self._VerifyCustomData(layer, path, expectedValue.get('customData'))

    # Verifies the spec at specPath in the given layer has a customData
    # field whose entries match expectedCustomData exactly. If
    # expectedCustomData is None, the spec must have no customData
    # authored.
    def _VerifyCustomData(self, layer, specPath, expectedCustomData):
        spec = layer.GetObjectAtPath(specPath)
        self.assertTrue(spec,
            "Expected to find spec at path {} in layer {}".format(
                specPath, layer.identifier))

        # If no expected customData was given, the spec must have no
        # customData authored at all.
        if expectedCustomData is None:
            self.assertFalse(spec.HasInfo("customData"),
                "Spec at {} in layer {} unexpectedly has customData "
                "authored: {}".format(
                    specPath, layer.identifier, spec.customData))
            return

        actualCustomData = spec.customData
        self.assertEqual(set(actualCustomData.keys()),
                         set(expectedCustomData.keys()),
            "customData keys on spec at {} in layer {} are {} which "
            "does not match the expected keys {}".format(
                specPath, layer.identifier,
                sorted(actualCustomData.keys()),
                sorted(expectedCustomData.keys())))
        for key, expectedValue in expectedCustomData.items():
            actualValue = actualCustomData[key]
            self.assertEqual(actualValue, expectedValue,
                "customData[{!r}] on spec at {} in layer {} is '{}' "
                "which does not match the expected value '{}'".format(
                    key, specPath, layer.identifier,
                    str(actualValue), str(expectedValue)))

    def test_BasicReferences(self):
        '''Tests path expression fixup after downstream dependency namespace 
        edits across a reference.'''

        # Layer 1 defines /Ref/Child (the prim to edit) and
        # /Ref/Child.attr (the property to edit), /RefSibling (the
        # reparent destination), and /Local which carries path-
        # expression-bearing fields targeting both /Ref/Child* and
        # /InternalRef/Child*. 
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }

            def "RefSibling" {
            }

            def "Local" (
                customData = {
                    pathExpression primExpr = "/Ref/Child /InternalRef/Child"
                }
            ) {
                pathExpression expr = "/Ref/Child /InternalRef/Child" (
                    customData = {
                        pathExpression propExpr = "/Ref/Child.attr /InternalRef/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Ref/Child /InternalRef/Child",
                    "/Ref/Child.attr /InternalRef/Child.attr"]
            }

            def "InternalRef" (
                references = </Ref>
            ) {
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 references /Ref from /Prim and defines /Other with
        # the same path-expression-bearing fields, targeting /Prim/Child*.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Prim" (
                references = @''' + layer1.identifier + '''@</Ref>
            ) {
            }

            def "Other" (
                customData = {
                    pathExpression primExpr = "/Prim/Child /Prim/Child.attr"
                }
            ) {
                pathExpression expr = "/Prim/Child" (
                    customData = {
                        pathExpression propExpr = "/Prim/Child /Prim/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Prim/Child",
                    "/Prim/Child.attr"]
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
                '/Child': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child.attr /InternalRef/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Child /InternalRef/Child'),
                        Sdf.PathExpression(
                            '/Ref/Child.attr /InternalRef/Child.attr')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child.renamed /InternalRef/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Child /InternalRef/Child'),
                        Sdf.PathExpression(
                            '/Ref/Child.renamed /InternalRef/Child.renamed')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.renamed')]),
                },
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Renamed /InternalRef/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Renamed /InternalRef/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Renamed.renamed '
                            '/InternalRef/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Renamed /InternalRef/Renamed'),
                        Sdf.PathExpression(
                            '/Ref/Renamed.renamed '
                            '/InternalRef/Renamed.renamed')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Renamed /Prim/Renamed.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Renamed /Prim/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression('/Prim/Renamed.renamed')]),
                },
            },
        })

        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        # Path expressions targeting /Ref/Renamed.renamed directly can be 
        # updated to the new path. However, those targeting it across a 
        # reference (internal or not) no longer get the new .moved attribute
        # composed in, so path expressions containing /InternalRef/Renamed.renamed
        # (layer 1) or /Prim/Renamed.renamed (layer 2) have that path removed 
        # instead.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {},
            },
            '/RefSibling': {'.moved': None},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Renamed /InternalRef/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Renamed /InternalRef/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/RefSibling.moved'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Renamed /InternalRef/Renamed'),
                        Sdf.PathExpression('/RefSibling.moved')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        # For the same reason as above, path expressions containing 
        # /InternalRef/Renamed (layer 1) or /Prim/Renamed (layer 2) have that 
        # path removed, while path expressions containing /Ref/Renamed are 
        # updated to /RefSibling/Moved.
        self._VerifyLayerContents(layer1, {
            '/Ref': {},
            '/RefSibling': {
                '.moved': None,
                '/Moved': {},
            },
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/RefSibling/Moved'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/RefSibling/Moved'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/RefSibling.moved'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/RefSibling/Moved'),
                        Sdf.PathExpression('/RefSibling.moved')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
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

        # The prim/property structure returns to its original state with
        # one notable exception: the path-expressions that don't directly target 
        # /Ref/Child are NOT restored. 
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Ref/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Child'),
                        Sdf.PathExpression('/Ref/Child.attr')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reinitialize to reset the path expressions.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Verify the initial layer contents have been restored.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child.attr /InternalRef/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Child /InternalRef/Child'),
                        Sdf.PathExpression(
                            '/Ref/Child.attr /InternalRef/Child.attr')]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor, "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Ref/Child /InternalRef/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Ref/Child /InternalRef/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Child'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Delete: /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {},
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
            '/InternalRef': {},
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

    def test_BasicPayloads(self):
        '''Test path expression fixup after downstream dependency namespace edits 
        across a payload. This also verifies that path expression fixup on a 
        dependent stage is skipped when the payload is unloaded.
        '''

        # Layer 1 defines /Ref/Child (the prim to edit) and
        # /Ref/Child.attr (the property to edit), /RefSibling (the
        # reparent destination), and /Local with path-expression-bearing fields.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }

            def "RefSibling" {
            }

            def "Local" (
                customData = {
                    pathExpression primExpr = "/Ref/Child /Ref/Child.attr"
                }
            ) {
                pathExpression expr = "/Ref/Child" (
                    customData = {
                        pathExpression propExpr = "/Ref/Child /Ref/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Ref/Child",
                    "/Ref/Child.attr"]
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 payloads /Ref from /Prim and defines /Other with the
        # same path-expression-bearing fields, targeting /Prim/Child*.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            def "Prim" (
                payload = @''' + layer1.identifier + '''@</Ref>
            ) {
            }

            def "Other" (
                customData = {
                    pathExpression primExpr = "/Prim/Child /Prim/Child.attr"
                }
            ) {
                pathExpression expr = "/Prim/Child" (
                    customData = {
                        pathExpression propExpr = "/Prim/Child /Prim/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Prim/Child",
                    "/Prim/Child.attr"]
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
            '/Ref': {
                '/Child': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child /Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Child'),
                        Sdf.PathExpression('/Ref/Child.attr')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /Ref/Child.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child /Ref/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Child'),
                        Sdf.PathExpression('/Ref/Child.renamed')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.renamed')]),
                },
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Renamed /Ref/Renamed.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Renamed /Ref/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Renamed'),
                        Sdf.PathExpression('/Ref/Renamed.renamed')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Renamed /Prim/Renamed.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Renamed /Prim/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression('/Prim/Renamed.renamed')]),
                },
            },
        })

        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        # Path expressions targeting /Ref/Renamed.renamed directly can be 
        # updated to the new path. However, those targeting it across a 
        # payload no longer get the new .moved attribute composed in, so path 
        # expressions containing /Prim/Renamed.renamed (layer 2) have that path 
        # removed instead.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {},
            },
            '/RefSibling': {'.moved': None},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Renamed /RefSibling.moved'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Renamed /RefSibling.moved'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Renamed'),
                        Sdf.PathExpression('/RefSibling.moved')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        # For the same reason as above, path expressions containing 
        # /Prim/Renamed (layer 2) have that path removed, while path expressions 
        # containing /Ref/Renamed are updated to /RefSibling/Moved.
        self._VerifyLayerContents(layer1, {
            '/Ref': {},
            '/RefSibling': {
                '.moved': None,
                '/Moved': {},
            },
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/RefSibling/Moved /RefSibling.moved'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/RefSibling/Moved'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/RefSibling/Moved /RefSibling.moved'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/RefSibling/Moved'),
                        Sdf.PathExpression('/RefSibling.moved')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reinitialize to reset for the delete cases.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)
        
        # Verify the layers have been returned to their initial state.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Child /Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Child /Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Child'),
                        Sdf.PathExpression('/Ref/Child.attr')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor, "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Ref/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Ref/Child'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Child'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Delete: /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        self._VerifyLayerContents(layer1, {
            '/Ref': {},
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reset both layers. 
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Unload /Prim on stage 2 so the payload contents are not composed into 
        # the dependent stage's namespace. When /Prim is unloaded on stage 2,
        # /Prim/Child does not compose into the stage's namespace, so
        # the dependent stage path expression fixup cannot reach the
        # /Other expression entries that point at /Prim/Child* paths.
        # Layer 1's own path expressions on /Local are still fixed up
        # because the rename happens on stage 1.
        stage2.Unload('/Prim')

        # Rename: /Ref/Child -> /Ref/Renamed (with /Prim unloaded)
        with self.ApplyEdits(editor,
                "Rename /Ref/Child -> /Ref/Renamed (/Prim unloaded)"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Layer 1 is fixed up as usual since the rename is on stage 1.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.attr': None},
            },
            '/RefSibling': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Ref/Renamed /Ref/Renamed.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Ref/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Ref/Renamed /Ref/Renamed.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Ref/Renamed'),
                        Sdf.PathExpression('/Ref/Renamed.attr')]),
                },
            },
        })
        # Layer 2 expressions are NOT fixed up because /Prim is unloaded on 
        # stage 2 and /Prim/Child does not compose to anything.
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

        # Reload /Prim and observe the broken state: /Other expressions
        # still point at /Prim/Child paths even though the payload now
        # composes /Prim/Renamed.
        stage2.Load('/Prim')
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })

    def test_BasicSublayers(self):
        '''Tests path expression fixup after downstream dependency namespace 
        edits across sublayers.

        This test sets up a four stage hierarchy and registers ONLY
        stage3 as a dependent of the stage 1 editor to show that:

          - layer1 is fixed up directly because it's the edit target.
          - layer2 and layer3Sub are fixed up because they sit in
            stage3's layer stack (transitively or directly).
          - layer3 is fixed up because stage3 is registered as a
            dependent stage.
          - layer4 is NOT fixed up: stage4 was never registered as a
            dependent stage even though layer4 sublayers layer3.
        '''

        # Reusable USDA snippet for the /OtherN prim spec body that
        # carries the four path-expression-bearing fields targeting
        # /Ref/Child*. Each /OtherN is a root prim in its own layer 
        # so fixup is per-layer.
        def _OtherSpec(name):
            return ('''
            def "''' + name + '''" (
                customData = {
                    pathExpression primExpr = "/Ref/Child /Ref/Child.attr"
                }
            ) {
                pathExpression expr = "/Ref/Child" (
                    customData = {
                        pathExpression propExpr = "/Ref/Child /Ref/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Ref/Child",
                    "/Ref/Child.attr"]
            }
            ''')

        # Layer 1 is the edited base layer. It defines /Ref/Child,
        # /Ref/Child.attr, /RefSibling, and /Local with the four path-
        # expression-bearing fields.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }

            def "RefSibling" {
            }
        ''' + _OtherSpec("Local"))
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 sublayers layer1 and defines /Other2.
        layer2 = Sdf.Layer.CreateAnonymous("layer2.usda")
        layer2ImportString = ('''#usda 1.0
            (
                subLayers = [@''' + layer1.identifier + '''@]
            )
        ''' + _OtherSpec("Other2"))
        layer2.ImportFromString(layer2ImportString)

        # Layer 3 Sub will be a sublayer of layer3 and defines /Other3Sub.
        layer3Sub = Sdf.Layer.CreateAnonymous("layer3-sub.usda")
        layer3SubImportString = ('''#usda 1.0
        ''' + _OtherSpec("Other3Sub"))
        layer3Sub.ImportFromString(layer3SubImportString)

        # Layer 3 sublayers layer2 (which sublayers layer1) and
        # layer3Sub. Defines /Other3.
        layer3 = Sdf.Layer.CreateAnonymous("layer3.usda")
        layer3ImportString = ('''#usda 1.0
            (
                subLayers = [
                    @''' + layer2.identifier + '''@,
                    @''' + layer3Sub.identifier + '''@
                ]
            )
        ''' + _OtherSpec("Other3"))
        layer3.ImportFromString(layer3ImportString)

        # Layer 4 sublayers layer3. Stage 4 is intentionally not
        # registered as a dependent, so layer 4 stays untouched
        # throughout the test.
        layer4 = Sdf.Layer.CreateAnonymous("layer4.usda")
        layer4ImportString = ('''#usda 1.0
            (
                subLayers = [@''' + layer3.identifier + '''@]
            )
        ''' + _OtherSpec("Other4"))
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
        # holds its /OtherN prim with the same path-expression-bearing
        # fields, so they all get verified together against the same
        # expected values at each step of the test.
        def _VerifyOtherLayerExprs(expectedPrimExpr, expectedExprDefault,
                                   expectedPropExpr, expectedArr):
            def _Contents(name):
                return {
                    '/' + name: {
                        'customData': {
                            'primExpr': expectedPrimExpr,
                        },
                        '.expr': {
                            'default': expectedExprDefault,
                            'customData': {
                                'propExpr': expectedPropExpr,
                            },
                        },
                        '.arr': {
                            'default': expectedArr,
                        },
                    },
                }
            self._VerifyLayerContents(layer2, _Contents('Other2'))
            self._VerifyLayerContents(layer3Sub, _Contents('Other3Sub'))
            self._VerifyLayerContents(layer3, _Contents('Other3'))

        # Verify layer4 keeps its original path expressions -- stage4
        # was not registered as a dependent stage so /Other4 is never
        # updated regardless of what edits are applied via stage1.
        def _VerifyLayer4Unchanged():
            self._VerifyLayerContents(layer4, {
                '/Other4': {
                    'customData': {
                        'primExpr': Sdf.PathExpression(
                            '/Ref/Child /Ref/Child.attr'),
                    },
                    '.expr': {
                        'default': Sdf.PathExpression('/Ref/Child'),
                        'customData': {
                            'propExpr': Sdf.PathExpression(
                                '/Ref/Child /Ref/Child.attr'),
                        },
                    },
                    '.arr': {
                        'default': Sdf.PathExpressionArray([
                            Sdf.PathExpression('/Ref/Child'),
                            Sdf.PathExpression('/Ref/Child.attr')]),
                    },
                },
            })

        # Helper to verify layer1 contents at each step. layer1 carries /Ref,
        # /RefSibling, and /Local. /Local's expressions get the same
        # fixup treatment as the /OtherN prims since /Local is
        # essentially the same shape on the edited stage.
        def _VerifyLayer1Contents(refContents, refSiblingContents,
                                  localPrimExpr, localExprDefault,
                                  localPropExpr, localArr):
            self._VerifyLayerContents(layer1, {
                '/Ref': refContents,
                '/RefSibling': refSiblingContents,
                '/Local': {
                    'customData': {
                        'primExpr': localPrimExpr,
                    },
                    '.expr': {
                        'default': localExprDefault,
                        'customData': {
                            'propExpr': localPropExpr,
                        },
                    },
                    '.arr': {
                        'default': localArr,
                    },
                },
            })

        # Verify the initial layer contents.
        _VerifyLayer1Contents(
            refContents={'/Child': {'.attr': None}},
            refSiblingContents={},
            localPrimExpr=Sdf.PathExpression(
                '/Ref/Child /Ref/Child.attr'),
            localExprDefault=Sdf.PathExpression('/Ref/Child'),
            localPropExpr=Sdf.PathExpression(
                '/Ref/Child /Ref/Child.attr'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression('/Ref/Child.attr')]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/Ref/Child /Ref/Child.attr'),
            Sdf.PathExpression('/Ref/Child'),
            Sdf.PathExpression('/Ref/Child /Ref/Child.attr'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression('/Ref/Child.attr')]))
        _VerifyLayer4Unchanged()

        # Rename: /Ref/Child.attr -> /Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/Ref/Child.attr -> /Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Child.attr', '/Ref/Child.renamed'))

        _VerifyLayer1Contents(
            refContents={'/Child': {'.renamed': None}},
            refSiblingContents={},
            localPrimExpr=Sdf.PathExpression(
                '/Ref/Child /Ref/Child.renamed'),
            localExprDefault=Sdf.PathExpression('/Ref/Child'),
            localPropExpr=Sdf.PathExpression(
                '/Ref/Child /Ref/Child.renamed'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression('/Ref/Child.renamed')]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/Ref/Child /Ref/Child.renamed'),
            Sdf.PathExpression('/Ref/Child'),
            Sdf.PathExpression('/Ref/Child /Ref/Child.renamed'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression('/Ref/Child.renamed')]))
        _VerifyLayer4Unchanged()

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        _VerifyLayer1Contents(
            refContents={'/Renamed': {'.renamed': None}},
            refSiblingContents={},
            localPrimExpr=Sdf.PathExpression(
                '/Ref/Renamed /Ref/Renamed.renamed'),
            localExprDefault=Sdf.PathExpression('/Ref/Renamed'),
            localPropExpr=Sdf.PathExpression(
                '/Ref/Renamed /Ref/Renamed.renamed'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Renamed'),
                Sdf.PathExpression('/Ref/Renamed.renamed')]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/Ref/Renamed /Ref/Renamed.renamed'),
            Sdf.PathExpression('/Ref/Renamed'),
            Sdf.PathExpression('/Ref/Renamed /Ref/Renamed.renamed'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Renamed'),
                Sdf.PathExpression('/Ref/Renamed.renamed')]))
        _VerifyLayer4Unchanged()

        # Reparent: /Ref/Renamed.renamed -> /RefSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed.renamed -> /RefSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Ref/Renamed.renamed', '/RefSibling.moved'))

        _VerifyLayer1Contents(
            refContents={'/Renamed': {}},
            refSiblingContents={'.moved': None},
            localPrimExpr=Sdf.PathExpression(
                '/Ref/Renamed /RefSibling.moved'),
            localExprDefault=Sdf.PathExpression('/Ref/Renamed'),
            localPropExpr=Sdf.PathExpression(
                '/Ref/Renamed /RefSibling.moved'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Renamed'),
                Sdf.PathExpression('/RefSibling.moved')]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/Ref/Renamed /RefSibling.moved'),
            Sdf.PathExpression('/Ref/Renamed'),
            Sdf.PathExpression('/Ref/Renamed /RefSibling.moved'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Renamed'),
                Sdf.PathExpression('/RefSibling.moved')]))
        _VerifyLayer4Unchanged()

        # Reparent: /Ref/Renamed -> /RefSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Ref/Renamed -> /RefSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Renamed', '/RefSibling/Moved'))

        _VerifyLayer1Contents(
            refContents={},
            refSiblingContents={'.moved': None, '/Moved': {}},
            localPrimExpr=Sdf.PathExpression(
                '/RefSibling/Moved /RefSibling.moved'),
            localExprDefault=Sdf.PathExpression('/RefSibling/Moved'),
            localPropExpr=Sdf.PathExpression(
                '/RefSibling/Moved /RefSibling.moved'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/RefSibling/Moved'),
                Sdf.PathExpression('/RefSibling.moved')]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/RefSibling/Moved /RefSibling.moved'),
            Sdf.PathExpression('/RefSibling/Moved'),
            Sdf.PathExpression('/RefSibling/Moved /RefSibling.moved'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/RefSibling/Moved'),
                Sdf.PathExpression('/RefSibling.moved')]))
        _VerifyLayer4Unchanged()

        # Reinitialize all layers so the deletes start from the
        # original path expressions without the previous reparent
        # edits.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)
        layer3Sub.ImportFromString(layer3SubImportString)
        layer3.ImportFromString(layer3ImportString)
        layer4.ImportFromString(layer4ImportString)

        # Delete: /Ref/Child.attr
        with self.ApplyEdits(editor, "Delete /Ref/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Ref/Child.attr'))

        _VerifyLayer1Contents(
            refContents={'/Child': {}},
            refSiblingContents={},
            localPrimExpr=Sdf.PathExpression('/Ref/Child'),
            localExprDefault=Sdf.PathExpression('/Ref/Child'),
            localPropExpr=Sdf.PathExpression('/Ref/Child'),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression.Nothing()]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression('/Ref/Child'),
            Sdf.PathExpression('/Ref/Child'),
            Sdf.PathExpression('/Ref/Child'),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Ref/Child'),
                Sdf.PathExpression.Nothing()]))
        _VerifyLayer4Unchanged()

        # Delete: /Ref/Child
        with self.ApplyEdits(editor, "Delete /Ref/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Ref/Child'))

        _VerifyLayer1Contents(
            refContents={},
            refSiblingContents={},
            localPrimExpr=Sdf.PathExpression.Nothing(),
            localExprDefault=Sdf.PathExpression.Nothing(),
            localPropExpr=Sdf.PathExpression.Nothing(),
            localArr=Sdf.PathExpressionArray([
                Sdf.PathExpression.Nothing(),
                Sdf.PathExpression.Nothing()]))
        _VerifyOtherLayerExprs(
            Sdf.PathExpression.Nothing(),
            Sdf.PathExpression.Nothing(),
            Sdf.PathExpression.Nothing(),
            Sdf.PathExpressionArray([
                Sdf.PathExpression.Nothing(),
                Sdf.PathExpression.Nothing()]))
        _VerifyLayer4Unchanged()


    def _RunTestBasicGlobalClassArcs(self, classArcType):
        '''Helper for testing path expression fixup after downstream dependency 
        namespace edits across global class arcs and their implied class specs. 
        classArcType can be either 'inherits' or 'specializes
        '''

        # Layer 1 defines /Class/Child and /Class/Child.attr,
        # /ClassSibling, /Prim which inherits or specializes /Class,
        # and /Local with path-expression-bearing fields that target
        # /Prim/Child* (the path translated through /Prim's class arc).
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

            def "Local" (
                customData = {
                    pathExpression primExpr = "/Prim/Child /Prim/Child.attr"
                }
            ) {
                pathExpression expr = "/Prim/Child" (
                    customData = {
                        pathExpression propExpr = "/Prim/Child /Prim/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Prim/Child",
                    "/Prim/Child.attr"]
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 references /Prim from layer1 as /Prim2 (so the global
        # class arc on /Prim propagates as an implied class arc to
        # /Class in layer2's namespace) and defines /Other with path-
        # expression-bearing fields that include both /Prim2/Child*
        # paths (via the reference plus class arc) and /Class/Child*
        # paths (directly through the class spec, which composes via
        # the implied class arc).
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

            def "Other" (
                customData = {
                    pathExpression primExpr = "/Prim2/Child /Prim2/Child.attr /Class/Child /Class/Child.attr"
                }
            ) {
                pathExpression expr = "/Prim2/Child /Class/Child" (
                    customData = {
                        pathExpression propExpr = "/Prim2/Child /Prim2/Child.attr /Class/Child /Class/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Prim2/Child /Class/Child",
                    "/Prim2/Child.attr /Class/Child.attr"]
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
                '/Child': {'.attr': None},
            },
            '/ClassSibling': {},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.attr')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {'.attr': None},
            },
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim2/Child /Prim2/Child.attr '
                        '/Class/Child /Class/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim2/Child /Class/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim2/Child /Prim2/Child.attr '
                            '/Class/Child /Class/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim2/Child /Class/Child'),
                        Sdf.PathExpression(
                            '/Prim2/Child.attr /Class/Child.attr')]),
                },
            },
        })

        # Rename: /Class/Child.attr -> /Class/Child.renamed
        with self.ApplyEdits(editor,
                '/Class/Child.attr -> /Class/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Class/Child.attr', '/Class/Child.renamed'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Child': {'.renamed': None},
            },
            '/ClassSibling': {},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression('/Prim/Child.renamed')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {'.renamed': None},
            },
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim2/Child /Prim2/Child.renamed '
                        '/Class/Child /Class/Child.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim2/Child /Class/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim2/Child /Prim2/Child.renamed '
                            '/Class/Child /Class/Child.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim2/Child /Class/Child'),
                        Sdf.PathExpression(
                            '/Prim2/Child.renamed /Class/Child.renamed')]),
                },
            },
        })

        # Rename: /Class/Child -> /Class/Renamed
        with self.ApplyEdits(editor, '/Class/Child -> /Class/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/Child', '/Class/Renamed'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Renamed': {'.renamed': None},
            },
            '/ClassSibling': {},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Renamed /Prim/Renamed.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Renamed /Prim/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression('/Prim/Renamed.renamed')]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Renamed': {'.renamed': None},
            },
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim2/Renamed /Prim2/Renamed.renamed '
                        '/Class/Renamed /Class/Renamed.renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim2/Renamed /Class/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim2/Renamed /Prim2/Renamed.renamed '
                            '/Class/Renamed /Class/Renamed.renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim2/Renamed /Class/Renamed'),
                        Sdf.PathExpression(
                            '/Prim2/Renamed.renamed '
                            '/Class/Renamed.renamed')]),
                },
            },
        })

        # Reparent: /Class/Renamed.renamed -> /ClassSibling.moved
        with self.ApplyEdits(editor,
                "Reparent /Class/Renamed.renamed -> /ClassSibling.moved"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/Class/Renamed.renamed', '/ClassSibling.moved'))

        # /ClassSibling is not inherited or specialized by /Prim, so the
        # property is no longer reachable through /Prim's class arc and
        # the /Prim/Renamed.renamed (and /Class/Renamed.renamed)
        # operands are stripped from the path expressions.
        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Renamed': {},
            },
            '/ClassSibling': {'.moved': None},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Renamed': {},
            },
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim2/Renamed /Class/Renamed'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim2/Renamed /Class/Renamed'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim2/Renamed /Class/Renamed'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim2/Renamed /Class/Renamed'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reparent: /Class/Renamed -> /ClassSibling/Moved
        with self.ApplyEdits(editor,
                "Reparent /Class/Renamed -> /ClassSibling/Moved"):
            self.assertTrue(editor.MovePrimAtPath(
                '/Class/Renamed', '/ClassSibling/Moved'))

        # The prim itself is moved out from under /Class so /Prim/Renamed
        # is no longer reachable via the class arc and the remaining
        # operands are stripped too.
        self._VerifyLayerContents(layer1, {
            '/Class': {},
            '/ClassSibling': {
                '.moved': None,
                '/Moved': {},
            },
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Reinitialize to reset the path expressions for the delete cases.
        layer1.ImportFromString(layer1ImportString)
        layer2.ImportFromString(layer2ImportString)

        # Delete: /Class/Child.attr
        with self.ApplyEdits(editor, "Delete /Class/Child.attr"):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/Class/Child.attr'))

        self._VerifyLayerContents(layer1, {
            '/Class': {
                '/Child': {},
            },
            '/ClassSibling': {},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression('/Prim/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression('/Prim/Child'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {
                '/Child': {},
            },
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim2/Child /Class/Child'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim2/Child /Class/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim2/Child /Class/Child'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim2/Child /Class/Child'),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

        # Delete: /Class/Child
        with self.ApplyEdits(editor, "Delete /Class/Child"):
            self.assertTrue(editor.DeletePrimAtPath('/Class/Child'))

        self._VerifyLayerContents(layer1, {
            '/Class': {},
            '/ClassSibling': {},
            '/Prim': {},
            '/Local': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim2': {},
            '/Class': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression.Nothing(),
                },
                '.expr': {
                    'default': Sdf.PathExpression.Nothing(),
                    'customData': {
                        'propExpr': Sdf.PathExpression.Nothing(),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression.Nothing(),
                        Sdf.PathExpression.Nothing()]),
                },
            },
        })

    def test_BasicInherits(self):
        self._RunTestBasicGlobalClassArcs("inherits")

    def test_BasicSpecializes(self):
        self._RunTestBasicGlobalClassArcs("specializes")

    def test_BasicVariants(self):
        '''Tests path expression fixup from downstream dependency namespace edits 
        across a reference contained within a variant both when the variant is 
        selected and when it is not.
        '''

        # Layer 1 defines /Ref/Child and /Ref/Child.attr (the prim and
        # property to edit).
        layer1 = Sdf.Layer.CreateAnonymous("layer1.usda")
        layer1ImportString = ('''#usda 1.0
            def "Ref" {
                def "Child" {
                    custom int attr
                }
            }
        ''')
        layer1.ImportFromString(layer1ImportString)

        # Layer 2 defines /Variant with a variant set "v" whose
        # "selected" variant references /Ref from layer1, /Prim which
        # references /Variant, and /Other with path-expression-bearing
        # fields whose operands list both /Prim/Child* and /Variant/Child* 
        # (which does NOT compose on stage2 because the variant has no 
        # selection). Only the /Prim/Child* operands get fixed up: 
        # /Variant/Child* operands are left alone.
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

            def "Other" (
                customData = {
                    pathExpression primExpr = "/Prim/Child /Prim/Child.attr /Variant/Child /Variant/Child.attr"
                }
            ) {
                pathExpression expr = "/Prim/Child /Variant/Child" (
                    customData = {
                        pathExpression propExpr = "/Prim/Child /Prim/Child.attr /Variant/Child /Variant/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Prim/Child /Variant/Child",
                    "/Prim/Child.attr /Variant/Child.attr"]
            }
        ''')
        layer2.ImportFromString(layer2ImportString)

        # Session layer authors the variant selection on /Prim so the
        # reference inside the variant composes when stage 2 is opened.
        sessionLayer = Sdf.Layer.CreateAnonymous("session.usda")
        sessionLayer.ImportFromString('''#usda 1.0
            over "Prim" (
                variants = {
                    string v = "selected"
                }
            ) {
            }
        ''')

        # Open both layers as stages. Stage 2 uses the session layer to
        # provide the variant selection.
        stage1 = Usd.Stage.Open(layer1, Usd.Stage.LoadAll)
        stage2 = Usd.Stage.Open(layer2, sessionLayer)

        # Create an editor for stage 1 with stage 2 as an additional
        # dependent stage.
        editor = Usd.NamespaceEditor(stage1)
        editor.AddDependentStage(stage2)

        # Verify the initial layer contents.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.attr': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr '
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr '
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child /Variant/Child'),
                        Sdf.PathExpression(
                            '/Prim/Child.attr /Variant/Child.attr')]),
                },
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
                '/Child': {'.renamed': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.renamed '
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.renamed '
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child /Variant/Child'),
                        Sdf.PathExpression(
                            '/Prim/Child.renamed /Variant/Child.attr')]),
                },
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        with self.ApplyEdits(editor, '/Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Only /Prim/Child is fixed up for the same reason as above.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Renamed /Prim/Renamed.renamed '
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Renamed /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Renamed /Prim/Renamed.renamed '
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Renamed /Variant/Child'),
                        Sdf.PathExpression(
                            '/Prim/Renamed.renamed /Variant/Child.attr')]),
                },
            },
        })

        # Reinitialize the path expressions for the delete cases.
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
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child /Variant/Child'),
                        Sdf.PathExpression('/Variant/Child.attr')]),
                },
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
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Variant/Child'),
                        Sdf.PathExpression('/Variant/Child.attr')]),
                },
            },
        })

        # Reinitialize layers and mute the session layer so /Prim has
        # no variant selection on stage 2. Now neither /Prim/Child* nor
        # /Variant/Child* composes on stage 2; the path expressions in
        # /Other have no composed dependency on /Ref/Child* and stay
        # untouched.
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
        # path expressions on /Other are unchanged.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Child': {'.renamed': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr '
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr '
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child /Variant/Child'),
                        Sdf.PathExpression(
                            '/Prim/Child.attr /Variant/Child.attr')]),
                },
            },
        })

        # Rename: /Ref/Child -> /Ref/Renamed
        # layer1 is updated; layer2 /Other expressions remain unchanged.
        with self.ApplyEdits(editor, 'Muted: /Ref/Child -> /Ref/Renamed'):
            self.assertTrue(editor.MovePrimAtPath(
                '/Ref/Child', '/Ref/Renamed'))

        # Now that there is no variant selection, layer1 is updated, but all the
        # path expressions on /Other are unchanged.
        self._VerifyLayerContents(layer1, {
            '/Ref': {
                '/Renamed': {'.renamed': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Variant': {},
            '/Prim': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Child /Prim/Child.attr '
                        '/Variant/Child /Variant/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/Child /Variant/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Child /Prim/Child.attr '
                            '/Variant/Child /Variant/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Child /Variant/Child'),
                        Sdf.PathExpression(
                            '/Prim/Child.attr /Variant/Child.attr')]),
                },
            },
        })

    def test_BasicRelocates(self):
        '''Tests path expression fixup after downstream dependency namespace edits 
        across a reference where a child of the referencing prim is then 
        relocated.'''

        # Layer 1 defines /World/Ref/Child (the prim to edit) and
        # /World/Ref/Child.attr (the property to edit), plus
        # /World/RefSibling (the reparent destination outside /Ref).
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

        # Layer 2 defines /Prim which references /World plus a
        # relocates that moves /Prim/Ref to /Relocated. /Other carries
        # path-expression-bearing fields whose operands list both
        # /Relocated/Child* (which composes on stage 2 via the
        # relocate) and /Prim/Ref/Child* (which is the tombstone relocate 
        # source path: it does not exist on stage 2, so those paths are 
        # NOT fixed up).
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

            def "Other" (
                customData = {
                    pathExpression primExpr = "/Relocated/Child /Relocated/Child.attr /Prim/Ref/Child /Prim/Ref/Child.attr"
                }
            ) {
                pathExpression expr = "/Relocated/Child /Prim/Ref/Child" (
                    customData = {
                        pathExpression propExpr = "/Relocated/Child /Relocated/Child.attr /Prim/Ref/Child /Prim/Ref/Child.attr"
                    }
                )
                pathExpression[] arr = [
                    "/Relocated/Child /Prim/Ref/Child",
                    "/Relocated/Child.attr /Prim/Ref/Child.attr"]
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
            '/World': {
                '/Ref': {
                    '/Child': {'.attr': None},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Child /Relocated/Child.attr '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Child /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Child /Relocated/Child.attr '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Child /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Relocated/Child.attr /Prim/Ref/Child.attr')]),
                },
            },
        })

        # Rename: /World/Ref/Child.attr -> /World/Ref/Child.renamed
        with self.ApplyEdits(editor,
                '/World/Ref/Child.attr -> /World/Ref/Child.renamed'):
            self.assertTrue(editor.MovePropertyAtPath(
                '/World/Ref/Child.attr', '/World/Ref/Child.renamed'))

        # On stage 2 the relocated property /Relocated/Child.attr
        # renames to /Relocated/Child.renamed; the /Prim/Ref/Child.attr
        # operand has no composed dependency (relocate source) and is
        # left untouched.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {'.renamed': None},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Child /Relocated/Child.renamed '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Child /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Child /Relocated/Child.renamed '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Child /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Relocated/Child.renamed /Prim/Ref/Child.attr')]),
                },
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
                    '/Renamed': {'.renamed': None},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Renamed /Relocated/Renamed.renamed '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Renamed /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Renamed /Relocated/Renamed.renamed '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Renamed /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Relocated/Renamed.renamed '
                            '/Prim/Ref/Child.attr')]),
                },
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
                '/RefSibling': {'.moved': None},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Renamed /Prim/RefSibling.moved '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Renamed /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Renamed /Prim/RefSibling.moved '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Renamed /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Prim/RefSibling.moved /Prim/Ref/Child.attr')]),
                },
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
                    '.moved': None,
                    '/Moved': {},
                },
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/RefSibling/Moved /Prim/RefSibling.moved '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Prim/RefSibling/Moved /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/RefSibling/Moved /Prim/RefSibling.moved '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Prim/RefSibling/Moved /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Prim/RefSibling.moved /Prim/Ref/Child.attr')]),
                },
            },
        })

        # Edit: Move /World/RefSibling/Moved back to its original path
        # /World/Ref/Child.
        with self.ApplyEdits(editor,
                "Reparent /World/RefSibling/Moved -> /World/Ref/Child"):
            self.assertTrue(editor.MovePrimAtPath(
                '/World/RefSibling/Moved', '/World/Ref/Child'))

        # Edit: Move /World/RefSibling.moved back to its original path
        # /World/Ref/Child.attr.
        with self.ApplyEdits(editor,
                "Reparent /World/RefSibling.moved -> /World/Ref/Child.attr"):
            self.assertTrue(editor.MovePropertyAtPath(
                '/World/RefSibling.moved', '/World/Ref/Child.attr'))

        # Verify the layers are fully back to their initial contents.
        # Unlike the references test, no operands were stripped through
        # the reparent sequence: each move kept a valid composed path
        # on stage 2, so reparenting back fully restores both layers.
        self._VerifyLayerContents(layer1, {
            '/World': {
                '/Ref': {
                    '/Child': {'.attr': None},
                },
                '/RefSibling': {},
            },
        })
        self._VerifyLayerContents(layer2, {
            '/Prim': {},
            '/Relocated': {},
            '/Other': {
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Child /Relocated/Child.attr '
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Child /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Child /Relocated/Child.attr '
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Child /Prim/Ref/Child'),
                        Sdf.PathExpression(
                            '/Relocated/Child.attr /Prim/Ref/Child.attr')]),
                },
            },
        })

        # Delete: /World/Ref/Child.attr
        with self.ApplyEdits(editor, 'Delete /World/Ref/Child.attr'):
            self.assertTrue(editor.DeletePropertyAtPath(
                '/World/Ref/Child.attr'))

        # On stage 2 the property at /Relocated/Child.attr disappears;
        # operands referencing it are stripped. /Prim/Ref/Child.attr
        # (relocate-source path) has no composed dependency and stays.
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
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Relocated/Child /Prim/Ref/Child '
                        '/Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression(
                        '/Relocated/Child /Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Relocated/Child /Prim/Ref/Child '
                            '/Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression(
                            '/Relocated/Child /Prim/Ref/Child'),
                        Sdf.PathExpression('/Prim/Ref/Child.attr')]),
                },
            },
        })

        # Delete: /World/Ref/Child
        with self.ApplyEdits(editor, 'Delete /World/Ref/Child'):
            self.assertTrue(editor.DeletePrimAtPath('/World/Ref/Child'))

        # On stage 2 the relocated prim /Relocated/Child disappears;
        # the remaining /Relocated/Child operand is stripped.
        # /Prim/Ref/Child* operands remain.
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
                'customData': {
                    'primExpr': Sdf.PathExpression(
                        '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                },
                '.expr': {
                    'default': Sdf.PathExpression('/Prim/Ref/Child'),
                    'customData': {
                        'propExpr': Sdf.PathExpression(
                            '/Prim/Ref/Child /Prim/Ref/Child.attr'),
                    },
                },
                '.arr': {
                    'default': Sdf.PathExpressionArray([
                        Sdf.PathExpression('/Prim/Ref/Child'),
                        Sdf.PathExpression('/Prim/Ref/Child.attr')]),
                },
            },
        })


if __name__ == '__main__':
    unittest.main()
