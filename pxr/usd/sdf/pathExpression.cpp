//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathPatternParser.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "predicateExpressionParser.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    // SdfPathExpression::Op
    TF_ADD_ENUM_NAME(SdfPathExpression::Complement);
    TF_ADD_ENUM_NAME(SdfPathExpression::ImpliedUnion);
    TF_ADD_ENUM_NAME(SdfPathExpression::Union);
    TF_ADD_ENUM_NAME(SdfPathExpression::Intersection);
    TF_ADD_ENUM_NAME(SdfPathExpression::Difference);
    TF_ADD_ENUM_NAME(SdfPathExpression::ExpressionRef);
    TF_ADD_ENUM_NAME(SdfPathExpression::Pattern);
}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfPathExpression>();
    TfType::Define<VtArray<SdfPathExpression>>();
}

SdfPathExpression::ExpressionReference const &
SdfPathExpression::ExpressionReference::Weaker()
{
    static ExpressionReference *theWeaker =
        new ExpressionReference { SdfPath(), "_" };
    return *theWeaker;
}

static bool
ParsePathExpression(std::string const &inputStr,
                    std::string const &parseContext,
                    SdfPathExpression *pattern,
                    std::string *errMsg);

SdfPathExpression::SdfPathExpression(std::string const &inputStr,
                                     std::string const &parseContext)
{
    std::string errMsg;
    if (!inputStr.empty() &&
        !ParsePathExpression(inputStr, parseContext, this, &errMsg)) {
        *this = {};
        _parseError = errMsg;
        TF_RUNTIME_ERROR(errMsg);
    }
}

SdfPathExpression const &
SdfPathExpression::Everything()
{
    static SdfPathExpression const *theEverything = new SdfPathExpression("//");
    return *theEverything;
}

SdfPathExpression const &
SdfPathExpression::EveryDescendant()
{
    static SdfPathExpression const *
        theEveryDescendant = new SdfPathExpression(".//");
    return *theEveryDescendant;
}

SdfPathExpression const &
SdfPathExpression::Nothing()
{
    static SdfPathExpression const *theNothing = new SdfPathExpression {};
    return *theNothing;
}

SdfPathExpression const &
SdfPathExpression::WeakerRef()
{
    static SdfPathExpression const *theWeaker = new SdfPathExpression {
        SdfPathExpression::MakeAtom(ExpressionReference::Weaker())
    };
    return *theWeaker;
}

SdfPathExpression
SdfPathExpression::MakeComplement(SdfPathExpression &&right)
{
    SdfPathExpression ret;

    // If right is either Everything or Nothing, its complement is just the
    // other.
    if (right == Everything()) {
        ret = Nothing();
    }
    else if (right == Nothing()) {
        ret = Everything();
    }
    else {
        // Move over the state, then push back 'Complement'.
        ret._ops = std::move(right._ops);
        ret._refs = std::move(right._refs);
        ret._patterns = std::move(right._patterns);
        // Complement of complement cancels.
        if (ret._ops.back() == Complement) {
            ret._ops.pop_back();
        }
        else {
            ret._ops.push_back(Complement);
        }
    }
    return ret;
}

SdfPathExpression
SdfPathExpression::MakeOp(
    Op op, SdfPathExpression &&left, SdfPathExpression &&right)
{
    SdfPathExpression ret;

    // If we have a Nothing or an Everything operand, then transform A - B into
    // A & ~B.  This makes all cases commutative, simplifying the code.
    if (op == Difference && (left == Nothing() || right == Nothing() ||
                             left == Everything() || right == Everything())) {
        op = Intersection;
        right = MakeComplement(std::move(right));
    }

    // Handle nothing and everything.
    if (left == Nothing()) {
        ret = (op == Intersection) ? Nothing() : std::move(right);
    }
    else if (right == Nothing()) {
        ret = (op == Intersection) ? Nothing() : std::move(left);
    }
    else if (left == Everything()) {
        ret = (op == Intersection) ? std::move(right) : Everything();
    }
    else if (right == Everything()) {
        ret = (op == Intersection) ? std::move(left) : Everything();
    }
    else {
        // Move the right ops, ensure we have enough space, then insert left.
        // Finally push back this new op.
        ret._ops = std::move(right._ops);
        ret._ops.reserve(ret._ops.size() + left._ops.size() + 1);
        ret._ops.insert(
            ret._ops.end(), left._ops.begin(), left._ops.end());
        ret._ops.push_back(op);
        
        // Move the left patterns & refs, then move-insert the same of the
        // right.
        ret._refs = std::move(left._refs);
        ret._refs.insert(ret._refs.end(),
                         make_move_iterator(right._refs.begin()),
                         make_move_iterator(right._refs.end()));
        ret._patterns = std::move(left._patterns);
        ret._patterns.insert(ret._patterns.end(),
                             make_move_iterator(right._patterns.begin()),
                             make_move_iterator(right._patterns.end()));
    }
    return ret;
}

SdfPathExpression
SdfPathExpression::MakeAtom(ExpressionReference &&ref)
{
    /// Just push back a 'ExpressionRef' op and the ref itself.
    SdfPathExpression ret;
    ret._ops.push_back(ExpressionRef);
    ret._refs.push_back(std::move(ref));
    return ret;
}

SdfPathExpression
SdfPathExpression::MakeAtom(PathPattern &&pattern)
{
    /// Just push back a 'Pattern' op and the pattern itself.
    SdfPathExpression ret;
    ret._ops.push_back(Pattern);
    ret._patterns.push_back(std::move(pattern));
    return ret;
}

void
SdfPathExpression::WalkWithOpStack(
    TfFunctionRef<void (std::vector<std::pair<Op, int>> const &)> logic,
    TfFunctionRef<void (ExpressionReference const &)> ref,
    TfFunctionRef<void (PathPattern const &)> pattern) const
{
    // Do nothing if this is the empty expression.
    if (IsEmpty()) {
        return;
    }

    // Operations are stored in reverse order.
    using OpIter = std::vector<Op>::const_reverse_iterator;
    OpIter curOp = _ops.rbegin();

    // Calls are stored in forward order.
    using RefIter = std::vector<ExpressionReference>::const_iterator;
    RefIter curRef = _refs.begin();

    // Patterns are stored in forward order.
    using PatternIter = std::vector<PathPattern>::const_iterator;
    PatternIter curPattern = _patterns.begin();

    // A stack of iterators and indexes tracks where we are in the expression.
    // The indexes delimit the operands while processing an operation:
    //
    // index ----->      0     1      2
    // operation -> Union(<lhs>, <rhs>)
    std::vector<std::pair<Op, int>> stack {1, {*curOp, 0}};

    while (!stack.empty()) {
        Op stackOp = stack.back().first;
        int &operandIndex = stack.back().second;
        int operandIndexEnd = 0;

        // Invoke 'ref' for ExpressionRef operations, 'pattern' for PathPattern
        // operation, otherwise 'logic'.
        if (stackOp == ExpressionRef) {
            ref(*curRef++);
        } else if (stackOp == Pattern) {
            pattern(*curPattern++);
        } else {
            logic(stack);
            operandIndex++;
            operandIndexEnd =
                (stackOp == Complement) ? 2 : 3; // Complement is a unary op.
        }

        // If we've reached the end of an operation, pop it from the stack,
        // otherwise push the next operation on.
        if (operandIndex == operandIndexEnd) {
            stack.pop_back();
        }
        else {
            stack.emplace_back(*(++curOp), 0);
        }
    }
}

void
SdfPathExpression::Walk(
    TfFunctionRef<void (Op, int)> logic,
    TfFunctionRef<void (ExpressionReference const &)> ref,
    TfFunctionRef<void (PathPattern const &)> pattern) const
{
    auto adaptLogic = [&logic](std::vector<std::pair<Op, int>> const &stack) {
        return logic(stack.back().first, stack.back().second);
    };
    return WalkWithOpStack(adaptLogic, ref, pattern);
}

SdfPathExpression
SdfPathExpression::ReplacePrefix(SdfPath const &oldPrefix,
                                 SdfPath const &newPrefix) &&
{
    // We are an rvalue so we mutate & return ourselves.
    for (auto &ref: _refs) {
        ref.path = ref.path.ReplacePrefix(oldPrefix, newPrefix);
    }
    for (auto &pattern: _patterns) {
        pattern.SetPrefix(
            pattern.GetPrefix().ReplacePrefix(oldPrefix, newPrefix));
    }
    return std::move(*this);
}

bool
SdfPathExpression::IsAbsolute() const
{
    for (auto &ref: _refs) {
        if (!ref.path.IsEmpty() && !ref.path.IsAbsolutePath()) {
            return false;
        }
    }
    for (auto const &pat: _patterns) {
        if (!pat.GetPrefix().IsAbsolutePath()) {
            return false;
        }
    }
    return true;
}

SdfPathExpression
SdfPathExpression::MakeAbsolute(SdfPath const &anchor) &&
{
    // We are an rvalue so we mutate & return ourselves.
    for (auto &ref: _refs) {
        ref.path = ref.path.MakeAbsolutePath(anchor);
    }
    for (auto &pattern: _patterns) {
        pattern.SetPrefix(pattern.GetPrefix().MakeAbsolutePath(anchor));
    }
    return std::move(*this);
}

bool
SdfPathExpression::ContainsWeakerExpressionReference() const
{
    for (ExpressionReference const &ref: _refs) {
        if (ref.name == "_") {
            return true;
        }
    }
    return false;
}

SdfPathExpression
SdfPathExpression::ResolveReferences(
    TfFunctionRef<
    SdfPathExpression (ExpressionReference const &)> resolve) &&
{
    if (IsEmpty()) {
        return {};
    }
    
    std::vector<SdfPathExpression> stack;
    
    auto logic = [&stack](Op op, int argIndex) {
        if (op == Complement) {
            if (argIndex == 1) {
                stack.back() = MakeComplement(std::move(stack.back()));
            }
        }
        else {
            if (argIndex == 2) {
                SdfPathExpression arg2 = std::move(stack.back());
                stack.pop_back();
                stack.back() = MakeOp(
                    op, std::move(stack.back()), std::move(arg2));
            }
        }
    };

    auto resolveRef = [&stack, &resolve](ExpressionReference const &ref) {
        stack.push_back(resolve(ref));
    };
    
    auto patternIdent = [&stack](PathPattern const &pattern) {
        stack.push_back(MakeAtom(pattern));
    };

    // Walk, resolving references.
    Walk(logic, resolveRef, patternIdent);

    TF_DEV_AXIOM(stack.size() == 1);

    return std::move(stack.back());
}

SdfPathExpression
SdfPathExpression::ComposeOver(SdfPathExpression const &weaker) &&
{
    if (IsEmpty()) {
        *this = weaker;
        return std::move(*this);
    }
    auto resolve = [&weaker](ExpressionReference const &ref) {
        return ref.name == "_" ? weaker : SdfPathExpression::MakeAtom(ref);
    };
    return std::move(*this).ResolveReferences(resolve);
}

std::string
SdfPathExpression::GetText() const
{
    std::string result;
    if (IsEmpty()) {
        return result;
    }

    auto opName = [](Op k) {
        switch (k) {
        case Complement: return "~";
        case ImpliedUnion: return " ";
        case Union: return " + ";
        case Intersection: return " & ";
        case Difference: return " - ";
        default: break;
        };
        return "<unknown>";
    };

    std::vector<Op> opStack;

    auto printLogic = [&opName, &opStack, &result](
        std::vector<std::pair<Op, int>> const &stack) {

        const Op op = stack.back().first;
        const int argIndex = stack.back().second;
        
        // Parenthesize this subexpression if we have a parent op, and either:
        // - the parent op has a stronger precedence than this op
        // - the parent op has the same precedence as this op, and this op is
        //   the right-hand-side of the parent op.
        bool parenthesize = false;
        if (stack.size() >= 2 /* has a parent op */) {
            Op parentOp;
            int parentIndex;
            std::tie(parentOp, parentIndex) = stack[stack.size()-2];
            parenthesize =
                parentOp < op || (parentOp == op && parentIndex == 2);
        }

        if (parenthesize && argIndex == 0) {
            result += '(';
        }
        if (op == Complement ? argIndex == 0 : argIndex == 1) {
            result += opName(op);
        }
        if (parenthesize &&
            (op == Complement ? argIndex == 1 : argIndex == 2)) {
            result += ')';
        }                
    };

    auto printExprRef = [&result](ExpressionReference const &ref) {
        result += "%" + ref.path.GetAsString();
        result += (ref.name == "_") ? "_" : ":" + ref.name;
    };

    auto printPathPattern = [&result](PathPattern const &pattern) {
        result += pattern.GetText();
    };

    WalkWithOpStack(printLogic, printExprRef, printPathPattern);
    
    return result;
}

std::ostream &
operator<<(std::ostream &out, SdfPathExpression const &expr)
{
    return out << expr.GetText();
}    

////////////////////////////////////////////////////////////////////////
// Parsing code.
struct Sdf_PathExprBuilder
{
    Sdf_PathExprBuilder() { OpenGroup(); }
    
    void PushOp(SdfPathExpression::Op op) { _stacks.back().PushOp(op); }

    void PushExpressionRef(SdfPath &&path, std::string &&name) {
        _stacks.back().PushExpressionRef(std::move(path), std::move(name));
    }

    void PushPattern(SdfPathPattern &&pattern) {
        _stacks.back().PushPattern(std::move(pattern));
    }

    void OpenGroup() { _stacks.emplace_back(); }

    void CloseGroup() {
        SdfPathExpression innerExpr = _stacks.back().Finish();
        _stacks.pop_back();
        _stacks.back().PushExpr(std::move(innerExpr));
    }

    SdfPathExpression Finish() {
        SdfPathExpression result = _stacks.back().Finish();
        _stacks.clear();
        return result;
    }

private:
    struct _Stack {

        void PushOp(SdfPathExpression::Op op) {
            // Reduce while prior ops have higher precendence.
            while (!opStack.empty() && opStack.back() <= op) {
                _Reduce();
            }
            opStack.push_back(op);
        }

        void PushExpressionRef(SdfPath &&path, std::string &&name) {
            exprStack.push_back(
                SdfPathExpression::MakeAtom(
                    { std::move(path), std::move(name) }));
                    
        }

        void PushPattern(SdfPathPattern &&pattern) {
            exprStack.push_back(
                SdfPathExpression::MakeAtom(std::move(pattern)));
        }

        void PushExpr(SdfPathExpression &&expr) {
            exprStack.push_back(std::move(expr));
        }

        SdfPathExpression Finish() {
            while (!opStack.empty()) {
                _Reduce();
            }
            SdfPathExpression ret = std::move(exprStack.back());
            exprStack.clear();
            return ret;
        }

    private:
        void _Reduce() {
            SdfPathExpression::Op op = opStack.back();
            opStack.pop_back();
            SdfPathExpression right = std::move(exprStack.back());
            exprStack.pop_back();

            if (op == SdfPathExpression::Complement) {
                // Complement is the only unary op.
                exprStack.push_back(
                    SdfPathExpression::MakeComplement(std::move(right)));
            }
            else {
                // All other ops are all binary.
                SdfPathExpression left = std::move(exprStack.back());
                exprStack.pop_back();
                exprStack.push_back(
                    SdfPathExpression::MakeOp(
                        op, std::move(left), std::move(right))
                    );
            }
        }                
        
        // Working space.
        std::vector<SdfPathExpression::Op> opStack;
        std::vector<SdfPathExpression> exprStack;
    };

    std::vector<_Stack> _stacks;
};

////////////////////////////////////////////////////////////////////////
// Path expression grammar

namespace {

using namespace PXR_PEGTL_NAMESPACE;

template <class Rule, class Sep>
using LookaheadList = seq<Rule, star<at<Sep, Rule>, Sep, Rule>>;

template <class Rule> using OptSpaced = pad<Rule, blank>;

////////////////////////////////////////////////////////////////////////
// Expression references.
struct DotDot : two<'.'> {};
struct DotDots : list<DotDot, one<'/'>> {};

struct ExpressionRefName : seq<one<':'>, identifier> {};

struct ExpressionRefPathAndName :
    seq<list<identifier, one<'/'>>, ExpressionRefName> {};

struct AbsExpressionRefPath : seq<one<'/'>, ExpressionRefPathAndName> {};

struct RelExpressionRefPath :
    seq<opt<DotDots>,
        if_then_else<one<'/'>,
                     ExpressionRefPathAndName, ExpressionRefName>> {};

struct ExpressionRefPath : sor<AbsExpressionRefPath, RelExpressionRefPath> {};

struct WeakerRef :
    seq<string<'%', '_'>, not_at<sor<identifier_other, one<':'>>>> {};

struct ExpressionReference : sor<
    WeakerRef,
    seq<one<'%'>, ExpressionRefPath>
    > {};


////////////////////////////////////////////////////////////////////////
// Path exprs.

struct PathExpr;

struct PathExprOpenGroup : one<'('> {};
struct PathExprCloseGroup : one<')'> {};

struct PathExprAtom
    : sor<
    ExpressionReference,
    SdfPathPatternParser::PathPattern,
    if_must<PathExprOpenGroup, OptSpaced<PathExpr>, PathExprCloseGroup>
    >
{};

struct PathSetComplement : one<'~'> {};
struct PathSetImpliedUnion : plus<blank> {};
struct PathSetUnion : one<'+'> {};
struct PathSetIntersection : one<'&'> {};
struct PathSetDifference : one<'-'> {};

struct PathFactor : seq<opt<OptSpaced<PathSetComplement>>, PathExprAtom> {};

struct PathOperator : sor<OptSpaced<PathSetUnion>,
                          OptSpaced<PathSetIntersection>,
                          OptSpaced<PathSetDifference>,
                          PathSetImpliedUnion> {};

struct PathExpr : LookaheadList<PathFactor, PathOperator> {};

////////////////////////////////////////////////////////////////////////
// Actions /////////////////////////////////////////////////////////////

template <class Rule>
struct PathExprAction : nothing<Rule> {};

template <>
struct PathExprAction<WeakerRef>
{
    template <class Input>
    static void apply(Input const &in, Sdf_PathExprBuilder &builder) {
        builder.PushExpressionRef(SdfPath(), "_");
    }
};

template <>
struct PathExprAction<ExpressionRefPath>
{
    template <class Input>
    static void apply(Input const &in, Sdf_PathExprBuilder &builder) {
        std::string pathPart = TfStringGetBeforeSuffix(in.string(), ':');
        builder.PushExpressionRef(
            pathPart.empty() ? SdfPath() : SdfPath(pathPart),
            TfStringGetSuffix(in.string(), ':'));
    }
};

template <SdfPathExpression::Op op>
struct PathExprOpAction
{
    template <class Input>
    static void apply(Input const &in, Sdf_PathExprBuilder &builder) {
        builder.PushOp(op);
    }
};

template <> struct PathExprAction<PathSetComplement>
    : PathExprOpAction<SdfPathExpression::Complement> {};
template <> struct PathExprAction<PathSetImpliedUnion>
    : PathExprOpAction<SdfPathExpression::ImpliedUnion> {};
template <> struct PathExprAction<PathSetUnion>
    : PathExprOpAction<SdfPathExpression::Union> {};
template <> struct PathExprAction<PathSetIntersection>
    : PathExprOpAction<SdfPathExpression::Intersection> {};
template <> struct PathExprAction<PathSetDifference>
    : PathExprOpAction<SdfPathExpression::Difference> {};

template <>
struct PathExprAction<PathExprOpenGroup>
{
    template <class Input>
    static void apply(Input const &in, Sdf_PathExprBuilder &builder) {
        builder.OpenGroup();
    }
};

template <>
struct PathExprAction<PathExprCloseGroup>
{
    template <class Input>
    static void apply(Input const &in, Sdf_PathExprBuilder &builder) {
        builder.CloseGroup();
    }
};

// This rule dispatches to the PathPatternAction to build the pattern, then we
// can consume it in the 'success' member function.
template <>
struct PathExprAction<SdfPathPatternParser::PathPattern>
    : change_action_and_states<
    SdfPathPatternActions::PathPatternAction,
    SdfPathPatternActions::PatternBuilder>
{
    template <class Input>
    static void success(Input const &,
                        SdfPathPatternActions::PatternBuilder &patternBuilder,
                        Sdf_PathExprBuilder &builder) {
        builder.PushPattern(std::move(patternBuilder.pattern));
    }
};

} // anon

////////////////////////////////////////////////////////////////////////
// Parsing drivers.

static bool
ParsePathExpression(std::string const &inputStr,
                    std::string const &parseContext,
                    SdfPathExpression *expr,
                    std::string *errMsgOut)
{
    Sdf_PathExprBuilder builder;
    try {
        parse<must<pad<PathExpr, blank>, eolf>, PathExprAction>(
            string_input<> { inputStr,
                parseContext.empty() ? "<input>" : parseContext.c_str()
            }, builder);
        if (expr) {
            *expr = builder.Finish();
        }
    }
    catch (parse_error const &err) {
        std::string errMsg = err.what();
        errMsg += " -- ";
        bool first = true;
        for (position const &p: err.positions()) {
            if (!first) {
                errMsg += ", ";
            }
            first = false;
            errMsg += to_string(p);
        }
        if (errMsgOut) {
            *errMsgOut = std::move(errMsg);
        }
        return false;
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
