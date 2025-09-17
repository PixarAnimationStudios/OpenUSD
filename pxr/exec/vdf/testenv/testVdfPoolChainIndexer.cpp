//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (childPoints)
    (pool)
);


static void
CallbackFunction(const VdfContext &)
{
}

static void
BuildNetwork(VdfTestUtils::Network &graph)
{
    // Create a pair of callback node types that roughly resemble the
    // movers and parallel movers in a pool chain.

    VdfTestUtils::CallbackNodeType moverType(&CallbackFunction);
    moverType
        .ReadWrite<int>(_tokens->pool, _tokens->pool)
        ;

    VdfTestUtils::CallbackNodeType parallelMoverType(&CallbackFunction);
    parallelMoverType
        .ReadWrite<int>(_tokens->pool, _tokens->pool)
        .Read<int>(_tokens->childPoints)
        ;

    // Pool chain indexing doesn't consider masks (other than checking if an
    // output has an affects mask), so just use the same one for everything in
    // this test.
    const VdfMask mask = VdfMask::AllOnes(2);

    /* Build a network with a parallel mover where Mover3 & 4
     * feed into the childPoints of the parallel mover.
     * 
     *
     *     Mover1
     *         |
     *     Mover2
     *        /|\
     *       / | \
     *  Mover3 |  \
     *    /    |  |
     *   | Mover4 |
     *   |     |  |
     *    \   /   |
     *     \ /   /
     *  ParallelMover
     *          |
     *      Mover5
     *
     */

    // Don't create the "movers" in the same order as the expected pool chain
    // index order, since if we do so many of the pool chain index relationships
    // that we test for here will be true, just by virtue of the order we
    // create the outputs were created in.
    graph.Add("Mover5", moverType);
    VdfTestUtils::Node &mover5 = graph["Mover5"];
    mover5.GetVdfNode()->GetOutput()->SetAffectsMask(mask);

    graph.Add("Mover4", moverType);
    VdfTestUtils::Node &mover4 = graph["Mover4"];
    mover4.GetVdfNode()->GetOutput()->SetAffectsMask(mask);

    // We don't set affects masks for some movers, since pool index order
    // shouldn't require affects masks.
    graph.Add("Mover3", moverType);
    VdfTestUtils::Node &mover3 = graph["Mover3"];

    graph.Add("Mover2", moverType);
    VdfTestUtils::Node &mover2 = graph["Mover2"];

    graph.Add("Mover1", moverType);
    VdfTestUtils::Node &mover1 = graph["Mover1"];
    mover1.GetVdfNode()->GetOutput()->SetAffectsMask(mask);

    graph.Add("ParallelMover", parallelMoverType);
    VdfTestUtils::Node &parallelMover = graph["ParallelMover"];
    parallelMover.GetVdfNode()->GetOutput()->SetAffectsMask(mask);

    mover1.Output(_tokens->pool)
        >> mover2.In(_tokens->pool, mask);

    // Connect Mover2's pool output to the 3 targets:
    // (Mover3, Mover4, ParallelMover)
    mover2.Output(_tokens->pool)
        >> mover3.In(_tokens->pool, mask);
    mover2.Output(_tokens->pool)
        >> mover4.In(_tokens->pool, mask);
    mover2.Output(_tokens->pool)
        >> parallelMover.In(_tokens->pool, mask);

    // Connect childPoints into the parallel mover
    mover3.Output(_tokens->pool)
        >> parallelMover.In(_tokens->childPoints, mask);
    mover4.Output(_tokens->pool)
        >> parallelMover.In(_tokens->childPoints, mask);

    // Connect the Mover5 downstream of ParallelMover
    parallelMover.Output(_tokens->pool)
        >> mover5.In(_tokens->pool, mask);
}

int
main(int argc, char *argv[])
{
    // Test that pool chain indexing places movers in child branches of a
    // parallel mover before the parallel mover in the pool chain index order.

    VdfTestUtils::Network graph;
    BuildNetwork(graph);

    const VdfNetwork &network = graph.GetNetwork();

    VdfPoolChainIndex mover1Index =
        network.GetPoolChainIndex(*graph["Mover1"].GetOutput());
    VdfPoolChainIndex mover2Index =
        network.GetPoolChainIndex(*graph["Mover2"].GetOutput());
    VdfPoolChainIndex mover3Index =
        network.GetPoolChainIndex(*graph["Mover3"].GetOutput());
    VdfPoolChainIndex mover4Index =
        network.GetPoolChainIndex(*graph["Mover4"].GetOutput());
    VdfPoolChainIndex mover5Index =
        network.GetPoolChainIndex(*graph["Mover5"].GetOutput());

    VdfPoolChainIndex parallelMoverIndex =
        network.GetPoolChainIndex(*graph["ParallelMover"].GetOutput());

    TF_AXIOM(mover1Index < mover2Index);

    TF_AXIOM(mover2Index < mover3Index);
    TF_AXIOM(mover2Index < mover4Index);
    TF_AXIOM(mover2Index < parallelMoverIndex);

    TF_AXIOM(mover3Index < parallelMoverIndex);
    TF_AXIOM(mover4Index < parallelMoverIndex);
    TF_AXIOM(mover3Index != mover4Index);

    TF_AXIOM(parallelMoverIndex < mover5Index);

    return 0;
}
