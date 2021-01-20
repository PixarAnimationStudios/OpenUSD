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

from pxr import Sdf
import unittest

class TestSdfAttributeBlocking(unittest.TestCase):
    def test_Basic(self):
        # generate a test layer with a default attr and time sample attr
        layer = Sdf.Layer.CreateAnonymous()
        prim = Sdf.PrimSpec(layer, "Sphere", Sdf.SpecifierDef)

        # configure a default attr
        defAttr = Sdf.AttributeSpec(prim, "visibility", Sdf.ValueTypeNames.Token)  
        defAttr.default = "visible"

        # configure a time sampled attr
        sampleAttr = Sdf.AttributeSpec(prim, "xformOp:transform", Sdf.ValueTypeNames.Double)
        layer.SetTimeSample(sampleAttr.path, 101, 101.0)
        layer.SetTimeSample(sampleAttr.path, 102, 102.0)
        layer.SetTimeSample(sampleAttr.path, 103, 103.0)
        layer.SetTimeSample(sampleAttr.path, 104, 104.0)

        # Test time sample based API
        for i in range(101, 105):
            self.assertEqual(layer.QueryTimeSample(sampleAttr.path, i), i)
            layer.SetTimeSample(sampleAttr.path, i, Sdf.ValueBlock())
            self.assertEqual(layer.QueryTimeSample(sampleAttr.path, i),
                             Sdf.ValueBlock())

        #Test default value API
        self.assertEqual(defAttr.default, 'visible')
        defAttr.default = Sdf.ValueBlock()
        self.assertEqual(defAttr.default, Sdf.ValueBlock())

if __name__ == "__main__":
    unittest.main()
