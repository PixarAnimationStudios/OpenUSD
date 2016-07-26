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
import unittest

class TestPyContainerConversions(unittest.TestCase):
    def test_VectorConversions(self):
        vec = (1, 2, 3)
        vecTimesTwo = Tf.Tf_TestPyContainerConversions.GetVectorTimesTwo(vec)
        self.assertEqual(vecTimesTwo, [2.0, 4.0, 6.0])

    def test_VectorConversions(self):
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

