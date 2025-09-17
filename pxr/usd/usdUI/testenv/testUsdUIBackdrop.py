#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd, UsdUI
import unittest

class TestUsdUIBackdrop(unittest.TestCase):
    def test_Basic(self):
        fileName = "backdrop.usda"
        stage = Usd.Stage.CreateNew(fileName)

        rootPrim = stage.DefinePrim("/ModelShading")
        materialsPath = rootPrim.GetPath().AppendChild('Materials')
        parentMaterialPath = materialsPath.AppendChild('ParentMaterial')
       
        # Test Backdrop
        backdrop = UsdUI.Backdrop.Define(
            stage, parentMaterialPath.AppendChild("Backdrop_1"))

        assert(backdrop)

        # Test Description
        descAttr = backdrop.GetDescriptionAttr()
        assert descAttr
        assert descAttr.GetTypeName() == 'token', \
            "Type of description attribute should be 'token', not %s" % descAttr.GetTypeName()
        descAttr.Set("Backdrop test description")

        stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
