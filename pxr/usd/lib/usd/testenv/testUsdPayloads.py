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

import os, sys, unittest
from pxr import Gf, Tf, Sdf, Pcp, Usd

allFormats = ['usd' + x for x in 'ac']

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
        #   |                   /Baz/Garply ---(P)---> /Garply
        #   |                                          /Garply/Qux
        #   |               payload4.fmt
        #   |                /Corge
        #   /Bar ---(P)--->  /Corge/Waldo
        #   |                /Corge/Waldo/Fred
        #   |
        #   |                   payload5.fmt
        #   /Grizzly ---(P)---> /Bear
        #   |            |      /Bear/Market
        #   |            |
        #   |            |      payload6.fmt
        #   |           (P)---> /Adams
        #   |                   /Adams/Onis
        #   |
        #   /IntBase
        #   /IntBase/IntContents
        #   |
        #   /IntPayload ---(P) ---> /IntBase
        #                           /IntBase/IntContents

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
        payloadListOp = Sdf.PayloadListOp()
        payloadListOp.explicitItems = [
            Sdf.Payload(self.payload3.GetRootLayer().identifier, "/Garply")]
        p.SetMetadata("payload", payloadListOp)

        # Create payload4.fmt
        self.payload4 = Usd.Stage.CreateInMemory("payload4."+fmt)
        p = self.payload4.DefinePrim("/Corge/Waldo/Fred", "Scope")

        # Create payload5.fmt
        self.payload5 = Usd.Stage.CreateInMemory("payload5."+fmt)
        p = self.payload5.DefinePrim("/Bear/Market", "Scope")

        # Create payload6.fmt
        self.payload6 = Usd.Stage.CreateInMemory("payload6."+fmt)
        p = self.payload6.DefinePrim("/Adams/Onis", "Scope")

        #
        # Create the scene that references payload1 and payload2
        #
        # Intentionally using the prim-payload API.
        self.stage = Usd.Stage.CreateInMemory("scene."+fmt)
        p = self.stage.DefinePrim("/Sad", "Scope")
        p.GetPayloads().AddPayload(
            Sdf.Payload(self.payload1.GetRootLayer().identifier, "/Sad"))

        # Intentionally using the overloaded prim-payload API.
        p = self.stage.DefinePrim("/Foo/Baz", "Scope")
        p.GetPayloads().AddPayload(
            self.payload2.GetRootLayer().identifier, "/Baz")

        # Create a sub-root payload.
        p = self.stage.DefinePrim("/Bar", "Scope")
        p.GetPayloads().AddPayload(
            self.payload4.GetRootLayer().identifier, "/Corge/Waldo")

        # Create a list of payloads.
        p = self.stage.DefinePrim("/Grizzly", "Scope")
        p.GetPayloads().SetPayloads([
            Sdf.Payload(self.payload5.GetRootLayer().identifier, "/Bear"),
            Sdf.Payload(self.payload6.GetRootLayer().identifier, "/Adams")])

        p = self.stage.DefinePrim("/IntBase/IntContents", "Scope")
        p = self.stage.DefinePrim("/IntPayload", "Scope")
        p.GetPayloads().AddInternalPayload("/IntBase")

        # Test expects initial state to be all unloaded.
        self.stage.Unload()

    def PrintPaths(self, msg=""):
        print("    Paths: "+msg)
        for p in self.stage.Traverse():
            print "    ", p
        print("")

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
        #   |                         /Garply/Qux
        #   |
        #   |                         payload4.fmt
        #   |                         /Corge
        #   /__Master_<#D> ---(P)---> /Corge/Waldo
        #   |                         /Corge/Waldo/Fred
        #   |
        #   |                         payload5.fmt
        #   /__Master_<#E> ---(P)---> /Bear
        #   |                  |      /Bear/Market
        #   |                  |
        #   |                  |      payload6.fmt
        #   |                 (P)---> /Adams
        #   |                         /Adams/Onis
        #   |
        #   /IntBase
        #   /IntBase/IntContents
        #   |
        #   /__Master_<#F> ---(P) ---> /IntBase
        #                              /IntBase/IntContents
        #
        #   /Sad   ---(instance)---> /__Master_<#A>
        #   /Sad_1 ---(instance)---> /__Master_<#A>
        #   |  
        #   /Foo
        #   /Foo/Baz   ---(instance)---> /__Master_<#B>
        #   /Foo/Baz_1 ---(instance)---> /__Master_<#B>
        #   |
        #   /Bar   ---(instance)---> /__Master<#D>
        #   /Bar_1 ---(instance)---> /__Master<#D>
        #   |
        #   /Grizzly   ---(instance)---> /__Master<#E>
        #   /Grizzly_1 ---(instance)---> /__Master<#E>
        #   |
        #   /IntPayload   ---(instance)---> /__Master<#F>
        #   /IntPayload_1 ---(instance)---> /__Master<#F>


        super(InstancedAndPayloadedScene, self).__init__(fmt)

        sad = self.stage.GetPrimAtPath("/Sad")

        sad1 = self.stage.DefinePrim("/Sad_1", "Scope")
        sad1.GetPayloads().AddPayload(
            Sdf.Payload(self.payload1.GetRootLayer().identifier, "/Sad"))

        sad.SetInstanceable(True)
        sad1.SetInstanceable(True)

        baz = self.stage.GetPrimAtPath("/Foo/Baz")
        
        baz1 = self.stage.DefinePrim("/Foo/Baz_1", "Scope")
        baz1.GetPayloads().AddPayload(
            Sdf.Payload(self.payload2.GetRootLayer().identifier, "/Baz"))

        baz.SetInstanceable(True)
        baz1.SetInstanceable(True)

        bar = self.stage.GetPrimAtPath("/Bar")
        bar1 = self.stage.DefinePrim("/Bar_1", "Scope")
        bar1.GetPayloads().AddPayload(
            self.payload4.GetRootLayer().identifier, "/Corge/Waldo")
        bar.SetInstanceable(True)
        bar1.SetInstanceable(True)

        # Declare /Baz/Garply as instanceable within the payload
        # itself.
        self.payload2.GetPrimAtPath("/Baz/Garply").SetInstanceable(True)

        grizzly = self.stage.GetPrimAtPath("/Grizzly")

        grizzly1 = self.stage.DefinePrim("/Grizzly_1", "Scope")
        grizzly1.GetPayloads().SetPayloads([
            Sdf.Payload(self.payload5.GetRootLayer().identifier, "/Bear"),
            Sdf.Payload(self.payload6.GetRootLayer().identifier, "/Adams")])

        grizzly.SetInstanceable(True)
        grizzly1.SetInstanceable(True)

        intPayload = self.stage.GetPrimAtPath("/IntPayload")
        intPayload1 = self.stage.DefinePrim("/IntPayload_1", "Scope")
        intPayload1.GetPayloads().AddInternalPayload("/IntBase")
        intPayload.SetInstanceable(True)
        intPayload1.SetInstanceable(True)

        # Test expects initial state to be all unloaded.
        self.stage.Unload()

class TestUsdPayloads(unittest.TestCase):
    def test_InstancesWithPayloads(self):
        for fmt in allFormats:
            p = InstancedAndPayloadedScene(fmt)

            self.assertEqual(
                set(p.stage.FindLoadable()),
                set([Sdf.Path("/Sad"), Sdf.Path("/Sad_1"), 
                     Sdf.Path("/Foo/Baz"), Sdf.Path("/Foo/Baz_1"),
                     Sdf.Path("/Bar"), Sdf.Path("/Bar_1"),
                     Sdf.Path("/Grizzly"), Sdf.Path("/Grizzly_1"),
                     Sdf.Path("/IntPayload"), Sdf.Path("/IntPayload_1")]))

            sad = p.stage.GetPrimAtPath("/Sad")
            sad_1 = p.stage.GetPrimAtPath("/Sad_1")

            bar = p.stage.GetPrimAtPath("/Bar")
            bar_1 = p.stage.GetPrimAtPath("/Bar_1")

            grizzly = p.stage.GetPrimAtPath("/Grizzly")
            grizzly_1 = p.stage.GetPrimAtPath("/Grizzly_1")

            intPayload = p.stage.GetPrimAtPath("/IntPayload")
            intPayload_1 = p.stage.GetPrimAtPath("/IntPayload_1")

            # All instances should report that they have payloads authored
            # directly on them, but have not yet been loaded.
            self.assertTrue(sad.HasAuthoredPayloads())
            self.assertTrue(sad_1.HasAuthoredPayloads())
            self.assertTrue(not sad.IsLoaded())
            self.assertTrue(not sad_1.IsLoaded())

            self.assertTrue(bar.HasAuthoredPayloads())
            self.assertTrue(bar_1.HasAuthoredPayloads())
            self.assertTrue(not bar.IsLoaded())
            self.assertTrue(not bar_1.IsLoaded())

            self.assertTrue(grizzly.HasAuthoredPayloads())
            self.assertTrue(grizzly_1.HasAuthoredPayloads())
            self.assertTrue(not grizzly.IsLoaded())
            self.assertTrue(not grizzly_1.IsLoaded())

            self.assertTrue(intPayload.HasAuthoredPayloads())
            self.assertTrue(intPayload_1.HasAuthoredPayloads())
            self.assertTrue(not intPayload.IsLoaded())
            self.assertTrue(not intPayload_1.IsLoaded())

            # Since there is no composition arc to instanceable data
            # (due to the payloads not being loaded), these prims have no
            # master.
            self.assertTrue(not sad.GetMaster())
            self.assertTrue(not sad_1.GetMaster())

            self.assertTrue(not bar.GetMaster())
            self.assertTrue(not bar_1.GetMaster())

            self.assertTrue(not grizzly.GetMaster())
            self.assertTrue(not grizzly_1.GetMaster())

            self.assertTrue(not intPayload.GetMaster())
            self.assertTrue(not intPayload_1.GetMaster())

            # Instances should be independently loadable. This should
            # cause a master to be created for the loaded prim.
            sad.Load()
            self.assertTrue(sad.IsLoaded())
            self.assertTrue(not sad_1.IsLoaded())
            self.assertTrue(sad.GetMaster())
            self.assertTrue(not sad_1.GetMaster())

            bar.Load()
            self.assertTrue(bar.IsLoaded())
            self.assertTrue(not bar_1.IsLoaded())
            self.assertTrue(bar.GetMaster())
            self.assertTrue(not bar_1.GetMaster())

            grizzly.Load()
            self.assertTrue(grizzly.IsLoaded())
            self.assertTrue(not grizzly_1.IsLoaded())
            self.assertTrue(grizzly.GetMaster())
            self.assertTrue(not grizzly_1.GetMaster())

            intPayload.Load()
            self.assertTrue(intPayload.IsLoaded())
            self.assertTrue(not intPayload_1.IsLoaded())
            self.assertTrue(intPayload.GetMaster())
            self.assertTrue(not intPayload_1.GetMaster())

            # The master prim should not report that it has a loadable
            # payload. Its load state cannot be independently controlled.
            master = sad.GetMaster()
            self.assertTrue(not master.HasAuthoredPayloads())
            self.assertTrue(master not in p.stage.FindLoadable())
            self.assertEqual([prim.GetName() for prim in master.GetChildren()],
                             ["Panda"])

            master2 = bar.GetMaster()
            self.assertTrue(not master2.HasAuthoredPayloads())
            self.assertTrue(master2 not in p.stage.FindLoadable())
            self.assertEqual([prim.GetName() for prim in master2.GetChildren()],
                             ["Fred"])

            master3 = grizzly.GetMaster()
            self.assertTrue(not master3.HasAuthoredPayloads())
            self.assertTrue(master3 not in p.stage.FindLoadable())
            self.assertEqual([prim.GetName() for prim in master3.GetChildren()],
                             ["Onis", "Market"])

            master4 = intPayload.GetMaster()
            self.assertTrue(not master4.HasAuthoredPayloads())
            self.assertTrue(master4 not in p.stage.FindLoadable())
            self.assertEqual([prim.GetName() for prim in master4.GetChildren()],
                             ["IntContents"])

            # Loading the second instance will cause Usd to assign it to the
            # first instance's master.
            sad_1.Load()
            self.assertEqual(sad.GetMaster(), sad_1.GetMaster())

            sad.Unload()
            sad_1.Unload()
            self.assertTrue(not sad.IsLoaded())
            self.assertTrue(not sad_1.IsLoaded())
            self.assertTrue(not sad.GetMaster())
            self.assertTrue(not sad_1.GetMaster())
            self.assertTrue(not master)

            bar_1.Load()
            self.assertEqual(bar.GetMaster(), bar_1.GetMaster())

            bar.Unload()
            bar_1.Unload()
            self.assertTrue(not bar.IsLoaded())
            self.assertTrue(not bar_1.IsLoaded())
            self.assertTrue(not bar.GetMaster())
            self.assertTrue(not bar_1.GetMaster())
            self.assertTrue(not master2)

            grizzly_1.Load()
            self.assertEqual(grizzly.GetMaster(), grizzly_1.GetMaster())

            grizzly.Unload()
            grizzly_1.Unload()
            self.assertTrue(not grizzly.IsLoaded())
            self.assertTrue(not grizzly_1.IsLoaded())
            self.assertTrue(not grizzly.GetMaster())
            self.assertTrue(not grizzly_1.GetMaster())
            self.assertTrue(not master3)

            intPayload_1.Load()
            self.assertEqual(intPayload.GetMaster(), intPayload_1.GetMaster())

            intPayload.Unload()
            intPayload_1.Unload()
            self.assertTrue(not intPayload.IsLoaded())
            self.assertTrue(not intPayload_1.IsLoaded())
            self.assertTrue(not intPayload.GetMaster())
            self.assertTrue(not intPayload_1.GetMaster())
            self.assertTrue(not master4)

            # Loading the payload for an instanceable prim will cause
            # payloads nested in descendants of that prim's master to be 
            # loaded as well.
            baz = p.stage.GetPrimAtPath("/Foo/Baz")
            baz_1 = p.stage.GetPrimAtPath("/Foo/Baz_1")

            baz.Load()
            self.assertTrue(baz.IsLoaded())
            self.assertTrue(baz.GetMaster())

            master = baz.GetMaster()
            self.assertEqual(
                [prim.GetName() for prim in master.GetChildren()], ["Garply"])

            garply = master.GetChild("Garply")
            self.assertTrue(garply.HasAuthoredPayloads())
            self.assertTrue(garply.IsLoaded())
            self.assertTrue(garply.GetMaster())

            master = garply.GetMaster()
            self.assertEqual([prim.GetName() for prim in master.GetChildren()],
                             ["Qux"])

            baz_1.Load()
            self.assertEqual(baz.GetMaster(), baz_1.GetMaster())

            # Nested payloads in masters can be individually unloaded. This
            # affects all instances.
            garply.Unload()
            self.assertTrue(not garply.IsLoaded())
            self.assertTrue(not garply.GetMaster())
            self.assertEqual(baz.GetMaster(), baz_1.GetMaster())

    def test_Payloads(self):
        for fmt in allFormats:
            p = PayloadedScene(fmt)

            # These assertions will be used several times.
            def AssertBaseAssumptions():
                self.assertTrue(
                    not p.stage.GetPrimAtPath("/").HasAuthoredPayloads())
                self.assertTrue(
                    p.stage.GetPrimAtPath("/Sad").HasAuthoredPayloads())
                self.assertTrue(
                    p.stage.GetPrimAtPath("/Foo/Baz").HasAuthoredPayloads())
                self.assertTrue(
                    p.stage.GetPrimAtPath("/Grizzly").HasAuthoredPayloads())
                self.assertTrue(
                    p.stage.GetPrimAtPath("/IntPayload").HasAuthoredPayloads())
                self.assertEqual(
                    p.stage.GetPrimAtPath("/Foo/Baz").\
                        GetMetadata("payload").prependedItems,
                    [Sdf.Payload(p.payload2.GetRootLayer().identifier, "/Baz")])
                self.assertEqual(
                    p.stage.GetPrimAtPath("/Sad").\
                        GetMetadata("payload").prependedItems,
                    [Sdf.Payload(p.payload1.GetRootLayer().identifier, "/Sad")])
                self.assertEqual(
                    p.stage.GetPrimAtPath("/Grizzly").\
                        GetMetadata("payload").explicitItems,
                    [Sdf.Payload(p.payload5.GetRootLayer().identifier, "/Bear"),
                     Sdf.Payload(p.payload6.GetRootLayer().identifier, "/Adams")])
                self.assertEqual(
                    p.stage.GetPrimAtPath("/IntPayload").\
                        GetMetadata("payload").prependedItems,
                    [Sdf.Payload("", "/IntBase")])

            AssertBaseAssumptions()

            #
            # The following prims must be loaded to be reachable.
            #
            p.stage.Load()
            self.assertTrue(
                not p.stage.GetPrimAtPath("/Sad/Panda").HasAuthoredPayloads())
            self.assertTrue(
                p.stage.GetPrimAtPath("/Foo/Baz/Garply").HasAuthoredPayloads())
            self.assertEqual(
                p.stage.GetPrimAtPath("/Foo/Baz/Garply").\
                    GetMetadata("payload").explicitItems,
                [Sdf.Payload(p.payload3.GetRootLayer().identifier, "/Garply")])
            self.assertTrue(not p.stage.GetPrimAtPath("/Grizzly/Market").\
                                HasAuthoredPayloads())
            self.assertTrue(not p.stage.GetPrimAtPath("/Grizzly/Onis").\
                                HasAuthoredPayloads())
            self.assertTrue(not p.stage.GetPrimAtPath("/IntPayload/IntContents").\
                                HasAuthoredPayloads())

            #
            # We do not expect the previous assertions to change based on load
            # state.
            #
            AssertBaseAssumptions()
            p.stage.Unload()
            AssertBaseAssumptions()


    def test_ClearPayload(self):
        for fmt in allFormats:
            p = PayloadedScene(fmt)
            p.stage.Load()

            # We expect these prims to survive the edits below, so hold weak 
            # ptrs to them to verify this assumption.
            sad = p.stage.GetPrimAtPath("/Sad")
            baz = p.stage.GetPrimAtPath("/Foo/Baz")
            grizzly = p.stage.GetPrimAtPath("/Grizzly")
            intPayload = p.stage.GetPrimAtPath("/IntPayload")

            self.assertTrue(sad.HasAuthoredPayloads())
            self.assertTrue(baz.HasAuthoredPayloads())
            self.assertTrue(grizzly.HasAuthoredPayloads())
            self.assertTrue(intPayload.HasAuthoredPayloads())

            #
            # Try clearing the payload while the prim is unloaded.
            #
            self.assertTrue(sad.IsLoaded())
            self.assertTrue(sad.GetMetadata("payload"))
            sad.Unload()
            self.assertTrue(sad.GetMetadata("payload"))
            self.assertTrue(not sad.IsLoaded())
            self.assertTrue(sad.GetPayloads().ClearPayloads())
            self.assertTrue(not sad.HasAuthoredPayloads())
            self.assertEqual(sad.GetMetadata("payload"), None)
            # Assert that it's loaded because anything without a payload is
            # considered loaded.
            self.assertTrue(sad.IsLoaded())

            #
            # Unload this one while it's loaded.
            #
            self.assertTrue(baz.IsLoaded())
            self.assertTrue(baz.GetMetadata("payload"))
            self.assertTrue(baz.GetPayloads().ClearPayloads())
            self.assertTrue(not baz.HasAuthoredPayloads())
            self.assertEqual(baz.GetMetadata("payload"), None)
            # Again, assert that it's loaded because anything without a payload 
            # is considered loaded.
            self.assertTrue(baz.IsLoaded())

            #
            # Unload this one while it's loaded.
            #
            self.assertTrue(grizzly.IsLoaded())
            self.assertTrue(grizzly.GetMetadata("payload"))
            self.assertTrue(grizzly.GetPayloads().ClearPayloads())
            self.assertTrue(not grizzly.HasAuthoredPayloads())
            self.assertEqual(grizzly.GetMetadata("payload"), None)
            # Again, assert that it's loaded because anything without a payload 
            # is considered loaded.
            self.assertTrue(grizzly.IsLoaded())

            #
            # Unload this one while it's loaded.
            #
            self.assertTrue(intPayload.IsLoaded())
            self.assertTrue(intPayload.GetMetadata("payload"))
            self.assertTrue(intPayload.GetPayloads().ClearPayloads())
            self.assertTrue(not intPayload.HasAuthoredPayloads())
            self.assertEqual(intPayload.GetMetadata("payload"), None)
            # Again, assert that it's loaded because anything without a payload 
            # is considered loaded.
            self.assertTrue(intPayload.IsLoaded())

    def test_Bug160419(self):
        for fmt in allFormats:
            payloadLayer = Sdf.Layer.CreateAnonymous("payload."+fmt)
            Sdf.CreatePrimInLayer(payloadLayer, "/Payload/Cube")

            rootLayer = Sdf.Layer.CreateAnonymous("root."+fmt)
            refPrim = Sdf.PrimSpec(rootLayer, "Ref", Sdf.SpecifierDef)
            refPrim = Sdf.PrimSpec(refPrim, "Child", Sdf.SpecifierDef)
            refPrim.payloadList.Prepend(
                Sdf.Payload(payloadLayer.identifier, "/Payload"))

            rootPrim = Sdf.PrimSpec(rootLayer, "Root", Sdf.SpecifierDef)
            rootPrim.referenceList.Prepend(
                Sdf.Reference(primPath="/Ref/Child"))

            stage = Usd.Stage.Open(rootLayer)
            self.assertEqual(set(stage.GetLoadSet()),
                             set([Sdf.Path("/Ref/Child"), Sdf.Path("/Root")]))
            self.assertTrue(stage.GetPrimAtPath("/Root").IsLoaded())
            self.assertTrue(stage.GetPrimAtPath("/Ref/Child").IsLoaded())

if __name__ == "__main__":
    unittest.main()
