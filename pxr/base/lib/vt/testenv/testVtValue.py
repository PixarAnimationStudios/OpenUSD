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
import sys
import math
from pxr import Vt

status = 0

def err( msg ):
    global status
    status += 1
    return "ERROR: " + msg + " failed"

class TestVtValue(unittest.TestCase):

    def test_AutoConversion(self):
        '''Call a func that takes a VtValue without a VtValue (test
           auto-conversion)'''
        self.assertEqual(Vt._test_ValueTypeName(1.234), 'double')
        self.assertEqual(Vt._test_ValueTypeName('hello'), 'string')
        self.assertEqual(Vt._test_ValueTypeName(Ellipsis), 'TfPyObjWrapper')

    def test_Ident(self):
        self.assertEqual(Vt._test_Ident(1.234), 1.234)

    def test_Str(self):
        self.assertEqual(Vt._test_Str(Ellipsis), str(Ellipsis))
        self.assertEqual(Vt._test_Str((1,2,3)), str((1,2,3)))
        self.assertEqual(Vt._test_Str(Vt.DoubleArray()), str(Vt.DoubleArray()))
        self.assertEqual(Vt._test_Str(1.234), str(1.234))
        self.assertEqual(Vt._test_Str(u'unicode'), 'unicode')

    def test_ValueTypeName(self):
        self.assertEqual(Vt._test_ValueTypeName(True), 'bool')
        self.assertEqual(Vt._test_ValueTypeName(False), 'bool')
        self.assertEqual(Vt._test_ValueTypeName(0), 'int')
        self.assertEqual(Vt._test_ValueTypeName(1), 'int')

        self.assertEqual(Vt._test_ValueTypeName(Vt.Bool(True)), 'bool')
        self.assertEqual(Vt._test_ValueTypeName(Vt.UChar(100)), 'unsigned char')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Short(1234)), 'short')
        self.assertEqual(Vt._test_ValueTypeName(Vt.UShort(1234)), 'unsigned short')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Int(12345)), 'int')
        self.assertEqual(Vt._test_ValueTypeName(Vt.UInt(12345)), 'unsigned int')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Long(1234)), 'long')
        self.assertEqual(Vt._test_ValueTypeName(Vt.ULong(100)), 'unsigned long')

        self.assertEqual(Vt._test_ValueTypeName(Vt.Half(1.234)), 'half')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Float(1.234)), 'float')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Double(1.234)), 'double')

    def test_Dictionary(self):
        good = {'key' : 'value',
                'key2' : 'value',
                'key3' : [1,2,3,'one','two','three'],
                'key4' : ['four', 'five', { 'six' : 7, 'eight' : [ {'nine':9} ] } ],
                'key5' : { 'key6' : 'value' },
                'key5' : None,
                'key6' : u'value',
                }

        bad1 = {1 : 2}
        bad2 = {'key' : Ellipsis}

        self.assertEqual(good, Vt._test_Ident(good))

        vtdict = Vt._ReturnDictionary(good)
        self.assertEqual(good, vtdict)
        self.assertEqual(vtdict['key6'], 'value')

        with self.assertRaises(TypeError):
            Vt._ReturnDictionary(bad1)
        with self.assertRaises(TypeError):
            Vt._ReturnDictionary(bad2)

if __name__ == '__main__':
    unittest.main()
