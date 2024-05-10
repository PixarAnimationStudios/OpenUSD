//
// Copyright 2024 Pixar
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

#ifndef PXR_USD_SDF_PATH_PATTERN_PARSER_H
#define PXR_USD_SDF_PATH_PATTERN_PARSER_H

#include "pxr/pxr.h"

#include "pxr/usd/sdf/predicateExpressionParser.h"
#include "pxr/base/pegtl/pegtl.hpp"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

namespace SdfPathPatternParser {

using namespace PXR_PEGTL_NAMESPACE;

template <class Rule, class Sep>
using LookaheadList = seq<Rule, star<at<Sep, Rule>, Sep, Rule>>;

template <class Rule> using OptSpaced = pad<Rule, blank>;

////////////////////////////////////////////////////////////////////////
// Path patterns with predicates.
struct PathPatStretch : two<'/'> {};
struct PathPatSep : sor<PathPatStretch, one<'/'>> {};

struct BracedPredExpr
    : if_must<one<'{'>,
              OptSpaced<SdfPredicateExpressionParser::PredExpr>,
              one<'}'>> {};

struct PrimPathWildCard :
    seq<
    plus<sor<identifier_other, one<'?','*'>>>,
    opt<one<'['>,plus<sor<identifier_other, one<'[',']','!','-','?','*'>>>>
    > {};

struct PropPathWildCard :
    seq<
    plus<sor<identifier_other, one<':','?','*'>>>,
    opt<one<'['>,plus<sor<identifier_other, one<':','[',']','!','-','?','*'>>>>
    > {};

struct PrimPathPatternElemText : PrimPathWildCard {};
struct PropPathPatternElemText : PropPathWildCard {};

struct PrimPathPatternElem
    : if_then_else<PrimPathPatternElemText, opt<BracedPredExpr>,
                   BracedPredExpr> {};

struct PropPathPatternElem
    : if_then_else<PropPathPatternElemText, opt<BracedPredExpr>,
                   BracedPredExpr> {};

struct PathPatternElems
    : seq<LookaheadList<PrimPathPatternElem, PathPatSep>,
          if_must_else<one<'.'>, PropPathPatternElem, opt<PathPatStretch>>> {};

struct AbsPathPattern : seq<PathPatSep, opt<PathPatternElems>> {};

struct DotDot : two<'.'> {};
struct DotDots : list<DotDot, one<'/'>> {};

struct ReflexiveRelative : one<'.'> {};

struct AbsoluteStart : at<one<'/'>> {};

struct PathPattern :
    sor<
    if_must<AbsoluteStart, AbsPathPattern>,
    seq<DotDots, if_then_else<PathPatSep, opt<PathPatternElems>, success>>,
    PathPatternElems,
    seq<ReflexiveRelative, opt<PathPatStretch, opt<PathPatternElems>>>
    >
{};

} // SdfPathPatternParser


namespace SdfPathPatternActions {

using namespace PXR_PEGTL_NAMESPACE;

using namespace SdfPathPatternParser;

// Actions /////////////////////////////////////////////////////////////

struct PatternBuilder
{
    // The final resulting pattern winds up here.
    SdfPathPattern pattern;

    // These are used during parsing.
    std::string curElemText;
    SdfPredicateExpression curPredExpr;
};


template <class Rule>
struct PathPatternAction : nothing<Rule> {};

template <>
struct PathPatternAction<AbsoluteStart>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.pattern.SetPrefix(SdfPath::AbsoluteRootPath());
    }
};

template <>
struct PathPatternAction<PathPatStretch>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        // '//' appends a component representing arbitrary hierarchy.
        TF_VERIFY(builder.pattern.AppendStretchIfPossible());
    }
};

// Change action & state to the PredicateExpressionParser so it can parse &
// build a predicate expression for us.
template <>
struct PathPatternAction<SdfPredicateExpressionParser::PredExpr>
    : change_action_and_states<SdfPredicateExpressionParser::PredAction,
                               SdfPredicateExprBuilder>
{
    template <class Input>
    static void success(Input const &in,
                        SdfPredicateExprBuilder &predExprBuilder,
                        PatternBuilder &builder) {
        builder.curPredExpr = predExprBuilder.Finish();
    }
};

template <>
struct PathPatternAction<PrimPathPatternElemText>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.curElemText = in.string();
    }
};

template <>
struct PathPatternAction<PropPathPatternElemText>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.curElemText = in.string();
    }
};

template <>
struct PathPatternAction<PrimPathPatternElem>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.pattern.AppendChild(builder.curElemText, builder.curPredExpr);
        builder.curElemText.clear();
        builder.curPredExpr = SdfPredicateExpression();
    }
};

template <>
struct PathPatternAction<PropPathPatternElem>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.pattern.AppendProperty(builder.curElemText,
                                       builder.curPredExpr);
        builder.curElemText.clear();
        builder.curPredExpr = SdfPredicateExpression();
    }
};

template <>
struct PathPatternAction<ReflexiveRelative>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.pattern.SetPrefix(SdfPath::ReflexiveRelativePath());
    }
};

template <>
struct PathPatternAction<DotDot>
{
    template <class Input>
    static void apply(Input const &in, PatternBuilder &builder) {
        builder.pattern.AppendChild("..");
    }
};

} // SdfPathPatternActions

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_PATTERN_PARSER_H

