#!/pxrpythonsubst

import os, sys
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, AssertException,
                            ExpectedErrors, ExitTest)

from pxr import Gf, Tf, Sdf, Pcp, Usd

allFormats = ['usd' + x for x in 'abc']

class PayloadedScene(object):
    #
    # This class was stolen from testUsdStageLoadUnload.py
    #
    def __init__(self, fmt):
        # Construct the following test case:
        #
        # stage.fmt         payload1.fmt
        #   /Sad  ---(P)---> /Sad
        #   |                /Sad/Panda
        #   |
        #   |                   
        #   /Foo                payload2.fmt
        #   /Foo/Baz ---(P)---> /Baz                   payload3.fmt
        #                       /Baz/Garply ---(P)---> /Garply
        #                                              /Garply/Qux

        # Create payload1.fmt
        self.payload1 = Usd.Stage.CreateInMemory("payload1."+fmt)
        p = self.payload1.DefinePrim("/Sad/Panda", "Scope")

        # Create payload3.fmt
        self.payload3 = Usd.Stage.CreateInMemory("payload3."+fmt)
        p = self.payload3.DefinePrim("/Garply/Qux", "Scope")

        # Create payload2.fmt
        # Intentionally using the metadata API.
        self.payload2 = Usd.Stage.CreateInMemory("payload2."+fmt)
        p = self.payload2.DefinePrim("/Baz/Garply", "Scope")
        p.SetMetadata("payload", 
                      Sdf.Payload(self.payload3.GetRootLayer().identifier,
                                  "/Garply"))

        #
        # Create the scene that references payload1 and payload2
        #
        # Intentionally using the prim-payload API.
        self.stage = Usd.Stage.CreateInMemory("scene."+fmt)
        p = self.stage.DefinePrim("/Sad", "Scope")
        p.SetPayload(Sdf.Payload(self.payload1.GetRootLayer().identifier, "/Sad"))

        # Intentionally using the overloaded prim-payload API.
        p = self.stage.DefinePrim("/Foo/Baz", "Scope")
        p.SetPayload(self.payload2.GetRootLayer().identifier, "/Baz")

    def PrintPaths(self, msg=""):
        print("    Paths: "+msg)
        for p in self.stage.Traverse():
            print "    ", p
        print("")


def TestPayloads():
    for fmt in allFormats:
        p = PayloadedScene(fmt)

        # These assertions will be used several times.
        def AssertBaseAssumptions():
            assert not p.stage.GetPrimAtPath("/").HasPayload()
            assert p.stage.GetPrimAtPath("/Sad").HasPayload()
            assert p.stage.GetPrimAtPath("/Foo/Baz").HasPayload()
            assert p.stage.GetPrimAtPath("/Foo/Baz").GetPayload() == \
                Sdf.Payload(p.payload2.GetRootLayer().identifier, "/Baz")
            assert p.stage.GetPrimAtPath("/Sad").GetPayload() == \
                Sdf.Payload(p.payload1.GetRootLayer().identifier, "/Sad")

        AssertBaseAssumptions()

        #
        # The following prims must be loaded to be reachable.
        #
        p.stage.Load()
        assert not p.stage.GetPrimAtPath("/Sad/Panda").HasPayload()
        assert p.stage.GetPrimAtPath("/Foo/Baz/Garply").HasPayload()
        assert p.stage.GetPrimAtPath("/Foo/Baz/Garply").GetPayload() == \
                    Sdf.Payload(p.payload3.GetRootLayer().identifier, "/Garply")

        #
        # We do not expect the previous assertions to change based on load
        # state.
        #
        AssertBaseAssumptions()
        p.stage.Unload()
        AssertBaseAssumptions()


def TestClearPayload():
    for fmt in allFormats:
        p = PayloadedScene(fmt)
        p.stage.Load()

        # We expect these prims to survive the edits below, so hold weak ptrs to
        # them to verify this assumption.
        sad = p.stage.GetPrimAtPath("/Sad")
        baz = p.stage.GetPrimAtPath("/Foo/Baz")

        assert sad.HasPayload()
        assert baz.HasPayload()

        #
        # Try clearing the payload while the prim is unloaded.
        #
        assert sad.IsLoaded()
        assert sad.GetMetadata("payload")
        sad.Unload()
        assert sad.GetMetadata("payload")
        assert not sad.IsLoaded()
        assert sad.ClearPayload()
        assert not sad.HasPayload()
        assert sad.GetMetadata("payload") == None
        # Assert that it's loaded because anything without a payload is
        # considered loaded.
        assert sad.IsLoaded()

        #
        # Unload this one while it's loaded.
        #
        assert baz.IsLoaded()
        assert baz.GetMetadata("payload")
        assert baz.ClearPayload()
        assert not baz.HasPayload()
        assert baz.GetMetadata("payload") == None
        # Again, assert that it's loaded because anything without a payload is
        # considered loaded.
        assert baz.IsLoaded()

class InstancedAndPayloadedScene(PayloadedScene):
    def __init__(self, fmt):
        # Extend the PayloadedScene to add another Sad prim and
        # make them instanceable. This yields:
        #
        # stage.fmt                  payload1.fmt
        #   /__Master_<#A> ---(P)---> /Sad
        #   |                         /Sad/Panda
        #   |
        #   |                         payload2.fmt
        #   /__Master_<#B> ---(P)---> /Baz
        #   |                         /Baz/Garply ---(inst)---> /__Master_<#C>
        #   |
        #   |                         payload3.fmt
        #   /__Master_<#C> ---(P)---> /Garply
        #   |                         /Qux
        #   |
        #   /Sad   ---(instance)---> /__Master_<#A>
        #   /Sad_1 ---(instance)---> /__Master_<#A>
        #   |  
        #   /Foo
        #   /Foo/Baz   ---(instance)---> /__Master_<#B>
        #   /Foo/Baz_1 ---(instance)---> /__Master_<#B>

        super(InstancedAndPayloadedScene, self).__init__(fmt)

        sad = self.stage.GetPrimAtPath("/Sad");

        sad1 = self.stage.DefinePrim("/Sad_1", "Scope")
        sad1.SetPayload(sad.GetPayload())

        sad.SetInstanceable(True)
        sad1.SetInstanceable(True)

        baz = self.stage.GetPrimAtPath("/Foo/Baz")
        
        baz1 = self.stage.DefinePrim("/Foo/Baz_1", "Scope")
        baz1.SetPayload(baz.GetPayload())

        baz.SetInstanceable(True)
        baz1.SetInstanceable(True)

        # Declare /Baz/Garply as instanceable within the payload
        # itself.
        self.payload2.GetPrimAtPath("/Baz/Garply").SetInstanceable(True)

def TestInstancesWithPayloads():
    for fmt in allFormats:
        p = InstancedAndPayloadedScene(fmt)

        assert set(p.stage.FindLoadable()) == \
            set([Sdf.Path("/Sad"), Sdf.Path("/Sad_1"), 
                 Sdf.Path("/Foo/Baz"), Sdf.Path("/Foo/Baz_1")])

        sad = p.stage.GetPrimAtPath("/Sad")
        sad_1 = p.stage.GetPrimAtPath("/Sad_1")

        # Both instances should report that they have payloads authored
        # directly on them, but have not yet been loaded.
        assert sad.HasPayload()
        assert sad_1.HasPayload()
        assert not sad.IsLoaded()
        assert not sad_1.IsLoaded()

        # Since there is no composition arc to instanceable data
        # (due to the payloads not being loaded), both prims have no
        # master.
        assert not sad.GetMaster()
        assert not sad_1.GetMaster()

        # Instances should be independently loadable. This should
        # cause a master to be created for the loaded prim.
        sad.Load()
        assert sad.IsLoaded()
        assert not sad_1.IsLoaded()
        assert sad.GetMaster()
        assert not sad_1.GetMaster()

        # The master prim should not report that it has a loadable
        # payload. Its load state cannot be independently controlled.
        master = sad.GetMaster()
        assert not master.HasPayload()
        assert master not in p.stage.FindLoadable()
        assert [prim.GetName() for prim in master.GetChildren()] == ["Panda"]

        # Loading the second instance will cause Usd to assign it to the
        # first instance's master.
        sad_1.Load()
        assert sad.GetMaster() == sad_1.GetMaster()

        sad.Unload()
        sad_1.Unload()
        assert not sad.IsLoaded()
        assert not sad_1.IsLoaded()
        assert not sad.GetMaster()
        assert not sad_1.GetMaster()
        assert not master

        # Loading the payload for an instanceable prim will cause
        # payloads nested in descendants of that prim's master to be 
        # loaded as well.
        baz = p.stage.GetPrimAtPath("/Foo/Baz")
        baz_1 = p.stage.GetPrimAtPath("/Foo/Baz_1")

        baz.Load()
        assert baz.IsLoaded()
        assert baz.GetMaster()

        master = baz.GetMaster()
        assert [prim.GetName() for prim in master.GetChildren()] == ["Garply"]

        garply = master.GetChild("Garply")
        assert garply.HasPayload()
        assert garply.IsLoaded()
        assert garply.GetMaster()

        master = garply.GetMaster()
        assert [prim.GetName() for prim in master.GetChildren()] == ["Qux"]

        baz_1.Load()
        assert baz.GetMaster() == baz_1.GetMaster()

        # Nested payloads in masters can be individually unloaded. This
        # affects all instances.
        garply.Unload()
        assert not garply.IsLoaded()
        assert not garply.GetMaster()
        assert baz.GetMaster() == baz_1.GetMaster()
    

if __name__ == "__main__":
    TestPayloads();
    TestClearPayload();
    TestInstancesWithPayloads()
    print("OK")

