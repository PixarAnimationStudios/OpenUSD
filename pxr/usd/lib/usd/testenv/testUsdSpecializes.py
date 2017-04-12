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

import unittest
from pxr import Usd, Sdf, Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdSpecializes(unittest.TestCase):
    def test_BasicApi(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            specA = stage.DefinePrim("/SpecA")
            concrete = stage.OverridePrim("/Concrete")
            items = None

            assert not concrete.HasAuthoredSpecializes()
            assert concrete.GetSpecializes().AppendSpecialize(specA.GetPath())
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 1)
            self.assertEqual(concrete.GetMetadata("specializes").addedItems[0],
                        specA.GetPath())
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)
            # This will be used later in the test.
            items = concrete.GetMetadata("specializes").addedItems

            assert concrete.GetSpecializes().RemoveSpecialize(specA.GetPath())
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").deletedItems), 1)
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)

            assert concrete.GetSpecializes().ClearSpecializes()
            assert not concrete.HasAuthoredSpecializes()
            assert not concrete.GetMetadata("specializes")

            # Set the list of added items explicitly.
            assert concrete.GetSpecializes().SetSpecializes(items)
            assert concrete.HasAuthoredSpecializes()
            self.assertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("specializes").explicitItems), 1)

    def test_SpecializedPrim(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            specA = stage.CreateClassPrim("/SpecA")
            stage.DefinePrim("/SpecA/Child")

            concrete = stage.DefinePrim("/Concrete")

            assert not concrete.GetChildren() 
            assert concrete.GetSpecializes().AppendSpecialize(specA.GetPath())

            self.assertEqual(concrete.GetChildren()[0].GetPath(),
                        concrete.GetPath().AppendChild("Child"))

            assert concrete.GetSpecializes().RemoveSpecialize(specA.GetPath())
            assert len(concrete.GetChildren()) == 0

if __name__ == '__main__':
    unittest.main()
