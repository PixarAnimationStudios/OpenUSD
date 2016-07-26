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
from pxr import Tf
import unittest

########################################################################
# TfScopeDescription
class TestTfPyScopeDescription(unittest.TestCase):
    def test_TfScopeDescription(self):
        self.assertEqual(0, len(Tf.GetCurrentScopeDescriptionStack()))

        with Tf.ScopeDescription('one') as firstDescription:
            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('one', Tf.GetCurrentScopeDescriptionStack()[-1])

            with Tf.ScopeDescription('two'):
                self.assertEqual(2, len(Tf.GetCurrentScopeDescriptionStack()))
                self.assertEqual('two', Tf.GetCurrentScopeDescriptionStack()[-1])

            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('one', Tf.GetCurrentScopeDescriptionStack()[-1])

            with Tf.ScopeDescription('three'):
                self.assertEqual(2, len(Tf.GetCurrentScopeDescriptionStack()))
                self.assertEqual('three', Tf.GetCurrentScopeDescriptionStack()[-1])

            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('one', Tf.GetCurrentScopeDescriptionStack()[-1])

            firstDescription.SetDescription('different')
            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('different', Tf.GetCurrentScopeDescriptionStack()[-1])

        self.assertEqual(0, len(Tf.GetCurrentScopeDescriptionStack()))

if __name__ == '__main__':
    unittest.main()
