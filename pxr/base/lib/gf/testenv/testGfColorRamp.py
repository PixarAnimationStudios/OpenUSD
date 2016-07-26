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

import sys
import unittest
import math
from pxr import Gf

class TestGfColorRamp(unittest.TestCase):

    def test_Constructors(self):
        results = []
        results.append( Gf.ColorRamp() )
        results.append( Gf.ColorRamp( Gf.RGB() ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB() ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB() ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB(), 1 ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB(), 1, 2 ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB(), 1, 2, 3 ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB(), 1, 2, 3, 4 ) )
        results.append( Gf.ColorRamp( Gf.RGB(), Gf.RGB(), Gf.RGB(), 1, 2, 3, 4, 5 ) )
        for r in results:
            self.assertIsInstance(r, Gf.ColorRamp)

    def test_Operators(self):
        # generate some unique values
        r1 = Gf.ColorRamp( Gf.RGB( 1, 2, 3 ) )
        r2 = Gf.ColorRamp( Gf.RGB( 1, 2, 4 ) )
        r3 = Gf.ColorRamp( Gf.RGB( 3, 2, 1 ) )
        results = [r1, r2, r3]

        # test operators
        for i in range(len(results)):
            # Test repr
            self.assertTrue(eval(repr(results[i])) == results[i])
            # Test equality
            for j in range(len(results)):
                if i == j:
                    self.assertTrue(results[i] == results[j])
                else:
                    self.assertTrue(results[i] != results[j])

    def test_Eval(self):
        '''Eval -- make sure ends and middle are exact, make sure betweens are
        between them.'''
        r = Gf.ColorRamp()
        self.assertTrue(r.Eval(0) == Gf.RGB(0, 0, 1) and
                r.Eval(0.5) == Gf.RGB(0, 1, 0) and
                r.Eval(1) == Gf.RGB(1, 0, 0))

        self.assertFalse(r.Eval(0.25).r != 0 or \
        r.Eval(0.75).b != 0 or \
        r.Eval(0.25).g <= 0 or \
        r.Eval(0.25).g >= 1 or \
        r.Eval(0.25).b <= 0 or \
        r.Eval(0.25).b >= 1 or \
        r.Eval(0.75).r <= 0 or \
        r.Eval(0.75).r >= 1 or \
        r.Eval(0.75).g <= 0 or \
        r.Eval(0.75).g >= 1)


    def test_Properties(self):
        r = Gf.ColorRamp()

        testVal = 0.314159
        for propName in 'midPos widthMin widthMax widthMidIn widthMidOut'.split():
            self.assertNotEqual(getattr(r, propName), testVal)
            setattr(r, propName, testVal)
            self.assertEqual(getattr(r, propName), testVal)

if __name__ == '__main__':
    unittest.main()
