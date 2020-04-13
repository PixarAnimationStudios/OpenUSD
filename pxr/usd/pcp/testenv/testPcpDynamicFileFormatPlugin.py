#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Sdf, Pcp, Plug, Vt
import os, unittest

class TestPcpDynamicFileFormatPlugin(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        testRoot = os.path.join(os.path.dirname(__file__), 'PcpPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'

        # Register dso plugins.  Discard possible exception due to TestPlugDsoEmpty.
        # The exception only shows up here if it happens in the main thread so we
        # can't rely on it.
        try:
            Plug.Registry().RegisterPlugins(testPluginsDsoSearch)
        except RuntimeError:
            pass

    def setUp(self):
        # We expect there to be no layers left loaded when we start each test
        # case so we can start fresh. By the tearDown completes this needs to 
        # be true.
        assert not Sdf.Layer.GetLoadedLayers()

    def _CreatePcpCache(self, rootLayer):
        return Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

    def test_FileFormat(self):

        print("\ntest_FileFormat Start\n")

        # Open the cone.testpcpdynamic file with no arguments. This will
        # read the contents in as a normal sdf file
        dynamicConeFile = 'cone.testpcpdynamic'
        noArgConeLayer = Sdf.Layer.FindOrOpen(dynamicConeFile)
        assert noArgConeLayer
        assert noArgConeLayer.GetFileFormat().formatId == "Test_PcpDynamicFileFormat"
        # Compare the contents against the no argument baseline.
        baselineConeLayer = Sdf.Layer.FindOrOpen('baseline/cone_0.sdf')
        assert baselineConeLayer
        assert noArgConeLayer.ExportToString() == baselineConeLayer.ExportToString()
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(noArgConeLayer, True)
        assert noArgConeLayer.ExportToString() == baselineConeLayer.ExportToString()

        # Open the sphere.testpcpdynamic file with no arguments. This will
        # read the contents in as a normal sdf file
        dynamicSphereFile = 'sphere.testpcpdynamic'
        noArgSphereLayer = Sdf.Layer.FindOrOpen(dynamicSphereFile)
        assert noArgSphereLayer
        assert noArgSphereLayer.GetFileFormat().formatId == "Test_PcpDynamicFileFormat"
        # Compare the contents against the no argument baseline.
        baselineSphereLayer = Sdf.Layer.FindOrOpen('baseline/sphere_0.sdf')
        assert baselineSphereLayer
        assert noArgSphereLayer.ExportToString() == baselineSphereLayer.ExportToString()
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(noArgSphereLayer, True)
        assert noArgSphereLayer.ExportToString() == baselineSphereLayer.ExportToString()

        # Now open the dynamic cone file with file format arguments for 
        # depth and num. The contents will be dynamicly generated.
        procConeLayer = Sdf.Layer.FindOrOpen(dynamicConeFile, 
                                             {"TestPcp_depth":"3", "TestPcp_num":"2"})
        assert procConeLayer
        assert procConeLayer.GetFileFormat().formatId == "Test_PcpDynamicFileFormat"
        baselineProcLayer = Sdf.Layer.FindOrOpen('baseline/proc_3_2.sdf')
        assert baselineProcLayer
        # The baseline comparison file uses a placeholder asset path so update
        # it with the cone file's real path (converted to '/' on windows) and 
        # then compare against the dynamic baseline
        refConeLayerPath = procConeLayer.realPath.replace('\\', '/')
        baselineProcLayer.UpdateExternalReference('placeholder.sdf', 
                                                  refConeLayerPath)
        assert procConeLayer.ExportToString() == baselineProcLayer.ExportToString()
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(procConeLayer, True)
        assert procConeLayer.ExportToString() == baselineProcLayer.ExportToString()

        # Open the dynamic sphere file with the same file format arguments.
        # The dynamic contents should be exactly the same as the cone, but
        # the asset paths replaced with the sphere asset.
        procSphereLayer = Sdf.Layer.FindOrOpen(dynamicSphereFile, 
                                               {"TestPcp_depth":"3", "TestPcp_num":"2"})
        assert procSphereLayer
        assert procSphereLayer.GetFileFormat().formatId == "Test_PcpDynamicFileFormat"
        refSphereLayerPath = procSphereLayer.realPath.replace('\\', '/')
        baselineProcLayer.UpdateExternalReference(refConeLayerPath, 
                                                  refSphereLayerPath)
        assert procSphereLayer.ExportToString() == baselineProcLayer.ExportToString()
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(procSphereLayer, True)
        assert procSphereLayer.ExportToString() == baselineProcLayer.ExportToString()

        print("test_FileFormat Success!\n")
            
    def _GeneratePrimIndexPaths(self, rootPrimPath, depth, num, expectedNumPaths, payloadId = None):
        # Helper to generate all the payload paths that need to included to 
        # load the dynamicly generated subtree at rootPrimPath with the
        # given depth and num

        # Paths are generated recursively
        def _GenerateRecursivePrimIndexPaths(rootPrimPath, depth, num):
            # Add this prim itself
            dynamicPaths = [rootPrimPath]
            # Depth decreases by one at each child level
            childDepth = depth-1
            if childDepth > 0:
                # Recursively add each child and its generated children.
                for i in range(num):
                    childPrimPath = "{0}/Xform_{1}_{2}_{3}".format(
                        rootPrimPath, "" if payloadId is None else payloadId, 
                        childDepth, i)
                    dynamicPaths.extend(
                        _GenerateRecursivePrimIndexPaths(
                            childPrimPath, childDepth, num))
            return dynamicPaths

        paths = _GenerateRecursivePrimIndexPaths(rootPrimPath, depth, num)
        # Verify that we generated the number of paths we expected. This is an
        # invariant however it's helpful to force us to provide the number of
        # expected paths so that we understand what expect from the test.
        assert len(paths) == expectedNumPaths
        return paths

    def _VerifyFoundDynamicPayloads(self, cache, payloads):
        for payload in payloads:
            print ("Finding prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            primIndex = cache.FindPrimIndex(payload)
            assert primIndex.IsValid()
            # The prim index for the payload will have dynamic file format
            # dependency data.
            assert cache.GetDynamicFileFormatArgumentDependencyData(payload)

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            geomIndex = cache.FindPrimIndex(payload + "/geom")
            assert geomIndex.IsValid()
            # The geom reference prim index will not have dynamic file format
            # dependency data.
            assert cache.GetDynamicFileFormatArgumentDependencyData(payload + "/geom").IsEmpty()

    def _VerifyNotFoundDynamicPayloads(self, cache, payloads):
        for payload in payloads:
            print ("Finding prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            primIndex = cache.FindPrimIndex(payload)
            assert primIndex is None
            assert cache.GetDynamicFileFormatArgumentDependencyData(payload).IsEmpty()

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            geomIndex = cache.FindPrimIndex(payload + "/geom")
            assert geomIndex is None

    def _VerifyComputeDynamicPayloads(self, cache, payloads, hasPayloadId=False):

        for payload in payloads:
            print ("Computing prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            (primIndex, err) = cache.ComputePrimIndex(payload)
            assert primIndex.IsValid()
            assert not err
            # The prim index for the payload will have dynamic file format
            # dependency data.
            depData = cache.GetDynamicFileFormatArgumentDependencyData(payload)
            assert depData
            if hasPayloadId:
                assert (sorted(depData.GetRelevantFieldNames()) == 
                        ["TestPcp_argDict", "TestPcp_depth", "TestPcp_height",
                         "TestPcp_num", "TestPcp_radius"])
            else:
                assert (sorted(depData.GetRelevantFieldNames()) == 
                        ["TestPcp_depth", "TestPcp_height", "TestPcp_num", 
                         "TestPcp_radius"])

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            (geomIndex, err) = cache.ComputePrimIndex(payload + "/geom")
            assert geomIndex.IsValid()
            assert not err
            # The geom reference prim index will not have dynamic file format
            # dependency data.
            assert cache.GetDynamicFileFormatArgumentDependencyData(payload + "/geom").IsEmpty()

    def _TestChangeInfo(self, cache, prim, field, newValue, expectedSignificantChanges):
        # Test that authoring a new value for an relevant field like 
        # height does cause significant changes.
        for expected in expectedSignificantChanges:
            dep = cache.GetDynamicFileFormatArgumentDependencyData(expected)
            oldValue = prim.GetInfo(field) if prim.HasInfo(field) else None
            assert dep.CanFieldChangeAffectFileFormatArguments(
                field, oldValue, newValue)

        with Pcp._TestChangeProcessor(cache) as cp:
            prim.SetInfo(field, newValue)
            assert cp.GetSignificantChanges() == expectedSignificantChanges, \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

    def test_BasicRead(self):
        print("\ntest_Read Start\n")

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootCone
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        assert rootLayer
        cache = self._CreatePcpCache(rootLayer)
                                        
        # Payloads for /RootCone - depth = 4, num = 3 : produces 40 payloads                                        
        payloads = self._GeneratePrimIndexPaths("/RootCone", 4, 3, 40)
        cache.RequestPayloads(payloads,[])
                                                                                                        
        # Compute prim indices for each of the dynamic payloads and verify
        # they were generated correctly.                                                                                                        
        self._VerifyComputeDynamicPayloads(cache, payloads)

        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        assert Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"4", "TestPcp_num":"3", "TestPcp_radius":"50"}))
        assert Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"3", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"25"}))
        assert Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"12.5"}))
        assert Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"6.25"}))

        print("test_BasicRead Success!\n")

    def test_Changes(self):
        print("\ntest_Changes Start\n")

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootSphere
        # and /RootMulti as well.
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        assert rootLayer
        cache = self._CreatePcpCache(rootLayer)

        # Payloads prims for /RootSphere - depth = 4, num = 3 : produces 40 
        # payloads
        payloads = self._GeneratePrimIndexPaths("/RootSphere", 4, 3, 40)
        cache.RequestPayloads(payloads,[])

        # Verify that relevant fields are not in the possible arguments list
        # for the cache yet before the prim indices that use them are computed.
        assert not cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius"]:
            assert not cache.IsPossibleDynamicFileFormatArgumentField(field)
        # Verify the same for irrelevant fields
        assert not cache.IsPossibleDynamicFileFormatArgumentField("documentation")
        assert not cache.IsPossibleDynamicFileFormatArgumentField("TestPcp_argDict")

        # Compute and verify the prim indices for the dynamic payloads.
        self._VerifyComputeDynamicPayloads(cache, payloads)

        # Verify that the relevant fields are now possible dynamic arguments.
        assert cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius"]:
            assert cache.IsPossibleDynamicFileFormatArgumentField(field)
        # Verify that irrelevant fields are never possible dynamic arguments
        assert not cache.IsPossibleDynamicFileFormatArgumentField("documentation")
        assert not cache.IsPossibleDynamicFileFormatArgumentField("TestPcp_argDict")

        # Test that authoring a new value for an irrelevant field like 
        # documentation does not cause any significant changes.
        self._TestChangeInfo(cache, rootLayer.GetPrimAtPath('/RootSphere'),
                             'documentation', 'update documentation', 
                             [])
        # Verify that all the cached prim indices still exist
        self._VerifyFoundDynamicPayloads(cache, payloads)

        # Test that authoring a new value for an relevant field on the root 
        # prim like height does cause significant changes.
        self._TestChangeInfo(cache, rootLayer.GetPrimAtPath('/RootSphere'),
                             'TestPcp_height', 2.0, 
                             ['/RootSphere'])
        # Verify that all the cached dynamic prim indices were invalidated
        self._VerifyNotFoundDynamicPayloads(cache, payloads)
        # Verify that the invalidation of ALL dynamic prim indices
        # removed the relevant fields from possible dynamic arguments.
        assert not cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius"]:
            assert not cache.IsPossibleDynamicFileFormatArgumentField(field)

        # Recompute the dynamic payload prim indices.
        self._VerifyComputeDynamicPayloads(cache, payloads)

        # Verify that relevant fields are possible dynamic arguments again.
        assert cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius"]:
            assert cache.IsPossibleDynamicFileFormatArgumentField(field)

        # Assert that we can find the params layer that was referenced by in by 
        # /RootSphere.
        assert Sdf.Layer.Find("params.sdf")
        # FindOrOpen the params layer so that we still have an open reference
        # to it when we change it. Otherwise the layer might get deleted when
        # the prim indexes referencing it are invalidated and we could lose the
        # changes.
        paramsLayer = Sdf.Layer.FindOrOpen("params.sdf")
        # Verify that changing num on the referenced prim causes a significant
        # change to /RootSphere as it is used to compose the dynamic arguments
        self._TestChangeInfo(cache, paramsLayer.GetPrimAtPath('/Params'),
                             'TestPcp_num', 2, 
                             ['/RootSphere'])

        # Verify all dynamic payload prim indices are invalid one again.
        self._VerifyNotFoundDynamicPayloads(cache, payloads)

        # Generate the new list of dynamic payload prims for the new metadata
        # of num = 2
        newPayloads = self._GeneratePrimIndexPaths("/RootSphere", 4, 2, 15)

        # Verify that we can compute the new payload prim indices.
        # Note that we don't have to update the requested payloads since the 
        # original set of payloads is actually a super set of our new set.
        self._VerifyComputeDynamicPayloads(cache, newPayloads)

        # Test changing relevant info over a dynamic generated subprim spec.
        subprimSpec = rootLayer.GetPrimAtPath('/RootSphere/Xform__3_0')
        # Note that we defined an over in root.sdf so that this spec would 
        # exist for convenience.
        assert subprimSpec
        # Add depth metadata info to the subprim. This causes a significant 
        # change to the subprim only.
        self._TestChangeInfo(cache, subprimSpec,
                             'TestPcp_depth', 4, 
                             ['/RootSphere/Xform__3_0'])
        # Verify that the dynamic prims at and below this prim have gone 
        # invalid.
        self._VerifyNotFoundDynamicPayloads(cache, [
            '/RootSphere/Xform__3_0',
            '/RootSphere/Xform__3_0/Xform__2_0',
            '/RootSphere/Xform__3_0/Xform__2_0/Xform__1_0',
            '/RootSphere/Xform__3_0/Xform__2_0/Xform__1_1',
            '/RootSphere/Xform__3_0/Xform__2_1',
            '/RootSphere/Xform__3_0/Xform__2_1/Xform__1_0',
            '/RootSphere/Xform__3_0/Xform__2_1/Xform__1_1'
            ])
        # Verify that all the other dynamic prims did not go invalid
        self._VerifyFoundDynamicPayloads(cache, [
            '/RootSphere', 
            '/RootSphere/Xform__3_1',
            '/RootSphere/Xform__3_1/Xform__2_0',
            '/RootSphere/Xform__3_1/Xform__2_0/Xform__1_0',
            '/RootSphere/Xform__3_1/Xform__2_0/Xform__1_1',
            '/RootSphere/Xform__3_1/Xform__2_1',
            '/RootSphere/Xform__3_1/Xform__2_1/Xform__1_0',
            '/RootSphere/Xform__3_1/Xform__2_1/Xform__1_1'
            ])
        # Regenerate the new payload prim paths at and below the subprim. The
        # depth has been reset to 4 at this level
        newSubprimPayloads = self._GeneratePrimIndexPaths(
            '/RootSphere/Xform__3_0', 4, 2, 15)
        # We have new paths so request the new payloads
        cache.RequestPayloads(newSubprimPayloads, [])
        # Verify that we can compute these new prim indices.
        self._VerifyComputeDynamicPayloads(cache, newSubprimPayloads)

        # Test changing a relevant field on a dynamically generated subprim
        # that is not itself dynamic.
        geomSpec = rootLayer.GetPrimAtPath('/RootSphere/geom')
        # Note that we defined an over in root.sdf so that this spec would 
        # exist for convenience.
        assert geomSpec
        assert cache.FindPrimIndex('/RootSphere/geom')
        # Set depth on the existing geom spec. Verify that it doesn't cause any
        # significant changes to any prim indices even though it's a possible
        # dynamic argument field.
        assert cache.IsPossibleDynamicFileFormatArgumentField('TestPcp_depth')
        self._TestChangeInfo(cache, geomSpec,
                             'TestPcp_depth', 4, 
                             [])

        # Test that authoring a new value for a possible dynamic argument
        # that is determined to not be relevant by the file format's conditions
        # doesn't cause a significant change. argDict is defined in the test
        # file format as only being relevant if a payload has a 'payloadId'
        # already included as a file format argument in its asset path. 
        # /RootSphere has no payloads with this condition so argDict changes
        # do not affect it.
        self._TestChangeInfo(cache, rootLayer.GetPrimAtPath('/RootSphere'),
                             'TestPcp_argDict', {}, 
                             [])

        # Test this same case with RootMulti to prove it still works. /RootMulti
        # defines two payloads, each with an asset path that includes a 
        # payloadId file format argument so argDict will be a relevant field.
        # Note that this is just a small subset of the payload prims under 
        # /RootMulti. We don't need all of them for this test example.
        rootMultiPayloads = self._GeneratePrimIndexPaths(
            "/RootMulti/Xform_1_2_0", 2, 3, 4, payloadId=1)
        cache.RequestPayloads(rootMultiPayloads + ["/RootMulti"], [])
        self._VerifyComputeDynamicPayloads(cache, rootMultiPayloads, hasPayloadId=True)
        self._TestChangeInfo(cache, rootLayer.GetPrimAtPath('/RootMulti'),
                             'TestPcp_argDict', {}, 
                             ["/RootMulti"])
        self._VerifyNotFoundDynamicPayloads(cache, rootMultiPayloads)

        print("test_Changes Success!\n")

    def test_SubrootRefChange(self):
        print("\ntest_SubrootRefChange Start\n")

        # Create a PcpCache for subrootref.sdf. This file contains a single 
        # /Root prim with a subroot reference to a child of /RootMulti in 
        # root.sdf
        rootLayerFile = 'subrootref.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        assert rootLayer
        cache = self._CreatePcpCache(rootLayer)

        # Payloads prims for /Root - depth = 2, num = 4 : produces 5 
        # payloads. Note that the depth is one less than actually defined on 
        # /RootMulti as the reference is to a child of RootMulti.
        payloads = self._GeneratePrimIndexPaths("/Root", 2, 4, 5, payloadId=1)

        # XXX: Because an outstanding bug with subroot referercing prim inside
        # of ancestor payloads, we need to request payloads for /RootMulti as
        # well.
        cache.RequestPayloads(payloads + ["/RootMulti"],[])

        assert not cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius", "TestPcp_argDict"]:
            assert not cache.IsPossibleDynamicFileFormatArgumentField(field)

        # Compute and verify the prim indices for the dynamic payloads.
        self._VerifyComputeDynamicPayloads(cache, payloads, hasPayloadId=True)

        # Because our primIndices include RootMulti which has payloads with
        # a "payloadId" format argument, "argDict" becomes another possible
        # dynamic argument field.
        assert cache.HasAnyDynamicFileFormatArgumentDependencies()
        for field in ["TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius", "TestPcp_argDict"]:
            assert cache.IsPossibleDynamicFileFormatArgumentField(field)

        # Change height on the root prim. Verify this is a significant change
        # on the Root prim as it references in a subprim that is itself still
        # dynamic
        self._TestChangeInfo(cache, rootLayer.GetPrimAtPath('/Root'),
                             'TestPcp_height', 2.0, 
                             ['/Root'])
        # Verify our prim indices were invalidated.
        self._VerifyNotFoundDynamicPayloads(cache, payloads)

        # Recompute the prim indices.
        self._VerifyComputeDynamicPayloads(cache, payloads, hasPayloadId=True)

        # Assert that we can find root.sdf which was referenced in to /Root. It
        # will already be open.
        assert Sdf.Layer.Find("root.sdf")
        # FindOrOpen the layer so that we still have an open reference
        # to it when we change it. Otherwise the layer might get deleted when
        # the prim indexes referencing it are invalidated and we could lose the
        # changes.
        refLayer = Sdf.Layer.FindOrOpen("root.sdf")

        # Change num the argDict on /RootMulti on the referenced layer which is
        # parent prim to referenced subprim. This will cause a significant 
        # change to /Root. The change to argDict is a significant change to 
        # /RootMulti which is significant to all of /RootMulti's children and 
        # therefore significant to any prim referencing one of those children.
        self._TestChangeInfo(cache, refLayer.GetPrimAtPath('/RootMulti'),
                             'TestPcp_argDict', {"1": {"TestPcp_num":5}}, 
                             ['/Root'])
        self._VerifyNotFoundDynamicPayloads(cache, payloads)

        # There will now be an extra dynamic payload available under root.
        # Verify that we can compute all the new prim indices.
        newPayloads = self._GeneratePrimIndexPaths("/Root", 2, 5, 6, payloadId=1)
        cache.RequestPayloads(newPayloads + ["/RootMulti"],[])
        self._VerifyComputeDynamicPayloads(cache, newPayloads, hasPayloadId=True)

        # XXX: Todo: Add another case here for making a metadata change in 
        # params.sdf which is referenced by root.sdf. This would be expected
        # to propagate to /Root in subrootref.sdf. But right now there is 
        # another bug related to subroot references which causes the node
        # provided by params.sdf to be culled from the subroot ref tree.

        print ("Computing prim index for " + "/SubrootGeomRef")
        # Verify each dynamic payload's prim index is computed 
        # without errors and has dynamic file arguments
        (primIndex, err) = cache.ComputePrimIndex("/SubrootGeomRef")
        assert primIndex.IsValid()
        assert not err
        self._TestChangeInfo(cache, refLayer.GetPrimAtPath('/RootMulti'),
                             'TestPcp_argDict', {"1": {"TestPcp_num":3}}, 
                             ['/Root', '/SubrootGeomRef'])

        print("test_SubrootRefChange Success\n")

if __name__ == "__main__":
    unittest.main()
