//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/usdaFileFormat.h"

#include "pxr/usd/usd/usdFileFormat.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdUsdaFileFormatTokens, USD_USDA_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdUsdaFileFormat, SdfTextFileFormat);
}

UsdUsdaFileFormat::UsdUsdaFileFormat()
    : SdfTextFileFormat(UsdUsdaFileFormatTokens->Id,
                        UsdUsdaFileFormatTokens->Version,
                        UsdUsdFileFormatTokens->Target)
{
    // Do Nothing.
}

UsdUsdaFileFormat::~UsdUsdaFileFormat()
{
    // Do Nothing.
}

PXR_NAMESPACE_CLOSE_SCOPE

