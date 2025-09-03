//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_FACADE
#define PXR_EXEC_VDF_DATA_MANAGER_FACADE

#include "pxr/pxr.h"

#include "pxr/exec/vdf/executorInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfMask;
class VdfOutput;
class VdfVector;

/// Facade that gives clients access to values cached on an executor, without
/// exposing the entire VdfExecutorInterface API.
///
class VdfDataManagerFacade
{
public:
    explicit VdfDataManagerFacade(const VdfExecutorInterface &executor)
        : _executor(&executor)
    {}

    /// Returns the value cached at \p output for the given \p mask.
    ///
    /// Returns `nullptr` on cache misses.
    /// 
    const VdfVector *GetOutputValue(
        const VdfOutput &output,
        const VdfMask &mask) const {
        return _executor->GetOutputValue(output, mask);
    }

private:
    const VdfExecutorInterface *_executor;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
