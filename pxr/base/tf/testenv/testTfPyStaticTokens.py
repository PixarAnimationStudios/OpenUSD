#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf
import unittest

class TestTfPyStaticTokens(unittest.TestCase):

    def test_PyStaticTokens(self):
        testTokens = (
            ('orange', 'orange'),
            ('pear', "d'Anjou")
            )

        for scope in (Tf._testStaticTokens, Tf._TestStaticTokens):
            for attrName,expectedValue in testTokens:
                self.assertTrue(hasattr(scope, attrName))
                value = getattr(scope, attrName)
                self.assertEqual(value, expectedValue,
                    "Unexpected value for {0}: got '{1}', expected '{2}'".format(
                        attrName, value, expectedValue))

            # Not wrapping arrays yet, just the array elements.
            self.assertFalse(hasattr(scope, 'apple'))

if __name__ == '__main__':
    unittest.main()

