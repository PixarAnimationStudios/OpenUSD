//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_ST_EXT_COMP_COMPUTED_INPUT_SOURCE_H
#define PXR_IMAGING_HD_ST_EXT_COMP_COMPUTED_INPUT_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/extCompInputSource.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdStExtCompCpuComputationSharedPtr =
    std::shared_ptr<class HdStExtCompCpuComputation>;

///
/// An Hd Buffer Source Computation that is used to bind an ExtComputation
/// input to a specific output of another ExtComputation.
///
class HdSt_ExtCompComputedInputSource final : public HdSt_ExtCompInputSource
{
public:
    /// Constructs the computation, binding inputName to sourceOutputName
    /// on buffer source representation of the source computation.
    HDST_API
    HdSt_ExtCompComputedInputSource(
        const TfToken &inputName,
        const HdStExtCompCpuComputationSharedPtr &source,
        const TfToken &sourceOutputName);

    HDST_API
    ~HdSt_ExtCompComputedInputSource() override;

    /// Returns true once the source computation has been resolved.
    HDST_API
    bool Resolve() override;

    /// Obtains the value of the output from the source computation.
    HDST_API
    const VtValue &GetValue() const override;

protected:
    /// Returns true if the binding is successful.
    bool _CheckValid() const override;

private:
    HdStExtCompCpuComputationSharedPtr _source;
    size_t                             _sourceOutputIdx;

    HdSt_ExtCompComputedInputSource() = delete;
    HdSt_ExtCompComputedInputSource(
        const HdSt_ExtCompComputedInputSource &) = delete;
    HdSt_ExtCompComputedInputSource &operator = (
        const HdSt_ExtCompComputedInputSource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_COMPUTED_INPUT_SOURCE_H
