//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HD_EXT_COMPUATION_CONTEXT_H
#define HD_EXT_COMPUATION_CONTEXT_H

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

#endif // HD_EXT_COMPUATION_CONTEXT_H
