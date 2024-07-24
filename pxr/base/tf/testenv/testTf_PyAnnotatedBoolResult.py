#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf
import unittest

class TestTfPyAnnotatedBoolResult(unittest.TestCase):

    def test_boolResult(self):
        result = Tf._TestAnnotatedBoolResult(True, 'This is true')
        self.assertTrue(result)
        self.assertEqual(result.annotation, 'This is true')
        boolResult, annotation = result
        self.assertTrue(boolResult)
        self.assertEqual(annotation, 'This is true')

        result = Tf._TestAnnotatedBoolResult(False, 'This is false')
        self.assertFalse(result)
        self.assertEqual(result.annotation, 'This is false')
        boolResult, annotation = result
        self.assertFalse(boolResult)
        self.assertEqual(annotation, 'This is false')

if __name__ == '__main__':
    unittest.main()

