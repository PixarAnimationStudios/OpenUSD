#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import Sdf
import unittest

class TestSdfLayerMuting(unittest.TestCase):
    
    def test_UnmuteMutedLayer(self):
        """Unmuting an initially muted layer should unmute the
           layer and load the file."""

        pathA = 'testSdfLayerMuting.testenv/a.sdf'
        layerA = Sdf.Layer.FindOrOpen(pathA)
        self.assertTrue(layerA is not None)
        self.assertFalse(layerA.IsMuted())
        self.assertFalse(layerA.empty)
        self.assertTrue(layerA.GetPrimAtPath('/Test') is not None)

        pathB = 'testSdfLayerMuting.testenv/b.sdf'

        # Mute layer B before opening it
        Sdf.Layer.AddToMutedLayers(pathB)

        layerB = Sdf.Layer.FindOrOpen(pathB)
        self.assertTrue(layerB is not None)
        self.assertTrue(layerB.IsMuted())
        self.assertTrue(layerB.empty)
        self.assertTrue(layerB.GetPrimAtPath('/Test') is None)

        # Now mute layer A and unmute layer B
        Sdf.Layer.AddToMutedLayers(pathA)
        Sdf.Layer.RemoveFromMutedLayers(pathB)

        self.assertTrue(layerA.IsMuted())
        self.assertTrue(layerA.empty)
        self.assertTrue(layerA.GetPrimAtPath('/Test') is None)

        self.assertFalse(layerB.IsMuted())
        self.assertFalse(layerB.empty)
        self.assertTrue(layerB.GetPrimAtPath('/Test') is not None)
        
if __name__ == '__main__':
    unittest.main()
