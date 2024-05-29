#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Usd, Tf

class TestPrimFlagsPredicate(unittest.TestCase):
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory('primFlags.usd')
        self.stage.DefinePrim("/Parent")
        self.stage.OverridePrim("/Parent/Child")
    
    def testNullPrim(self):
        '''
        Invalid prims raise an exception.
        '''
        with self.assertRaises(Tf.ErrorException):
            Usd.PrimDefaultPredicate(Usd.Prim())
    
    def testSimpleParentChild(self):
        '''
        Test absent child of a defined parent
        '''
        self.assertFalse(Usd.PrimDefaultPredicate(self.stage.GetPrimAtPath('/Parent/Child')))
        self.assertTrue(Usd.PrimDefaultPredicate(self.stage.GetPrimAtPath('/Parent')))

if __name__ == "__main__":
    unittest.main()
