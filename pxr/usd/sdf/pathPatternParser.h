//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

