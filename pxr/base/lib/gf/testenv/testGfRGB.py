#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from pxr import Gf
import unittest

import ctypes

def ToFloat(x):
    return ctypes.c_float(x).value

class TestGfRGB(unittest.TestCase):

    def Test(self, cls):
        def make(*vals):
            return cls((vals*size)[:size])

        size = len(cls())

        # default ctor zero initializes
        self.assertEqual(cls(), cls())
        self.assertEqual([0]*size, [x for x in cls()])
        
        # unary ctor sets all components
        self.assertEqual([ToFloat(1.234)]*size, [x for x in cls(1.234)])

        # explicit ctor
        ctorArgs = range(123, 123 + size)
        self.assertEqual([x for x in cls(*ctorArgs)], range(123, 123 + size))

        # construct from vec
        if size == 3:
            self.assertEqual(cls(Gf.Vec3f(1,2,3)), cls(1,2,3))
        elif size == 4:
            self.assertEqual(cls(Gf.Vec4f(1,2,3,4)), cls(1,2,3,4))
            
        # Clamp
        self.assertEqual(make(-.5,.5,.75,1.25).Clamp(), make(0,.5,.75,1))
        self.assertEqual(make(1,2,3,4).Clamp(), make(1,1,1,1))
        self.assertEqual(make(1,2,3,4).Clamp(min=2), make(2,1,1,1))
        self.assertEqual(make(1,2,3,4).Clamp(max=2), make(1,2,2,2))
        self.assertEqual(make(1,2,3,4).Clamp(min=2,max=3), make(2,2,3,3))

        # Clamp should modify in-place
        x = make(1,2,3,4)
        x.Clamp(2, 3)
        self.assertEqual(x, make(2,2,3,3))

        # IsBlack and IsWhite
        self.assertTrue(make(0).IsBlack())
        self.assertTrue(make(1).IsWhite())
        self.assertFalse(make(1).IsBlack())
        self.assertFalse(make(0).IsWhite())
        self.assertFalse(make(0.5).IsBlack())
        self.assertFalse(make(0.5).IsWhite())

        # Transform
        # XXX add test

        # GetComplement
        self.assertEqual(make(0).GetComplement(), make(1))
        self.assertEqual(make(0, 0.25, 0.75, 1).GetComplement(), make(1, 0.75, 0.25, 0))
        self.assertEqual(make(-1, 0, 1, 2).GetComplement(), make(2, 1, 0, -1))
        
        # GetVec
        # XXX add test
        
        # GetHSV
        # XXX add test

        # SetHSV
        # XXX add test

        # r,g,b,a
        # XXX add test

        # repr
        self.assertEqual(eval(repr(make(1,2,3,4))), make(1,2,3,4))
        self.assertEqual(eval(repr(make(0))), make(0))
        self.assertEqual(eval(repr(make(1))), make(1))

        # str
        self.assertTrue(len(str(make(0))))
        self.assertTrue(len(str(make(1))))
        self.assertTrue(len(str(make(1,2,3,4))))

        # operators
        # XXX add test

        # tuples implicitly convert to rgb(a)
        self.assertEqual(cls((1,)*size), make(1))
        self.assertEqual(cls((0,)*size), make(0))

    def test_RGB(self):
        self.Test(Gf.RGB)

    def test_RGBA(self):
        self.Test(Gf.RGBA)

if __name__ == '__main__':
    unittest.main()
