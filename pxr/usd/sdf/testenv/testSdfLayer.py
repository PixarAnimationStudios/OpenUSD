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

import os, platform, itertools, sys, unittest

# Initialize Ar to use ArDefaultResolver unless a different implementation
# is specified via the TEST_SDF_LAYER_RESOLVER to allow testing with other
# filesystem-based resolvers.
preferredResolver = os.environ.get(
    "TEST_SDF_LAYER_RESOLVER", "ArDefaultResolver")

from pxr import Ar
Ar.SetPreferredResolver(preferredResolver)

# Import other modules from pxr after Ar to ensure we don't pull on Ar
# before the preferred resolver has been specified.
from pxr import Sdf, Tf

class TestSdfLayer(unittest.TestCase):
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

    def test_OpenWithInvalidFormat(self):
        l = Sdf.Layer.FindOrOpen('foo.invalid')
        self.assertIsNone(l)

        # XXX: 
        # OpenAsAnonymous raises a coding error when it cannot determine a
        # file format. This is inconsistent with FindOrOpen and is purely
        # historical.
        with self.assertRaises(Tf.ErrorException):
            l = Sdf.Layer.OpenAsAnonymous('foo.invalid')

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
        def _TestWithTag(tag, validExtension):
            layer = Sdf.Layer.CreateAnonymous(tag)
            layerId = layer.identifier
            self.assertEqual(Sdf.Layer.FindOrOpen(layerId), layer)

            del layer
            self.assertFalse(
                [l for l in Sdf.Layer.GetLoadedLayers() 
                 if l.identifier == layerId])

            # FindOrOpen currently throws a coding error when given an
            # anonymous layer identifier with an unrecognized (or no)
            # extension.
            if validExtension:
                self.assertFalse(Sdf.Layer.FindOrOpen(layerId))
            else:
                with self.assertRaises(Tf.ErrorException):
                    self.assertFalse(Sdf.Layer.FindOrOpen(layerId))

        _TestWithTag("", validExtension=False)
        _TestWithTag(".sdf", validExtension=True)
        _TestWithTag(".invalid", validExtension=False)
        _TestWithTag("test", validExtension=False)
        _TestWithTag("test.invalid", validExtension=False)
        _TestWithTag("test.sdf", validExtension=True)

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

    @unittest.skipIf(platform.system() == "Windows" and
                     not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "This test case currently fails on Windows due to "
                     "path canonicalization issues except with Ar 2.0.")
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

    def test_UpdateExternalReference(self):
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

        # Calling UpdateExternalReference with an empty old layer path is
        # not allowed.
        origLayer = srcLayer.ExportToString()
        self.assertFalse(srcLayer.UpdateExternalReference("", ""))
        self.assertEqual(origLayer, srcLayer.ExportToString())

        # Calling UpdateExternalReference with an asset path that does not
        # exist should result in no changes to the layer.
        self.assertTrue(srcLayer.UpdateExternalReference(
            "nonexistent.sdf", "foo.sdf"))
        self.assertEqual(origLayer, srcLayer.ExportToString())

        # Test renaming / removing sublayers.
        self.assertTrue(srcLayer.UpdateExternalReference(
            "sublayer_1.sdf", "new_sublayer_1.sdf"))
        self.assertEqual(
            srcLayer.subLayerPaths, ["new_sublayer_1.sdf", "sublayer_2.sdf"])

        self.assertTrue(srcLayer.UpdateExternalReference("sublayer_2.sdf", ""))
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

        self.assertTrue(srcLayer.UpdateExternalReference(
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

        self.assertTrue(srcLayer.UpdateExternalReference(
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
        self.assertTrue(srcLayer.UpdateExternalReference(
            "ref_1.sdf", "new_ref_1.sdf"))
        for prim in primsWithReferences:
            self.assertEqual(
                prim.referenceList.explicitItems,
                [Sdf.Reference("new_ref_1.sdf", "/Ref"),
                 Sdf.Reference("ref_2.sdf", "/Ref2")],
                "Unexpected references {0} at {1}"
                .format(prim.referenceList, prim.path))

        self.assertTrue(srcLayer.UpdateExternalReference(
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

    @unittest.skipIf(platform.system() == "Windows" and
                     not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "This test case currently fails on Windows due to "
                     "path canonicalization issues except with Ar 2.0.")
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

    @unittest.skipIf(preferredResolver != "ArDefaultResolver",
                     "Test uses search-path functionality specific to "
                     "ArDefaultResolver")
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

if __name__ == "__main__":
    unittest.main()
