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

template <class Rule> using OptSpaced = pad<Rule, blank>;

////////////////////////////////////////////////////////////////////////
// Path patterns with predicates.
struct PathPatStretch : two<'/'> {};
struct PathPatSep : sor<PathPatStretch, one<'/'>> {};

// Named close rules so the Errors class can attach targeted messages.
struct PredExprClose     : one<'}'> {};
struct BracketClassClose : one<']'> {};

// '{' commits to requiring a predicate expression and a closing '}'.
// PredExprClose is named so the Errors class can target it specifically.
struct BracedPredExpr
    : if_must<one<'{'>,
              OptSpaced<SdfPredicateExpressionParser::PredExpr>,
              PredExprClose> {};

// '[' commits to requiring valid content and a closing ']'.
struct BracketClass :
    if_must<one<'['>,
            plus<sor<identifier_other, one<'!','-','?','*'>>>,
            BracketClassClose> {};

struct PrimPathWildCard :
    plus<sor<BracketClass, identifier_other, one<'?','*'>>> {};

struct PropPathWildCard :
    plus<sor<BracketClass, identifier_other, one<':','?','*'>>> {};

struct PrimPathPatternElemText : PrimPathWildCard {};
struct PropPathPatternElemText : PropPathWildCard {};

struct PrimPathPatternElem
    : if_then_else<PrimPathPatternElemText, opt<BracedPredExpr>,
                   BracedPredExpr> {};

struct PropPathPatternElem
    : if_then_else<PropPathPatternElemText, opt<BracedPredExpr>,
                   BracedPredExpr> {};

// A single '/' that is not the start of '//'. Used as a commitment point:
// once matched, a path pattern element is unconditionally required.
struct PatSepSlash : seq<one<'/'>, not_at<one<'/'>>> {};

// PatSepSlash commits to requiring a following prim pattern element.
struct PrimPatStep : if_must<PatSepSlash, PrimPathPatternElem> {};

// '//' as separator, but only when a prim element immediately follows.
// Trailing '//' without a following element falls through to
// opt<PathPatStretch> at the end of PathPatternElems.  Note: the lookahead
// parses PathPatStretch+PrimPathPatternElem twice; acceptable given the bounded
// syntax involved.
struct StretchStep : seq<
    at<seq<PathPatStretch, PrimPathPatternElem>>,
    PathPatStretch,
    PrimPathPatternElem> {};

struct PathPatternElems
    : seq<PrimPathPatternElem,
          star<sor<StretchStep, PrimPatStep>>,
          if_must_else<one<'.'>, PropPathPatternElem, opt<PathPatStretch>>> {};

struct AbsPathPattern : seq<PathPatSep, opt<PathPatternElems>> {};

struct DotDot  : two<'.'> {};
struct DotDots : list<DotDot, one<'/'>> {};

struct ReflexiveRelative : one<'.'> {};

struct AbsoluteStart : at<one<'/'>> {};

// After DotDots, single '/' commits to requiring pattern elements.  '//'
// retains optional-elements behavior since a bare '..//' (stretch from ancestor
// context) is a valid pattern with no following elements.
struct DotDotsStep        : if_must<PatSepSlash, PathPatternElems> {};
struct DotDotsStretchTail : seq<PathPatStretch, opt<PathPatternElems>> {};

struct PathPattern :
    sor<
    if_must<AbsoluteStart, AbsPathPattern>,
    seq<DotDots, opt<sor<DotDotsStretchTail, DotDotsStep>>>,
    PathPatternElems,
    seq<ReflexiveRelative, opt<PathPatStretch, opt<PathPatternElems>>>
    >
{};

////////////////////////////////////////////////////////////////////////
// Errors.
//
// Inherits from SdfPredicateExpressionParser::Errors so predicate rule errors
// propagate automatically without re-listing them here. Only
// path-pattern-specific rules need PARSE_ERROR entries.
//
// The struct-override macro is used (rather than a message pointer) because the
// derived Errors class has no message member of its own -- it uses the base's
// raise() for inherited rules and overrides raise() directly for local ones.

template <class Rule>
struct Errors : SdfPredicateExpressionParser::Errors<Rule> {};

#define PARSE_ERROR(rule, msg)                                              \
    template <> struct Errors<rule>                                         \
        : SdfPredicateExpressionParser::Errors<rule> {                      \
        template <class Input, class... States>                             \
        [[noreturn]] static void raise(Input const &in, States &&...) {     \
            throw parse_error(msg, in);                                     \
        }                                                                   \
    }

PARSE_ERROR(PrimPathPatternElem, "expected path pattern element");
PARSE_ERROR(PathPatternElems,    "expected path pattern element after '/'");
PARSE_ERROR(PropPathPatternElem, "expected property pattern element after '.'");
PARSE_ERROR(PredExprClose,       "expected '}' to close predicate expression");
PARSE_ERROR(BracketClassClose,   "expected ']' to close bracket class");

#undef PARSE_ERROR

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
