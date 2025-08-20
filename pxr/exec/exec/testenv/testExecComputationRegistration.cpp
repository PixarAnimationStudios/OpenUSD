//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/registerSchema.h"

#include "pxr/exec/ef/time.h"
#include "pxr/exec/esf/stage.h"
#include "pxr/exec/esfUsd/sceneAdapter.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (appliedSchemaComputation)
    (attr)
    (attributeComputation)
    (attributeComputedValueComputation)
    (attributeName)
    (baseAndDerivedSchemaComputation)
    (derivedSchemaComputation)
    (dispatchedPrimComputation)
    (dispatchedPrimComputationOnCustomSchema)
    (emptyComputation)
    (missingComputation)
    (multiApplySchemaComputation)
    (namespaceAncestorInput)
    (noInputsComputation)
    (nonComputationalSchemaComputation)
    (primComputation)
    (relationshipName)
    (stageAccessComputation)
    (unknownSchemaTypeComputation)
);

// Attempt to register a computation for a schema type that is not registered
// with TfType.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestUnknownSchemaType)
{
    self.PrimComputation(_tokens->unknownSchemaTypeComputation)
        .Callback<double>(+[](const VdfContext &) { return 1.0; });
}

// Attempt to register a computation for a schema type that is tagged in
// plugInfo as not allowing plugin computations.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecNonComputationalSchema)
{
    self.PrimComputation(_tokens->nonComputationalSchemaComputation)
        .Callback<double>(+[](const VdfContext &) { return 1.0; });
}

// Attempt to register a computation for a schema type that has conflicting
// plugInfo declarations with respect to whether or not it allows plugin
// computations.
//
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecConflictingComputationalSchema)
{
    self.PrimComputation(_tokens->nonComputationalSchemaComputation)
        .Callback<double>(+[](const VdfContext &) { return 1.0; });
}

// Register computations for a typed schema.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecComputationRegistrationCustomSchema)
{
    self.PrimComputation(_tokens->emptyComputation);

    // Attempt to register a prim computation that uses a builtin computation
    // name.
    self.PrimComputation(ExecBuiltinComputations->computeTime);

    self.PrimComputation(_tokens->noInputsComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });

    // A prim computation that exercises various kinds of inputs.
    self.PrimComputation(_tokens->primComputation)
        .Callback<double>([](const VdfContext &ctx) { ctx.SetOutput(11.0); })
        .Inputs(
            Computation<double>(_tokens->primComputation),
            Attribute(_tokens->attributeName)
                .Computation<int>(_tokens->attributeComputation),
            AttributeValue<int>(_tokens->attributeName)
                .Required(),
            Relationship(_tokens->relationshipName)
                .TargetedObjects<int>(_tokens->primComputation),
            NamespaceAncestor<bool>(_tokens->primComputation)
                .InputName(_tokens->namespaceAncestorInput)
        );

    // A prim computation that returns the current time.
    self.PrimComputation(_tokens->stageAccessComputation)
        .Callback<EfTime>([](const VdfContext &ctx) {
            ctx.SetOutput(EfTime());
        })
        .Inputs(
            Stage()
                .Computation<EfTime>(ExecBuiltinComputations->computeTime)
                .Required()
        );

    // A prim computation that returns the value of the attribute 'attr' (of
    // type double), or 0.0, if there is no attribute of that name on the
    // owning prim.
    self.PrimComputation(_tokens->attributeComputedValueComputation)
        .Callback<double>([](const VdfContext &ctx) {
            const double *const valuePtr =
                ctx.GetInputValuePtr<double>(
                    ExecBuiltinComputations->computeValue);
            ctx.SetOutput(valuePtr ? *valuePtr : 0.0);
        })
        .Inputs(
            Attribute(_tokens->attr)
                .Computation<double>(ExecBuiltinComputations->computeValue)
        );

    // A computation that is registered on both the base and derived schemas.
    self.PrimComputation(_tokens->baseAndDerivedSchemaComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });

    // A dispatched prim computation.
    self.DispatchedPrimComputation(_tokens->dispatchedPrimComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });

    // A dispatched prim computation that only applies to CustomSchema.
    self.DispatchedPrimComputation(
        _tokens->dispatchedPrimComputationOnCustomSchema,
        TfType::FindByName("TestExecComputationRegistrationCustomSchema"))
        .Callback(+[](const VdfContext &) { return 1.0; });
}

// Register computations for a derived typed schema.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecComputationRegistrationDerivedCustomSchema)
{
    self.PrimComputation(_tokens->derivedSchemaComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });

    // This overrides the computation of the same name on the base schema.
    // (We add an input here so we can verify this definition is stronger.)
    self.PrimComputation(_tokens->baseAndDerivedSchemaComputation)
        .Callback(+[](const VdfContext &) { return 1.0; })
        .Inputs(
            AttributeValue<int>(_tokens->attributeName)
        );
}

// Register computations for an applied schema.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecComputationRegistrationCustomAppliedSchema)
{
    // A computation that is registered only for the applied schema.
    self.PrimComputation(_tokens->appliedSchemaComputation)
        .Callback(+[](const VdfContext &ctx) { return 42; });

    // A computation that is registered for the applied schema and also for a
    // typed schema.
    self.PrimComputation(_tokens->primComputation)
        .Callback<double>([](const VdfContext &ctx) { ctx.SetOutput(42.0); });
}

// Register computations for a multi-apply schema.
EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(
    TestExecComputationRegistrationCustomMultiApplySchema)
{
    self.PrimComputation(_tokens->multiApplySchemaComputation)
        .Callback(+[](const VdfContext &ctx) { return 42; });
}

// TODO: Support client code that registers schema inside their own namespaces.
#if 0

namespace client_namespace {

struct TestNamespacedSchemaType {};


EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestNamespacedSchemaType)
{
    self.PrimComputation(_tokens->noInputsComputation)
        .Callback(+[](const VdfContext &) { return 1.0; });
}

} // namespace client_namespace

#endif

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

// RAII class that verifies the expected errors is emitted during the lifetime
// of the object and that the commentary matches the expected error strings.
//
class ExpectedErrors {
public:

    // Expects the given number of errors to be emitted.
    ExpectedErrors(
        const TfCallContext &callContext,
        const size_t numErrors)
        : _callContext(callContext)
        , _numErrors(numErrors)
    {
    }

    // Expects the given error messages to be emitted.
    ExpectedErrors(
        const TfCallContext &callContext,
        const std::set<std::string> &expectedErrors)
        : _callContext(callContext)
        , _expectedErrors(expectedErrors)
        , _numErrors(_expectedErrors.size())
    {
    }

    // Expects the given number of errors to be emitted, and we expect to find
    // the given error messages among the, where the number of expect error
    // messages is less than the number of expected errors.
    //
    ExpectedErrors(
        const TfCallContext &callContext,
        const size_t numErrors,
        const std::set<std::string> &expectedErrors)
        : _callContext(callContext)
        , _expectedErrors(expectedErrors)
        , _numErrors(numErrors)
    {
        TF_AXIOM(_expectedErrors.size() < _numErrors);
    }

    // The destructor is where we actually verify that the expected errors were
    // emitted.
    //
    ~ExpectedErrors() {
        const size_t numErrors = std::distance(_mark.begin(), _mark.end());

        // If all that is required is an expected number of errors, return if
        // the count matches.
        if (_expectedErrors.empty() && numErrors == _numErrors) {
            return;
        }

        if (numErrors != _numErrors) {
            // Make a vector, and not a set, to make the error message clear
            // when the same error is emitted more than once.
            std::vector<std::string> errors;
            for (auto it=_mark.begin(); it!=_mark.end(); ++it) {
                errors.push_back(it->GetCommentary());
            }

            TF_FATAL_ERROR(
                "Expected numErrors == %zu; got %zu:\n  %s\n"
                "in %s at line %zu of %s",
                _numErrors, numErrors,
                TfStringJoin(errors.begin(), errors.end(), "\n  ").c_str(),
                _callContext.GetFunction(),
                _callContext.GetLine(),
                _callContext.GetFile());
        }

        std::set<std::string> errors;
        for (auto it=_mark.begin(); it!=_mark.end(); ++it) {
            errors.insert(it->GetCommentary());
        }

        std::set<std::string> missingErrors, unexpectedErrors;
        std::set_difference(
            _expectedErrors.begin(), _expectedErrors.end(),
            errors.begin(), errors.end(),
            std::inserter(missingErrors, missingErrors.begin()));
        std::set_difference(
            errors.begin(), errors.end(),
            _expectedErrors.begin(), _expectedErrors.end(),
            std::inserter(unexpectedErrors, unexpectedErrors.begin()));

        // If the number of expected errors is greater than the number of
        // expected error messages, then we have a certain number of unexpected
        // errors that we actually expect.
        const size_t numExpectedUnexpectedErrors =
            _numErrors > _expectedErrors.size()
            ? (_numErrors - _expectedErrors.size()) : 0;

        if (!missingErrors.empty() ||
            unexpectedErrors.size() != numExpectedUnexpectedErrors) {
            std::string errorMessage =
                "Emitted errors differed from expected errors:\n";

            if (!missingErrors.empty()) {
                errorMessage += TfStringPrintf(
                    "Missing:\n  %s\n",
                    TfStringJoin(missingErrors, "\n  ").c_str());
            }
            if (unexpectedErrors.size() != numExpectedUnexpectedErrors) {
                errorMessage += TfStringPrintf(
                    "Unexpected:\n  %s\n",
                    TfStringJoin(unexpectedErrors, "\n  ").c_str());
            }

            errorMessage +=
                TfStringPrintf(
                    "\nin %s at line %zu of %s",
                    _callContext.GetFunction(),
                    _callContext.GetLine(),
                    _callContext.GetFile());
            TF_FATAL_ERROR("%s", errorMessage.c_str());
        }

        _mark.Clear();
    }

private:
    const TfCallContext _callContext;
    const std::set<std::string> _expectedErrors;
    const size_t _numErrors;
    TfErrorMark _mark;
};

#define EXPECTED_ERRORS(name, ...)                                              \
    ExpectedErrors name(TF_CALL_CONTEXT, __VA_ARGS__)

static EsfStage
_NewStageFromLayer(
    const char *const layerContents)
{
    const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    layer->ImportFromString(layerContents);
    TF_AXIOM(layer);
    const UsdStageRefPtr usdStage = UsdStage::Open(layer);
    TF_AXIOM(usdStage);
    return EsfUsdSceneAdapter::AdaptStage(usdStage);
}

static void
_PrintInputKeys(
    const TfSmallVector<Exec_InputKey, 1> &inputKeys)
{
    std::cout << "\nPrinting " << inputKeys.size() << " input keys:\n";

    for (const Exec_InputKey &key : inputKeys) {
        std::cout << "\nkey:\n";
        std::cout << "  input name: " << key.inputName << "\n";
        std::cout << "  computation name: " << key.computationName << "\n";
        std::cout << "  result type: " << key.resultType << "\n";
        std::cout << "  local traversal path: "
                  << key.providerResolution.localTraversal << "\n";
        std::cout << "  traversal: "
                  << static_cast<int>(key.providerResolution.dynamicTraversal)
                  << "\n";
        std::cout << "  optional: " << key.optional << "\n";
    }

    std::cout << std::flush;
}

// This test case needs to run first in order to encounter the errors we look
// for here.
//
static void
TestRegistrationErrors()
{
    // The errors that are emitted because of conflicting plugins aren't stable
    // because order can vary, so they are not included among the expected error
    // messages here.
    EXPECTED_ERRORS(expected, 7, {
        "Attempt to register computation 'unknownSchemaTypeComputation' using "
        "an unknown type.",

        "Attempt to register computation '__computeTime' with a name that uses "
        "the prefix '__', which is reserved for builtin computations.",

        "Attempt to register computation 'nonComputationalSchemaComputation' "
        "for schema TestExecNonComputationalSchema, which was declared as "
        "not allowing plugin computations by plugin "
        "'TestExecPluginComputation'.",

        "Unknown schema type name 'UnknownSchemaType' encountered when reading "
        "Exec plugInfo."
    });

    // The first time we pull on the defintion registry, errors for bad
    // registrations are emitted.
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    EsfJournal *const nullJournal = nullptr;

    {
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def ConflictingPluginRegistrationSchema "Prim"
        {
        }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim,
                TfToken("conflictingRegistrationComputation"),
                EsfSchemaConfigKey(),
                nullJournal);
        TF_AXIOM(primCompDef);
    }

    {
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
            def Scope "Prim" (
                apiSchemas = ["NonComputationalSchema"]
            ) {
            }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim,
                TfToken("nonComputationalSchemaComputation"),
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(!primCompDef);
    }

    {
        // Make sure we don't find a computation that was registered on a
        // schema with conflicting allowsPluginComputations plugInfo.
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def ConflictingComputationalSchema "Prim"
        {
        }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim,
                TfToken("nonComputationalSchemaComputation"),
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(!primCompDef);
    }
}

// Test that an unknown applied schema is ignored and we still find computations
// registered for an applied schema.
//
static void
TestUnknownSchemaType()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def TestUnknownSchemaType "Prim" (
            apiSchemas = ["CustomAppliedSchema"]
        ) {
        }
    )usd");
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    {
        // Look up a computation registered for the applied schema type.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->appliedSchemaComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);
    }
}

// Test that attempts to look up builtin stage computations on prims (other
// than the pseudo-root) are rejected.
//
static void
TestStageBuiltinComputationOnPrim()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def TestUnknownSchemaType "Prim" {
        }
    )usd");
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    const Exec_ComputationDefinition *const primCompDef =
        reg.GetComputationDefinition(
            *prim, ExecBuiltinComputations->computeTime,
            EsfSchemaConfigKey(), nullJournal);
    TF_AXIOM(!primCompDef);
}

static void
TestTypedSchemaComputationRegistration()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");
    const EsfPrim pseudoroot = stage->GetPrimAtPath(SdfPath("/"), nullJournal);
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    {
        // Look up a computation that wasn't registered.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->missingComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(!primCompDef);
    }

    {
        // Look up a computation with no callback or inputs.
        //
        // (Once we support composition of computation definitions, we will
        // want some kind of validation to ensure we end up with a callback.)
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->emptyComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        ASSERT_EQ(
            primCompDef->GetInputKeys(*prim, nullJournal)->Get().size(),
            0);
    }

    {
        // Look up a computation with no inputs.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->noInputsComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        ASSERT_EQ(
            primCompDef->GetInputKeys(*prim, nullJournal)->Get().size(),
            0);
    }

    {
        // Look up a stage bultin computation.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *pseudoroot, ExecBuiltinComputations->computeTime,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        ASSERT_EQ(
            primCompDef->GetInputKeys(*prim, nullJournal)->Get().size(),
            0);
    }

    {
        // Look up a plugin computation on the stage pseudo-root.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *pseudoroot, _tokens->noInputsComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(!primCompDef);
    }

    {
        // Look up a computation with multiple inputs.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->primComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 5);

        _PrintInputKeys(inputKeys->Get());

        size_t index = 0;
        {
            const Exec_InputKey &key = inputKeys->Get()[index++];
            ASSERT_EQ(key.inputName, _tokens->primComputation);
            ASSERT_EQ(key.computationName, _tokens->primComputation);
            ASSERT_EQ(key.resultType, TfType::Find<double>());
            ASSERT_EQ(key.providerResolution.localTraversal, SdfPath("."));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, true);
        }

        {
            const Exec_InputKey &key = inputKeys->Get()[index++];
            ASSERT_EQ(key.inputName, _tokens->attributeComputation);
            ASSERT_EQ(key.computationName, _tokens->attributeComputation);
            ASSERT_EQ(key.resultType, TfType::Find<int>());
            ASSERT_EQ(key.providerResolution.localTraversal,
                      SdfPath(".attributeName"));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, true);
        }

        {
            const Exec_InputKey &key = inputKeys->Get()[index++];
            ASSERT_EQ(key.inputName, _tokens->attributeName);
            ASSERT_EQ(
                key.computationName, ExecBuiltinComputations->computeValue);
            ASSERT_EQ(key.resultType, TfType::Find<int>());
            ASSERT_EQ(key.providerResolution.localTraversal,
                      SdfPath(".attributeName"));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::Local);
            ASSERT_EQ(key.optional, false);
        }

        {
            const Exec_InputKey &key = inputKeys->Get()[index++];
            ASSERT_EQ(key.inputName, _tokens->primComputation);
            ASSERT_EQ(key.computationName, _tokens->primComputation);
            ASSERT_EQ(key.resultType, TfType::Find<int>());
            ASSERT_EQ(key.providerResolution.localTraversal,
                      SdfPath(".relationshipName"));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::
                          RelationshipTargetedObjects);
            ASSERT_EQ(key.optional, true);
        }

        {
            const Exec_InputKey &key = inputKeys->Get()[index++];
            ASSERT_EQ(key.inputName, _tokens->namespaceAncestorInput);
            ASSERT_EQ(key.computationName, _tokens->primComputation);
            ASSERT_EQ(key.resultType, TfType::Find<bool>());
            ASSERT_EQ(key.providerResolution.localTraversal, SdfPath("."));
            ASSERT_EQ(key.providerResolution.dynamicTraversal,
                      ExecProviderResolution::DynamicTraversal::
                          NamespaceAncestor);
            ASSERT_EQ(key.optional, true);
        }
    }

    {
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->stageAccessComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 1);

        _PrintInputKeys(inputKeys->Get());

        const Exec_InputKey &key = inputKeys->Get()[0];
        ASSERT_EQ(key.inputName, ExecBuiltinComputations->computeTime);
        ASSERT_EQ(key.computationName, ExecBuiltinComputations->computeTime);
        ASSERT_EQ(key.resultType, TfType::Find<EfTime>());
        ASSERT_EQ(key.providerResolution.localTraversal, SdfPath("/"));
        ASSERT_EQ(key.providerResolution.dynamicTraversal,
                  ExecProviderResolution::DynamicTraversal::Local);
        ASSERT_EQ(key.optional, false);
    }

    {
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->attributeComputedValueComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 1);

        _PrintInputKeys(inputKeys->Get());

        const Exec_InputKey &key = inputKeys->Get()[0];
        ASSERT_EQ(key.inputName, ExecBuiltinComputations->computeValue);
        ASSERT_EQ(key.computationName, ExecBuiltinComputations->computeValue);
        ASSERT_EQ(key.resultType, TfType::Find<double>());
        ASSERT_EQ(key.providerResolution.localTraversal, SdfPath(".attr"));
        ASSERT_EQ(key.providerResolution.dynamicTraversal,
                  ExecProviderResolution::DynamicTraversal::Local);
        ASSERT_EQ(key.optional, true);
    }
}

static void
TestDerivedSchemaComputationRegistration()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def DerivedCustomSchema "Prim" {
        }
    )usd");
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    {
        // Look up a computation registered for the derived schema type.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->derivedSchemaComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);
    }

    {
        // Look up a computation registered for the base and derived schema
        // types.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->baseAndDerivedSchemaComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        // Make sure we got the definition from the derived schema (i.e., the
        // stronger one).
        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 1);
    }

    {
        // Look up a computation registered for the base schema type.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->noInputsComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);
    }
}

static void
TestAppliedSchemaComputationRegistration()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();

    {
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
            def Scope "Prim" (apiSchemas = ["CustomAppliedSchema"]) {
            }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        {
            // Look up a computation registered for the applied schema type.
            const Exec_ComputationDefinition *const primCompDef =
                reg.GetComputationDefinition(
                    *prim, _tokens->appliedSchemaComputation,
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(primCompDef);
        }

        {
            // Look up another computation, which is registered for the
            // applied schema, with no inputs.
            const Exec_ComputationDefinition *const primCompDef =
                reg.GetComputationDefinition(
                    *prim, _tokens->primComputation,
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(primCompDef);
            const auto inputKeys =
                primCompDef->GetInputKeys(*prim, nullJournal);
            ASSERT_EQ(inputKeys->Get().size(), 0);
        }
    }

    {
        // Test computation registrations for an API schema that's applied to
        // a prim that also has a typed schema with computation registrations.
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
            def CustomSchema "Prim" (apiSchemas = ["CustomAppliedSchema"]) {
            }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        {
            // Look up a computation that is only registered for the applied
            // schema type.
            const Exec_ComputationDefinition *const primCompDef =
                reg.GetComputationDefinition(
                    *prim, _tokens->appliedSchemaComputation,
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(primCompDef);
        }

        {
            // Look up a computation that is also registered for the typed
            // schema and verify that the typed schema wins.
            const Exec_ComputationDefinition *const primCompDef =
                reg.GetComputationDefinition(
                    *prim, _tokens->primComputation,
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(primCompDef);
            const auto inputKeys =
                primCompDef->GetInputKeys(*prim, nullJournal);
            ASSERT_EQ(inputKeys->Get().size(), 5);
        }
    }

    {
        // Test that, for now, we ignore multi-apply schemas during computation
        // lookup.
        const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
            def Scope "Prim" (apiSchemas = ["CustomMultiApplySchema:test"]) {
            }
        )usd");
        const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
        TF_AXIOM(prim->IsValid(nullJournal));

        {
            // Look up a computation registered for the applied schema type.
            const Exec_ComputationDefinition *const primCompDef =
                reg.GetComputationDefinition(
                    *prim, _tokens->multiApplySchemaComputation,
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(!primCompDef);
        }
    }
}

static void
TestPluginSchemaComputationRegistration()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def PluginComputationSchema "Prim"
        {
        }

        def CustomSchema "NonPluginPrim"
        {
        }

        def ExtraPluginComputationSchema "ExtraPrim"
        {
        }
    )usd");
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    {
        EXPECTED_ERRORS(expected, {
            "Attempt to register computation 'unregisteredComputation' for "
            "schema TestExecComputationRegistrationCustomSchema, for which "
            "computation registration has already been completed."
        });

        // Look up a computation registered in a plugin, causing the plugin to
        // be loaded.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, TfToken("myComputation"),
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 2);

        {
            // Make sure we *don't* find the computation that the plugin
            // attempted to register on CustomSchema, for which computations were
            // already registered.
            const EsfPrim prim =
                stage->GetPrimAtPath(SdfPath("/NonPluginPrim"), nullJournal);
            TF_AXIOM(prim->IsValid(nullJournal));

            const Exec_ComputationDefinition *const primDef =
                reg.GetComputationDefinition(
                    *prim, TfToken("unregisteredComputation"),
                    EsfSchemaConfigKey(), nullJournal);
            TF_AXIOM(!primDef);
        }
    }

    {
        // Look up another computation that was registered by the plugin we just
        // loaded.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, TfToken("anotherComputation"),
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(primCompDef);

        const auto inputKeys =
            primCompDef->GetInputKeys(*prim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 1);
    }

    {
        // Look up a computation on a prim with a different schema, which is
        // defined in the same plugin that defines computations for
        // PluginComputationSchema.
        const EsfPrim extraPrim =
            stage->GetPrimAtPath(SdfPath("/ExtraPrim"), nullJournal);
        TF_AXIOM(extraPrim->IsValid(nullJournal));

        const Exec_ComputationDefinition *const extraPrimCompDef =
            reg.GetComputationDefinition(
                *extraPrim, TfToken("myComputation"),
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(extraPrimCompDef);

        const auto inputKeys =
            extraPrimCompDef->GetInputKeys(*extraPrim, nullJournal);
        ASSERT_EQ(inputKeys->Get().size(), 0);
    }
}

static void
TestDispatchedComputations()
{
    EsfJournal *const nullJournal = nullptr;
    const Exec_DefinitionRegistry &reg = Exec_DefinitionRegistry::GetInstance();
    const EsfStage stage = _NewStageFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
        def Scope "Scope" {
        }
    )usd");
    const EsfPrim prim = stage->GetPrimAtPath(SdfPath("/Prim"), nullJournal);
    TF_AXIOM(prim->IsValid(nullJournal));

    const EsfPrim scope = stage->GetPrimAtPath(SdfPath("/Scope"), nullJournal);
    TF_AXIOM(scope->IsValid(nullJournal));

    {
        // Look up a dispatched prim computation, which is keyed off of the
        // schema config key.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->dispatchedPrimComputation,
                prim->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(primCompDef);
    }

    {
        // Attempt to look up a dispatched prim computation that only dispatches
        // onto CustomSchema.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *scope, _tokens->dispatchedPrimComputationOnCustomSchema,
                prim->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(!primCompDef);
    }

    {
        // Look up the same dispatched prim computation on a prim with the
        // matching schema.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->dispatchedPrimComputationOnCustomSchema,
                prim->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(primCompDef);
    }

    {
        // Attempt to look up a dispatched prim computation with a different
        // schema config key.

        TF_AXIOM(prim->GetSchemaConfigKey(nullJournal) !=
                 scope->GetSchemaConfigKey(nullJournal));

        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *scope, _tokens->dispatchedPrimComputation,
                scope->GetSchemaConfigKey(nullJournal), nullJournal);
        TF_AXIOM(!primCompDef);
    }

    {
        // Attempt to look up a dispatched prim computation with an empty
        // schema config key.
        const Exec_ComputationDefinition *const primCompDef =
            reg.GetComputationDefinition(
                *prim, _tokens->dispatchedPrimComputation,
                EsfSchemaConfigKey(), nullJournal);
        TF_AXIOM(!primCompDef);
    }
}

static void
_SetupTestPlugins()
{
    const std::string pluginPath =
        TfStringCatPaths(
            TfGetPathName(ArchGetExecutablePath()),
            "ExecPlugins/lib/TestExec*/Resources/") + "/";

    const PlugPluginPtrVector plugins =
        PlugRegistry::GetInstance().RegisterPlugins(pluginPath);
    
    ASSERT_EQ(plugins.size(), 3);
}

int main()
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins =
        PlugRegistry::GetInstance().RegisterPlugins(TfAbsPath("resources"));
    ASSERT_EQ(testPlugins.size(), 1);
    ASSERT_EQ(testPlugins[0]->GetName(), "testExecComputationRegistration");

    const TfType schemaType =
        TfType::FindByName("TestExecComputationRegistrationCustomSchema");
    TF_AXIOM(!schemaType.IsUnknown());

    _SetupTestPlugins();

    TestRegistrationErrors();
    TestUnknownSchemaType();
    TestStageBuiltinComputationOnPrim();
    TestTypedSchemaComputationRegistration();
    TestDerivedSchemaComputationRegistration();
    TestAppliedSchemaComputationRegistration();
    TestPluginSchemaComputationRegistration();
    TestDispatchedComputations();

    return 0;
}
