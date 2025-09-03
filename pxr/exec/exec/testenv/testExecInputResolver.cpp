//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/exec/inputResolver.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/outputKey.h"
#include "pxr/exec/exec/providerResolution.h"
#include "pxr/exec/exec/registerSchema.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/exec/esfUsd/sceneAdapter.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE;

class VdfContext;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (attr)
    (attributeComputation)
    (inputName)
    (dispatchedAttributeComputation)
    (dispatchedPrimComputation)
    (nonExistentComputation)
    (primComputation)
);

#define ASSERT_EQ(expr, expected)                                              \
    [&] {                                                                      \
        auto&& expr_ = expr;                                                   \
        if (expr_ != expected) {                                               \
            TF_FATAL_ERROR(                                                    \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'",        \
                TfStringify(expected).c_str(),                                 \
                TfStringify(expr_).c_str());                                   \
        }                                                                      \
    }()

#define ASSERT_OUTPUT_KEY(                                                     \
    actual, expectedProvider, expectedSchemaKey, expectedDefinition)           \
    {                                                                          \
        const Exec_OutputKey expected{                                         \
            expectedProvider, expectedSchemaKey, expectedDefinition};          \
        const Exec_OutputKey::Identity actualOutputKeyIdentity =               \
            (actual).MakeIdentity();                                           \
        const Exec_OutputKey::Identity expectedOutputKeyIdentity =             \
            expected.MakeIdentity();                                           \
        ASSERT_EQ(actualOutputKeyIdentity, expectedOutputKeyIdentity);         \
    }

PXR_NAMESPACE_OPEN_SCOPE

static std::ostream &
operator<<(std::ostream &out, const Exec_OutputKey::Identity &outputKeyIdentity)
{
    return out << outputKeyIdentity.GetDebugName();
}

static std::ostream &
operator<<(std::ostream &out, const EsfJournal &journal)
{
    if (journal.begin() == journal.end()) {
        return out << "{}";
    }
    out << "{";
    for (const EsfJournal::value_type &entry : journal) {
        out << "\n    <" << entry.first.GetText() << "> "
            << '(' << entry.second.GetDescription() << ')';
    }
    out << "\n}";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

// TestExecInputResolverCustomSchema is a codeless schema that's loaded for this
// test only. The schema is loaded from testenv/testExecInputResolver/resources.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecInputResolverCustomSchema)
{
    self.PrimComputation(_tokens->primComputation)
        .Callback<int>(+[](const VdfContext &){ return 0; });

    self.DispatchedPrimComputation(_tokens->dispatchedPrimComputation)
        .Callback<int>(+[](const VdfContext &){ return 0; });

    self.AttributeComputation(
        _tokens->attr,
        _tokens->attributeComputation)
        .Callback<int>(+[](const VdfContext &){ return 0; });

    self.DispatchedAttributeComputation(
        _tokens->dispatchedAttributeComputation)
        .Callback<int>(+[](const VdfContext &){ return 0; });
}

class Fixture
{
public:
    const Exec_ComputationDefinition *primComputationDefinition;
    const Exec_ComputationDefinition *dispatchedPrimComputationDefinition;
    const Exec_ComputationDefinition *attributeComputationDefinition;
    const Exec_ComputationDefinition *dispatchedAttributeComputationDefinition;
    const Exec_ComputationDefinition *timeComputationDefinition;
    EsfJournal journal;

    Fixture()
    {
        EsfJournal *const nullJournal = nullptr;
        const Exec_DefinitionRegistry &reg =
            Exec_DefinitionRegistry::GetInstance();

        // Instantiate a stage that we can use to get ahold of the computation
        // definitions that we expect to find in the test cases.

        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
            def CustomSchema "Prim" {
                int attr
            }
        )usd");

        const EsfPrim prim =
            stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        primComputationDefinition =
            reg.GetComputationDefinition(
                *prim.Get(), _tokens->primComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primComputationDefinition);

        dispatchedPrimComputationDefinition =
            reg.GetComputationDefinition(
                *prim.Get(), _tokens->dispatchedPrimComputation,
                prim->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(primComputationDefinition);

        const EsfAttribute attribute =
            stage->GetAttributeAtPath(SdfPath("/Prim.attr"), nullJournal);
        TF_AXIOM(attribute->IsValid(nullJournal));

        attributeComputationDefinition =
            reg.GetComputationDefinition(
                *attribute.Get(), _tokens->attributeComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(attributeComputationDefinition);

        dispatchedAttributeComputationDefinition =
            reg.GetComputationDefinition(
                *attribute.Get(), _tokens->dispatchedAttributeComputation,
                attribute->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(attributeComputationDefinition);

        const EsfPrim pseudoRoot =
            stage->GetPrimAtPath(SdfPath("/"), nullJournal);
        TF_AXIOM(pseudoRoot->IsValid(nullJournal));

        timeComputationDefinition =
            reg.GetComputationDefinition(
                *pseudoRoot.Get(), ExecBuiltinComputations->computeTime,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(timeComputationDefinition);
    }

    void NewStageFromLayer(const char *layerContents)
    {
        _stage = std::make_unique<EsfStage>(_NewStageFromLayer(layerContents));
    }

    EsfObject GetObjectAtPath(const char *pathString) const
    {
        return _stage->Get()->GetObjectAtPath(SdfPath(pathString), nullptr);
    }

    Exec_OutputKeyVector ResolveInput(
        const EsfObject &origin,
        const TfToken &computationName,
        const TfType resultType,
        const EsfSchemaConfigKey dispatchingSchemaKey,
        const SdfPath &localTraversal,
        const ExecProviderResolution::DynamicTraversal dynamicTraversal)
    {
        TF_AXIOM(origin->IsValid(nullptr));

        const bool fallsBackToDispatched =
            (dispatchingSchemaKey != EsfSchemaConfigKey());

        const Exec_InputKey inputKey {
            _tokens->inputName,
            computationName,
            /* metadataKey */ TfToken(),
            resultType,
            ExecProviderResolution {
                localTraversal,
                dynamicTraversal
            },
            fallsBackToDispatched,
            false, /* optional */ 
        };
        return Exec_ResolveInput(
            *_stage, origin, dispatchingSchemaKey, inputKey, &journal);
    }

private:

    static EsfStage _NewStageFromLayer(const char *layerContents)
    {
        const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
        layer->ImportFromString(layerContents);
        TF_AXIOM(layer);
        UsdStageRefPtr usdStage = UsdStage::Open(layer);
        TF_AXIOM(usdStage);
        return EsfUsdSceneAdapter::AdaptStage(usdStage);
    }

private:
    // Hold an EsfStage by unique_ptr because it's not default-constructible.
    std::unique_ptr<EsfStage> _stage;
};

// Test that Exec_ResolveInput finds a computation on the origin prim.
static void
TestResolveToComputationPrimOrigin(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0],
        fixture.GetObjectAtPath("/Origin"),
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal.Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds a computation on the origin attribute.
static void
TestResolveToComputationAttributeOrigin(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
            int attr
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Prim.attr"),
        _tokens->attributeComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0],
        fixture.GetObjectAtPath("/Prim.attr"),
        EsfSchemaConfigKey(),
        fixture.attributeComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal.Add(SdfPath("/Prim.attr"), EsfEditReason::ResyncedObject);
    expectedJournal.Add(SdfPath("/Prim"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput fails to find a computation on the origin object
// if that object does not define a computation by that name.
//
static void
TestResolveToComputationOrigin_NoSuchComputation(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->nonExistentComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 0);

    EsfJournal expectedJournal;
    expectedJournal.Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput fails to find a computation on the origin object
// if a computation of the requested name was found, but it does not match the
// requested result type.
//
static void
TestResolveToComputationOrigin_WrongResultType(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->primComputation,
        TfType::Find<double>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 0);

    EsfJournal expectedJournal;
    expectedJournal.Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds a computation on the nearest namespace
// ancestor that defines the requested computation.
//
static void
TestResolveToNamespaceAncestor(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Root" {
            def CustomSchema "Ancestor" {
                def Scope "Scope1" {
                    def Scope "Scope2" {
                        def Scope "Origin" {
                        }
                    }
                }
            }
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Root/Ancestor/Scope1/Scope2/Origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::NamespaceAncestor);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Root/Ancestor"), 
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Root/Ancestor/Scope1/Scope2/Origin"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root/Ancestor/Scope1/Scope2"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root/Ancestor/Scope1"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root/Ancestor"),
            EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput fails to find a computation on the nearest
// namespace ancestor if no ancestor defines a computation by that name.
//
static void
TestResolveToNamespaceAncestor_NoSuchAncestor(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def Scope "Root" {
            def Scope "Parent" {
                def CustomSchema "Origin" {
                }
            }
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Root/Parent/Origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::NamespaceAncestor);

    ASSERT_EQ(outputKeys.size(), 0);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Root/Parent/Origin"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root/Parent"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root"),
            EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput fails to find a computation on the nearest
// namespace ancestor if all ancestors define computations of the requested
// name, but of different result types.
//
static void
TestResolveToNamespaceAncestor_WrongResultType(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Root" {
            def CustomSchema "Parent" {
                def CustomSchema "Origin" {
                }
            }
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Root/Parent/Origin"),
        _tokens->primComputation,
        TfType::Find<double>(),
        EsfSchemaConfigKey(),
        SdfPath("."),
        ExecProviderResolution::DynamicTraversal::NamespaceAncestor);

    ASSERT_EQ(outputKeys.size(), 0);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Root/Parent/Origin"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root/Parent"),
            EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Root"),
            EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds a computation on the owning prim when the
// origin is an attribute, and the local traversal is "..".
//
static void
TestResolveToOwningPrim(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "OwningPrim" {
            double origin = 1.0
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/OwningPrim.origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath(".."),
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/OwningPrim"), 
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/OwningPrim.origin"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/OwningPrim"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds computations on the targeted objects when
// the origin is a prim, the local traversal is the relative path to a
// relationship and the dynamic traversal is TargetedObjects.
//
static void
TestResolveToTargetedObjects(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
            add rel myRel = [</Origin/A>, </Origin.forwardingRel>]
            add rel forwardingRel = </Origin/B>
            def CustomSchema "A" {}
            def CustomSchema "B" {}
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath(".myRel"),
        ExecProviderResolution::DynamicTraversal::RelationshipTargetedObjects);

    ASSERT_EQ(outputKeys.size(), 2);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Origin/A"), 
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);
    ASSERT_OUTPUT_KEY(
        outputKeys[1], 
        fixture.GetObjectAtPath("/Origin/B"), 
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myRel"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myRel"), EsfEditReason::ChangedTargetPaths)
        .Add(SdfPath("/Origin.forwardingRel"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.forwardingRel"), EsfEditReason::ChangedTargetPaths)
        .Add(SdfPath("/Origin/A"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/B"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput silently ignores missing targets.
static void
TestResolveToTargetedObjects_MissingTarget(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
            add rel myRel = [</Origin/A>, </Origin/B>]
            def CustomSchema "A" {}
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->primComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath(".myRel"),
        ExecProviderResolution::DynamicTraversal::RelationshipTargetedObjects);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Origin/A"), 
        EsfSchemaConfigKey(),
        fixture.primComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myRel"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myRel"), EsfEditReason::ChangedTargetPaths)
        .Add(SdfPath("/Origin/A"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/B"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds computations on the targeted objects
// when the origin is a prim, the local traversal is the relative path to an
// attribute and the dynamic traversal is ConnectionTargetedObjects.
//
static void
TestResolveToConnectionTargetedObjects(Fixture &fixture)
{
    // TODO: When we provide a way for prims to register 'computeValue'
    // computations, we can add an attribute connection that targets a prim
    // here.
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
            int myAttr.connect = [</Origin/A.attr>, </Origin/B.attr>]
            def CustomSchema "A" {
                int attr
            }
            def CustomSchema "B" {
                int attr
            }
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->attributeComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath(".myAttr"),
        ExecProviderResolution::DynamicTraversal::ConnectionTargetedObjects);

    ASSERT_EQ(outputKeys.size(), 2);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Origin/A.attr"), 
        EsfSchemaConfigKey(),
        fixture.attributeComputationDefinition);
    ASSERT_OUTPUT_KEY(
        outputKeys[1], 
        fixture.GetObjectAtPath("/Origin/B.attr"), 
        EsfSchemaConfigKey(),
        fixture.attributeComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myAttr"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myAttr"), EsfEditReason::ChangedConnectionPaths)
        .Add(SdfPath("/Origin/A"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/A.attr"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/B"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/B.attr"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput silently ignores missing connection targets.
static void
TestResolveToConnectionTargetedObjects_MissingConnectionTarget(
    Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Origin" {
            int myAttr.connect = [</Origin/A.attr>, </Origin/B.attr>]
            def CustomSchema "A" {
                int attr
            }
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Origin"),
        _tokens->attributeComputation,
        TfType::Find<int>(),
        EsfSchemaConfigKey(),
        SdfPath(".myAttr"),
        ExecProviderResolution::DynamicTraversal::ConnectionTargetedObjects);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Origin/A.attr"), 
        EsfSchemaConfigKey(),
        fixture.attributeComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Origin"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myAttr"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin.myAttr"), EsfEditReason::ChangedConnectionPaths)
        .Add(SdfPath("/Origin/A"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/A.attr"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Origin/B.attr"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test that Exec_ResolveInput finds a computation on the stage (i.e., on the
// pseudoroot prim), and the local traversal is "/".
//
static void
TestResolveToStage(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Root" {
        }
    )usd");

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Root") /* origin */,
        ExecBuiltinComputations->computeTime,
        TfType::Find<EfTime>(),
        EsfSchemaConfigKey(),
        SdfPath("/") /* localTraversal */,
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/"),
        EsfSchemaConfigKey(),
        fixture.timeComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Directly test dispatched input resolution here by resolving using the parent
// prim as the origin, but providing the config key for the child prim's schema,
// which is the schema that dispatches the computation we request.
//
static void
TestResolveForDispatchedPrimComputation(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def Scope "Parent" {
            def CustomSchema "Child" {
            }
        }
    )usd");

    constexpr EsfJournal *nullJournal = nullptr;

    const EsfObject parent = fixture.GetObjectAtPath("/Parent");
    TF_AXIOM(parent->IsValid(nullJournal));
    TF_AXIOM(parent->IsPrim());
    const EsfObject child = fixture.GetObjectAtPath("/Parent/Child");
    TF_AXIOM(child->IsValid(nullJournal));
    TF_AXIOM(child->IsPrim());

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        parent /* origin */,
        _tokens->dispatchedPrimComputation,
        TfType::Find<int>(),
        child->GetSchemaConfigKey(nullJournal),
        SdfPath(".") /* localTraversal */,
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        parent,
        child->GetSchemaConfigKey(nullJournal),
        fixture.dispatchedPrimComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Parent"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Directly test dispatched input resolution here by resolving using an
// attribute on the parent prim as the origin, but providing the config key for
// the child prim's schema, which is the schema that dispatches the attribute
// computation we request.
//
static void
TestResolveForDispatchedAttributeComputation(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def Scope "Parent" {
            int attr
            def CustomSchema "Child" {
            }
        }
    )usd");

    constexpr EsfJournal *nullJournal = nullptr;

    const EsfObject attr = fixture.GetObjectAtPath("/Parent.attr");
    TF_AXIOM(attr->IsValid(nullJournal));
    TF_AXIOM(attr->IsAttribute());
    const EsfObject child = fixture.GetObjectAtPath("/Parent/Child");
    TF_AXIOM(child->IsValid(nullJournal));
    TF_AXIOM(child->IsPrim());

    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        attr /* origin */,
        _tokens->dispatchedAttributeComputation,
        TfType::Find<int>(),
        child->GetSchemaConfigKey(nullJournal),
        SdfPath(".") /* localTraversal */,
        ExecProviderResolution::DynamicTraversal::Local);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        attr,
        child->GetSchemaConfigKey(nullJournal),
        fixture.dispatchedAttributeComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Parent.attr"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Parent"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

// Test resolving an input from a dispatched computation via relationship
// targets.
//
static void
TestResolveForDispatchedComputation_RelTarget(Fixture &fixture)
{
    fixture.NewStageFromLayer(R"usd(#usda 1.0
        def Scope "Parent" {
            def CustomSchema "Child" {
                add rel myRel = </Parent/A>
            }
            def Scope "A" {}
        }
    )usd");

    constexpr EsfJournal *nullJournal = nullptr;

    const EsfObject parent = fixture.GetObjectAtPath("/Parent");
    TF_AXIOM(parent->IsValid(nullJournal));
    TF_AXIOM(parent->IsPrim());
    const EsfObject child = fixture.GetObjectAtPath("/Parent/Child");
    TF_AXIOM(child->IsValid(nullJournal));
    TF_AXIOM(child->IsPrim());


    const Exec_OutputKeyVector outputKeys = fixture.ResolveInput(
        fixture.GetObjectAtPath("/Parent/Child"),
        _tokens->dispatchedPrimComputation,
        TfType::Find<int>(),
        child->GetSchemaConfigKey(nullJournal),
        SdfPath(".myRel"),
        ExecProviderResolution::DynamicTraversal::
        RelationshipTargetedObjects);

    ASSERT_EQ(outputKeys.size(), 1);
    ASSERT_OUTPUT_KEY(
        outputKeys[0], 
        fixture.GetObjectAtPath("/Parent/A"), 
        child->GetSchemaConfigKey(nullJournal),
        fixture.dispatchedPrimComputationDefinition);

    EsfJournal expectedJournal;
    expectedJournal
        .Add(SdfPath("/Parent/Child"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Parent/Child.myRel"), EsfEditReason::ResyncedObject)
        .Add(SdfPath("/Parent/Child.myRel"), EsfEditReason::ChangedTargetPaths)
        .Add(SdfPath("/Parent/A"), EsfEditReason::ResyncedObject);
    ASSERT_EQ(fixture.journal, expectedJournal);
}

int main()
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecInputResolver");

    const TfType customSchemaType =
        TfType::FindByName("TestExecInputResolverCustomSchema");
    TF_AXIOM(!customSchemaType.IsUnknown());

    std::vector tests {
        TestResolveToComputationPrimOrigin,
        TestResolveToComputationAttributeOrigin,
        TestResolveToComputationOrigin_NoSuchComputation,
        TestResolveToComputationOrigin_WrongResultType,
        TestResolveToNamespaceAncestor,
        TestResolveToNamespaceAncestor_NoSuchAncestor,
        TestResolveToNamespaceAncestor_WrongResultType,
        TestResolveToOwningPrim,
        TestResolveToTargetedObjects,
        TestResolveToTargetedObjects_MissingTarget,
        TestResolveToConnectionTargetedObjects,
        TestResolveToConnectionTargetedObjects_MissingConnectionTarget,
        TestResolveToStage,
        TestResolveForDispatchedPrimComputation,
        TestResolveForDispatchedAttributeComputation,
        TestResolveForDispatchedComputation_RelTarget,
    };
    for (const auto &test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
