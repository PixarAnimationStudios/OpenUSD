//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_EXT_COMPUTATION_CPU_CALLBACK_H
#define PXR_IMAGING_HD_EXT_COMPUTATION_CPU_CALLBACK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdExtComputationContext;
using HdExtComputationCpuCallbackSharedPtr =
    std::shared_ptr<class HdExtComputationCpuCallback>;

/// \class HdExtComputationCallback
///
/// A callback for an ext computation filling the outputs given the
/// input values and values of the input computations.
///
class HdExtComputationCpuCallback
{
public:
    HD_API virtual ~HdExtComputationCpuCallback();

    /// Run the computation.
    virtual void Compute(HdExtComputationContext * ctx) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
