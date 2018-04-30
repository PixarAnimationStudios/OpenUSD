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

        assert backdrop

        # Test Description
        descAttr = backdrop.GetDescriptionAttr()
        assert descAttr
        assert descAttr.GetTypeName() == 'token', \
            "Type of description attribute should be 'token', not %s" % descAttr.GetTypeName()
        descAttr.Set("Backdrop test description")

        stage.GetRootLayer().Save()

if __name__ == "__main__":
    unittest.main()
