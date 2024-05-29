//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/inMemoryAsset.h"

PXR_NAMESPACE_OPEN_SCOPE

ArAsset::ArAsset()
{
}

ArAsset::~ArAsset()
{
}

std::shared_ptr<ArAsset>
ArAsset::GetDetachedAsset() const
{
    return ArInMemoryAsset::FromAsset(*this);
}

PXR_NAMESPACE_CLOSE_SCOPE
