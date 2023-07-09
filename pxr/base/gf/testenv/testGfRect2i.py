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
from __future__ import division

import sys, math
import unittest
from pxr import Gf

class TestGfRect2i(unittest.TestCase):

    def test_Constructor(self):
        self.assertIsInstance(Gf.Rect2i(), Gf.Rect2i)
        self.assertIsInstance(Gf.Rect2i(Gf.Vec2i(), Gf.Vec2i()), Gf.Rect2i)
        self.assertIsInstance(Gf.Rect2i(Gf.Vec2i(), 1, 1), Gf.Rect2i)

        self.assertTrue(Gf.Rect2i().IsNull())
        self.assertTrue(Gf.Rect2i().IsEmpty())
        self.assertFalse(Gf.Rect2i().IsValid())

        # further test of above.
        r = Gf.Rect2i(Gf.Vec2i(), Gf.Vec2i(-1,0))
        self.assertTrue(not r.IsNull() and r.IsEmpty())

        self.assertEqual(Gf.Rect2i(Gf.Vec2i(-1, 1), Gf.Vec2i(1, -1)).GetNormalized(),
            Gf.Rect2i(Gf.Vec2i(-1, -1), Gf.Vec2i(1, 1)))

    def test_Properties(self):
        r = Gf.Rect2i()
        r.max = Gf.Vec2i()
        r.min = Gf.Vec2i(1,1)
        r.GetNormalized()
        r.max = Gf.Vec2i(1,1)
        r.min = Gf.Vec2i()
        r.GetNormalized()

        r.min = Gf.Vec2i(3, 1)
        self.assertEqual(r.min, Gf.Vec2i(3, 1))

        r.max = Gf.Vec2i(4, 5)
        self.assertEqual(r.max, Gf.Vec2i(4, 5))

        r.minX = 10
        self.assertEqual(r.minX, 10)

        r.maxX = 20
        self.assertEqual(r.maxX, 20)

        r.minY = 30
        self.assertEqual(r.minY, 30)

        r.maxY = 40
        self.assertEqual(r.maxY, 40)

        r = Gf.Rect2i(Gf.Vec2i(), Gf.Vec2i(10, 10))
        self.assertEqual(r.GetCenter(), Gf.Vec2i(5, 5))
        self.assertEqual(r.GetArea(), 121)
        self.assertEqual(r.GetHeight(), 11)
        self.assertEqual(r.GetWidth(), 11)
        self.assertEqual(r.GetSize(), Gf.Vec2i(11, 11))

        r.Translate(Gf.Vec2i(10, 10))
        self.assertEqual(r, Gf.Rect2i(Gf.Vec2i(10, 10), Gf.Vec2i(20, 20)))


        r1 = Gf.Rect2i()
        r2 = Gf.Rect2i(Gf.Vec2i(), Gf.Vec2i(1,1))

        r1.GetIntersection(r2)
        r2.GetIntersection(r1)
        r1.GetIntersection(r1)

        r1.GetUnion(r2)
        r2.GetUnion(r1)
        r1.GetUnion(r1)

        r1 = Gf.Rect2i(Gf.Vec2i(0, 0), Gf.Vec2i(10, 10))
        r2 = Gf.Rect2i(Gf.Vec2i(5, 5), Gf.Vec2i(15, 15))
        self.assertEqual(r1.GetIntersection(r2), Gf.Rect2i(Gf.Vec2i(5, 5), Gf.Vec2i(10, 10)))
        self.assertEqual(r1.GetUnion(r2), Gf.Rect2i(Gf.Vec2i(0, 0), Gf.Vec2i(15, 15)))
        tmp = Gf.Rect2i(r1)
        tmp += r2
        self.assertEqual(tmp, Gf.Rect2i(Gf.Vec2i(0, 0), Gf.Vec2i(15, 15)))

        self.assertTrue(r1.Contains(Gf.Vec2i(3,3)) and not r1.Contains(Gf.Vec2i(11,11)))

        self.assertEqual(r1, Gf.Rect2i(Gf.Vec2i(0, 0), Gf.Vec2i(10, 10)))
        self.assertTrue(r1 != r2)

        self.assertEqual(r1, eval(repr(r1)))

        self.assertTrue(len(str(Gf.Rect2i())))

    def test_Hash(self):
        r = Gf.Rect2i(
            Gf.Vec2i(1, 2),
            Gf.Vec2i(10, 20)
        )

        self.assertEqual(hash(r), hash(r))
        self.assertEqual(hash(r), hash(Gf.Rect2i(r)))

if __name__ == '__main__':
    unittest.main()
