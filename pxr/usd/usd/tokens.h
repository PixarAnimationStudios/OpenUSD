//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USD_TOKENS_H
#define USD_TOKENS_H

/// \file usd/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdTokensType
///
/// \link UsdTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdTokens->apiSchemas);
/// \endcode
struct UsdTokensType {
    USD_API UsdTokensType();
    /// \brief "apiSchemas"
    /// 
    ///  A listop metadata containing the API schemas which have been applied to this prim, using the Apply() method on the particular schema class.  
    const TfToken apiSchemas;
    /// \brief "clips"
    /// 
    ///  Dictionary that contains the definition of the clip sets on this prim. See \ref UsdClipsAPI::GetClips. 
    const TfToken clips;
    /// \brief "clipSets"
    /// 
    ///  ListOp that may be used to affect how opinions from clip sets are applied during value resolution.  See \ref UsdClipsAPI::GetClipSets. 
    const TfToken clipSets;
    /// \brief "collection"
    /// 
    /// Property namespace prefix for the UsdCollectionAPI schema.
    const TfToken collection;
    /// \brief "collection:__INSTANCE_NAME__"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_;
    /// \brief "collection:__INSTANCE_NAME__:excludes"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_Excludes;
    /// \brief "collection:__INSTANCE_NAME__:expansionRule"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_ExpansionRule;
    /// \brief "collection:__INSTANCE_NAME__:includeRoot"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_IncludeRoot;
    /// \brief "collection:__INSTANCE_NAME__:includes"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_Includes;
    /// \brief "collection:__INSTANCE_NAME__:membershipExpression"
    /// 
    /// UsdCollectionAPI
    const TfToken collection_MultipleApplyTemplate_MembershipExpression;
    /// \brief "colorSpaceDefinition"
    /// 
    /// Property namespace prefix for the UsdColorSpaceDefinitionAPI schema.
    const TfToken colorSpaceDefinition;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:blueChroma"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_BlueChroma;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:gamma"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_Gamma;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:greenChroma"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_GreenChroma;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:linearBias"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_LinearBias;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:name"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_Name;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:redChroma"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_RedChroma;
    /// \brief "colorSpaceDefinition:__INSTANCE_NAME__:whitePoint"
    /// 
    /// UsdColorSpaceDefinitionAPI
    const TfToken colorSpaceDefinition_MultipleApplyTemplate_WhitePoint;
    /// \brief "colorSpace:name"
    /// 
    /// UsdColorSpaceAPI
    const TfToken colorSpaceName;
    /// \brief "custom"
    /// 
    /// Fallback value for UsdColorSpaceDefinitionAPI::GetNameAttr()
    const TfToken custom;
    /// \brief "exclude"
    /// 
    ///  This is the token used to exclude a path from a collection.  Although it is not a possible value for the "expansionRule" attribute, it is used as the expansionRule for excluded paths  in UsdCollectionAPI::MembershipQuery::IsPathIncluded. 
    const TfToken exclude;
    /// \brief "expandPrims"
    /// 
    /// Fallback value for UsdCollectionAPI::GetExpansionRuleAttr()
    const TfToken expandPrims;
    /// \brief "expandPrimsAndProperties"
    /// 
    /// Possible value for UsdCollectionAPI::GetExpansionRuleAttr()
    const TfToken expandPrimsAndProperties;
    /// \brief "explicitOnly"
    /// 
    /// Possible value for UsdCollectionAPI::GetExpansionRuleAttr()
    const TfToken explicitOnly;
    /// \brief "fallbackPrimTypes"
    /// 
    ///  A dictionary metadata that maps the name of a concrete schema prim type to an ordered list of schema prim types to use instead if the schema prim type doesn't exist in version of USD being used. 
    const TfToken fallbackPrimTypes;
    /// \brief "APISchemaBase"
    /// 
    /// Schema identifer and family for UsdAPISchemaBase
    const TfToken APISchemaBase;
    /// \brief "ClipsAPI"
    /// 
    /// Schema identifer and family for UsdClipsAPI
    const TfToken ClipsAPI;
    /// \brief "CollectionAPI"
    /// 
    /// Schema identifer and family for UsdCollectionAPI
    const TfToken CollectionAPI;
    /// \brief "ColorSpaceAPI"
    /// 
    /// Schema identifer and family for UsdColorSpaceAPI
    const TfToken ColorSpaceAPI;
    /// \brief "ColorSpaceDefinitionAPI"
    /// 
    /// Schema identifer and family for UsdColorSpaceDefinitionAPI
    const TfToken ColorSpaceDefinitionAPI;
    /// \brief "ModelAPI"
    /// 
    /// Schema identifer and family for UsdModelAPI
    const TfToken ModelAPI;
    /// \brief "Typed"
    /// 
    /// Schema identifer and family for UsdTyped
    const TfToken Typed;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdTokensType
extern USD_API TfStaticData<UsdTokensType> UsdTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
