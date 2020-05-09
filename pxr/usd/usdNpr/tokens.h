//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDNPR_TOKENS_H
#define USDNPR_TOKENS_H

/// \file usdNpr/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdNpr/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdNprTokensType
///
/// \link UsdNprTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdNprTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdNprTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdNprTokens->boundaries);
/// \endcode
struct UsdNprTokensType {
    USDNPR_API UsdNprTokensType();
    /// \brief "boundaries"
    /// 
    /// Boolean - Does the stroke draw boundaries of the surfaces.
    const TfToken boundaries;
    /// \brief "boundaryWidth"
    /// 
    /// UsdNprContour
    const TfToken boundaryWidth;
    /// \brief "contour:surfaces"
    /// 
    /// UsdNprContour
    const TfToken contourSurfaces;
    /// \brief "contour:viewPoint"
    /// 
    /// UsdNprContour
    const TfToken contourViewPoint;
    /// \brief "creases"
    /// 
    /// Boolean - Does the stroke draw creases of the surfaces.
    const TfToken creases;
    /// \brief "creaseWidth"
    /// 
    /// UsdNprContour
    const TfToken creaseWidth;
    /// \brief "drawBoundary"
    /// 
    /// UsdNprContour
    const TfToken drawBoundary;
    /// \brief "drawCrease"
    /// 
    /// UsdNprContour
    const TfToken drawCrease;
    /// \brief "drawSilhouette"
    /// 
    /// UsdNprContour
    const TfToken drawSilhouette;
    /// \brief "silhouettes"
    /// 
    /// Boolean - Does the stroke draw silhouette of the surfaces.
    const TfToken silhouettes;
    /// \brief "silhouetteWidth"
    /// 
    /// UsdNprContour
    const TfToken silhouetteWidth;
    /// \brief "surfaces"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI
    /// to represent surface-linking of a UsdContour prim.
    const TfToken surfaces;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdNprTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdNprTokensType
extern USDNPR_API TfStaticData<UsdNprTokensType> UsdNprTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
