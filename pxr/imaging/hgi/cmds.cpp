//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/cmds.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiCmds::HgiCmds()
    : _submitted(false)
{}

HgiCmds::~HgiCmds() = default;

bool
HgiCmds::IsSubmitted() const
{
    return _submitted;
}

bool
HgiCmds::_Submit(Hgi* hgi, HgiSubmitWaitType wait)
{
    return false;
}

void
HgiCmds::_SetSubmitted()
{
    _submitted = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
