//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRI_TOKENS_H
#define USDRI_TOKENS_H

/// \file usdRi/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdRi/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdRiTokensType
///
/// \link UsdRiTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdRiTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdRiTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdRiTokens->renderContext);
/// \endcode
struct UsdRiTokensType {
    USDRI_API UsdRiTokensType();
    /// \brief "ri"
    /// 
    /// UsdShadeMaterial / Hydra render context token for UsdRi
    const TfToken renderContext;
    /// \brief "StatementsAPI"
    /// 
    /// Schema identifier and family for UsdRiStatementsAPI
    const TfToken StatementsAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdRiTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdRiTokensType
extern USDRI_API TfStaticData<UsdRiTokensType> UsdRiTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
