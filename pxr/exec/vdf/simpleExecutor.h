//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SIMPLE_EXECUTOR_H
#define PXR_EXEC_VDF_SIMPLE_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/dataManagerBasedExecutor.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/parallelDataManagerVector.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorBufferData;
class VdfExecutorErrorLogger;
class VdfExecutorFactoryBase;
class VdfSchedule;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSimpleExecutor
///
/// \brief Executes a VdfNetwork to compute a requested set of values using
///        depth first search.
///
class VDF_API_TYPE VdfSimpleExecutor : 
    public VdfDataManagerBasedExecutor<
        VdfParallelDataManagerVector,
        VdfExecutorInterface>
{
public:
    /// Destructor
    ///
    VDF_API
    virtual ~VdfSimpleExecutor();

    /// Factory construction.
    ///
    VDF_API
    virtual const VdfExecutorFactoryBase &GetFactory() const override final;

protected:

    /// The data manager type used by this executor.
    ///
    using DataManagerType = VdfParallelDataManagerVector ;

    /// The data handle type defined by the data manager.
    ///
    using _DataHandle = typename DataManagerType::DataHandle;

    /// Executes the \p schedule.
    ///
    /// VdfSimpleExecutor ignores the computeRequest and computes all the
    /// outputs in the schedule.
    ///
    VDF_API
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger);

    /// Prepares a buffer to be used as a read/write output.
    ///
    VDF_API
    void _PrepareReadWriteBuffer(
        VdfExecutorBufferData *bufferData,
        const VdfInput &input,
        const VdfMask &mask,
        const VdfSchedule &schedule);

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
