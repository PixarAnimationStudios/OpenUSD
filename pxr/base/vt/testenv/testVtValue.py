#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
        self.assertEqual(Vt._test_Str(Vt.Token('hello')), 'hello')

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

        self.assertEqual(Vt._test_ValueTypeName(Vt.Half(1.234)), 'pxr_half::half')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Float(1.234)), 'float')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Double(1.234)), 'double')

        # Make sure that Python strings end up as strings, unless they're
        # coerced via Vt.Token
        self.assertEqual(Vt._test_ValueTypeName('hello'), 'string')
        self.assertEqual(Vt._test_ValueTypeName(u'hello'), 'string')
        self.assertEqual(Vt._test_ValueTypeName(Vt.Token('hello')), 'TfToken')

    def test_IntValueRoundTrip(self):
        '''Make sure we correctly convert ints of various sizes in the value
        python bindings
        '''
        self.assertEqual(Vt._test_Ident(0), 0)
        self.assertEqual(Vt._test_Ident(100), 100)
        self.assertEqual(Vt._test_Ident(2**32 - 1), 2**32 - 1)
        self.assertEqual(Vt._test_Ident(2**64 - 1), 2**64 - 1)

    def test_Dictionary(self):
        good = {'key' : 'value',
                'key2' : 'value',
                'key3' : [1,2,3,'one','two','three'],
                'key4' : ['four', 'five', { 'six' : 7, 'eight' : [ {'nine':9} ] } ],
                'key5' : { 'key6' : 'value' },
                'key6' : None,
                'key7' : u'value',
                }

        bad1 = {1 : 2}
        bad2 = {'key' : Ellipsis}

        self.assertEqual(good, Vt._test_Ident(good))

        vtdict = Vt._ReturnDictionary(good)
        self.assertEqual(good, vtdict)
        self.assertEqual(vtdict['key7'], 'value')

        with self.assertRaises(TypeError):
            Vt._ReturnDictionary(bad1)
        with self.assertRaises(TypeError):
            Vt._ReturnDictionary(bad2)

        # Test passing and returning python lists of dicts <->
        # std::vector<VtDictionary>
        testDictList = [{'foo':'bar'}, {'baz':123}, {'good':good}]
        self.assertEqual(testDictList, Vt._DictionaryArrayIdent(testDictList))
        self.assertEqual([], Vt._DictionaryArrayIdent([]))

        with self.assertRaises(TypeError):
            Vt._DictionaryArrayIdent('notAList')
        with self.assertRaises(TypeError):
            Vt._DictionaryArrayIdent(['notADict'])
        with self.assertRaises(TypeError):
            Vt._DictionaryArrayIdent([{1234:'keyNotAString'}])

    def test_WrappedTypesComparable(self):
        """
        Makes sure that values wrapped in a value wrapper are still
        equality-comparable.
        """
        t1 = Vt.Token("hello")
        t2 = Vt.Token("hello")
        t3 = Vt.Token("world")
        f1 = Vt.Float(1.0)
        f2 = Vt.Float(1.0)
        d1 = Vt.Double(1.0)

        self.assertEqual(t1, t2)
        self.assertNotEqual(t1, t3)
        self.assertNotEqual(t1, f1)
        self.assertEqual(f1, f2)
        self.assertNotEqual(f1, d1)

if __name__ == '__main__':
    unittest.main()
