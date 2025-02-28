//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceAttributeTypeName.h"

#include "pxr/imaging/hd/dataSourceLocator.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceAttributeTypeName::UsdImagingDataSourceAttributeTypeName(
    const UsdAttribute &usdAttr)
    :  _usdAttr(usdAttr)
{
}


PXR_NAMESPACE_CLOSE_SCOPE
