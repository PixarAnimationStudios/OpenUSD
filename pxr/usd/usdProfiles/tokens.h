//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROFILES_TOKENS_H
#define USDPROFILES_TOKENS_H

/// \file UsdProfiles/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdProfilesTokensType
///
/// \link UsdProfilesTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdProfilesTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdProfilesTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdProfilesTokens->ClaimsAPI);
/// \endcode
struct UsdProfilesTokensType {
    USDPROFILES_API UsdProfilesTokensType();
    /// \brief "ClaimsAPI"
    /// 
    /// Schema identifer and family for UsdProfilesClaimsAPI
    const TfToken ClaimsAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdProfilesTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdProfilesTokensType
extern USDPROFILES_API TfStaticData<UsdProfilesTokensType> UsdProfilesTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
