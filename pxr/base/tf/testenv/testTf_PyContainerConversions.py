#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf
import unittest

class TestPyContainerConversions(unittest.TestCase):
    def test_VectorConversions(self):
        vec = (1, 2, 3)
        vecTimesTwo = Tf.Tf_TestPyContainerConversions.GetVectorTimesTwo(vec)
        self.assertEqual(vecTimesTwo, [2.0, 4.0, 6.0])

    def test_PairConversions(self):
        pair = (1, 2)
        pairTimesTwo = Tf.Tf_TestPyContainerConversions.GetPairTimesTwo(pair)
        self.assertEqual(pairTimesTwo, (2.0, 4.0))

    def test_VectorTokensConversions(self):
        tokens = ['my', 'list', 'of', 'tokens']
        self.assertEqual(tokens, Tf.Tf_TestPyContainerConversions.GetTokens(tokens))

        mixedList = ['a', 1, 'b', 2, dict()]
        with self.assertRaises(TypeError):
            Tf.Tf_TestPyContainerConversions.GetTokens(mixedList)

if __name__ == '__main__':
    unittest.main()

