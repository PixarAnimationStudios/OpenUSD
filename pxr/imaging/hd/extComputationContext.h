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
    /// The code will issue a coding error and return a empty value
    /// if the input is missing.
    ///
    virtual const VtValue &GetInputValue(const TfToken &name) const = 0;

    ///
    /// Obtains the value of an named input to the computation.
    ///
    /// The code will issue a coding error and return a default constructed
    /// value if the input is missing or is of the wrong type.
    ///
    template <typename T>
    T GetTypedInputValue(const TfToken &name) const;
    
    ///
    /// Obtains the value of an named input to the computation.
    ///
    /// If the input isn't present, a nullptr will be returned.
    ///
    virtual const VtValue *GetOptionalInputValuePtr(
                                                 const TfToken &name) const = 0;

    ///
    /// Obtains the value of an named input to the computation.
    ///
    /// If the input isn't present, a nullptr will be returned. If the input
    /// is of the wrong type, a coding error will be issued and a nullptr will
    /// be returned.
    ///
    template <typename T>
    const T *GetOptionalTypedInputValuePtr(const TfToken &name) const;

    ///
    /// Set the value of the specified output.
    ///
    virtual void SetOutputValue(const TfToken &name, const VtValue &output) = 0;

    ///
    /// Set the value of the specified output.
    ///
    template <typename T>
    void SetTypedOutputValue(const TfToken &name, const T &output);

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

template <typename T>
T
HdExtComputationContext::GetTypedInputValue(const TfToken &name) const
{
    const VtValue &v = GetInputValue(name);
    if (!v.IsHolding<T>()) {
        TF_CODING_ERROR(
            "HdExtComputationContext::GetTypedInputValue<T> called with type T"
            "not matching the type of the input value for '%s'.",
            name.GetText());
        return {};
    }

    return v.UncheckedGet<T>();
}

template <typename T>
const T *
HdExtComputationContext::GetOptionalTypedInputValuePtr(const TfToken &name) const
{
    const VtValue * const v = GetOptionalInputValuePtr(name);
    if (!v) {
        return nullptr;
    }
   
    if (!v->IsHolding<T>()) {
        TF_CODING_ERROR(
            "HdExtComputationContext::GetOptionalTypedInputValue<T> called with "
            "type T not matching the type of the input value for '%s'.",
            name.GetText());
        return nullptr;
    }

    return &(v->UncheckedGet<T>());
}

template <typename T>
void
HdExtComputationContext::
SetTypedOutputValue(const TfToken &name, const T &output)
{
    SetOutputValue(name, VtValue(output));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_EXT_COMPUTATION_CONTEXT_H
