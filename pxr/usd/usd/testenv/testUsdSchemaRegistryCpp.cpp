//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/schemaRegistry.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/arch/systemInfo.h"
PXR_NAMESPACE_USING_DIRECTIVE

static const int SCHEMA_BASE_INIT = 1971;
static const int TEST_BASE_INIT = 44;
static const int TEST_DERIVED_INIT = 42;
static const int MUTATED_VAL = 22;

// These test cases are not meant to be full coverage of the UsdPrimDefinition 
// API for prim and property data access. Instead they focus on the C++ specific
// functionality like the templated accessors. The python testUsdSchemaRegistry
// test handles the rest of the code coverage. 

static void
TestPrimMetadata()
{
    const UsdPrimDefinition *primDef = 
        UsdSchemaRegistry::GetInstance().FindConcretePrimDefinition(
            TfToken("MetadataTest"));
    TF_AXIOM(primDef);

    // Get various prim metadata fields from the templated GetMetadata on the 
    // prim definition.
    TfToken typeName;
    TF_AXIOM(primDef->GetMetadata(SdfFieldKeys->TypeName, &typeName));
    TF_AXIOM(typeName == TfToken("MetadataTest"));

    std::string doc;
    TF_AXIOM(primDef->GetMetadata(SdfFieldKeys->Documentation, &doc));
    TF_AXIOM(doc == "Testing documentation metadata");

    bool hidden;
    TF_AXIOM(primDef->GetMetadata(SdfFieldKeys->Hidden, &hidden));
    TF_AXIOM(hidden == true);

    std::string testCustomMetadata;
    TF_AXIOM(primDef->GetMetadata(TfToken("testCustomMetadata"), 
                &testCustomMetadata));
    TF_AXIOM(testCustomMetadata == "garply");

    // Dictionary metadata can be gotten by whole value as well as queried
    // for individual keys in the metadata value.
    VtDictionary testDictionaryMetadata;
    TF_AXIOM(primDef->GetMetadata(TfToken("testDictionaryMetadata"), 
             &testDictionaryMetadata));
    TF_AXIOM(testDictionaryMetadata == VtDictionary(
        {{"name", VtValue("foo")},
         {"value", VtValue(2)}}));

    std::string testDictionaryMetadataName;
    TF_AXIOM(primDef->GetMetadataByDictKey(
        TfToken("testDictionaryMetadata"), TfToken("name"),
        &testDictionaryMetadataName));
    TF_AXIOM(testDictionaryMetadataName == "foo");

    int testDictionaryMetadataValue;
    TF_AXIOM(primDef->GetMetadataByDictKey(
        TfToken("testDictionaryMetadata"), TfToken("value"),
        &testDictionaryMetadataValue));
    TF_AXIOM(testDictionaryMetadataValue == 2);

    // Verify getting existing values by the wrong type returns false and 
    // doesn't write to the output value.
    double val = 0.0;
    TF_AXIOM(!primDef->GetMetadata(SdfFieldKeys->Hidden, &val));
    TF_AXIOM(val == 0.0);

    // XXX: It's reasonable to expect that calling GetMetadataDictKey using
    // an output type that doesn't match the type of the value at the key in the 
    // dictionary would return false. However SdfLayer::HasFieldDictKey (which
    // GetMetadataDictKey eventually calls) does not return false on a type
    // mismatch. This may be a bug or it may be intentional so for now at least,
    // this case will return true, but won't write out to the value.
    TF_AXIOM(primDef->GetMetadataByDictKey(
             TfToken("testDictionaryMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    // But calling GetMetaByDictKey on non-dicionary metadata or a non-existent
    // key in valid dictionary metadata does return false.
    TF_AXIOM(!primDef->GetMetadataByDictKey(
             TfToken("testCustomMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!primDef->GetMetadataByDictKey(
             TfToken("testDictionaryMetadata"), TfToken("bogus"), &val));
    TF_AXIOM(val == 0.0);
}

static void
TestAttributeMetadata()
{
    // Get prim definition for our test schema.
    const UsdPrimDefinition *primDef = 
        UsdSchemaRegistry::GetInstance().FindConcretePrimDefinition(
            TfToken("MetadataTest"));
    TF_AXIOM(primDef);

    // Get the valid test attribute definition from the prim definition
    const TfToken attrNameToken("testAttr");
    UsdPrimDefinition::Attribute attrDef = 
        primDef->GetAttributeDefinition(attrNameToken);

    // Valid attribute conversion to bool
    TF_AXIOM(attrDef);
    TF_AXIOM(attrDef.IsAttribute());

    // Can also be gotten as a valid property or downcast to property.
    TF_AXIOM(primDef->GetPropertyDefinition(attrNameToken));
    TF_AXIOM(UsdPrimDefinition::Property(attrDef));

    // It is not a relationship so will bool convert to false if gotten as a
    // relationship or converted to a relationship.
    TF_AXIOM(!attrDef.IsRelationship());
    TF_AXIOM(!primDef->GetRelationshipDefinition(attrNameToken));
    TF_AXIOM(!UsdPrimDefinition::Relationship(attrDef));

    // Get type name metadata for the attribute through attr def and prim def
    // templated accessors. 
    {
        TfToken typeName;
        TF_AXIOM(attrDef.GetMetadata(SdfFieldKeys->TypeName, &typeName));
        TF_AXIOM(typeName == "string");
    }
    {
        TfToken typeName;
        TF_AXIOM(primDef->GetPropertyMetadata(
            attrNameToken, SdfFieldKeys->TypeName, &typeName));
        TF_AXIOM(typeName == "string");
    }
    // Verify the type name accessors on attr def (not templated)
    TF_AXIOM(attrDef.GetTypeName() == SdfValueTypeNames->String);
    TF_AXIOM(attrDef.GetTypeNameToken() == "string");

    // Test the multiple templated accessors for getting the fallback value
    // for an attribute definition.
    {
        std::string fallback;
        TF_AXIOM(attrDef.GetMetadata(SdfFieldKeys->Default, &fallback));
        TF_AXIOM(fallback == "foo");
    }
    {
        std::string fallback;
        TF_AXIOM(primDef->GetPropertyMetadata(
            attrNameToken, SdfFieldKeys->Default, &fallback));
        TF_AXIOM(fallback == "foo");
    }
    {
        std::string fallback;
        TF_AXIOM(attrDef.GetFallbackValue(&fallback));
        TF_AXIOM(fallback == "foo");
    }
    {
        std::string fallback;
        TF_AXIOM(primDef->GetAttributeFallbackValue(attrNameToken, &fallback));
        TF_AXIOM(fallback == "foo");
    }

    // Get allowed tokens metadata for the attribute through attr def and prim 
    // def templated accessors. 
    {
        VtTokenArray allowTokens;
        TF_AXIOM(attrDef.GetMetadata(SdfFieldKeys->AllowedTokens, &allowTokens));
        TF_AXIOM(allowTokens == VtTokenArray({TfToken("bar"), TfToken("baz")}));
    }
    {
        VtTokenArray allowTokens;
        TF_AXIOM(primDef->GetPropertyMetadata(
            attrNameToken, SdfFieldKeys->AllowedTokens, &allowTokens));
        TF_AXIOM(allowTokens == VtTokenArray({TfToken("bar"), TfToken("baz")}));
    }

    // Dictionary metadata can be gotten by whole value as well as queried
    // for individual keys in the metadata value.
    {
        VtDictionary testDictionaryMetadata;
        TF_AXIOM(attrDef.GetMetadata(
            TfToken("testDictionaryMetadata"), 
            &testDictionaryMetadata));
        TF_AXIOM(testDictionaryMetadata == VtDictionary(
            {{"name", VtValue("bar")},
            {"value", VtValue(3)}}));

        std::string testDictionaryMetadataName;
        TF_AXIOM(attrDef.GetMetadataByDictKey(
            TfToken("testDictionaryMetadata"), TfToken("name"),
            &testDictionaryMetadataName));
        TF_AXIOM(testDictionaryMetadataName == "bar");

        int testDictionaryMetadataValue;
        TF_AXIOM(attrDef.GetMetadataByDictKey(
            TfToken("testDictionaryMetadata"), TfToken("value"),
            &testDictionaryMetadataValue));
        TF_AXIOM(testDictionaryMetadataValue == 3);
    }
    {
        VtDictionary testDictionaryMetadata;
        TF_AXIOM(primDef->GetPropertyMetadata(attrNameToken,
            TfToken("testDictionaryMetadata"), 
            &testDictionaryMetadata));
        TF_AXIOM(testDictionaryMetadata == VtDictionary(
            {{"name", VtValue("bar")},
            {"value", VtValue(3)}}));

        std::string testDictionaryMetadataName;
        TF_AXIOM(primDef->GetPropertyMetadataByDictKey(attrNameToken,
            TfToken("testDictionaryMetadata"), TfToken("name"),
            &testDictionaryMetadataName));
        TF_AXIOM(testDictionaryMetadataName == "bar");

        int testDictionaryMetadataValue;
        TF_AXIOM(primDef->GetPropertyMetadataByDictKey(attrNameToken,
            TfToken("testDictionaryMetadata"), TfToken("value"),
            &testDictionaryMetadataValue));
        TF_AXIOM(testDictionaryMetadataValue == 3);
    }

    // Verify getting existing values by the wrong type returns false and 
    // doesn't write to the output value.
    double val = 0.0;
    TF_AXIOM(!attrDef.GetFallbackValue(&val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!primDef->GetAttributeFallbackValue(attrNameToken, &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!attrDef.GetMetadata(SdfFieldKeys->AllowedTokens, &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!primDef->GetPropertyMetadata(
        attrNameToken, SdfFieldKeys->AllowedTokens, &val));
    TF_AXIOM(val == 0.0);

    // XXX: It's reasonable to expect that calling GetMetadataDictKey using
    // an output type that doesn't match the type of the value at the key in the 
    // dictionary would return false. However SdfLayer::HasFieldDictKey (which
    // GetMetadataDictKey eventually calls) does not return false on a type
    // mismatch. This may be a bug or it may be intentional so for now at least,
    // this case will return true, but won't write out to the value.
    TF_AXIOM(attrDef.GetMetadataByDictKey(
             TfToken("testDictionaryMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(primDef->GetPropertyMetadataByDictKey(attrNameToken,
             TfToken("testDictionaryMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    // But calling GetMetaByDictKey on non-dicionary metadata or a non-existent
    // key in valid dictionary metadata does return false.
    TF_AXIOM(!attrDef.GetMetadataByDictKey(
             TfToken("testCustomMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!primDef->GetPropertyMetadataByDictKey(attrNameToken,
             TfToken("testCustomMetadata"), TfToken("name"), &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!attrDef.GetMetadataByDictKey(
             TfToken("testDictionaryMetadata"), TfToken("bogus"), &val));
    TF_AXIOM(val == 0.0);
    TF_AXIOM(!primDef->GetPropertyMetadataByDictKey(attrNameToken,
             TfToken("testDictionaryMetadata"), TfToken("bogus"), &val));
    TF_AXIOM(val == 0.0);
}

static void
TestRelationshipMetadata()
{
    // Get prim definition for our test schema.
    const UsdPrimDefinition *primDef = 
        UsdSchemaRegistry::GetInstance().FindConcretePrimDefinition(
            TfToken("MetadataTest"));
    TF_AXIOM(primDef);

    // Get the valid test relationship definition from the prim definition
    const TfToken relNameToken("testRel");
    UsdPrimDefinition::Relationship relDef = 
        primDef->GetRelationshipDefinition(relNameToken);

    // Valid relationship conversion to bool
    TF_AXIOM(relDef);
    TF_AXIOM(relDef.IsRelationship());

    // Can also be gotten as a valid property or downcast to property.
    TF_AXIOM(primDef->GetPropertyDefinition(relNameToken));
    TF_AXIOM(UsdPrimDefinition::Property(relDef));

    // It is not an attribute so will bool convert to false if gotten as an
    // attribute or converted to an attribute.
    TF_AXIOM(!relDef.IsAttribute());
    TF_AXIOM(!primDef->GetAttributeDefinition(relNameToken));
    TF_AXIOM(!UsdPrimDefinition::Attribute(relDef));

    // Get variability metadata for the relationship through attr def and prim 
    // def templated accessors. 
    {
        SdfVariability variability;
        TF_AXIOM(relDef.GetMetadata(SdfFieldKeys->Variability, &variability));
        TF_AXIOM(variability == SdfVariabilityUniform);
    }
    {
        SdfVariability variability;
        TF_AXIOM(primDef->GetPropertyMetadata(
            relNameToken, SdfFieldKeys->Variability, &variability));
        TF_AXIOM(variability == SdfVariabilityUniform);
    }
    // Verify the variability accessor on rel def (not templated)
    TF_AXIOM(relDef.GetVariability() == SdfVariabilityUniform);

    // Dictionary metadata can be gotten by whole value as well as queried
    // for individual keys in the metadata value.
    {
        VtDictionary testDictionaryMetadata;
        TF_AXIOM(relDef.GetMetadata(
            TfToken("testDictionaryMetadata"), 
            &testDictionaryMetadata));
        TF_AXIOM(testDictionaryMetadata == VtDictionary(
            {{"name", VtValue("baz")},
            {"value", VtValue(5)}}));

        std::string testDictionaryMetadataName;
        TF_AXIOM(relDef.GetMetadataByDictKey(
            TfToken("testDictionaryMetadata"), TfToken("name"),
            &testDictionaryMetadataName));
        TF_AXIOM(testDictionaryMetadataName == "baz");

        int testDictionaryMetadataValue;
        TF_AXIOM(relDef.GetMetadataByDictKey(
            TfToken("testDictionaryMetadata"), TfToken("value"),
            &testDictionaryMetadataValue));
        TF_AXIOM(testDictionaryMetadataValue == 5);
    }
    {
        VtDictionary testDictionaryMetadata;
        TF_AXIOM(primDef->GetPropertyMetadata(relNameToken,
            TfToken("testDictionaryMetadata"), 
            &testDictionaryMetadata));
        TF_AXIOM(testDictionaryMetadata == VtDictionary(
            {{"name", VtValue("baz")},
            {"value", VtValue(5)}}));

        std::string testDictionaryMetadataName;
        TF_AXIOM(primDef->GetPropertyMetadataByDictKey(relNameToken,
            TfToken("testDictionaryMetadata"), TfToken("name"),
            &testDictionaryMetadataName));
        TF_AXIOM(testDictionaryMetadataName == "baz");

        int testDictionaryMetadataValue;
        TF_AXIOM(primDef->GetPropertyMetadataByDictKey(relNameToken,
            TfToken("testDictionaryMetadata"), TfToken("value"),
            &testDictionaryMetadataValue));
        TF_AXIOM(testDictionaryMetadataValue == 5);
    }
}

static void
TestInvalidProperties()
{
    // Get prim definition for our test schema.
    const UsdPrimDefinition *primDef = 
        UsdSchemaRegistry::GetInstance().FindConcretePrimDefinition(
            TfToken("MetadataTest"));
    TF_AXIOM(primDef);

    // Default constructed property, attribute, relationship.
    // All are invalid and have an empty name.
    // We can still query IsAttribute, IsRelationship, and GetName on invalid
    // properties. Any other queries are not allowed on invalid prims and will
    // likely result in a crash.
    UsdPrimDefinition::Property prop;
    TF_AXIOM(!prop);
    TF_AXIOM(!prop.IsAttribute());
    TF_AXIOM(!prop.IsRelationship());
    TF_AXIOM(prop.GetName().IsEmpty());
    UsdPrimDefinition::Attribute attr;
    TF_AXIOM(!attr);
    TF_AXIOM(!attr.IsAttribute());
    TF_AXIOM(!attr.IsRelationship());
    TF_AXIOM(attr.GetName().IsEmpty());
    UsdPrimDefinition::Relationship rel;
    TF_AXIOM(!rel);
    TF_AXIOM(!rel.IsAttribute());
    TF_AXIOM(!rel.IsRelationship());
    TF_AXIOM(rel.GetName().IsEmpty());

    // Get a property that doesn't exist as each property definition type.
    // The property definitions will all be invalid, but will contain the name
    // of the requested property.
    prop = primDef->GetPropertyDefinition(TfToken("bogus"));
    TF_AXIOM(!prop);
    TF_AXIOM(!prop.IsAttribute());
    TF_AXIOM(!prop.IsRelationship());
    TF_AXIOM(prop.GetName() == "bogus");
    attr = primDef->GetAttributeDefinition(TfToken("bogus"));
    TF_AXIOM(!attr);
    TF_AXIOM(!attr.IsAttribute());
    TF_AXIOM(!attr.IsRelationship());
    TF_AXIOM(attr.GetName() == "bogus");
    rel = primDef->GetRelationshipDefinition(TfToken("bogus"));
    TF_AXIOM(!rel);
    TF_AXIOM(!rel.IsAttribute());
    TF_AXIOM(!rel.IsRelationship());
    TF_AXIOM(rel.GetName() == "bogus");
}

int main(int argc, char** argv)
{
    const std::string testDir = ArchGetCwd() + "/resources";
    printf ("%s", testDir.c_str());
    TF_AXIOM(!PlugRegistry::GetInstance().RegisterPlugins(testDir).empty());

    TestPrimMetadata();
    TestAttributeMetadata();
    TestRelationshipMetadata();
    TestInvalidProperties();
    
    printf("Passed!\n");
    
    return EXIT_SUCCESS;
}
