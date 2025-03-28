#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd, UsdUI
import unittest

class TestUsdUISceneGraphPrim(unittest.TestCase):
    def test_Basic(self):
        fileName = "sceneGraph.usda"
        stage = Usd.Stage.CreateNew(fileName)
       
        # Create this prim first, since it's the "entrypoint" to the layer, and
        # we want it to appear at the top
        rootPrim = stage.DefinePrim("/ANode")
       
        # Test Node
        sceneGraphPrim = UsdUI.SceneGraphPrimAPI.Apply(rootPrim)
        assert(sceneGraphPrim)
       
        # Test displayName
        displayNameAttr = sceneGraphPrim.GetDisplayNameAttr()
        assert displayNameAttr
        displayNameAttr = sceneGraphPrim.CreateDisplayNameAttr()
        assert displayNameAttr, "Failed creating display attribute"
        assert displayNameAttr.GetTypeName() == 'token', \
            "Type of position attribute should be 'token', not %s" % \
            displayNameAttr.GetTypeName()
        displayNameAttr.Set('foo')
       
        # Test Display Color
        displayGroupAttr = sceneGraphPrim.GetDisplayGroupAttr()
        assert displayGroupAttr
        displayGroupAttr = sceneGraphPrim.CreateDisplayGroupAttr()
        assert displayGroupAttr, "Failed creating display color attribute"
        assert displayGroupAttr.GetTypeName() == 'token', \
            "Type of position attribute should be 'token', not %s" % \
            displayGroupAttr.GetTypeName()
        displayGroupAttr.Set("bar")
       
        stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
