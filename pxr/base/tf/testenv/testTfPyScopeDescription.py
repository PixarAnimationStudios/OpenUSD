#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
