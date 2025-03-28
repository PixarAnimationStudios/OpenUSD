//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSEMANTICS_TOKENS_H
#define USDSEMANTICS_TOKENS_H

/// \file usdSemantics/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdSemantics/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSemanticsTokensType
///
/// \link UsdSemanticsTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdSemanticsTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdSemanticsTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdSemanticsTokens->semanticsLabels);
/// \endcode
struct UsdSemanticsTokensType {
    USDSEMANTICS_API UsdSemanticsTokensType();
    /// \brief "semantics:labels"
    /// 
    /// Property namespace prefix for the UsdSemanticsLabelsAPI schema.
    const TfToken semanticsLabels;
    /// \brief "semantics:labels:__INSTANCE_NAME__"
    /// 
    /// UsdSemanticsLabelsAPI
    const TfToken semanticsLabels_MultipleApplyTemplate_;
    /// \brief "SemanticsLabelsAPI"
    /// 
    /// Schema identifer and family for UsdSemanticsLabelsAPI
    const TfToken SemanticsLabelsAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdSemanticsTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdSemanticsTokensType
extern USDSEMANTICS_API TfStaticData<UsdSemanticsTokensType> UsdSemanticsTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
