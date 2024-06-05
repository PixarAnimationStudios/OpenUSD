//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_EXT_COMPUTATION_CONTEXT_H
#define PXR_IMAGING_HD_EXT_COMPUTATION_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Interface class that defines the execution environment for the client
/// to run a computation.
///
class HdExtComputationContext {
public:
    HdExtComputationContext() = default;
    virtual ~HdExtComputationContext();

    ///
    /// Obtains the value of an named input to the computation.
    ///
    /// The code will issue a coding error and return a empty array
    /// if the input is missing or of a different type.
    ///
    virtual const VtValue &GetInputValue(const TfToken &name) const = 0;

    ///
    /// Obtains the value of an named input to the computation.
    ///
    /// If the input isn't present or of a different type
    /// nullptr will be returned.
    ///
    virtual const VtValue *GetOptionalInputValuePtr(
                                                 const TfToken &name) const = 0;

    ///
    /// Set the value of the specified output.
    ///
    virtual void SetOutputValue(const TfToken &name, const VtValue &output) = 0;

    ///
    /// Called to indicate an error occurred while executing a computation
    /// so that its output are invalid.
    ///
    virtual void RaiseComputationError() = 0;

private:
    HdExtComputationContext(const HdExtComputationContext &)           = delete;
    HdExtComputationContext &operator = (const HdExtComputationContext &)
                                                                       = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_EXT_COMPUTATION_CONTEXT_H
