//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_SUB_EXECUTOR_H
#define PXR_EXEC_EF_SUB_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/vdf/dataManagerBasedSubExecutor.h"
#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/speculationExecutor.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;
class VdfSchedule;

///////////////////////////////////////////////////////////////////////////////
//
// \class EfSubExecutor.
//
// \brief Executed a VdfNetwork to compute a requested set of value, and uses
//        cached output values from a parent executor, if unavailable in the
//        local data manager.
//
template <
    template <typename> class EngineType,
    typename DataManagerType>
class EfSubExecutor : 
    public VdfDataManagerBasedSubExecutor<DataManagerType, VdfExecutorInterface>
{
    // Base type definition
    typedef
        VdfDataManagerBasedSubExecutor<DataManagerType, VdfExecutorInterface>
        Base;

    // The speculation executor engine alias declaration, to be bound as a 
    // template template parameter.
    template <typename T>
    using SpeculationEngineType =
        typename EngineType<T>::SpeculationExecutorEngine;

    // Executor factory.
    typedef
        VdfExecutorFactory<
            EfSubExecutor<EngineType, DataManagerType>,
            VdfSpeculationExecutor<SpeculationEngineType, DataManagerType>>
        _Factory;

public:

    /// Default constructor
    ///
    EfSubExecutor() :
        _engine(*this, &this->_dataManager)
    { }

    /// Construct with a parent executor
    ///
    explicit EfSubExecutor(const VdfExecutorInterface *parentExecutor) :
        Base(parentExecutor),
        _engine(*this, &this->_dataManager)
    { }

    /// Destructor
    ///
    virtual ~EfSubExecutor() {}

    /// Factory construction.
    ///
    virtual const VdfExecutorFactoryBase &GetFactory() const override final {
        return _factory;
    }

protected:

    /// Run this executor with the given \p schedule and \p request.
    ///
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger);

private:
    
    /// Clears the data in the data manager.
    ///
    void _ClearData();

    // The factory shared amongst executors of this type.
    //
    static const _Factory _factory;

    // This is the engine that will do most of our hard work for us.
    //
    EngineType<DataManagerType> _engine;

};

///////////////////////////////////////////////////////////////////////////////

template <template <typename> class EngineType, typename DataManagerType>
const typename EfSubExecutor<EngineType, DataManagerType>::_Factory
    EfSubExecutor<EngineType, DataManagerType>::_factory;

template <template <typename> class EngineType, typename DataManagerType>
void
EfSubExecutor<EngineType, DataManagerType>::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{
    // If we have an empty request, bail out.
    if (computeRequest.IsEmpty()) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfSubExecutor::Run");

    _engine.RunSchedule(schedule, computeRequest, errorLogger);
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfSubExecutor<EngineType, DataManagerType>::_ClearData()
{
    TRACE_FUNCTION();

    // If the data manager is empty, don't even attempt to clear it.
    if (!Base::_dataManager.IsEmpty()) {
        Base::_dataManager.Clear();
    }

    Base::InvalidateTopologicalState();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
