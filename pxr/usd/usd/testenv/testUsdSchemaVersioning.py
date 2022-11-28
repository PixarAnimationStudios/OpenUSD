#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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

import os, unittest
from pxr import Plug, Usd, Tf

class TestUsdSchemaRegistry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdSchemaVersioning", \
            "Failed to load expected test plugin"
    
    def test_SchemaInfo(self):
        """Tests getting SchemaInfo from the schema registry"""
        print ("test_SchemaInfo")

        # Types representing three versions of a typed schema registered for 
        # this test.
        basicVersion0Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned')
        basicVersion1Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned_1')
        basicVersion2Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned_2')
        self.assertFalse(basicVersion0Type.isUnknown)
        self.assertFalse(basicVersion1Type.isUnknown)
        self.assertFalse(basicVersion2Type.isUnknown)

        # The expected values from the SchemaInfo struct for each schema type.
        expectedVersion0Info = {
            "type" : basicVersion0Type,
            "identifier" : 'TestBasicVersioned',
            "kind" : Usd.SchemaKind.ConcreteTyped
        }

        expectedVersion1Info = {
            "type" : basicVersion1Type,
            "identifier" : 'TestBasicVersioned_1',
            "kind" : Usd.SchemaKind.ConcreteTyped
        }

        expectedVersion2Info = {
            "type" : basicVersion2Type,
            "identifier" : 'TestBasicVersioned_2',
            "kind" : Usd.SchemaKind.ConcreteTyped
        }

        # Simple helper that a SchemaInfo matches an expected values.
        def _VerifySchemaInfo(schemaInfo, expected) :
            self.assertEqual(schemaInfo.type, expected["type"])
            self.assertEqual(schemaInfo.identifier, expected["identifier"])
            self.assertEqual(schemaInfo.kind, expected["kind"])

        # Find schema by TfType.
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo(basicVersion0Type),
            expectedVersion0Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo(basicVersion1Type),
            expectedVersion1Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo(basicVersion2Type),
            expectedVersion2Info)

        # Find schema by identifier.
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned'),
            expectedVersion0Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned_1'),
            expectedVersion1Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned_2'),
            expectedVersion2Info)

if __name__ == "__main__":
    unittest.main()
