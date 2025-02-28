#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Pcp, Plug, Tf, Vt
import os, unittest

# Get whether the env var is set for using attribute values instead of field
# values. Note that this is the env var that matches the EnvSetting defined in 
# the test file format plugin. But we can't use Tf.GetEnvSetting as that plugin
# isn't loaded until the first time a prim index that needs it is composed.
USE_ATTRS = \
    os.environ['TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS_USE_ATTRIBUTE_INPUTS'] == "1"

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
        self.assertFalse(Sdf.Layer.GetLoadedLayers())

    def _CreatePcpCache(self, rootLayer, usd=False):
        return Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd=usd)

    def test_FileFormat(self):

        print("\ntest_FileFormat Start\n")

        # Open the cone.testpcpdynamic file with no arguments. This will
        # read the contents in as a normal sdf file
        dynamicConeFile = 'cone.testpcpdynamic'
        noArgConeLayer = Sdf.Layer.FindOrOpen(dynamicConeFile)
        self.assertTrue(noArgConeLayer)
        self.assertEqual(noArgConeLayer.GetFileFormat().formatId,
                         "Test_PcpDynamicFileFormat")
        # Compare the contents against the no argument baseline.
        baselineConeLayer = Sdf.Layer.FindOrOpen('baseline/cone_0.sdf')
        self.assertTrue(baselineConeLayer)
        self.assertEqual(noArgConeLayer.ExportToString(),
                         baselineConeLayer.ExportToString())
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(noArgConeLayer, True)
        self.assertEqual(noArgConeLayer.ExportToString(), 
                         baselineConeLayer.ExportToString())

        # Open the sphere.testpcpdynamic file with no arguments. This will
        # read the contents in as a normal sdf file
        dynamicSphereFile = 'sphere.testpcpdynamic'
        noArgSphereLayer = Sdf.Layer.FindOrOpen(dynamicSphereFile)
        self.assertTrue(noArgSphereLayer)
        self.assertEqual(noArgSphereLayer.GetFileFormat().formatId,
                        "Test_PcpDynamicFileFormat")
        # Compare the contents against the no argument baseline.
        baselineSphereLayer = Sdf.Layer.FindOrOpen('baseline/sphere_0.sdf')
        self.assertTrue(baselineSphereLayer)
        self.assertEqual(noArgSphereLayer.ExportToString(),
                         baselineSphereLayer.ExportToString())
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(noArgSphereLayer, True)
        self.assertEqual(noArgSphereLayer.ExportToString(),
                         baselineSphereLayer.ExportToString())

        # Now open the dynamic cone file with file format arguments for 
        # depth and num. The contents will be dynamicly generated.
        procConeLayer = Sdf.Layer.FindOrOpen(dynamicConeFile, 
            {"TestPcp_depth":"3", "TestPcp_num":"2"})
        self.assertTrue(procConeLayer)
        self.assertEqual(procConeLayer.GetFileFormat().formatId,
                         "Test_PcpDynamicFileFormat")

        # Read produces different procedural layers depending on whether we're 
        # using attribute inputs vs metadata inputs.
        if USE_ATTRS:
            baselineProcLayer = Sdf.Layer.FindOrOpen(
                'baseline/proc_attr_3_2.sdf')
        else:
            baselineProcLayer = Sdf.Layer.FindOrOpen(
                'baseline/proc_metadata_3_2.sdf')
        self.assertTrue(baselineProcLayer)
        # The baseline comparison file uses a placeholder asset path so update
        # it with the cone file's real path (converted to '/' on windows) and 
        # then compare against the dynamic baseline
        refConeLayerPath = procConeLayer.realPath.replace('\\', '/')
        baselineProcLayer.UpdateCompositionAssetDependency('placeholder.sdf', 
                                                           refConeLayerPath)
        self.assertEqual(procConeLayer.ExportToString(),
                         baselineProcLayer.ExportToString())
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(procConeLayer, True)
        self.assertEqual(procConeLayer.ExportToString(),
                         baselineProcLayer.ExportToString())

        # Open the dynamic sphere file with the same file format arguments.
        # The dynamic contents should be exactly the same as the cone, but
        # the asset paths replaced with the sphere asset.
        procSphereLayer = Sdf.Layer.FindOrOpen(dynamicSphereFile, 
            {"TestPcp_depth":"3", "TestPcp_num":"2"})
        self.assertTrue(procSphereLayer)
        self.assertEqual(procSphereLayer.GetFileFormat().formatId,
                         "Test_PcpDynamicFileFormat")
        refSphereLayerPath = procSphereLayer.realPath.replace('\\', '/')
        baselineProcLayer.UpdateCompositionAssetDependency(refConeLayerPath, 
                                                           refSphereLayerPath)
        self.assertEqual(procSphereLayer.ExportToString(),
                         baselineProcLayer.ExportToString())
        # Force reload the procedural layer to make sure it still works correctly
        Sdf.Layer.Reload(procSphereLayer, True)
        self.assertEqual(procSphereLayer.ExportToString(),
                         baselineProcLayer.ExportToString())

        print("test_FileFormat Success!\n")
            
    def _GeneratePrimIndexPaths(self, rootPrimPath, depth, num, 
                                expectedNumPaths, payloadId = None):
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
        self.assertEqual(len(paths), expectedNumPaths)
        return paths

    def _VerifyFoundDynamicPayloads(self, cache, payloads):
        for payload in payloads:
            print ("Finding prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            primIndex = cache.FindPrimIndex(payload)
            self.assertTrue(primIndex.IsValid())
            # The prim index for the payload will have dynamic file format
            # dependency data.
            self.assertTrue(
                cache.GetDynamicFileFormatArgumentDependencyData(payload))

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            geomIndex = cache.FindPrimIndex(payload + "/geom")
            self.assertTrue(geomIndex.IsValid())
            # The geom reference prim index will not have dynamic file format
            # dependency data.
            self.assertTrue(
                cache.GetDynamicFileFormatArgumentDependencyData(payload + "/geom").IsEmpty())

    def _VerifyNotFoundDynamicPayloads(self, cache, payloads):
        for payload in payloads:
            print ("Finding prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            primIndex = cache.FindPrimIndex(payload)
            self.assertIsNone(primIndex)
            self.assertTrue(
                cache.GetDynamicFileFormatArgumentDependencyData(payload).IsEmpty())

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            geomIndex = cache.FindPrimIndex(payload + "/geom")
            self.assertIsNone(geomIndex)

    def _ComputeAndVerifyDynamicPayloads(self, cache, payloads, 
                                         expectedRelevantFieldOrAttrNames):
        for payload in payloads:
            print ("Computing prim index for " + payload)
            # Verify each dynamic payload's prim index is computed 
            # without errors and has dynamic file arguments
            (primIndex, err) = cache.ComputePrimIndex(payload)
            self.assertTrue(primIndex.IsValid())
            self.assertFalse(err)
            # The prim index for the payload will have dynamic file format
            # dependency data.
            depData = cache.GetDynamicFileFormatArgumentDependencyData(payload)
            assert depData
            if USE_ATTRS:   
                self.assertEqual(sorted(depData.GetRelevantAttributeNames()), 
                                 sorted(expectedRelevantFieldOrAttrNames))
            else:
                self.assertEqual(sorted(depData.GetRelevantFieldNames()), 
                                 sorted(expectedRelevantFieldOrAttrNames))

            # Verify that there is a geom spec under each payload and that
            # these do not have dynamic file arguments.
            (geomIndex, err) = cache.ComputePrimIndex(payload + "/geom")
            self.assertTrue(geomIndex.IsValid())
            self.assertFalse(err)
            # The geom reference prim index will not have dynamic file format
            # dependency data.
            self.assertTrue(
                cache.GetDynamicFileFormatArgumentDependencyData(payload + "/geom").IsEmpty())

    def _TestChangeMetadataValue(self, cache, prim, field, newValue, 
                                 expectedSignificantChanges):
        # Test that authoring a new value for an relevant field like 
        # height does cause significant changes.
        oldValue = prim.GetInfo(field) if prim.HasInfo(field) else None
        for expected in expectedSignificantChanges:
            dep = cache.GetDynamicFileFormatArgumentDependencyData(expected)
            self.assertTrue(dep.CanFieldChangeAffectFileFormatArguments(
                field, oldValue, newValue), 
                msg=("Field %s: %s -> %s" % (field, oldValue, newValue)))

        with Pcp._TestChangeProcessor(cache) as cp:
            prim.SetInfo(field, newValue)
            self.assertEqual(cp.GetSignificantChanges(), 
                             expectedSignificantChanges)

    def _TestChangeAttributeDefaultValue(self, cache, prim, attrName, newValue, 
                                         expectedSignificantChanges):
        # Test that authoring a new default value for an relevant attribute like 
        # height does cause significant changes.
        attrPath = prim.path.AppendProperty(attrName)
        attr = prim.GetAttributeAtPath(attrPath)
        oldValue = \
            attr.GetInfo("default") if attr and attr.HasInfo("default") else None

        for expected in expectedSignificantChanges:
            dep = cache.GetDynamicFileFormatArgumentDependencyData(expected)
            self.assertTrue(
                dep.CanAttributeDefaultValueChangeAffectFileFormatArguments(
                    attrName, oldValue, newValue), 
                msg=("Attribute %s: %s -> %s" % (attrName, oldValue, newValue)))

        with Pcp._TestChangeProcessor(cache) as cp:
            # Attribute spec may not exist already so create it if necessary
            # before setting the default value.
            if not attr:
                attr = Sdf.AttributeSpec(prim, attrName, 
                    Sdf.GetValueTypeNameForValue(newValue))
            attr.SetInfo("default", newValue)
            self.assertEqual(cp.GetSignificantChanges(), 
                             expectedSignificantChanges)

    def _TestChangeValue(self, cache, prim, fieldOrAttrName, newValue, 
                         expectedSignificantChanges):
        # Test that authoring a new value for an argumennt like height (via 
        # attribute or metadata value) does cause significant changes.
        if USE_ATTRS:
            self._TestChangeAttributeDefaultValue(cache, prim, fieldOrAttrName, 
                newValue, expectedSignificantChanges)
        else:
            self._TestChangeMetadataValue(cache, prim, fieldOrAttrName, 
                newValue, expectedSignificantChanges)

    def test_BasicRead(self):
        print("\ntest_Read Start\n")

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootCone
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Payloads for /RootCone - depth = 4, num = 3 : produces 40 payloads                                        
        payloads = self._GeneratePrimIndexPaths("/RootCone", 4, 3, 40)
        cache.RequestPayloads(payloads,[])
                                                                                                        
        # Compute prim indices for each of the dynamic payloads and verify
        # they were generated correctly.
        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])

        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"4", "TestPcp_num":"3", "TestPcp_radius":"50"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"3", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"25"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"12.5"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", 
                 "TestPcp_radius":"6.25"})))

        print("test_BasicRead Success!\n")

    def test_PayloadsInVariants(self):
        # Test dynamic payloads and arguments authored in variants.

        print("\ntest_PayloadsInVariants start\n")

        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # /Variant overrides the TestPcp_depth and TestPcp_num values that are
        # originally defined in params.sdf.
        payloads = self._GeneratePrimIndexPaths("/Variant", 5, 4, 341)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])

        print("\ntest_PayloadsInVariants Success!\n")

    def test_NestedVariants(self):
        # Exercise a scenario with nested variants, dynamic payloads and
        # references to those constructs that don't resolve as expected.
        print("\ntest_NestedVariants start\n")

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootCone
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Payloads for /ReferenceAndNestedVariants - depth = 2, num = 2 : produces 3 payloads  
        payloads = self._GeneratePrimIndexPaths("/ReferenceAndNestedVariants", 2, 2, 3)
        cache.RequestPayloads(payloads,[])

        # Compute prim indices for each of the dynamic payloads and verify
        # they were generated correctly.
        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        # Height should be set to 120, overriding the height 22 set in the variant
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_height":"120", "TestPcp_num":"2", 
                 "TestPcp_radius":"50"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"120", "TestPcp_num":"2", 
                 "TestPcp_radius":"25"})))

        print("\ntest_NestedVariants success!\n")

    def test_InheritsAndVariants(self):
        # Test that all inherits and variants can contribute
        # opinions to parameters for dynamic payloads

        print("\ntest_InheritsAndVariants start\n")

        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Inherits

        payloads = self._GeneratePrimIndexPaths("/Inherits", 2, 3, 4)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_num":"3", "TestPcp_radius":"50"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"25"
                 })))

        # Variants

        payloads = self._GeneratePrimIndexPaths("/VariantWithParams", 2, 3, 4)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_num":"3", "TestPcp_radius":"20"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"10"
                 })))
        
        payloads = self._GeneratePrimIndexPaths("/VariantWithParams2", 2, 3, 4)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_num":"3", "TestPcp_radius":"20"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"10"
                 })))
  
        print("\ntest_InheritsAndVariants Success!\n")
        
    def test_WeakerOpinions(self):
        # Test that opinions from weaker nodes can
        # affect parameters for stronger dynamic payloads
    
        print("\ntest_WeakerOpinions start\n")

        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Specializes arcs are weaker than payloads, but can still
        # affect parameters for dynamic payloads
        payloads = self._GeneratePrimIndexPaths("/Specializes", 3, 3, 13)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"3", "TestPcp_num":"3", "TestPcp_radius":"50"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"25"
                 })))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"12.5"
                 })))

        # Weaker siblings of the parent node can
        # affect parameters too
        payloads = self._GeneratePrimIndexPaths("/WeakerParentSibling", 2, 3, 4)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_num":"3", "TestPcp_radius":"50"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"25"
                 })))

        print("\ntest_WeakerOpinions Success!\n")

    def test_SiblingPayloads(self):
        # Test that weaker regular sibling payloads do not affect
        # parameters for stronger dynamic payloads and that
        # stronger sibling payloads do

        print("\ntest_SiblingPayloads start\n")
        
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # /SiblingPayloads overrides the TestPcp_depth and TestPcp_num values that are
        # originally defined in params.sdf.
        payloads = self._GeneratePrimIndexPaths("/SiblingPayloads", 2, 3, 4)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that layers for each dynamic depth were generated and opened.
        dynamicLayerFileName = "sphere.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"2", "TestPcp_num":"3"})))
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"1", "TestPcp_height":"3", "TestPcp_num":"3", "TestPcp_radius":"1.5"
                 })))

        print("\ntest_SiblingPayloads Success!\n")

    def test_AncestralPayloads(self):
        print("\ntest_AncestralPayloads start\n")
        
        # Test that loading a dynamic payload when composing ancestral
        # opinions picks up the right arguments.

        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Compute the prim index for /SubrootReference, which references
        # /RootCone/Xform__3_2. When composing that prim index to incorporate
        # ancestral opinions, Pcp should ignore the parameters authored on
        # /SubrootReference because they do not map to the payload on
        # /RootCone. If those parameters were *not* ignored, the prim being
        # referenced would not be found leading to incorrect unresolved prim
        # path errors.
        pi, err = cache.ComputePrimIndex("/SubrootReference")
        self.assertFalse(err)
        self.assertTrue(
            (Sdf.Layer.Find("cone.testpcpdynamic", 
                            args={ "TestPcp_depth" : "4",
                                   "TestPcp_num" : "3",
                                   "TestPcp_radius" : "50" })
             .GetPrimAtPath("/Root/Xform__3_2"))
            in pi.primStack)
        
        print("\ntest_AncestralPayloads Success!\n")

    def test_PayloadInVariant(self):
        # Test that payloads can pick up weaker opinions from parent siblings
        print("\ntest_PayloadInVariant start\n")
        
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        payloads = self._GeneratePrimIndexPaths("/PayloadInVariant", 4, 4, 85)
        cache.RequestPayloads(payloads, [])

        self._ComputeAndVerifyDynamicPayloads(cache, payloads, 
             ["TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"])
        
        # Verify that the layer for the top dynamic depth was generated and opened.
        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth":"4", "TestPcp_num":"4", "TestPcp_radius" : "50"})))

        print("\ntest_PayloadInVariant Success!\n")

    def test_AncestralPayloads2(self):
        # Similar to test_AncestralPayloads but adds a non-internal reference
        # arc to further exercise path translation logic during argument
        # composition.

        print("\ntest_AncestralPayloads2 start\n")

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString("""
        #sdf 1.4.32

        def "Root" (
            TestPcp_depth = 1
            TestPcp_num = 1
            references = @./root.sdf@</SubrootReference>
        )
        {
            int TestPcp_depth = 1
            int TestPcp_num = 1
        }
        """.strip())

        primSpec = Sdf.CreatePrimInLayer(rootLayer, "/Root")
        primSpec.referenceList.explicitItems = [ 
            Sdf.Reference("root.sdf", "/SubrootReference") 
        ]
        cache = self._CreatePcpCache(rootLayer)

        # Compute the prim index for /Root. We should not see any composition
        # errors for the same reasons mentioned in test_AncestralPayloads.
        pi, err = cache.ComputePrimIndex("/Root")
        self.assertFalse(err)
        self.assertTrue(
            (Sdf.Layer.Find("cone.testpcpdynamic", 
                            args={ "TestPcp_depth" : "4",
                                   "TestPcp_num" : "3",
                                   "TestPcp_radius" : "50" })
             .GetPrimAtPath("/Root/Xform__3_2"))
            in pi.primStack)
        
        print("\ntest_AncestralPayloads2 Success!\n")

    def test_AncestralPayloads3(self):
        # Test that evaluating ancestral payloads at the end works

        print("\ntest_AncestralPayloads3 start\n")

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootCone
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        cache.RequestPayloads(["/World/Sets/MySet"],[])

        pi, err = cache.ComputePrimIndex("/World/Sets/MySet/Group1/Subgroup/Relocated")
        self.assertFalse(err)

        dynamicLayerFileName = "cone.testpcpdynamic"
        self.assertTrue(Sdf.Layer.Find(Sdf.Layer.CreateIdentifier(
                dynamicLayerFileName,
                {"TestPcp_depth": "4",  "TestPcp_num":"4", 
                 "TestPcp_radius":"50"})))
                                        
        self.assertTrue(
            Sdf.Layer.Find("cone.testpcpdynamic", 
                            args={ "TestPcp_depth" : "4",
                                   "TestPcp_num" : "4",
                                   "TestPcp_radius" : "50" })
             .GetPrimAtPath("/Root/Xform__3_3"))
        
        print("\ntest_AncestralPayloads3 Success!\n")

    def test_AncestralPayloadsAndVariants(self):
        # Test that loading a dynamic payload when composing ancestral
        # opinions within variants picks up the right arguments.

        print("\ntest_AncestralPayloadsAndVariants start\n")

        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer)

        # Compute the prim index for /SubrootReferenceAndVariant, which
        # references /Variant/Xform__4_3. When composing that prim index to
        # incorporate ancestral opinions, Pcp should ignore the parameters
        # authored on /SubrootReferenceAndVariant because they do not map to the
        # payload on /Variant. If those parameters were *not* ignored, the prim
        # being referenced would not be found leading to incorrect unresolved
        # prim path errors.
        pi, err = cache.ComputePrimIndex("/SubrootReferenceAndVariant")
        self.assertFalse(err)
        self.assertTrue(
            (Sdf.Layer.Find("cone.testpcpdynamic", 
                            args={ "TestPcp_depth" : "5",
                                   "TestPcp_num" : "4",
                                   "TestPcp_radius" : "50" })
             .GetPrimAtPath("/Root/Xform__4_3"))
            in pi.primStack)
        
        print("\ntest_AncestralPayloadsAndVariants Success!\n")

    def test_Changes(self):
        # Change processing behavior can be different for Pcp caches in USD mode
        # vs not (especially in regards to property change processing). Run the
        # test with caches in both modes to make sure we get same behavior.
        self._TestChangesImpl(True)
        self._TestChangesImpl(False)

    def _TestChangesImpl(self, cacheInUsdMode):
        print("\ntest_Changes (cacheInUsdMode={}) Start\n".format(
                cacheInUsdMode))

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootSphere
        # and /RootMulti as well.
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer, usd=cacheInUsdMode)

        # Payloads prims for /RootSphere - depth = 4, num = 3 : produces 40 
        # payloads
        payloads = self._GeneratePrimIndexPaths("/RootSphere", 4, 3, 40)
        cache.RequestPayloads(payloads,[])

        def _HasAnyFieldOrAttrDependencies():
            if USE_ATTRS:
                return cache.HasAnyDynamicFileFormatArgumentAttributeDependencies()
            else:
                return cache.HasAnyDynamicFileFormatArgumentFieldDependencies()

        def _IsPossibleRelevantFieldOrAttrName(name):
            if USE_ATTRS:
                return cache.IsPossibleDynamicFileFormatArgumentAttribute(name)
            else:
                return cache.IsPossibleDynamicFileFormatArgumentField(name)

        # The field or attributes that expect to be pulled on for each computed
        # payload prim index in most of the following test cases.
        expectedRelevantFieldOrAttrNames = [
            "TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"]

        # Verify that relevant field or attributes are not in the possible 
        # arguments list for the cache yet before the prim indices that use them
        # are computed.
        self.assertFalse(_HasAnyFieldOrAttrDependencies())
        for field in expectedRelevantFieldOrAttrNames:
            self.assertFalse(_IsPossibleRelevantFieldOrAttrName(field))
        # Verify the same for irrelevant fields/attributes.
        self.assertFalse(_IsPossibleRelevantFieldOrAttrName("documentation"))
        self.assertFalse(_IsPossibleRelevantFieldOrAttrName("TestPcp_argDict"))

        # Compute and verify the prim indices for the dynamic payloads.
        self._ComputeAndVerifyDynamicPayloads(
            cache, payloads, expectedRelevantFieldOrAttrNames)

        # Verify that the relevant fields or attributes are now possible dynamic
        # arguments.
        self.assertTrue(_HasAnyFieldOrAttrDependencies())
        for field in expectedRelevantFieldOrAttrNames:
            self.assertTrue(_IsPossibleRelevantFieldOrAttrName(field))
        # Verify that irrelevant fields/attributes are never possible dynamic 
        # arguments.
        self.assertFalse(_IsPossibleRelevantFieldOrAttrName("documentation"))
        self.assertFalse(_IsPossibleRelevantFieldOrAttrName("TestPcp_argDict"))

        # Test that authoring a new value for an irrelevant field/attribute like 
        # documentation does not cause any significant changes.
        self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootSphere'),
                    'documentation', 'update documentation', 
                    [])
        # Verify that all the cached prim indices still exist
        self._VerifyFoundDynamicPayloads(cache, payloads)

        # Test that authoring a new value for an relevant field or attribute on
        # the root prim like height does cause significant changes.
        self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootSphere'),
                    'TestPcp_height', 2.0, 
                    ['/RootSphere'])
        # Verify that all the cached dynamic prim indices were invalidated
        self._VerifyNotFoundDynamicPayloads(cache, payloads)
        # Verify that the invalidation of ALL dynamic prim indices
        # removed the relevant fields/attributes from possible dynamic arguments.
        self.assertFalse(_HasAnyFieldOrAttrDependencies())
        for field in expectedRelevantFieldOrAttrNames:
            self.assertFalse(_IsPossibleRelevantFieldOrAttrName(field))

        # Recompute the dynamic payload prim indices.
        self._ComputeAndVerifyDynamicPayloads(
            cache, payloads, expectedRelevantFieldOrAttrNames)

        # Verify that relevant fields/attributes are possible dynamic arguments 
        # again.
        self.assertTrue(_HasAnyFieldOrAttrDependencies())
        for field in expectedRelevantFieldOrAttrNames:
            self.assertTrue(_IsPossibleRelevantFieldOrAttrName(field))

        # Assert that we can find the params layer that was referenced by in by 
        # /RootSphere.
        self.assertTrue(Sdf.Layer.Find("params.sdf"))
        # FindOrOpen the params layer so that we still have an open reference
        # to it when we change it. Otherwise the layer might get deleted when
        # the prim indexes referencing it are invalidated and we could lose the
        # changes.
        paramsLayer = Sdf.Layer.FindOrOpen("params.sdf")
        # Verify that changing num on the referenced prim causes a significant
        # change to /RootSphere as it is used to compose the dynamic arguments
        self._TestChangeValue(cache, paramsLayer.GetPrimAtPath('/Params'),
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
        self._ComputeAndVerifyDynamicPayloads(
            cache, newPayloads, expectedRelevantFieldOrAttrNames)

        # Test that authoring a new value for a possible dynamic argument
        # that is determined to not be relevant by the file format's conditions
        # doesn't cause a significant change.
        #
        # To set this up first author a change to root prim's depth, setting it
        # to zero. This itself is a significant change to the root prim and 
        # invalidates all our prim indices.
        self._TestChangeValue(cache, paramsLayer.GetPrimAtPath('/Params'),
                    'TestPcp_depth', 0, 
                    ['/RootSphere'])
        self._VerifyNotFoundDynamicPayloads(cache, newPayloads)
        # Build the only dynamic payload for /RootSphere. No recursion occurs
        # because the depth is zero. Depth is the only relevant field/attribute
        # as the file format doesn't check any other parameters when the depth
        # is 0.
        self._ComputeAndVerifyDynamicPayloads(
            cache, ['/RootSphere'], ['TestPcp_depth'])

        # Now author the depth parameter to be -1. This parameter is relevant
        # but there is no significant change as the file format tells us that
        # a depth change from 0 to -1 has no effect on the file format arguments
        # (they both equate to a depth of zero).
        self._TestChangeValue(cache, paramsLayer.GetPrimAtPath('/Params'),
                    'TestPcp_depth', -1, 
                    [])
        # No significant change so /RootSphere is still valid without 
        # recomputing.
        self._VerifyFoundDynamicPayloads(cache, ['/RootSphere'])

        # Now author the depth parameter back to its original value of 4. The 
        # change from -1 to 4 is singnificant and invalidates /RootSphere.
        self._TestChangeValue(cache, paramsLayer.GetPrimAtPath('/Params'),
                    'TestPcp_depth', 4, 
                    ['/RootSphere'])
        self._VerifyNotFoundDynamicPayloads(cache, newPayloads)
        # Verify that we can compute the same payload prim indices as before we
        # starting playing with the depth.
        self._ComputeAndVerifyDynamicPayloads(
            cache, newPayloads, expectedRelevantFieldOrAttrNames)

        # Test changing a relevant value over a dynamic generated subprim spec.
        subprimSpec = rootLayer.GetPrimAtPath('/RootSphere/Xform__3_0')
        # Note that we defined an over in root.sdf so that this spec would 
        # exist for convenience.
        self.assertTrue(subprimSpec)
        # Change the depth on the subprim. This causes a significant change to 
        # the subprim only.
        self._TestChangeValue(cache, subprimSpec,
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
        self._ComputeAndVerifyDynamicPayloads(
            cache, newSubprimPayloads, expectedRelevantFieldOrAttrNames)

        # Test changing a relevant field/attribute on a dynamically generated 
        # subprim that is not itself dynamic.
        geomSpec = rootLayer.GetPrimAtPath('/RootSphere/geom')
        # Note that we defined an over in root.sdf so that this spec would 
        # exist for convenience.
        self.assertTrue(geomSpec)
        self.assertTrue(cache.FindPrimIndex('/RootSphere/geom'))
        # Set depth on the existing geom spec. Verify that it doesn't cause any
        # significant changes to any prim indices even though it's a possible
        # dynamic argument field/attribute.
        self.assertTrue(_IsPossibleRelevantFieldOrAttrName('TestPcp_depth'))
        self._TestChangeValue(cache, geomSpec,
                    'TestPcp_depth', 4, 
                    [])

        print("test_Changes (cacheInUsdMode={}) Success!\n".format(
                cacheInUsdMode))

    def test_ChangesMultiPayload(self):
        # Change processing behavior can be different for Pcp caches in USD mode
        # vs not (especially in regards to property change processing). Run the
        # test with caches in both modes to make sure we get same behavior.
        self._TestChangesMultiPayloadImpl(True)
        self._TestChangesMultiPayloadImpl(False)

    def _TestChangesMultiPayloadImpl(self, cacheInUsdMode):
        print("\ntest_ChangesMultiPayload (cacheInUsdMode={}) Start\n".format(
                cacheInUsdMode))

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootSphere
        # and /RootMulti as well.
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer, usd=cacheInUsdMode)

        # Test setting parameters for a specific payload when there's more than
        # one payload on the same prim. Our test plugin supports tagging a 
        # payload via "payloadId" in payload's file format arguments. The 
        # metadata and attributes versions of the plugin give us two different
        # ways to target the specific payload with arguments via its payloadId.
        #
        # /RootMulti defines two payloads, each with an asset path that includes
        # a payloadId file format argument. Generate the expected paths that
        # would be generated by each ID'ed payload and request they all be 
        # loaded.
        rootMultiPayloads1 = self._GeneratePrimIndexPaths(
            "/RootMulti", 3, 4, 21, payloadId="Pl1")
        rootMultiPayloads2 = self._GeneratePrimIndexPaths(
            "/RootMulti", 4, 2, 15, payloadId="Pl2")
        cache.RequestPayloads(rootMultiPayloads1 + rootMultiPayloads2, [])

        # The field or attributes that expect to be pulled on for each computed
        # payload prim index.
        expectedRelevantFieldOrAttrNames = [
            "TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"]

        if USE_ATTRS:
            # For the attribute default values case the test code supports
            # targeting the specific payload by using its ID as a namespace 
            # prefix for the parameter attribute. For this example we use 
            # "Pl1:TestPcp_num" which sets the "num" parameter for just the 
            # payload with payloadId=Pl1.

            # Namespaced attributes that will be pulled on for each payload ID.
            expectedPl1RelevantAttrNames = [
                "Pl1:TestPcp_depth", "Pl1:TestPcp_height", 
                "Pl1:TestPcp_num", "Pl1:TestPcp_radius"]
            expectedPl2RelevantAttrNames = [
                "Pl2:TestPcp_depth", "Pl2:TestPcp_height", 
                "Pl2:TestPcp_num", "Pl2:TestPcp_radius"]
            
            # Compute the root prim's prim index. Its relevant dynamic 
            # attributes will be all of them (the general argument attributes 
            # plus the namespaced attributes for both payloads).
            self._ComputeAndVerifyDynamicPayloads(
                cache, ["/RootMulti"], 
                (expectedRelevantFieldOrAttrNames + 
                 expectedPl1RelevantAttrNames + 
                 expectedPl2RelevantAttrNames))

            # Compute the prim indexes for the prims descendants dynamically 
            # generated by each payload. The relevant dynamic attributes will 
            # be the general argument attributes plus the namespaced attributes
            # for ONLY the payload ID that generated them as the payload ID is
            # present in the descendant prim payload asset paths.
            #
            # Note that both rootMultiPayloads1 and rootMultiPayloads2 start 
            # with /RootMulti (which has a different set of relevant attributes
            # tested just above) which is why we skip the first payload in each
            # list here.
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads1[1:], 
                expectedRelevantFieldOrAttrNames + expectedPl1RelevantAttrNames)
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads2[1:], 
                expectedRelevantFieldOrAttrNames + expectedPl2RelevantAttrNames)

            # Make a change to an attribute for a payload ID that doesn't exist.
            # This will not produce a significant change or invalidate any 
            # prim indexes.
            self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootMulti'),
                        'Pl3:TestPcp_num', 1, 
                        [])
            self._VerifyFoundDynamicPayloads(cache, rootMultiPayloads1)
            self._VerifyFoundDynamicPayloads(cache, rootMultiPayloads2)

            # Change "num" for just the "Pl1" payload in root prim. This is a
            # significant change in the root prim. All prims indexes are 
            # invalidated (including the ones produced by "Pl2") because the
            # root is invalidated.
            self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootMulti'),
                        'Pl1:TestPcp_num', 1, 
                        ["/RootMulti"])
            self._VerifyNotFoundDynamicPayloads(cache, rootMultiPayloads1)
            self._VerifyNotFoundDynamicPayloads(cache, rootMultiPayloads2)

            # Verify we can recompute all our same prim indexes again except 
            # with fewer generated for Pl1.
            rootMultiPayloads1 = self._GeneratePrimIndexPaths(
                "/RootMulti", 3, 1, 3, payloadId="Pl1")

            self._ComputeAndVerifyDynamicPayloads(
                cache, ["/RootMulti"], 
                (expectedRelevantFieldOrAttrNames + 
                 expectedPl1RelevantAttrNames + 
                 expectedPl2RelevantAttrNames))
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads1[1:], 
                expectedRelevantFieldOrAttrNames + expectedPl1RelevantAttrNames)
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads2[1:], 
                expectedRelevantFieldOrAttrNames + expectedPl2RelevantAttrNames)

        else:
            # For the metadata field values case the test code supports
            # targeting the specific payload by including a dictionary of the
            # relevant arguments keyed by the payload ID in the "argDict" 
            # metadata field of the prim.

            # Compute the prim indexes for the root prim and all prims 
            # descendants dynamically generated by each payload. The relevant 
            # dynamic fields will now also include "TestPcp_argDict" since
            # all payloads will have a payload ID.
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads1, 
                expectedRelevantFieldOrAttrNames + ["TestPcp_argDict"])
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads2, 
                expectedRelevantFieldOrAttrNames + ["TestPcp_argDict"])

            # Make a change to the argDict dictionary for a payload ID that 
            # doesn't exist, "Pl3". This will not produce a significant change 
            # or invalidate any prim indexes because the 
            # CanFieldChangeAffectFileFormatArguments implementation in our 
            # file format plugin, checks that any argDict changes are relevant
            # to the specific payload ID of the payload layer.
            self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootMulti'),
                        'TestPcp_argDict', {"Pl3": {"TestPcp_num":1}}, 
                        [])
            self._VerifyFoundDynamicPayloads(cache, rootMultiPayloads1)
            self._VerifyFoundDynamicPayloads(cache, rootMultiPayloads2)

            # Via the argDict, change "num" for just the "Pl1" payload in the 
            # root prim. This is a significant change in the root prim. All 
            # prims indexes are  invalidated (including the ones produced by 
            # "Pl2") because the root is invalidated.
            self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/RootMulti'),
                        'TestPcp_argDict', {"Pl1": {"TestPcp_num":1}}, 
                        ["/RootMulti"])
            self._VerifyNotFoundDynamicPayloads(cache, rootMultiPayloads1)
            self._VerifyNotFoundDynamicPayloads(cache, rootMultiPayloads2)
 
            # Verify we can recompute all our same prim indexes again except 
            # with fewer generated for Pl1.
            rootMultiPayloads1 = self._GeneratePrimIndexPaths(
                "/RootMulti", 3, 1, 3, payloadId="Pl1")

            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads1, 
                expectedRelevantFieldOrAttrNames + ["TestPcp_argDict"])
            self._ComputeAndVerifyDynamicPayloads(
                cache, rootMultiPayloads2, 
                expectedRelevantFieldOrAttrNames + ["TestPcp_argDict"])

        print("test_ChangesMultiPayload (cacheInUsdMode={}) Success!\n".format(
                cacheInUsdMode))

    def test_SubrootRefChange(self):
        # Change processing behavior can be different for Pcp caches in USD mode
        # vs not (especially in regards to property change processing). Run the
        # test with caches in both modes to make sure we get same behavior.
        self._TestSubrootRefChangeImpl(True)
        self._TestSubrootRefChangeImpl(False)

    def _TestSubrootRefChangeImpl(self, cacheInUsdMode):
        print("\ntest_SubrootRefChange (cacheInUsdMode={}) Start\n".format(
                cacheInUsdMode))

        def _HasAnyFieldOrAttrDependencies():
            if USE_ATTRS:
                return cache.HasAnyDynamicFileFormatArgumentAttributeDependencies()
            else:
                return cache.HasAnyDynamicFileFormatArgumentFieldDependencies()

        def _IsPossibleRelevantFieldOrAttrName(name):
            if USE_ATTRS:
                return cache.IsPossibleDynamicFileFormatArgumentAttribute(name)
            else:
                return cache.IsPossibleDynamicFileFormatArgumentField(name)

        # Enumerate the names of the relevant field/attribute names for the 
        # payloads in this test case. Start with the basic argument names.
        childRelevantFieldOrAttrNames = [
            "TestPcp_num", "TestPcp_depth", "TestPcp_height", "TestPcp_radius"]
        rootRelevantFieldOrAttrNames = None
        # Since we're dealing with reference to a child prim of /RootMulti which
        # has two payloads with specified payload IDs, we'll have additional
        # arguments related to those payload IDs.
        if USE_ATTRS: 
            # For the attributes case, all the child prims need the attributes 
            # that apply to the payload with ID "Pl1" as the reference is to 
            # a prim child introduced by "Pl1" in /RootMulti
            childRelevantFieldOrAttrNames.extend([
                "Pl1:TestPcp_num", "Pl1:TestPcp_depth", 
                "Pl1:TestPcp_height", "Pl1:TestPcp_radius"])
            # Additionally the root prim, which has the subroot reference, will
            # ancestrally depend on /RootMulti and will additionally have the
            # "Pl2" payload specific attributes be relevant to it as well.
            rootRelevantFieldOrAttrNames = childRelevantFieldOrAttrNames + [
                "Pl2:TestPcp_num", "Pl2:TestPcp_depth", 
                "Pl2:TestPcp_height", "Pl2:TestPcp_radius"]
        else:
            # For the metadata field case, all payload ID specific arguments are
            # contained in the same field, "TestPcp_argDict".
            childRelevantFieldOrAttrNames.append("TestPcp_argDict")
            rootRelevantFieldOrAttrNames = childRelevantFieldOrAttrNames

        # Create a PcpCache for subrootref.sdf. This file contains a single 
        # /Root prim with a subroot reference to a child of /RootMulti in 
        # root.sdf
        rootLayerFile = 'subrootref.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)
        cache = self._CreatePcpCache(rootLayer, usd=cacheInUsdMode)

        # Payloads prims for /Root - depth = 2, num = 4 : produces 5 
        # payloads. Note that the depth is one less than actually defined on 
        # /RootMulti as the reference is to a child of RootMulti.
        payloads = self._GeneratePrimIndexPaths("/Root", 2, 4, 5, payloadId="Pl1")
        rootPayload = ["/Root"]
        childPayloads = payloads[1:]
        cache.RequestPayloads(payloads, [])

        self.assertFalse(_HasAnyFieldOrAttrDependencies())
        for field in rootRelevantFieldOrAttrNames:
            self.assertFalse(_IsPossibleRelevantFieldOrAttrName(field))

        # Compute and verify the prim indices for the dynamic payloads.
        self._ComputeAndVerifyDynamicPayloads(cache, rootPayload, 
            rootRelevantFieldOrAttrNames)
        self._ComputeAndVerifyDynamicPayloads(cache, childPayloads, 
            childRelevantFieldOrAttrNames)

        # Because our primIndices include RootMulti which has payloads with
        # a "payloadId" format argument, "argDict" becomes another possible
        # dynamic argument field.
        self.assertTrue(_HasAnyFieldOrAttrDependencies())
        for field in rootRelevantFieldOrAttrNames:
            self.assertTrue(_IsPossibleRelevantFieldOrAttrName(field))

        # Change height on the root prim. Verify this is a significant change
        # on the Root prim as it references in a subprim that is itself still
        # dynamic
        self._TestChangeValue(cache, rootLayer.GetPrimAtPath('/Root'),
                             'TestPcp_height', 2.0, 
                             ['/Root'])
        # Verify our prim indices were invalidated.
        self._VerifyNotFoundDynamicPayloads(cache, payloads)

        # Recompute the prim indices.
        self._ComputeAndVerifyDynamicPayloads(cache, rootPayload, 
            rootRelevantFieldOrAttrNames)
        self._ComputeAndVerifyDynamicPayloads(cache, childPayloads, 
            childRelevantFieldOrAttrNames)

        # Assert that we can find root.sdf which was referenced in to /Root. It
        # will already be open.
        self.assertTrue(Sdf.Layer.Find("root.sdf"))
        # FindOrOpen the layer so that we still have an open reference
        # to it when we change it. Otherwise the layer might get deleted when
        # the prim indexes referencing it are invalidated and we could lose the
        # changes.
        refLayer = Sdf.Layer.FindOrOpen("root.sdf")

        # Change value of the "num" argument for just the payload with ID "Pl1" 
        # on /RootMulti on the referenced layer. Since /RootMulti is the 
        # parent prim to referenced subprim, this will cause a significant 
        # change to /Root. The change to field/attribute is a significant change
        # to /RootMulti which is significant to all of /RootMulti's children and 
        # therefore significant to any prim referencing one of those children.
        if USE_ATTRS:
            self._TestChangeValue(cache, refLayer.GetPrimAtPath('/RootMulti'),
                                'Pl1:TestPcp_num', 5, 
                                ['/Root'])
        else:
            self._TestChangeValue(cache, refLayer.GetPrimAtPath('/RootMulti'),
                                'TestPcp_argDict', {"Pl1": {"TestPcp_num":5}}, 
                                ['/Root'])

        self._VerifyNotFoundDynamicPayloads(cache, payloads)

        # There will now be an extra dynamic payload available under root.
        # Verify that we can compute all the new prim indices.
        newPayloads = self._GeneratePrimIndexPaths("/Root", 2, 5, 6, payloadId="Pl1")
        rootPayload = ["/Root"]
        childPayloads = newPayloads[1:]
        cache.RequestPayloads(newPayloads, [])
        self._ComputeAndVerifyDynamicPayloads(cache, rootPayload, 
            rootRelevantFieldOrAttrNames)
        self._ComputeAndVerifyDynamicPayloads(cache, childPayloads, 
            childRelevantFieldOrAttrNames)

        # XXX: Todo: Add another case here for making a metadata change in 
        # params.sdf which is referenced by root.sdf. This would be expected
        # to propagate to /Root in subrootref.sdf. But right now there is 
        # another bug related to subroot references which causes the node
        # provided by params.sdf to be culled from the subroot ref tree.

        print ("Computing prim index for " + "/SubrootGeomRef")
        # /SubrootGeomRef directly references a child prim of the dynamic 
        # /RootMulti. Note that we do not have to request payloads on 
        # /SubrootGeomRef nor /RootMulti as ancestral payloads of the target of 
        # a subroot reference are always included.

        # Verify each dynamic payload's prim index is computed 
        # without errors and has dynamic file arguments
        (primIndex, err) = cache.ComputePrimIndex("/SubrootGeomRef")
        self.assertTrue(primIndex.IsValid())
        self.assertFalse(err)
        if USE_ATTRS:
            self._TestChangeValue(cache, refLayer.GetPrimAtPath('/RootMulti'),
                             'Pl1:TestPcp_num', 3, 
                             ['/Root', '/SubrootGeomRef'])
        else:
            self._TestChangeValue(cache, refLayer.GetPrimAtPath('/RootMulti'),
                             'TestPcp_argDict', {"Pl1": {"TestPcp_num":3}}, 
                             ['/Root', '/SubrootGeomRef'])

        print("test_SubrootRefChange (cacheInUsdMode={}) Success\n".format(
                cacheInUsdMode))

    @unittest.skipIf(not USE_ATTRS, 'Test case requires USE_ATTRS == True')
    def test_AttrNamespaceEdits(self):
        # Change processing behavior can be different for Pcp caches in USD mode
        # vs not (especially in regards to property change processing). Run the
        # test with caches in both modes to make sure we get same behavior.
        self._TestAttrNamespaceEditsImpl(True)
        self._TestAttrNamespaceEditsImpl(False)

    def _TestAttrNamespaceEditsImpl(self, cacheInUsdMode):
        print("\ntest_AttrNamespaceEdits (cacheInUsdMode={}) Start\n".format(
                cacheInUsdMode))

        # Create a PcpCache for root.sdf. Has a dynamic root prim /RootSphere
        # and /RootCone as well.
        rootLayerFile = 'root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)
        self.assertTrue(rootLayer)

        cache = self._CreatePcpCache(rootLayer, usd=cacheInUsdMode)

        # Payloads prims for /RootSphere - depth = 4, num = 3 : produces 40 
        # payloads
        payloads = self._GeneratePrimIndexPaths("/RootSphere", 4, 3, 40)
        cache.RequestPayloads(payloads,[])

        # The field or attributes that expect to be pulled on for each computed
        # payload prim index in most of the following test cases.
        expectedRelevantFieldOrAttrNames = [
            "TestPcp_depth", "TestPcp_height", "TestPcp_num", "TestPcp_radius"]

        # Makes an edit and verifies that it is not a significant change to 
        # /RootSphere. Verifies All payload prim indexes are still valid after
        # the edit.
        def _MakeInsignificantEdit(editFunc):
            with Pcp._TestChangeProcessor(cache) as cp:
                editFunc()
                self.assertNotIn("/RootSphere", cp.GetSignificantChanges())

            self._VerifyFoundDynamicPayloads(cache, payloads)

        # Makes an edit and verifies that it is a significant change to 
        # /RootSphere. Verifies All payload prim indexes are invalidated after
        # the edit and then recomputes all payload prim indexes again.
        def _MakeSignificantEdit(editFunc):
            with Pcp._TestChangeProcessor(cache) as cp:
                editFunc()
                self.assertIn("/RootSphere", cp.GetSignificantChanges())

            self._VerifyNotFoundDynamicPayloads(cache, payloads)

            # Recompute our payload prim indexes.
            self._ComputeAndVerifyDynamicPayloads(
                cache, payloads, expectedRelevantFieldOrAttrNames)

        # Performs an attribute rename
        def _RenameAttr(attrSpec, name):
            attrSpec.name = name

        # Performs a attribute reparent
        def _ReparentAttr(srcPath, dstPath):
            edit = Sdf.BatchNamespaceEdit()
            edit.Add(srcPath, dstPath) 
            rootLayer.Apply(edit)

        def _GetHeightAttr():
            return rootLayer.GetAttributeAtPath("/RootSphere.TestPcp_height")

        primSpec = rootLayer.GetPrimAtPath("/RootSphere")
        self.assertTrue(primSpec)

        # Compute and verify the prim indices for the dynamic payloads before
        # starting any edits.
        self._ComputeAndVerifyDynamicPayloads(
            cache, payloads, expectedRelevantFieldOrAttrNames)

        # We're going to do namespace edits with the height attribute. Assert
        # that it does not yet exist on the /RootSphere spec on our root layer.        
        self.assertFalse(_GetHeightAttr())

        # Create the attribute spec. This will not cause a significant change 
        # and will not invalidate the prim indexes as the new spec is added
        # without a default value.
        _MakeInsignificantEdit(
            lambda: Sdf.AttributeSpec(
                primSpec, "TestPcp_height", Sdf.ValueTypeNames.Double))

        # Verify the attribute exists.
        attrSpec = _GetHeightAttr()
        self.assertTrue(attrSpec)

        # Rename the "height" attribute to another attribute name that does not
        # impact the dynamic file format. This IS a significant change and will
        # invalidate prim indexes as it counts as removing "height" and change 
        # processing currently has no way of determining that "height" did not 
        # have a default value set.
        _MakeSignificantEdit(
            lambda: _RenameAttr(attrSpec, "notARelevantAttr"))
        self.assertFalse(_GetHeightAttr())

        # Rename the attribute again to another attribute name that does not
        # impact the dynamic file format. This is NOT a significant change and 
        # won't invalidate prim indexes because neither the old or new attribute
        # affects the dynamic file format.
        _MakeInsignificantEdit(
            lambda: _RenameAttr(attrSpec, "stillNotARelevantAttr"))
        self.assertFalse(_GetHeightAttr())

        # Rename the irrelevant attribute back to "height". This is still not a 
        # significant change and will not invalidate prim indexes as the 
        # "height" attribute still doesn't have a default value after rename.
        _MakeInsignificantEdit(
            lambda: _RenameAttr(attrSpec, "TestPcp_height"))
        self.assertTrue(_GetHeightAttr())

        # Reparent the height attribute to another prim (reparent is change 
        # managed differently than rename). The height attribute is treated as 
        # removed. Unlike in the RemoveProperty case below, this IS a 
        # significant change because moving a spec doesn't account for whether
        # a spec has only required fields like remove does.
        _MakeSignificantEdit(
            lambda: _ReparentAttr("/RootSphere.TestPcp_height", 
                                  "/RootCone.TestPcp_height"))
        self.assertFalse(_GetHeightAttr())

        # Reparent the height attribute back to our prim. The height attribute
        # is treated as added. This is NOT a significant change because the 
        # attribute spec does not have a default value.
        _MakeInsignificantEdit(
            lambda: _ReparentAttr("/RootCone.TestPcp_height", 
                                  "/RootSphere.TestPcp_height"))
        self.assertTrue(_GetHeightAttr())

        # Remove the height attribute altogether. This is not a significant 
        # change since the removed property only has required fields and change
        # processing knows it could not have had a default value.
        _MakeInsignificantEdit(
            lambda: primSpec.RemoveProperty(attrSpec))
        self.assertFalse(_GetHeightAttr())

        # Do the same set of tests again, but this time set a default value 
        # after adding the attribute.

        # Create the attribute spec and set the default value. This will cause a
        # significant change when the default value is set.
        def _CreateAttrWithDefault():
            with Sdf.ChangeBlock():
                attr = Sdf.AttributeSpec(
                    primSpec, "TestPcp_height", Sdf.ValueTypeNames.Double)
                attr.SetInfo("default", 1.0)

        _MakeSignificantEdit(_CreateAttrWithDefault)

        # Verify the attribute exists.
        attrSpec = _GetHeightAttr()
        self.assertTrue(attrSpec)

        # Rename the "height" attribute to another attribute name that does not
        # impact the dynamic file format. This IS a significant change and will
        # invalidate prim indexes as it counts as removing "height" and change 
        # processing currently has no way of determining that "height" did not 
        # have a default value set.
        _MakeSignificantEdit(
            lambda: _RenameAttr(attrSpec, "notARelevantAttr"))
        self.assertFalse(_GetHeightAttr())

        # Rename the attribute again to another attribute name that does not
        # impact the dynamic file format. This is NOT a significant change and 
        # won't invalidate prim indexes because neither the old or new attribute
        # affects the dynamic file format.
        _MakeInsignificantEdit(
            lambda: _RenameAttr(attrSpec, "stillNotARelevantAttr"))
        self.assertFalse(_GetHeightAttr())

        # Rename the irrelevant attribute back to "height". This time it IS a 
        # significant change because the attribute has a default value.
        _MakeSignificantEdit(
            lambda: _RenameAttr(attrSpec, "TestPcp_height"))
        self.assertTrue(_GetHeightAttr())

        # Reparent the height attribute to another prim (reparent is change 
        # managed differently than rename). The height attribute is treated as 
        # removed. This is a signifcant change just like RemoveProperty below
        # will be.
        _MakeSignificantEdit(
            lambda: _ReparentAttr("/RootSphere.TestPcp_height", 
                                  "/RootCone.TestPcp_height"))
        self.assertFalse(_GetHeightAttr())

        # Reparent the height attribute back to our prim. The height attribute
        # is treated as added. This is a significant change because the 
        # attribute spec has a default value.
        _MakeSignificantEdit(
            lambda: _ReparentAttr("/RootCone.TestPcp_height", 
                                  "/RootSphere.TestPcp_height"))
        self.assertTrue(_GetHeightAttr())

        # Remove the height attribute altogether. This is now now also a 
        # significant change since the removed property has fields besides the 
        # required fields and so the default value could have changed.
        _MakeSignificantEdit(
            lambda: primSpec.RemoveProperty(attrSpec))
        self.assertFalse(_GetHeightAttr())

        print("test_AttrNamespaceEdits (cacheInUsdMode={}) Success\n".format(
                cacheInUsdMode))

if __name__ == "__main__":
    unittest.main()
