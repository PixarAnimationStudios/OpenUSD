//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_GEOM_VALIDATOR_TOKENS_H
#define PXR_USD_USD_GEOM_VALIDATOR_TOKENS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_GEOM_VALIDATOR_NAME_TOKENS                   \
    ((subsetFamilies, "usdGeom:SubsetFamilies"))

#define USD_GEOM_VALIDATOR_KEYWORD_TOKENS                \
    (UsdGeomSubset)                                      \
    (UsdGeomValidators)

///\def
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdGeom:, which is the name of
/// the usdGeom plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorNameTokens, USDGEOM_API,
                         USD_GEOM_VALIDATOR_NAME_TOKENS);

///\def
/// Tokens representing keywords associated with any validator in the usdGeom
/// plugin. Clients can use this to inspect validators contained within a
/// specific keyword, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdGeomValidatorKeywordTokens, USDGEOM_API,
                         USD_GEOM_VALIDATOR_KEYWORD_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
