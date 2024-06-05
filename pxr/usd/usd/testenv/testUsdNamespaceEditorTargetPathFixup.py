#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, os, unittest
from pxr import Sdf, Usd, Tf, Plug

class TestUsdNamespaceEditorTargetPathFixup(unittest.TestCase):

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
    # These expected contentss indicate that a layer is expected to have a root 
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
    def _VerifyLayerContents(self, layer, expectedContentsDict) :
        # Verify the expected contents dictionary is a valid format first for
        # aid in debugging test failures.
        self._VerifyExpectedLayerContentsFormat(expectedContentsDict)

        # Verify the expected number of root prims
        self.assertEqual(len(expectedContentsDict), len(layer.rootPrims),
            "The expected number of root prims in {} doesn't match the number "
            "of root prims {} on the layer".format(
                list(expectedContentsDict.keys()), list(layer.rootPrims.keys()))) 

        # Verify the expected contents of eache root prim.
        for rootPrimPath, expectedChild in expectedContentsDict.items():
            self._VerifyPrimContents(layer, rootPrimPath, expectedChild)

    # This is a helper for writing and updating the test. This just makes sure
    # the expectedContents dictionary is in the correct format and can be called
    # before using an expectedContents dictionary to actually verify any layer 
    # contents. Makes it easier to distinguish between test writing error and 
    # test failures.
    def _VerifyExpectedLayerContentsFormat(self, expectedContents, pathKey=None):
        self.assertTrue(isinstance(expectedContents, dict), 
            "Expected contents for prim key '{}' is not a dictionary".format(
                pathKey or "None"))
        for k, v in expectedContents.items():
            if k.startswith('.'):
                self.assertTrue(isinstance(v, Sdf.PathListOp), 
                    "Value '{}' for property key '{}' is invalid; it must be an "
                    "Sdf.PathListOp".format(str(v), k))
            elif k.startswith('/'):
                self._VerifyExpectedLayerContentsFormat(v, k)
            else:
                self.assertTrue(False, "Invalid expected contents dictionary "
                    "key '{}'; it must start with '/' for prim children or '.' "
                    "for property children".format(k))

    # Verifies the contents of a prim in the layer has the expected contents
    # indicated by the expectedContentsDict
    def _VerifyPrimContents(self, layer, path, expectedContentsDict):
        # Prim must exist
        prim = layer.GetPrimAtPath(path)
        self.assertTrue(prim, "Expected to find prim at path {}".format(path))

        # Prim must have the same number of prim and property children as 
        # indicated in the expected contents.
        self.assertEqual(len(expectedContentsDict), 
            len(prim.properties) + len(prim.nameChildren),
            "The expected number of prims and properties in {} doesn't match "
            "the combined number of child properties {} and prims {} on the "
            "prim at {}".format(
                list(expectedContentsDict.keys()), list(prim.properties.keys()),
                list(prim.nameChildren.keys()), path)) 

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
                self._VerifyPrimContents(layer, childPath, expectedChildContent)

    # Verifies the contents of a property in the layer has the expected 
    # connections or relationship targets listOp value.
    def _VerifyPropertyContents(self, layer, path, expectedListOpValue):
        # Property must exist
        prop = layer.GetPropertyAtPath(path)
        self.assertTrue(prop, 
            "Expected to find property at path {}".format(path))
        
        if isinstance(prop, Sdf.AttributeSpec):
            # Is attribute, connectionPaths field must match listOp
            listOpValue = prop.GetInfo(Sdf.AttributeSpec.ConnectionPathsKey)
            self.assertEqual(listOpValue, expectedListOpValue,
                "Attribute at {} has 'connectionsPaths' value '{}' which does "
                "not match the expected value '{}'".format(
                    path, listOpValue, expectedListOpValue))
        elif isinstance(prop, Sdf.RelationshipSpec):
            # Is relationship, targets field must match listOp
            listOpValue = prop.GetInfo(Sdf.RelationshipSpec.TargetsKey)
            self.assertEqual(listOpValue, expectedListOpValue,
                "Relationship at {} has 'target' value '{}' which does "
                "not match the expected value '{}'".format(
                    path, listOpValue, expectedListOpValue))
        else:
            # Invalid property type, always fail.
            self.assertTrue(false, 
                "Property at {} is not a valid property type".format(path))

    # Opens the basic stage and verifies its initial contents.
    def _OpenBasicStage(self):
        self.stage = Usd.Stage.Open("basic/root.usda")
        self.stage.Reload()
        stageLayers = self.stage.GetUsedLayers()

        # The stage has three layers whose contents we'll verify. Store each
        # of these layers and verify their original contents.

        # Root layer
        self.rootLayer = Sdf.Find("basic/root.usda")
        self.assertTrue(self.rootLayer)
        self.assertTrue(self.rootLayer in stageLayers)
        self._VerifyLayerContents(self.rootLayer, {
            "/Root" : {
                "/A" : {
                    ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Root/A.targetAttr"]),
                    "/B" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A/B.targetAttr"])
                    },                       
                    "/C" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A/C.targetAttr"]),
                        ".targetAttr" : Sdf.PathListOp.Create(
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A.targetAttr"])
                    },
                },
                "/OtherPrim" : {
                    ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Root/A.targetAttr"]),
                    ".otherRel" : Sdf.PathListOp.Create(
                        deletedItems = [
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr"],
                        prependedItems = ["/Root/A"],
                        appendedItems = [
                            "/Root/A/B",
                            "/Root/A/C"
                        ])
                }
            }
        })
        
        # First sublayer
        self.sub1Layer = Sdf.Find("basic/sub1.usda")
        self.assertTrue(self.sub1Layer)
        self.assertTrue(self.sub1Layer in stageLayers)
        self._VerifyLayerContents(self.sub1Layer, {
            "/Root" : {
                "/A" : {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        prependedItems = ["/Root/A.targetAttr"]),
                    "/B" : {
                        ".targetAttr" : Sdf.PathListOp.Create(
                            prependedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/C.targetAttr"])
                    }
                },
                "/OtherPrim" :  {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        deletedItems = ["/Root/A.targetAttr"],
                        prependedItems = ["/Root/A/B.targetAttr"],
                        appendedItems = ["/Root/A/C.targetAttr"]),
                    ".otherRel" : Sdf.PathListOp.Create(
                        deletedItems = [
                            "/Root/A",
                            "/Root/A/B",
                            "/Root/A/C"],
                        prependedItems = ["/Root/A.targetAttr"],
                        appendedItems = [
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                }
            }
        })

        # Second sublayer
        self.sub2Layer = Sdf.Find("basic/sub2.usda")
        self.assertTrue(self.sub2Layer)
        self.assertTrue(self.sub2Layer in stageLayers)
        self._VerifyLayerContents(self.sub2Layer, {
            "/Root" : {
                "/A" : {
                    ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Root/A/B.targetAttr",
                        "/Root/A/C.targetAttr"])
                },
                "/OtherPrim" : {
                    ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Root/A.targetAttr",
                        "/Root/A/B.targetAttr",
                        "/Root/A/C.targetAttr"]),
                    ".otherRel" : Sdf.PathListOp.Create(
                        deletedItems = ["/Root/A"],
                        prependedItems = ["/Root/A/B"],
                        appendedItems = ["/Root/A/C"]),
                }
            }
        })
   
    def _ApplyCompareAndReset(self, editor, rootContents, sub1Contents, sub2Contents):
        self.assertTrue(editor.CanApplyEdits())
        self.assertTrue(editor.ApplyEdits())
        self._VerifyLayerContents(self.rootLayer, rootContents)
        self._VerifyLayerContents(self.sub1Layer, sub1Contents)
        self._VerifyLayerContents(self.sub2Layer, sub2Contents)
        self.stage.Reload()

    def test_DeletePrimWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root. This is the only root prim so it deletes everything.
        # This is mostly a sanity check that deleting everything at the root
        # doesn't cause any issues with respect to found target path 
        # dependencies
        self.assertTrue(editor.DeletePrimAtPath("/Root"))
        self._ApplyCompareAndReset(editor,
            rootContents = {},
            sub1Contents = {},
            sub2Contents = {})

        # Delete /Root/A. This deletes it and its descendants. All connections
        # and relationship target listOps in /Root/OtherPrim are cleaned up to 
        # remove targets to objects under /Root/A. Note all listOp items are 
        # removed (including listOp deletes) and that fully cleared explicit 
        # list ops become an explicit empty listOp (i.e. None) while 
        # non-explicit list ops end up with no opinion.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([]),
                        ".otherRel" : Sdf.PathListOp()
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp(),
                        ".otherRel" : Sdf.PathListOp(),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([]),
                        ".otherRel" : Sdf.PathListOp(),
                    }
                }
            })

        # Delete /Root/A/B. All connections and relationship target listOps that
        # list prim /Root/A/B or property /Root/A/B.targetAttr have those paths
        # removed.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = ["/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = ["/Root/A/C"])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Delete /Root/A/C. All connections and relationship target listOps that
        # list prim /Root/A/C or property /Root/A/C.targetAttr have those paths
        # removed.
        self.assertTrue(editor.DeletePrimAtPath("/Root/A/C"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = ["/Root/A/B"])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/B.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"]),
                    }
                }
            })

    def test_DeletePropertyWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Delete /Root/A.targetAttr. All connections and relationship target 
        # listOps that list the property /Root/A.targetAttr have that path
        # removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A.targetAttr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = ["/Root/A/B.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp(),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {},
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })
            
        # Delete /Root/A/B.targetAttr. All connections and relationship target 
        # listOps that list the property /Root/A/B.targetAttr have that path
        # removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/B.targetAttr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create()
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = ["/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {}
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })
            
        # Delete /Root/A/C.targetAttr. All connections and relationship target 
        # listOps that list the property /Root/A/C.targetAttr have that path
        # removed.
        self.assertTrue(editor.DeletePropertyAtPath("/Root/A/C.targetAttr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create()
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = ["/Root/A/B.targetAttr",]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

    def test_RenamePrimWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Test renaming various prims on the stage. Any connection or 
        # relationship target paths that refer to the prim or any of its 
        # descendants will be fixed to use the path with the rename applied.

        # Rename /Root/A
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/Foo" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/Foo.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/Foo/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/Foo/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/Foo/B.targetAttr",
                                    "/Root/Foo.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/Foo.targetAttr",
                                "/Root/Foo/B.targetAttr"],
                            prependedItems = ["/Root/Foo"],
                            appendedItems = [
                                "/Root/Foo/B",
                                "/Root/Foo/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/Foo" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/Foo.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/Foo.targetAttr",
                                    "/Root/Foo/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/Foo.targetAttr"],
                            prependedItems = ["/Root/Foo/B.targetAttr"],
                            appendedItems = ["/Root/Foo/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/Foo",
                                "/Root/Foo/B",
                                "/Root/Foo/C"],
                            prependedItems = ["/Root/Foo.targetAttr"],
                            appendedItems = [
                                "/Root/Foo/B.targetAttr",
                                "/Root/Foo/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/Foo" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/Foo/B.targetAttr",
                            "/Root/Foo/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/Foo.targetAttr",
                            "/Root/Foo/B.targetAttr",
                            "/Root/Foo/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/Foo"],
                            prependedItems = ["/Root/Foo/B"],
                            appendedItems = ["/Root/Foo/C"]),
                    }
                }
            })
            
        # Rename /Root/A/B
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A/B"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/Foo" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/Foo.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/Foo.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/Foo.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/Foo",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/Foo" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/Foo.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/Foo",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/Foo.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/Foo.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/Foo.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/Foo"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })
            
        # Rename /Root/A/C
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root/A/C"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/Foo" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/Foo.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/Foo"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A/Foo.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/Foo"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/Foo.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A/Foo.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/A/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/Foo"]),
                    }
                }
            })
            
        # Rename /Root
        self.assertTrue(editor.RenamePrim(
            self.stage.GetPrimAtPath("/Root"), "Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Foo/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Foo/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Foo/A/B.targetAttr",
                                    "/Foo/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Foo/A.targetAttr",
                                "/Foo/A/B.targetAttr"],
                            prependedItems = ["/Foo/A"],
                            appendedItems = [
                                "/Foo/A/B",
                                "/Foo/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Foo" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Foo/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Foo/A.targetAttr",
                                    "/Foo/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Foo/A.targetAttr"],
                            prependedItems = ["/Foo/A/B.targetAttr"],
                            appendedItems = ["/Foo/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Foo/A",
                                "/Foo/A/B",
                                "/Foo/A/C"],
                            prependedItems = ["/Foo/A.targetAttr"],
                            appendedItems = [
                                "/Foo/A/B.targetAttr",
                                "/Foo/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Foo" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo/A/B.targetAttr",
                            "/Foo/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo/A.targetAttr",
                            "/Foo/A/B.targetAttr",
                            "/Foo/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Foo/A"],
                            prependedItems = ["/Foo/A/B"],
                            appendedItems = ["/Foo/A/C"]),
                    }
                }
            })
            
    def test_RenamePropertyWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Test renaming various properties on the stage. Any connection or 
        # relationship target paths that refer to the property will be fixed to 
        # use the new property name

        # Rename /Root/A.targetAttr
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPropertyAtPath("/Root/A.targetAttr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.foo"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.targetAttr",
                                    "/Root/A.foo"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.foo",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.foo"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.foo",
                                    "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.foo"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.foo"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".foo" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.foo",
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Rename /Root/A/B.targetAttr
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPropertyAtPath("/Root/A/B.targetAttr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.foo"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.foo",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.foo"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".foo" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.foo"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.foo",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.foo",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.foo",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Rename /Root/A/C.targetAttr
        self.assertTrue(editor.RenameProperty(
            self.stage.GetPropertyAtPath("/Root/A/C.targetAttr"), "foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.foo"]),
                            ".foo" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A/C.foo"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/C.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.foo"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.foo"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

    def test_ReparentPrimWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Test moving various prims on the stage to new parent prims. Any 
        # connection or relationship target paths that refer to the prim or any
        # of its descendants will be fixed to use the prim's new path within
        # the namespace hierarchy.

        # Move /Root/A to be a root level prim /A
        self.assertTrue(editor.MovePrimAtPath("/Root/A", "/A"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/A" : {
                    ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                        "/A.targetAttr"]),
                    "/B" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/A/B.targetAttr"])
                    },                       
                    "/C" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/A/C.targetAttr"]),
                        ".targetAttr" : Sdf.PathListOp.Create(
                            appendedItems = [
                                "/A/B.targetAttr",
                                "/A.targetAttr"])
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/A.targetAttr",
                                "/A/B.targetAttr"],
                            prependedItems = ["/A"],
                            appendedItems = [
                                "/A/B",
                                "/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/A" : {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        prependedItems = ["/A.targetAttr"]),
                    "/B" : {
                        ".targetAttr" : Sdf.PathListOp.Create(
                            prependedItems = [
                                "/A.targetAttr",
                                "/A/C.targetAttr"])
                    }
                },
                "/Root" : {
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/A.targetAttr"],
                            prependedItems = ["/A/B.targetAttr"],
                            appendedItems = ["/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/A",
                                "/A/B",
                                "/A/C"],
                            prependedItems = ["/A.targetAttr"],
                            appendedItems = [
                                "/A/B.targetAttr",
                                "/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/A" : {
                    ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                        "/A/B.targetAttr",
                        "/A/C.targetAttr"])
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/A.targetAttr",
                            "/A/B.targetAttr",
                            "/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/A"],
                            prependedItems = ["/A/B"],
                            appendedItems = ["/A/C"]),
                    }
                }
            })

        # Move /Root/A to be a root level prim while also renaming it to "Foo"
        self.assertTrue(editor.MovePrimAtPath("/Root/A", "/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Foo.targetAttr"]),
                    "/B" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Foo/B.targetAttr"])
                    },                       
                    "/C" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Foo/C.targetAttr"]),
                        ".targetAttr" : Sdf.PathListOp.Create(
                            appendedItems = [
                                "/Foo/B.targetAttr",
                                "/Foo.targetAttr"])
                    },
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Foo.targetAttr",
                                "/Foo/B.targetAttr"],
                            prependedItems = ["/Foo"],
                            appendedItems = [
                                "/Foo/B",
                                "/Foo/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Foo" : {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        prependedItems = ["/Foo.targetAttr"]),
                    "/B" : {
                        ".targetAttr" : Sdf.PathListOp.Create(
                            prependedItems = [
                                "/Foo.targetAttr",
                                "/Foo/C.targetAttr"])
                    }
                },
                "/Root" : {
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Foo.targetAttr"],
                            prependedItems = ["/Foo/B.targetAttr"],
                            appendedItems = ["/Foo/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Foo",
                                "/Foo/B",
                                "/Foo/C"],
                            prependedItems = ["/Foo.targetAttr"],
                            appendedItems = [
                                "/Foo/B.targetAttr",
                                "/Foo/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Foo" : {
                    ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Foo/B.targetAttr",
                        "/Foo/C.targetAttr"])
                },
                "/Root" : {
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Foo.targetAttr",
                            "/Foo/B.targetAttr",
                            "/Foo/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Foo"],
                            prependedItems = ["/Foo/B"],
                            appendedItems = ["/Foo/C"]),
                    }
                }
            })

        # Move /Root/A/B to be a root level prim /B
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/B" : {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        prependedItems = ["/B.targetAttr"])
                },                       
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/B.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/B" : {
                    ".targetAttr" : Sdf.PathListOp.Create(
                        prependedItems = [
                            "/Root/A.targetAttr",
                            "/Root/A/C.targetAttr"])
                },
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),

                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/B.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })
            
        # Move /Root/A/B one hierarchy depth level up to be a direct child of 
        # /Root.
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/Root/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/B" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/B.targetAttr"])
                    },                       
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/B.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/B" : {
                        ".targetAttr" : Sdf.PathListOp.Create(
                            prependedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/C.targetAttr"])
                    },
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/B.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Move /Root/A/B to be a child of its sibling /Root/A/C
        self.assertTrue(editor.MovePrimAtPath("/Root/A/B", "/Root/A/C/B"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/C/B.targetAttr",
                                    "/Root/A.targetAttr"]),
                            "/B" : {
                                ".otherAttr" : Sdf.PathListOp.Create(
                                    prependedItems = ["/Root/A/C/B.targetAttr"])
                            },
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/C/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/C/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/C" : {
                            "/B" : {
                                ".targetAttr" : Sdf.PathListOp.Create(
                                    prependedItems = [
                                        "/Root/A.targetAttr",
                                        "/Root/A/C.targetAttr"])
                            }
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/C/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/C/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/C/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C/B.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/C/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/C/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Move /Root/A/C to be a root level prim while also renaming it to "Foo"
        self.assertTrue(editor.MovePrimAtPath("/Root/A/C", "/Foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Foo" : {
                    ".otherAttr" : Sdf.PathListOp.Create(
                        prependedItems = ["/Foo.targetAttr"]),
                    ".targetAttr" : Sdf.PathListOp.Create(
                        appendedItems = [
                            "/Root/A/B.targetAttr",
                            "/Root/A.targetAttr"])
                },
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Foo"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Foo.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Foo"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Foo.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Foo.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Foo"]),
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
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/Foo.targetAttr"]),
                        ".targetAttr" : Sdf.PathListOp.Create(
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A.targetAttr"])
                    },
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/Foo"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/Foo.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/Foo"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/Foo.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/Foo.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/Foo"]),
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
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"]),
                            "/Foo" : {
                                ".otherAttr" : Sdf.PathListOp.Create(
                                    prependedItems = [
                                        "/Root/A/B/Foo.targetAttr"]),
                                ".targetAttr" : Sdf.PathListOp.Create(
                                    appendedItems = [
                                        "/Root/A/B.targetAttr",
                                        "/Root/A.targetAttr"])
                            },
                        },                       
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/B/Foo"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A/B/Foo.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/B/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/B/Foo"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/B/Foo.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A/B/Foo.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/A/B/Foo.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/B/Foo"]),
                    }
                }
            })

    def test_ReparentPropertyWithTargets(self):
        self._OpenBasicStage()
        editor = Usd.NamespaceEditor(self.stage)

        # Test moving various properties on the stage to new parent prims. Any 
        # connection or relationship target paths that refer to the property 
        # will be fixed to use the property's new path within the namespace 
        # hierarchy.

        # Move targetAttr from /Root/A to be a property of /Root instead
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A.targetAttr", "/Root.targetAttr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.targetAttr",
                                    "/Root.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root.targetAttr",
                                    "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                        "/Root/A/B.targetAttr",
                        "/Root/A/C.targetAttr"]),
                    "/A" : {},
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Move targetAttr from /Root/A/B to be a property of /Root instead
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/B.targetAttr", "/Root.targetAttr"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root.targetAttr",
                                    "/Root/A.targetAttr"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    ".targetAttr" : Sdf.PathListOp.Create(
                        prependedItems = [
                            "/Root/A.targetAttr",
                            "/Root/A/C.targetAttr"]),
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root.targetAttr",
                            "/Root/A/C.targetAttr"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Move targetAttr from /Root/A/C to be a property of /Root/A instead 
        # while also renaming it to "foo"
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A/C.targetAttr", "/Root/A.foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".foo" : Sdf.PathListOp.Create(
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A.targetAttr"]),
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A.foo"]),
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A.targetAttr",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A.targetAttr"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A.targetAttr",
                                    "/Root/A.foo"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A.targetAttr"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A.targetAttr"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A.foo"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        ".targetAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/B.targetAttr",
                            "/Root/A.foo"])
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A.targetAttr",
                            "/Root/A/B.targetAttr",
                            "/Root/A.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

        # Move targetAttr from /Root/A to be a property of /Root/A/C instead 
        # while also renaming it to "foo"
        self.assertTrue(editor.MovePropertyAtPath(
            "/Root/A.targetAttr", "/Root/A/C.foo"))
        self._ApplyCompareAndReset(editor,
            rootContents = {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C.foo"]),
                        "/B" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/B.targetAttr"])
                        },                       
                        "/C" : {
                            ".otherAttr" : Sdf.PathListOp.Create(
                                prependedItems = ["/Root/A/C.targetAttr"]),
                            ".targetAttr" : Sdf.PathListOp.Create(
                                appendedItems = [
                                    "/Root/A/B.targetAttr",
                                    "/Root/A/C.foo"])
                        },
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C.foo"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A/C.foo",
                                "/Root/A/B.targetAttr"],
                            prependedItems = ["/Root/A"],
                            appendedItems = [
                                "/Root/A/B",
                                "/Root/A/C"
                            ])
                    }
                }
            },
            sub1Contents= {
                "/Root" : {
                    "/A" : {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            prependedItems = ["/Root/A/C.foo"]),
                        "/B" : {
                            ".targetAttr" : Sdf.PathListOp.Create(
                                prependedItems = [
                                    "/Root/A/C.foo",
                                    "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" :  {
                        ".otherAttr" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A/C.foo"],
                            prependedItems = ["/Root/A/B.targetAttr"],
                            appendedItems = ["/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = [
                                "/Root/A",
                                "/Root/A/B",
                                "/Root/A/C"],
                            prependedItems = ["/Root/A/C.foo"],
                            appendedItems = [
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.targetAttr"]),
                    }
                }
            },
            sub2Contents = {
                "/Root" : {
                    "/A" : {
                        "/C" : {
                            ".foo" : Sdf.PathListOp.CreateExplicit([
                                "/Root/A/B.targetAttr",
                                "/Root/A/C.targetAttr"])
                        }
                    },
                    "/OtherPrim" : {
                        ".otherAttr" : Sdf.PathListOp.CreateExplicit([
                            "/Root/A/C.foo",
                            "/Root/A/B.targetAttr",
                            "/Root/A/C.targetAttr"]),
                        ".otherRel" : Sdf.PathListOp.Create(
                            deletedItems = ["/Root/A"],
                            prependedItems = ["/Root/A/B"],
                            appendedItems = ["/Root/A/C"]),
                    }
                }
            })

if __name__ == '__main__':
    unittest.main()
