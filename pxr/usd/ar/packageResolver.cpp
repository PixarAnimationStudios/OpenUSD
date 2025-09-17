//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/packageResolver.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ArPackageResolver>();
}

ArPackageResolver::ArPackageResolver()
{
}

ArPackageResolver::~ArPackageResolver()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
