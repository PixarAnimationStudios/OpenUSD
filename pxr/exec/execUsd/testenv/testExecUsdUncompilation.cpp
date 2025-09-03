//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/exec/systemDiagnostics.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/namespaceEditor.h"
#include "pxr/usd/usd/prim.h"

#include <optional>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DECLARE_WEAK_PTRS(UsdStage);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (constantComputation)
    (usesAttributeValue)
    (usesNamespaceAncestor)
    (inputName)
    (customAttr)
);

static int
_CommonComputationCallback(const VdfContext &ctx)
{
    const int *const inputValue = ctx.GetInputValuePtr<int>(_tokens->inputName);
    return inputValue ? *inputValue : 0;
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdUncompilationCustomSchema)
{
    self.PrimComputation(_tokens->constantComputation)
        .Callback<int>(+[](const VdfContext &){ return 42; });

    self.PrimComputation(_tokens->usesAttributeValue)
        .Callback<int>(_CommonComputationCallback)
        .Inputs(
            AttributeValue<int>(_tokens->customAttr)
                .InputName(_tokens->inputName));

    self.PrimComputation(_tokens->usesNamespaceAncestor)
        .Callback<int>(_CommonComputationCallback)
        .Inputs(
            NamespaceAncestor<int>(_tokens->constantComputation)
                .InputName(_tokens->inputName));
}

class Fixture
{
public:
    ExecUsdSystem &NewSystemFromLayer(const char *const layerContents) {
        TF_AXIOM(!_system);

        const SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
        layer->ImportFromString(layerContents);
        TF_AXIOM(layer);

        _stage = UsdStage::Open(layer);
        TF_AXIOM(_stage);
        _namespaceEditor.emplace(_stage);
        _system.emplace(_stage);

        return *_system;
    }

    UsdPrim GetPrimAtPath(const char *const pathStr) const {
        return _stage->GetPrimAtPath(SdfPath(pathStr));
    }

    UsdAttribute GetAttributeAtPath(const char *const pathStr) const {
        return _stage->GetAttributeAtPath(SdfPath(pathStr));
    }

    UsdNamespaceEditor &GetNamespaceEditor() {
        return *_namespaceEditor;
    }

    void GraphNetwork(const char *const filename) {
        ExecSystem::Diagnostics diagnostics(&*_system);
        diagnostics.GraphNetwork(filename);
    }

private:
    UsdStageRefPtr _stage;
    std::optional<UsdNamespaceEditor> _namespaceEditor;
    std::optional<ExecUsdSystem> _system;
};

static void
TestUncompileConstantComputation(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Provider1" {
        }

        def CustomSchema "Provider2" {
        }
    )usd");

    ExecUsdRequest request = system.BuildRequest({
        {fixture.GetPrimAtPath("/Provider1"), _tokens->constantComputation},
        {fixture.GetPrimAtPath("/Provider2"), _tokens->constantComputation},
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestUncompileConstantComputation-compiled.dot");

    // Resync Provider1. Nodes contributing to the first value key should be
    // uncompiled, but nodes contributing to the second value key should not.
    fixture.GetPrimAtPath("/Provider1").SetActive(false);

    fixture.GraphNetwork("TestUncompileConstantComputation-uncompiled.dot");
}

static void
TestUncompileAttributeInput(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "AttributeOwner" {
            int customAttr = 42
        }
    )usd");

    ExecUsdRequest request = system.BuildRequest({
        {fixture.GetPrimAtPath("/AttributeOwner"), _tokens->usesAttributeValue},
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestUncompileAttributeInput-compiled.dot");

    // Resync the custom attribute. The provider for 'usesAttributeValue' was
    // not resynced, so its callback node and leaf node will still exist. But,
    // the attribute input node feeding into the callback node should have been
    // uncompiled.
    fixture.GetPrimAtPath("/AttributeOwner")
        .RemoveProperty(_tokens->customAttr);
    fixture.GraphNetwork("TestUncompileAttributeInput-uncompiled.dot");
}

static void
TestUncompileNamespaceAncestorInput(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Ancestor" {
            def Scope "Scope1" {
                def Scope "Scope2" {
                    def CustomSchema "Provider" {
                    }
                }
            }
        }
        def CustomSchema "NewAncestor" {
        }
    )usd");

    const char *const providerPath = "/Ancestor/Scope1/Scope2/Provider";
    ExecUsdRequest request = system.BuildRequest({
        {fixture.GetPrimAtPath(providerPath), _tokens->usesNamespaceAncestor}
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestUncompileNamespaceAncestorInput-compiled.dot");

    // Reparent the provider to a different ancestor. This is a resync on the
    // provider, but not on the original ancestor that provided the input value.
    fixture.GetNamespaceEditor().ReparentPrim(
        fixture.GetPrimAtPath(providerPath),
        fixture.GetPrimAtPath("/NewAncestor"));
    fixture.GetNamespaceEditor().ApplyEdits();
    fixture.GraphNetwork("TestUncompileNamespaceAncestorInput-uncompiled.dot");
}

static void
TestUncompileRecursiveResync(Fixture &fixture)
{
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def Scope "Root1" {
            def CustomSchema "A" {
                def CustomSchema "B" {
                    int customAttr = 10
                }
            }
        }
        def Scope "Root2" {
            def CustomSchema "C" {
            }
        }
    )usd");

    ExecUsdRequest request = system.BuildRequest({
       {fixture.GetPrimAtPath("/Root1/A/B"), _tokens->usesAttributeValue},
       {fixture.GetPrimAtPath("/Root1/A/B"), _tokens->constantComputation},
       {fixture.GetPrimAtPath("/Root1/A/B"), _tokens->usesNamespaceAncestor},
       {fixture.GetPrimAtPath("/Root2/C"), _tokens->constantComputation} 
    });
    system.PrepareRequest(request);
    fixture.GraphNetwork("TestUncompileRecursiveResync-compiled.dot");

    // Trigger a recursive resync on /Root1. This implies resyncs on all
    // descendants of /Root1. This will uncompile nodes for computations whose
    // providers are descendants of /Root1. But, nodes whose providers are
    // descendants of /Root2 should be unaffected.
    fixture.GetPrimAtPath("/Root1").SetActive(false);
    fixture.GraphNetwork("TestUncompileRecursiveResync-uncompiled.dot");
}

int main()
{
    // Load the custom schema.
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
    .RegisterPlugins(TfAbsPath("resources"));
    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdUncompilation");

    const TfType customSchemaType =
        TfType::FindByName("TestExecUsdUncompilationCustomSchema");
    TF_AXIOM(!customSchemaType.IsUnknown());

    std::vector tests {
        TestUncompileConstantComputation,
        TestUncompileAttributeInput,
        TestUncompileNamespaceAncestorInput,
        TestUncompileRecursiveResync
    };
    for (const auto &test : tests) {
        Fixture fixture;
        test(fixture);
    }
}
