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
    
    def test_ParseFamilyAndVersion(self):
        """Tests the parsing of family and version values from schema 
        identifier values"""
        print ("test_ParseFamilyAndVersion")

        # Helper for verifying the parsed identifier produces expected families
        # and versions.
        def _VerifyVersionParsing(identifier, family, version, 
                                  expectFamilyAllowed=True, 
                                  expectIdentifierAllowed=True):
            # Verify the identifier can be parsed into the given family and
            # version
            self.assertEqual(
                Usd.SchemaRegistry.ParseSchemaFamilyAndVersionFromIdentifier(
                    identifier),
                (family, version))

            # Verify the allowed-ness of the parsed family.
            isFamilyAllowed = Usd.SchemaRegistry.IsAllowedSchemaFamily(family)
            if expectFamilyAllowed:
                self.assertTrue(isFamilyAllowed)
            else:
                self.assertFalse(isFamilyAllowed)

            # Verify the allowed-ness of the identifier.
            isIdentifierAllowed = \
                Usd.SchemaRegistry.IsAllowedSchemaIdentifier(identifier)
            if expectIdentifierAllowed:
                self.assertTrue(isIdentifierAllowed)
            else:
                self.assertFalse(isIdentifierAllowed)

            # Make the identifier from the family and version and verify that it
            # matches the identifier...
            identifierFromFamilyAndVersion = \
                Usd.SchemaRegistry.MakeSchemaIdentifierForFamilyAndVersion(
                    family, version)
            # ...except if the parsed family is allowed but the identifier is 
            # not, this  will because the identifier is not the identifier that 
            # family and version produce, so it won't match. 
            if expectFamilyAllowed and not expectIdentifierAllowed:
                self.assertNotEqual(identifierFromFamilyAndVersion, identifier)
            else:
                self.assertEqual(identifierFromFamilyAndVersion, identifier)

        # Parse allowed versioned identifiers.
        _VerifyVersionParsing("Foo", "Foo", 0)        
        _VerifyVersionParsing("Foo_1", "Foo", 1)        
        _VerifyVersionParsing("Foo_20", "Foo", 20)        

        _VerifyVersionParsing("Foo_Bar", "Foo_Bar", 0)        
        _VerifyVersionParsing("Foo_Bar_1", "Foo_Bar", 1)        
        _VerifyVersionParsing("Foo_Bar_20", "Foo_Bar", 20)        

        _VerifyVersionParsing("Foo_", "Foo_", 0)        
        _VerifyVersionParsing("Foo__1", "Foo_", 1)        
        _VerifyVersionParsing("Foo__20", "Foo_", 20)        

        _VerifyVersionParsing("Foo_1_", "Foo_1_", 0)        
        _VerifyVersionParsing("Foo_1__1", "Foo_1_", 1)        
        _VerifyVersionParsing("Foo_1__20", "Foo_1_", 20)      

        _VerifyVersionParsing("_Foo", "_Foo", 0)        
        _VerifyVersionParsing("_Foo_1", "_Foo", 1)        
        _VerifyVersionParsing("_Foo_20", "_Foo", 20)        

        # Parse bad versioned identifiers that parse into unallowed families.
        _VerifyVersionParsing("_1", "", 1, 
            expectFamilyAllowed=False, expectIdentifierAllowed=False)  
        _VerifyVersionParsing("Foo_20_1", "Foo_20", 1, 
            expectFamilyAllowed=False, expectIdentifierAllowed=False)  
        _VerifyVersionParsing("Foo_22.5", "Foo_22.5", 0, 
            expectFamilyAllowed=False, expectIdentifierAllowed=False)  
        _VerifyVersionParsing("Foo_-1", "Foo_-1", 0, 
            expectFamilyAllowed=False, expectIdentifierAllowed=False)  
        _VerifyVersionParsing("", "", 0, 
            expectFamilyAllowed=False, expectIdentifierAllowed=False)  

        # Parse bad versioned identifiers that parse into allowed families,
        # but don't use the expected suffix format for that version.
        _VerifyVersionParsing("Foo_0", "Foo", 0, 
            expectFamilyAllowed=True, expectIdentifierAllowed=False)  
        _VerifyVersionParsing("Foo_01", "Foo", 1, 
            expectFamilyAllowed=True, expectIdentifierAllowed=False)  

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
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 0
        }

        expectedVersion1Info = {
            "type" : basicVersion1Type,
            "identifier" : 'TestBasicVersioned_1',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 1
        }

        expectedVersion2Info = {
            "type" : basicVersion2Type,
            "identifier" : 'TestBasicVersioned_2',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 2
        }

        # Simple helper that verifies a SchemaInfo matches expected values.
        def _VerifySchemaInfo(schemaInfo, expected) :
            self.assertEqual(schemaInfo.type, expected["type"])
            self.assertEqual(schemaInfo.identifier, expected["identifier"])
            self.assertEqual(schemaInfo.kind, expected["kind"])
            self.assertEqual(schemaInfo.family, expected["family"])
            self.assertEqual(schemaInfo.version, expected["version"])

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

        # Find schema by family and version.
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned', 0),
            expectedVersion0Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned', 1),
            expectedVersion1Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo('TestBasicVersioned', 2),
            expectedVersion2Info)

        # Simple helper that verifies a list of SchemaInfo all match an expected
        # values.
        def _VerifySchemaInfoList(schemaInfoList, expectedList) :
            self.assertEqual(len(schemaInfoList), len(expectedList))
            for schemaInfo, expected in zip(schemaInfoList, expectedList):
                _VerifySchemaInfo(schemaInfo, expected)

        # Find all schemas in a family
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily('TestBasicVersioned'),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])

        # Find filtered schemas in a family: VersionPolicy = All
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0, 
                Usd.SchemaRegistry.VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                Usd.SchemaRegistry.VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2, Usd.SchemaRegistry.VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])

        # Find filtered schemas in a family: VersionPolicy = GreaterThan
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                Usd.SchemaRegistry.VersionPolicy.GreaterThan),
            [expectedVersion2Info, expectedVersion1Info, ])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                Usd.SchemaRegistry.VersionPolicy.GreaterThan),
            [expectedVersion2Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                Usd.SchemaRegistry.VersionPolicy.GreaterThan),
            [])

        # Find filtered schemas in a family: VersionPolicy = GreaterThanOrEqual
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                Usd.SchemaRegistry.VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                Usd.SchemaRegistry.VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info, expectedVersion1Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2, 
                Usd.SchemaRegistry.VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info])

        # Find filtered schemas in a family: VersionPolicy = LessThan
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0, 
                Usd.SchemaRegistry.VersionPolicy.LessThan),
            [])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1, 
                Usd.SchemaRegistry.VersionPolicy.LessThan),
            [expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                Usd.SchemaRegistry.VersionPolicy.LessThan),
            [expectedVersion1Info, expectedVersion0Info])

        # Find filtered schemas in a family: VersionPolicy = LessThanOrEqual
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                Usd.SchemaRegistry.VersionPolicy.LessThanOrEqual),
            [expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                Usd.SchemaRegistry.VersionPolicy.LessThanOrEqual),
            [expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                Usd.SchemaRegistry.VersionPolicy.LessThanOrEqual),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
    
        # Edge cases:
        # Verify that calling FindSchemaInfo and FindSchemaInfosInFamily, 
        # passing a registered non-zero versioned schema identifier as the 
        # family, does not return any schema info.
        validVersionedIdentifier = 'TestBasicVersioned_1'
        # Find by identifier is valid
        self.assertIsNotNone(
            Usd.SchemaRegistry.FindSchemaInfo(validVersionedIdentifier))
        # Find by family and version 0 is not valid
        self.assertIsNone(
            Usd.SchemaRegistry.FindSchemaInfo(validVersionedIdentifier, 0))
        # Find all in family produces no schemas.
        self.assertEqual(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(validVersionedIdentifier), 
            [])
        self.assertEqual(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                validVersionedIdentifier, 0, 
                Usd.SchemaRegistry.VersionPolicy.All), 
            [])

if __name__ == "__main__":
    unittest.main()
