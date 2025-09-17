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
    def test_TfScopeDescriptionContextManager(self):
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

    def test_TfScopeDescriptionDecorator(self):
        @Tf.ScopeDescription('outer')
        def Outer():
            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('outer', Tf.GetCurrentScopeDescriptionStack()[-1])

            Inner()

            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('outer', Tf.GetCurrentScopeDescriptionStack()[-1])

            Inner()

            self.assertEqual(1, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('outer', Tf.GetCurrentScopeDescriptionStack()[-1])

        @Tf.ScopeDescription('inner')
        def Inner():
            self.assertEqual(2, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('inner', Tf.GetCurrentScopeDescriptionStack()[-1])

            with Tf.ScopeDescription('mixed'):
                self.assertEqual(3, len(Tf.GetCurrentScopeDescriptionStack()))
                self.assertEqual('mixed', Tf.GetCurrentScopeDescriptionStack()[-1])

            self.assertEqual(2, len(Tf.GetCurrentScopeDescriptionStack()))
            self.assertEqual('inner', Tf.GetCurrentScopeDescriptionStack()[-1])

        self.assertEqual(0, len(Tf.GetCurrentScopeDescriptionStack()))
        Outer()
        self.assertEqual(0, len(Tf.GetCurrentScopeDescriptionStack()))

if __name__ == '__main__':
    unittest.main()
