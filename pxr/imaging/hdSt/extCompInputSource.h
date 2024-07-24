//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_ST_EXT_COMP_INPUT_SOURCE_H
#define PXR_IMAGING_HD_ST_EXT_COMP_INPUT_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferSource.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class VtValue;

using HdSt_ExtCompInputSourceSharedPtr =
        std::shared_ptr<class HdSt_ExtCompInputSource>;
using HdSt_ExtCompInputSourceSharedPtrVector =
        std::vector<HdSt_ExtCompInputSourceSharedPtr>;

///
/// Abstract base class for a Buffer Source that represents a binding to an
/// input to an ExtComputation.
///
class HdSt_ExtCompInputSource : public HdNullBufferSource {
public:
    /// Constructs the input binding with the name inputName
    HDST_API
    HdSt_ExtCompInputSource(const TfToken &inputName);

    HDST_API
    ~HdSt_ExtCompInputSource() override;

    /// Returns the name of the input.
    HDST_API
    const TfToken &GetName() const override final;

    /// Returns the value associated with the input.
    HDST_API
    virtual const VtValue &GetValue() const = 0;

private:
    TfToken _inputName;

    HdSt_ExtCompInputSource() = delete;
    HdSt_ExtCompInputSource(
        const HdSt_ExtCompInputSource &) = delete;
    HdSt_ExtCompInputSource &operator = (
        const HdSt_ExtCompInputSource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_INPUT_SOURCE_H
