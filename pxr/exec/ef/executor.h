//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_EXECUTOR_H
#define PXR_EXEC_EF_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/subExecutor.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/vdf/dataManagerBasedExecutor.h"
#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/speculationExecutor.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;
class VdfSchedule;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfExecutor
///
/// \brief Executes a VdfNetwork to compute a requested set of values.
///
///
template <
    template <typename> class EngineType,
    typename DataManagerType>
class EfExecutor : 
    public VdfDataManagerBasedExecutor<DataManagerType, VdfExecutorInterface>
{
    // Base class type.
    typedef
        VdfDataManagerBasedExecutor<DataManagerType, VdfExecutorInterface>
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

    /// Default constructor.
    ///
    EfExecutor() : _engine(*this, &this->_dataManager) {}

    /// Destructor.
    ///
    virtual ~EfExecutor() {}

    /// Factory construction.
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
        VdfExecutorErrorLogger *errorLogger);

    // Causes the DataManager to clear its temporary execution buffers.
    //
    void _ClearData();

private:

    // The factory shared amongst executors of this type.
    //
    static const _Factory _factory;

    // This is the engine that will do most of our hard work for us.
    //
    EngineType<DataManagerType> _engine;
};

///////////////////////////////////////////////////////////////////////////////

template <template <typename> class EngineType, typename DataManagerType>
const typename EfExecutor<EngineType, DataManagerType>::_Factory
    EfExecutor<EngineType, DataManagerType>::_factory;

template <template <typename> class EngineType, typename DataManagerType>
void
EfExecutor<EngineType, DataManagerType>::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{    
    // If we have an empty request, bail.
    if (computeRequest.IsEmpty()) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfExecutor::Run");

    _engine.RunSchedule(schedule, computeRequest, errorLogger);
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfExecutor<EngineType, DataManagerType>::_ClearData()
{
    Base::_dataManager.Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
