#!/pxrpythonsubst

from pxr import Tf, Pcp, Sdf
from Mentor.Runtime import (Assert, AssertEqual, ExpectedErrors,
                            FindDataFile, Fixture, MTR_EXIT_TEST,
                            SetAssertMode)

import os
import sys

# The subdirectory that contains test resources.
TEST_DIR = "testPcpOwner.testenv"

# Assertion failures should terminate the test.
SetAssertMode(MTR_EXIT_TEST)

class TestPcpOwner(Fixture):
    def ClassSetup(self):
        """Automatically invoked once before any test cases are run."""

        def OpenLayer(name):
            path = FindDataFile(os.path.join(TEST_DIR, name))
            return Sdf.Layer.FindOrOpen(path)

        # Find the sibling sublayers that contain value opinions.
        self.animLayer     = OpenLayer("anim.usda")
        self.strongerLayer = OpenLayer("stronger.usda")
        self.userLayer     = OpenLayer("owned.usda")
        self.weakerLayer   = OpenLayer("weaker.usda")

        Assert(self.strongerLayer)
        Assert(self.userLayer)
        Assert(self.weakerLayer)

        # Open the shot, and ensure that it has a session layer.
        rootLayerPath = FindDataFile(os.path.join(TEST_DIR, "shot.usda"))
        self.rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
        Assert(self.rootLayer)

        self.sessionLayer = Sdf.Layer.CreateAnonymous("session")
        Assert(self.sessionLayer)
        self.sessionLayer.sessionOwner = "testUser"
        Assert(self.sessionLayer.sessionOwner)

        self.layerStackId = \
            Pcp.LayerStackIdentifier( self.rootLayer, self.sessionLayer )

    def TestOwnerBasics(self):
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
            Assert(cache)
            (layerStack, errors) = cache.ComputeLayerStack(self.layerStackId)
            AssertEqual(len(errors), 0)
            Assert(layerStack)

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
            AssertEqual(strongestLayer, expectedSourceLayer)

        # Verify that this field was read correctly, then clear it.
        AssertEqual(self.userLayer.owner, "_PLACEHOLDER_")
        self.userLayer.owner = ""

        # Initially, the strongest layer's opinion should win, as usual.
        TestResolve(self.strongerLayer)

        # Make the owner of the "owned" layer the same as the owner denoted by
        # the session layer. Now the "owned" layer's opinion should win.
        self.userLayer.owner = self.sessionLayer.sessionOwner;
        TestResolve(self.userLayer)

        # If we change the session's owner, the "stronger" layer should win
        # once again. Setting it back should produce the previous result.
        self.sessionLayer.sessionOwner = "_bogus_user_"
        TestResolve(self.strongerLayer)

        self.sessionLayer.sessionOwner = self.userLayer.owner;
        TestResolve(self.userLayer)

        # Similarly, changing the owner of a sublayer should also let the
        # "stronger" layer win once again.
        self.userLayer.owner = "_other_bogus_user_"
        TestResolve(self.strongerLayer)

        self.userLayer.owner = self.sessionLayer.sessionOwner;
        TestResolve(self.userLayer)

        # Changing both the session owner and the layer's owner to another
        # value should still produce the expected result.
        self.sessionLayer.sessionOwner = "_foo_"
        self.weakerLayer.owner = "_foo_"
        TestResolve(self.weakerLayer)

        # Changing the required "hasOwnedSubLayers" bit on the parent layer
        # (anim) should affect the results.
        Assert(self.animLayer.hasOwnedSubLayers)
        self.animLayer.hasOwnedSubLayers = False
        Assert(not self.animLayer.hasOwnedSubLayers)
        TestResolve(self.strongerLayer)

        self.animLayer.hasOwnedSubLayers = True
        Assert(self.animLayer.hasOwnedSubLayers)
        TestResolve(self.weakerLayer)

        # Having more than one sibling layer with the same owner should
        # be detected as an error.
        self.sessionLayer.sessionOwner = "_foo_"
        self.weakerLayer.owner = "_foo_"
        self.strongerLayer.owner = "_foo_"
        # XXX: Force the layer stack creation because Pcp doesn't
        #      respond to Sd notices.
        cache = Pcp.Cache(self.layerStackId)
        Assert(cache)
        (layerStack, errors) = cache.ComputeLayerStack(self.layerStackId)

        for err in errors:
            print >> sys.stderr, err
            Assert(isinstance(err, Pcp.ErrorInvalidSublayerOwnership),
                   "Unexpected Error: %s" % err)

        AssertEqual(len(errors), 1)
            

if __name__ == "__main__":
    from Mentor.Runtime import Runner
    Runner().Main()
