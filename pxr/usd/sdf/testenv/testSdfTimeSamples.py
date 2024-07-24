#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Sdf

class TestSdfTimeSampleOrdering(unittest.TestCase):
    layerText = '''#sdf 1.4.32
def "Prim"{
    int i.timeSamples = {-1 : -10, 0 : 0, 1 : 10}
}
'''
    def test_negativeTimeSamples(self):
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString(self.layerText)
        primSpec = layer.GetPrimAtPath('/Prim')
        attributeSpec = primSpec.attributes['i']
        timeSamples = attributeSpec.GetInfo('timeSamples')
        self.assertEqual(list(timeSamples), sorted(list(timeSamples)))
        
if __name__ == '__main__':
    unittest.main()    
