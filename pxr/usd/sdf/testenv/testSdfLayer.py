#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, platform, itertools, sys, unittest

# Initialize Ar to use Sdf_TestResolver unless a different implementation
# is specified via the TEST_SDF_LAYER_RESOLVER to allow testing with other
# filesystem-based resolvers.
sdfTestResolver = "Sdf_TestResolver"
preferredResolver = os.environ.get(
    "TEST_SDF_LAYER_RESOLVER", sdfTestResolver)

from pxr import Ar
Ar.SetPreferredResolver(preferredResolver)

# Import other modules from pxr after Ar to ensure we don't pull on Ar
# before the preferred resolver has been specified.
from pxr import Sdf, Tf, Plug

class TestSdfLayer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register dso plugins.
        testRoot = os.path.join(os.path.dirname(__file__), 'SdfPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'
        Plug.Registry().RegisterPlugins(testPluginsDsoSearch)

    def test_IdentifierWithArgs(self):
        paths = [
            ("foo.sdf", 
             "foo.sdf", 
             {}),
            ("foo.sdf1!@#$%^*()-_=+[{]}|;:',<.>", 
             "foo.sdf1!@#$%^*()-_=+[{]}|;:',<.>", 
             {}),
            ("foo.sdf:SDF_FORMAT_ARGS:a=b&c=d", 
             "foo.sdf",
             {"a":"b", "c":"d"}),
            ("foo.sdf?otherargs&evenmoreargs:SDF_FORMAT_ARGS:a=b&c=d", 
             "foo.sdf?otherargs&evenmoreargs",
             {"a":"b", "c":"d"}),
        ]
        
        for (identifier, path, args) in paths:
            splitPath, splitArgs = Sdf.Layer.SplitIdentifier(identifier)
            self.assertEqual(path, splitPath)
            self.assertEqual(args, splitArgs)

            joinedIdentifier = Sdf.Layer.CreateIdentifier(splitPath, splitArgs)
            self.assertEqual(identifier, joinedIdentifier)

    def test_SetIdentifier(self):
        layer = Sdf.Layer.CreateAnonymous()

        # Can't change a layer's identifier if another layer with the same
        # identifier and resolved path exists.
        existingLayer = Sdf.Layer.CreateNew("testSetIdentifier.sdf")
        with self.assertRaises(Tf.ErrorException):
            layer.identifier = existingLayer.identifier

        # Can't change a layer's identifier to the empty string.
        with self.assertRaises(Tf.ErrorException):
            layer.identifier = ""

        # Can't change a layer's identifier to an anonymous layer identifier.
        with self.assertRaises(Tf.ErrorException):
            layer.identifier = "anon:testing"

    def test_SetIdentifierWithArgs(self):
        layer = Sdf.Layer.CreateAnonymous()
        layer.Export("testSetIdentifierWithArgs.sdf")

        layer = Sdf.Layer.FindOrOpen(
            "testSetIdentifierWithArgs.sdf", args={"a":"b"})
        self.assertTrue(layer)

        # Can't change arguments when setting a new identifier
        with self.assertRaises(Tf.ErrorException):
            layer.identifier = Sdf.Layer.CreateIdentifier(
                "testSetIdentifierWithArgs.sdf", {"a":"c"})

        with self.assertRaises(Tf.ErrorException):
            layer.identifier = Sdf.Layer.CreateIdentifier(
                "testSetIdentifierWithArgs.sdf", {"b":"d"})

        # Can change the identifier if we leave the args the same.
        layer.identifier = Sdf.Layer.CreateIdentifier(
            "testSetIdentifierWithArgsNew.sdf", {"a":"b"})

    def test_SaveWithArgs(self):
        Sdf.Layer.CreateAnonymous().Export("testSaveWithArgs.sdf")

        # Verify that a layer opened with file format arguments can be saved.
        layer = Sdf.Layer.FindOrOpen("testSaveWithArgs.sdf", args={"a":"b"})
        self.assertTrue("a=b" in layer.identifier)
        self.assertTrue("a" in layer.GetFileFormatArguments())

        layer.documentation = "test_SaveWithArgs"
        self.assertTrue(layer.Save())

    def test_OpenWithInvalidFormat(self):
        l = Sdf.Layer.FindOrOpen('foo.invalid')
        self.assertIsNone(l)

        # XXX: 
        # OpenAsAnonymous raises a coding error when it cannot determine a
        # file format. This is inconsistent with FindOrOpen and is purely
        # historical.
        with self.assertRaises(Tf.ErrorException):
            l = Sdf.Layer.OpenAsAnonymous('foo.invalid')

    def test_OpenWithThrownException(self):
        fileFormat = Sdf.FileFormat.FindByExtension(".testexception")
        self.assertTrue(fileFormat)

        layer = Sdf.Layer.CreateAnonymous()
        layer.Export("test.testexception")

        with self.assertRaises(Tf.ErrorException):
            l = Sdf.Layer.FindOrOpen('test.testexception')
            self.assertIsNone(l)
        
        with self.assertRaises(Tf.ErrorException):
            l = Sdf.Layer.OpenAsAnonymous('test.testexception')
            self.assertIsNone(l)

    def test_FindWithAnonymousIdentifier(self):
        def _TestWithTag(tag):
            layer = Sdf.Layer.CreateAnonymous(tag)
            layerId = layer.identifier
            self.assertEqual(Sdf.Layer.Find(layerId), layer)

            del layer
            self.assertNotIn(
                layerId, [l.identifier for l in Sdf.Layer.GetLoadedLayers()])

            self.assertFalse(Sdf.Layer.Find(layerId))

        _TestWithTag("")
        _TestWithTag(".sdf")
        _TestWithTag(".invalid")
        _TestWithTag("test")
        _TestWithTag("test.invalid")
        _TestWithTag("test.sdf")

    def test_FindOrOpenWithAnonymousIdentifier(self):
        def _TestWithTag(tag):
            layer = Sdf.Layer.CreateAnonymous(tag)
            layerId = layer.identifier
            self.assertEqual(Sdf.Layer.FindOrOpen(layerId), layer)

            del layer
            self.assertFalse(
                [l for l in Sdf.Layer.GetLoadedLayers() 
                 if l.identifier == layerId])

            self.assertFalse(Sdf.Layer.FindOrOpen(layerId))

        _TestWithTag("")
        _TestWithTag(".sdf")
        _TestWithTag(".invalid")
        _TestWithTag("test")
        _TestWithTag("test.invalid")
        _TestWithTag("test.sdf")

    def test_AnonymousIdentifiersDisplayName(self):
        # Ensure anonymous identifiers work as expected

        ident = 'anonIdent.sdf'
        l = Sdf.Layer.CreateAnonymous(ident)
        self.assertEqual(l.GetDisplayName(), ident)

        identWithColons = 'anonIdent:afterColon.sdf'
        l = Sdf.Layer.CreateAnonymous(identWithColons)
        self.assertEqual(l.GetDisplayName(), identWithColons)

        l = Sdf.Layer.CreateAnonymous()
        self.assertEqual(l.GetDisplayName(), '')

    def test_UpdateAssetInfo(self):
        # Test that calling UpdateAssetInfo on a layer whose resolved
        # path hasn't changed doesn't cause notification to be sent.
        layer = Sdf.Layer.CreateNew('TestUpdateAssetInfo.sdf')
        self.assertTrue(layer)

        class _Listener:
            def __init__(self):
                self._listener = Tf.Notice.RegisterGlobally(
                    Sdf.Notice.LayersDidChange, self._HandleNotice)
                self.receivedNotice = False

            def _HandleNotice(self, notice, sender):
                self.receivedNotice = True

        listener = _Listener()

        oldResolvedPath = layer.resolvedPath
        layer.UpdateAssetInfo()
        newResolvedPath = layer.resolvedPath

        self.assertEqual(oldResolvedPath, newResolvedPath)
        self.assertFalse(listener.receivedNotice)

    def test_UpdateCompositionAssetDependency(self):
        srcLayer = Sdf.Layer.CreateAnonymous()
        srcLayerStr = '''\
#sdf 1.4.32
(
    subLayers = [
        @sublayer_1.sdf@,
        @sublayer_2.sdf@
    ]
)

def "Root" (
    payload = @payload_1.sdf@</Payload>
    references = [
        @ref_1.sdf@</Ref>,
        @ref_2.sdf@</Ref2>
    ]
)
{
    def "Child" (
        payload = @payload_1.sdf@</Payload>
        references = [
            @ref_1.sdf@</Ref>,
            @ref_2.sdf@</Ref2>
        ]
    )
    {
    }

    variantSet "v" = {
        "x" (
            payload = [
                @payload_1.sdf@</Payload>, 
                @payload_2.sdf@</Payload2>
            ]
            references = [
                @ref_1.sdf@</Ref>,
                @ref_2.sdf@</Ref2>
            ]
        ) {
            def "ChildInVariant" (
                payload = [
                    @payload_1.sdf@</Payload>, 
                    @payload_2.sdf@</Payload2>
                ]
                references = [
                    @ref_1.sdf@</Ref>,
                    @ref_2.sdf@</Ref2>
                ]
            )
            {
            }
        }
    }
}
        '''
        srcLayer.ImportFromString(srcLayerStr)

        # Calling UpdateCompositionAssetDependency with an empty old layer path
        # is not allowed.
        origLayer = srcLayer.ExportToString()
        self.assertFalse(srcLayer.UpdateCompositionAssetDependency("", ""))
        self.assertEqual(origLayer, srcLayer.ExportToString())

        # Calling UpdateCompositionAssetDependency with an asset path that does
        # not exist should result in no changes to the layer.
        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "nonexistent.sdf", "foo.sdf"))
        self.assertEqual(origLayer, srcLayer.ExportToString())

        # Test renaming / removing sublayers.
        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "sublayer_1.sdf", "new_sublayer_1.sdf"))
        self.assertEqual(
            srcLayer.subLayerPaths, ["new_sublayer_1.sdf", "sublayer_2.sdf"])

        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "sublayer_2.sdf", ""))
        self.assertEqual(srcLayer.subLayerPaths, ["new_sublayer_1.sdf"])

        # Test renaming / removing payloads.
        primsWithReferences = [
            srcLayer.GetPrimAtPath(p) for p in
            ["/Root", "/Root/Child", "/Root{v=x}", "/Root{v=x}ChildInVariant"]
        ]
        primsWithSinglePayload = [
            srcLayer.GetPrimAtPath(p) for p in
            ["/Root", "/Root/Child"]
        ]
        primsWithPayloadList = [
            srcLayer.GetPrimAtPath(p) for p in
            ["/Root{v=x}", "/Root{v=x}ChildInVariant"]
        ]

        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "payload_1.sdf", "new_payload_1.sdf"))
        for prim in primsWithSinglePayload:
            self.assertEqual(
                prim.payloadList.explicitItems, 
                [Sdf.Payload("new_payload_1.sdf", "/Payload")],
                "Unexpected payloads {0} at {1}".format(prim.payloadList, prim.path))
        for prim in primsWithPayloadList:
            self.assertEqual(
                prim.payloadList.explicitItems, 
                [Sdf.Payload("new_payload_1.sdf", "/Payload"),
                 Sdf.Payload("payload_2.sdf", "/Payload2")],
                "Unexpected payloads {0} at {1}".format(prim.payloadList, prim.path))

        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "new_payload_1.sdf", ""))
        for prim in primsWithSinglePayload:
            self.assertEqual(
                prim.payloadList.explicitItems, [],
                "Unexpected payloads {0} at {1}".format(prim.payloadList, prim.path))
        for prim in primsWithPayloadList:
            self.assertEqual(
                prim.payloadList.explicitItems, 
                [Sdf.Payload("payload_2.sdf", "/Payload2")],
                "Unexpected payloads {0} at {1}".format(prim.payloadList, prim.path))

        # Test renaming / removing references.
        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "ref_1.sdf", "new_ref_1.sdf"))
        for prim in primsWithReferences:
            self.assertEqual(
                prim.referenceList.explicitItems,
                [Sdf.Reference("new_ref_1.sdf", "/Ref"),
                 Sdf.Reference("ref_2.sdf", "/Ref2")],
                "Unexpected references {0} at {1}"
                .format(prim.referenceList, prim.path))

        self.assertTrue(srcLayer.UpdateCompositionAssetDependency(
            "ref_2.sdf", ""))
        for prim in primsWithReferences:
            self.assertEqual(
                prim.referenceList.explicitItems,
                [Sdf.Reference("new_ref_1.sdf", "/Ref")],
                "Unexpected references {0} at {1}"
                .format(prim.referenceList, prim.path))

    def test_Traverse(self):
        ''' Tests Sdf.Layer.Traverse '''

        srcLayer = Sdf.Layer.CreateAnonymous()
        srcLayerStr = '''\
#sdf 1.4.32

def "Root"
{
    double myAttr = 0
    rel myRel

    def "Child"
    {
    }

    variantSet "v" = {
        "x" {
            def "ChildInVariant"
            {
                double myAttr
            }
        }
    }
}
        '''
        srcLayer.ImportFromString(srcLayerStr)

        primPaths = set()
        propPaths = set()
        def visit(path):
            if path.IsPrimPath():
                primPaths.add(path)
            elif path.IsPropertyPath():
                propPaths.add(path)

        srcLayer.Traverse(Sdf.Path('/'), visit)

        self.assertIn(Sdf.Path('/Root'), primPaths)
        self.assertIn(Sdf.Path('/Root/Child'), primPaths)
        self.assertIn(Sdf.Path('/Root{v=x}ChildInVariant'), primPaths)

        self.assertIn(Sdf.Path('/Root.myAttr'), propPaths)
        self.assertIn(Sdf.Path('/Root{v=x}ChildInVariant.myAttr'), propPaths)
        self.assertIn(Sdf.Path('/Root.myRel'), propPaths)

    def test_ExpiredLayerRepr(self):
        l = Sdf.Layer.CreateAnonymous()
        self.assertTrue(l)
        Sdf._TestTakeOwnership(l)
        self.assertFalse(l)
        self.assertTrue(l.expired)
        self.assertTrue(len(repr(l)))

    def test_Import(self):
        # Create a test layer on disk to import and then verify that
        # we can import its contents into another layer.
        newLayer = Sdf.Layer.CreateNew('TestLayerImport.sdf')
        Sdf.PrimSpec(newLayer, 'Root', Sdf.SpecifierDef)
        self.assertTrue(newLayer.Save())

        anonLayer = Sdf.Layer.CreateAnonymous('TestLayerImport')
        self.assertTrue(anonLayer)
        self.assertTrue(anonLayer.Import(newLayer.identifier))

        self.assertEqual(newLayer.ExportToString(), anonLayer.ExportToString())

        # Test error cases. These should not affect the contents of the
        # destination layer.
        self.assertFalse(anonLayer.Import(''))
        self.assertEqual(newLayer.ExportToString(), anonLayer.ExportToString())

        self.assertFalse(anonLayer.Import('bogus.sdf'))
        self.assertEqual(newLayer.ExportToString(), anonLayer.ExportToString())

    def test_LayersWithEquivalentPaths(self):
        # Test that FindOrOpen and Find return the same layer when
        # given different paths that should point to the same location.
        if not os.path.isdir("eqPaths"):
            os.makedirs("eqPaths")

        # These paths should all be equivalent.
        testPaths = [
            "",
            os.getcwd(),
            "eqPaths/../.",
            os.path.join(os.getcwd(), "eqPaths/../.")
        ]

        # Iterate over all permutations of these paths calling FindOrOpen
        # or Find on each one and verifying that they all return the same
        # layer.
        i = 0
        for paths in itertools.permutations(testPaths):
            i += 1
            testLayerName = "FindOrOpenEqPaths_{}.sdf".format(i)
            testLayer = Sdf.Layer.CreateAnonymous()
            Sdf.CreatePrimInLayer(testLayer, "/TestLayer_{}".format(i))
            self.assertTrue(testLayer.Export(testLayerName))

            paths = [os.path.join(p, testLayerName) for p in paths]
            firstLayer = Sdf.Layer.FindOrOpen(paths[0])
            self.assertTrue(
                firstLayer,
                "Unable to open {} (from {})".format(paths[0], paths))

            for p in paths:
                testLayer = Sdf.Layer.FindOrOpen(p)
                self.assertTrue(
                    testLayer, "Unable to open {} (from {})".format(p, paths))
                self.assertEqual(
                    firstLayer, testLayer,
                    "Layer opened with {} not the same as layer opened "
                    "with {}".format(p, paths[0]))

                testLayer = Sdf.Layer.Find(p)
                self.assertTrue(
                    testLayer, "Unable to find {} (from {})".format(p, paths))
                self.assertEqual(
                    firstLayer, testLayer,
                    "Layer found with {} not the same as layer opened "
                    "with {}".format(p, paths[0]))

    def test_FindRelativeToLayer(self):
        with self.assertRaises(Tf.ErrorException):
            Sdf.Layer.FindRelativeToLayer(None, 'foo.sdf')

        anchorLayer = Sdf.Layer.CreateNew('TestFindRelativeToLayer.sdf')
        self.assertFalse(Sdf.Layer.FindRelativeToLayer(anchorLayer, ''))

        def _TestWithRelativePath(relLayerPath):
            absLayerPath = os.path.abspath(relLayerPath)
            if os.path.isfile(absLayerPath):
                os.remove(absLayerPath)

            # FindRelativeToLayer and FindOrOpenRelativeToLayer should
            # return an invalid layer since this layer hasn't been opened yet.
            self.assertFalse(Sdf.Layer.FindRelativeToLayer(
                anchorLayer, relLayerPath))
            self.assertFalse(Sdf.Layer.FindOrOpenRelativeToLayer(
                anchorLayer, relLayerPath))

            # Create new layer at the given path. This will also open the
            # layer in the layer registry.
            relLayer = Sdf.Layer.CreateNew(absLayerPath)

            # FindRelativeToLayer and FindOrOpenRelativeToLayer should
            # now find the new layer.
            self.assertTrue(Sdf.Layer.FindRelativeToLayer(
                anchorLayer, relLayerPath))
            self.assertTrue(Sdf.Layer.FindOrOpenRelativeToLayer(
                anchorLayer, relLayerPath))

        _TestWithRelativePath('FindRelativeLayer.sdf')
        _TestWithRelativePath('subdir/FindRelativeLayer.sdf')

    def test_FindOrOpenRelativeToLayer(self):
        with self.assertRaises(Tf.ErrorException):
            Sdf.Layer.FindOrOpenRelativeToLayer(None, 'foo.sdf')

        anchorLayer = Sdf.Layer.CreateNew('TestFindOrOpenRelativeToLayer.sdf')
        self.assertFalse(Sdf.Layer.FindOrOpenRelativeToLayer(anchorLayer, ''))

        def _TestWithRelativePath(relLayerPath):
            absLayerPath = os.path.abspath(relLayerPath)
            if os.path.isfile(absLayerPath):
                os.remove(absLayerPath)

            # FindRelativeToLayer and FindOrOpenRelativeToLayer should
            # return an invalid layer since this layer hasn't been opened yet.
            self.assertFalse(Sdf.Layer.FindRelativeToLayer(
                anchorLayer, relLayerPath))
            self.assertFalse(Sdf.Layer.FindOrOpenRelativeToLayer(
                anchorLayer, relLayerPath))

            # Create new layer at the given path. We use Export to create a
            # new layer here as it does not open the new layer immediately.
            self.assertTrue(anchorLayer.Export(absLayerPath))

            # FindRelativeToLayer should fail to find the new layer since it
            # hasn't been opened, but FindOrOpenRelativeToLayer should succeed.
            self.assertFalse(Sdf.Layer.FindRelativeToLayer(
                anchorLayer, relLayerPath))
            self.assertTrue(Sdf.Layer.FindOrOpenRelativeToLayer(
                anchorLayer, relLayerPath))

        _TestWithRelativePath('FindOrOpenRelativeLayer.sdf')
        _TestWithRelativePath('subdir/FindOrOpenRelativeLayer.sdf')

    @unittest.skipIf(preferredResolver != sdfTestResolver,
                     "Test uses search-path functionality specific to "
                     "the default test resolver")
    def test_FindOrOpenDefaultResolverSearchPaths(self):
        # Set up test directory structure by exporting layers. We
        # don't use Sdf.Layer.CreateNew here to avoid populating the
        # layer registry.
        layerA_Orig = Sdf.Layer.CreateAnonymous()
        Sdf.CreatePrimInLayer(layerA_Orig, "/LayerA")
        layerA_Orig.Export("dir1/sub/searchPath.sdf")

        layerB_Orig = Sdf.Layer.CreateAnonymous()
        Sdf.CreatePrimInLayer(layerB_Orig, "/LayerB")
        layerB_Orig.Export("dir2/sub/searchPath.sdf")

        # This should fail since there is no searchPath.sdf layer in the
        # current directory and no context is bound.
        self.assertFalse(Sdf.Layer.FindOrOpen("sub/searchPath.sdf"))

        # Bind an Ar.DefaultResolverContext with dir1 as a search path.
        # Now sub/searchPath.sdf should be discovered in dir1/.
        ctx1 = Ar.DefaultResolverContext([os.path.abspath("dir1/")])
        with Ar.ResolverContextBinder(ctx1):
            layerA = Sdf.Layer.FindOrOpen("sub/searchPath.sdf")
            self.assertTrue(layerA)
            self.assertTrue(layerA.GetPrimAtPath("/LayerA"))
            self.assertEqual(layerA.identifier, "sub/searchPath.sdf")

        # Do the same as above, but with dir2 as the search path.
        # Now sub/searchPath.sdf should be discovered in dir2/.
        ctx2 = Ar.DefaultResolverContext([os.path.abspath("dir2/")])
        with Ar.ResolverContextBinder(ctx2):
            layerB = Sdf.Layer.FindOrOpen("sub/searchPath.sdf")
            self.assertTrue(layerB)
            self.assertTrue(layerB.GetPrimAtPath("/LayerB"))
            self.assertEqual(layerB.identifier, "sub/searchPath.sdf")

        # Note that layerB has the same identifier as layerA, but
        # different resolved paths.
        self.assertEqual(layerA.identifier, layerB.identifier)
        self.assertNotEqual(layerA.realPath, layerB.realPath)

        # Sdf.Layer.Find should fail since no context is bound.
        self.assertFalse(Sdf.Layer.Find("sub/searchPath.sdf"))

        # Binding the contexts from above will allow Sdf.Layer.Find
        # to find the right layers.
        with Ar.ResolverContextBinder(ctx1):
            self.assertEqual(Sdf.Layer.Find("sub/searchPath.sdf"), layerA)

        with Ar.ResolverContextBinder(ctx2):
            self.assertEqual(Sdf.Layer.Find("sub/searchPath.sdf"), layerB)

        # Anonymous layers should be discoverable regardless of context.
        anonLayerA = Sdf.Layer.CreateAnonymous()
        with Ar.ResolverContextBinder(ctx1):
            self.assertEqual(Sdf.Layer.Find(anonLayerA.identifier), anonLayerA)

        with Ar.ResolverContextBinder(ctx2):
            anonLayerB = Sdf.Layer.CreateAnonymous()
        self.assertEqual(Sdf.Layer.Find(anonLayerB.identifier), anonLayerB)

    def test_FindOrOpenNoAssetAnonLayers(self):
        """
        This test case confirms assumptions about how anonymous layers behave 
        for fileformats that can read/generate layers without requiring a 
        resolved asset to read. These assumptions allow anonymous layer 
        identifiers along with file format arguments to be used to refer to 
        fully dynamically generated layers without requiring a placeholder real 
        asset file that is never read.
        """

        # Verify that our test file format exists. This file format doesn't 
        # read any assets and instead creates a simple layer with a pseudoroot
        # and optionally a single prim spec at the root with a name defined by
        # the file format argument "rootName" if the arg is provided.
        fileFormat = Sdf.FileFormat.FindByExtension(".testsdfnoasset")
        self.assertTrue(fileFormat)

        # Example anonymous layer identifier of our file format type
        layerId = "anon:placeholder:.testsdfnoasset"
        self.assertTrue(Sdf.Layer.IsAnonymousLayerIdentifier(layerId))

        # Layer find should not find this layer.
        self.assertFalse(Sdf.Layer.Find(layerId))

        # FindOrOpen however is able to open an anonymous layer with this
        # identifier because the format's Read function doesn't need an asset
        # to read this file.
        layer1 = Sdf.Layer.FindOrOpen(layerId)
        self.assertTrue(layer1)
        self.assertTrue(layer1.anonymous)
        # No args were provided so no root prim was created.
        self.assertFalse(layer1.rootPrims)

        # Layer find does find the layer now that it's open and it's the only
        # loaded layer.
        self.assertEqual(layer1, Sdf.Layer.Find(layerId))
        self.assertEqual(set(Sdf.Layer.GetLoadedLayers()), set([layer1]))

        # Now we try with layer args that should populate the layer
        layerArgs = {"rootName":"Generated"}
        # Layer find on the existing anonymous layer identifier but with args
        # now should not find the layer
        self.assertFalse(Sdf.Layer.Find(layerId, layerArgs))

        # FindOrOpen is also able to open an anonymous layer with this
        # identifier and the given args.
        layer2 = Sdf.Layer.FindOrOpen(layerId, layerArgs)
        self.assertTrue(layer2)
        self.assertTrue(layer2.anonymous)
        # Since a rootName argument was provided, a root prim with the provided
        # name is generated on the layer.
        self.assertEqual(list(layer2.rootPrims.keys()),
                         list(["Generated"]))

        # Layer find does find the layer with the provided args now too and 
        # both layers are loaded.
        self.assertEqual(layer2, Sdf.Layer.Find(layerId, layerArgs))
        self.assertEqual(set(Sdf.Layer.GetLoadedLayers()),
                         set([layer1, layer2]))

        # Call FindOrOpen using the same identifier and args. Verify that it
        # does not open a new layer and returns the existing layer.
        layer3 = Sdf.Layer.FindOrOpen(layerId, layerArgs)
        self.assertEqual(layer2, layer3)
        layer4 = Sdf.Layer.FindOrOpen(layer2.identifier)
        self.assertEqual(layer2, layer4)
        self.assertEqual(set(Sdf.Layer.GetLoadedLayers()),
                         set([layer1, layer2]))

        # Open another anonymous layer with a different identifier but the 
        # same file format arguments. This is a new layer because the identifier
        # is different
        layer5 = Sdf.Layer.FindOrOpen("anon:placeholder:other.testsdfnoasset", 
                                      layerArgs)
        self.assertNotEqual(layer2, layer5)
        self.assertTrue(layer5.anonymous)
        self.assertEqual(list(layer5.rootPrims.keys()),
                         list(["Generated"]))

        # Open another anonymous layer with the same identifier but a different
        # rootName arg. This will also be a new layer
        layer6 = Sdf.Layer.FindOrOpen(layerId, {"rootName":"Other"})
        self.assertNotEqual(layer2, layer6)
        self.assertTrue(layer6.anonymous)
        self.assertEqual(list(layer6.rootPrims.keys()),
                         list(["Other"]))

        self.assertEqual(set(Sdf.Layer.GetLoadedLayers()),
                         set([layer1, layer2, layer5, layer6]))

        # Test muting the anonymous layer. This will removed the generated
        # root prim.
        layer2.SetMuted(True)
        self.assertTrue(layer2.IsMuted())
        self.assertFalse(layer2.rootPrims)
        # Test unmuting the layer, the generated root prim returns.
        layer2.SetMuted(False)
        self.assertFalse(layer2.IsMuted())
        self.assertEqual(list(layer2.rootPrims.keys()),
                         list(["Generated"]))

        # Test reload
        # First reload returns false since layer hasn't changed.
        self.assertFalse(layer2.Reload())
        # Edit the layer adding a prim to dirty it
        Sdf.CreatePrimInLayer(layer2, Sdf.Path("/NewPrim"))
        self.assertEqual(list(layer2.rootPrims.keys()),
                         list(["Generated", "NewPrim"]))
        # Reload now succeeds and the layer is in the state when it was first
        # opened with its args.
        self.assertTrue(layer2.Reload())
        self.assertEqual(list(layer2.rootPrims.keys()),
                         list(["Generated"]))

        # Test CreateAnonymous for layers of this format. CreateAnonymous 
        # does NOT read any layer contents so the layer will be empty after 
        # creation regardless of what file format arguments are provided. While
        # this behavior may not necessarily useful in this context, it's 
        # consistent in that CreateNew and CreateAnonymous should never "read"
        # anything.

        # Create an anonymous layer with no arguments. It generates no prims.
        layer7 = Sdf.Layer.CreateAnonymous(".testsdfnoasset")
        self.assertTrue(layer7)
        self.assertTrue(layer7.anonymous)
        self.assertFalse(layer7.rootPrims)

        # Create two new anonymous layers with the same arguments as used for
        # layer2 in Sdf.Layer.FindOrOpen. Both layers will still have no 
        # root prims because we don't "read" the anonymous layer and so don't
        # generate the layer contents.
        layer8 = Sdf.Layer.CreateAnonymous(".testsdfnoasset", layerArgs)
        layer9 = Sdf.Layer.CreateAnonymous(".testsdfnoasset", layerArgs)
        self.assertTrue(layer8)
        self.assertTrue(layer9)
        self.assertFalse(layer8.rootPrims)
        self.assertFalse(layer9.rootPrims)
        # As with all layers from CreateAnonymous, they will be unique layers
        # even when generated with the same tag and args.
        self.assertNotEqual(layer8, layer9)
        self.assertNotEqual(layer8.identifier, layer9.identifier)
        self.assertEqual(
            set(Sdf.Layer.GetLoadedLayers()),
            set([layer1, layer2, layer5, layer6, layer7, layer8, layer9]))

        # Force reload the created anonymous layers. This will call Read and
        # generate the appropriate layer contents from the original file format
        # args.
        self.assertTrue(layer7.Reload(force=True))
        self.assertTrue(layer8.Reload(force=True))
        self.assertTrue(layer9.Reload(force=True))
        self.assertFalse(layer7.rootPrims)
        self.assertEqual(list(layer8.rootPrims.keys()),
                         list(["Generated"]))
        self.assertEqual(list(layer9.rootPrims.keys()),
                         list(["Generated"]))

    def test_CreatePrimInLayer(self):
        layer = Sdf.Layer.CreateAnonymous()
        self.assertFalse(layer.GetPrimAtPath("/root"))
        rootSpec = Sdf.CreatePrimInLayer(layer, '/root')
        # Must return new prim spec
        self.assertTrue(rootSpec)
        # Prim spec must match what we retrieve via namespace
        self.assertEqual(rootSpec, layer.GetPrimAtPath('/root'))
        with self.assertRaises(Tf.ErrorException):
            # Must fail with non-prim path
            Sdf.CreatePrimInLayer(layer, '/root.property')
        # Must be able to create variants
        variantSpec = Sdf.CreatePrimInLayer(layer, '/root{x=y}')
        self.assertTrue(variantSpec)
        self.assertEqual(variantSpec, layer.GetPrimAtPath('/root{x=y}'))
        # New variant names use prepend
        self.assertTrue('x' in rootSpec.variantSetNameList.prependedItems)
        self.assertTrue(len(rootSpec.variantSetNameList.addedItems) == 0)

    def test_ReloadAfterSetIdentifier(self):
        layer = Sdf.Layer.CreateNew('TestReloadAfterSetIdentifier.sdf')
        
        # CreateNew creates a new empty layer on disk. Modifying it and
        # then reloading should reset the layer back to its original
        # empty state.
        prim = Sdf.CreatePrimInLayer(layer, '/test')
        self.assertTrue(layer.Reload())
        self.assertFalse(prim)

        # However, changing a layer's identifier does not immediately
        # save the layer under its new filename. Because of that, there's
        # nothing for Reload to reload from, so it does nothing.
        prim = Sdf.CreatePrimInLayer(layer, '/test')
        layer.identifier = 'TestReloadAfterSetIdentifier_renamed.sdf'
        self.assertFalse(layer.Reload())
        self.assertTrue(prim)

    def test_VariantInertness(self):
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString(
'''#sdf 1.4.32
over "test"
{
    variantSet "vars" = {
        "off" {
        }
        "render" (
            payload = @foobar@
        ) {
        }
    }

    variantSet "empty" = {
        "nothing" {
        }
    }
}
''')
        self.assertTrue(layer.GetPrimAtPath('/test{vars=off}').isInert)
        self.assertFalse(layer.GetPrimAtPath('/test{vars=render}').isInert)
        self.assertFalse(layer.GetObjectAtPath('/test{empty=}').isInert)
        self.assertFalse(layer.GetObjectAtPath('/test{vars=}').isInert)

        # XXX this will have to be fixed up when bug PRES-82547 is fixed.
        # Possibly by deleting this case, if we decide that empty variant set
        # specs are not allowed.
        emptySet = layer.GetObjectAtPath('/test{empty=}')
        emptySet.RemoveVariant(layer.GetObjectAtPath('/test{empty=nothing}'))
        self.assertTrue(emptySet.isInert)

    def test_FileFormatTargets(self):
        # Export a dummy layer that we can try to open below.
        Sdf.Layer.CreateAnonymous().Export('dummy.test_target_format')

        # Opening a layer with the primary format target specified should
        # give the same layer as opening the layer with no target specified.
        # Note that the target argument does not show up in the identifier
        # in this case.
        layerA = Sdf.Layer.FindOrOpen(
            'dummy.test_target_format', args={'target':'A'})
        self.assertTrue(layerA)
        self.assertTrue('target=A' not in layerA.identifier)
        self.assertTrue('target' not in layerA.GetFileFormatArguments())
        self.assertEqual(
            layerA.GetFileFormat(),
            Sdf.FileFormat.FindById('test_target_format_A'))

        layerA2 = Sdf.Layer.FindOrOpen('dummy.test_target_format')
        self.assertTrue(layerA2)
        self.assertEqual(layerA, layerA2)

        # Opening a layer with another target specified should yield
        # a different layer, since this is a invoking a different file
        # format.
        layerB = Sdf.Layer.FindOrOpen(
            'dummy.test_target_format', args={'target':'B'})
        self.assertTrue(layerB)
        self.assertTrue('target=B' in layerB.identifier)
        self.assertTrue('target' in layerB.GetFileFormatArguments())
        self.assertEqual(
            layerB.GetFileFormat(),
            Sdf.FileFormat.FindById('test_target_format_B'))
        self.assertNotEqual(layerA, layerB)

        # Saving changes to both layers should be allowed. However, there
        # is no built-in synchronization so if both layers end up writing
        # to the same file they may stomp over each other.
        layerA.documentation = "From layerA"
        self.assertTrue(layerA.Save())

        layerB.documentation = "From layerB"
        self.assertTrue(layerB.Save())

        # Creating a new layer with a file format target should work. Since
        # the specified target is the primary format target, it does not
        # show up in the identifier.
        newLayerA = Sdf.Layer.CreateNew(
            'new_layer.test_target_format', args={'target':'A'})
        self.assertTrue(newLayerA)
        self.assertTrue('target=A' not in newLayerA.identifier)
        self.assertTrue('target' not in newLayerA.GetFileFormatArguments())
        self.assertEqual(
            newLayerA.GetFileFormat(),
            Sdf.FileFormat.FindById('test_target_format_A'))

        # Creating a new layer without specifying a target will fail because
        # the layer we created above has the same identifier.
        with self.assertRaises(Tf.ErrorException):
            newLayerA2 = Sdf.Layer.CreateNew('new_layer.test_target_format')

        # However, creating a layer with a different target should work.
        newLayerB = Sdf.Layer.CreateNew(
            'new_layer.test_target_format', args={'target':'B'})
        self.assertTrue(newLayerB)
        self.assertTrue('target=B' in newLayerB.identifier)
        self.assertTrue('target' in newLayerB.GetFileFormatArguments())
        self.assertEqual(
            newLayerB.GetFileFormat(),
            Sdf.FileFormat.FindById('test_target_format_B'))
        self.assertNotEqual(newLayerA, newLayerB)

        # Looking up these newly-created layers with and without
        # arguments should work as expected.
        self.assertEqual(
            newLayerA, Sdf.Layer.Find('new_layer.test_target_format'))
        self.assertEqual(
            newLayerA,
            Sdf.Layer.Find('new_layer.test_target_format',
                           args={'target':'A'}))
        self.assertEqual(
            newLayerB,
            Sdf.Layer.Find('new_layer.test_target_format',
                           args={'target':'B'}))

    def test_OpenCloseThreadSafety(self):
        import concurrent.futures
        PATH = "testSdfLayer_OpenCloseThreadSafety.sdf"
        OPENS = 25
        WORKERS = 16
        ITERATIONS = 10
        layer = Sdf.Layer.CreateNew(PATH)
        layer.Save()
        del layer

        for _ in range(ITERATIONS):
            with (concurrent.futures.ThreadPoolExecutor(
                    max_workers=WORKERS)) as executor:
                futures = [
                    executor.submit(
                        lambda: bool(Sdf.Layer.FindOrOpen(PATH)))
                    for _ in range(OPENS)]
                completed = sum(1 if f.result() else 0 for f in
                                concurrent.futures.as_completed(futures))
            self.assertEqual(completed, OPENS)
    
    def test_DefaultPrim(self):
        layer = Sdf.Layer.CreateAnonymous()
        
        # Set defaultPrim
        layer.defaultPrim = '/foo'
        self.assertEqual(layer.GetDefaultPrimAsPath(), '/foo')

        # Set the defaultPrim by name, should return path
        layer.defaultPrim = 'bar'
        Sdf.CreatePrimInLayer(layer, '/bar')
        self.assertEqual(layer.GetDefaultPrimAsPath(), '/bar')

        # Set sub-root prims as default, should return path
        layer.defaultPrim = 'foo/bar'
        Sdf.CreatePrimInLayer(layer, '/foo/bar')
        self.assertEqual(layer.GetDefaultPrimAsPath(), '/foo/bar')

        # Set invalid prim path as default, should return empty path
        layer.defaultPrim = 'foo.bar'
        self.assertEqual(layer.GetDefaultPrimAsPath(), '')
        # Set invalid path as default, should return empty path
        layer.defaultPrim = '//'
        self.assertEqual(layer.GetDefaultPrimAsPath(), '')
        layer.defaultPrim = ''
        self.assertEqual(layer.GetDefaultPrimAsPath(), '')

        # Try layer-level authoring API.
        self.assertTrue(layer.HasDefaultPrim())
        layer.ClearDefaultPrim()
        self.assertFalse(layer.HasDefaultPrim())

if __name__ == "__main__":
    unittest.main()
