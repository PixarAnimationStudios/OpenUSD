//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/typeNotice.h"

PXR_NAMESPACE_OPEN_SCOPE

// Note:  We do not register a TfType for TfTypeWasDeclaredNotice here.
// Instead, we register it in Type.cpp.  See Tf_TypeRegistry's constructor.

TfTypeWasDeclaredNotice::TfTypeWasDeclaredNotice( TfType t ) :
    _type(t)
{
}

TfTypeWasDeclaredNotice::~TfTypeWasDeclaredNotice()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
