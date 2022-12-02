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
import os
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
        for i in range(1000000):
            checku(i)
        for i in range(-500000, 500000):
            checks(i)

        # A wider range of valid values.
        for i in range(0, 1000000000, 9337):
            checks(i)
        for i in range(-500000000, 500000000, 9337):
            checks(i)

        # Get the max/min values.
        ulmax, lmax, lmin = (
            Tf._GetULongMax(), Tf._GetLongMax(), Tf._GetLongMin())

        # Check the extrema and one before to ensure they work.
        for n in [ulmax-1, ulmax]:
            checku(n)

        for n in [lmin, lmin+1, lmax-1, lmax]:
            checks(n)

        # Check that some beyond the extrema over/underflow.
        #
        # Unsigned overflow.
        for i in range(1, 1000):
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

    def test_utf8_collation_ordering(self):

        if Tf.GetEnvSetting('TF_UTF8_IDENTIFIERS') == False:
            return

        def get_bytes(s):
            codepoint_split = s.strip().split(' ')
            unicode_str = ''
            for byte in codepoint_split:
                unicode_str += chr(int(byte, 16))
            encoded_bytes = unicode_str.encode('utf-8', "ignore")
            return encoded_bytes

        # The files are designed so each line in the file will order as being greater than or equal to the previous one, when using the UCA and the Default Unicode Collation Element Table.
        # There are known non-conformances in the current implementation that are called out in the CollationTest_NON_IGNORABLE_SHORT_ExceptionList.txt file.
        # This test will test consecutive lines to ensure that the previous line is less than the current.
        # If this fails, and the lines are not in the exception list, the test will fail.
        def parse_and_test_file(file, start_index):
            lines = []
            with open(file, 'r') as file_to_test:
                lines = file_to_test.readlines()
            
            input_base_name = os.path.splitext(os.path.basename(file))
            exception_file_name = input_base_name[0] + "_ExceptionList" + input_base_name[1]
            exception_lines = []
            with open(exception_file_name, 'r') as exception_file:
                exception_lines = exception_file.readlines()
                
            # create the exception list
            # each exception is formatted as first_element_in_test ';' second_element_in_test
            exceptions = []
            for exception_line in exception_lines:
                tokens = exception_line.split(';')
                exceptions.append((tokens[0].strip(), tokens[1].strip()))
                
            for ln in range(start_index, len(lines) - 1):
                is_exception = False
                for exception_element in exceptions:
                    if exception_element[0] == lines[ln].strip() and exception_element[1] == lines[ln + 1].strip():
                        is_exception = True
                        break
                
                if not is_exception:
                    self.assertGreaterEqual(Tf.DictionaryStrcmp(get_bytes(lines[ln + 1]), get_bytes(lines[ln])), 0, 'Failure in file {} on lines {} and {} with code points {} and {}'.format(file, ln, ln+1, lines[ln], lines[ln+1]))

        # First 10 lines are comments
        parse_and_test_file('./CollationTest_NON_IGNORABLE_SHORT.txt', 9)

if __name__ == '__main__':
    unittest.main()
