//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/timestamp.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

void
ArTimestamp::_IssueInvalidGetTimeError() const
{
    TF_CODING_ERROR("Cannot call GetTime on an invalid ArTimestamp");
}

PXR_NAMESPACE_CLOSE_SCOPE
