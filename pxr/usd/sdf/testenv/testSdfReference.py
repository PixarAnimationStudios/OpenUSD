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

from __future__ import print_function

from pxr import Sdf, Tf
import unittest

class TestSdfReferences(unittest.TestCase):
    def test_Basic(self):
        # Test all combinations of the following keyword arguments.
        args = [
            ['assetPath', '//unit/layer.sdf'],
            ['primPath', '/rootPrim'],
            ['layerOffset', Sdf.LayerOffset(48, -2)],
            ['customData', {'key': 42, 'other': 'yes'}],
        ]
        
        for n in range(1 << len(args)):
            kw = {}
            for i in range(len(args)):
                if (1 << i) & n:
                    kw[args[i][0]] = args[i][1]
        
            ref = Sdf.Reference(**kw)
            print('  Testing Repr for: ' + repr(ref))
        
            self.assertEqual(ref, eval(repr(ref)))
            for arg, value in args:
                if arg in kw:
                    self.assertEqual(eval('ref.' + arg), value)
                else:
                    self.assertEqual(eval('ref.' + arg), eval('Sdf.Reference().' + arg))
        
        
        print("\nTesting Sdf.Reference immutability.")
        
        # There is no proxy for the Reference yet (we don't have a good
        # way to support nested proxies).  Make sure the user can't modify
        # temporary Reference objects.
        with self.assertRaises(AttributeError):
            Sdf.Reference().assetPath = '//unit/blah.sdf'

        with self.assertRaises(AttributeError):
            Sdf.Reference().primPath = '/root'

        with self.assertRaises(AttributeError):
            Sdf.Reference().layerOffset = Sdf.LayerOffset()

        with self.assertRaises(AttributeError):
            Sdf.Reference().layerOffset.offset = 24

        with self.assertRaises(AttributeError):
            Sdf.Reference().layerOffset.scale = 2

        with self.assertRaises(AttributeError):
            Sdf.Reference().customData = {'refCustomData': {'refFloat': 1.0}}
        
        # Code coverage.
        ref0 = Sdf.Reference(customData={'a': 0})
        ref1 = Sdf.Reference(customData={'a': 0, 'b': 1})
        self.assertTrue(ref0 == ref0)
        self.assertTrue(ref0 != ref1)
        self.assertTrue(ref0 < ref1)
        self.assertTrue(ref1 > ref0)
        self.assertTrue(ref0 <= ref1)
        self.assertTrue(ref1 >= ref0)

        # Regression test for bug USD-5000 where less than operator was not 
        # fully anti-symmetric
        r1 = Sdf.Reference()
        r2 = Sdf.Reference('//test/layer.sdf', layerOffset=Sdf.LayerOffset(48, -2))
        self.assertTrue(r1 < r2)
        self.assertFalse(r2 < r1)

        # Test IsInternal()

        # r2 can not be an internal reference since it's assetPath is not empty
        self.assertFalse(r2.IsInternal())

        # ref0 is an internal referennce because it has an empty assetPath
        self.assertTrue(ref0.IsInternal())

        # Test invalid asset paths.
        with self.assertRaises(Tf.ErrorException):
            p = Sdf.Reference('\x01\x02\x03')

        with self.assertRaises(Tf.ErrorException):
            p = Sdf.AssetPath('\x01\x02\x03')
            p = Sdf.AssetPath('foobar', '\x01\x02\x03')

if __name__ == "__main__":
    unittest.main()
