#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Tf, Pcp, Sdf
import os, sys, unittest

class TestPcpOwner(unittest.TestCase):
    def setUp(self):
        """Automatically invoked once before any test cases are run."""

        # Find the sibling sublayers that contain value opinions.
        self.animLayer     = Sdf.Layer.FindOrOpen("anim.sdf")
        self.strongerLayer = Sdf.Layer.FindOrOpen("stronger.sdf")
        self.userLayer     = Sdf.Layer.FindOrOpen("owned.sdf")
        self.weakerLayer   = Sdf.Layer.FindOrOpen("weaker.sdf")

        self.assertTrue(self.strongerLayer)
        self.assertTrue(self.userLayer)
        self.assertTrue(self.weakerLayer)

        # Open the shot, and ensure that it has a session layer.
        self.rootLayer = Sdf.Layer.FindOrOpen("shot.sdf")
        self.assertTrue(self.rootLayer)

        self.sessionLayer = Sdf.Layer.CreateAnonymous("session")
        self.assertTrue(self.sessionLayer)
        self.sessionLayer.sessionOwner = "testUser"
        self.assertTrue(self.sessionLayer.sessionOwner)

        self.layerStackId = \
            Pcp.LayerStackIdentifier( self.rootLayer, self.sessionLayer )

    def test_OwnerBasics(self):
        """Test basic composition behaviors related to layer ownership.

        Note that we don't normally expect these values to be changed at
        runtime, but the system should still respond correctly."""

        def TestResolve(expectedSourceLayer):
            path = "/Sphere.radius"

            # XXX: We create the cache from scratch each time because
            #      Pcp doesn't yet respond to Sd notices (regarding
            #      changes to layer ownership).  When it does we
            #      should create the cache and layer stack in
            #      ClassSetup() and hold them as properties on self.
            cache = Pcp.Cache(self.layerStackId)
            self.assertTrue(cache)
            (layerStack, errors) = cache.ComputeLayerStack(self.layerStackId)
            self.assertEqual(len(errors), 0)
            self.assertTrue(layerStack)

            # Find the strongest layer with an opinion about the radius
            # default.
            strongestLayer = None
            for layer in layerStack.layers:
                radiusAttr = layer.GetObjectAtPath(path)
                if radiusAttr and radiusAttr.default:
                    strongestLayer = layer
                    break

            # Ensure the strongest layer with an opinion is the
            # expectedSourceLayer
            self.assertEqual(strongestLayer, expectedSourceLayer)

        # Verify that this field was read correctly, then clear it.
        self.assertEqual(self.userLayer.owner, "_PLACEHOLDER_")
        self.userLayer.owner = ""

        # Initially, the strongest layer's opinion should win, as usual.
        TestResolve(self.strongerLayer)

        # Make the owner of the "owned" layer the same as the owner denoted by
        # the session layer. Now the "owned" layer's opinion should win.
        self.userLayer.owner = self.sessionLayer.sessionOwner
        TestResolve(self.userLayer)

        # If we change the session's owner, the "stronger" layer should win
        # once again. Setting it back should produce the previous result.
        self.sessionLayer.sessionOwner = "_bogus_user_"
        TestResolve(self.strongerLayer)

        self.sessionLayer.sessionOwner = self.userLayer.owner
        TestResolve(self.userLayer)

        # Similarly, changing the owner of a sublayer should also let the
        # "stronger" layer win once again.
        self.userLayer.owner = "_other_bogus_user_"
        TestResolve(self.strongerLayer)

        self.userLayer.owner = self.sessionLayer.sessionOwner
        TestResolve(self.userLayer)

        # Changing both the session owner and the layer's owner to another
        # value should still produce the expected result.
        self.sessionLayer.sessionOwner = "_foo_"
        self.weakerLayer.owner = "_foo_"
        TestResolve(self.weakerLayer)

        # Changing the required "hasOwnedSubLayers" bit on the parent layer
        # (anim) should affect the results.
        self.assertTrue(self.animLayer.hasOwnedSubLayers)
        self.animLayer.hasOwnedSubLayers = False
        self.assertTrue(not self.animLayer.hasOwnedSubLayers)
        TestResolve(self.strongerLayer)

        self.animLayer.hasOwnedSubLayers = True
        self.assertTrue(self.animLayer.hasOwnedSubLayers)
        TestResolve(self.weakerLayer)

        # Having more than one sibling layer with the same owner should
        # be detected as an error.
        self.sessionLayer.sessionOwner = "_foo_"
        self.weakerLayer.owner = "_foo_"
        self.strongerLayer.owner = "_foo_"
        # XXX: Force the layer stack creation because Pcp doesn't
        #      respond to Sd notices.
        cache = Pcp.Cache(self.layerStackId)
        self.assertTrue(cache)
        (layerStack, errors) = cache.ComputeLayerStack(self.layerStackId)

        for err in errors:
            print >> sys.stderr, err
            self.assertTrue(isinstance(err, Pcp.ErrorInvalidSublayerOwnership),
                   "Unexpected Error: %s" % err)

        self.assertEqual(len(errors), 1)

if __name__ == "__main__":
    unittest.main()
