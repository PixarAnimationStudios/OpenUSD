//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_BASED_SUB_EXECUTOR_H
#define PXR_EXEC_VDF_DATA_MANAGER_BASED_SUB_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/dataManagerBasedExecutor.h"
#include "pxr/exec/vdf/executionStats.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfDataManagerSubBasedExecutor
///
/// \brief Base class for executors that use a data manager, and support
/// reading caches from a parent executor.
///

template < typename DataManagerType, typename BaseClass >
class VdfDataManagerBasedSubExecutor :
    public VdfDataManagerBasedExecutor < DataManagerType, BaseClass >
{
    // Base type definition
    typedef VdfDataManagerBasedExecutor<DataManagerType, BaseClass> Base;

public:

    /// Default constructor
    ///
    VdfDataManagerBasedSubExecutor() {}

    /// Construct with parent executor
    ///
    explicit VdfDataManagerBasedSubExecutor(
        const VdfExecutorInterface *parentExecutor) :
        Base(parentExecutor)
    {}

    /// Destructor
    ///
    virtual ~VdfDataManagerBasedSubExecutor() {}

protected:

    /// Returns a value for the cache that flows across \p connection.
    ///
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const override {
        // Lookup the output value in the local data manager, first!
        if (const VdfVector *data = 
                Base::_dataManager.GetInputValue(connection, mask)) {
            return data;
        }

        // If available, also check the parent executor for the output value.
        return _GetParentExecutorValue(connection.GetSourceOutput(), mask);
    }

    /// Returns an output value for reading.
    ///
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask ) const override {
        // Lookup the output value in the local data manager, first!
        if (const VdfVector *data = 
                Base::_dataManager.GetOutputValueForReading(
                    Base::_dataManager.GetDataHandle(output.GetId()), mask)) {
            return data;
        }
        
        // If available, also check the parent executor for the output value.
        return _GetParentExecutorValue(output, mask);
    }

private:

    // Query the parent executor for an output value.
    const VdfVector *_GetParentExecutorValue(
        const VdfOutput &output,
        const VdfMask &mask) const {
        const VdfExecutorInterface *parentExecutor = Base::GetParentExecutor();
        return parentExecutor
            ? parentExecutor->GetOutputValue(output, mask)
            : nullptr;
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
