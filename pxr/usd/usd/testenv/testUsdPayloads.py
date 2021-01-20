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

from __future__ import print_function

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
            print("    ", p)
        print("")

class InstancedAndPayloadedScene(PayloadedScene):
    def __init__(self, fmt):
        # Extend the PayloadedScene to add another Sad prim and
        # make them instanceable. This yields:
        #
        # stage.fmt                  payload1.fmt
        #   /__Prototype_<#A> --(P)--> /Sad
        #   |                          /Sad/Panda
        #   |
        #   |                         payload2.fmt
        #   /__Prototype_<#B> --(P)--> /Baz
        #   |                          /Baz/Garply --(inst)--> /__Prototype_<#C>
        #   |
        #   |                         payload3.fmt
        #   /__Prototype_<#C> --(P)--> /Garply
        #   |                          /Garply/Qux
        #   |
        #   |                         payload4.fmt
        #   |                          /Corge
        #   /__Prototype_<#D> --(P)--> /Corge/Waldo
        #   |                          /Corge/Waldo/Fred
        #   |
        #   |                          payload5.fmt
        #   /__Prototype_<#E> --(P)--> /Bear
        #   |                    |     /Bear/Market
        #   |                    |
        #   |                    |     payload6.fmt
        #   |                   (P)--> /Adams
        #   |                          /Adams/Onis
        #   |
        #   /IntBase
        #   /IntBase/IntContents
        #   |
        #   /__Prototype_<#F> --(P) --> /IntBase
        #                               /IntBase/IntContents
        #
        #   /Sad   --(instance)--> /__Prototype_<#A>
        #   /Sad_1 --(instance)--> /__Prototype_<#A>
        #   |  
        #   /Foo
        #   /Foo/Baz   --(instance)--> /__Prototype_<#B>
        #   /Foo/Baz_1 --(instance)--> /__Prototype_<#B>
        #   |
        #   /Bar   --(instance)--> /__Prototype<#D>
        #   /Bar_1 --(instance)--> /__Prototype<#D>
        #   |
        #   /Grizzly   --(instance)--> /__Prototype<#E>
        #   /Grizzly_1 --(instance)--> /__Prototype<#E>
        #   |
        #   /IntPayload   --(instance)--> /__Prototype<#F>
        #   /IntPayload_1 --(instance)--> /__Prototype<#F>


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
            # prototype.
            self.assertTrue(not sad.GetPrototype())
            self.assertTrue(not sad_1.GetPrototype())

            self.assertTrue(not bar.GetPrototype())
            self.assertTrue(not bar_1.GetPrototype())

            self.assertTrue(not grizzly.GetPrototype())
            self.assertTrue(not grizzly_1.GetPrototype())

            self.assertTrue(not intPayload.GetPrototype())
            self.assertTrue(not intPayload_1.GetPrototype())

            # Instances should be independently loadable. This should
            # cause a prototype to be created for the loaded prim.
            sad.Load()
            self.assertTrue(sad.IsLoaded())
            self.assertTrue(not sad_1.IsLoaded())
            self.assertTrue(sad.GetPrototype())
            self.assertTrue(not sad_1.GetPrototype())

            bar.Load()
            self.assertTrue(bar.IsLoaded())
            self.assertTrue(not bar_1.IsLoaded())
            self.assertTrue(bar.GetPrototype())
            self.assertTrue(not bar_1.GetPrototype())

            grizzly.Load()
            self.assertTrue(grizzly.IsLoaded())
            self.assertTrue(not grizzly_1.IsLoaded())
            self.assertTrue(grizzly.GetPrototype())
            self.assertTrue(not grizzly_1.GetPrototype())

            intPayload.Load()
            self.assertTrue(intPayload.IsLoaded())
            self.assertTrue(not intPayload_1.IsLoaded())
            self.assertTrue(intPayload.GetPrototype())
            self.assertTrue(not intPayload_1.GetPrototype())

            # The prototype prim should not report that it has a loadable
            # payload. Its load state cannot be independently controlled.
            prototype = sad.GetPrototype()
            self.assertTrue(not prototype.HasAuthoredPayloads())
            self.assertTrue(prototype not in p.stage.FindLoadable())
            self.assertEqual(
                [prim.GetName() for prim in prototype.GetChildren()],
                ["Panda"])

            prototype2 = bar.GetPrototype()
            self.assertTrue(not prototype2.HasAuthoredPayloads())
            self.assertTrue(prototype2 not in p.stage.FindLoadable())
            self.assertEqual(
                [prim.GetName() for prim in prototype2.GetChildren()],
                ["Fred"])

            prototype3 = grizzly.GetPrototype()
            self.assertTrue(not prototype3.HasAuthoredPayloads())
            self.assertTrue(prototype3 not in p.stage.FindLoadable())
            self.assertEqual(
                [prim.GetName() for prim in prototype3.GetChildren()],
                ["Onis", "Market"])

            prototype4 = intPayload.GetPrototype()
            self.assertTrue(not prototype4.HasAuthoredPayloads())
            self.assertTrue(prototype4 not in p.stage.FindLoadable())
            self.assertEqual(
                [prim.GetName() for prim in prototype4.GetChildren()],
                ["IntContents"])

            # Loading the second instance will cause Usd to assign it to the
            # first instance's prototype.
            sad_1.Load()
            self.assertEqual(sad.GetPrototype(), sad_1.GetPrototype())

            sad.Unload()
            sad_1.Unload()
            self.assertTrue(not sad.IsLoaded())
            self.assertTrue(not sad_1.IsLoaded())
            self.assertTrue(not sad.GetPrototype())
            self.assertTrue(not sad_1.GetPrototype())
            self.assertTrue(not prototype)

            bar_1.Load()
            self.assertEqual(bar.GetPrototype(), bar_1.GetPrototype())

            bar.Unload()
            bar_1.Unload()
            self.assertTrue(not bar.IsLoaded())
            self.assertTrue(not bar_1.IsLoaded())
            self.assertTrue(not bar.GetPrototype())
            self.assertTrue(not bar_1.GetPrototype())
            self.assertTrue(not prototype2)

            grizzly_1.Load()
            self.assertEqual(grizzly.GetPrototype(), grizzly_1.GetPrototype())

            grizzly.Unload()
            grizzly_1.Unload()
            self.assertTrue(not grizzly.IsLoaded())
            self.assertTrue(not grizzly_1.IsLoaded())
            self.assertTrue(not grizzly.GetPrototype())
            self.assertTrue(not grizzly_1.GetPrototype())
            self.assertTrue(not prototype3)

            intPayload_1.Load()
            self.assertEqual(intPayload.GetPrototype(),
                             intPayload_1.GetPrototype())

            intPayload.Unload()
            intPayload_1.Unload()
            self.assertTrue(not intPayload.IsLoaded())
            self.assertTrue(not intPayload_1.IsLoaded())
            self.assertTrue(not intPayload.GetPrototype())
            self.assertTrue(not intPayload_1.GetPrototype())
            self.assertTrue(not prototype4)

            # Loading the payload for an instanceable prim will cause
            # payloads nested in descendants of that prim's prototype to be 
            # loaded as well.
            baz = p.stage.GetPrimAtPath("/Foo/Baz")
            baz_1 = p.stage.GetPrimAtPath("/Foo/Baz_1")

            baz.Load()
            self.assertTrue(baz.IsLoaded())
            self.assertTrue(baz.GetPrototype())

            prototype = baz.GetPrototype()
            self.assertEqual(
                [prim.GetName() for prim in prototype.GetChildren()],
                ["Garply"])

            garply = prototype.GetChild("Garply")
            self.assertTrue(garply.HasAuthoredPayloads())
            self.assertTrue(garply.IsLoaded())
            self.assertTrue(garply.GetPrototype())

            prototype = garply.GetPrototype()
            self.assertEqual(
                [prim.GetName() for prim in prototype.GetChildren()],
                ["Qux"])

            baz_1.Load()
            self.assertEqual(baz.GetPrototype(), baz_1.GetPrototype())

            # Prims in prototypes cannot be individually (un)loaded.
            with self.assertRaises(Tf.ErrorException):
                garply.Unload()

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

            # Set the list of payloads to explicitly empty from cleared
            # verifying that it is indeed set to explicit.empty list op
            self.assertTrue(intPayload.GetPayloads().SetPayloads([]))
            # XXX: Though there is payload metadata that is an explicit empty
            # payload list op, HasAuthoredPayloads still returns true as there
            # are no actual payloads for this prim. This is inconsistent with
            # explicit empty list op behavior of 
            # HasAuthoredReferences/Inherits/Specializes which only return 
            # whether metadata is present. This inconsistency should be fixed.
            self.assertTrue(not intPayload.HasAuthoredPayloads())
            explicitEmpty = Sdf.PayloadListOp()
            explicitEmpty.ClearAndMakeExplicit()
            self.assertEqual(intPayload.GetMetadata("payload"), explicitEmpty)

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

    def test_SubrootReferencePayloads(self):
        """Tests the behavior of subroot references to prims with both direct
           and ancestral payloads."""

        for fmt in allFormats:
            # Layer1 prim /A
            layer1 = Sdf.Layer.CreateAnonymous("layer1." + fmt)
            primA = Sdf.PrimSpec(layer1, "A", Sdf.SpecifierDef)
            Sdf.PrimSpec(primA, "AChild", Sdf.SpecifierDef)

            # Layer2, prims /B/C
            layer2 = Sdf.Layer.CreateAnonymous("layer2." + fmt)
            primB = Sdf.PrimSpec(layer2, "B", Sdf.SpecifierDef)
            Sdf.PrimSpec(primB, "BChild", Sdf.SpecifierDef)
            primC = Sdf.PrimSpec(primB, "C", Sdf.SpecifierDef)
            Sdf.PrimSpec(primC, "CChild", Sdf.SpecifierDef)

            # Layer3, prim /D
            layer3 = Sdf.Layer.CreateAnonymous("layer3." + fmt)
            primD = Sdf.PrimSpec(layer3, "D", Sdf.SpecifierDef)
            Sdf.PrimSpec(primD, "DChild", Sdf.SpecifierDef)

            # Root layer, prim /E
            root = Sdf.Layer.CreateAnonymous("root." + fmt)
            primE = Sdf.PrimSpec(root, "E", Sdf.SpecifierDef)
            Sdf.PrimSpec(primE, "EChild", Sdf.SpecifierDef)

            # Prim /A has a payload to prim /B
            primA.payloadList.Prepend(Sdf.Payload(layer2.identifier, "/B"))

            # Root prim /E has a subroot reference to /A/C which comes from /A's
            # reference to /B
            primE.referenceList.Prepend(Sdf.Reference(layer1.identifier, "/A/C"))

            # Open the stage and get the root prim /E
            stage = Usd.Stage.Open(root)
            rootPrim = stage.GetPrimAtPath("/E")

            # Prim /E has no payloads for load/unload. An ancestral payload arc
            # from /A to /B is brought into the prim index with the reference
            # arc from /E to /A/C giving the prim stack:
            #
            # /E (root)
            # /A/C (ref)
            # /B/C (ancestral payload)
            #   
            # But this does not mark the prim index for /E as having a payload 
            # which is consistent with all ancestral payloads.
            # This ancestral payload arc, that is necessary to build the subroot
            # reference, is always included and can't be unloaded as it is not 
            # from a loadable ancestor of /E, but rather is internal to the 
            # subroot reference itself.
            self.assertTrue(rootPrim)
            self.assertFalse(rootPrim.HasPayload())
            self.assertTrue(rootPrim.IsLoaded())

            # Verify the children come from root E and referenced C.
            self.assertEqual(rootPrim.GetChildren(),
                             [stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])
            self.assertEqual(stage.GetPrimAtPath("/E").GetAllChildren(),
                             [stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])

            # Unload rootPrim /E and note that it can't be unloaded. Children
            # and load state stay the same.
            rootPrim.Unload()
            self.assertTrue(rootPrim.IsLoaded())
            self.assertEqual(rootPrim.GetChildren(),
                             [stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])
            self.assertEqual(stage.GetPrimAtPath("/E").GetAllChildren(),
                             [stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])

            # Now add a payload from the prim /B/C in layer1 to the prim /D.
            # The composed prim stack for /E will now be:
            #
            # /E (root)
            # /A/C (ref)
            # /B/C (ancestral payload)
            # /D (payload)
            primC.payloadList.Prepend(Sdf.Payload(layer3.identifier, "/D"))

            # Root prim now DOES have a payload as the payload to /D from /B/C
            # is a direct payload of /B/C. /E now a payload and should not be
            # loaded 
            rootPrim = stage.GetPrimAtPath("/E")
            self.assertTrue(rootPrim)
            self.assertTrue(rootPrim.HasPayload())
            self.assertFalse(rootPrim.IsLoaded())

            # Because /E is unloaded, GetChildren() returns empty, now 
            # GetAllChildren still returns the CChild and EChild as this does 
            # not unload the ancestral payload from the reference.
            self.assertTrue(stage.GetPrimAtPath("/E"))
            self.assertEqual(stage.GetPrimAtPath("/E").GetChildren(), [])
            self.assertEqual(stage.GetPrimAtPath("/E").GetAllChildren(),
                             [stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])

            # Now load /E. GetChildren() and GetAllChildren() return the same
            # and both lists now include the DChild from the extra payload.
            rootPrim.Load()
            self.assertTrue(rootPrim.IsLoaded())
            self.assertTrue(stage.GetPrimAtPath("/E"))
            self.assertEqual(stage.GetPrimAtPath("/E").GetChildren(),
                             [stage.GetPrimAtPath("/E/DChild"),
                              stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])
            self.assertEqual(stage.GetPrimAtPath("/E").GetAllChildren(),
                             [stage.GetPrimAtPath("/E/DChild"),
                              stage.GetPrimAtPath("/E/CChild"),
                              stage.GetPrimAtPath("/E/EChild")])


if __name__ == "__main__":
    unittest.main()
