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

static char const *
Stringify(SdfPredicateFunctionResult r) {
    if (r) { return r.IsConstant() ? "constant true" : "varying true"; }
    else { return r.IsConstant() ? "constant false" : "varying false"; }
}

TF_CONDITIONALLY_COMPILE_TIME_ENABLED_DEBUG_CODES(
    false, // Set to 'true' to compile-in debug output support.
    SDF_PATH_EXPRESSION_EVAL
    );

#define DEBUG_MSG(...) \
    TF_DEBUG_MSG(SDF_PATH_EXPRESSION_EVAL, __VA_ARGS__)

SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase::_EvalExpr(
    TfFunctionRef<
    SdfPredicateFunctionResult (bool /*skip*/)> evalPattern) const
{
    SdfPredicateFunctionResult result =
        SdfPredicateFunctionResult::MakeConstant(false);
    int nest = 0;
    auto opIter = _ops.cbegin(), opEnd = _ops.cend();

    // The current implementation favors short-circuiting over constance
    // propagation.  It might be beneficial to avoid short-circuiting when
    // constancy isn't known, in hopes of establishing constancy.  See similar
    // comments in predicateProgram.h, for SdfPredicateProgram::operator() for
    // more detail.

    // Helper for short-circuiting.  Advance, ignoring everything until we
    // reach the next Close that brings us to the starting nest level.
    auto shortCircuit = [&]() {
        const int origNest = nest;
        for (; opIter != opEnd; ++opIter) {
            switch(*opIter) {
            case EvalPattern:
                evalPattern(/*skip=*/true); break; // Skip matches.
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
    DEBUG_MSG("_EvalExpr\n");
    for (; opIter != opEnd; ++opIter) {
        switch (*opIter) {
        case EvalPattern:
            DEBUG_MSG("- EvalPattern\n");
            result.SetAndPropagateConstancy(evalPattern(/*skip=*/false));
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
_PatternImplBase::_Init(
    SdfPathExpression::PathPattern const &pattern,
    TfFunctionRef<int (SdfPredicateExpression const &)> linkPredicate)
{
    // Build a matcher.
    _prefix = pattern.GetPrefix();
    _isProperty = pattern.IsProperty();
    _stretchBegin = false;
    _stretchEnd = false;
    auto const &predicateExprs = pattern.GetPredicateExprs();

    // This will technically over-reserve by the number of 'stretch' (//)
    // components, but it's worth it to not thrash the heap.
    _components.reserve(pattern.GetComponents().size());
    for (auto iter = std::cbegin(pattern.GetComponents()),
             end = std::cend(pattern.GetComponents()); iter != end; ++iter) {
        SdfPathExpression::PathPattern::Component const &component = *iter;
        // A 'stretch' (//) component.
        if (component.IsStretch()) {
            if (_components.empty()) {
                _stretchBegin = true;
            }
            // If this is the end of the components, just mark that.
            if (std::next(iter) == end) {
                _stretchEnd = true;
            }
            // Otherwise finish any existing segment & start a new segment.
            else {
                if (_segments.empty()) {
                    _segments.push_back({ 0, 0 });
                }
                _segments.back().end = _components.size();
                _segments.push_back({ _components.size(), _components.size() });
            }
            continue;
        }
        // A literal text name (or empty name which must have a predicate).
        if (component.isLiteral || component.text.empty()) {
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
    // Close the final segment.
    if (!_components.empty()) {
        if (_segments.empty()) {
            _segments.push_back({ 0, 0 });
        }
        _segments.back().end = _components.size();
    }
}

SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase::
_PatternImplBase::_Match(
    SdfPath const &path,
    TfFunctionRef<SdfPredicateFunctionResult (int, SdfPath const &)>
    runNthPredicate) const
{
    DEBUG_MSG("_Match(<%s>)\n", path.GetAsString().c_str());
    
    using ComponentIter = typename std::vector<_Component>::const_iterator;
    using Result = SdfPredicateFunctionResult;
    
    // Only support prim and prim property paths.
    if (!path.IsAbsoluteRootOrPrimPath() &&
        !path.IsPrimPropertyPath()) {
        TF_WARN("Unsupported path <%s>; can only match prim or "
                "prim-property paths", path.GetAsString().c_str());
        return Result::MakeConstant(false);
    }

    // Check prefix & property-ness.  If this pattern demands a property path
    // then we can early-out if the path in question is not a property path.
    // Otherwise this path may or may not match properties.
    if (!path.HasPrefix(_prefix)) {
        // If the given path is a prefix of _prefix, then this is a varying
        // false, since descendants could match. Otherwise a constant false.
        Result result = _prefix.HasPrefix(path) ?
            Result::MakeVarying(false) : Result::MakeConstant(false);
        DEBUG_MSG("<%s> lacks prefix <%s> -> %s\n",
                  path.GetAsString().c_str(),
                  _prefix.GetAsString().c_str(),
                  Stringify(result));
        return result;
    }
    if (_isProperty && !path.IsPrimPropertyPath()) {
        DEBUG_MSG("pattern demands a property; <%s> is not -> varying false\n",
                  path.GetAsString().c_str());
        return Result::MakeVarying(false);
    }

    // If this pattern has no components, it matches if it is the same as the
    // prefix, or if it has the prefix if there's a stretch following.
    if (_components.empty()) {
        // Accepts all descendant paths.
        if (_stretchBegin || _stretchEnd) {
            DEBUG_MSG("pattern accepts all paths -> constant true\n");
            return Result::MakeConstant(true);
        }
        // Accepts only the prefix exactly.
        if (path == _prefix) {
            DEBUG_MSG("pattern accepts exactly <%s> == <%s> -> varying true\n",
                      _prefix.GetAsString().c_str(),
                      path.GetAsString().c_str());
            return Result::MakeVarying(true);
        }
        DEBUG_MSG("pattern accepts exactly <%s> != <%s> -> constant false\n",
                  _prefix.GetAsString().c_str(),
                  path.GetAsString().c_str());
        return Result::MakeConstant(false);
    }

    // Split the path into prefixes but skip any covered by _prefix.
    // XXX:TODO Plumb-in caller-supplied vector for reuse by GetPrefixes().
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

    // Check if \p segment matches at exactly \p pathIter.
    auto checkMatch = [this, &runNthPredicate](
        _Segment const &seg, SdfPathVector::const_iterator pathIter) {

        for (auto iter = _components.cbegin() + seg.begin,
                 end = _components.cbegin() + seg.end;
             iter != end; ++iter, ++pathIter) {
            switch (iter->type) {
            case ExplicitName: {
                // ExplicitName entries with empty text are components with only
                // predicates. (e.g. //{somePredicate}) They implicitly match
                // all names.
                std::string const &name = _explicitNames[iter->patternIndex];
                if (!name.empty() && name != pathIter->GetName()) {
                    DEBUG_MSG("Name '%s' != '%s' -> varying false\n",
                              name.c_str(), pathIter->GetName().c_str());
                    return Result::MakeVarying(false);
                }
                DEBUG_MSG("Name '%s' == '%s' -> continuing\n",
                          name.c_str(), pathIter->GetName().c_str());
            }
                break;
            case Regex:
                if (!_regexes[iter->patternIndex].Match(pathIter->GetName())) {
                    DEBUG_MSG("Regex does not match '%s' -> varying false\n",
                              pathIter->GetName().c_str());
                    return Result::MakeVarying(false);
                }
                DEBUG_MSG("Regex matches '%s' -> continuing\n",
                          pathIter->GetName().c_str());
                break;
            };
            // Evaluate a predicate if this component has one.
            if (iter->predicateIndex != -1) {
                Result predResult =
                    runNthPredicate(iter->predicateIndex, *pathIter);
                if (!predResult) {
                    // The predicate's result's constancy is valid to
                    // propagate here.
                    DEBUG_MSG("Predicate fails '%s' -> %s\n",
                              pathIter->GetAsString().c_str(),
                              Stringify(predResult));
                    return predResult;
                }
            }
        }
        return Result::MakeVarying(true);
    };

    // Note!  In case of a match, this function updates 'matchLoc' to mark the
    // location of the match in [pathBegin, pathEnd).
    auto searchMatch = [&](_Segment const &seg,
                           SdfPathVector::const_iterator pathBegin,
                           SdfPathVector::const_iterator pathEnd) {
        // Search the range [pathBegin, pathEnd) to match seg.
        // Naive search to start... TODO: improve!
        size_t segSize = seg.GetSize();
        size_t numPaths = std::distance(pathBegin, pathEnd);
        if (segSize > numPaths) {
            DEBUG_MSG("segment longer than path components -> varying false\n");
            return Result::MakeVarying(false);
        }

        SdfPathVector::const_iterator
            pathSearchEnd = pathBegin + (numPaths - segSize) + 1;

        Result result;
        for (; pathBegin != pathSearchEnd; ++pathBegin) {
            DEBUG_MSG("checking match at <%s>\n",
                      pathBegin->GetAsString().c_str());
            result = checkMatch(seg, pathBegin);
            if (result) {
                DEBUG_MSG("found match -> %s\n", Stringify(result));
                matchLoc = pathBegin;
                return result;
            }
        }
        DEBUG_MSG("no match found -> %s\n", Stringify(result));
        return result;
    };            

    // Track the number of matching components remaining.
    int numComponentsLeft = _components.size();

    // For each segment:
    const size_t componentsSize = _components.size();
    for (_Segment const &segment: _segments) {
        // If there are more matching components remaining than the number of
        // path elements, this cannot possibly match.
        if (numComponentsLeft > std::distance(matchLoc, matchEnd)) {
            return Result::MakeVarying(false);
        }

        // Decrement number of matching components remaining by this segment's
        // size.
        numComponentsLeft -= segment.GetSize();

        // First segment must match at the beginning.
        if (!_stretchBegin && segment.StartsAt(0)) {
            const Result result = checkMatch(segment, matchLoc);
            if (!result) {
                DEBUG_MSG("segment does not match at start -> %s\n",
                          Stringify(result));
                return result;
            }
            matchLoc += segment.GetSize();
            // If there is only one segment, it needs to match the whole.
            if (!_stretchEnd &&
                segment.EndsAt(componentsSize) &&
                matchLoc != matchEnd) {
                DEBUG_MSG("segment does not match at end -> varying false\n");
                return Result::MakeVarying(false);
            }
        }
        // Final segment must match at the end.
        else if (!_stretchEnd && segment.EndsAt(componentsSize)) {
            const Result result =
                checkMatch(segment, matchEnd - segment.GetSize());
            if (!result) {
                DEBUG_MSG("segment does not match at end -> %s\n",
                          Stringify(result));
                return result;
            }
            matchLoc = matchEnd;
        }
        // Interior segments search for a match within the range.
        else {
            // We can restrict the search range by considering how many
            // components we have remaining to match against.
            const Result result =
                searchMatch(segment, matchLoc, matchEnd - numComponentsLeft);
            if (!result) {
                DEBUG_MSG("found no match in interior -> %s\n",
                          Stringify(result));
                return result;
            }
            matchLoc += segment.GetSize();
        }
    }

    DEBUG_MSG("_Match(<%s>) succeeds -> varying true\n",
              path.GetAsString().c_str());

    return Result::MakeVarying(true);
}

SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase
::_PatternImplBase::_Next(
    _PatternIncrSearchState &search,
    SdfPath const &path,
    TfFunctionRef<
    SdfPredicateFunctionResult (int, SdfPath const &)> runNthPredicate) const
{
    using Result = SdfPredicateFunctionResult;
    
    // If we're constant, return the constant value.
    if (search._constantDepth != -1) {
        return Result::MakeConstant(search._constantValue);
    }
    
    // Only support prim and prim property paths.
    if (!path.IsAbsoluteRootOrPrimPath() &&
        !path.IsPrimPropertyPath()) {
        TF_WARN("Unsupported path <%s>; can only match prim or "
                "prim-property paths", path.GetAsString().c_str());
        search._constantDepth = 0;
        search._constantValue = false;
        return Result::MakeConstant(false);
    }

    // Check prefix if we aren't into matching segments yet.  If we are into
    // segments, we have already checked the prefix.
    if (search._segmentMatchDepths.empty() && !path.HasPrefix(_prefix)) {
        search._constantDepth = 0;
        search._constantValue = false;
        return Result::MakeConstant(false);
    }

    // If this pattern demands a property path then we can early-out if the path
    // in question is not a property path.  Otherwise this path may or may not
    // match properties.
    if (_isProperty && !path.IsPrimPropertyPath()) {
        return Result::MakeVarying(false);
    }

    const size_t pathElemCount = path.GetPathElementCount();
    const size_t prefixElemCount = _prefix.GetPathElementCount();

    // If this pattern has no components, it matches if there's a stretch or if
    // it is the same length as the prefix (which means it is identical to the
    // prefix, since we've already done the has-prefix check above).
    if (_components.empty()) {
        if (_stretchBegin || _stretchEnd) {
            // The pattern allows arbitrary elements following the prefix.
            search._constantDepth = 0;
            search._constantValue = true;
            return Result::MakeConstant(search._constantValue);
        }
        else if (pathElemCount > prefixElemCount) {
            // The given path is descendant to the prefix, but the pattern
            // requires an exact match.
            search._constantDepth = 0;
            search._constantValue = false;
            return Result::MakeConstant(search._constantValue);
        }
        // The path is exactly _prefix.
        return Result::MakeVarying(true);
    }

    // We're not a constant value, the prefix matches, and we have components to
    // match against -- we're looking to match those components.  Get the
    // segment we're trying to match.  If we've already matched all segments but
    // we're still searching, it means we need to try to re-match the final
    // segment.  Consider a case like //Foo//foo/bar incrementally matching
    // against the path /Foo/geom/foo/bar/foo/bar/foo/bar.  We'll keep
    // rematching the final foo/bar bit, to get /Foo/geom/foo/bar,
    // /Foo/geom/foo/bar/foo/bar, and /Foo/geom/foo/bar/foo/bar/foo/bar.  In
    // this case we pop the final segment match depth to proceed with rematching
    // that segment.
    using Segment = _PatternImplBase::_Segment;
    if (search._segmentMatchDepths.size() == _segments.size()) {
        // We're looking for a rematch with the final segment.
        search._segmentMatchDepths.pop_back();
    }
    const size_t curSegIdx = search._segmentMatchDepths.size();
    Segment const &curSeg = _segments[curSegIdx];
    Segment const *prevSegPtr = curSegIdx ? &_segments[curSegIdx-1] : nullptr;
    
    // If we are attempting to match the first segment, ensure we have enough
    // components (or exactly the right number if there is no stretch begin).
    
    const size_t numMatchComponents = pathElemCount - (
        prevSegPtr ?
        search._segmentMatchDepths.back() + prevSegPtr->GetSize() :
        prefixElemCount);

    if (numMatchComponents < curSeg.GetSize()) {
        // Not enough path components yet, but we could match once we
        // descend to a long enough path.
        return Result::MakeVarying(false);
    }

    // If we're matching the first segment and there's no stretch begin, the
    // number of components must match exactly.
    if (!prevSegPtr &&
        !_stretchBegin && numMatchComponents > curSeg.GetSize()) {
        // Too many components; we cannot match this or any descendant path.
        search._constantDepth = pathElemCount;
        search._constantValue = false;
        return Result::MakeConstant(false);
    }

    // Check for a match here.  Go from the end of the path back, and look for
    // literal component matches first since those are the fastest to check.

    SdfPath workingPath = path;
    auto compIter = make_reverse_iterator(_components.cbegin() + curSeg.end),
        compEnd = make_reverse_iterator(_components.cbegin() + curSeg.begin);

    // First pass explicit names & their predicates.
    for (; compIter != compEnd;
         ++compIter, workingPath = workingPath.GetParentPath()) {
        if (compIter->type == _PatternImplBase::ExplicitName) {
            // ExplicitName entries with empty text are components with only
            // predicates. (e.g. //{somePredicate}) They implicitly match all
            // names.
            std::string const &name = _explicitNames[compIter->patternIndex];
            if (!name.empty() && name != workingPath.GetName()) {
                return Result::MakeVarying(false);
            }
            // Invoke predicate if this component has one.
            if (compIter->predicateIndex != -1) {
                SdfPredicateFunctionResult predResult =
                    runNthPredicate(compIter->predicateIndex, workingPath);
                if (!predResult) {
                    if (predResult.IsConstant()) {
                        search._constantDepth = pathElemCount;
                        search._constantValue = false;
                    }
                    return predResult;
                }
            }
        }
    }
    // Second pass, regexes & their predicates.
    compIter = make_reverse_iterator(_components.cbegin() + curSeg.end);
    workingPath = path;
    for (; compIter != compEnd;
         ++compIter, workingPath = workingPath.GetParentPath()) {
        if (compIter->type == _PatternImplBase::Regex) {
            if (!_regexes[compIter->patternIndex].Match(
                    workingPath.GetName())) {
                return Result::MakeVarying(false);
            }
            // Invoke predicate if this component has one.
            if (compIter->predicateIndex != -1) {
                SdfPredicateFunctionResult predResult =
                    runNthPredicate(compIter->predicateIndex, workingPath);
                if (!predResult) {
                    if (predResult.IsConstant()) {
                        search._constantDepth = pathElemCount;
                        search._constantValue = false;
                    }
                    return predResult;
                }
            }
        }
    }

    // We have matched this component here, so push its match depth.
    search._segmentMatchDepths.push_back(pathElemCount);

    // If we've completed matching, we can mark ourselves constant if we end
    // with stretch.
    if (search._segmentMatchDepths.size() == _segments.size()) {
        if (_stretchEnd) {
            search._constantDepth = pathElemCount;
            search._constantValue = true;
            return Result::MakeConstant(true);
        }
        return Result::MakeVarying(true);
    }

    // We have taken the next step, but we have more matching to do.
    return Result::MakeVarying(false);
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
                        expr.GetText().c_str());
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
        case Expr::Pattern: return Eval::EvalPattern;
        case Expr::ExpressionRef:
            TF_CODING_ERROR("Building evaluator for incomplete "
                            "SdfPathExpression");
            break;
        };
        return static_cast<typename Eval::_Op>(-1);
    };

    auto translateLogic = [&](Expr::Op op, int argIndex) {
        switch (op) {
        case Expr::Complement: // Complement (aka Not) is postfix, RPN-style.
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
                        expr.GetText().c_str());
    };

    TfErrorMark m;

    // Walk the expression and build the "compiled" evaluator.
    expr.Walk(translateLogic, issueReferenceError, translatePattern);

    return m.IsClean();
}

PXR_NAMESPACE_CLOSE_SCOPE
