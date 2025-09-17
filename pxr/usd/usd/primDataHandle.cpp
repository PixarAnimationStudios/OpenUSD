//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/primDataHandle.h"
#include "pxr/usd/usd/primData.h"

PXR_NAMESPACE_OPEN_SCOPE


// defined in primData.cpp.
std::string Usd_DescribePrimData(const Usd_PrimData *p);

std::string
Usd_PrimDataHandle::GetDescription(SdfPath const &proxyPrimPath) const
{
    return Usd_DescribePrimData(_p.get(), proxyPrimPath);
}


PXR_NAMESPACE_CLOSE_SCOPE

