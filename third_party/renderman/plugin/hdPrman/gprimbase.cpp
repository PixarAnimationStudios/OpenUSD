//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/gprimbase.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_GprimBase::~HdPrman_GprimBase() = default; 

std::vector<riley::GeometryPrototypeId> 
HdPrman_GprimBase::GetPrototypeIds() const
{
    const std::vector<riley::GeometryPrototypeId> ret(_prototypeIds);
    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE
