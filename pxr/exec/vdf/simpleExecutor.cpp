//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/simpleExecutor.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/dataManagerHashTable.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorFactoryBase.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/speculationExecutor.h"
#include "pxr/exec/vdf/speculationExecutorEngine.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfSpeculationNode;

namespace {

// Simple executor factory.
struct _SimpleExecutorFactory : public VdfExecutorFactoryBase {
    // The speculation executor type to manufacture.
    using SpeculationExecutorType =
        VdfSpeculationExecutor<
            VdfSpeculationExecutorEngine,
            VdfDataManagerHashTable>;

    // Manufacture a child executor
    std::unique_ptr<VdfExecutorInterface>
    ManufactureChildExecutor(
        const VdfExecutorInterface *parentExecutor) const final {
        TF_CODING_ERROR("Cannot manufacture child from VdfSimpleExecutor.");
        return std::unique_ptr<VdfExecutorInterface>();
    }

    // Manufacture a speculation executor
    std::unique_ptr<VdfSpeculationExecutorBase>
    ManufactureSpeculationExecutor(
        const VdfSpeculationNode *speculationNode,
        const VdfExecutorInterface *parentExecutor) const final {
        return std::unique_ptr<VdfSpeculationExecutorBase>(
            new SpeculationExecutorType(speculationNode, parentExecutor));
    }
};

}

VdfSimpleExecutor::~VdfSimpleExecutor()
{
}

const VdfExecutorFactoryBase &
VdfSimpleExecutor::GetFactory() const
{
    static const _SimpleExecutorFactory _factory;
    return _factory;
}

void
VdfSimpleExecutor::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{
    TRACE_FUNCTION();

    _dataManager.Resize(*schedule.GetNetwork());

    VdfEvaluationState state(*this, schedule, errorLogger);

    const VdfSchedule::ScheduleNodeVector &scheduleNodes =
        schedule.GetScheduleNodeVector();
    for (const VdfScheduleNode &scheduleNode : scheduleNodes) {
        // Make sure we reclaim all caches that we are about to re-execute.
        const VdfNode &node = *scheduleNode.node;
        VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
            const VdfOutput &output = *schedule.GetOutput(outputId);

            // Touch the output.
            _dataManager.Touch(output.GetId());

            // Create a data handle for the output.
            const _DataHandle dataHandle =
                _dataManager.GetOrCreateDataHandle(output.GetId());

            // Reset the private buffer and apply the request mask.
            const VdfMask &requestMask = schedule.GetRequestMask(outputId);
            VdfExecutorBufferData *bufferData =
                _dataManager.GetPrivateBufferData(dataHandle);
            bufferData->ResetExecutorCache(requestMask);

            // Pass down read/write buffers before executing the callback.
            if (const VdfInput *ai = output.GetAssociatedInput()) {
                _PrepareReadWriteBuffer(bufferData, *ai, requestMask, schedule);
            }
        }

        // Compute the node.
        node.Compute(VdfContext(state, node));

        // Publish each one of the computed outputs.
        VDF_FOR_EACH_SCHEDULED_OUTPUT_ID(outputId, schedule, node) {
            const _DataHandle dataHandle = _dataManager.GetDataHandle(
                schedule.GetOutput(outputId)->GetId());
            _dataManager.PublishPrivateBufferData(dataHandle);
        }
    }
}

void 
VdfSimpleExecutor::_PrepareReadWriteBuffer(
    VdfExecutorBufferData *bufferData,
    const VdfInput &input,
    const VdfMask &mask,
    const VdfSchedule &schedule)
{
    if (bufferData->GetExecutorCache()) {
        return;
    }

    const VdfOutput *output = input.GetAssociatedOutput();
    if (!TF_VERIFY(output)) {
        return;
    }

    // Always create a new output cache and make a copy for read/write buffers.
    // The simple executor does not support buffer passing.
    VdfVector *value = _dataManager.CreateOutputCache(*output, bufferData);

    const size_t numInputNodes = input.GetNumConnections();
    if (numInputNodes == 1 && !input[0].GetMask().IsAllZeros()) {
        const VdfVector *sourceData = _dataManager.GetOutputValueForReading(
            _dataManager.GetDataHandle(input[0].GetSourceOutput().GetId()),
            mask);
        TF_AXIOM(sourceData);

        *value = *sourceData;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
