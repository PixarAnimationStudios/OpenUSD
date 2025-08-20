//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/sceneAdapter.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/attributeQuery.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/exec/esf/relationship.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/scope.h"

#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        std::cout << std::flush;                                        \
        std::cerr << std::flush;                                        \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
    }()

namespace
{

struct Fixture
{
    SdfLayerRefPtr layer;
    UsdStageConstRefPtr stage;
    EsfJournal * const journal = nullptr;

    Fixture()
    {
        layer = SdfLayer::CreateAnonymous(".usda");
        const bool importedLayer = layer->ImportFromString(R"usd(
            #usda 1.0
            def Scope "Prim1" (
                prepend apiSchemas = ["CollectionAPI:collection1"]
                doc = "prim doc"
            ) {
                int attr1 (doc = "attr doc")
                int attr1 = 1
                int attr1.connect = [</Prim1.ns1:ns2:attr2>, </Prim1.attr3>]
                int ns1:ns2:attr2 (doc = "prop doc")
                int ns1:ns2:attr2 = 2
                double attr3.spline = {
                    1: 0,
                    2: 1,
                }
                rel rel1 (doc = "rel doc")
                rel rel1 = [</Prim1.attr1>, </Prim1.rel2>]
                rel rel2 = [</Prim2>, </Prim3>]
            }
            def Scope "Prim2" (
                prepend apiSchemas = ["CollectionAPI:collection1"]
            ) {
            }
            def Scope "Prim3" (
                prepend apiSchemas = ["CollectionAPI:collection2"]
            ) {
            }
            )usd");
        TF_AXIOM(importedLayer);

        stage = UsdStage::Open(layer);
        TF_AXIOM(stage);
    }
};

} // anonymous namespace

template <typename ObjectType>
static void
_TestMetadata(
    Fixture &fixture,
    const ObjectType &object,
    const std::string &expectedValue)
{
    TF_AXIOM(
        object->IsValidMetadataKey(SdfFieldKeys->Documentation));
    TF_AXIOM(
        !object->IsValidMetadataKey(TfToken("bogusMetadataKey")));
    ASSERT_EQ(
        object->GetMetadataValueType(SdfFieldKeys->Documentation),
        TfType::Find<std::string>());
    ASSERT_EQ(
        object->GetMetadata(SdfFieldKeys->Documentation),
        expectedValue);
}

// Tests that EsfUsd_Stage behaves as UsdStage.
static void
TestStage(Fixture &fixture)
{
    const EsfStage stage = EsfUsdSceneAdapter::AdaptStage(fixture.stage);

    const EsfPrim prim = stage->GetPrimAtPath(
        SdfPath("/Prim1"), fixture.journal);
    TF_AXIOM(prim->IsValid(fixture.journal));

    const EsfAttribute attr = stage->GetAttributeAtPath(
        SdfPath("/Prim1.attr1"), fixture.journal);
    TF_AXIOM(attr->IsValid(fixture.journal));

    const EsfProperty prop = stage->GetPropertyAtPath(
        SdfPath("/Prim1.ns1:ns2:attr2"), fixture.journal);
    TF_AXIOM(prop->IsValid(fixture.journal));
}

// Tests that EsfUsd_Objects behave as UsdObjects.
static void
TestObject(Fixture &fixture)
{
    const EsfObject primObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1")));
    TF_AXIOM(primObject->IsValid(fixture.journal));
    _TestMetadata(fixture, primObject, "prim doc");

    const EsfObject attrObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attrObject->IsValid(fixture.journal));
    _TestMetadata(fixture, attrObject, "attr doc");

    const EsfObject relObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.rel1")));
    TF_AXIOM(relObject->IsValid(fixture.journal));
    _TestMetadata(fixture, relObject, "rel doc");

    const EsfObject invalidObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Does/Not/Exist")));
    TF_AXIOM(!invalidObject->IsValid(fixture.journal));
}

// Tests that EsfUsd_Prims behave as UsdPrims.
static void
TestPrim(Fixture &fixture)
{
    const EsfPrim prim = EsfUsdSceneAdapter::AdaptPrim(
        fixture.stage->GetPrimAtPath(SdfPath("/Prim1")));
    TF_AXIOM(prim->IsValid(fixture.journal));
    _TestMetadata(fixture, prim, "prim doc");

    const EsfPrim pseudoRootPrim = prim->GetParent(fixture.journal);
    TF_AXIOM(pseudoRootPrim->IsValid(fixture.journal));
    TF_AXIOM(pseudoRootPrim->GetPath(fixture.journal) == SdfPath("/"));

    const TfType expectedType = TfType::Find<UsdGeomScope>();
    TF_AXIOM(prim->GetType(fixture.journal) == expectedType);

    const TfTokenVector expectedSchemas{ TfToken("CollectionAPI:collection1") };
    TF_AXIOM(prim->GetAppliedSchemas(fixture.journal) == expectedSchemas);

    const EsfAttribute attr = prim->GetAttribute(
        TfToken("attr1"), fixture.journal);
    TF_AXIOM(attr->IsValid(fixture.journal));
    TF_AXIOM(attr->GetPath(fixture.journal) == SdfPath("/Prim1.attr1"));
}

// Tests that EsfUsd_Properties behave as UsdProperties.
static void
TestProperty(Fixture &fixture)
{
    const EsfProperty prop = EsfUsdSceneAdapter::AdaptProperty(
        fixture.stage->GetPropertyAtPath(SdfPath("/Prim1.ns1:ns2:attr2")));
    TF_AXIOM(prop->IsValid(fixture.journal));
    _TestMetadata(fixture, prop, "prop doc");

    TF_AXIOM(prop->GetBaseName(fixture.journal) == TfToken("attr2"));
    TF_AXIOM(prop->GetNamespace(fixture.journal) == TfToken("ns1:ns2"));
}

// Tests that EsfUsd_Relationships behave as UsdRelationships.
static void
TestRelationship(Fixture &fixture)
{
    const EsfObject object = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.rel1")));
    TF_AXIOM(object->IsRelationship());
    const EsfRelationship rel = object->AsRelationship();
    TF_AXIOM(rel->IsValid(fixture.journal));
    _TestMetadata(fixture, rel, "rel doc");

    const SdfPathVector targets = rel->GetTargets(fixture.journal);
    ASSERT_EQ(targets.size(), 2);
    TF_AXIOM(
        targets ==
        SdfPathVector({SdfPath("/Prim1.attr1"), SdfPath("/Prim1.rel2")}));

    const SdfPathVector forwardedTargets =
        rel->GetForwardedTargets(fixture.journal);
    ASSERT_EQ(forwardedTargets.size(), 3);
    TF_AXIOM(
        forwardedTargets ==
        SdfPathVector(
            {SdfPath("/Prim1.attr1"), SdfPath("/Prim2"), SdfPath("/Prim3")}));
}

// Tests that EsfUsd_Attributes behave as UsdAttributes.
static void
TestAttribute(Fixture &fixture)
{
    const EsfAttribute attr = EsfUsdSceneAdapter::AdaptAttribute(
        fixture.stage->GetAttributeAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attr->IsValid(fixture.journal));
    _TestMetadata(fixture, attr, "attr doc");

    TF_AXIOM(attr->GetValueTypeName(fixture.journal) == SdfValueTypeNames->Int);

    const SdfPathVector targets = attr->GetConnections(fixture.journal);
    ASSERT_EQ(targets.size(), 2);
    TF_AXIOM(
        targets ==
        SdfPathVector({
            SdfPath("/Prim1.ns1:ns2:attr2"), SdfPath("/Prim1.attr3")}));
}

// Tests that EsfUsd_AttributeQuery behaves as UsdAttributeQuery.
static void
TestAttributeQuery(Fixture &fixture)
{
    const UsdAttribute usdAttr =
        fixture.stage->GetAttributeAtPath(SdfPath("/Prim1.attr1"));
    const UsdAttributeQuery usdQuery(usdAttr);

    const EsfAttribute esfAttr = EsfUsdSceneAdapter::AdaptAttribute(usdAttr);
    const EsfAttributeQuery esfQuery = esfAttr->GetQuery();

    VtValue esfValue, usdValue;
    TF_AXIOM(esfQuery->IsValid() == usdQuery.IsValid());
    TF_AXIOM(esfQuery->Get(&esfValue, UsdTimeCode::Default()) ==
        usdQuery.Get(&usdValue, UsdTimeCode::Default()));
    TF_AXIOM(esfValue.IsHolding<int>() == usdValue.IsHolding<int>());
    TF_AXIOM((esfValue.UncheckedGet<int>() == 
        usdValue.UncheckedGet<int>()) == 1);

    TF_AXIOM(esfQuery->GetPath() == SdfPath("/Prim1.attr1"));
    TF_AXIOM(esfQuery->GetSpline().has_value() == usdQuery.HasSpline());
    TF_AXIOM(esfQuery->ValueMightBeTimeVarying() ==
        usdQuery.ValueMightBeTimeVarying());
    TF_AXIOM(!esfQuery->IsTimeVarying(
        UsdTimeCode::Default(), UsdTimeCode(0.0)));
}

// Tests EsfUsd_AttributeQuery with a time-varying spline attribute.
static void
TestSplineAttributeQuery(Fixture &fixture)
{
    const UsdAttribute usdAttr =
        fixture.stage->GetAttributeAtPath(SdfPath("/Prim1.attr3"));
    const UsdAttributeQuery usdQuery(usdAttr);

    const EsfAttribute esfAttr = EsfUsdSceneAdapter::AdaptAttribute(usdAttr);
    const EsfAttributeQuery esfQuery = esfAttr->GetQuery();

    VtValue esfValue, usdValue;
    TF_AXIOM(esfQuery->IsValid() == usdQuery.IsValid());
    TF_AXIOM(esfQuery->Get(&esfValue, UsdTimeCode(2.0)) ==
        usdQuery.Get(&usdValue, UsdTimeCode(2.0)));
    TF_AXIOM(esfValue.IsHolding<double>() == usdValue.IsHolding<double>());
    TF_AXIOM((esfValue.UncheckedGet<double>() == 
        usdValue.UncheckedGet<double>()) == 1.0);

    TF_AXIOM(esfQuery->GetPath() == SdfPath("/Prim1.attr3"));
    TF_AXIOM(esfQuery->GetSpline().has_value() == usdQuery.HasSpline());
    TF_AXIOM(esfQuery->ValueMightBeTimeVarying() ==
        usdQuery.ValueMightBeTimeVarying());
    TF_AXIOM(esfQuery->IsTimeVarying(UsdTimeCode(1.0), UsdTimeCode(2.0)));
    TF_AXIOM(!esfQuery->IsTimeVarying(UsdTimeCode(2.0), UsdTimeCode(3.0)));
}

static void
TestGetSchemaConfigKey(Fixture &fixture)
{
    const EsfObject pseudoRootObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/")));
    TF_AXIOM(pseudoRootObject->IsValid(fixture.journal));
    
    const EsfObject prim1Object = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1")));
    TF_AXIOM(prim1Object->IsValid(fixture.journal));

    const EsfObject attrObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attrObject->IsValid(fixture.journal));

    const EsfObject relObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.rel1")));
    TF_AXIOM(relObject->IsValid(fixture.journal));

    const EsfObject prim2Object = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim2")));
    TF_AXIOM(prim2Object->IsValid(fixture.journal));

    const EsfObject prim3Object = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim3")));
    TF_AXIOM(prim3Object->IsValid(fixture.journal));

    TF_AXIOM(pseudoRootObject->GetSchemaConfigKey(fixture.journal) ==
             EsfSchemaConfigKey());
    TF_AXIOM(prim1Object->GetSchemaConfigKey(fixture.journal) !=
             EsfSchemaConfigKey());
    TF_AXIOM(attrObject->GetSchemaConfigKey(fixture.journal) ==
             prim1Object->GetSchemaConfigKey(fixture.journal));
    TF_AXIOM(relObject->GetSchemaConfigKey(fixture.journal) ==
             prim1Object->GetSchemaConfigKey(fixture.journal));
    TF_AXIOM(prim1Object->GetSchemaConfigKey(fixture.journal) ==
             prim2Object->GetSchemaConfigKey(fixture.journal));
    TF_AXIOM(prim1Object->GetSchemaConfigKey(fixture.journal) !=
             prim3Object->GetSchemaConfigKey(fixture.journal));
}

int main()
{
    const std::vector tests {
        TestStage,
        TestObject,
        TestPrim,
        TestProperty,
        TestRelationship,
        TestAttribute,
        TestAttributeQuery,
        TestSplineAttributeQuery,
        TestGetSchemaConfigKey
    };
    for (auto test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
