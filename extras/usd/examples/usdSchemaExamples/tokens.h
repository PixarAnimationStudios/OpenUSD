//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDSCHEMAEXAMPLES_TOKENS_H
#define USDSCHEMAEXAMPLES_TOKENS_H

/// \file usdSchemaExamples/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSchemaExamplesTokensType
///
/// \link UsdSchemaExamplesTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdSchemaExamplesTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdSchemaExamplesTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdSchemaExamplesTokens->complexString);
/// \endcode
struct UsdSchemaExamplesTokensType {
    USDSCHEMAEXAMPLES_API UsdSchemaExamplesTokensType();
    /// \brief "complexString"
    /// 
    /// UsdSchemaExamplesComplex
    const TfToken complexString;
    /// \brief "intAttr"
    /// 
    /// UsdSchemaExamplesSimple
    const TfToken intAttr;
    /// \brief "params:mass"
    /// 
    /// UsdSchemaExamplesParamsAPI
    const TfToken paramsMass;
    /// \brief "params:velocity"
    /// 
    /// UsdSchemaExamplesParamsAPI
    const TfToken paramsVelocity;
    /// \brief "params:volume"
    /// 
    /// UsdSchemaExamplesParamsAPI
    const TfToken paramsVolume;
    /// \brief "target"
    /// 
    /// UsdSchemaExamplesSimple
    const TfToken target;
    /// \brief "ComplexPrim"
    /// 
    /// Schema identifer and family for UsdSchemaExamplesComplex
    const TfToken ComplexPrim;
    /// \brief "ParamsAPI"
    /// 
    /// Schema identifer and family for UsdSchemaExamplesParamsAPI
    const TfToken ParamsAPI;
    /// \brief "SimplePrim"
    /// 
    /// Schema identifer and family for UsdSchemaExamplesSimple
    const TfToken SimplePrim;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdSchemaExamplesTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdSchemaExamplesTokensType
extern USDSCHEMAEXAMPLES_API TfStaticData<UsdSchemaExamplesTokensType> UsdSchemaExamplesTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
