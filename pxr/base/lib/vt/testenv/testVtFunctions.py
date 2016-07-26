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

import unittest
from pxr import Tf, Vt

class TestVtFunctions(unittest.TestCase):

    empty = Vt.FloatArray()

    def test_Constructors(self):
        v = Vt.FloatArray((1,2,3,4,5,6))
        w = Vt.FloatArray(range(9))

    def test_Cat(self):
        v = Vt.FloatArray((1,2,3,4,5,6))

        vcat = Vt.Cat(v,v)
        self.assertEqual(len(vcat), 12)
        self.assertEqual(vcat[11], 6)
        vcat = Vt.Cat(Vt.Cat(v),Vt.Cat(v,v,v,v),Vt.Cat(v,v,v,v,v))
        self.assertEqual(len(vcat), 60)
        self.assertEqual(vcat[58], 5)
        vcat = Vt.Cat(v,self.empty)
        self.assertEqual(vcat, Vt.Cat(v))
        e = Vt.Cat(self.empty,self.empty)
        self.assertEqual((len(e)), 0)

    def test_AnyTrue(self):
        self.assertTrue(Vt.AnyTrue(Vt.FloatArray((1,0,0))))
        self.assertFalse(Vt.AnyTrue(Vt.FloatArray((0,0,0))))
        self.assertTrue(Vt.AnyTrue(Vt.FloatArray((1,2,3))))
        self.assertFalse(Vt.AnyTrue(self.empty))

    def test_AllTrue(self):
        self.assertFalse(Vt.AllTrue(Vt.FloatArray((1,0,0))))
        self.assertFalse(Vt.AllTrue(Vt.FloatArray((0,0,0))))
        self.assertTrue(Vt.AllTrue(Vt.FloatArray((1,2,3))))
        self.assertFalse(Vt.AllTrue(self.empty))

if __name__ == '__main__':
    unittest.main()
