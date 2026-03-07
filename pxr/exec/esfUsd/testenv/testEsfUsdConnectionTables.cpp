//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/exec/esfUsd/stageData.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/stage.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <set>
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

static std::set<SdfPath>
_MakePathSet(const SdfPathVector &paths) {
    return std::set<SdfPath>(paths.begin(), paths.end());
}

namespace
{

struct Fixture : TfWeakBase
{
    // Hold onto the payload layer when it is unloaded.
    SdfLayerRefPtr payloadLayer;
    UsdStageRefPtr stage;
    std::shared_ptr<EsfUsdStageData> stageData;
    EsfUsdStageData::ChangedPathSet changedTargetPaths;
    EsfJournal *const journal = nullptr;

    Fixture(SdfLayerRefPtr &&layerIn = {})
    {
        payloadLayer = SdfLayer::CreateAnonymous(".usda");
        {
            const bool importedLayer = payloadLayer->ImportFromString(R"usd(
            #usda 1.0
            def Scope "Prim" (
            ) {
                int attr1.connect = [</Prim.attr2>]
                int attr2
                int attr3
            }
            )usd");
            TF_AXIOM(importedLayer);
        }

        // If a layer was passed in, use that as the root layer. Otherwise, use
        // a default layer that payloads in payloadLayer.
        //
        // The stage takes ownership of the root layer.
        const SdfLayerRefPtr rootLayer =
            [&layerIn, &payloadLayer=payloadLayer]
        {
            if (layerIn) {
                return layerIn;
            }

            const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
            const bool importedLayer = layer->ImportFromString(R"usd(
            #usda 1.0
            over "Prim" (
                payload = @)usd" + payloadLayer->GetIdentifier() +
                    R"usd(@</Prim>
            ) {
            }
            )usd");
            TF_AXIOM(importedLayer);

            return layer;
        }();

        stage = UsdStage::Open(rootLayer);
        TF_AXIOM(stage);

        stageData = EsfUsdStageData::RegisterStage(stage);

        const UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"));
        TF_AXIOM(prim && prim.IsLoaded());

        TfNotice::Register(
            TfCreateWeakPtr(this),
            &Fixture::_DidObjectsChanged,
            UsdStageConstPtr(stage));
    }

    void AssertChangedTargets(
        const std::set<SdfPath> &expected)
    {
        std::set<SdfPath> actual(
            changedTargetPaths.begin(), changedTargetPaths.end());
        ASSERT_EQ(actual, expected);

        changedTargetPaths.clear();
    }

    // Update the stage data in response to scene changes.
    void _DidObjectsChanged(
        const UsdNotice::ObjectsChanged &objectsChanged)
    {
        EsfUsdStageData &stageData = EsfUsdStageData::GetStageData(stage);

        for (const SdfPath &path : objectsChanged.GetResyncedPaths()) {
            stageData.UpdateForResync(path, &changedTargetPaths);
        }

        for (const SdfPath &path : objectsChanged.GetChangedInfoOnlyPaths()) {
            const TfTokenVector changedFields =
                objectsChanged.GetChangedFields(path);
            if (std::find(
                    changedFields.begin(), changedFields.end(),
                    SdfFieldKeys->ConnectionPaths) != changedFields.end()) {
                stageData.UpdateForChangedAttributeConnections(
                    path, &changedTargetPaths);
            }
        }
    }
};

} // anonymous namespace

// Tests connection table updates in resonse to attribute connection edits.
//
static void
TestConnectionEdits(Fixture &fixture)
{
    const EsfStage stage = EsfUsdSceneAdapter::AdaptStage(fixture.stage);

    const EsfPrim prim = stage->GetPrimAtPath(
        SdfPath("/Prim"), fixture.journal);
    TF_AXIOM(prim->IsValid(fixture.journal));

    const EsfAttribute attr1 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr1"), fixture.journal);
    TF_AXIOM(attr1->IsValid(fixture.journal));

    const EsfAttribute attr2 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr2"), fixture.journal);
    TF_AXIOM(attr2->IsValid(fixture.journal));

    const EsfAttribute attr3 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr3"), fixture.journal);
    TF_AXIOM(attr3->IsValid(fixture.journal));

    // Initially, there is a single authored connection.
    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr1")}));

    // Author another connection.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr3"))
        .AddConnection(SdfPath("/Prim.attr2"));

    fixture.AssertChangedTargets({SdfPath("/Prim.attr2")});

    ASSERT_EQ(
        attr3->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr1"), SdfPath("/Prim.attr3")}));

    // Remove the first connection.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr1"))
        .RemoveConnection(SdfPath("/Prim.attr2"));

    fixture.AssertChangedTargets({SdfPath("/Prim.attr2")});

    ASSERT_EQ(attr1->GetConnections(fixture.journal).size(), 0);

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr3")}));

    // Author another connection on attr3.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr3"))
        .AddConnection(SdfPath("/Prim.attr1"));

    fixture.AssertChangedTargets({SdfPath("/Prim.attr1")});

    ASSERT_EQ(
        attr3->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2"), SdfPath("/Prim.attr1")}));

    ASSERT_EQ(
        attr1->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr3")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr3")}));

    // Change the order of the connections on attr3.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr3"))
        .SetConnections({SdfPath("/Prim.attr1"), SdfPath("/Prim.attr2")});

    fixture.AssertChangedTargets({});

    ASSERT_EQ(
        attr3->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr1"), SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        attr1->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr3")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr3")}));

    // Author a connection to a prim.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr1"))
        .AddConnection(SdfPath("/Prim"));

    fixture.AssertChangedTargets({SdfPath("/Prim")});

    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim")}));

    ASSERT_EQ(
        prim->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr1")}));
}

// Stocastically test changes to multiple connections targeting a single
// attribute in response to activation changes on the owning prims.
//
static void
TestConnectionEditsStochastic(Fixture &unused)
{
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    const bool importedLayer = rootLayer->ImportFromString(R"usd(
    #usda 1.0
    def Scope "Prim" {
    }
    )usd");
    TF_AXIOM(importedLayer);

    Fixture fixture(std::move(rootLayer));

    const std::string layerContents(R"usd(
    #usda 1.0
    def Scope "Prim" {
        int attr

        def Scope "Child1" {
        }

        def Scope "Child2" {
        }
    }
    )usd");

    const EsfStage stage = EsfUsdSceneAdapter::AdaptStage(fixture.stage);

    // Compute the actual incoming connections on /Prim.attr for validation
    // below.
    // 
    // Compute the actual incoming connections by traversing all active, loaded,
    // defined, non-abstract prims, gathering their attributes, and building up
    // a map of incoming connections for each targeted attribute.
    //
    const auto computeActualIncoming = [stage = fixture.stage]
    {
        std::set<SdfPath> actualIncoming;

        for (const UsdPrim &prim : stage->GetPseudoRoot().GetDescendants()) {
            for (const UsdAttribute &attr : prim.GetAttributes()) {
                SdfPathVector targetPaths;
                if (attr.GetConnections(&targetPaths)) {
                    TF_AXIOM(targetPaths.size() == 1 &&
                             targetPaths[0] == SdfPath("/Prim.attr"));
                    actualIncoming.insert(attr.GetPath());
                }
            }
        }

        return actualIncoming;
    };

    static const unsigned numAttributes = 10;
    static const unsigned numIterations = 100;

    for (unsigned i=0; i<numIterations; ++i) {

        // Re-initialize the root layer and create child attributes.
        fixture.stage->GetRootLayer()->ImportFromString(layerContents);
        {
            const SdfPrimSpecHandle child1 =
                fixture.stage->GetRootLayer()->GetPrimAtPath(
                    SdfPath("/Prim/Child1"));
            TF_AXIOM(child1);
            const SdfPrimSpecHandle child2 =
                fixture.stage->GetRootLayer()->GetPrimAtPath(
                    SdfPath("/Prim/Child2"));
            TF_AXIOM(child2);

            for (unsigned attrI=0; attrI<numAttributes; ++attrI) {
                SdfAttributeSpecHandle attrSpec;
                attrSpec = SdfAttributeSpec::New(
                    child1,
                    TfStringPrintf("attr%u", attrI),
                    SdfGetValueTypeNameForValue(VtValue(0)));
                TF_AXIOM(attrSpec);
                attrSpec = SdfAttributeSpec::New(
                    child2,
                    TfStringPrintf("attr%u", attrI),
                    SdfGetValueTypeNameForValue(VtValue(0)));
                TF_AXIOM(attrSpec);
            }
        }

        const EsfAttribute attr = stage->GetAttributeAtPath(
            SdfPath("/Prim.attr"), fixture.journal);
        TF_AXIOM(attr->IsValid(fixture.journal));

        // For each child attribute, we make a random decision whether or not
        // to make a connection so that we get a different set of incoming
        // connections on each iteration.
        std::mt19937 rng(i);
        std::bernoulli_distribution makeConnection;

        for (unsigned attrI=0; attrI<numAttributes; ++attrI) {
            if (makeConnection(rng)) {
                fixture.stage->GetAttributeAtPath(
                    SdfPath(TfStringPrintf("/Prim/Child1.attr%u", attrI)))
                    .AddConnection(SdfPath("/Prim.attr"));
            }
            if (makeConnection(rng)) {
                fixture.stage->GetAttributeAtPath(
                    SdfPath(TfStringPrintf("/Prim/Child2.attr%u", attrI)))
                    .AddConnection(SdfPath("/Prim.attr"));
            }
        }

        // Validate that we compute the correct set of incoming connections
        // for /Prim.attr, then modify activation state for the child prims,
        // re-validating at each step.
        ASSERT_EQ(
            _MakePathSet(attr->GetIncomingConnections(fixture.journal)),
            computeActualIncoming());

        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child1")).SetActive(false);
        ASSERT_EQ(
            _MakePathSet(attr->GetIncomingConnections(fixture.journal)),
            computeActualIncoming());

        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child1")).SetActive(true);
        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child2")).SetActive(false);
        ASSERT_EQ(
            _MakePathSet(attr->GetIncomingConnections(fixture.journal)),
            computeActualIncoming());

        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child1")).SetActive(true);
        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child2")).SetActive(true);
        ASSERT_EQ(
            _MakePathSet(attr->GetIncomingConnections(fixture.journal)),
            computeActualIncoming());

        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child1")).SetActive(false);
        fixture.stage->GetPrimAtPath(SdfPath("/Prim/Child2")).SetActive(false);
        ASSERT_EQ(
            _MakePathSet({}),
            computeActualIncoming());
    }
}

// Tests connection table updates in response to prim resyncs.
static void
TestPrimResync(Fixture &fixture)
{
    const EsfStage stage = EsfUsdSceneAdapter::AdaptStage(fixture.stage);

    const EsfAttribute attr1 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr1"), fixture.journal);
    TF_AXIOM(attr1->IsValid(fixture.journal));

    const EsfAttribute attr2 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr2"), fixture.journal);
    TF_AXIOM(attr2->IsValid(fixture.journal));

    const EsfAttribute attr3 = stage->GetAttributeAtPath(
        SdfPath("/Prim.attr3"), fixture.journal);
    TF_AXIOM(attr3->IsValid(fixture.journal));

    // Initially, there is a single authored connection.
    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr1")}));

    // Author another connection.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim.attr3"))
        .AddConnection(SdfPath("/Prim.attr2"));

    fixture.AssertChangedTargets({SdfPath("/Prim.attr2")});

    ASSERT_EQ(
        attr3->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        _MakePathSet(attr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>({SdfPath("/Prim.attr1"), SdfPath("/Prim.attr3")}));

    // Unload the root prim
    fixture.stage->Unload(SdfPath("/Prim"));

    fixture.AssertChangedTargets({SdfPath("/Prim.attr2")});

    TF_AXIOM(!attr1->IsValid(fixture.journal));
    TF_AXIOM(!attr2->IsValid(fixture.journal));
    TF_AXIOM(!attr3->IsValid(fixture.journal));

    // Load the root prim again
    TF_AXIOM(fixture.stage->Load(SdfPath("/Prim")));
    TF_AXIOM(attr1->IsValid(fixture.journal));
    TF_AXIOM(attr2->IsValid(fixture.journal));
    TF_AXIOM(attr3->IsValid(fixture.journal));

    TF_AXIOM(fixture.stage->GetPrimAtPath(SdfPath("/Prim")).IsLoaded());
    TF_AXIOM(fixture.stage->GetPrimAtPath(SdfPath("/Prim")).IsActive());
    TF_AXIOM(fixture.stage->GetPrimAtPath(SdfPath("/Prim")).IsDefined());
    TF_AXIOM(!fixture.stage->GetPrimAtPath(SdfPath("/Prim")).IsAbstract());

    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        attr3->GetConnections(fixture.journal),
        SdfPathVector({SdfPath("/Prim.attr2")}));

    ASSERT_EQ(
        _MakePathSet(attr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>({SdfPath("/Prim.attr1"), SdfPath("/Prim.attr3")}));

    // Import an empty root layer so that we get a resync for a prim that no
    // no longer exists in the scene.
    const bool importedLayer =
        fixture.stage->GetRootLayer()->ImportFromString(R"usd(
            #usda 1.0
        )usd");
    TF_AXIOM(importedLayer);

    fixture.AssertChangedTargets({SdfPath("/Prim.attr2")});
    TF_AXIOM(!attr1->IsValid(fixture.journal));
    TF_AXIOM(!attr2->IsValid(fixture.journal));
    TF_AXIOM(!attr3->IsValid(fixture.journal));
}

// Tests connection updates in response to resyncs on prims that have descendant
// prims.
static void
TestMultiPrimResync(Fixture &unused)
{
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    const bool importedLayer = rootLayer->ImportFromString(R"usd(
    #usda 1.0
    def Scope "Prim" {
        def Scope "Parent1" {
            int attr1.connect = [<.attr2>, </Prim/Parent2.attr2>]
            int attr2
            def Scope "Child" {
                int attr1.connect = [<.attr2>, </Prim/Parent2/Child.attr2>]
                int attr2
            }
        }
        def Scope "Parent2" {
            int attr1.connect = [<.attr2>, </Prim/Parent1.attr2>]
            int attr2
            def Scope "Child" {
                int attr1.connect = [<.attr2>, </Prim/Parent1/Child.attr2>]
                int attr2
            }
        }
    }
    )usd");
    TF_AXIOM(importedLayer);

    Fixture fixture(std::move(rootLayer));

    const EsfStage stage = EsfUsdSceneAdapter::AdaptStage(fixture.stage);

    const EsfAttribute attr1 = stage->GetAttributeAtPath(
        SdfPath("/Prim/Parent1.attr1"), fixture.journal);
    TF_AXIOM(attr1->IsValid(fixture.journal));

    const EsfAttribute attr2 = stage->GetAttributeAtPath(
        SdfPath("/Prim/Parent1.attr2"), fixture.journal);
    TF_AXIOM(attr2->IsValid(fixture.journal));

    const EsfAttribute childAttr1 = stage->GetAttributeAtPath(
        SdfPath("/Prim/Parent1/Child.attr1"), fixture.journal);
    TF_AXIOM(childAttr1->IsValid(fixture.journal));

    const EsfAttribute childAttr2 = stage->GetAttributeAtPath(
        SdfPath("/Prim/Parent1/Child.attr2"), fixture.journal);
    TF_AXIOM(childAttr2->IsValid(fixture.journal));

    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1.attr2"), SdfPath("/Prim/Parent2.attr2")}));

    ASSERT_EQ(
        _MakePathSet(attr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>(
            {SdfPath("/Prim/Parent1.attr1"), SdfPath("/Prim/Parent2.attr1")}));

    ASSERT_EQ(
        childAttr1->GetConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1/Child.attr2"),
             SdfPath("/Prim/Parent2/Child.attr2")}));

    ASSERT_EQ(
        _MakePathSet(childAttr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>(
            {SdfPath("/Prim/Parent1/Child.attr1"),
             SdfPath("/Prim/Parent2/Child.attr1")}));

    // Deactivate one branch.
    fixture.stage->GetPrimAtPath(SdfPath("/Prim/Parent2")).SetActive(false);

    fixture.AssertChangedTargets(
        {SdfPath("/Prim/Parent1.attr2"),
         SdfPath("/Prim/Parent1/Child.attr2"),
         SdfPath("/Prim/Parent2.attr2"),
         SdfPath("/Prim/Parent2/Child.attr2")});

    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1.attr2"), SdfPath("/Prim/Parent2.attr2")}));

    ASSERT_EQ(
        attr2->GetIncomingConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1.attr1")}));
    ASSERT_EQ(
        childAttr2->GetIncomingConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1/Child.attr1")}));

    // Add a connection to an inactive attribute.
    fixture.stage->GetAttributeAtPath(SdfPath("/Prim/Parent2.attr1"))
        .AddConnection(SdfPath("/Prim/Parent1/Child.attr2"));

    fixture.AssertChangedTargets({});

    ASSERT_EQ(
        _MakePathSet(childAttr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>({SdfPath("/Prim/Parent1/Child.attr1")}));

    // Re-activate the branch.
    fixture.stage->GetPrimAtPath(SdfPath("/Prim/Parent2")).SetActive(true);

    fixture.AssertChangedTargets(
        {SdfPath("/Prim/Parent1.attr2"),
         SdfPath("/Prim/Parent1/Child.attr2"),
         SdfPath("/Prim/Parent2.attr2"),
         SdfPath("/Prim/Parent2/Child.attr2")});

    ASSERT_EQ(
        attr1->GetConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1.attr2"), SdfPath("/Prim/Parent2.attr2")}));

    ASSERT_EQ(
        _MakePathSet(attr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>(
            {SdfPath("/Prim/Parent1.attr1"), SdfPath("/Prim/Parent2.attr1")}));

    ASSERT_EQ(
        childAttr1->GetConnections(fixture.journal),
        SdfPathVector(
            {SdfPath("/Prim/Parent1/Child.attr2"),
             SdfPath("/Prim/Parent2/Child.attr2")}));

    ASSERT_EQ(
        _MakePathSet(childAttr2->GetIncomingConnections(fixture.journal)),
        std::set<SdfPath>(
            {SdfPath("/Prim/Parent1/Child.attr1"),
             SdfPath("/Prim/Parent2/Child.attr1"),
             SdfPath("/Prim/Parent2.attr1")}));

    // De-activate the other branch
    fixture.stage->GetPrimAtPath(SdfPath("/Prim/Parent1")).SetActive(false);

    fixture.AssertChangedTargets(
        {SdfPath("/Prim/Parent1.attr2"),
         SdfPath("/Prim/Parent1/Child.attr2"),
         SdfPath("/Prim/Parent2.attr2"),
         SdfPath("/Prim/Parent2/Child.attr2")});

    TF_AXIOM(!attr1->IsValid(fixture.journal));
    TF_AXIOM(!childAttr1->IsValid(fixture.journal));

    {
        const EsfAttribute attr1 = stage->GetAttributeAtPath(
            SdfPath("/Prim/Parent2.attr1"), fixture.journal);
        TF_AXIOM(attr1->IsValid(fixture.journal));

        const EsfAttribute attr2 = stage->GetAttributeAtPath(
            SdfPath("/Prim/Parent2.attr2"), fixture.journal);
        TF_AXIOM(attr2->IsValid(fixture.journal));

        const EsfAttribute childAttr1 = stage->GetAttributeAtPath(
            SdfPath("/Prim/Parent2/Child.attr1"), fixture.journal);
        TF_AXIOM(childAttr1->IsValid(fixture.journal));

        const EsfAttribute childAttr2 = stage->GetAttributeAtPath(
            SdfPath("/Prim/Parent2/Child.attr2"), fixture.journal);
        TF_AXIOM(childAttr2->IsValid(fixture.journal));

        ASSERT_EQ(
            attr1->GetConnections(fixture.journal),
            SdfPathVector(
                {SdfPath("/Prim/Parent2.attr2"),
                 SdfPath("/Prim/Parent1.attr2"),
                 SdfPath("/Prim/Parent1/Child.attr2")}));

        ASSERT_EQ(
            attr2->GetIncomingConnections(fixture.journal),
            SdfPathVector(
                {SdfPath("/Prim/Parent2.attr1")}));

        ASSERT_EQ(
            childAttr1->GetConnections(fixture.journal),
            SdfPathVector(
                {SdfPath("/Prim/Parent2/Child.attr2"),
                 SdfPath("/Prim/Parent1/Child.attr2")}));

        ASSERT_EQ(
            childAttr2->GetIncomingConnections(fixture.journal),
            SdfPathVector(
                {SdfPath("/Prim/Parent2/Child.attr1")})); 

        // Remove the spec for an attribute that owns connections.
        const SdfLayerHandle layer = fixture.stage->GetRootLayer();
        const SdfPrimSpecHandle prim =
            layer->GetPrimAtPath(SdfPath("/Prim/Parent2/Child"));
        TF_AXIOM(prim);
        const SdfAttributeSpecHandle attr =
            layer->GetAttributeAtPath(SdfPath("/Prim/Parent2/Child.attr1"));
        TF_AXIOM(attr);
        prim->RemoveProperty(attr);

        fixture.AssertChangedTargets(
            {SdfPath("/Prim/Parent1/Child.attr2"),
             SdfPath("/Prim/Parent2/Child.attr2")});

        TF_AXIOM(!childAttr1->IsValid(fixture.journal));

        ASSERT_EQ(
            childAttr2->GetIncomingConnections(fixture.journal),
            SdfPathVector());
    }
}

int main()
{
    const std::vector tests {
        TestConnectionEdits,
        TestConnectionEditsStochastic,
        TestPrimResync,
        TestMultiPrimResync,
    };

    for (auto test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
