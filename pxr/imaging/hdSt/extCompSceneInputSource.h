//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_COMP_SCENE_INPUT_SOURCE_H
#define PXR_IMAGING_HD_ST_EXT_COMP_SCENE_INPUT_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/extCompInputSource.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE


///
/// An Hd Buffer Source Computation that is used to bind an ExtComputation input
/// to a value provided by the scene delegate.
///
class HdSt_ExtCompSceneInputSource final : public HdSt_ExtCompInputSource
{
public:
    /// Constructs the computation, binding inputName to the provided value.
    HDST_API
    HdSt_ExtCompSceneInputSource(
        const TfToken &inputName, const VtValue &value);

    HDST_API
    ~HdSt_ExtCompSceneInputSource() override;

    /// Set the state of the computation to resolved and returns true.
    HDST_API
    bool Resolve() override;

    /// Returns the value associated with this input.
    HDST_API
    const VtValue &GetValue() const override;

protected:
    /// Returns if this computation binding is valid.
    bool _CheckValid() const override;

private:
    VtValue _value;

    // No copying, assignment or default construction.
    HdSt_ExtCompSceneInputSource() = delete;
    HdSt_ExtCompSceneInputSource(
        const HdSt_ExtCompSceneInputSource &) = delete;
    HdSt_ExtCompSceneInputSource &operator = (
        const HdSt_ExtCompSceneInputSource &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_SCENE_INPUT_SOURCE_H
