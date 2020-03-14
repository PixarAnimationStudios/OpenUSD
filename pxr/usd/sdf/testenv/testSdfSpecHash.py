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

import sys, unittest
from pxr import Sdf

class TestSdfSpecHash(unittest.TestCase):
    def test_Basic(self):
        # Test using a python dictionary to associate values with specs.
        # This relies on the hash function for spec handles.
        test_dict = {}
        test_value = 'foo'

        layer = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.PrimSpec(layer, "foo", Sdf.SpecifierOver)
        test_dict[primSpec] = test_value
        for i in range(10):
            self.assertEqual(test_dict[ layer.GetObjectAtPath(primSpec.path) ], 
                             test_value)

if __name__ == "__main__":
    unittest.main()
