//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/parallelExecutorDataVector.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(double(0));
}

using DataHandle = Vdf_ParallelExecutorDataVector::DataHandle;

constexpr static DataHandle InvalidHandle = DataHandle(-1); // _LocationInvalid
constexpr static DataHandle PendingHandle = DataHandle(-2); // _LocationPending

bool _IsValidDataHandle(Vdf_ParallelExecutorDataVector::DataHandle handle) {
    return !(handle == InvalidHandle || handle == PendingHandle);
}

int main() {

    WorkSetMaximumConcurrencyLimit();

    const size_t nodes = 50000;

    // instantiate network
    VdfTestUtils::Network graph;
    VdfNetwork &network = graph.GetNetwork();

    // allocate vectors
    std::vector<VdfVector *> vectors(nodes);

    const TfType &doubleType = TfType::Find<double>();
    for (size_t i = 0; i < nodes; i++) {
        // allocate nodes
        VdfInputSpecs inputSpecs;
        VdfOutputSpecs outputSpecs;
        inputSpecs.ReadConnector(doubleType, TfToken("dummy"));
        outputSpecs.Connector(doubleType, TfToken("dummy"));
        VdfNode * node = new VdfSpeculationNode(&network, inputSpecs, outputSpecs);
        new VdfOutput(*node, 0);

        // allocate vectors
        vectors[i] = new VdfTypedVector<double>(double(i));
    }

    // size data vectors properly
    Vdf_ParallelExecutorDataVector dataVector;
    dataVector.Resize(network);
    // instantiate location and output buffers
    WorkParallelForN(
        nodes,
        [&dataVector](size_t begin, size_t end) {
            for (size_t i = begin; i < end; i++) {
                VdfId id = (size_t(0) << 32 | i);
                DataHandle createdHandle = dataVector.GetOrCreateDataHandle(id);
                TF_AXIOM(_IsValidDataHandle(createdHandle));
            }
        });

    // Setup a parallel tbb task so that 4 threads can potentially pound
    // on a single location at once, with one thread actually filling
    // the buffers at that location with data
    const size_t iters = nodes * 4;
    WorkParallelForN(
        iters,
        [&dataVector,&vectors,nodes](size_t begin, size_t end) {
            for (size_t i = begin; i < end; i++) {
                size_t idx = i;
                while (idx >= nodes) {
                    idx -= nodes;
                }
                VdfId id = (size_t(1) << 32 | idx);
                DataHandle createdHandle = dataVector.GetOrCreateDataHandle(id);
                DataHandle gotHandle = dataVector.GetDataHandle(id);
                TF_AXIOM(_IsValidDataHandle(gotHandle));
                TF_AXIOM(createdHandle == gotHandle);
                if (i < nodes) {
                    // Fill this location with data
                    VdfExecutorBufferData * buffer =
                        dataVector.GetPrivateBufferData(gotHandle);
                    buffer->TakeOwnership(vectors[i]);
                }
            }
        });
    for (size_t i = 0; i < nodes; i++) {
        // check that the vectors aren't de-initialized
        VdfVector::ReadAccessor<double> accessor = 
            vectors[i]->GetReadAccessor<double>();
        TF_AXIOM(accessor[0] == double(i));
    }
    std::cout << "done" << std::endl;

}
