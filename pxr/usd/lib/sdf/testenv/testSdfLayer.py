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

import sys, unittest
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

if __name__ == "__main__":
    unittest.main()
