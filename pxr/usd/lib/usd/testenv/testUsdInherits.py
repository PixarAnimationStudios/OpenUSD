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

class TestUsdInherits(unittest.TestCase):
    def test_BasicApi(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            classA = stage.CreateClassPrim("/ClassA")
            concrete = stage.OverridePrim("/Concrete")
            items = None

            assert not concrete.HasAuthoredInherits()
            assert concrete.GetInherits().AddInherit(classA.GetPath())
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 1)
            self.assertEqual(concrete.GetMetadata("inheritPaths").prependedItems[0],
                        classA.GetPath())
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)
            # This will be used later in the test.
            items = concrete.GetMetadata("inheritPaths").prependedItems

            assert concrete.GetInherits().RemoveInherit(classA.GetPath())
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 1)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)

            assert concrete.GetInherits().ClearInherits()
            assert not concrete.HasAuthoredInherits()
            assert not concrete.GetMetadata("inheritPaths")

            # Set the list of added items explicitly.
            assert concrete.GetInherits().SetInherits(items)
            assert concrete.HasAuthoredInherits()
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").prependedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 0)
            self.assertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 1)


    def test_InheritedPrim(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("x."+fmt)
            classA = stage.CreateClassPrim("/ClassA")
            stage.DefinePrim("/ClassA/Child")

            concrete = stage.DefinePrim("/Concrete")

            assert not concrete.GetChildren() 
            assert concrete.GetInherits().AddInherit(classA.GetPath())

            self.assertEqual(concrete.GetChildren()[0].GetPath(),
                        concrete.GetPath().AppendChild("Child"))

            assert concrete.GetInherits().RemoveInherit(classA.GetPath())
            assert len(concrete.GetChildren()) == 0

if __name__ == '__main__':
    unittest.main()
