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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pathExpressionEval.h"

#include "pxr/base/tf/errorMark.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
Sdf_PathExpressionEvalBase::_Match(
    TfFunctionRef<bool (bool /*skip*/)> patternMatch) const
{
    bool result = false;
    int nest = 0;
    auto opIter = _ops.cbegin(), opEnd = _ops.cend();

    // Helper for short-circuiting.  Advance, ignoring everything until we
    // reach the next Close that brings us to the starting nest level.
    auto shortCircuit = [&]() {
        const int origNest = nest;
        for (; opIter != opEnd; ++opIter) {
            switch(*opIter) {
            case PatternMatch:
                patternMatch(/*skip=*/true); break; // Skip matches.
            case Not: case And: case Or: break; // Skip operations.
            case Open: ++nest; break;
            case Close:
                if (--nest == origNest) {
                    return;
                }
                break;
            };
        }
    };
    
    // Evaluate the predicate expression by processing operations and invoking
    // predicate functions.
    for (; opIter != opEnd; ++opIter) {
        switch (*opIter) {
        case PatternMatch:
            result = patternMatch(/*skip=*/false);
            break;
        case Not:
            result = !result;
            break;
        case And:
        case Or: {
            const bool decidingValue = *opIter != And;
            // If the And/Or result is already the deciding value,
            // short-circuit.  Otherwise the result is the rhs, so continue.
            if (result == decidingValue) {
                shortCircuit();
            }
        }
            break;
        case Open: ++nest; break;
        case Close: --nest; break;
        };
    }
    return result;
}

void
Sdf_PathExpressionEvalBase::
_PatternMatchBase::_Init(
    SdfPathExpression::PathPattern const &pattern,
    TfFunctionRef<int (SdfPredicateExpression const &)> linkPredicate)
{
    // Build a matcher.
    _prefix = pattern.GetPrefix();
    _isProperty = pattern.IsProperty();
    auto const &predicateExprs = pattern.GetPredicateExprs();
    _numMatchingComponents = pattern.GetComponents().size();
    for (SdfPathExpression::PathPattern::Component const &component:
             pattern.GetComponents()) {
        // A 'stretch' (//) component.
        if (component.IsStretch()) {
            // Stretches are non-matching components.
            --_numMatchingComponents;
            _components.push_back({ Stretch, -1, -1 });
        }
        // A literal text name (or empty name which must have a predicate).
        else if (component.isLiteral || component.text.empty()) {
            _explicitNames.push_back(component.text);
            _components.push_back({ ExplicitName,
                    static_cast<int>(_explicitNames.size()-1), -1 });
        }
        // A glob pattern (we translate to regex).
        else {
            _regexes.emplace_back(component.text, ArchRegex::GLOB);
            _components.push_back({ Regex,
                    static_cast<int>(_regexes.size()-1), -1 });
        }
        // If the component has a predicate, link that.
        if (component.predicateIndex != -1) {
            _components.back().predicateIndex =
                linkPredicate(predicateExprs[component.predicateIndex]);
        }
    }
}

bool
Sdf_PathExpressionEvalBase::
_PatternMatchBase::_Match(SdfPath const &path,
                          TfFunctionRef<
                          bool (int, SdfPath const &)> runNthPredicate) const
{
    using ComponentIter = typename std::vector<_Component>::const_iterator;
    
    // Only support prim and prim property paths.
    if (!path.IsAbsoluteRootOrPrimPath() &&
        !path.IsPrimPropertyPath()) {
        TF_WARN("Unsupported path <%s>; can only match prim or "
                "prim-property paths", path.GetAsString().c_str());
        return false;
    }

    // If this pattern has no components, it matches if it is the same as the
    // prefix.
    if (_components.empty()) {
        return path == _prefix;
    }
    
    // Check prefix & property-ness.  If this pattern demands a property path
    // then we can early-out if the path in question is not a property path.
    // Otherwise this path may or may not match properties.
    if (!path.HasPrefix(_prefix) ||
        (_isProperty && !path.IsPrimPropertyPath())) {
        return false;
    }

    // Split the path into prefixes but skip any covered by _prefix.
    SdfPathVector prefixes;
    path.GetPrefixes(
        &prefixes, path.GetPathElementCount() - _prefix.GetPathElementCount());
    
    SdfPathVector::const_iterator matchLoc = prefixes.begin();
    const SdfPathVector::const_iterator matchEnd = prefixes.end();

    // Process each matching "segment", which is a sequence of matching
    // components separated by "stretch" components.  For example, if the
    // pattern is /foo//bar/baz//qux, there are three segments: [foo], [bar,
    // baz], and [qux]. The first segment [foo] must match at the head of the
    // path. The next segment, [bar, baz] can match anywhere following up to the
    // sum of the number of components in the subsequent segments. The final
    // segment [qux] must match at the end.

    const auto componentEnd = _components.cend();

    struct Segment {
        bool IsEmpty() const {
            return begin == end;
        }
        bool StartsAt(ComponentIter iter) const {
            return begin == iter;
        }
        bool EndsAt(ComponentIter iter) const {
            return end == iter;
        }
        size_t GetSize() const {
            return std::distance(begin, end);
        }
        ComponentIter begin;
        ComponentIter end;
    };

    // Advance \p seg to the next segment.  If \p priming is true, the begin
    // iterator is not adjusted -- this is used to set up the first segment.
    auto nextSegment = [componentEnd](Segment &seg, bool priming=false) {
        if (!priming) {
            seg.begin = seg.end;
            while (seg.begin != componentEnd && seg.begin->type == Stretch) {
                ++seg.begin;
            }
            seg.end = seg.begin;
        }
        while (seg.end != componentEnd && seg.end->type != Stretch) {
            ++seg.end;
        }
    };

    // Check if \p segment matches at exactly \p pathIter.
    auto checkMatch = [this, &runNthPredicate](
        Segment const &seg, SdfPathVector::const_iterator pathIter) {

        for (auto iter = seg.begin; iter != seg.end; ++iter, ++pathIter) {
            switch (iter->type) {
            case ExplicitName: {
                // ExplicitName entries with empty text are components with only
                // predicates. (e.g. //{somePredicate}) They implicitly match
                // all names.
                std::string const &name = _explicitNames[iter->patternIndex];
                if (!name.empty() && name != pathIter->GetName()) {
                    return false;
                }
            }
                break;
            case Regex:
                if (!_regexes[iter->patternIndex].Match(pathIter->GetName())) {
                    return false;
                }
                break;
            case Stretch:
                TF_CODING_ERROR("invalid 'stretch' component in segment");
                break;
            };
            // Evaluate a predicate if this component has one.
            if (iter->predicateIndex != -1 &&
                !runNthPredicate(iter->predicateIndex, *pathIter)) {
                return false;
            }
        }
        return true;
    };

    // Note!  In case of a match, this function updates 'matchLoc' to mark the
    // location of the match in [pathBegin, pathEnd).
    auto searchMatch = [&](Segment const &seg,
                           SdfPathVector::const_iterator pathBegin,
                           SdfPathVector::const_iterator pathEnd) {
        // Search the range [pathBegin, pathEnd) to match seg.
        // Naive search to start... TODO: improve!
        size_t segSize = std::distance(seg.begin, seg.end);
        size_t numPaths = std::distance(pathBegin, pathEnd);
        if (segSize > numPaths) {
            return false;
        }

        SdfPathVector::const_iterator
            pathSearchEnd = pathBegin + (numPaths - segSize) + 1;

        for (; pathBegin != pathSearchEnd; ++pathBegin) {
            if (checkMatch(seg, pathBegin)) {
                matchLoc = pathBegin;
                return true;
            }
        }
        return false;
    };            

    // Track the number of matching components remaining.
    int numMatchingComponentsLeft = _numMatchingComponents;

    // For each segment:
    Segment segment { _components.cbegin(), _components.cbegin() };
    nextSegment(segment, /*priming=*/true);
    for (; !segment.StartsAt(componentEnd); nextSegment(segment)) {
        // Skip empty segments.
        if (segment.IsEmpty()) {
            continue;
        }

        // If there are more matching components remaining than the number of
        // path elements, this cannot possibly match.
        if (numMatchingComponentsLeft > std::distance(matchLoc, matchEnd)) {
            return false;
        }

        // Decrement number of matching components remaining by this segment's
        // size.
        numMatchingComponentsLeft -= segment.GetSize();

        // First segment must match at the beginning.
        if (segment.StartsAt(_components.cbegin())) {
            if (!checkMatch(segment, matchLoc)) {
                return false;
            }
            matchLoc += segment.GetSize();
            // If there is only one segment, it needs to match the whole.
            if (segment.EndsAt(_components.cend()) && matchLoc != matchEnd) {
                return false;
            }
        }
        // Final segment must match at the end.
        else if (segment.EndsAt(_components.cend())) {
            if (!checkMatch(segment, matchEnd - segment.GetSize())) {
                return false;
            }
            matchLoc = matchEnd;
        }
        // Interior segments search for a match within the range.
        else {
            // We can restrict the search range by considering how many
            // components we have remaining to match against.
            if (!searchMatch(segment, matchLoc,
                             matchEnd - numMatchingComponentsLeft)) {
                return false;
            }
            matchLoc += segment.GetSize();
        }
    }

    return true;
}

bool
Sdf_MakePathExpressionEvalImpl(
    Sdf_PathExpressionEvalBase &eval,
    SdfPathExpression const &expr,
    TfFunctionRef<
    void (SdfPathExpression::PathPattern const &)> translatePattern)
{
    using Expr = SdfPathExpression;
    using Eval = Sdf_PathExpressionEvalBase;

    if (!expr.IsComplete()) {
        TF_CODING_ERROR("Cannot build evaluator for incomplete "
                        "SdfPathExpression; must contain only absolute "
                        "paths and no expression references: <%s>",
                        expr.GetDebugString().c_str());
        return false;
    }
    
    // Walk expr and populate eval.
    std::string errs;

    auto exprToEvalOp = [](Expr::Op op) {
        switch (op) {
        case Expr::Complement: return Eval::Not;
        case Expr::Union: case Expr::ImpliedUnion: return Eval::Or;
        case Expr::Intersection: case Expr::Difference: return Eval::And;
            // Note that Difference(A, B) is transformed to And(A, !B) below.
        case Expr::Pattern: return Eval::PatternMatch;
        case Expr::ExpressionRef:
            TF_CODING_ERROR("Building evaluator for incomplete "
                            "SdfPathExpression");
            break;
        };
        return static_cast<typename Eval::_Op>(-1);
    };

    auto translateLogic = [&](Expr::Op op, int argIndex) {
        switch (op) {
        case Expr::Complement: // Not is postfix, RPN-style.
            if (argIndex == 1) {
                eval._ops.push_back(Eval::Not);
            }
            break;
        case Expr::Union:        // Binary logic ops are infix to facilitate
        case Expr::ImpliedUnion: // short-circuiting.
        case Expr::Intersection:
        case Expr::Difference:
            if (argIndex == 1) {
                eval._ops.push_back(exprToEvalOp(op));
                eval._ops.push_back(Eval::Open);
            }
            else if (argIndex == 2) {
                // The set-difference operation (a - b) is transformed to (a &
                // ~b) which is represented in boolean logic as (a and not b),
                // so we apply a postfix Not here if the op is 'Difference'.
                if (op == Expr::Difference) {
                    eval._ops.push_back(Eval::Not);
                }
                eval._ops.push_back(Eval::Close);
            }
            break;
        case Expr::Pattern:
            break; // do nothing, handled in translatePattern.
        case Expr::ExpressionRef:
            TF_CODING_ERROR("Cannot build evaluator for incomplete "
                            "SdfPathExpression");
            break;
        };
    };

    // This should never be called, since the path expression is checked for
    // "completeness" above, which means that it must have no unresolved
    // references.
    auto issueReferenceError = [&expr](Expr::ExpressionReference const &) {
        TF_CODING_ERROR("Unexpected reference in path expression: <%s>",
                        expr.GetDebugString().c_str());
    };

    TfErrorMark m;

    // Walk the expression and build the "compiled" evaluator.
    expr.Walk(translateLogic, issueReferenceError, translatePattern);

    return m.IsClean();
}

PXR_NAMESPACE_CLOSE_SCOPE

