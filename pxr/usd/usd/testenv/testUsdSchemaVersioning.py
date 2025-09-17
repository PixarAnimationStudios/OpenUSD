#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, unittest
from pxr import Plug, Usd, Tf

# Map the version policy enum to reduce text clutter.
VersionPolicy = Usd.SchemaRegistry.VersionPolicy

class TestUsdSchemaRegistry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdSchemaVersioning", \
            "Failed to load expected test plugin"
    
        # Types representing three versions of a typed schema registered for 
        # this test.
        cls.BasicVersion0Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned')
        cls.BasicVersion1Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned_1')
        cls.BasicVersion2Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestBasicVersioned_2')
        assert(not cls.BasicVersion0Type.isUnknown)
        assert(not cls.BasicVersion1Type.isUnknown)
        assert(not cls.BasicVersion2Type.isUnknown)

        # Types representing three versions of a single-apply API schema
        # registered for this test.
        cls.SingleApiVersion0Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedSingleApplyAPI')
        cls.SingleApiVersion1Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedSingleApplyAPI_1')
        cls.SingleApiVersion2Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedSingleApplyAPI_2')
        assert(not cls.SingleApiVersion0Type.isUnknown)
        assert(not cls.SingleApiVersion1Type.isUnknown)
        assert(not cls.SingleApiVersion2Type.isUnknown)

        # Types representing three versions of a multiple-apply API schema
        # registered for this test.
        cls.MultiApiVersion0Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedMultiApplyAPI')
        cls.MultiApiVersion1Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedMultiApplyAPI_1')
        cls.MultiApiVersion2Type = Tf.Type.FindByName(
            'TestUsdSchemaVersioningTestVersionedMultiApplyAPI_2')
        assert(not cls.MultiApiVersion0Type.isUnknown)
        assert(not cls.MultiApiVersion1Type.isUnknown)
        assert(not cls.MultiApiVersion2Type.isUnknown)

    def test_ParseFamilyAndVersion(self):
        """Tests the parsing of family and version values from schema 
        identifier values"""

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

        # The expected values from the SchemaInfo struct for each schema type.
        expectedVersion0Info = {
            "type" : self.BasicVersion0Type,
            "identifier" : 'TestBasicVersioned',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 0
        }

        expectedVersion1Info = {
            "type" : self.BasicVersion1Type,
            "identifier" : 'TestBasicVersioned_1',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 1
        }

        expectedVersion2Info = {
            "type" : self.BasicVersion2Type,
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
            Usd.SchemaRegistry.FindSchemaInfo(self.BasicVersion0Type),
            expectedVersion0Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo(self.BasicVersion1Type),
            expectedVersion1Info)
        _VerifySchemaInfo(
            Usd.SchemaRegistry.FindSchemaInfo(self.BasicVersion2Type),
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
                VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2, VersionPolicy.All),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])

        # Find filtered schemas in a family: VersionPolicy = GreaterThan
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                VersionPolicy.GreaterThan),
            [expectedVersion2Info, expectedVersion1Info, ])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                VersionPolicy.GreaterThan),
            [expectedVersion2Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                VersionPolicy.GreaterThan),
            [])

        # Find filtered schemas in a family: VersionPolicy = GreaterThanOrEqual
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info, expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info, expectedVersion1Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2, 
                VersionPolicy.GreaterThanOrEqual),
            [expectedVersion2Info])

        # Find filtered schemas in a family: VersionPolicy = LessThan
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0, 
                VersionPolicy.LessThan),
            [])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1, 
                VersionPolicy.LessThan),
            [expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                VersionPolicy.LessThan),
            [expectedVersion1Info, expectedVersion0Info])

        # Find filtered schemas in a family: VersionPolicy = LessThanOrEqual
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 0,
                VersionPolicy.LessThanOrEqual),
            [expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 1,
                VersionPolicy.LessThanOrEqual),
            [expectedVersion1Info, expectedVersion0Info])
        _VerifySchemaInfoList(
            Usd.SchemaRegistry.FindSchemaInfosInFamily(
                'TestBasicVersioned', 2,
                VersionPolicy.LessThanOrEqual),
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
                VersionPolicy.All), 
            [])

    def test_PrimIsA(self):
        """Tests all Usd.Prim API for querying if a prim 'IsA' with 
           versioned typed schemas"""        

        # The expected values from the SchemaInfo struct for each schema type.
        expectedVersion0Info = {
            "type" : self.BasicVersion0Type,
            "identifier" : 'TestBasicVersioned',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 0
        }

        expectedVersion1Info = {
            "type" : self.BasicVersion1Type,
            "identifier" : 'TestBasicVersioned_1',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 1
        }

        expectedVersion2Info = {
            "type" : self.BasicVersion2Type,
            "identifier" : 'TestBasicVersioned_2',
            "kind" : Usd.SchemaKind.ConcreteTyped,
            "family" : 'TestBasicVersioned',
            "version" : 2
        }

        # Create a prim typed with each version of the schema.
        stage = Usd.Stage.CreateInMemory()
        primV0 = stage.DefinePrim("/Prim", "TestBasicVersioned")
        primV1 = stage.DefinePrim("/Prim1", "TestBasicVersioned_1")
        primV2 = stage.DefinePrim("/Prim2", "TestBasicVersioned_2")

        self.assertTrue(primV0)
        self.assertTrue(primV1)
        self.assertTrue(primV2)

        # Verifies that IsA is true for the prim, using all three input types,
        # for the schema specified in schemaInfo.
        def _VerifyIsA(prim, schemaInfo):
            # By TfType
            self.assertTrue(prim.IsA(schemaInfo["type"]))
            # By Identifier
            self.assertTrue(prim.IsA(schemaInfo["identifier"]))
            # By Family and Version
            self.assertTrue(prim.IsA(schemaInfo["family"], 
                                     schemaInfo["version"]))

        # Verifies that IsA is false for the prim, using all three input types,
        # for the schema specified in schemaInfo.
        def _VerifyNotIsA(prim, schemaInfo):
            # By TfType
            self.assertFalse(prim.IsA(schemaInfo["type"]))
            # By Identifier
            self.assertFalse(prim.IsA(schemaInfo["identifier"]))
            # By Family and Version
            self.assertFalse(prim.IsA(schemaInfo["family"], 
                                      schemaInfo["version"]))

        def _VerifyIsInFamily(prim, schemaInfo, versionPolicy):
            # By TfType
            self.assertTrue(prim.IsInFamily(
                schemaInfo["type"], versionPolicy))
            # By Identifier
            self.assertTrue(prim.IsInFamily(
                schemaInfo["identifier"], versionPolicy))
            # By Family and Version
            self.assertTrue(prim.IsInFamily(
                schemaInfo["family"], schemaInfo["version"], versionPolicy))

        def _VerifyNotIsInFamily(prim, schemaInfo, versionPolicy):
            # By TfType
            self.assertFalse(prim.IsInFamily(
                schemaInfo["type"], versionPolicy))
            # By Identifier
            self.assertFalse(prim.IsInFamily(
                schemaInfo["identifier"], versionPolicy))
            # By Family and Version
            self.assertFalse(prim.IsInFamily(
                schemaInfo["family"], schemaInfo["version"], versionPolicy))

        # Prim0 IsA version 0 of "TestBasicVersioned". Verify IsA calls with
        # each schema version
        _VerifyIsA(primV0, expectedVersion0Info)
        _VerifyNotIsA(primV0, expectedVersion1Info)
        _VerifyNotIsA(primV0, expectedVersion2Info)

        # Verify prim0 "is any version" in schema family "TestBasicVersioned"
        # and that its version is returned as 0
        self.assertTrue(primV0.IsInFamily("TestBasicVersioned"))
        self.assertEqual(
            primV0.GetVersionIfIsInFamily("TestBasicVersioned"), 0)

        # Verify the results of calling IsInFamily with each version 
        # policy filter for each schema version.
        _VerifyIsInFamily(
            primV0, expectedVersion0Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV0, expectedVersion1Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV0, expectedVersion2Info, VersionPolicy.All)

        _VerifyNotIsInFamily(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThan)
        _VerifyNotIsInFamily(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThan)
        _VerifyNotIsInFamily(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThan)

        _VerifyIsInFamily(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyNotIsInFamily(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyNotIsInFamily(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        _VerifyNotIsInFamily(
            primV0, expectedVersion0Info, VersionPolicy.LessThan)
        _VerifyIsInFamily(
            primV0, expectedVersion1Info, VersionPolicy.LessThan)
        _VerifyIsInFamily(
            primV0, expectedVersion2Info, VersionPolicy.LessThan)

        _VerifyIsInFamily(
            primV0, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        _VerifyIsInFamily(
            primV0, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        _VerifyIsInFamily(
            primV0, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Prim1 IsA version 1 of "TestBasicVersioned". Verify IsA calls with
        # each schema version
        _VerifyNotIsA(primV1, expectedVersion0Info)
        _VerifyIsA(primV1, expectedVersion1Info)
        _VerifyNotIsA(primV1, expectedVersion2Info)

        # Verify prim1 "is any version" in schema family "TestBasicVersioned" 
        # and that its version is returned as 1
        self.assertTrue(primV1.IsInFamily("TestBasicVersioned"))
        self.assertEqual(
            primV1.GetVersionIfIsInFamily("TestBasicVersioned"), 1)

        # Verify the results of calling IsInFamily with each version 
        # policy filter for each schema version.
        _VerifyIsInFamily(
            primV1, expectedVersion0Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV1, expectedVersion1Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV1, expectedVersion2Info, VersionPolicy.All)

        _VerifyIsInFamily(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThan)
        _VerifyNotIsInFamily(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThan)
        _VerifyNotIsInFamily(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThan)

        _VerifyIsInFamily(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyIsInFamily(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyNotIsInFamily(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        _VerifyNotIsInFamily(
            primV1, expectedVersion0Info, VersionPolicy.LessThan)
        _VerifyNotIsInFamily(
            primV1, expectedVersion1Info, VersionPolicy.LessThan)
        _VerifyIsInFamily(
            primV1, expectedVersion2Info, VersionPolicy.LessThan)

        _VerifyNotIsInFamily(
            primV1, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        _VerifyIsInFamily(
            primV1, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        _VerifyIsInFamily(
            primV1, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Prim2 IsA version 2 of "TestBasicVersioned". Verify IsA calls with
        # each schema version
        _VerifyNotIsA(primV2, expectedVersion0Info)
        _VerifyNotIsA(primV2, expectedVersion1Info)
        _VerifyIsA(primV2, expectedVersion2Info)

        # Verify prim2 "is any version" in schema family "TestBasicVersioned"
        # and that its version is returned as 2
        self.assertTrue(primV2.IsInFamily("TestBasicVersioned"))
        self.assertEqual(
            primV2.GetVersionIfIsInFamily("TestBasicVersioned"), 2)

        # Verify the results of calling IsInFamily with each version 
        # policy filter for each schema version.
        _VerifyIsInFamily(
            primV2, expectedVersion0Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV2, expectedVersion1Info, VersionPolicy.All)
        _VerifyIsInFamily(
            primV2, expectedVersion2Info, VersionPolicy.All)

        _VerifyIsInFamily(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThan)
        _VerifyIsInFamily(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThan)
        _VerifyNotIsInFamily(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThan)

        _VerifyIsInFamily(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyIsInFamily(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        _VerifyIsInFamily(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        _VerifyNotIsInFamily(
            primV2, expectedVersion0Info, VersionPolicy.LessThan)
        _VerifyNotIsInFamily(
            primV2, expectedVersion1Info, VersionPolicy.LessThan)
        _VerifyNotIsInFamily(
            primV2, expectedVersion2Info, VersionPolicy.LessThan)

        _VerifyNotIsInFamily(
            primV2, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        _VerifyNotIsInFamily(
            primV2, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        _VerifyIsInFamily(
            primV2, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Verify calls to IsInFamily with a version that doesn't 
        # itself exist but is from a valid schema family.

        # Version 3 of "TestBasicVersioned" does not exist and none of the 
        # prims "IsA" TestBasicVersioned_3
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestBasicVersioned_3"))
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestBasicVersioned", 3))
        self.assertFalse(primV2.IsA("TestBasicVersioned", 3))
        self.assertFalse(primV2.IsA("TestBasicVersioned_3"))

        # However, "TestBasicVersioned_3" can still be used to query about
        # IsInFamily; the prims are versions in the same schema family
        # that are less than version 3
        self.assertTrue(primV2.IsInFamily(
            "TestBasicVersioned", 3, VersionPolicy.LessThan))
        self.assertFalse(primV2.IsInFamily(
            "TestBasicVersioned", 3, VersionPolicy.GreaterThanOrEqual))
        self.assertTrue(primV2.IsInFamily(
            "TestBasicVersioned_3", VersionPolicy.LessThan))
        self.assertFalse(primV2.IsInFamily(
            "TestBasicVersioned_3", VersionPolicy.GreaterThanOrEqual))

    # Verifies that a prim HasAPI, using all three input types, for the schema 
    # specified in schemaInfo.
    def _VerifyHasAPI(self, prim, schemaInfo):
        # By TfType
        self.assertTrue(prim.HasAPI(schemaInfo["type"]))
        # By Identifier
        self.assertTrue(prim.HasAPI(schemaInfo["identifier"]))
        # By Family and Version
        self.assertTrue(prim.HasAPI(schemaInfo["family"], 
                                    schemaInfo["version"]))

    # Verifies that a prim does NOT HasAPI, using all three input types, for the
    # schema specified in schemaInfo.
    def _VerifyNotHasAPI(self, prim, schemaInfo):
        # By TfType
        self.assertFalse(prim.HasAPI(schemaInfo["type"]))
        # By Identifier
        self.assertFalse(prim.HasAPI(schemaInfo["identifier"]))
        # By Family and Version
        self.assertFalse(prim.HasAPI(schemaInfo["family"], 
                                        schemaInfo["version"]))

    # Verifies that a prim HasAPI with the specified instance name, using all 
    # three input types, for the schema specified in schemaInfo.
    def _VerifyHasAPIInstance(self, prim, schemaInfo, instanceName):
        # By TfType
        self.assertTrue(prim.HasAPI(schemaInfo["type"], instanceName))
        # By Identifier
        self.assertTrue(prim.HasAPI(schemaInfo["identifier"], instanceName))
        # By Family and Version
        self.assertTrue(prim.HasAPI(schemaInfo["family"], 
                                    schemaInfo["version"],
                                    instanceName))

    # Verifies that a prim does NOT HasAPI with the specified instance name, 
    # using all three input types, for the schema specified in schemaInfo.
    def _VerifyNotHasAPIInstance(self, prim, schemaInfo, instanceName):
        # By TfType
        self.assertFalse(prim.HasAPI(schemaInfo["type"], instanceName))
        # By Identifier
        self.assertFalse(prim.HasAPI(schemaInfo["identifier"], instanceName))
        # By Family and Version
        self.assertFalse(prim.HasAPI(schemaInfo["family"], 
                                    schemaInfo["version"], instanceName))

    # Verifies that a prim HasAPIInFamily, using all three input 
    # types, for the schema specified in schemaInfo.
    def _VerifyHasAPIInFamily(self, prim, schemaInfo, versionPolicy):
        # By TfType
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["type"], versionPolicy))
        # By Identifier
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["identifier"], versionPolicy))
        # By Family and Version
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["family"], schemaInfo["version"], versionPolicy))

    # Verifies that a prim does NOT HasAPIInFamily, using all three
    # input types, for the schema specified in schemaInfo.
    def _VerifyNotHasAPIInFamily(
            self, prim, schemaInfo, versionPolicy):
        # By TfType
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["type"], versionPolicy))
        # By Identifier
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["identifier"], versionPolicy))
        # By Family and Version
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["family"], schemaInfo["version"], versionPolicy))

    # Verifies that a prim HasAPIInFamily with the specified instance
    # name, using all three input types, for the schema specified in schemaInfo.
    def _VerifyHasAPIInFamilyInstance(
            self, prim, schemaInfo, versionPolicy, instanceName):
        # By TfType
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["type"], versionPolicy, instanceName))
        # By Identifier
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["identifier"], versionPolicy, instanceName))
        # By Family and Version
        self.assertTrue(prim.HasAPIInFamily(
            schemaInfo["family"], schemaInfo["version"], versionPolicy, 
            instanceName))

    # Verifies that a prim does NOT HasAPIInFamily with the specified
    # instance name, using all three input types, for the schema specified in
    # schemaInfo.
    def _VerifyNotHasAPIInFamilyInstance(
            self, prim, schemaInfo, versionPolicy, instanceName):
        # By TfType
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["type"], versionPolicy, instanceName))
        # By Identifier
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["identifier"], versionPolicy, instanceName))
        # By Family and Version
        self.assertFalse(prim.HasAPIInFamily(
            schemaInfo["family"], schemaInfo["version"], versionPolicy, 
            instanceName))

    def test_PrimHasAPI_SingleApply(self):
        """Tests all Usd.Prim API for querying if a prim 'HasAPI' with 
           versioned single-apply API schemas"""        

        # The expected values from the SchemaInfo struct for each schema type.
        expectedVersion0Info = {
            "type" : self.SingleApiVersion0Type,
            "identifier" : 'TestVersionedSingleApplyAPI',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 0
        }

        expectedVersion1Info = {
            "type" : self.SingleApiVersion1Type,
            "identifier" : 'TestVersionedSingleApplyAPI_1',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 1
        }

        expectedVersion2Info = {
            "type" : self.SingleApiVersion2Type,
            "identifier" : 'TestVersionedSingleApplyAPI_2',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 2
        }

        # Create a prim each with a different version of the schema applied.
        stage = Usd.Stage.CreateInMemory()
        primV0 = stage.DefinePrim("/Prim")
        primV1 = stage.DefinePrim("/Prim1")
        primV2 = stage.DefinePrim("/Prim2")

        self.assertTrue(primV0)
        self.assertTrue(primV1)
        self.assertTrue(primV2)

        primV0.ApplyAPI(self.SingleApiVersion0Type)
        primV1.ApplyAPI(self.SingleApiVersion1Type)
        primV2.ApplyAPI(self.SingleApiVersion2Type)

        # Prim0 HasAPI version 0 of "TestVersionedSingleApplyAPI" (and not any 
        # of the other versions). Verify HasAPI calls with each schema version.
        self._VerifyHasAPI(primV0, expectedVersion0Info)
        self._VerifyNotHasAPI(primV0, expectedVersion1Info)
        self._VerifyNotHasAPI(primV0, expectedVersion2Info)

        # Verify prim0 "has any API version" in schema family 
        # "TestVersionedSingleApplyAPI" and that its version is returned as 0
        self.assertTrue(
            primV0.HasAPIInFamily("TestVersionedSingleApplyAPI"))
        self.assertEqual(primV0.GetVersionIfHasAPIInFamily(
            "TestVersionedSingleApplyAPI"), 0)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion0Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion1Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion2Info, VersionPolicy.All)

        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThan)
        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThan)
        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThan)

        self._VerifyHasAPIInFamily(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        self._VerifyNotHasAPIInFamily(
            primV0, expectedVersion0Info, VersionPolicy.LessThan)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion1Info, VersionPolicy.LessThan)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion2Info, VersionPolicy.LessThan)

        self._VerifyHasAPIInFamily(
            primV0, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV0, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Prim1 HasAPI version 1 of "TestVersionedSingleApplyAPI" (and not any 
        # of the other versions). Verify HasAPI calls with each schema version.
        self._VerifyNotHasAPI(primV1, expectedVersion0Info)
        self._VerifyHasAPI(primV1, expectedVersion1Info)
        self._VerifyNotHasAPI(primV1, expectedVersion2Info)

        # Verify prim1 "has any API version" in schema family 
        # "TestVersionedSingleApplyAPI" and that its version is returned as 1
        self.assertTrue(
            primV1.HasAPIInFamily("TestVersionedSingleApplyAPI"))
        self.assertEqual(primV1.GetVersionIfHasAPIInFamily(
            "TestVersionedSingleApplyAPI"), 1)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion0Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion1Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion2Info, VersionPolicy.All)

        self._VerifyHasAPIInFamily(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThan)
        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThan)
        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThan)

        self._VerifyHasAPIInFamily(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion0Info, VersionPolicy.LessThan)
        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion1Info, VersionPolicy.LessThan)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion2Info, VersionPolicy.LessThan)

        self._VerifyNotHasAPIInFamily(
            primV1, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV1, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Prim2 HasAPI version 2 of "TestVersionedSingleApplyAPI" (and not any 
        # of the other versions). Verify HasAPI calls with each schema version.
        self._VerifyNotHasAPI(primV2, expectedVersion0Info)
        self._VerifyNotHasAPI(primV2, expectedVersion1Info)
        self._VerifyHasAPI(primV2, expectedVersion2Info)

        # Verify prim2 "has any API version" in schema family 
        # "TestVersionedSingleApplyAPI" and that its version is returned as 2
        self.assertTrue(
            primV2.HasAPIInFamily("TestVersionedSingleApplyAPI"))
        self.assertEqual(primV2.GetVersionIfHasAPIInFamily(
            "TestVersionedSingleApplyAPI"), 2)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion0Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion1Info, VersionPolicy.All)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion2Info, VersionPolicy.All)

        self._VerifyHasAPIInFamily(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThan)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThan)
        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThan)

        self._VerifyHasAPIInFamily(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual)

        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion0Info, VersionPolicy.LessThan)
        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion1Info, VersionPolicy.LessThan)
        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion2Info, VersionPolicy.LessThan)

        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion0Info, VersionPolicy.LessThanOrEqual)
        self._VerifyNotHasAPIInFamily(
            primV2, expectedVersion1Info, VersionPolicy.LessThanOrEqual)
        self._VerifyHasAPIInFamily(
            primV2, expectedVersion2Info, VersionPolicy.LessThanOrEqual)

        # Verify calls to HasAPIInFamily with a version that doesn't 
        # itself exist but is from a valid schema family.

        # Version 3 of "TestVersionedSingleApplyAPI" does not exist and none of
        # the prims "HasAPI" TestVersionedSingleApplyAPI_3
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestVersionedSingleApplyAPI_3"))
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestVersionedSingleApplyAPI", 3))
        self.assertFalse(primV2.HasAPI("TestVersionedSingleApplyAPI", 3))
        self.assertFalse(primV2.HasAPI("TestVersionedSingleApplyAPI_3"))

        # However, "TestVersionedSingleApplyAPI_3" can still be used to query
        # about HasAPIInFamily; the prims have applied APIs that are
        # versions in the same schema family that are less than version 3
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedSingleApplyAPI", 3, VersionPolicy.LessThan))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedSingleApplyAPI", 3, VersionPolicy.GreaterThanOrEqual))
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedSingleApplyAPI_3", VersionPolicy.LessThan))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedSingleApplyAPI_3", VersionPolicy.GreaterThanOrEqual))

    def test_PrimHasAPI_MultiApply(self):
        """Tests all Usd.Prim API for querying if a prim 'HasAPI' with 
           versioned multiple-apply API schemas"""        

        # The expected values from the SchemaInfo struct for each schema type.
        expectedVersion0Info = {
            "type" : self.MultiApiVersion0Type,
            "identifier" : 'TestVersionedMultiApplyAPI',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 0
        }

        expectedVersion1Info = {
            "type" : self.MultiApiVersion1Type,
            "identifier" : 'TestVersionedMultiApplyAPI_1',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 1
        }

        expectedVersion2Info = {
            "type" : self.MultiApiVersion2Type,
            "identifier" : 'TestVersionedMultiApplyAPI_2',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 2
        }

        # Create a prim each with a different versions and instances of the 
        # schema applied.
        stage = Usd.Stage.CreateInMemory()
        primV0 = stage.DefinePrim("/Prim")
        primV1 = stage.DefinePrim("/Prim1")
        primV2 = stage.DefinePrim("/Prim2")

        self.assertTrue(primV0)
        self.assertTrue(primV1)
        self.assertTrue(primV2)

        # The schemas are applied for combinations of version and instance:
        # prim0 -> "API:foo", "API_2:bar"
        # prim1 -> "API_1:foo", "API_1:bar"
        # prim2 -> "API_2:foo", "API:bar"
        self.assertTrue(primV0.ApplyAPI(self.MultiApiVersion0Type, "foo"))
        self.assertTrue(primV1.ApplyAPI(self.MultiApiVersion1Type, "foo"))
        self.assertTrue(primV2.ApplyAPI(self.MultiApiVersion2Type, "foo"))

        self.assertTrue(primV0.ApplyAPI(self.MultiApiVersion2Type, "bar"))
        self.assertTrue(primV1.ApplyAPI(self.MultiApiVersion1Type, "bar"))
        self.assertTrue(primV2.ApplyAPI(self.MultiApiVersion0Type, "bar"))

        # Helper for verifying all inputs to HasAPI produce the expected return 
        # values given the instances for the given schema it is expected to have
        def _VerifyHasAPIInstances(prim, schemaInfo, expectedHasInstanceNames):
            # Verify the return value for all calls to HasAPI without a 
            # specified instance name. We expect these to return true iff we
            # expect the prim to have any instances of the schema.
            if expectedHasInstanceNames:
                self._VerifyHasAPI(prim, schemaInfo)
            else:
                self._VerifyNotHasAPI(prim, schemaInfo)
            
            # Verify all calls to HasAPI "foo" return true iff we expect the
            # prim to have a "foo" instance of the schema.
            if "foo" in expectedHasInstanceNames:
                self._VerifyHasAPIInstance(prim, schemaInfo, "foo")
            else:
                self._VerifyNotHasAPIInstance(prim, schemaInfo, "foo")

            # Verify all calls to HasAPI "bar" return true iff we expect the
            # prim to have a "bar" instance of the schema.
            if "bar" in expectedHasInstanceNames:
                self._VerifyHasAPIInstance(prim, schemaInfo, "bar")
            else:
                self._VerifyNotHasAPIInstance(prim, schemaInfo, "bar")

        # Helper for verifying all inputs to HasAPIInFamily produce 
        # the expected return values given the instances for the given schema it
        # is expected to have
        def _VerifyHasAPIInFamilyInstances(
                prim, schemaInfo, versionPolicy, expectedHasInstanceNames):
            # Verify the return value for all calls to HasAPIInFamily
            # without a specified instance name. We expect these to return true
            # iff we expect the prim to have any instances of the schema.
            if expectedHasInstanceNames:
                self._VerifyHasAPIInFamily(
                    prim, schemaInfo, versionPolicy)
            else:
                self._VerifyNotHasAPIInFamily(
                    prim, schemaInfo, versionPolicy)
            
            # Verify all calls to HasAPIInFamily "foo" return true iff
            # we expect the# prim to have a "foo" instance of the schema.
            if "foo" in expectedHasInstanceNames:
                self._VerifyHasAPIInFamilyInstance(
                    prim, schemaInfo, versionPolicy, "foo")
            else:
                self._VerifyNotHasAPIInFamilyInstance(
                    prim, schemaInfo, versionPolicy, "foo")

            # Verify all calls to HasAPIInFamily "bar" return true iff
            # we expect the prim to have a "bar" instance of the schema.
            if "bar" in expectedHasInstanceNames:
                self._VerifyHasAPIInFamilyInstance(
                    prim, schemaInfo, versionPolicy, "bar")
            else:
                self._VerifyNotHasAPIInFamilyInstance(
                    prim, schemaInfo, versionPolicy, "bar")

        # Prim0 HasAPI version 0 of "TestVersionedMultiApplyAPI" with instance 
        # "foo" and version 2 of the same schema family with instance "bar". 
        # Verify HasAPI calls for these combinations of instance name and schema
        # version.
        _VerifyHasAPIInstances(primV0, expectedVersion0Info, ["foo"])
        _VerifyHasAPIInstances(primV0, expectedVersion1Info, [])
        _VerifyHasAPIInstances(primV0, expectedVersion2Info, ["bar"])

        # Verify prim0 "has any API version" with instance "foo" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 0
        self.assertTrue(primV0.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"))
        self.assertEqual(primV0.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"), 0)
        # Verify prim0 "has any API version" with instance "bar" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 2
        self.assertTrue(primV0.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"))
        self.assertEqual(primV0.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"), 2)
        # Verify prim0 "has any API version" with any instance in schema family 
        # "TestVersionedMultiApplyAPI". The "foo" and "bar" instances are 
        # different so the version for any instance is the latest version which
        # in this case is 2.
        self.assertTrue(primV0.HasAPIInFamily(
            "TestVersionedMultiApplyAPI"))
        self.assertEqual(primV0.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI"), 2)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion0Info, VersionPolicy.All, 
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion1Info, VersionPolicy.All,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion2Info, VersionPolicy.All,
            ["foo", "bar"])

        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThan, 
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThan, 
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThan, 
            [])

        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual, 
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual,
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual,
            ["bar"])

        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion0Info, VersionPolicy.LessThan,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion1Info, VersionPolicy.LessThan,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion2Info, VersionPolicy.LessThan,
            ["foo"])

        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion0Info, VersionPolicy.LessThanOrEqual,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion1Info, VersionPolicy.LessThanOrEqual,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV0, expectedVersion2Info, VersionPolicy.LessThanOrEqual,
            ["foo", "bar"])

        # Prim1 HasAPI version 1 of "TestVersionedMultiApplyAPI" with instance 
        # "foo" and instance "bar". Verify HasAPI calls for these combinations 
        # of instance name and schema version.
        _VerifyHasAPIInstances(primV1, expectedVersion0Info, [])
        _VerifyHasAPIInstances(primV1, expectedVersion1Info, ["foo", "bar"])
        _VerifyHasAPIInstances(primV1, expectedVersion2Info, [])

        # Verify prim1 "has any API version" with instance "foo" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 1
        self.assertTrue(primV1.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"))
        self.assertEqual(primV1.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"), 1)
        # Verify prim1 "has any API version" with instance "bar" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 1
        self.assertTrue(primV1.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"))
        self.assertEqual(primV1.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"), 1)
        # Verify prim1 "has any API version" with any instance in schema family 
        # "TestVersionedMultiApplyAPI". The "foo" and "bar" instances are 
        # the same so the version for any instance is the shared version 1.
        self.assertTrue(primV1.HasAPIInFamily(
            "TestVersionedMultiApplyAPI"))
        self.assertEqual(primV1.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI"), 1)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion0Info, VersionPolicy.All, 
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion1Info, VersionPolicy.All, 
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion2Info, VersionPolicy.All,
            ["foo", "bar"])

        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThan,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThan,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThan,
            [])

        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual,
            [])

        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion0Info, VersionPolicy.LessThan,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion1Info, VersionPolicy.LessThan,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion2Info, VersionPolicy.LessThan,
            ["foo", "bar"])

        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion0Info, VersionPolicy.LessThanOrEqual,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion1Info, VersionPolicy.LessThanOrEqual,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV1, expectedVersion2Info, VersionPolicy.LessThanOrEqual,
            ["foo", "bar"])

        # Prim2 HasAPI version 2 of "TestVersionedMultiApplyAPI" with instance 
        # "foo" and version 0 of the same schema family with instance "bar". 
        # Verify HasAPI calls for these combinations of instance name and schema
        # version.
        _VerifyHasAPIInstances(primV2, expectedVersion0Info, ["bar"])
        _VerifyHasAPIInstances(primV2, expectedVersion1Info, [])
        _VerifyHasAPIInstances(primV2, expectedVersion2Info, ["foo"])

        # Verify prim2 "has any API version" with instance "foo" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 2
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"))
        self.assertEqual(primV2.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "foo"), 2)
        # Verify prim2 "has any API version" with instance "bar" in schema
        # family "TestVersionedMultiApplyAPI" and that its version is returned
        # as 0
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"))
        self.assertEqual(primV2.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI", "bar"), 0)
        # Verify prim2 "has any API version" with any instance in schema family 
        # "TestVersionedMultiApplyAPI". The "foo" and "bar" instances are 
        # different so the version for any instance is the latest version which
        # in this case is 2.
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI"))
        self.assertEqual(primV2.GetVersionIfHasAPIInFamily(
            "TestVersionedMultiApplyAPI"), 2)

        # Verify the results of calling HasAPIInFamily with each 
        # version policy filter for each schema version.
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion0Info, VersionPolicy.All, ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion1Info, VersionPolicy.All, ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion2Info, VersionPolicy.All, ["foo", "bar"])

        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThan,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThan,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThan,
            [])

        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion0Info, VersionPolicy.GreaterThanOrEqual,
            ["foo", "bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion1Info, VersionPolicy.GreaterThanOrEqual,
            ["foo"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion2Info, VersionPolicy.GreaterThanOrEqual,
            ["foo"])

        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion0Info, VersionPolicy.LessThan,
            [])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion1Info, VersionPolicy.LessThan,
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion2Info, VersionPolicy.LessThan,
            ["bar"])

        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion0Info, VersionPolicy.LessThanOrEqual,
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion1Info, VersionPolicy.LessThanOrEqual,
            ["bar"])
        _VerifyHasAPIInFamilyInstances(
            primV2, expectedVersion2Info, VersionPolicy.LessThanOrEqual,
            ["foo", "bar"])

        # Verify calls to HasAPIInFamily with a version that doesn't 
        # itself exist but is from a valid schema family.

        # Version 3 of "TestVersionedMultiApplyAPI" does not exist and none of
        # the prims "HasAPI" TestVersionedMultiApplyAPI_3
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestVersionedMultiApplyAPI_3"))
        self.assertFalse(
            Usd.SchemaRegistry.FindSchemaInfo("TestVersionedMultiApplyAPI", 3))
        self.assertFalse(primV2.HasAPI("TestVersionedMultiApplyAPI", 3))
        self.assertFalse(primV2.HasAPI("TestVersionedMultiApplyAPI_3"))

        # However, "TestVersionedMultiApplyAPI_3" can still be used to query
        # about HasAPIInFamily; the prims have applied APIs that are
        # versions in the same schema family that are less than version 3
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", 3, VersionPolicy.LessThan))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", 3, VersionPolicy.GreaterThanOrEqual))
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI_3", VersionPolicy.LessThan))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI_3", VersionPolicy.GreaterThanOrEqual))

        # This also works when supplying HasAPIInFamily with a
        # specific instance name.
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", 3, VersionPolicy.LessThan, "foo"))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI", 3, VersionPolicy.GreaterThanOrEqual,
             "foo"))
        self.assertTrue(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI_3", VersionPolicy.LessThan, "foo"))
        self.assertFalse(primV2.HasAPIInFamily(
            "TestVersionedMultiApplyAPI_3", VersionPolicy.GreaterThanOrEqual,
             "foo"))

    def test_ApplyRemoveAPI(self):
        """Tests all Usd.Prim API for applying and removing API schema with 
           versioned API schemas"""        

        # The expected values from the SchemaInfo struct for each single-apply
        # schema type.
        expectedSingleApplyVersion0Info = {
            "type" : self.SingleApiVersion0Type,
            "identifier" : 'TestVersionedSingleApplyAPI',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 0
        }

        expectedSingleApplyVersion1Info = {
            "type" : self.SingleApiVersion1Type,
            "identifier" : 'TestVersionedSingleApplyAPI_1',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 1
        }

        expectedSingleApplyVersion2Info = {
            "type" : self.SingleApiVersion2Type,
            "identifier" : 'TestVersionedSingleApplyAPI_2',
            "kind" : Usd.SchemaKind.SingleApplyAPI,
            "family" : 'TestVersionedSingleApplyAPI',
            "version" : 2
        }

        # The expected values from the SchemaInfo struct for each multiple-apply
        # schema type.
        expectedMultiApplyVersion0Info = {
            "type" : self.MultiApiVersion0Type,
            "identifier" : 'TestVersionedMultiApplyAPI',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 0
        }

        expectedMultiApplyVersion1Info = {
            "type" : self.MultiApiVersion1Type,
            "identifier" : 'TestVersionedMultiApplyAPI_1',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 1
        }

        expectedMultiApplyVersion2Info = {
            "type" : self.MultiApiVersion2Type,
            "identifier" : 'TestVersionedMultiApplyAPI_2',
            "kind" : Usd.SchemaKind.MultipleApplyAPI,
            "family" : 'TestVersionedMultiApplyAPI',
            "version" : 2
        }

        # Verifies that a CanApplyAPI returns true for a prim, using all three 
        # input types, for the schema specified in schemaInfo.
        def _VerifyCanApplyAPI(prim, schemaInfo):
            # By TfType
            self.assertTrue(prim.CanApplyAPI(schemaInfo["type"]))
            # By Identifier
            self.assertTrue(prim.CanApplyAPI(schemaInfo["identifier"]))
            # By Family and Version
            self.assertTrue(prim.CanApplyAPI(schemaInfo["family"], 
                                             schemaInfo["version"]))

        # Verifies that a CanApplyAPI returns false for a prim, using all three 
        # input types, for the schema specified in schemaInfo.
        def _VerifyNotCanApplyAPI(prim, schemaInfo):
            # By TfType
            self.assertFalse(prim.CanApplyAPI(schemaInfo["type"]))
            # By Identifier
            self.assertFalse(prim.CanApplyAPI(schemaInfo["identifier"]))
            # By Family and Version
            self.assertFalse(prim.CanApplyAPI(schemaInfo["family"], 
                                              schemaInfo["version"]))

        # Verifies that a CanApplyAPI with the given instance name returns true
        # for a prim, using all three input types, for the schema specified in
        # schemaInfo.
        def _VerifyCanApplyAPIInstance(prim, schemaInfo, instanceName):
            # By TfType
            self.assertTrue(prim.CanApplyAPI(schemaInfo["type"], instanceName))
            # By Identifier
            self.assertTrue(prim.CanApplyAPI(schemaInfo["identifier"], instanceName))
            # By Family and Version
            self.assertTrue(prim.CanApplyAPI(schemaInfo["family"], 
                                             schemaInfo["version"], instanceName))

        # Verifies that a CanApplyAPI with the given instance name returns false
        # for a prim, using all three input types, for the schema specified in
        # schemaInfo.
        def _VerifyNotCanApplyAPIInstance(prim, schemaInfo, instanceName):
            # By TfType
            self.assertFalse(prim.CanApplyAPI(schemaInfo["type"], instanceName))
            # By Identifier
            self.assertFalse(prim.CanApplyAPI(schemaInfo["identifier"], instanceName))
            # By Family and Version
            self.assertFalse(prim.CanApplyAPI(schemaInfo["family"], 
                                              schemaInfo["version"], instanceName))

        # Verifies applying and removing a single-apply API schema by different
        # combinations of the available input types.
        def _VerifyApplyAndRemoveAPI(prim, schemaInfo):
            # The schema should start unapplied.
            self._VerifyNotHasAPI(prim, schemaInfo)

            # Verify Apply by type
            self.assertTrue(prim.ApplyAPI(schemaInfo["type"]))
            self._VerifyHasAPI(prim, schemaInfo)
            # Verify Remove by identifier
            self.assertTrue(prim.RemoveAPI(schemaInfo["identifier"]))
            self._VerifyNotHasAPI(prim, schemaInfo)

            # Verify Apply by identifier
            self.assertTrue(prim.ApplyAPI(schemaInfo["identifier"]))
            self._VerifyHasAPI(prim, schemaInfo)
            # Verify Remove by family and version
            self.assertTrue(prim.RemoveAPI(schemaInfo["family"], 
                                           schemaInfo["version"]))
            self._VerifyNotHasAPI(prim, schemaInfo)

            # Verify Apply by family and version
            self.assertTrue(prim.ApplyAPI(schemaInfo["family"], 
                                          schemaInfo["version"]))
            self._VerifyHasAPI(prim, schemaInfo)
            # Verify Remove by type
            self.assertTrue(prim.RemoveAPI(schemaInfo["type"]))
            self._VerifyNotHasAPI(prim, schemaInfo)

        # Verifies applying and removing a multiple-apply API schema instance by
        # different combinations of the available input types.
        def _VerifyApplyAndRemoveAPIInstance(prim, schemaInfo, instanceName):
            # The instacne of the schema should start unapplied.
            self._VerifyNotHasAPIInstance(prim, schemaInfo, instanceName)

            # Verify Apply by type
            self.assertTrue(prim.ApplyAPI(schemaInfo["type"], instanceName))
            self._VerifyHasAPIInstance(prim, schemaInfo, instanceName)
            # Verify Remove by identifier
            self.assertTrue(prim.RemoveAPI(schemaInfo["identifier"], instanceName))
            self._VerifyNotHasAPIInstance(prim, schemaInfo, instanceName)

            # Verify Apply by identifier
            self.assertTrue(prim.ApplyAPI(schemaInfo["identifier"], instanceName))
            self._VerifyHasAPIInstance(prim, schemaInfo, instanceName)
            # Verify Remove by family and version
            self.assertTrue(prim.RemoveAPI(schemaInfo["family"], 
                                           schemaInfo["version"], instanceName))
            self._VerifyNotHasAPIInstance(prim, schemaInfo, instanceName)

            # Verify Apply by family and version
            self.assertTrue(prim.ApplyAPI(schemaInfo["family"], 
                                          schemaInfo["version"], instanceName))
            self._VerifyHasAPIInstance(prim, schemaInfo, instanceName)
            # Verify Remove by type
            self.assertTrue(prim.RemoveAPI(schemaInfo["type"], instanceName))
            self._VerifyNotHasAPIInstance(prim, schemaInfo, instanceName)

        # Create a prim typed with each version of the typed schema. The type
        # affects the results of CanApplyAPI.
        stage = Usd.Stage.CreateInMemory()
        primV0 = stage.DefinePrim("/Prim", "TestBasicVersioned")
        primV1 = stage.DefinePrim("/Prim1", "TestBasicVersioned_1")
        primV2 = stage.DefinePrim("/Prim2", "TestBasicVersioned_2")

        # Each version of the single apply API has been designated as 
        # "can only apply to" the same version number of the test typed schema.
        # Verify the CanApplyAPI results for each API schema version.
        _VerifyCanApplyAPI(primV0, expectedSingleApplyVersion0Info)
        _VerifyNotCanApplyAPI(primV0, expectedSingleApplyVersion1Info)
        _VerifyNotCanApplyAPI(primV0, expectedSingleApplyVersion2Info)

        _VerifyNotCanApplyAPI(primV1, expectedSingleApplyVersion0Info)
        _VerifyCanApplyAPI(primV1, expectedSingleApplyVersion1Info)
        _VerifyNotCanApplyAPI(primV1, expectedSingleApplyVersion2Info)

        _VerifyNotCanApplyAPI(primV2, expectedSingleApplyVersion0Info)
        _VerifyNotCanApplyAPI(primV2, expectedSingleApplyVersion1Info)
        _VerifyCanApplyAPI(primV2, expectedSingleApplyVersion2Info)

        # Verify ApplyAPI and RemoveAPI for each version of the single apply API
        # schema. Note that CanApplyAPI results do not affect whether an API 
        # schema is successfully applied to prim through ApplyAPI.
        _VerifyApplyAndRemoveAPI(primV0, expectedSingleApplyVersion0Info)
        _VerifyApplyAndRemoveAPI(primV0, expectedSingleApplyVersion1Info)
        _VerifyApplyAndRemoveAPI(primV0, expectedSingleApplyVersion2Info)

        # Each version of the multiple apply API has been designated as 
        # "can only apply to" a subset of the test typed schema's versions as 
        # follows:
        #   API version 0 can only apply to typed schema version 0
        #   API version 1 can only apply to typed schema version 0 and 1
        #   API version 2 can only apply to typed schema version 1 and 2
        #
        # Verify the CanApplyAPI results for each API schema version.
        _VerifyCanApplyAPIInstance(
            primV0, expectedMultiApplyVersion0Info, "foo")
        _VerifyCanApplyAPIInstance(
            primV0, expectedMultiApplyVersion1Info, "foo")
        _VerifyNotCanApplyAPIInstance(
            primV0, expectedMultiApplyVersion2Info, "foo")

        _VerifyNotCanApplyAPIInstance(
            primV1, expectedMultiApplyVersion0Info, "foo")
        _VerifyCanApplyAPIInstance(
            primV1, expectedMultiApplyVersion1Info, "foo")
        _VerifyCanApplyAPIInstance(
            primV1, expectedMultiApplyVersion2Info, "foo")

        _VerifyNotCanApplyAPIInstance(
            primV2, expectedMultiApplyVersion0Info, "foo")
        _VerifyNotCanApplyAPIInstance(
            primV2, expectedMultiApplyVersion1Info, "foo")
        _VerifyCanApplyAPIInstance(
            primV2, expectedMultiApplyVersion2Info, "foo")

        # Verify ApplyAPI and RemoveAPI for each version of the multiple apply 
        # API schema. Note that CanApplyAPI results do not affect whether an API 
        # schema is successfully applied to prim through ApplyAPI.
        _VerifyApplyAndRemoveAPIInstance(
            primV0, expectedMultiApplyVersion0Info, "foo")
        _VerifyApplyAndRemoveAPIInstance(
            primV0, expectedMultiApplyVersion1Info, "foo")
        _VerifyApplyAndRemoveAPIInstance(
            primV0, expectedMultiApplyVersion2Info, "foo")

    def test_APISchemaVersionConflicts(self):
        """Tests the handling of API schema version conflicts in composed 
           API schema definitions"""        
        stage = Usd.Stage.CreateInMemory()

        def _MakeNewPrimPath() :
            _MakeNewPrimPath.primNum += 1
            return "/Prim" + str(_MakeNewPrimPath.primNum)
        _MakeNewPrimPath.primNum = 0

        # Define a prim with no type that we'll apply API schemas to. 
        prim = stage.DefinePrim(_MakeNewPrimPath())
        self.assertTrue(prim)
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])
        self.assertEqual(prim.GetAppliedSchemas(), [])

        # Apply version 1 of the single apply API. Authored and composed API
        # schemas will just have this API.
        self.assertTrue(prim.ApplyAPI("TestVersionedSingleApplyAPI_1"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_1"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr1"])

        # Now apply version 0 and version 2 of the single apply API 
        # All three versions will be in the authored API schemas.
        # But the composed API schemas will only have version 1 due as we only
        # allow one version of a schema from a family to be coposed into a prim.
        self.assertTrue(prim.ApplyAPI("TestVersionedSingleApplyAPI"))
        self.assertTrue(prim.ApplyAPI("TestVersionedSingleApplyAPI_2"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_1", 
             "TestVersionedSingleApplyAPI",
             "TestVersionedSingleApplyAPI_2"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr1"])

        # Now apply version 0 and version 2 of the multiple apply API with 
        # different instance names.
        # These both are added to the authored and composed API schemas. Even
        # though they are different versions of the same family, the differing
        # instance names cause this to not be a version conflict.
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI", "foo"))
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI_2", "bar"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_1", 
             "TestVersionedSingleApplyAPI",
             "TestVersionedSingleApplyAPI_2",
             "TestVersionedMultiApplyAPI:foo",
             "TestVersionedMultiApplyAPI_2:bar"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             "TestVersionedMultiApplyAPI:foo",
             "TestVersionedMultiApplyAPI_2:bar"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:bar:m_attr2",
             "multi:foo:m_attr",
             "s_attr1"])

        # Now apply version 1 of the multiple apply API with the same instance 
        # names as above.
        # These will be added to the authored API schemas but will be skipped
        # in the composed API schemas as they conflict with versions of the 
        # schema family applied with the same instance names.
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI_1", "foo"))
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI_1", "bar"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_1", 
             "TestVersionedSingleApplyAPI",
             "TestVersionedSingleApplyAPI_2",
             "TestVersionedMultiApplyAPI:foo",
             "TestVersionedMultiApplyAPI_2:bar",
             "TestVersionedMultiApplyAPI_1:foo",
             "TestVersionedMultiApplyAPI_1:bar"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             "TestVersionedMultiApplyAPI:foo",
             "TestVersionedMultiApplyAPI_2:bar"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:bar:m_attr2",
             "multi:foo:m_attr",
             "s_attr1"])

        # Define a new prim with a type that already has built-in API schemas.
        # The composed built-in schemas for this prim are version 1 of the
        # single apply API schema and version 2 of the multi apply schema with
        # the instance "foo".
        prim = stage.DefinePrim(_MakeNewPrimPath(), "TestPrimWithAPIBuiltins")
        self.assertTrue(prim)
        self.assertTrue(prim.IsA("TestPrimWithAPIBuiltins"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             "TestVersionedMultiApplyAPI_2:foo"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:foo:m_attr2",
             "s_attr1"])
        
        # Apply version 2 of the single apply API schema and version 0 of the 
        # multi apply schema with the instance "foo".
        # These schemas will show up in the authored API schemas but will not
        # be added to the composed API schemas as their versions conflict with
        # the existing built-in API schema from the prim type.
        self.assertTrue(prim.ApplyAPI("TestVersionedSingleApplyAPI_2"))
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI", "foo"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_2",
             "TestVersionedMultiApplyAPI:foo"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             "TestVersionedMultiApplyAPI_2:foo"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:foo:m_attr2",
             "s_attr1"])

        # Now apply version 0 of the multi apply schema with instance "bar". 
        # This schema will show up in both the authored and composed API schemas
        # because the instance name is different than the built-in schema's 
        # instance name so it is not considered a version conflict.
        self.assertTrue(prim.ApplyAPI("TestVersionedMultiApplyAPI", "bar"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestVersionedSingleApplyAPI_2",
             "TestVersionedMultiApplyAPI:foo",
             "TestVersionedMultiApplyAPI:bar"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             "TestVersionedMultiApplyAPI_2:foo",
             "TestVersionedMultiApplyAPI:bar"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:bar:m_attr",
             "multi:foo:m_attr2",
             "s_attr1"])

        # Define another new prim with no type.
        prim = stage.DefinePrim(_MakeNewPrimPath())
        self.assertTrue(prim)
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])
        self.assertEqual(prim.GetAppliedSchemas(), [])

        # Apply TestAPIWithAPIBuiltins1_API which includes others of our 
        # versioned schemas as built-ins. 
        self.assertTrue(prim.ApplyAPI("TestAPIWithAPIBuiltins1_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestAPIWithAPIBuiltins1_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestAPIWithAPIBuiltins1_API",
                "TestVersionedSingleApplyAPI_1",
                "TestVersionedMultiApplyAPI:foo"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr1",
             "multi:foo:m_attr",
             "s_attr1"])

        # Now apply TestAPIWithAPIBuiltins2_API which also includes other
        # versioned built-in API schemas.
        # One of these included schemas is a version conflict with the schemas
        # included by TestAPIWithAPIBuiltins1_API so ALL schemas that would be
        # included by TestAPIWithAPIBuiltins2_API are skipped in the composed
        # API schemas.
        self.assertTrue(prim.ApplyAPI("TestAPIWithAPIBuiltins2_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestAPIWithAPIBuiltins1_API",
             "TestAPIWithAPIBuiltins2_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestAPIWithAPIBuiltins1_API",
                "TestVersionedSingleApplyAPI_1",
                "TestVersionedMultiApplyAPI:foo"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr1",
             "multi:foo:m_attr",
             "s_attr1"])

        # Now apply TestAPIWithAPIBuiltins3_API which also includes other
        # versioned built-in API schemas.
        # At least one of these new schemas would have version conflict with the
        # schemas from TestAPIWithAPIBuiltins2_API, but since 
        # TestAPIWithAPIBuiltins2_API is excluded because of its own conflicts 
        # (and there are no conflicts with TestAPIWithAPIBuiltins1_API's 
        # schemas), the schemas from TestAPIWithAPIBuiltins3_API are composed in.
        self.assertTrue(prim.ApplyAPI("TestAPIWithAPIBuiltins3_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestAPIWithAPIBuiltins1_API",
             "TestAPIWithAPIBuiltins2_API",
             "TestAPIWithAPIBuiltins3_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestAPIWithAPIBuiltins1_API",
                "TestVersionedSingleApplyAPI_1",
                "TestVersionedMultiApplyAPI:foo",
             "TestAPIWithAPIBuiltins3_API",
                "TestVersionedMultiApplyAPI_2:bar",
                "TestVersionedMultiApplyAPI_2:baz"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr1",
             "b_attr3",
             "multi:bar:m_attr2",
             "multi:baz:m_attr2",
             "multi:foo:m_attr",
             "s_attr1"])

        # Now remove TestAPIWithAPIBuiltins1_API
        # This removes the version conflict for the schemas from 
        # TestAPIWithAPIBuiltins2_API so now those do appear in the composed
        # API schemas. However, now that TestAPIWithAPIBuiltins2_API's schemas
        # are included, one of the schemas from TestAPIWithAPIBuiltins3_API 
        # causes a version conflict so all of TestAPIWithAPIBuiltins3_API's
        # schemas are excluded from the composed API schemas.
        self.assertTrue(prim.RemoveAPI("TestAPIWithAPIBuiltins1_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestAPIWithAPIBuiltins2_API",
             "TestAPIWithAPIBuiltins3_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestAPIWithAPIBuiltins2_API",
                "TestVersionedMultiApplyAPI:bar",
                "TestVersionedSingleApplyAPI_2"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr2",
             "multi:bar:m_attr",
             "s_attr2"])

        # Now remove TestAPIWithAPIBuiltins2_API.
        # This removes the version conflict for the schemas from 
        # TestAPIWithAPIBuiltins3_API so now those do appear in the composed
        # API schemas again.
        self.assertTrue(prim.RemoveAPI("TestAPIWithAPIBuiltins2_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestAPIWithAPIBuiltins3_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestAPIWithAPIBuiltins3_API",
                "TestVersionedMultiApplyAPI_2:bar",
                "TestVersionedMultiApplyAPI_2:baz"])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr3",
             "multi:bar:m_attr2",
             "multi:baz:m_attr2"])

        # All of the following are examples of typed and API schema definitions
        # that are defined with internal version conflicts among their 
        # built-ins. These cases verify that conflicting API schemas are skipped
        # in these definitions.
        prim = stage.DefinePrim(
            _MakeNewPrimPath(), "TestPrimWithAPIVersionConflict1")
        self.assertTrue(prim)
        self.assertTrue(prim.IsA("TestPrimWithAPIVersionConflict1"))
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI",
             # Conflict: "TestVersionedSingleApplyAPI_1",
             # Conflict: "TestVersionedSingleApplyAPI_2",
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr"])

        prim = stage.DefinePrim(
            _MakeNewPrimPath(), "TestPrimWithAPIVersionConflict2")
        self.assertTrue(prim)
        self.assertTrue(prim.IsA("TestPrimWithAPIVersionConflict2"))
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestVersionedSingleApplyAPI_1",
             # Conflict: "TestVersionedSingleApplyAPI",
             # Conflict: "TestVersionedSingleApplyAPI_2",
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr1"])

        prim = stage.DefinePrim(_MakeNewPrimPath(), "")
        self.assertTrue(prim)
        self.assertTrue(
            prim.ApplyAPI("TestSingleApplyWithAPIVersionConflict1_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestSingleApplyWithAPIVersionConflict1_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestSingleApplyWithAPIVersionConflict1_API",
                "TestVersionedSingleApplyAPI",
                # Conflict: "TestVersionedSingleApplyAPI_1",
                # Conflict: "TestVersionedSingleApplyAPI_2",
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr"])

        prim = stage.DefinePrim(_MakeNewPrimPath(), "")
        self.assertTrue(prim)
        self.assertTrue(
            prim.ApplyAPI("TestSingleApplyWithAPIVersionConflict2_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestSingleApplyWithAPIVersionConflict2_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestSingleApplyWithAPIVersionConflict2_API",
                "TestVersionedSingleApplyAPI_1",
                # Conflict: "TestVersionedSingleApplyAPI",
                # Conflict: "TestVersionedSingleApplyAPI_2",
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["s_attr1"])

        prim = stage.DefinePrim(_MakeNewPrimPath(), "")
        self.assertTrue(prim)
        self.assertTrue(
            prim.ApplyAPI("TestSingleApplyWithAPIVersionConflict3_API"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestSingleApplyWithAPIVersionConflict3_API"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestSingleApplyWithAPIVersionConflict3_API",
                "TestAPIWithAPIBuiltins1_API",
                    "TestVersionedSingleApplyAPI_1",
                    "TestVersionedMultiApplyAPI:foo",
                # Conflict: "TestAPIWithAPIBuiltins2_API",
                    # Conflict: "TestVersionedMultiApplyAPI:bar",
                    # Conflict: "TestVersionedSingleApplyAPI_2"
                "TestAPIWithAPIBuiltins3_API",
                    "TestVersionedMultiApplyAPI_2:bar",
                    "TestVersionedMultiApplyAPI_2:baz"
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["b_attr1",
             "b_attr3",
             "multi:bar:m_attr2",
             "multi:baz:m_attr2",
             "multi:foo:m_attr",
             "s_attr1"])

        prim = stage.DefinePrim(_MakeNewPrimPath(), "")
        self.assertTrue(prim)
        self.assertTrue(
            prim.ApplyAPI("TestMultiApplyWithAPIVersionConflict_API", "bar"))
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestMultiApplyWithAPIVersionConflict_API:bar"])
        self.assertEqual(prim.GetAppliedSchemas(), 
            ["TestMultiApplyWithAPIVersionConflict_API:bar",
                "TestVersionedMultiApplyAPI_1:bar", 
                # Conflict: "TestVersionedMultiApplyAPI_2:bar", 
                "TestVersionedMultiApplyAPI_2:bar:foo",
                # Conflict: "TestVersionedMultiApplyAPI:bar:foo"
            ])
        self.assertEqual(prim.GetPropertyNames(), 
            ["multi:bar:foo:m_attr2",
             "multi:bar:m_attr1"])

    def test_AutoApplyAPI(self):
        """Tests versioning behavior with auto-apply API schemas"""        

        # Verify the auto-apply API schemas setup for this test.
        # We have three versions of the same schema family:
        #   Version 0 - TestVersionedAutoApplyAPI
        #   Version 2 - TestVersionedAutoApplyAPI_2
        #   Version 10 - TestVersionedAutoApplyAPI_10
        #
        # These versions were chosen as the version order, 10 > 2 > 0, is 
        # different than the lexicographical order of the suffixes, 
        # "_2" > "_10" > "".
        # 
        # All three versions of the schema are set up to auto apply to 
        # version 1 of the BasicVersioned typed schema. Only version 0 
        # of the auto-apply schema is setup to auto-apply to version 0
        # of the BasicVersioned typed schema.
        autoApplySchemas = Usd.SchemaRegistry.GetAutoApplyAPISchemas()
        self.assertEqual(autoApplySchemas["TestVersionedAutoApplyAPI"],
            ["TestBasicVersioned", "TestBasicVersioned_1"])
        self.assertEqual(autoApplySchemas["TestVersionedAutoApplyAPI_2"],
            ["TestBasicVersioned_1"])
        self.assertEqual(autoApplySchemas["TestVersionedAutoApplyAPI_10"],
            ["TestBasicVersioned_1"])

        # Create a prim typed with each version of the basic IsA schema.
        stage = Usd.Stage.CreateInMemory()
        primV0 = stage.DefinePrim("/Prim", "TestBasicVersioned")
        primV1 = stage.DefinePrim("/Prim1", "TestBasicVersioned_1")
        primV2 = stage.DefinePrim("/Prim2", "TestBasicVersioned_2")

        self.assertTrue(primV0)
        self.assertTrue(primV1)
        self.assertTrue(primV2)

        self.assertTrue(primV0.IsA(self.BasicVersion0Type))
        self.assertTrue(primV1.IsA(self.BasicVersion1Type))
        self.assertTrue(primV2.IsA(self.BasicVersion2Type))

        # TestBasicVersioned has only version 0 of TestVersionedAutoApplyAPI
        # auto-applied, so its the only API schema that applied to it.
        self.assertEqual(primV0.GetAppliedSchemas(), 
            ["TestVersionedAutoApplyAPI"])
        self.assertEqual(primV0.GetPropertyNames(), 
            ["a_attr"])

        # TestBasicVersioned_1 has all three versions of 
        # TestVersionedAutoApplyAPI applied. In this case only the latest 
        # version of the schema family is applied which is version 10. 
        self.assertEqual(primV1.GetAppliedSchemas(), 
            ["TestVersionedAutoApplyAPI_10"])
        self.assertEqual(primV1.GetPropertyNames(), 
            ["a_attr10"])

        # TestBasicVersioned_2 has none of the versions of 
        # TestVersionedAutoApplyAPI explicitly applied. This case proves that
        # the auto-applied schemas of the earlier versions of TestBasicVersioned
        # do not automatically propagate to this later version.
        self.assertEqual(primV2.GetAppliedSchemas(), 
            [])
        self.assertEqual(primV2.GetPropertyNames(), 
            [])


if __name__ == "__main__":
    unittest.main()
