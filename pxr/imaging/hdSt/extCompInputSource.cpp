//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/extCompInputSource.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_ExtCompInputSource::HdSt_ExtCompInputSource(const TfToken &inputName)
    : HdNullBufferSource()
    , _inputName(inputName)
{

}

HdSt_ExtCompInputSource::~HdSt_ExtCompInputSource() = default;

TfToken const &
HdSt_ExtCompInputSource::GetName() const
{
    return _inputName;
}


PXR_NAMESPACE_CLOSE_SCOPE

