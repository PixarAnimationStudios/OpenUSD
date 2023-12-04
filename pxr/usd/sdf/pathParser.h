//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_SDF_PATH_PARSER_H
#define PXR_USD_SDF_PATH_PARSER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/pxrPEGTL/pegtl.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
Sdf_ParsePath(std::string const &pathStr, SdfPath *path, std::string *errMsg);

namespace Sdf_PathParser {

namespace PEGTL_NS = tao::TAO_PEGTL_NAMESPACE;

////////////////////////////////////////////////////////////////////////
// SdfPath grammar:

struct Slash : PEGTL_NS::one<'/'> {};
struct Dot : PEGTL_NS::one<'.'> {};
struct DotDot : PEGTL_NS::two<'.'> {};

struct AbsoluteRoot : Slash {};
struct ReflexiveRelative : Dot {};

struct DotDots : PEGTL_NS::list<DotDot, Slash> {};

struct PrimName : PEGTL_NS::identifier {};

// XXX This replicates old behavior where '-' chars are allowed in variant set
// names in SdfPaths, but variant sets in layers cannot have '-' in their names.
// For now we preserve the behavior. Internal bug USD-8321 tracks removing
// support for '-' characters in variant set names in SdfPath.
struct VariantSetName :
    PEGTL_NS::seq<PEGTL_NS::identifier_first,
    PEGTL_NS::star<PEGTL_NS::sor<
    PEGTL_NS::identifier_other, PEGTL_NS::one<'-'>>>> {};

struct VariantName :
    PEGTL_NS::seq<PEGTL_NS::opt<
    PEGTL_NS::one<'.'>>, PEGTL_NS::star<
    PEGTL_NS::sor<PEGTL_NS::identifier_other,
    PEGTL_NS::one<'|', '-'>>>> {};

struct VarSelOpen : PEGTL_NS::pad<PEGTL_NS::one<'{'>, PEGTL_NS::blank> {};
struct VarSelClose : PEGTL_NS::pad<PEGTL_NS::one<'}'>, PEGTL_NS::blank> {};

struct VariantSelection : PEGTL_NS::if_must<
    VarSelOpen,
    VariantSetName, PEGTL_NS::pad<PEGTL_NS::one<'='>, 
    PEGTL_NS::blank>, PEGTL_NS::opt<VariantName>,
    VarSelClose>
{};

struct VariantSelections : PEGTL_NS::plus<VariantSelection> {};

template <class Rule, class Sep>
struct LookaheadList : PEGTL_NS::seq<Rule, PEGTL_NS::star<
    PEGTL_NS::at<Sep, Rule>, Sep, Rule>> {};

struct PrimElts : PEGTL_NS::seq<
    LookaheadList<PrimName, PEGTL_NS::sor<Slash, VariantSelections>>,
    PEGTL_NS::opt<VariantSelections>> {};

struct PropertyName : PEGTL_NS::list<PEGTL_NS::identifier, PEGTL_NS::one<':'>> {};

struct MapperPath;
struct TargetPath;

struct TargetPathOpen : PEGTL_NS::one<'['> {};
struct TargetPathClose : PEGTL_NS::one<']'> {};

template <class TargPath>
struct BracketPath : PEGTL_NS::if_must<TargetPathOpen, TargPath, TargetPathClose> {};

struct RelationalAttributeName : PropertyName {};

struct MapperKW : TAO_PEGTL_KEYWORD("mapper") {};

struct MapperArg : PEGTL_NS::identifier {};

struct MapperPathSeq : PEGTL_NS::if_must<
    PEGTL_NS::seq<Dot, MapperKW>, BracketPath<MapperPath>,
    PEGTL_NS::opt<Dot, MapperArg>> {};

struct Expression : TAO_PEGTL_KEYWORD("expression") {};

struct RelAttrSeq : PEGTL_NS::if_must<
    PEGTL_NS::one<'.'>, RelationalAttributeName,
    PEGTL_NS::opt<PEGTL_NS::sor<BracketPath<TargetPath>,
            MapperPathSeq,
            PEGTL_NS::if_must<Dot, Expression>>>> {};

struct TargetPathSeq : PEGTL_NS::seq<BracketPath<TargetPath>, 
    PEGTL_NS::opt<RelAttrSeq>> {};

struct PropElts :
    PEGTL_NS::seq<PEGTL_NS::one<'.'>, PropertyName,
        PEGTL_NS::opt<PEGTL_NS::sor<TargetPathSeq, MapperPathSeq,
        PEGTL_NS::if_must<Dot, Expression>>>
    >
{};

struct PathElts : PEGTL_NS::if_then_else<PrimElts, PEGTL_NS::opt<PropElts>, PropElts> {};

struct PrimFirstPathElts : PEGTL_NS::seq<PrimElts, PEGTL_NS::opt<PropElts>> {};

struct Path : PEGTL_NS::sor<
    PEGTL_NS::seq<AbsoluteRoot, PEGTL_NS::opt<PrimFirstPathElts>>,
    PEGTL_NS::seq<DotDots, PEGTL_NS::opt<PEGTL_NS::seq<Slash, PathElts>>>,
    PathElts,
    ReflexiveRelative
    > {};

struct TargetPath : Path {};
struct MapperPath : Path {};

////////////////////////////////////////////////////////////////////////
// Actions.

struct PPContext {
    std::vector<SdfPath> paths { 1 };
    enum { IsTargetPath, IsMapperPath } targetType;
    std::string varSetName;
    std::string varName;
};

template <class Input>
TfToken GetToken(Input const &in) {
    constexpr int BufSz = 32;
    char buf[BufSz];
    size_t strSize = std::distance(in.begin(), in.end());
    TfToken tok;
    if (strSize < BufSz) {
        // copy & null-terminate.
        std::copy(in.begin(), in.end(), buf);
        buf[strSize] = '\0';
        tok = TfToken(buf);
    }
    else {
        // fall back to string path.
        tok = TfToken(in.string());
    }
    return tok;
}

template <class Rule>
struct Action : PEGTL_NS::nothing<Rule> {};

} // Sdf_PathParser

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_PARSER_H