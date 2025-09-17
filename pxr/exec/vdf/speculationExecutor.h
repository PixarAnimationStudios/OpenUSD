//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPECULATION_EXECUTOR_H
#define PXR_EXEC_VDF_SPECULATION_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/dataManagerBasedSubExecutor.h"
#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/speculationExecutorBase.h"
#include "pxr/exec/vdf/speculationExecutorEngine.h"
#include "pxr/exec/vdf/speculationNode.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSpeculationExecutor
///
/// \brief Executor used in speculation.
///

template <
    template <typename> class EngineType,
    typename DataManagerType>
class VdfSpeculationExecutor :
    public VdfDataManagerBasedSubExecutor <
        DataManagerType,
        VdfSpeculationExecutorBase >
{
    // Base class type
    typedef 
        VdfDataManagerBasedSubExecutor <
            DataManagerType,
            VdfSpeculationExecutorBase >
        Base;

    // Executor factory.
    typedef
        VdfExecutorFactory<
            VdfSpeculationExecutor<EngineType, DataManagerType>,
            VdfSpeculationExecutor<EngineType, DataManagerType>>
        _Factory;

public:

    /// Constructs a speculation executor that was initiated from
    /// \p speculationNode while being computed by \p parentExecutor.
    ///
    VdfSpeculationExecutor(
        const VdfSpeculationNode *speculationNode,
        const VdfExecutorInterface *parentExecutor);

    /// Construct a speculation executor with the given \p parentExecutor,
    /// without registering a speculation node for cycle detection.
    ///
    explicit VdfSpeculationExecutor(
        const VdfExecutorInterface *parentExecutor) :
        VdfSpeculationExecutor(nullptr, parentExecutor)
    {}

    /// Destructor
    ///
    virtual ~VdfSpeculationExecutor() {}

    /// Set the cached value for a given \p output.
    ///
    virtual void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask) override final;

    /// Transfers ownership of \p value to the given \p output.
    ///
    virtual bool TakeOutputValue(
        const VdfOutput &output,
        VdfVector *value,
        const VdfMask &mask) override final;

    /// Returns the factory instance for this executor.
    ///
    virtual const VdfExecutorFactoryBase &GetFactory() const override final {
        return _factory;
    }

private:

    // Run this executor with the given \p schedule and \p request.
    //
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger) override final;

    // Mark the output as having been visited. On speculation executors,
    // we don't need to do anything other than notify the parent. Since this
    // executor is temporary, we never do invalidation on it, and therefore
    // can safely avoid doing the touching on the local data manager.
    //
    virtual void _TouchOutput(const VdfOutput &output) const override final {
        Base::GetNonSpeculationParentExecutor()->_TouchOutput(output);
    }

private:

    // The factory instance.
    //
    static const _Factory _factory;

    // This is the engine that will do most of our hard work for us.
    //
    EngineType<DataManagerType> _engine;

};

//////////////////////////////////////////////////////////////////////////////

template <template <typename> class EngineType, typename DataManagerType>
const typename VdfSpeculationExecutor<EngineType, DataManagerType>::_Factory
    VdfSpeculationExecutor<EngineType, DataManagerType>::_factory;

template <template <typename> class EngineType, typename DataManagerType>
VdfSpeculationExecutor<EngineType, DataManagerType>::VdfSpeculationExecutor(
    const VdfSpeculationNode *speculationNode,
    const VdfExecutorInterface *parentExecutor) :
    Base(parentExecutor),
    _engine(*this, &this->_dataManager)
{
    // Apply the executors speculation node.
    Base::_SetSpeculationNode(speculationNode);

    // Create sub stats on the parent executor and set them on this speculation
    // executor. Set a nullptr if the parent does not have stats itself, or if
    // speculationNode is a nullptr.
    VdfExecutionStats *stats = parentExecutor->GetExecutionStats();
    VdfExecutionStats *subStats = stats && speculationNode
        ? stats->AddSubStat(&speculationNode->GetNetwork(), speculationNode)
        : stats;
    Base::SetExecutionStats(subStats);

    // Propagate the interruption flag from the parent executor to the
    // speculation executor. This must be done in order to ensure that when the
    // parent executor has been interrupted, execution will also be interrupted
    // on the speculation executor.
    Base::SetInterruptionFlag(parentExecutor->GetInterruptionFlag());
}

template <template <typename> class EngineType, typename DataManagerType>
void
VdfSpeculationExecutor<EngineType, DataManagerType>::SetOutputValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    // Call into the base class to set the output value.
    Base::SetOutputValue(output, value, mask);

    // Make sure to also touch the output on the non-speculation-parent.
    _TouchOutput(output);
}

template <template <typename> class EngineType, typename DataManagerType>
bool
VdfSpeculationExecutor<EngineType, DataManagerType>::TakeOutputValue(
    const VdfOutput &output,
    VdfVector *value,
    const VdfMask &mask)
{
    // Call into the base class to take the output value.
    const bool success = Base::TakeOutputValue(output, value, mask);

    // Make sure to also touch the output on the non-speculation-parent.
    _TouchOutput(output);

    return success;
}

template <template <typename> class EngineType, typename DataManagerType>
void
VdfSpeculationExecutor<EngineType, DataManagerType>::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{
    TRACE_FUNCTION();

    TF_VERIFY(Base::GetParentExecutor());

    _engine.RunSchedule(schedule, computeRequest, errorLogger);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
