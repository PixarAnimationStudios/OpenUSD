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
    /// \brief "apiSchemaType"
    /// 
    ///  Custom-data key authored on an API schema class-prim in the  schema definition, to define the type of API schema. Possible  values include "nonApplied", "singleApply" and "multipleApply".  
    const TfToken apiSchemaType;
    /// \brief "clipActive"
    /// 
    ///  List of pairs (time, clip index) indicating the time on the stage at which the clip specified by the clip index is active. For instance, a value of [(0.0, 0), (20.0, 1)] indicates that clip 0 is active at time 0 and clip 1 is active at time 20. 
    const TfToken clipActive;
    /// \brief "clipAssetPaths"
    /// 
    ///  List of asset paths to the clips for this prim. This list is unordered, but elements in this list are referred to by index in other clip-related fields. 
    const TfToken clipAssetPaths;
    /// \brief "clipManifestAssetPath"
    /// 
    ///  Asset path for the clip manifest. The clip manifest indicates which attributes have time samples authored in the clips specified on this prim. During value resolution, we will only look for time samples  in clips if the attribute exists and is declared as varying in the manifest. Note that the clip manifest is only consulted to check check if an attribute exists and what its variability is. Other values and metadata authored in the manifest will be ignored.  For instance, if this prims' path is '/Prim_1', the clip prim path is '/Prim', and we want values for the attribute '/Prim_1.size', we will only look within this prims' clips if the attribute '/Prim.size' exists and is varying in the manifest. 
    const TfToken clipManifestAssetPath;
    /// \brief "clipPrimPath"
    /// 
    ///  Path to the prim in the clips from which time samples will be read. This prim's path will be substituted with this value to determine the final path in the clip from which to read data. For instance, if this prims' path is '/Prim_1', the clip prim path is '/Prim',  and we want to get values for the attribute '/Prim_1.size'. The clip prim path will be substituted in, yielding '/Prim.size', and each clip will be examined for values at that path. 
    const TfToken clipPrimPath;
    /// \brief "clips"
    /// 
    ///  Dictionary that contains the definition of the clip sets on this prim. See \ref UsdClipsAPI::GetClips. 
    const TfToken clips;
    /// \brief "clipSets"
    /// 
    ///  ListOp that may be used to affect how opinions from clip sets are applied during value resolution.  See \ref UsdClipsAPI::GetClipSets. 
    const TfToken clipSets;
    /// \brief "clipTemplateAssetPath"
    /// 
    ///  A template string representing a set of assets. This string can be of two forms: path/basename.###.usd and path/basename.##.##.usd. In either case, the number of hash marks in each section is variable. These control the amount of padding USD will supply when looking up  the assets. For instance, a value of 'foo.###.usd',  with clipTemplateStartTime=11, clipTemplateEndTime=15, and clipTemplateStride=1: USD will look for: foo.011.usd, foo.012.usd, foo.013.usd, foo.014.usd and foo.015.usd. 
    const TfToken clipTemplateAssetPath;
    /// \brief "clipTemplateEndTime"
    /// 
    ///  A double which indicates the end of the range USD will use to to search for asset paths. This value is inclusive in that range. For example usage see clipTemplateAssetPath. 
    const TfToken clipTemplateEndTime;
    /// \brief "clipTemplateStartTime"
    /// 
    ///  A double which indicates the start of the range USD will use  to search for asset paths. This value is inclusive in that range. For example usage see clipTemplateAssetPath. 
    const TfToken clipTemplateStartTime;
    /// \brief "clipTemplateStride"
    /// 
    ///  A double representing the increment value USD will use when searching for asset paths. For example usage see clipTemplateAssetPath. 
    const TfToken clipTemplateStride;
    /// \brief "clipTimes"
    /// 
    ///  List of pairs (stage time, clip time) indicating the time in the active clip that should be consulted for values at the corresponding stage time.   During value resolution, this list will be sorted by stage time;  times will then be linearly interpolated between consecutive entries. For instance, for clip times [(0.0, 0.0), (10.0, 20.0)],  at stage time 0, values from the active clip at time 0 will be used, at stage time 5, values from the active clip at time 10, and at stage  time 10, clip values at time 20. 
    const TfToken clipTimes;
    /// \brief "collection"
    /// 
    /// This is the namespace prefix used by  UsdCollectionAPI for authoring collections.
    const TfToken collection;
    /// \brief "exclude"
    /// 
    ///  This is the token used to exclude a path from a collection.  Although it is not a possible value for the "expansonRule" attribute, it is used as the expansionRule for excluded paths  in UsdCollectionAPI::MembershipQuery::IsPathIncluded. 
    const TfToken exclude;
    /// \brief "excludes"
    /// 
    /// UsdCollectionAPI
    const TfToken excludes;
    /// \brief "expandPrims"
    /// 
    ///  Possible value for the "expansionRule" attribute of a  collection. 
    const TfToken expandPrims;
    /// \brief "expandPrimsAndProperties"
    /// 
    ///  Possible value for the "expansionRule" attribute of a  collection. 
    const TfToken expandPrimsAndProperties;
    /// \brief "expansionRule"
    /// 
    /// Base name of the attribute used to encode  how the paths included in a collection must be expanded, UsdCollectionAPI
    const TfToken expansionRule;
    /// \brief "explicitOnly"
    /// 
    ///  Possible value for the "expansionRule" attribute of a  collection. 
    const TfToken explicitOnly;
    /// \brief "includeRoot"
    /// 
    ///  Name of a boolean collection-scoped attribute that, if true, indicates that the root path </> should be consideded as an included target path, UsdCollectionAPI
    const TfToken includeRoot;
    /// \brief "includes"
    /// 
    /// UsdCollectionAPI
    const TfToken includes;
    /// \brief "isPrivateApply"
    /// 
    ///  Custom-data key authored on an applied API schema class-prim  in the schema definition to indicate whether the auto-generated Apply method should be public or private. 
    const TfToken isPrivateApply;
    /// \brief "multipleApply"
    /// 
    ///  Possible value for customData["apiSchemaType"] on an API schema class prim.  
    const TfToken multipleApply;
    /// \brief "nonApplied"
    /// 
    ///  Possible value for customData["apiSchemaType"] on an API schema class prim.  
    const TfToken nonApplied;
    /// \brief "propertyNamespacePrefix"
    /// 
    ///  Custom-data key authored on a multiple-apply API schema class-prim in the schema definition, to define prefix for properties created by the API schema. 
    const TfToken propertyNamespacePrefix;
    /// \brief "singleApply"
    /// 
    ///  Possible value for customData["apiSchemaType"] on an API schema class prim.  
    const TfToken singleApply;
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
