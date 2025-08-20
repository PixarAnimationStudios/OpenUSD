//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/sceneAdapter.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/attributeQuery.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/property.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/scope.h"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

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
            ) {
                int attr1 = 1
                int ns1:ns2:attr2 = 2
                double attr3.spline = {
                    1: 0,
                    2: 1,
                }
                rel rel1
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

// Tests that ExecUsd_Objects behave as UsdObjects.
static void
TestObject(Fixture &fixture)
{
    const EsfObject primObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1")));
    TF_AXIOM(primObject->IsValid(fixture.journal));

    const EsfObject attrObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attrObject->IsValid(fixture.journal));

    const EsfObject relObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Prim1.rel1")));
    TF_AXIOM(relObject->IsValid(fixture.journal));

    const EsfObject invalidObject = EsfUsdSceneAdapter::AdaptObject(
        fixture.stage->GetObjectAtPath(SdfPath("/Does/Not/Exist")));
    TF_AXIOM(!invalidObject->IsValid(fixture.journal));
}

// Tests that ExecUsd_Prims behave as UsdPrims.
static void
TestPrim(Fixture &fixture)
{
    const EsfPrim prim = EsfUsdSceneAdapter::AdaptPrim(
        fixture.stage->GetPrimAtPath(SdfPath("/Prim1")));
    TF_AXIOM(prim->IsValid(fixture.journal));

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

// Tests that ExecUsd_Properties behave as UsdProperties.
static void
TestProperty(Fixture &fixture)
{
    const EsfProperty prop = EsfUsdSceneAdapter::AdaptProperty(
        fixture.stage->GetPropertyAtPath(SdfPath("/Prim1.ns1:ns2:attr2")));
    TF_AXIOM(prop->IsValid(fixture.journal));

    TF_AXIOM(prop->GetBaseName(fixture.journal) == TfToken("attr2"));
    TF_AXIOM(prop->GetNamespace(fixture.journal) == TfToken("ns1:ns2"));
}

// Tests that ExecUsd_Attributes behave as UsdAttributes.
static void
TestAttribute(Fixture &fixture)
{
    const EsfAttribute attr = EsfUsdSceneAdapter::AdaptAttribute(
        fixture.stage->GetAttributeAtPath(SdfPath("/Prim1.attr1")));
    TF_AXIOM(attr->IsValid(fixture.journal));

    TF_AXIOM(attr->GetValueTypeName(fixture.journal) == SdfValueTypeNames->Int);
}


// Tests that ExecUsd_AttributeQuery behaves as UsdAttributeQuery.
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

// Tests ExecUsd_AttributeQuery with a time-varying spline attribute.
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
