#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Pcp, Sdf, Tf

def _FormatPath(pathInput):
    return Sdf.Path(pathInput) if pathInput else Sdf.Path()

# Converts a string -> string dictionary into an Sdf.Path -> Sdf.Path 
# dictionary for easier expected value comparison.
def _FormatRelocatesDict(relocatesDict):
    return {_FormatPath(k) : _FormatPath(v) for k, v in relocatesDict.items()}

# Converts a (string, string) tuple list into (Sdf.Path, Sdf.Path) tuples
# for easiser expected valued comparison.
def _FormatRelocatesTupleList(relocatesList):
    return [(_FormatPath(k), _FormatPath(v)) for k, v in relocatesList]

class TestPcpLayerRelocatesEditBuilder(unittest.TestCase):


    # Adds a relocate to the edit builder that we expect to succeed and verifies
    # that we end up with the expected relocates map.
    def _AddRelocateAndVerify(self, builder, source, target, expectedRelocates):
        self.assertTrue(
            builder.Relocate(_FormatPath(source), _FormatPath(target)))
        self.assertEqual(builder.GetEditedRelocatesMap(), 
                         _FormatRelocatesDict(expectedRelocates))

    # Removes a relocate by source path from the edit builder, which we expect
    # to succeed, and verifies that we end up with the expected relocates map.
    def _RemoveRelocateAndVerify(self, builder, source, expectedRelocates):
        self.assertTrue(builder.RemoveRelocate(_FormatPath(source)))
        self.assertEqual(builder.GetEditedRelocatesMap(), 
                         _FormatRelocatesDict(expectedRelocates))

    # Attempts to add a relocate to the builder that we expect to fail and
    # verifies that adding it fails with the expected "why not" message.
    def _VerifyInvalidRelocate(self, builder, source, target, expectedWhyNot):
        initialRelocates = builder.GetEditedRelocatesMap()
        initialEdits = builder.GetEdits()
        result = builder.Relocate(_FormatPath(source), _FormatPath(target))
        self.assertFalse(result)
        self.assertEqual(result.whyNot, expectedWhyNot)
        # Also verify that the relocates map and edits were not changed at all.
        self.assertEqual(builder.GetEditedRelocatesMap(), initialRelocates)
        self.assertEqual(builder.GetEdits(), initialEdits)

    # Attempts to remove a relocate by source from the builder that, which we
    # expect to fail, and verifies that removing it fails with the expected
    # "why not" message.
    def _VerifyInvalidRemoveRelocate(self, builder, source, expectedWhyNot):
        initialRelocates = builder.GetEditedRelocatesMap()
        initialEdits = builder.GetEdits()
        result = builder.RemoveRelocate(_FormatPath(source))
        self.assertFalse(result)
        self.assertEqual(result.whyNot, expectedWhyNot)
        # Also verify that the relocates map and edits were not changed at all.
        self.assertEqual(builder.GetEditedRelocatesMap(), initialRelocates)
        self.assertEqual(builder.GetEdits(), initialEdits)

    # Creates the new empty layers, layer stack, and pcp cache for testing. 
    # The root layer stack of the returned cache will consist of a root layer
    # with two sublayers and will have a session layer.
    def _CreateTestLayersAndPcpCache(self):
        subLayer1 = Sdf.Layer.CreateAnonymous()
        subLayer2 = Sdf.Layer.CreateAnonymous()
        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.subLayerPaths = [subLayer1.identifier, subLayer2.identifier]
        sessionLayer = Sdf.Layer.CreateAnonymous()
 
        # Create the PcpCache
        layerStackId = Pcp.LayerStackIdentifier(rootLayer, sessionLayer)
        cache = Pcp.Cache(layerStackId, usd=True)

        # Call ComputeLayerStack before return so that cache.layerStack is 
        # populated.
        cache.ComputeLayerStack(layerStackId)
        return cache
        
    # Verifies the edits builder's GetEdits() returns the expected layer 
    # relocates edits.
    def _VerifyExpectedEdits(self, builder, expectedEdits):
        formattedExpectedEdits = [
            (layer, _FormatRelocatesTupleList(relocates))
                for layer, relocates in expectedEdits]
        self.assertEqual(builder.GetEdits(), formattedExpectedEdits)

    # Helper for applying relocates edits to layers making sure they are change
    # processed by the cache.
    def _ApplyEdits(self, cache, layerAndRelocatesPairs):
        with Pcp._TestChangeProcessor(cache):
            with Sdf.ChangeBlock():
                for layer, relocates in layerAndRelocatesPairs:
                    layer.relocates = relocates

    # Verifies the the edits builder returns the expected edits, applies these
    # edits, and then verifies the layer stack relocates match the builder's 
    # relocates map.
    def _ApplyAndVerifyExpectedEdits(self, builder, cache, expectedEdits):
        self._VerifyExpectedEdits(builder, expectedEdits)
        self._ApplyEdits(cache, builder.GetEdits())
        self.assertEqual(cache.layerStack.incrementalRelocatesSourceToTarget, 
                         builder.GetEditedRelocatesMap())

    def test_InvalidAuthoredRelocates(self):
        """Tests trying to add relocates that will always be invalid via the 
            PcpRelocatesEditBuilder"""
        
        # New layer stack and edit builder. Starts with no relocates.
        cache = self._CreateTestLayersAndPcpCache()
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), {})

        # All of these relocates will fail to add because they are never valid
        # source and target path configurations for relocates.
        self._VerifyInvalidRelocate(builder, "/Root/A", "/Root/A",
            "Cannot relocate </Root/A> to </Root/A>: The target of a relocate "
            "cannot be the same as its source.")
        self._VerifyInvalidRelocate(builder, "/Root/A", "/Root/A/B",
            "Cannot relocate </Root/A> to </Root/A/B>: The target of a "
            "relocate cannot be a descendant of its source.")
        self._VerifyInvalidRelocate(builder, "/Root/A/B", "/Root/A",
            "Cannot relocate </Root/A/B> to </Root/A>: The target of a "
            "relocate cannot be an ancestor of its source.")
        self._VerifyInvalidRelocate(builder, "/Root/A", "/Root",
            "Cannot relocate </Root/A> to </Root>: The target of a "
            "relocate cannot be an ancestor of its source.")
        self._VerifyInvalidRelocate(builder, "/Root", "/Root/A",
            "Cannot relocate </Root> to </Root/A>: The target of a "
            "relocate cannot be a descendant of its source.")
        self._VerifyInvalidRelocate(builder, "/Root", "/OtherRoot/Root",
            "Cannot relocate </Root> to </OtherRoot/Root>: Adding a relocate "
            "from </Root> would result in a root prim being relocated.")
        self._VerifyInvalidRelocate(builder, "Root/A", "/Root/B",
            "Cannot relocate <Root/A> to </Root/B>: Relocates must use "
            "absolute paths.")
        self._VerifyInvalidRelocate(builder, "/Root/A", "Root/B",
            "Cannot relocate </Root/A> to <Root/B>: Relocates must use "
            "absolute paths.")
        self._VerifyInvalidRelocate(builder, "/Root/A.prop1", "/Root/A.prop2",
            "Cannot relocate </Root/A.prop1> to </Root/A.prop2>: Only prims "
            "can be relocated.")
        self._VerifyInvalidRelocate(builder, "/Root/A{v=x}B", "/Root/C",
            "Cannot relocate </Root/A{v=x}B> to </Root/C>: Relocates cannot "
            "have any variant selections.")
        self._VerifyInvalidRelocate(builder, "/Root/A/B", "/Root/A{v=x}",
            "Cannot relocate </Root/A/B> to </Root/A{v=x}>: Relocates cannot "
            "have any variant selections.")

        # Each of above calls verifies the relocates map remained the same 
        # (empty). Also verify that no layer edits are produced by builder.
        self.assertEqual(builder.GetEdits(), [])

    def test_BasicRelocateBuilding(self):
        """Tests the standard behaviors of calling Relocate(source, target) on
           an edits builder and the expected relocates maps that result."""

        # New layer stack and edit builder. Starts with no relocates.
        cache = self._CreateTestLayersAndPcpCache()
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), {})

        # Steps:
        #   1) Relocate /Root/A -> /Root/C
        #   2) Relocate /Root/B -> /Root/C
        # Result: The second relocate will fail because /Root/C is already the 
        # target of a relocate from step 1.
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/C",
            {"/Root/A" : "/Root/C"})
        self._VerifyInvalidRelocate(builder, "/Root/B", "/Root/C",
            "Cannot relocate </Root/B> to </Root/C>: A relocate from </Root/A> "
            "to </Root/C> already exists and the same target cannot be "
            "relocated to again.")
        # Remove the relocate. Remove only works using the source path /Root/A;
        # we cannot remove via the target path /Root/C.
        self._VerifyInvalidRemoveRelocate(builder, "/Root/C",
            "Cannot remove relocate for source path </Root/C>: No relocate "
            "with the source path found.")
        self._RemoveRelocateAndVerify(builder, "/Root/A",
            {})

        # Steps: 
        #   1) Relocate /Root/B -> /Root/C
        #   2) Relocate /Root/A -> /Root/B
        # Result: The second fails because /Root/B has been relocated away 
        # making it no longer a valid target prim path.
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/C",
            {"/Root/B" : "/Root/C"})
        self._VerifyInvalidRelocate(builder, "/Root/A", "/Root/B",
            "Cannot relocate </Root/A> to </Root/B>: The target of the "
            "relocate is the same as the source of an existing relocate from "
            "</Root/B> to </Root/C>; the only prim that can be relocated to "
            "</Root/B> is the existing relocate's target </Root/C>, which "
            "will remove the relocate.")
        # We can relocate /Root/C back to /Root/B to delete the relocate
        self._AddRelocateAndVerify(builder, "/Root/C", "/Root/B",
            {})

        # Steps: 
        #   1) Relocate /Root/A -> /Root/B
        #   2) Relocate /Root/B -> /Root/C 
        # Result: The second will be successful and will update the relocate 
        # to be from /Root/A -> /Root/C, leaving /Root/B out of the equation.
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/B",
            {"/Root/A" : "/Root/B"})
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/C",
            {"/Root/A" : "/Root/C"})
        # Relocate target /Root/C back to source /Root/A to delete the relocate.
        self._AddRelocateAndVerify(builder, "/Root/C", "/Root/A",
            {})

        # Steps:
        #   1) Relocate /Root/A -> /Root/B 
        #   2) Relocate /Root/A -> /Root/C 
        # Result: The second relocate will replace the original relocate. This
        # is because of how we relocate map source paths making this exactly
        # equivalent to the /Root/A -> /Root/B, /Root/B -> /Root/C case.
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/B",
            {"/Root/A" : "/Root/B"})
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/C",
            {"/Root/A" : "/Root/C"})
        # Remove the relocate.
        self._RemoveRelocateAndVerify(builder, "/Root/A",
            {})

        # Steps:
        #   1) Relocate /Root/A/C -> /Root/B/C
        #   2) Relocate /Root/A/D -> /Root/B/D
        #   3) Relocate /Root/A -> /Root/B
        # Result: The third relocate subsumes the previous two as it makes them
        # redundant
        self._AddRelocateAndVerify(builder, "/Root/A/C", "/Root/B/C",
            {"/Root/A/C" : "/Root/B/C"})
        self._AddRelocateAndVerify(builder, "/Root/A/D", "/Root/B/D",
            {"/Root/A/C" : "/Root/B/C",
             "/Root/A/D" : "/Root/B/D"})
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/B",
            {"/Root/A" : "/Root/B"})
        # Relocate /Root/B back to /Root/A. This also deletes the relocate. Note
        # that this does NOT revive the previous relocates from the first two
        # steps.
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/A",
            {})

        # Steps:
        #   1) Relocate /Root/A/C -> /Root/B/C
        #   2) Relocate /Root/A/D -> /Root/B/D
        #   3) Relocate /Root/A -> /Root/F
        # Result: The third relocate udpates the previous two relocates' sources
        self._AddRelocateAndVerify(builder, "/Root/A/C", "/Root/B/C",
            {"/Root/A/C" : "/Root/B/C"})
        self._AddRelocateAndVerify(builder, "/Root/A/D", "/Root/B/D",
            {"/Root/A/C" : "/Root/B/C",
             "/Root/A/D" : "/Root/B/D"})
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/F",
            {"/Root/A" : "/Root/F",
             "/Root/F/C" : "/Root/B/C",
             "/Root/F/D" : "/Root/B/D"})
        #   4) Relocate /Root/B -> /Root/F
        #   5) Relocate /Root/B -> /Root/A
        #   6) Relocate /Root/A -> /Root/B
        # Result: Steps 4 and 5 will both fail because of invalid targets that
        # are already relocation sources or target. 
        # Step 6 succeeds with the resulting /Root/F -> /Root/B mapping 
        # subsuming the updated children relocates.
        self._VerifyInvalidRelocate(builder, "/Root/B", "/Root/F",
            "Cannot relocate </Root/B> to </Root/F>: A relocate from </Root/A> "
            "to </Root/F> already exists and the same target cannot be "
            "relocated to again.")
        self._VerifyInvalidRelocate(builder, "/Root/B", "/Root/A",
            "Cannot relocate </Root/B> to </Root/A>: The target of the "
            "relocate is the same as the source of an existing relocate from "
            "</Root/A> to </Root/F>; the only prim that can be relocated to "
            "</Root/A> is the existing relocate's target </Root/F>, which "
            "will remove the relocate.")
        self._AddRelocateAndVerify(builder, "/Root/F", "/Root/B",
            {"/Root/A" : "/Root/B"})
        # Remove the relocate.
        self._RemoveRelocateAndVerify(builder, "/Root/A",
            {})

        # Steps:
        #   1) Relocate /Root/A/C -> /Root/B/C
        #   2) Relocate /Root/A/D -> /Root/B/D
        #   3) Relocate /Root/B -> /Root/F
        # Result: The third relocate udpates the previous two relocates' targets
        self._AddRelocateAndVerify(builder, "/Root/A/C", "/Root/B/C",
            {"/Root/A/C" : "/Root/B/C"})
        self._AddRelocateAndVerify(builder, "/Root/A/D", "/Root/B/D",
            {"/Root/A/C" : "/Root/B/C",
             "/Root/A/D" : "/Root/B/D"})
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/F",
            {"/Root/B" : "/Root/F",
             "/Root/A/C" : "/Root/F/C",
             "/Root/A/D" : "/Root/F/D"})
        #   4) Relocate /Root/F -> /Root/A
        # Result: This one is tricky to reason about. Relocating /Root/F to 
        # /Root/A is valid under all the conditions we enforce about sources and
        # targets and existing relocates. And since existing relocate targets 
        # have /Root/F as a prefix, we update those targets with the relocated
        # path. Thus we'd end up with:
        #   /Root/B -> /Root/A
        #   /Root/A/C -> /Root/A/C
        #   /Root/A/D -> /Root/A/D
        # As we can see, the last two map the source paths back to themselves.
        # Thus we've deleted the child relocates by mapping their relocated 
        # parent back their original source.
        self._AddRelocateAndVerify(builder, "/Root/F", "/Root/A",
            {"/Root/B" : "/Root/A"})
        # Remove the last relocate.
        self._RemoveRelocateAndVerify(builder, "/Root/B",
            {})

        # Steps:
        #   1) Relocate /Root/A/C -> /Root/B/C
        #   2) Relocate /Root/A/D -> /Root/B/D
        #   3) Relocate /Root/B -> /Root/A
        # Result: This is just like the previous case with the third step 
        # skipped. Relocating the children relocated parent back to their own
        # source parent ends up deleting the child relocates while adding the
        # inverse relocate for the parent path.
        self._AddRelocateAndVerify(builder, "/Root/A/C", "/Root/B/C",
            {"/Root/A/C" : "/Root/B/C"})
        self._AddRelocateAndVerify(builder, "/Root/A/D", "/Root/B/D",
            {"/Root/A/C" : "/Root/B/C",
             "/Root/A/D" : "/Root/B/D"})
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/A",
            {"/Root/B" : "/Root/A"})
        # Relocate the relocate.
        self._RemoveRelocateAndVerify(builder, "/Root/B",
            {})

        # Steps:
        #   1) Relocate /A -> /B
        #   2) Relocate /Root/A -> /A
        #   3) Relocate /A -> /B
        # Result: The first attempt at relocating /A to /B will fail because /A
        # is a root prim. The second relocate succeeds as we allow a non-root
        # prim to be relocated to become a root prim. After that relocating 
        # /A to /B will succeed as it results in only changing the path that the 
        # relocated non-root prim is moved to.
        self._VerifyInvalidRelocate(builder, "/A", "/B",
            "Cannot relocate </A> to </B>: Adding a relocate from </A> would "
            "result in a root prim being relocated.")
        self._AddRelocateAndVerify(builder, "/Root/A", "/A",
            {"/Root/A" : "/A"})
        self._AddRelocateAndVerify(builder, "/A", "/B",
            {"/Root/A" : "/B"})
        # Relocate target /B back to source /Root/A to delete the relocate, 
        # which works because it adjusts (deletes) an existing relocate and
        # wouldn't introduce an actual relocate from a root prim.
        self._AddRelocateAndVerify(builder, "/B", "/Root/A",
            {})

        # Steps:
        #   1) Relocate /Root/A -> /OtherRoot/A
        #   2) Relocate /OtherRoot/A/B -> /Root/B
        #   3) Relocate /OtherRoot -> /Root
        # Result: The first two relocates succeed as relocating prims to be
        # descendants of a different root prim is perfectly valid. The third
        # relocate will fail because even though relocating /OtherRoot would
        # update existing relocates, it would still have to add a new relocate
        # from /OtherRoot to /Root and that is not allowed.
        self._AddRelocateAndVerify(builder, "/Root/A", "/OtherRoot/A",
            {"/Root/A" : "/OtherRoot/A"})
        self._AddRelocateAndVerify(builder, "/OtherRoot/A/B", "/Root/B",
            {"/Root/A" : "/OtherRoot/A",
             "/OtherRoot/A/B" : "/Root/B"})
        self._VerifyInvalidRelocate(builder, "/OtherRoot", "/Root",
            "Cannot relocate </OtherRoot> to </Root>: Adding a relocate from "
            "</OtherRoot> would result in a root prim being relocated.")
        # Remove the /Root/A relocate. This updates the source path of the 
        # remaining relocate to be /Root/A/B instead of /OtherRoot/A/B to account
        # for the fact that /Root/A/B is no longer ancestrally relocated by the 
        # /Root/A to /OtherRoot/A relocate.
        self._RemoveRelocateAndVerify(builder, "/Root/A",
            {"/Root/A/B" : "/Root/B"})
        # Remove the /Root/A/B relocate.    
        self._RemoveRelocateAndVerify(builder, "/Root/A/B",
            {})

    def test_HierarchyRelocates(self):

        # New layer stack and edit builder. Starts with no relocates.
        cache = self._CreateTestLayersAndPcpCache()
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), {})

        # This example will assume we're working with a prim hierarchy:
        # /World
        #  | /Root
        #  |  | /A
        #  |  |  | /C
        #  |  |  |  | /E
        #  |  |  |  | /F
        #  |  |  | /D
        #  |  |  |  | /E
        #  |  |  |  | /F
        #  |  | /B
        #  |  |  | /C
        #  |  |  |  | /E
        #  |  |  |  | /F
        #  |  |  | /D
        #  |  |  |  | /E
        #  |  |  |  | /F
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)

        # Bottom up renaming of the entire namespace under /World/Root/A to have
        # prefix "New"

        # Rename leaf prims
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/C/E", "/World/Root/A/C/NewE", 
            {"/World/Root/A/C/E" : "/World/Root/A/C/NewE"})
        self._AddRelocateAndVerify(builder,
            "/World/Root/A/C/F", "/World/Root/A/C/NewF", 
            {"/World/Root/A/C/E" : "/World/Root/A/C/NewE",
             "/World/Root/A/C/F" : "/World/Root/A/C/NewF"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D/E", "/World/Root/A/D/NewE", 
            {"/World/Root/A/C/E" : "/World/Root/A/C/NewE",
             "/World/Root/A/C/F" : "/World/Root/A/C/NewF",
             "/World/Root/A/D/E" : "/World/Root/A/D/NewE"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D/F", "/World/Root/A/D/NewF", 
            {"/World/Root/A/C/E" : "/World/Root/A/C/NewE",
             "/World/Root/A/C/F" : "/World/Root/A/C/NewF",
             "/World/Root/A/D/E" : "/World/Root/A/D/NewE",
             "/World/Root/A/D/F" : "/World/Root/A/D/NewF"})

        # Rename leaf prim parents. These update the existing leaf prim 
        # relocates.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/C", "/World/Root/A/NewC", 
            {"/World/Root/A/C" : "/World/Root/A/NewC",
             "/World/Root/A/NewC/E" : "/World/Root/A/NewC/NewE",
             "/World/Root/A/NewC/F" : "/World/Root/A/NewC/NewF",
             "/World/Root/A/D/E" : "/World/Root/A/D/NewE",
             "/World/Root/A/D/F" : "/World/Root/A/D/NewF"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D", "/World/Root/A/NewD", 
            {"/World/Root/A/C" : "/World/Root/A/NewC",
             "/World/Root/A/NewC/E" : "/World/Root/A/NewC/NewE",
             "/World/Root/A/NewC/F" : "/World/Root/A/NewC/NewF",
             "/World/Root/A/D" : "/World/Root/A/NewD",
             "/World/Root/A/NewD/E" : "/World/Root/A/NewD/NewE",
             "/World/Root/A/NewD/F" : "/World/Root/A/NewD/NewF"})

        # Rename /World/Root/A. Updates all the existing relocates.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A", "/World/Root/NewA", 
            {"/World/Root/A" : "/World/Root/NewA",
             "/World/Root/NewA/C" : "/World/Root/NewA/NewC",
             "/World/Root/NewA/NewC/E" : "/World/Root/NewA/NewC/NewE",
             "/World/Root/NewA/NewC/F" : "/World/Root/NewA/NewC/NewF",
             "/World/Root/NewA/D" : "/World/Root/NewA/NewD",
             "/World/Root/NewA/NewD/E" : "/World/Root/NewA/NewD/NewE",
             "/World/Root/NewA/NewD/F" : "/World/Root/NewA/NewD/NewF"})

        # This example demonstrates reparenting prim hierarchy starting at 
        # /World/Root/A to be a child of /World/NewRoot instead of /World/Root. 
        # We can do this easily with a relocate from /World/Root/A to 
        # /World/NewRoot/A but we're going to do the move bottom to demonstrate
        # the behaviors where we can subsume existing relocates.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)

        # So first add a relocate for each leaf prim to the final reparented 
        # location. We end up with a relocate entry per prim.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/C/E", "/World/NewRoot/A/C/E", 
            {"/World/Root/A/C/E" : "/World/NewRoot/A/C/E"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/C/F", "/World/NewRoot/A/C/F", 
            {"/World/Root/A/C/E" : "/World/NewRoot/A/C/E",
             "/World/Root/A/C/F" : "/World/NewRoot/A/C/F"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D/E", "/World/NewRoot/A/D/E", 
            {"/World/Root/A/C/E" : "/World/NewRoot/A/C/E",
             "/World/Root/A/C/F" : "/World/NewRoot/A/C/F",
             "/World/Root/A/D/E" : "/World/NewRoot/A/D/E"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D/F", "/World/NewRoot/A/D/F", 
            {"/World/Root/A/C/E" : "/World/NewRoot/A/C/E",
             "/World/Root/A/C/F" : "/World/NewRoot/A/C/F",
             "/World/Root/A/D/E" : "/World/NewRoot/A/D/E",
             "/World/Root/A/D/F" : "/World/NewRoot/A/D/F"})

        # Now add relocates for the leaf parent prims. Since the parent path 
        # relocates would map the leaf prims paths to the exact same locations 
        # as the existing leaf relocates, the parent relocates will subsume the
        # existing leaf relocates.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/C", "/World/NewRoot/A/C", 
            {"/World/Root/A/C" : "/World/NewRoot/A/C", # subsumes the existing descendant relocates
             "/World/Root/A/D/E" : "/World/NewRoot/A/D/E",
             "/World/Root/A/D/F" : "/World/NewRoot/A/D/F"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A/D", "/World/NewRoot/A/D", 
            {"/World/Root/A/C" : "/World/NewRoot/A/C",
             "/World/Root/A/D" : "/World/NewRoot/A/D"}) # subsumes the existing descendant relocates

        # Now add a relocate for the next parent up which is the root of subtree
        # /World/Root/A. This subsumes the descendant relocates again leaving us
        # with a single relocate for the entire subtree.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/A", "/World/NewRoot/A", 
            {"/World/Root/A" : "/World/NewRoot/A"})

        # Now we'll do a similar thing moving an entire hierarchy starting at 
        # /World/Root/B to be at /World/NewRoot/B instead. We'll start with the
        # leaf prims here again too.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/B/C/E", "/World/NewRoot/B/C/E", 
            {"/World/Root/A" : "/World/NewRoot/A",
             "/World/Root/B/C/E" : "/World/NewRoot/B/C/E"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/B/C/F", "/World/NewRoot/B/C/F", 
            {"/World/Root/A" : "/World/NewRoot/A",
             "/World/Root/B/C/E" : "/World/NewRoot/B/C/E",
             "/World/Root/B/C/F" : "/World/NewRoot/B/C/F"})
        self._AddRelocateAndVerify(builder,
            "/World/Root/B/D/E", "/World/NewRoot/B/D/E", 
            {"/World/Root/A" : "/World/NewRoot/A",
             "/World/Root/B/C/E" : "/World/NewRoot/B/C/E",
             "/World/Root/B/C/F" : "/World/NewRoot/B/C/F",
             "/World/Root/B/D/E" : "/World/NewRoot/B/D/E"})
        self._AddRelocateAndVerify(builder, 
            "/World/Root/B/D/F", "/World/NewRoot/B/D/F", 
            {"/World/Root/A" : "/World/NewRoot/A",
             "/World/Root/B/C/E" : "/World/NewRoot/B/C/E",
             "/World/Root/B/C/F" : "/World/NewRoot/B/C/F",
             "/World/Root/B/D/E" : "/World/NewRoot/B/D/E",
             "/World/Root/B/D/F" : "/World/NewRoot/B/D/F"})

        # Now instead of moving the parents of the leaf prims, we'll just skip 
        # that step and move the root of the subtree /World/Root/B. This 
        # subsumes the leaf prim relocates just like if we had moved their 
        # parent paths directly.
        self._AddRelocateAndVerify(builder, 
            "/World/Root/B", "/World/NewRoot/B", 
            {"/World/Root/A" : "/World/NewRoot/A",
             "/World/Root/B" : "/World/NewRoot/B"})
        
    def test_RelocateToNone(self):
        """Tesst using relocates to empty path aka relocates to delete."""

        # New layer stack and edit builder. Starts with no relocates.
        cache = self._CreateTestLayersAndPcpCache()
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), {})

        # Basic relocate to empty.        
        self._AddRelocateAndVerify(builder, "/Root/A", "",
            {"/Root/A" : ""})
        self._AddRelocateAndVerify(builder, "/Root/B/C", "",
            {"/Root/A" : "",
             "/Root/B/C" : ""})
        
        # Relocating a prim to none subsumes existing relocates for any of its
        # descendants. 
        self._AddRelocateAndVerify(builder, "/Root/B", "",
            {"/Root/A" : "",
             "/Root/B" : ""}) # subsumes the </Root/B/C> to <> relocate

        # Add a standard relocate for the next cases.
        self._AddRelocateAndVerify(builder, "/Root/C", "/Foo",
            {"/Root/A" : "",
             "/Root/B" : "",
             "/Root/C" : "/Foo"})
        
        # Still invalid to relocate an existing relocate source (or any of its
        # descendants) again even if the target is none.
        self._VerifyInvalidRelocate(builder, "/Root/C", "",
            "Cannot relocate </Root/C> to <>: A relocate from </Root/C> to "
            "</Foo> already exists; neither the source </Root/C> nor any of "
            "its descendants can be relocated again using their original "
            "paths.")
        self._VerifyInvalidRelocate(builder, "/Root/C/Child", "",
            "Cannot relocate </Root/C/Child> to <>: A relocate from </Root/C> "
            "to </Foo> already exists; neither the source </Root/C> nor any of "
            "its descendants can be relocated again using their original "
            "paths.")
        
        # Relocating a child of a post-relocation path to none just adds a new
        # relocate
        self._AddRelocateAndVerify(builder, "/Foo/Bar", "",
            {"/Root/A" : "",
             "/Root/B" : "",
             "/Root/C" : "/Foo",
             "/Foo/Bar" : ""})
        # Relocating post-relocation path /Foo to none updates the existing
        # relocate from </Root/C> to </Foo> to be </Root/C> to <>. But it also
        # subsumes the relocate from </Foo/Bar> to <> since </Root/C> to <> 
        # now still deletes the prims that were previously relocated to </Foo>
        # and therefore </Foo/Bar>
        self._AddRelocateAndVerify(builder, "/Foo", "",
            {"/Root/A" : "",
             "/Root/B" : "",
             "/Root/C" : ""})

        # Add another standard relocate to a path a little deeper in namesspace.
        self._AddRelocateAndVerify(builder, "/Root/D", "/Root/Foo/Bar",
            {"/Root/A" : "",
             "/Root/B" : "",
             "/Root/C" : "",
             "/Root/D" : "/Root/Foo/Bar"})
        # Relocate an ancestor of the existing target path to none. This updates
        # the existing relocate from </Root/D> to </Root/Foo/Bar> to be 
        # </Root/D> to <> as the effect of deleting a parent prim is all 
        # descendants are deleted. Also adds </Root/Foo> to <> as a new 
        # relocate.
        self._AddRelocateAndVerify(builder, "/Root/Foo", "",
            {"/Root/A" : "",
             "/Root/B" : "",
             "/Root/C" : "",
             "/Root/D" : "",
             "/Root/Foo" : ""})

        # Relocate source path can never be empty; only target paths can be. 
        # Thus the Relocate method cannot be used to remove relocates to none.
        self._VerifyInvalidRelocate(builder, "", "/Root/A",
            "Cannot relocate <> to </Root/A>: Relocates source paths cannot be "
            "empty.")
        # RemoveRelocate can remove relocates to none by their source paths.
        self._RemoveRelocateAndVerify(builder, "/Root/A",
            {"/Root/B" : "",
             "/Root/C" : "",
             "/Root/D" : "",
             "/Root/Foo" : ""})
        self._RemoveRelocateAndVerify(builder, "/Root/C",
            {"/Root/B" : "",
             "/Root/D" : "",
             "/Root/Foo" : ""})

        # RemoveRelocate can only be called on actual existing source paths.
        # Calling it on a non-relocated ancestor path of other relocates (to try
        # to remove them all at once) will fail.
        self._VerifyInvalidRemoveRelocate(builder, "/Root",
            "Cannot remove relocate for source path </Root>: No relocate "
            "with the source path found.")

    def test_SingleLayerRelocatesUpdates(self):
        """Tests using the relocates builder to create and perform edits on 
           just one layer."""

        # New layer stack and edit builder. Starts with no relocates.
        cache = self._CreateTestLayersAndPcpCache()
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), {})

        # Root layer is layers[1] since the layer stack has a session layer.
        rootLayer = cache.layerStack.layers[1]

        # Add some simple relocates to prims that don't overlap in namespace.

        # Add a renaming relocate
        self._AddRelocateAndVerify(builder, "/Root/A", "/Root/B", 
            {"/Root/A" : "/Root/B"}) # added new relocate
        # Add a reparenting relocate.
        self._AddRelocateAndVerify(builder, "/Root/C", "/Root/D/C", 
            {"/Root/A" : "/Root/B",
             "/Root/C" : "/Root/D/C"}) #added new relocate
        # Add a rename and reparent relocate.
        self._AddRelocateAndVerify(builder, "/Root/E/F", "/Root/G", 
            {"/Root/A" : "/Root/B",
             "/Root/C" : "/Root/D/C",
             "/Root/E/F" : "/Root/G"}) #added new relocate

        # Verify the built layer edits and apply them to the layer stack. The
        # edits are all to the root layer. The edit builder was created 
        # without a specified "new relocates" layer which means all new 
        # relocates should be added to the root layer.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, 
                [("/Root/A", "/Root/B"),
                 ("/Root/C", "/Root/D/C"),
                 ("/Root/E/F", "/Root/G")])])

        # Create a new edit builder for the layer stack. It now starts with 
        # the relocates we set on the layer stack. The edits still start empty
        # because we haven't added any.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict(
                {"/Root/A" : "/Root/B",
                 "/Root/C" : "/Root/D/C",
                 "/Root/E/F" : "/Root/G"}))
        self.assertEqual(builder.GetEdits(), [])

        # We'll now add relocates that affect children of the prims we already
        # relocated.

        # Add a relocate that renames a child of /Root/A named "H" to be named
        # "I". With /Root/A relocated to /Root/B, /Root/A/H will end up 
        # relocated to /Root/B/I
        #
        # We'll start with what we can't do; we cannot use /Root/A/I as a target 
        # path as anything under /Root/A is already relocated away and can never
        # be a target path.
        self._VerifyInvalidRelocate(builder, "/Root/B/H", "/Root/A/I",
            "Cannot relocate </Root/B/H> to </Root/A/I>: Cannot relocate a "
            "prim to be a descendant of </Root/A> which is already relocated "
            "to </Root/B>.")
        # Nor can we use /Root/A/H as a source path because /Root/A has been
        # relocated away.
        self._VerifyInvalidRelocate(builder, "/Root/A/H", "/Root/B/I",
            "Cannot relocate </Root/A/H> to </Root/B/I>: A relocate from "
            "</Root/A> to </Root/B> already exists; neither the source "
            "</Root/A> nor any of its descendants can be relocated again using "
            "their original paths.")

        # The target of the relocate must be the final relocated target path
        # /Root/B/I and the source path must be the relocated source path 
        # /Root/B/H.
        self._AddRelocateAndVerify(builder, "/Root/B/H", "/Root/B/I", 
            {"/Root/A" : "/Root/B",
             "/Root/B/H" : "/Root/B/I", # added new child relocate
             "/Root/C" : "/Root/D/C",
             "/Root/E/F" : "/Root/G"})

        # Add a relocate that moves a child of /Root/C named "J" to be parented
        # under /Root/G which has itself been relocated from /Root/E/F. The 
        # final relocated path of "J" will be /Root/G/J
        #
        # First, we cannot use /Root/E/F/J as a target path as anything under 
        # /Root/E/F is already relocated away and can never be a target path.
        self._VerifyInvalidRelocate(builder, "/Root/D/C/J", "/Root/E/F/J",
            "Cannot relocate </Root/D/C/J> to </Root/E/F/J>: Cannot relocate a "
            "prim to be a descendant of </Root/E/F> which is already relocated "
            "to </Root/G>.")
        # Nor can we use the pre-relocation source path of /Root/C/J 
        self._VerifyInvalidRelocate(builder, "/Root/C/J", "/Root/G/J",
            "Cannot relocate </Root/C/J> to </Root/G/J>: A relocate from "
            "</Root/C> to </Root/D/C> already exists; neither the source "
            "</Root/C> nor any of its descendants can be relocated again using "
            "their original paths.")

        # The target of the relocate must use final relocated target path
        # /Root/G/J and the relocated source path /Root/D/C/J.
        self._AddRelocateAndVerify(builder, "/Root/D/C/J", "/Root/G/J", 
            {"/Root/A" : "/Root/B",
             "/Root/B/H" : "/Root/B/I",
             "/Root/C" : "/Root/D/C",
             "/Root/E/F" : "/Root/G",
             "/Root/D/C/J" : "/Root/G/J"}) # added new relocate

        # Verify the built layer edits and apply them to the layer stack. The
        # edits are all to the root layer in the layer stack. Note that the
        # edit contains ALL the relocates, not just the new ones, as this map is
        # value of the relocates field that needs to be set in the layer 
        # metadata.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, 
                [("/Root/A", "/Root/B"),
                 ("/Root/C", "/Root/D/C"),
                 ("/Root/E/F", "/Root/G"),
                 ("/Root/B/H", "/Root/B/I"),
                 ("/Root/D/C/J", "/Root/G/J")])])

        # Create a new edit builder for the layer stack again. It starts with 
        # all the relocates we set on the layer stack with the edits still 
        # empty.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict(
                {"/Root/A" : "/Root/B",
                 "/Root/B/H" : "/Root/B/I",
                 "/Root/C" : "/Root/D/C",
                 "/Root/E/F" : "/Root/G",
                 "/Root/D/C/J" : "/Root/G/J"}))
        self.assertEqual(builder.GetEdits(), [])

        # These next test examples will not cause new relocates to be added but
        # will update the existing relocates to remap them.        

        # Reparent the prim /Root/B to be /Root/New/B. Because /Root/B is 
        # already a target of a relocate from /Root/A, this will just update
        # existing relocates paths to account for the new reparent without 
        # adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/New/B", 
            {"/Root/A" : "/Root/New/B", # existing target is updated
             "/Root/New/B/H" : "/Root/New/B/I", # existing source and target are updated
             "/Root/C" : "/Root/D/C",
             "/Root/E/F" : "/Root/G",
             "/Root/D/C/J" : "/Root/G/J"})
        # Reparent the prim /Root/D/C to be /Root/New/C. Because /Root/D/C is 
        # already a target of a relocate from /Root/C, this will just update
        # existing relocates paths to account for the new reparent without 
        # adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/D/C", "/Root/New/C", 
            {"/Root/A" : "/Root/New/B",
             "/Root/New/B/H" : "/Root/New/B/I",
             "/Root/C" : "/Root/New/C", # existing target is updated
             "/Root/E/F" : "/Root/G",
             "/Root/New/C/J" : "/Root/G/J"}) # existing source is updated
        # Reparent and rename the prim /Root/G to be /Root/New/F. Because 
        # /Root/G is already a target of a relocate from /Root/E/F, this will 
        # just update existing relocates paths to account for the new reparent
        # without adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/G", "/Root/New/F", 
            {"/Root/A" : "/Root/New/B",
             "/Root/New/B/H" : "/Root/New/B/I",
             "/Root/C" : "/Root/New/C",
             "/Root/E/F" : "/Root/New/F", # existing target is updated
             "/Root/New/C/J" : "/Root/New/F/J"}) # existing target is updated

        # Verify the built layer edits and apply them to the layer stack. The
        # edits are all to the root layer in the layer stack.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (cache.layerStack.layers[1], 
                [("/Root/A", "/Root/New/B"),
                 ("/Root/C", "/Root/New/C"),
                 ("/Root/E/F", "/Root/New/F"),
                 ("/Root/New/B/H", "/Root/New/B/I"),
                 ("/Root/New/C/J", "/Root/New/F/J")])])

        # Create a new edit builder for the layer stack again. It starts with 
        # all the relocates we set on the layer stack with the edits still 
        # empty.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict(
                {"/Root/A" : "/Root/New/B",
                 "/Root/New/B/H" : "/Root/New/B/I",
                 "/Root/C" : "/Root/New/C",
                 "/Root/E/F" : "/Root/New/F",
                 "/Root/New/C/J" : "/Root/New/F/J"}))
        self.assertEqual(builder.GetEdits(), [])

        # These next test examples are similar to the previous ones and will
        # update the existing relocates to remap them. The difference here is
        # that we'll use origin source paths to add the relocates.

        # Reparent and rename the relocated /Root/New/B to be /Root/OldB. Since
        # /Root/New/B is already a target of a relocate from /Root/A, this will
        # just update existing relocates paths to account for the new prim move
        # without adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/New/B", "/Root/OldB", 
            {"/Root/A" : "/Root/OldB", # existing target is updated
             "/Root/OldB/H" : "/Root/OldB/I", # existing source and target are updated
             "/Root/C" : "/Root/New/C",
             "/Root/E/F" : "/Root/New/F",
             "/Root/New/C/J" : "/Root/New/F/J"})
        # Reparent the relocated prim /Root/New/C to be /Root/OldD/C. Since 
        # /Root/New/C is already a target of a relocate from /Root/C, this will
        # just update existing relocates paths to account for the new prim move
        # without adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/New/C", "/Root/OldD/C", 
            {"/Root/A" : "/Root/OldB",
             "/Root/OldB/H" : "/Root/OldB/I",
             "/Root/C" : "/Root/OldD/C", # existing target is updated
             "/Root/E/F" : "/Root/New/F",
             "/Root/OldD/C/J" : "/Root/New/F/J"}) # existing source is updated
        # Reparent and rename the relocated prim /Root/New/F to be /Root/OldG. 
        # Since /Root/New/F is already a target of a relocate from /Root/E/F, 
        # this will just update existing relocates paths to account for the new
        # prim move without adding a new entry
        self._AddRelocateAndVerify(builder, "/Root/New/F", "/Root/OldG", 
            {"/Root/A" : "/Root/OldB",
             "/Root/OldB/H" : "/Root/OldB/I",
             "/Root/C" : "/Root/OldD/C",
             "/Root/E/F" : "/Root/OldG", # existing target is updated
             "/Root/OldD/C/J" : "/Root/OldG/J"}) # existing target is updated

        # Verify the built layer edits and apply them to the layer stack. The
        # edits are all to layer[1], the root in the layer stack. Note that the
        # edit contains ALL the relocates, not just the new ones, as this map is
        # value of the relocates field that needs to be set in the layer 
        # metadata.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (cache.layerStack.layers[1], 
                [("/Root/A", "/Root/OldB"),
                 ("/Root/C", "/Root/OldD/C"),
                 ("/Root/E/F", "/Root/OldG"),
                 ("/Root/OldB/H", "/Root/OldB/I"),
                 ("/Root/OldD/C/J", "/Root/OldG/J")])])
        
        # Add some relocates to none
        self._AddRelocateAndVerify(builder, "/Root/ToDelete", "", 
            {"/Root/A" : "/Root/OldB",
             "/Root/OldB/H" : "/Root/OldB/I",
             "/Root/C" : "/Root/OldD/C",
             "/Root/E/F" : "/Root/OldG",
             "/Root/OldD/C/J" : "/Root/OldG/J",
             "/Root/ToDelete" : ""}) # added new relocate
        self._AddRelocateAndVerify(builder, "/Root/OldB", "", 
            {"/Root/A" : "", # existing target is updated
             # this relocate was deleted "/Root/OldB/H" : "/Root/OldB/I",
             "/Root/C" : "/Root/OldD/C",
             "/Root/E/F" : "/Root/OldG",
             "/Root/OldD/C/J" : "/Root/OldG/J",
             "/Root/ToDelete" : ""})

        # Verify the built layer edits with relocates to none and apply them to
        # the layer stack. The edits are all to layer[1], the root in the layer
        # stack.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (cache.layerStack.layers[1], 
                [("/Root/A", ""),
                 ("/Root/C", "/Root/OldD/C"),
                 ("/Root/E/F", "/Root/OldG"),
                 ("/Root/OldD/C/J", "/Root/OldG/J"),
                 ("/Root/ToDelete", "")])])

    def test_MultipleLayerRelocatesEdits(self):

        # New layer stack. It has four layers: a session layer and a root layer
        # with two sublayers
        cache = self._CreateTestLayersAndPcpCache()
        sessionLayer = cache.layerStack.layers[0]
        rootLayer = cache.layerStack.layers[1]
        sub1Layer = cache.layerStack.layers[2]
        sub2Layer = cache.layerStack.layers[3]

        # Create a builder for the layer stack with no "add relocates" layer. 
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)

        # Add some relocates
        self._AddRelocateAndVerify(builder, "/Root/A/B", "/Root/B",
            {"/Root/A/B" : "/Root/B"})
        self._AddRelocateAndVerify(builder, "/Root/A/C", "/Root/C",
            {"/Root/A/B" : "/Root/B",
             "/Root/A/C" : "/Root/C"})

        # Verify edits are for the root layer since the builder did not specify
        # a particular layer.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, 
                [("/Root/A/B", "/Root/B"),
                 ("/Root/A/C", "/Root/C")])])

        # Create a new builder that specifies new relocates should be added
        # to sublayer 1.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack, sub1Layer)

        # Add some new relocates that don't require any updates to existing 
        # relocates.
        self._AddRelocateAndVerify(builder, "/Root/A/D", "/Root/D",
            {"/Root/A/B" : "/Root/B",
             "/Root/A/C" : "/Root/C",
             "/Root/A/D" : "/Root/D"})
        self._AddRelocateAndVerify(builder, "/Root/A/E", "/Root/E",
            {"/Root/A/B" : "/Root/B",
             "/Root/A/C" : "/Root/C",
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E"})

        # Verify edits are only for the new relocates and are only to be applied
        # sublayer 1.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (sub1Layer, 
                [("/Root/A/D", "/Root/D"),
                 ("/Root/A/E", "/Root/E")])])

        # Create a new builder that specifies new relocates should be added
        # to the session layer.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack, sessionLayer)

        # Add one new relocate that doesn't require any updates to existing 
        # relocates.
        self._AddRelocateAndVerify(builder, "/Root/A/F", "/Root/F",
            {"/Root/A/B" : "/Root/B",
             "/Root/A/C" : "/Root/C",
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E",
             "/Root/A/F" : "/Root/F"})
        # Add another new relocate that ends up just changing an existing 
        # relocate's target path.
        self._AddRelocateAndVerify(builder, "/Root/C", "/Root/F/C",
            {"/Root/A/B" : "/Root/B",
             "/Root/A/C" : "/Root/F/C", # updated relocate target path
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E",
             "/Root/A/F" : "/Root/F"})

        # Verify that we have two layer edits:
        # 1. The new relocate is added to the session layer.
        # 2. The updated relocate target path is an update to the authored 
        #    relocates in the root layer where it was originally authored.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (sessionLayer, 
                [("/Root/A/F", "/Root/F")]),
            (rootLayer, 
                [("/Root/A/B", "/Root/B"),
                 ("/Root/A/C", "/Root/F/C")])])
        
        # For the next example, first copy the root layer relocates to 
        # sublayer2 before creating a new builder that will add new relocates
        # to the root layer. Copying relocates to sublayer2 did not change the
        # layer stack's relocates since they are redundant.
        self._ApplyEdits(cache, [(sub2Layer, rootLayer.relocates)])
        self.assertEqual(sub2Layer.relocates, 
                         [("/Root/A/B", "/Root/B"),
                          ("/Root/A/C", "/Root/F/C")])
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack, rootLayer)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict(
                {"/Root/A/B" : "/Root/B",
                 "/Root/A/C" : "/Root/F/C",
                 "/Root/A/D" : "/Root/D",
                 "/Root/A/E" : "/Root/E",
                 "/Root/A/F" : "/Root/F"}))

        # Add a relocate that updates an existing relocate that is authored in
        # the root layer (and redundantly in sublayer2)
        self._AddRelocateAndVerify(builder, "/Root/B", "/Root/A/G/B",
            {"/Root/A/B" : "/Root/A/G/B", # updated relocate
             "/Root/A/C" : "/Root/F/C",
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E",
             "/Root/A/F" : "/Root/F"}) 
        # Add another relocate that is added as new relocate and updates the
        # previously just updated relocate.
        self._AddRelocateAndVerify(builder, "/Root/A/G", "/Root/G",
            {"/Root/A/B" : "/Root/G/B", # updated relocate
             "/Root/A/C" : "/Root/F/C",
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E",
             "/Root/A/F" : "/Root/F",
             "/Root/A/G" : "/Root/G"}) # new relocate

        # Verify that the layer edits update both the relocates metadata in 
        # root layer and sublayer2 since the udpated relocate is authored in
        # both. However the new relocate entry is only added to the root layer's
        # relocates.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, 
                [("/Root/A/B", "/Root/G/B"),
                 ("/Root/A/C", "/Root/F/C"),
                 ("/Root/A/G", "/Root/G")]),
            (sub2Layer, 
                [("/Root/A/B", "/Root/G/B"),
                 ("/Root/A/C", "/Root/F/C")])])
        
        # Remove the relocate from /Root/A/C
        self._RemoveRelocateAndVerify(builder, "/Root/A/C",
            {"/Root/A/B" : "/Root/G/B",
             "/Root/A/D" : "/Root/D",
             "/Root/A/E" : "/Root/E",
             "/Root/A/F" : "/Root/F",
             "/Root/A/G" : "/Root/G"})

        # Verify that the layer edits remove the /Root/A/C relocate in both the
        # root layer and sublayer2 since the entry existed in both.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, 
                [("/Root/A/B", "/Root/G/B"),
                 ("/Root/A/G", "/Root/G")]),
            (sub2Layer, 
                [("/Root/A/B", "/Root/G/B")])])

    def test_ExistingRelocatesErrors(self):

        # New layer stack. It has four layers: a session layer and a root layer
        # with two sublayers
        cache = self._CreateTestLayersAndPcpCache()
        sessionLayer = cache.layerStack.layers[0]
        rootLayer = cache.layerStack.layers[1]
        sub1Layer = cache.layerStack.layers[2]
        sub2Layer = cache.layerStack.layers[3]

        # Add relocates to our layers where some the relocates will be invalid
        # for multiple reasons and will be rejected as error's from the layer
        # stack's relocates.
        self._ApplyEdits(cache, [
            (rootLayer, [
                ('/World', '/NewWorld'), # invalid authored relocate
                ('/Root/A', '/Root/B'),
                ('/Root/X', '/Root/Y'), # target is another's source
                ('/Root/RootTarget', '/Root/SameTarget')]), # same target               
            (sub1Layer, [
                ('/Root/C', '/Root/D'),
                ('/Root/C/Foo', '/Root/D/Bar'), # source is child of relocate source
                ('/Root/Y', '/Root/Z'), # source is another's target
                ('/Root/Sub1Target', '/Root/SameTarget')]), # same target
            (sub2Layer, [
                ('/Root/B', '/Root/B/C'), # invalid authored relocate
                ('/Root/E', '/Root/F'),
                ('/Root/Sub2Target', '/Root/SameTarget')]), # same target
        ])
        # Verify the layer stack's relocates are only the valid relocates above.
        self.assertEqual(cache.layerStack.incrementalRelocatesSourceToTarget,
            _FormatRelocatesDict({
                "/Root/A" : "/Root/B",
                "/Root/C" : "/Root/D",
                "/Root/E" : "/Root/F",
            }))

        # Create an edit builder for the layer stack. It's relocates map will
        # be the same as the layer stack's to start.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict({
                "/Root/A" : "/Root/B",
                "/Root/C" : "/Root/D",
                "/Root/E" : "/Root/F",
            }))

        # In this case the builder WILL start with suggested edits that would
        # remove the invalide relocates from the layers that have them. Applying
        # these edits clean up the layer relocates to have no errors.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, [
                ('/Root/A', '/Root/B')]),
            (sub1Layer, [
                ('/Root/C', '/Root/D')]),
            (sub2Layer, [
                ('/Root/E', '/Root/F')])])

        # Reset the relocates on the root layer and sublayer 2 to empty and add
        # a bunch of valid relocates to sublayer 1
        self._ApplyEdits(cache, [
            (rootLayer, []),
            (sub1Layer, [
                ('/Root/A', '/Root/B'),
                ('/Root/C', '/Root/B/C'),
                ('/Root/D', '/Root/B/D'),
                ('/Root/F', '/Root/B/C/G'),
                ('/Root/E', '/Root/B/D/G')]),
            (sub2Layer, []),
        ])
        # Assert all the sublayer 1 relocates are present in the layer stack.
        self.assertEqual(cache.layerStack.incrementalRelocatesSourceToTarget,
            _FormatRelocatesDict({
                '/Root/A' : '/Root/B',
                '/Root/C' : '/Root/B/C',
                '/Root/D' : '/Root/B/D',
                '/Root/F' : '/Root/B/C/G',
                '/Root/E' : '/Root/B/D/G'
            }))
        # A relocate builder starts with these relocates and produces no 
        # edits to start with.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict({
                '/Root/A' : '/Root/B',
                '/Root/C' : '/Root/B/C',
                '/Root/D' : '/Root/B/D',
                '/Root/F' : '/Root/B/C/G',
                '/Root/E' : '/Root/B/D/G'
            }))
        self.assertEqual(builder.GetEdits(), [])

        # Now update sublayer 2 to have a bunch of relocates using the same 
        # source paths as our existing relocates, but with all these being 
        # invalid relocates.
        self._ApplyEdits(cache, [
            (sub2Layer, [
                ('/Root/A', '/Root/A/B'), # invalid authored relocate
                ('/Root/C', '/Root/D'), # target is another's source
                ('/Root/D', '/Root/E'), # source is another's target
                ('/Root/F', '/Root/G'), # same target
                ('/Root/E', '/Root/G')]), # same target
        ])
        # Because sublayer 2 is weaker and sublayer has a relocate for each of
        # these paths, the valid sublayer 1 relocates are all stronger and will
        # produce the same relocates in the layer stack as before.
        self.assertEqual(cache.layerStack.incrementalRelocatesSourceToTarget,
            _FormatRelocatesDict({
                '/Root/A' : '/Root/B',
                '/Root/C' : '/Root/B/C',
                '/Root/D' : '/Root/B/D',
                '/Root/F' : '/Root/B/C/G',
                '/Root/E' : '/Root/B/D/G'
            }))
        # An edit builder starts with same relocates as well. However, because
        # sublayer 2 has an invalid authored relocate, the builder will start
        # with edits that remove this (and only this) invalid relocates from
        # sublayer 2. The other relocates would only be invalid if they were the
        # strongest opinion so we leave those relocates intact.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict({
                '/Root/A' : '/Root/B',
                '/Root/C' : '/Root/B/C',
                '/Root/D' : '/Root/B/D',
                '/Root/F' : '/Root/B/C/G',
                '/Root/E' : '/Root/B/D/G'
            }))
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (sub2Layer, [
                ('/Root/C', '/Root/D'),
                ('/Root/D', '/Root/E'),
                ('/Root/F', '/Root/G'),
                ('/Root/E', '/Root/G')])])

        # Now update the root layer to have the same invalid relocates as we 
        # set on sublayer 2 before.
        self._ApplyEdits(cache, [
            (rootLayer, [
                ('/Root/A', '/Root/A/B'), # invalid authored relocate
                ('/Root/C', '/Root/D'), # target is another's source
                ('/Root/D', '/Root/E'), # source is another's target
                ('/Root/F', '/Root/G'), # same target
                ('/Root/E', '/Root/G')]), # same target
        ])
        # Because root layer 1 is stronger, it's relocates will win over 
        # sublayer 1 making these relocates invalid. The only exception is 
        # invalid authored relocate /Root/A -> /Root/A/B which is ignored when
        # processing root layer as it will never be valid in any context. 
        # Ignoring this relocate allows the relocate of /Root/A -> /Root/B on 
        # sublayer 1 to come through so only that relocate is present on the 
        # layer stack.
        self.assertEqual(cache.layerStack.incrementalRelocatesSourceToTarget,
            _FormatRelocatesDict({
                "/Root/A" : "/Root/B",
            }))
        # An edit builder on the layer stack produces the same valid relocates.
        builder = Pcp.LayerRelocatesEditBuilder(cache.layerStack)
        self.assertEqual(builder.GetEditedRelocatesMap(), 
            _FormatRelocatesDict({
                "/Root/A" : "/Root/B",
            }))
        # This time the initial edits produced remove all relocates in ALL 
        # layers that use a source path of the invalid relocates from the root
        # layer. This is necessary to make sure that removing the invalid 
        # relocates from the root layer doesn't expose opinions from the weaker
        # layers that could change the relocates mapping of the layer stack.
        self._ApplyAndVerifyExpectedEdits(builder, cache, [
            (rootLayer, []),
            (sub1Layer, [
                ('/Root/A', '/Root/B')]),
            (sub2Layer, [])])

if __name__ == "__main__":
    unittest.main()
