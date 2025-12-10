//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execUsd/system.h"
#include "pxr/exec/execUsd/request.h"
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/registerSchema.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/stage.h"

#include <sstream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    
    (cyclicComputation)
    (cyclicComputationPairA)
    (cyclicComputationPairB)
    (customRel)
    (cyclicRelComputation)
    (cyclicAncestorComputation)
    (ancestor)
);

static void
ConfigureTestPlugin()
{
    const PlugPluginPtrVector testPlugins = PlugRegistry::GetInstance()
        .RegisterPlugins(TfAbsPath("resources"));

    TF_AXIOM(testPlugins.size() == 1);
    TF_AXIOM(testPlugins[0]->GetName() == "testExecUsdCycleDetection");
}

EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(TestExecUsdCycleDetectionCustomSchema)
{
    // This computation consumes itself as input.
    self.PrimComputation(_tokens->cyclicComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputation);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputation));

    // The following 2 computations consume each other as input.
    self.PrimComputation(_tokens->cyclicComputationPairA)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputationPairB);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputationPairB));

    self.PrimComputation(_tokens->cyclicComputationPairB)
        .Callback<int>(+[](const VdfContext &ctx) {
            return ctx.GetInputValue<int>(_tokens->cyclicComputationPairA);
        })
        .Inputs(Computation<int>(_tokens->cyclicComputationPairA));

    // This computation sources its input by invoking the same computation on
    // the relationship targeted objects. If there is a cycle of relationship
    // targets, then compiling this computation will result in a cycle.
    self.PrimComputation(_tokens->cyclicRelComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            VdfReadIterator<int> readIter(ctx, _tokens->cyclicRelComputation);
            int result = 0;
            for (; !readIter.IsAtEnd(); ++readIter) {
                result += *readIter;
            }
            return result;
        })
        .Inputs(
            Relationship(_tokens->customRel)
                .TargetedObjects<int>(_tokens->cyclicRelComputation));

    // This computation sources inputs by invoking the same computation on its
    // namespace ancestor, and the target of a relationship. If that
    // relationship targets a descendant object, there will be a cycle.
    self.PrimComputation(_tokens->cyclicAncestorComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            const int fallback = 0;
            const int sum =
                *ctx.GetInputValuePtr<int>(_tokens->ancestor, &fallback) +
                *ctx.GetInputValuePtr<int>(_tokens->customRel, &fallback);
            return sum;
        })
        .Inputs(
            NamespaceAncestor<int>(_tokens->cyclicAncestorComputation)
                .InputName(_tokens->ancestor),
            Relationship(_tokens->customRel)
                .TargetedObjects<int>(_tokens->cyclicAncestorComputation)
                .InputName(_tokens->customRel));
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
        _system.emplace(_stage);

        return *_system;
    }

    ExecUsdRequest BuildRequest(
        std::vector<ExecUsdValueKey> &&valueKeys) {
        return _system->BuildRequest(
            std::move(valueKeys));
    }

    UsdRelationship GetRelationshipAtPath(const char *const pathStr) const {
        return _stage->GetRelationshipAtPath(SdfPath(pathStr));
    }

    UsdPrim GetPrimAtPath(const char *const pathStr) const {
        return _stage->GetPrimAtPath(SdfPath(pathStr));
    }

private:
    UsdStageRefPtr _stage;
    std::optional<ExecUsdSystem> _system;
};

// Test that we detect a cycle when a computation sources itself as an input.
static bool
Test_CyclicComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->cyclicComputation}
    });

    system.PrepareRequest(request);
    
    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a pair of computations source eachother as
// inputs.
//
static bool
Test_CyclicComputationPair()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim" {
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim"), _tokens->cyclicComputationPairA}
    });

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a computation sources its input from a
// relationship target, which is part of a cycle of relationship targets.
//
static bool
Test_CyclicRelationshipComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "Prim1" {
            add rel customRel = [</Prim2>]
        }
        def CustomSchema "Prim2" {
            add rel customRel = [</Prim3>]
        }
        def CustomSchema "Prim3" {
            add rel customRel = [</Prim1>]
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim2"), _tokens->cyclicRelComputation}
    });

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a computation sources its input from a
// relationship target, which is part of a cycle of relationship targets. This
// specifically tests the case when the relationship cycle is large, and also
// tests the case where multiple leaf tasks depend on the same cycle.
//
static bool
Test_LargeCyclicRelationshipComputation()
{
    const int SIZE = 500;
    std::ostringstream layer;
    layer << "#usda 1.0\n";

    // Prim[i].customRel targets Prim[i+1].
    for (int i = 0; i < SIZE; ++i) {
        layer << "def CustomSchema \"Prim" << i << "\" {\n"
              << "    add rel customRel = [</Prim" << (i + 1) << ">]\n"
              << "}\n";
    }

    // Prim[SIZE].customRel targets Prim[0].
    layer << "def CustomSchema \"Prim" << SIZE << "\" {\n"
          << "    add rel customRel = [</Prim0>]\n"
          << "}\n";

    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(layer.str().c_str());

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/Prim0"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim100"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim200"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim300"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/Prim400"), _tokens->cyclicRelComputation},
    });

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a computation sources its input from its
// ancestor, but one of those ancestors sources its input from a descendant.
//
static bool
Test_CyclicAncestorComputation()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A" {
            add rel customRel = [</A/B/C>]

            def CustomSchema "B" {

                def CustomSchema "C" {
                }
            }
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicAncestorComputation}
    });

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a previously cycle-free scene introduces
// a cycle by authoring a new relationship target.
//
static bool
Test_CyclicRelationshipComputationAfterRecompile()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A" {
            add rel customRel = [</B>]
        }
        def CustomSchema "B" {
            add rel customRel = [</C>]
        }
        def CustomSchema "C" {
            rel customRel
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A"), _tokens->cyclicRelComputation}
    });

    // There are no cycles in the network. This should compile successfully.
    system.PrepareRequest(request);

    // Make a change such that </C> [cyclicRelComputation] now depends on
    // </A> [cyclicRelComputaiton]. This would introduce a cycle.
    fixture.GetRelationshipAtPath("/C.customRel").AddTarget(SdfPath("/A"));

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

// Test that we detect a cycle when a previously cycle-free scene recompiles
// two separate inputs that end up depending on each other, creating a
// "figure 8" cycle.
//
static bool
Test_CyclicRelationshipComputationFigure8()
{
    Fixture fixture;
    ExecUsdSystem &system = fixture.NewSystemFromLayer(R"usd(#usda 1.0
        def CustomSchema "A1" {
            add rel customRel = [</A2>]
        }
        def CustomSchema "A2" {
            rel customRel
        }
        def CustomSchema "B1" {
            add rel customRel = [</B2>]
        }
        def CustomSchema "B2" {
            rel customRel
        }
    )usd");

    ExecUsdRequest request = fixture.BuildRequest({
        {fixture.GetPrimAtPath("/A1"), _tokens->cyclicRelComputation},
        {fixture.GetPrimAtPath("/B1"), _tokens->cyclicRelComputation}
    });

    // This should compile the following cycle-free network:
    //
    //      [ A2 cyclicRelComputation ]         [ B2 cyclicRelComputation ]
    //                   V                                  V              
    //      [ A1 cyclicRelComputaiton ]         [ B1 cyclicRelComputation ]
    //                   V                                  V              
    //             [ Leaf Node ]                      [ Leaf Node ]        
    //
    system.PrepareRequest(request);

    // Make scene changes that would introduce a cycle in a "figure 8" pattern:
    //
    //                   +----------------+  +---------------+               
    //                   V                |   |              V              
    //      [ A2 cyclicRelComputation ]   |   |  [ B2 cyclicRelComputation ]
    //                   V                |   |              V              
    //      [ A1 cyclicRelComputaiton ]   |   |  [ B1 cyclicRelComputation ]
    //                   V         |      |   |     |        V              
    //             [ Leaf Node ]   +----- | --+     |  [ Leaf Node ]        
    //                                    +---------+                       
    //
    fixture.GetRelationshipAtPath("/A2.customRel").AddTarget(SdfPath("/B1"));
    fixture.GetRelationshipAtPath("/B2.customRel").AddTarget(SdfPath("/A1"));

    system.PrepareRequest(request);

    // Cycle detection should abort the process before reaching the end.
    return false;
}

TF_ADD_REGTEST(CyclicComputation);
TF_ADD_REGTEST(CyclicComputationPair);
TF_ADD_REGTEST(CyclicRelationshipComputation);
TF_ADD_REGTEST(LargeCyclicRelationshipComputation);
TF_ADD_REGTEST(CyclicAncestorComputation);
TF_ADD_REGTEST(CyclicRelationshipComputationAfterRecompile);
TF_ADD_REGTEST(CyclicRelationshipComputationFigure8);

int main(int argc, char **argv)
{
    ConfigureTestPlugin();
    return TfRegTest::Main(argc, argv);
}
