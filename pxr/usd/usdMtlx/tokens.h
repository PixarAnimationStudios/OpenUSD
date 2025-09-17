//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMTLX_TOKENS_H
#define USDMTLX_TOKENS_H

/// \file usdMtlx/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdMtlxTokensType
///
/// \link UsdMtlxTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdMtlxTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdMtlxTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdMtlxTokens->configMtlxVersion);
/// \endcode
struct UsdMtlxTokensType {
    USDMTLX_API UsdMtlxTokensType();
    /// \brief "config:mtlx:version"
    /// 
    /// UsdMtlxMaterialXConfigAPI
    const TfToken configMtlxVersion;
    /// \brief "out"
    /// 
    /// Special token for the usdMtlx library.
    const TfToken DefaultOutputName;
    /// \brief "MaterialXConfigAPI"
    /// 
    /// Schema identifer and family for UsdMtlxMaterialXConfigAPI
    const TfToken MaterialXConfigAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdMtlxTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdMtlxTokensType
extern USDMTLX_API TfStaticData<UsdMtlxTokensType> UsdMtlxTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
