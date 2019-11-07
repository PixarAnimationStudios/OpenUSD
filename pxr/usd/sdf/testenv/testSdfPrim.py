#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

import sys, unittest
from pxr import Sdf, Tf

class TestSdfPrim(unittest.TestCase):
    def test_NameChildrenInsert(self):
        import copy, random

        layer = Sdf.Layer.CreateAnonymous("test")
        rootPrim = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
        groundTruthList = []
        prevGroundTruthList = []

        for i in range(1000):
            primName = 'geom{0}'.format(i)
            insertIndex = random.randint(-100, 100)

            primSpec = Sdf.PrimSpec(layer, primName, Sdf.SpecifierDef, 'Scope')

            rootPrim.nameChildren.insert(insertIndex, primSpec)
            groundTruthList.insert(insertIndex, primName)
            nameChildrenList = [x.name for x in rootPrim.nameChildren.values()]

            if nameChildrenList == groundTruthList:
                prevGroundTruthList = copy.deepcopy(groundTruthList)
            else:
                print("FAILED with primName {0} and insertIndex {1}".format(
                    primName, insertIndex))
                print("groundTruthList is {0}".format(groundTruthList))
                print("            we got {0}".format(nameChildrenList))
                print("     previous list {0}".format(prevGroundTruthList))
                self.fail("Prim insertion test failed")


if __name__ == "__main__":
    unittest.main()

