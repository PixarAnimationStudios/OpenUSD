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

from pxr import Tf
import logging
import unittest

class TestStringUtils(unittest.TestCase):
    """
    Test Tf String Utils (The python wrapped porting of the utility functions).
    """
    def setUp(self):
        self.log = logging.getLogger()

    def test_StringSplit(self):
        """Testing StringSplit() function. This function is supposed to behave
        like the split method on python string objects."""

        self.log.info("Testing string split cases")

        self.assertEqual([], Tf.StringSplit("",""))
        self.assertEqual([], Tf.StringSplit("abcd",""))
        self.assertEqual([], Tf.StringSplit("","ccc"))

        s = "abcd"
        self.assertEqual(s.split("a"), Tf.StringSplit(s, "a"))
        self.assertEqual(s.split("b"), Tf.StringSplit(s, "b"))
        self.assertEqual(s.split("c"), Tf.StringSplit(s, "c"))
        self.assertEqual(s.split("d"), Tf.StringSplit(s, "d"))
        self.assertEqual(s.split("abcd"), Tf.StringSplit(s, "abcd"))
        self.assertEqual(s.split("ab"), Tf.StringSplit(s, "ab"))

        s = "a:+b:+c:+d"
        self.assertEqual(s.split(":+"), Tf.StringSplit(s, ":+"))

        s = "a:+b:+c:d"
        self.assertEqual(s.split(":+"), Tf.StringSplit(s, ":+"))

    def test_Unicode(self):
        """Testing that we can pass python unicode objects to wrapped
        functions expecting std::string"""
        self.log.info("Testing unicode calls")
        self.assertEqual(Tf.StringSplit('123', '2'), ['1', '3'])
        self.assertEqual(Tf.StringSplit('123', u'2'), ['1', '3'])
        self.assertEqual(Tf.StringSplit(u'123', '2'), ['1', '3'])
        self.assertEqual(Tf.StringSplit(u'123', u'2'), ['1', '3'])

        self.assertEqual(Tf.DictionaryStrcmp('apple', 'banana'), -1)
        self.assertEqual(Tf.DictionaryStrcmp('apple', u'banana'), -1)
        self.assertEqual(Tf.DictionaryStrcmp(u'apple', 'banana'), -1)
        self.assertEqual(Tf.DictionaryStrcmp(u'apple', u'banana'), -1)

    def test_StringToLong(self):

        def checks(val):
            self.assertEqual(Tf.StringToLong(repr(val)), val)
        def checku(val):
            self.assertEqual(Tf.StringToULong(repr(val)), val)

        # A range of valid values.
        for i in xrange(1000000):
            checku(i)
        for i in xrange(-500000, 500000):
            checks(i)

        # A wider range of valid values.
        for i in xrange(0, 1000000000, 9337):
            checks(i)
        for i in xrange(-500000000, 500000000, 9337):
            checks(i)

        # Get the max/min values.
        ulmax, lmax, lmin = (
            Tf._GetULongMax(), Tf._GetLongMax(), Tf._GetLongMin())

        # Check the extrema and one before to ensure they work.
        map(checku, [ulmax-1, ulmax])
        map(checks, [lmin, lmin+1, lmax-1, lmax])

        # Check that some beyond the extrema over/underflow.
        #
        # Unsigned overflow.
        for i in xrange(1, 1000):
            with self.assertRaises(ValueError):
                checku(ulmax + i)
            with self.assertRaises(ValueError):
                checks(lmax + i)
            with self.assertRaises(ValueError):
                checks(lmin - i)

    def test_Identifiers(self):
        self.assertFalse(Tf.IsValidIdentifier(''))
        self.assertTrue(Tf.IsValidIdentifier('hello9'))
        self.assertFalse(Tf.IsValidIdentifier('9hello'))
        self.assertTrue(Tf.IsValidIdentifier('hello_world'))
        self.assertTrue(Tf.IsValidIdentifier('HELLO_WORLD'))
        self.assertTrue(Tf.IsValidIdentifier('hello_world_1234'))
        self.assertFalse(Tf.IsValidIdentifier('hello_#world#_1234'))
        self.assertFalse(Tf.IsValidIdentifier('h e l l o'))

        self.assertEqual(Tf.MakeValidIdentifier(''), '_')
        self.assertEqual(Tf.MakeValidIdentifier('hello9'), 'hello9')
        self.assertEqual(Tf.MakeValidIdentifier('9hello'), '_hello')
        self.assertEqual(
            Tf.MakeValidIdentifier('hello_#world#_1234'), 'hello__world__1234')
        self.assertFalse(Tf.IsValidIdentifier('h e l l o'), 'h_e_l_l_o')
        self.assertFalse(Tf.IsValidIdentifier('!@#$%'), '_____')


if __name__ == '__main__':
    unittest.main()
