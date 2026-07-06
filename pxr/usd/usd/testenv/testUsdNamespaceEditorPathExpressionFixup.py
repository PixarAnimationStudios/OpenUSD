#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Sdf, Usd, Tf

class TestUsdNamespaceEditorPathExpressionFixup(unittest.TestCase):

    # Verifies the expected layer contents of a layer in relation to the
    # prim and property hierarchies and the PathExpression default values of
    # path expression-valued attributes.
    #
    # The format of the expected contents is a nested dictionary
    # representing the full prim hierarchy as demonstrated in the example:
    #
    # {
    #     "/Root" : {
    #         "/A" : {
    #             ".fromRoot:test" :
    #                 Sdf.PathExpression("/Root/A//Mesh*"),
    #             ".attr" : None,
    #         },
    #         "/OtherPrim" : {
    #             ".fromRoot:refs" :
    #                 Sdf.PathExpression("/Root/A /Root/B"),
    #         }
    #     }
    # }
    #
    # Keys starting with '/' indicate prim children.
    # Keys starting with '.' indicate property children whose default value
    # must match the given Sdf.PathExpression if one is provided, or whose
    # existence is verified if None is given.
    #
    def _VerifyLayerContents(self, layer, expectedContentsDict):
        self._VerifyExpectedLayerContentsFormat(expectedContentsDict)

        self.assertEqual(len(expectedContentsDict), len(layer.rootPrims),
            "The expected number of root prims in {} doesn't match the number "
            "of root prims {} on the layer".format(
                list(expectedContentsDict.keys()), list(layer.rootPrims.keys())))

        for rootPrimPath, expectedChild in expectedContentsDict.items():
            self._VerifyPrimContents(layer, rootPrimPath, expectedChild)

    def _VerifyExpectedLayerContentsFormat(self, expectedContents, pathKey=None):
        self.assertTrue(isinstance(expectedContents, dict),
            "Expected contents for prim key '{}' is not a dictionary".format(
                pathKey or "None"))
        for k, v in expectedContents.items():
            if k.startswith('.'):
                self.assertTrue(
                    isinstance(v, Sdf.PathExpression) or isinstance(v, Sdf.PathExpressionArray) or v is None,
                    "Value '{}' for property key '{}' is invalid; it must be "
                    "an Sdf.PathExpression or None".format(str(v), k))
            elif k.startswith('/'):
                self._VerifyExpectedLayerContentsFormat(v, k)
            else:
                self.assertTrue(False, "Invalid expected contents dictionary "
                    "key '{}'; it must start with '/' for prim children or '.' "
                    "for property children".format(k))

    def _VerifyPrimContents(self, layer, path, expectedContentsDict):
        prim = layer.GetPrimAtPath(path)
        self.assertTrue(prim, "Expected to find prim at path {}".format(path))

        self.assertEqual(len(expectedContentsDict),
            len(prim.properties) + len(prim.nameChildren),
            "The expected number of prims and properties in {} doesn't match "
            "the combined number of child properties {} and prims {} on the "
            "prim at {}".format(
                list(expectedContentsDict.keys()), list(prim.properties.keys()),
                list(prim.nameChildren.keys()), path))

        for childName, expectedChildContent in expectedContentsDict.items():
            childPath = Sdf.Path(str(path) + childName)

            if childPath.IsPrimPropertyPath():
                self._VerifyPropertyContents(
                    layer, childPath, expectedChildContent)
            else:
                self._VerifyPrimContents(layer, childPath, expectedChildContent)

    def _VerifyPropertyContents(self, layer, path, expectedValue):
        prop = layer.GetPropertyAtPath(path)
        self.assertTrue(prop,
            "Expected to find property at path {}".format(path))

        if expectedValue is None:
            return

        self.assertTrue(isinstance(prop, Sdf.AttributeSpec),
            "Property at {} is not an attribute".format(path))

        actualValue = prop.default
        self.assertEqual(actualValue, expectedValue,
            "Attribute at {} has default value '{}' which does not match the "
            "expected value '{}'".format(
                path,
                str(actualValue) if actualValue else "None",
                str(expectedValue) if expectedValue else "None"))

    # Verifies that the spec at specPath in the given layer has a customData
    # field whose entries match expectedCustomData exactly. The expected
    # values must be Sdf.PathExpression or Sdf.PathExpressionArray instances;
    # this helper is for path-expression-valued customData only.
    def _VerifyCustomData(self, layer, specPath, expectedCustomData):
        spec = layer.GetObjectAtPath(specPath)
        self.assertTrue(spec,
            "Expected to find spec at path {} in layer {}".format(
                specPath, layer.identifier))
        actualCustomData = spec.customData
        self.assertEqual(set(actualCustomData.keys()),
                         set(expectedCustomData.keys()),
            "customData keys on spec at {} in layer {} are {} which does not "
            "match the expected keys {}".format(
                specPath, layer.identifier,
                sorted(actualCustomData.keys()),
                sorted(expectedCustomData.keys())))
        for key, expectedValue in expectedCustomData.items():
            actualValue = actualCustomData[key]
            self.assertEqual(actualValue, expectedValue,
                "customData[{!r}] on spec at {} in layer {} is '{}' which "
                "does not match the expected value '{}'".format(
                    key, specPath, layer.identifier,
                    str(actualValue), str(expectedValue)))

    def _OpenBasicStage(self):
        self.stage = Usd.Stage.Open("basic/root.usda")
        self.stage.Reload()
        stageLayers = self.stage.GetUsedLayers()

        self.rootLayer = Sdf.Find("basic/root.usda")
        self.assertTrue(self.rootLayer)
        self.assertTrue(self.rootLayer in stageLayers)

        self.sub1Layer = Sdf.Find("basic/sub1.usda")
        self.assertTrue(self.sub1Layer)
        self.assertTrue(self.sub1Layer in stageLayers)

        self.sub2Layer = Sdf.Find("basic/sub2.usda")
        self.assertTrue(self.sub2Layer)
        self.assertTrue(self.sub2Layer in stageLayers)

                # Verify initial layer contents.
        self._VerifyLayerContents(self.rootLayer, {
            "/Root" : {
                "/A" : {
                    ".fromRoot:a" :
                        Sdf.PathExpression("/Root/A//Mesh*"),
                    "/B" : {
                        ".fromRoot:b" :
                            Sdf.PathExpression("/Root/A/B /Root/Other"),
                    },
                    "/C" : {
                        ".attr" : None,
                        ".fromRoot:c" :
                            Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                    },
                },
                "/OtherPrim" : {
                    ".fromRoot:arr" :
                        Sdf.PathExpressionArray(
                            (Sdf.PathExpression("/Root/A/B//Geom*"), 
                             Sdf.PathExpression("/Root/A/C"))),
                    ".fromRoot:global" :
                        Sdf.PathExpression("//Mesh*"),
                    ".fromRoot:exprRef" :
                        Sdf.PathExpression(
                            "/Root/OtherPrim %/Root/A:test"),
                }
            }
        })

        self._VerifyLayerContents(self.sub1Layer, {
            "/Root" : {
                "/A" : {
                    ".fromLayer1:a" :
                        Sdf.PathExpression("/Root/A/B /Root/A/C"),
                    "/B" : {
                        ".attr" : None,
                        ".fromLayer1:b" :
                            Sdf.PathExpression(
                                "/Root/A /Root/A/C "
                                "/Root/A.attr /Root/A/C.attr"),
                    },
                },
                "/OtherPrim" : {
                    ".fromLayer1:other" :
                        Sdf.PathExpression("/Root/A //Geom*"),
                }
            }
        })

        self._VerifyLayerContents(self.sub2Layer, {
            "/Root" : {
                "/A" : {
                    ".attr" : None,
                    ".fromLayer2:a" :
                        Sdf.PathExpression(
                            "/Root/A/B /Root/A/B.attr "
                            "/Root/A/C /Root/A/C.attr"),
                },
                "/OtherPrim" : {
                    ".fromLayer2:arr" :
                        Sdf.PathExpressionArray((
                            Sdf.PathExpression("/Root/A/B"), 
                            Sdf.PathExpression("/Root/A/C"),
                            Sdf.PathExpression("/Root/A"),
                            Sdf.PathExpression(
                                "/Root/A/B.attr /Root/A/C.attr /Root/A.attr"))),
                    ".fromLayer2:global" :
                        Sdf.PathExpression("//B*"),
                    ".fromLayer2:exprRef" :
                        Sdf.PathExpression("%/Root/A:fromLayer2"),
                }
            }
        })

        # Verify the baseline path-expression customData entries authored on
        # both prim specs and property specs in each layer.
        self._VerifyCustomData(self.rootLayer, "/Root/A", {
            "rootPrimExpr" : Sdf.PathExpression("/Root/A//Mesh*"),
            "rootPrimArr" : Sdf.PathExpressionArray((
                Sdf.PathExpression("/Root/A/B"),
                Sdf.PathExpression("/Root/A/C"))),
        })
        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/A/B /Root/A/C.attr"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A", {
            "layer1PrimExpr" : Sdf.PathExpression("/Root/A/B"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A.fromLayer1:a", {
            "layer1PropExpr" : Sdf.PathExpression("/Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A", {
            "layer2PrimExpr" : Sdf.PathExpression("/Root/A/B /Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression("/Root/A/B.attr"),
        })

    def _ApplyCompareAndReset(self, editor, rootContents, sub1Contents,
                              sub2Contents):
        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertFalse(result.warnings)
        self.assertTrue(editor.ApplyEdits())
        self._VerifyLayerContents(self.rootLayer, rootContents)
        self._VerifyLayerContents(self.sub1Layer, sub1Contents)
        self._VerifyLayerContents(self.sub2Layer, sub2Contents)
        self.stage.Reload()

    # Apply the editor's pending edits without performing any layer-contents
    # comparison. New path-expression-field tests use this variant and verify
    # the relevant customData entries inline before calling ResetStage.
    def _Apply(self, editor):
        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertFalse(result.warnings)
        self.assertTrue(editor.ApplyEdits())

    def test_DeletePrimWithPathExpressions(self):
        """Test deleting prims. Path expression components on surviving prims 
        that refer to the deleted prim are removed."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root. 
        # This is the only root prim so it deletes everything.
        # This is mostly a sanity check that deleting everything at the root
        # doesn't cause any issues with respect to found path expression 
        # dependencies.
        self.assertTrue(editor.DeletePrimAtPath("/Root"))
        self._ApplyCompareAndReset(editor,
            rootContents = {},
            sub1Contents = {},
            sub2Contents = {})

        # Delete /Root/A. 
        # This deletes it and its descendants. All path expressions in 
        # /Root/OtherPrim containing /Root/A are cleaned up to remove 
        # patterns/expression references to objects under /Root/A. 
        # If all components of the path expression were under /Root/A,
        # it leaves an empty path expression (Sdf.PathExpression.Nothing()).
        self.assertTrue(editor.DeletePrimAtPath("/Root/A"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray([
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression.Nothing()]),
                         ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("//Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray([
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression.Nothing()]),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression.Nothing(),
                    }
                }
            }) 

        # Delete /Root/A/B. 
        # All path expressions with components that list prim /Root/A/B or 
        # property /Root/A/B.attr have those paths removed.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression.Nothing(),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray([
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression("/Root/A/C")]),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/C"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression.Nothing(), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Delete /Root/A/C. 
        # All path expressions with components that list  prim /Root/A/C or 
        # property /Root/A/C.attr have those paths removed.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A/C"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression.Nothing())),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression.Nothing(),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

    def test_DeletePropertyWithPathExpressions(self):
        """Test deleting properties. Surviving path expression components that  
        refer to the deleted property are removed."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root/A.attr (spec in sub2). 
        # All path expressions that list the property /Root/A.attr 
        # have that path removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A.attr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/C.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })


        # Delete /Root/A/B.attr (spec in sub1). 
        # All path expressions that list the property /Root/A/B.attr 
        # have that path removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/B.attr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Delete /Root/A/C.attr (spec in root). 
        # All path expressions that list the property /Root/A/C.attr 
        # have that path removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/C.attr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

    def test_RenamePrimWithPathExpressions(self):
        """Test renaming various prims. Path expression prefixes that match
        the renamed prim path (or are descendants) are updated."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Rename /Root/A to Foo
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/Foo" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/Foo//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/Foo/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/Foo/C & /Root/Foo/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/Foo/B//Geom*"), 
                                Sdf.PathExpression("/Root/Foo/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/Foo:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/Foo" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/Foo/B /Root/Foo/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/Foo /Root/Foo/C "
                                    "/Root/Foo.attr /Root/Foo/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/Foo //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/Foo" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/Foo/B /Root/Foo/B.attr "
                                "/Root/Foo/C /Root/Foo/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/Foo/B"), 
                                Sdf.PathExpression("/Root/Foo/C"),
                                Sdf.PathExpression("/Root/Foo"),
                                Sdf.PathExpression(
                                    "/Root/Foo/B.attr /Root/Foo/C.attr /Root/Foo.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/Foo:fromLayer2"),
                    }
                }
            })
        
        # Rename /Root/A/B to Foo
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A/B"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/Foo" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/Foo /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/Foo"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/Foo//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/Foo /Root/A/C"),
                        "/Foo" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/Foo /Root/A/Foo.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/Foo"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/Foo.attr /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Rename /Root/A/C to Foo
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A/C"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/Foo" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/Foo & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/Foo"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/Foo"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/Foo "
                                    "/Root/A.attr /Root/A/Foo.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/Foo /Root/A/Foo.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/Foo"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/Foo.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Rename /Root to Foo
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Foo/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Foo/A/B /Foo/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Foo/A/C & /Foo/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Foo/A/B//Geom*"), 
                                Sdf.PathExpression("/Foo/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Foo/OtherPrim %/Foo/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Foo" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Foo/A/B /Foo/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Foo/A /Foo/A/C "
                                    "/Foo/A.attr /Foo/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Foo/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Foo" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Foo/A/B /Foo/A/B.attr "
                                "/Foo/A/C /Foo/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Foo/A/B"), 
                                Sdf.PathExpression("/Foo/A/C"),
                                Sdf.PathExpression("/Foo/A"),
                                Sdf.PathExpression(
                                    "/Foo/A/B.attr /Foo/A/C.attr /Foo/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Foo/A:fromLayer2"),
                    }
                }
            })

    def test_RenamePropertyWithPathExpressions(self):
        """Test renaming properties. Only property-path references matching
        the renamed property are updated; prim-path references and properties
        on other prims are untouched."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Rename /Root/A.attr to foo (spec in sub2)
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPrimAtPath("/Root/A").GetProperty("attr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.foo /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".foo" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/C.attr /Root/A.foo"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Rename /Root/A/B.attr to foo (spec in sub1)
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPrimAtPath("/Root/A/B").GetProperty("attr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".foo" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.foo "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.foo /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Rename /Root/A/C.attr to foo (spec in root)
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPrimAtPath("/Root/A/C").GetProperty("attr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".foo" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A/C.foo"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A/C.foo"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/C.foo /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

    def test_ReparentPrimWithPathExpressions(self):
        """Test reparenting prims. Path expression prefixes matching the
        moved prim are updated to the new path."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Move /Root/A to /A (reparent to root)
        self.assertTrue(editor.MovePrimAtPath("/Root/A", "/A"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/A" : {
                    ".fromRoot:a" :
                        Sdf.PathExpression("/A//Mesh*"),
                    "/B" : {
                        ".fromRoot:b" :
                            Sdf.PathExpression("/A/B /Root/Other"),
                    },
                    "/C" : {
                        ".attr" : None,
                        ".fromRoot:c" :
                            Sdf.PathExpression("/A/C & /A/B"),
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/A/B//Geom*"), 
                                Sdf.PathExpression("/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/A" : {
                    ".fromLayer1:a" :
                        Sdf.PathExpression("/A/B /A/C"),
                    "/B" : {
                        ".attr" : None,
                        ".fromLayer1:b" :
                            Sdf.PathExpression(
                                "/A /A/C "
                                "/A.attr /A/C.attr"),
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/A" : {
                    ".attr" : None,
                    ".fromLayer2:a" :
                        Sdf.PathExpression(
                            "/A/B /A/B.attr "
                            "/A/C /A/C.attr"),
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/A/B"), 
                                Sdf.PathExpression("/A/C"),
                                Sdf.PathExpression("/A"),
                                Sdf.PathExpression(
                                    "/A/B.attr /A/C.attr /A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A to /Foo (reparent + rename)
        self.assertTrue(editor.MovePrimAtPath("/Root/A", "/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    ".fromRoot:a" :
                        Sdf.PathExpression("/Foo//Mesh*"),
                    "/B" : {
                        ".fromRoot:b" :
                            Sdf.PathExpression("/Foo/B /Root/Other"),
                    },
                    "/C" : {
                        ".attr" : None,
                        ".fromRoot:c" :
                            Sdf.PathExpression("/Foo/C & /Foo/B"),
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Foo/B//Geom*"), 
                                Sdf.PathExpression("/Foo/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Foo:test"),
                    }
                }
            },
            sub1Contents = {
                "/Foo" : {
                    ".fromLayer1:a" :
                        Sdf.PathExpression("/Foo/B /Foo/C"),
                    "/B" : {
                        ".attr" : None,
                        ".fromLayer1:b" :
                            Sdf.PathExpression(
                                "/Foo /Foo/C "
                                "/Foo.attr /Foo/C.attr"),
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Foo //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Foo" : {
                    ".attr" : None,
                    ".fromLayer2:a" :
                        Sdf.PathExpression(
                            "/Foo/B /Foo/B.attr "
                            "/Foo/C /Foo/C.attr"),
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Foo/B"), 
                                Sdf.PathExpression("/Foo/C"),
                                Sdf.PathExpression("/Foo"),
                                Sdf.PathExpression(
                                    "/Foo/B.attr /Foo/C.attr /Foo.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Foo:fromLayer2"),
                    }
                }
            })

        # Move /Root/A/B to /B (reparent child to root)
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/B" : {
                    ".fromRoot:b" :
                        Sdf.PathExpression("/B /Root/Other"),
                },
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/B" : {
                    ".attr" : None,
                    ".fromLayer1:b" :
                        Sdf.PathExpression(
                            "/Root/A /Root/A/C "
                            "/Root/A.attr /Root/A/C.attr"),
                },
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/B /Root/A/C"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/B /B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/B.attr /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A/B to /Root/B (reparent up one level)
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/Root/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/B"),
                        },
                    },
                    "/B" : {
                        ".fromRoot:b" :
                            Sdf.PathExpression("/Root/B /Root/Other"),
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/B /Root/A/C"),
                    },
                    "/B" : {
                        ".attr" : None,
                        ".fromLayer1:b" :
                            Sdf.PathExpression(
                                "/Root/A /Root/A/C "
                                "/Root/A.attr /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/B /Root/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/B.attr /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A/B to a child of /Root/A/C (reparent under sibling)
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/Root/A/C/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/C/B"),
                            "/B" : {
                                ".fromRoot:b" :
                                    Sdf.PathExpression("/Root/A/C/B /Root/Other"),
                            },
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/C/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/C/B /Root/A/C"),
                        "/C" : {
                            "/B" : {
                                ".attr" : None,
                                ".fromLayer1:b" :
                                    Sdf.PathExpression(
                                        "/Root/A /Root/A/C "
                                        "/Root/A.attr /Root/A/C.attr"),
                            },
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/C/B /Root/A/C/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/C/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/C/B.attr /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })
        
        # Move /Root/A/C to be a root level prim while also renaming it to "Foo"
        self.assertTrue(editor.MovePrimAtPath("/Root/A/C", "/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    ".attr" : None,
                    ".fromRoot:c" :
                        Sdf.PathExpression("/Foo & /Root/A/B"),
                },
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Foo"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Foo"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Foo "
                                    "/Root/A.attr /Foo.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Foo /Foo.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Foo"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Foo.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })
        
        # Move /Root/A/C one hierarchy depth level up to be a direct child of 
        # /Root while also renaming it to "Foo".
        self.assertTrue(editor.MovePrimAtPath("/Root/A/C", "/Root/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/Foo" : {
                        ".attr" : None,
                        ".fromRoot:c" :
                            Sdf.PathExpression("/Root/Foo & /Root/A/B"),
                    },
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/Foo"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/Foo"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/Foo "
                                    "/Root/A.attr /Root/Foo.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/Foo /Root/Foo.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/Foo"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/Foo.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })
        
        # Move /Root/A/C to be a child of its sibling /Root/A/B while also 
        # renaming it to "Foo"
        self.assertTrue(editor.MovePrimAtPath("/Root/A/C", "/Root/A/B/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                            "/Foo" : {
                                ".attr" : None,
                                ".fromRoot:c" :
                                    Sdf.PathExpression("/Root/A/B/Foo & /Root/A/B"),
                            },
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/B/Foo"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/B/Foo"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/B/Foo "
                                    "/Root/A.attr /Root/A/B/Foo.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/B/Foo /Root/A/B/Foo.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/B/Foo"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/B/Foo.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

    def test_ReparentPropertyWithPathExpressions(self):
        """Test reparenting properties. Property-path references matching
        the moved property are updated to the new path."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Move /Root/A.attr to /Root.attr (spec in sub2)
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A.attr", "/Root.attr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root.attr /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    ".attr" : None,
                    "/A" : {
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/C.attr /Root.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A/B.attr to /Root.attr (spec in sub1)
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/B.attr", "/Root.attr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    ".attr" : None,
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root.attr "
                                "/Root/A/C /Root/A/C.attr"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root.attr /Root/A/C.attr /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A/C.attr to /Root/A.foo (spec in root)
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/C.attr", "/Root/A.foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        ".foo" : None,
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A.attr /Root/A.foo"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".attr" : None,
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A.foo"),
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A.foo /Root/A.attr"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

        # Move /Root/A.attr to /Root/A/C.foo (spec in sub2)
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A.attr", "/Root/A/C.foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".fromRoot:a" :
                            Sdf.PathExpression("/Root/A//Mesh*"),
                        "/B" : {
                            ".fromRoot:b" :
                                Sdf.PathExpression("/Root/A/B /Root/Other"),
                        },
                        "/C" : {
                            ".attr" : None,
                            ".fromRoot:c" :
                                Sdf.PathExpression("/Root/A/C & /Root/A/B"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromRoot:arr" :
                            Sdf.PathExpressionArray(
                                (Sdf.PathExpression("/Root/A/B//Geom*"), 
                                Sdf.PathExpression("/Root/A/C"))),
                        ".fromRoot:global" :
                            Sdf.PathExpression("//Mesh*"),
                        ".fromRoot:exprRef" :
                            Sdf.PathExpression(
                                "/Root/OtherPrim %/Root/A:test"),
                    }
                }
            },
            sub1Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer1:a" :
                            Sdf.PathExpression("/Root/A/B /Root/A/C"),
                        "/B" : {
                            ".attr" : None,
                            ".fromLayer1:b" :
                                Sdf.PathExpression(
                                    "/Root/A /Root/A/C "
                                    "/Root/A/C.foo /Root/A/C.attr"),
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer1:other" :
                            Sdf.PathExpression("/Root/A //Geom*"),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".fromLayer2:a" :
                            Sdf.PathExpression(
                                "/Root/A/B /Root/A/B.attr "
                                "/Root/A/C /Root/A/C.attr"),
                        "/C" : {
                            ".foo" : None,
                        },
                    },
                    "/OtherPrim" : {
                        ".fromLayer2:arr" :
                            Sdf.PathExpressionArray((
                                Sdf.PathExpression("/Root/A/B"), 
                                Sdf.PathExpression("/Root/A/C"),
                                Sdf.PathExpression("/Root/A"),
                                Sdf.PathExpression(
                                    "/Root/A/B.attr /Root/A/C.attr /Root/A/C.foo"))),
                        ".fromLayer2:global" :
                            Sdf.PathExpression("//B*"),
                        ".fromLayer2:exprRef" :
                            Sdf.PathExpression("%/Root/A:fromLayer2"),
                    }
                }
            })

    def test_PathExpressionsFromCompositionArcs(self):
        """Test path expression fixup in the presence of references.
        Local expressions are fixed up directly; expressions from referenced
        layers cannot be edited and may require relocates."""
        # Setup: Layer to be referenced with prims that have relationship and 
        # attributes with targets and connections. /Model/A has a pathexpression 
        # including /Model/B (which exists). /Model/B's path expression targets 
        # /Model/A (which exists) and /Model/C (which does NOT exist but will 
        # speculatively map to a prim in the root layer that will exist.)
        refLayer = Sdf.Layer.CreateAnonymous("ref.usda")
        refLayer.ImportFromString('''#usda 1.0
            def "Model" 
            {
                def "A"
                {
                    pathExpression expr = "/Model/B"
                }
                
                def "B"
                {
                    pathExpression[] arr = ["/Model/A", "/Model/C"]
                }
            }
        ''')

        # Root layer which references the above. Defines /Root/C which has 
        # a path expression mapping to A and B (brought in by the reference) 
        # and is also the prim that is speculatively included by the path 
        # expression on /Model/B. Also provides an over to A that adds a 
        # pattern for /Root/C.
        rootLayer = Sdf.Layer.CreateAnonymous("root.usda")
        rootLayer.ImportFromString('''#usda 1.0
            def "Root" (
                references = @''' + refLayer.identifier + '''@</Model>
            )
            {
                def "C"
                {
                    pathExpression test = "/Root/A /Root/B"
                }

                over "A"
                {
                    pathExpression expr = "/Root/C %_"
                }
            }
        ''')

        # Create a stage and editor.
        stage = Usd.Stage.Open(rootLayer)
        editor = Usd.NamespaceEditor(stage)

        # Verify initial prims
        self.assertEqual(stage.GetPrimAtPath("/Root").GetChildrenNames(), 
                         ["A", "B", "C"])
        # Verify no relocates
        self.assertEqual(rootLayer.relocates, [])

        # Verify initial composed expressions.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/A.expr').Get(),
            Sdf.PathExpression('/Root/C /Root/B'))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/B.arr').Get(),
                Sdf.PathExpressionArray([
                    Sdf.PathExpression('/Root/A'),
                    Sdf.PathExpression('/Root/C')]))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/C.test').Get(),
            Sdf.PathExpression('/Root/A /Root/B'))

        # Rename /Root/A to /Root/Moved_A.
        self.assertTrue(editor.MovePrimAtPath('/Root/A', '/Root/Moved_A'))

        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertEqual(len(result.warnings), 0)
        self.assertTrue(editor.ApplyEdits())

        # Verify the prim was renamed.
        self.assertEqual(stage.GetPrimAtPath("/Root").GetChildrenNames(), 
                         ["Moved_A", "B", "C"])
        # Verify A was relocated.
        self.assertEqual(rootLayer.relocates, [("/Root/A", "/Root/Moved_A")])

        # Verify the path expressions containing "/Root/A" have been updated.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_A.expr').Get(),
            Sdf.PathExpression('/Root/C /Root/B'))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/B.arr').Get(),
                Sdf.PathExpressionArray([
                    Sdf.PathExpression('/Root/Moved_A'),
                    Sdf.PathExpression('/Root/C')]))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/C.test').Get(),
            Sdf.PathExpression('/Root/Moved_A /Root/B'))
    
        # Rename /Root/B to /Root/Moved_B.
        self.assertTrue(editor.MovePrimAtPath('/Root/B', '/Root/Moved_B'))

        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertEqual(len(result.warnings), 0)
        self.assertTrue(editor.ApplyEdits())

        # Verify that B was renamed.
        self.assertEqual(stage.GetPrimAtPath("/Root").GetChildrenNames(), 
                         ["Moved_A", "Moved_B", "C"])
        # Verify B is now also relocated.
        self.assertEqual(rootLayer.relocates, [
            ("/Root/A", "/Root/Moved_A"),
            ("/Root/B", "/Root/Moved_B")])

        # Verify the path expressions containing "/Root/B" have been updated.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_A.expr').Get(),
            Sdf.PathExpression('/Root/C /Root/Moved_B'))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_B.arr').Get(),
                Sdf.PathExpressionArray([
                    Sdf.PathExpression('/Root/Moved_A'),
                    Sdf.PathExpression('/Root/C')]))
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/C.test').Get(),
            Sdf.PathExpression('/Root/Moved_A /Root/Moved_B'))
        
        # Rename /Root/C to /Root/Moved_C.
        self.assertTrue(editor.MovePrimAtPath('/Root/C', '/Root/Moved_C'))

        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertEqual(len(result.warnings), 1)
        self.assertTrue(editor.ApplyEdits())

        # Verify that C was renamed.
        self.assertEqual(stage.GetPrimAtPath("/Root").GetChildrenNames(), 
                         ["Moved_A", "Moved_B", "Moved_C"])
        # Verify that the relocates haven't changed as moving /Root/C does not
        # require relocates.
        self.assertEqual(rootLayer.relocates, [
            ("/Root/A", "/Root/Moved_A"),
            ("/Root/B", "/Root/Moved_B")])

        # /Root/Moved_A properties have been updated to target Moved_C because
        # the opinions that target C are in the root layer and can be edited.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_A.expr').Get(),
            Sdf.PathExpression('/Root/Moved_C /Root/Moved_B'))
        
        # /Root/Moved_B properties have NOT been updated to target Moved_C 
        # because these opinions are in the reference and /Root/C cannot be
        # relocated as it has no opinions across the reference itself.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_B.arr').Get(),
            Sdf.PathExpressionArray([
                Sdf.PathExpression('/Root/Moved_A'),
                Sdf.PathExpression('/Root/C')]))
        
        # Moved_C has the same targets as before the rename.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_C.test').Get(),
            Sdf.PathExpression('/Root/Moved_A /Root/Moved_B'))
        
        # Delete /Root/Moved_A
        self.assertTrue(editor.DeletePrimAtPath("/Root/Moved_A"))
        self.assertEqual(len(editor.CanApplyEdits().warnings), 0)
        self.assertTrue(editor.ApplyEdits())

        # Verify Moved_A is no longer a prim
        self.assertEqual(stage.GetPrimAtPath("/Root").GetChildrenNames(), 
                         ["Moved_B", "Moved_C"])
        # Verify the relocates have been updated so /Root/A now maps to empty
        self.assertEqual(rootLayer.relocates, [
            ("/Root/A", Sdf.Path()), 
            ("/Root/B", "/Root/Moved_B")])
        
        # /Root/Moved_B properties have been updated but instead of 
        # /Root/Moved_A being removed from its targets, it has returned to 
        # using the unrelocated path of /Root/A. This is because Pcp doesn't 
        # removed composed targets that have been relocated.
        # 
        # XXX: This really should remove the deleted targets to /Root/A but we
        # don't currently add the mapping to empty in the map function for 
        # /Root/A. This is because the PcpTargetIndex computation uses the
        # map function to map the target path and it will produce a composition
        # error if the path is not mappable. An unmappable target path that is
        # due to a "relocate to delete" should not be an error, but any other 
        # case where it's not mappable is an error. Unfortunately, we don't have
        # an easy way of distinguishing between these two cases in the target
        # index so for now we have to let deleted prims map to avoid false 
        # errors in the presence of a relocate to delete.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_B.arr').Get(),
            Sdf.PathExpressionArray([
                    Sdf.PathExpression('/Root/A'),
                    Sdf.PathExpression('/Root/C')]))

        # /Root/Moved_C, on the other hand, does have targets to Moved_A removed
        # because the opinions are local to the root layer.
        self.assertEqual(
            stage.GetAttributeAtPath(
                '/Root/Moved_C.test').Get(),
            Sdf.PathExpression('/Root/Moved_B'))

    def test_NonDefaultPrims(self):
        """Verify that path expression fixup works on undefined (over),
        abstract (class), inactive, and unloaded prims."""

        def _EditAndVerifyNonDefaultPrims(layer, stage):
            editor = Usd.NamespaceEditor(stage)
            self.assertTrue(editor.MovePrimAtPath('/Root/PrimA', '/Foo'))
            result = editor.CanApplyEdits()
            self.assertTrue(result)
            self.assertTrue(editor.ApplyEdits())

            self._VerifyLayerContents(layer, {
                "/Foo" : {
                    ".test" :
                        Sdf.PathExpression("/Root/PrimB"),
                },
                "/Root" : {
                    "/PrimB" : {
                        ".test" :
                            Sdf.PathExpression("/Foo"),
                    },
                },
            })

        undefinedLayer = Sdf.Layer.CreateAnonymous()
        undefinedLayer.ImportFromString('''#usda 1.0
        over "Root"
        {
            over "PrimA"
            {
                pathExpression test = "/Root/PrimB"
            }

            over "PrimB"
            {
                pathExpression test = "/Root/PrimA"
            }
        }
        ''')
        _EditAndVerifyNonDefaultPrims(undefinedLayer, Usd.Stage.Open(undefinedLayer))

        abstractLayer = Sdf.Layer.CreateAnonymous()
        abstractLayer.ImportFromString('''#usda 1.0
        class "Root"
        {
            class "PrimA"
            {
                pathExpression test = "/Root/PrimB"
            }

            class "PrimB"
            {
                pathExpression test = "/Root/PrimA"
            }
        }
        ''')
        _EditAndVerifyNonDefaultPrims(abstractLayer, Usd.Stage.Open(abstractLayer))
        
        inactiveLayer = Sdf.Layer.CreateAnonymous()
        inactiveLayer.ImportFromString('''#usda 1.0
        def "Root"
        {
            def "PrimA"
            {
                pathExpression test = "/Root/PrimB"
            }

            def "PrimB"
            {
                pathExpression test = "/Root/PrimA"
            }
        }
        ''')
        
        inactiveStage = Usd.Stage.Open(inactiveLayer)
        inactiveStage.GetPrimAtPath("/Root/PrimA").SetActive(False)
        inactiveStage.GetPrimAtPath("/Root/PrimB").SetActive(False)
        
        _EditAndVerifyNonDefaultPrims(inactiveLayer, inactiveStage)

        # A prim with an unloaded payload (/Root/PrimB) has a path expression
        # containing a prefix that has changed path (/Root/PrimA -> /Foo). Verify
        # that the path expression is updated correctly.
        payloadLayer = Sdf.Layer.CreateAnonymous()
        payloadLayer.ImportFromString('''#usda 1.0
        def "Payload"
        {
        }
        ''')

        unloadedLayer = Sdf.Layer.CreateAnonymous("abstract.usda")
        unloadedLayer.ImportFromString('''#usda 1.0
        def "Root" 
        {
            def "PrimA"
            {
                pathExpression test = "/Root/PrimB"
            }

            def "PrimB" (
                payload = @''' + payloadLayer.identifier + '''@</Payload>
            )
            { 
                pathExpression test = "/Root/PrimA"
            }
        }
        ''')

        unloadedStage = Usd.Stage.Open(unloadedLayer)
        bPrim = unloadedStage.GetPrimAtPath("/Root/PrimB")
        self.assertTrue(bPrim.HasPayload())
        bPrim.Unload()

        _EditAndVerifyNonDefaultPrims(unloadedLayer, unloadedStage)    

    def test_AllSyntaxForms(self):
        """Verify that ReplacePrefix only modifies the prefix portion of
        every path expression syntax form, even when the renamed prim name
        appears in non-prefix positions."""
        layer = Sdf.Layer.CreateAnonymous()
        Sdf.CreatePrimInLayer(layer, '/Root')
        Sdf.CreatePrimInLayer(layer, '/Root/A')
        Sdf.CreatePrimInLayer(layer, '/Root/A/A')
        holderSpec = Sdf.CreatePrimInLayer(layer, '/Root/Holder')

        cases = {
            'exact': '/Root/A',
            'descendant': '/Root/A/A',
            'stretch': '/Root/A//A',
            'wildcard': '/Root/A/A*',
            'stretchWild': '/Root/A//*A*',
            'predicate': '/Root/A//{A}',
            'exprRef': '%/Root/A:A',
            'complement': '~/Root/A//A',
            'union': '/Root/A /Root/A//A',
            'intersection': '/Root/A/A* & /Root/A//A',
            'difference': '/Root/A - /Root/A//A',
            'noMatch': '/Root/Other/A',
            'contextIndep': '//A',
        }
        for name, expr in cases.items():
            attr = Sdf.AttributeSpec(
                holderSpec, name, Sdf.ValueTypeNames.PathExpression)
            attr.default = Sdf.PathExpression(expr)

        stage = Usd.Stage.Open(layer)
        editor = Usd.NamespaceEditor(stage)
        self.assertTrue(editor.MovePrimAtPath('/Root/A', '/Root/B'))
        result = editor.CanApplyEdits()
        self.assertTrue(result)
        self.assertTrue(editor.ApplyEdits())

        expected = {
            'exact': '/Root/B',
            'descendant': '/Root/B/A',
            'stretch': '/Root/B//A',
            'wildcard': '/Root/B/A*',
            'stretchWild': '/Root/B//*A*',
            'predicate': '/Root/B//{A}',
            'exprRef': '%/Root/B:A',
            'complement': '~/Root/B//A',
            'union': '/Root/B /Root/B//A',
            'intersection': '/Root/B/A* & /Root/B//A',
            'difference': '/Root/B - /Root/B//A',
            'noMatch': '/Root/Other/A',
            'contextIndep': '//A',
        }
        for name, expectedExpr in expected.items():
            prop = layer.GetPropertyAtPath('/Root/Holder.' + name)
            self.assertEqual(prop.default, Sdf.PathExpression(expectedExpr),
                "Attribute '{}': expected '{}', got '{}'".format(
                    name, expectedExpr,
                    prop.default.GetText() if prop.default else "None"))

    # The test methods below mirror the test_*WithPathExpressions methods
    # above but verify path-expression-valued customData *fields* on prim
    # specs and on property specs, rather than path-expression-valued
    # *attribute default values*. The fixture authors customData with
    # pathExpression entries on /Root/A and on the fromRoot:a/fromLayer1:a/
    # fromLayer2:a properties; see _OpenBasicStage for the baseline.

    def test_DeletePrimWithPathExpressionFields(self):
        """Test deleting prims. Path expression-valued customData fields on
        surviving prims and properties have references to the deleted path
        and its descendants removed."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root/A/B. The single-path entries that reference /Root/A/B
        # or its child property /Root/A/B.attr collapse to Nothing(); array
        # and union entries drop just the matching path.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A/B"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A", {
            "rootPrimExpr" : Sdf.PathExpression("/Root/A//Mesh*"),
            "rootPrimArr" : Sdf.PathExpressionArray((
                Sdf.PathExpression.Nothing(),
                Sdf.PathExpression("/Root/A/C"))),
        })
        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/A/C.attr"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A", {
            "layer1PrimExpr" : Sdf.PathExpression.Nothing(),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A.fromLayer1:a", {
            "layer1PropExpr" : Sdf.PathExpression("/Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A", {
            "layer2PrimExpr" : Sdf.PathExpression("/Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression.Nothing(),
        })

    def test_DeletePropertyWithPathExpressionFields(self):
        """Test deleting properties. Path expression-valued customData fields
        have references to the deleted property removed."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root/A/C.attr (spec in root). Only rootPropExpr references
        # this property; everything else is unchanged.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/C.attr"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/A/B"),
        })
        # Prim-level customData on /Root/A is unaffected by deleting a
        # property on a sibling prim.
        self._VerifyCustomData(self.rootLayer, "/Root/A", {
            "rootPrimExpr" : Sdf.PathExpression("/Root/A//Mesh*"),
            "rootPrimArr" : Sdf.PathExpressionArray((
                Sdf.PathExpression("/Root/A/B"),
                Sdf.PathExpression("/Root/A/C"))),
        })
        self.stage.Reload()

        # Delete /Root/A/B.attr (spec in sub1). Only layer2PropExpr references
        # it, collapsing to Nothing().
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/B.attr"))
        self._Apply(editor)

        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression.Nothing(),
        })

    def test_RenamePrimWithPathExpressionFields(self):
        """Test renaming prims. Path expression-valued customData fields have
        their path prefixes updated to reflect the new prim name."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Rename /Root/A/B to Foo.
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A/B"), "Foo"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A", {
            "rootPrimExpr" : Sdf.PathExpression("/Root/A//Mesh*"),
            "rootPrimArr" : Sdf.PathExpressionArray((
                Sdf.PathExpression("/Root/A/Foo"),
                Sdf.PathExpression("/Root/A/C"))),
        })
        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/A/Foo /Root/A/C.attr"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A", {
            "layer1PrimExpr" : Sdf.PathExpression("/Root/A/Foo"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A.fromLayer1:a", {
            "layer1PropExpr" : Sdf.PathExpression("/Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A", {
            "layer2PrimExpr" : Sdf.PathExpression("/Root/A/Foo /Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression("/Root/A/Foo.attr"),
        })

    def test_RenamePropertyWithPathExpressionFields(self):
        """Test renaming properties. Path expression-valued customData fields
        have their property path components updated to the new property
        name."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Rename /Root/A/C.attr to renamedAttr.
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPrimAtPath("/Root/A/C").GetProperty("attr"),
            "renamedAttr"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" :
                Sdf.PathExpression("/Root/A/B /Root/A/C.renamedAttr"),
        })
        self.stage.Reload()

        # Rename /Root/A/B.attr to fooAttr (spec in sub1).
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPrimAtPath("/Root/A/B").GetProperty("attr"),
            "fooAttr"))
        self._Apply(editor)

        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression("/Root/A/B.fooAttr"),
        })

    def test_ReparentPrimWithPathExpressionFields(self):
        """Test reparenting prims. Path expression-valued customData fields
        have their path prefixes updated to reflect the new prim location."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Move /Root/A/B to /Root/B (sibling of /Root/A).
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/Root/B"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A", {
            "rootPrimExpr" : Sdf.PathExpression("/Root/A//Mesh*"),
            "rootPrimArr" : Sdf.PathExpressionArray((
                Sdf.PathExpression("/Root/B"),
                Sdf.PathExpression("/Root/A/C"))),
        })
        
        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/B /Root/A/C.attr"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A", {
            "layer1PrimExpr" : Sdf.PathExpression("/Root/B"),
        })
        self._VerifyCustomData(self.sub1Layer, "/Root/A.fromLayer1:a", {
            "layer1PropExpr" : Sdf.PathExpression("/Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A", {
            "layer2PrimExpr" : Sdf.PathExpression("/Root/B /Root/A/C"),
        })
        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression("/Root/B.attr"),
        })

    def test_ReparentPropertyWithPathExpressionFields(self):
        """Test reparenting properties. Path expression-valued customData
        fields have their property path components updated to the new
        location."""
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Move /Root/A/C.attr to /Root/A.movedAttr.
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/C.attr", "/Root/A.movedAttr"))
        self._Apply(editor)

        self._VerifyCustomData(self.rootLayer, "/Root/A.fromRoot:a", {
            "rootPropExpr" : Sdf.PathExpression("/Root/A/B /Root/A.movedAttr"),
        })
        self.stage.Reload()

        # Move /Root/A/B.attr to /Root.movedAttr.
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/B.attr", "/Root.movedAttr"))
        self._Apply(editor)

        self._VerifyCustomData(self.sub2Layer, "/Root/A.fromLayer2:a", {
            "layer2PropExpr" : Sdf.PathExpression("/Root.movedAttr"),
        })


if __name__ == '__main__':
    unittest.main()
