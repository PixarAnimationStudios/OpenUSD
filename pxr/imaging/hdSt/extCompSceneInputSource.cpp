//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/extCompSceneInputSource.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_ExtCompSceneInputSource::HdSt_ExtCompSceneInputSource(
    const TfToken &inputName,
    const VtValue &value)
    : HdSt_ExtCompInputSource(inputName)
    , _value(value)
{
}

HdSt_ExtCompSceneInputSource::~HdSt_ExtCompSceneInputSource() = default;

bool
HdSt_ExtCompSceneInputSource::Resolve()
{
    if (!_TryLock()) return false;

    _SetResolved();
    return true;
}

const VtValue &
HdSt_ExtCompSceneInputSource::GetValue() const
{
    return _value;
}

bool
HdSt_ExtCompSceneInputSource::_CheckValid() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

