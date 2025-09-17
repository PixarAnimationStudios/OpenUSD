//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROC_TOKENS_H
#define USDPROC_TOKENS_H

/// \file usdProc/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdProc/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdProcTokensType
///
/// \link UsdProcTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdProcTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdProcTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdProcTokens->proceduralSystem);
/// \endcode
struct UsdProcTokensType {
    USDPROC_API UsdProcTokensType();
    /// \brief "proceduralSystem"
    /// 
    /// UsdProcGenerativeProcedural
    const TfToken proceduralSystem;
    /// \brief "GenerativeProcedural"
    /// 
    /// Schema identifer and family for UsdProcGenerativeProcedural
    const TfToken GenerativeProcedural;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdProcTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdProcTokensType
extern USDPROC_API TfStaticData<UsdProcTokensType> UsdProcTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
