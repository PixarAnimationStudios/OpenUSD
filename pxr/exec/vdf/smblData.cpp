//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/smblData.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfSMBLData::~VdfSMBLData()
{
    delete _cache;
}

void 
VdfSMBLData::Clear()
{
    delete _cache;
    _cache = NULL;
    _cacheMask = VdfMask();
}

PXR_NAMESPACE_CLOSE_SCOPE
