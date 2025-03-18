//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pathExpressionEval.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/ostreamMethods.h"

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

void
Sdf_PathExpressionEvalBase::_PatternIncrSearchState::Pop(int newDepth)
{
    while (!_segmentMatchDepths.empty() &&
           _segmentMatchDepths.back() >= newDepth) {
        _segmentMatchDepths.pop_back();
    }
    if (newDepth <= _constantDepth) {
        _constantDepth = -1;
    }
}

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
            DEBUG_MSG("- Not %s -> %s\n",
                      Stringify(result), Stringify(!result));
            result = !result;
            break;
        case And:
        case Or: {
            DEBUG_MSG("- %s (lhs = %s)\n",
                      *opIter == And ? "And" : "Or", result ? "true" : "false");
            const bool decidingValue = *opIter != And;
            // If the And/Or result is already the deciding value,
            // short-circuit.  Otherwise the result is the rhs, so continue.
            if (result == decidingValue) {
                DEBUG_MSG("- Short-circuiting '%s', with %s\n",
                          *opIter == And ? "And" : "Or",
                          Stringify(result));
                shortCircuit();
            }
        }
            break;
        case Open: DEBUG_MSG("- Open\n"); ++nest; break;
        case Close: DEBUG_MSG("- Close\n"); --nest; break;
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
    // _matchObjType set below.
    _stretchBegin = false;
    _stretchEnd = false;
    auto const &predicateExprs = pattern.GetPredicateExprs();

    // A helper to close and append a segment to _segments.
    auto closeAndAppendSegment = [this]() {
        _segments.push_back({
                _segments.empty() ? 0 : _segments.back().end,
                _components.size()
            });
    };
    
    // This will technically over-reserve by the number of 'stretch' (//)
    // components, but it's worth it to not thrash the heap.
    _components.reserve(pattern.GetComponents().size());
    for (auto iter = std::cbegin(pattern.GetComponents()),
             end = std::cend(pattern.GetComponents()); iter != end; ++iter) {
        SdfPathExpression::PathPattern::Component const &component = *iter;
        // A 'stretch' (//) component.
        if (component.IsStretch()) {
            // If this is the end of the components, mark that.
            if (std::next(iter) == end) {
                _stretchEnd = true;
            }
            // If this pattern begins with stretch, we don't yet have a segment.
            if (_components.empty()) {
                _stretchBegin = true;
            }
            // Otherwise this stretch completes a segment -- append it.
            else {
                closeAndAppendSegment();
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
            // Must match the whole component.
            _regexes.emplace_back("^" + component.text + "$", ArchRegex::GLOB);
            _components.push_back({ Regex,
                    static_cast<int>(_regexes.size()-1), -1 });
        }
        // If the component has a predicate, link that.
        if (component.predicateIndex != -1) {
            _components.back().predicateIndex =
                linkPredicate(predicateExprs[component.predicateIndex]);
        }
    }
    // Close the final segment if necessary, for patterns that do not end in
    // stretch.  Patterns that do end in stretch close the final segment in the
    // above loop.
    if (!_stretchEnd && !_components.empty()) {
        closeAndAppendSegment();
    }

    // Set the object types this pattern can match.  If the pattern isn't
    // explicitly a property, then it can match only prims if the final
    // component's text is not empty.  That is, patterns like '/foo//' or '//'
    // or '/predicate//{test}' can match either prims or properties, but
    // patterns like '/foo//bar', '//baz{test}', '/foo/[Bb]' can only match
    // prims.
    if (pattern.IsProperty()) {
        // The pattern demands a property.
        _matchObjType = _MatchPropOnly;
    }
    else if (_stretchEnd ||
             (!_components.empty() && _components.back().type == ExplicitName &&
              _explicitNames[_components.back().patternIndex].empty())) {
        // Trailing stretch, or last component has empty text means this can
        // match both prims & properties.
        _matchObjType = _MatchPrimOrProp;
    }
    else {
        // No trailing stretch, and the final component requires a prim
        // name/regex match means this pattern can only match prims.
        _matchObjType = _MatchPrimOnly;
    }

    if (TfDebug::IsEnabled(SDF_PATH_EXPRESSION_EVAL)) {
        auto stringifyComponent = [this](_Component const &component) {
            std::string result = (component.type == ExplicitName)
                ? TfStringPrintf(
                    "'%s'", _explicitNames[component.patternIndex].c_str())
                : TfStringPrintf("<regex %d>", component.patternIndex);
            if (component.predicateIndex != -1) {
                result += TfStringPrintf(" pred %d", component.predicateIndex);
            }
            return result;
        };
        std::vector<std::string> segmentStrs;
        for (_Segment const &seg: _segments) {
            std::vector<std::string> compStrs;
            for (size_t i = seg.begin; i != seg.end; ++i) {
                compStrs.push_back(stringifyComponent(_components[i]));
            }
            segmentStrs.push_back("[" + TfStringJoin(compStrs, ", ") + "]");
        }
        DEBUG_MSG("_PatternImplBase::_Init\n"
                  "  pattern      : <%s>\n"
                  "  prefix       : <%s>\n"
                  "  stretchBegin : %d\n"
                  "  stretchEnd   : %d\n"
                  "  segments     : %s\n",
                  pattern.GetText().c_str(),
                  _prefix.GetAsString().c_str(),
                  _stretchBegin,
                  _stretchEnd,
                  TfStringJoin(segmentStrs, ", ").c_str());
    }
}


// Check if \p segment matches at exactly \p pathIterInOut.  Update \p
// pathIterInOut to the position past this match if there is a match and return
// a truthy result.  Otherwise leave \p pathIterInOut untouched and return a
// falsey result.
SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase::
_PatternImplBase::_CheckExactMatch(
    _Segment const &seg,
    _RunNthPredFn runNthPredicate,
    SdfPathVector::const_iterator pathIterEnd,
    SdfPathVector::const_iterator &pathIterInOut) const
{
    using Result = SdfPredicateFunctionResult;
    
    auto pathIter = pathIterInOut;

    const auto end = _components.cbegin() + seg.end;
    auto iter = _components.cbegin() + seg.begin;

    if (std::distance(iter, end) > std::distance(pathIter, pathIterEnd)) {
        DEBUG_MSG("Insufficient path elements remaining at '%s' to match: "
                  "have %zu, need %zu -> varying false\n",
                  pathIter->GetName().c_str(),
                  std::distance(pathIter, pathIterEnd), seg.GetSize());
        return Result::MakeVarying(false);
    }

    for (; iter != end; ++iter, ++pathIter) {
        switch (iter->type) {
        case ExplicitName: {
            // ExplicitName entries with empty text are components with only
            // predicates. (e.g. //{somePredicate}) They implicitly match all
            // names.
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
    TF_AXIOM(iter == end);
    pathIterInOut = pathIter;
    return Result::MakeVarying(true);
};

// Check if \p segment matches at exactly \p pathIterInOut, or at \p
// pathIterInOut-1 if the first component in \p segment is a bare predicate.
SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase::
_PatternImplBase::_CheckMatch(
    _Segment const &seg,
    _RunNthPredFn runNthPredicate,
    SdfPathVector::const_iterator pathIterBegin,
    SdfPathVector::const_iterator pathIterEnd,
    SdfPathVector::const_iterator &pathIterInOut) const {
    SdfPredicateFunctionResult result;
    if (pathIterInOut != pathIterBegin &&
        _IsBarePredicate(_components[seg.begin])) {
        --pathIterInOut;
        result = _CheckExactMatch(
            seg, runNthPredicate, pathIterEnd, pathIterInOut);
        if (result) {
            return result;
        }
        ++pathIterInOut;
    }
    result = _CheckExactMatch(seg, runNthPredicate, pathIterEnd, pathIterInOut);
    return result;
};

SdfPredicateFunctionResult
Sdf_PathExpressionEvalBase::
_PatternImplBase::_Match(
    SdfPath const &path,
    _RunNthPredFn runNthPredicate) const
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
    const bool isPrimPropertyPath = path.IsPrimPropertyPath();
    if (_matchObjType == _MatchPropOnly && !isPrimPropertyPath) {
        DEBUG_MSG("pattern demands a property; <%s> is a prim path -> "
                  "varying false\n", path.GetAsString().c_str());
        return Result::MakeVarying(false);
    }
    if (_matchObjType == _MatchPrimOnly && isPrimPropertyPath) {
        DEBUG_MSG("pattern demands a prim; <%s> is a property path -> "
                  "constant false\n", path.GetAsString().c_str());
        return Result::MakeConstant(false);
    }

    // If this pattern has no components, it matches if it is the same as the
    // prefix, or if it has the prefix if there's a stretch following.
    if (_components.empty()) {
        // Accepts all descendant paths.
        if (_stretchBegin || _stretchEnd) {
            DEBUG_MSG("pattern accepts all descendant paths "
                      "-> constant true\n");
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
    const SdfPathVector prefixes = [&]() {
        // We need an extra prefix in 'prefixes' if we have leading stretch and
        // the first component is a bare predicate, and if this pattern's
        // _prefix has elements.
        const int extraPrefix =
            static_cast<int>(_stretchBegin &&
                             _IsBarePredicate(_components.front()) &&
                             _prefix.GetPathElementCount() != 0);
        size_t numPrefixes =
            path.GetPathElementCount() - _prefix.GetPathElementCount() +
            extraPrefix;
        return numPrefixes ? path.GetPrefixes(numPrefixes) : SdfPathVector {};
    }();

    if (prefixes.empty()) {
        // The path has insufficient components to match.
        DEBUG_MSG("path matches prefix but pattern requires additional "
                  "components -> varying false\n");
        return Result::MakeVarying(false);
    }

    DEBUG_MSG("Examining paths not covered by pattern prefix <%s>:\n    %s\n",
              _prefix.GetAsString().c_str(),
              TfStringify(prefixes).c_str());

    SdfPathVector::const_iterator matchLoc = prefixes.begin();
    const SdfPathVector::const_iterator matchEnd = prefixes.end();
    
    // Process each matching "segment", which is a sequence of matching
    // components separated by "stretch" components.  For example, if the
    // pattern is /foo//bar/baz//qux, there are three segments: [foo], [bar,
    // baz], and [qux]. The first segment [foo] must match at the head of the
    // path. The next segment, [bar, baz] can match anywhere following up to the
    // sum of the number of components in the subsequent segments. The final
    // segment [qux] must match at the end.  There is one slighly sticky detail
    // where a segment that starts with a "bare predicate" can overlap with the
    // prior segment.  E.g. given /foo//{pred}/baz//qux, the [{pred}, baz] could
    // match at `/foo/baz` if `/foo` passes {pred}.
        
    // Note!  In case of a match, this function updates 'matchLoc' to mark the
    // location of the match in [pathBegin, pathEnd).
    auto searchMatch = [&](_Segment const &seg,
                           SdfPathVector::const_iterator pathBegin,
                           SdfPathVector::const_iterator pathEnd,
                           SdfPathVector::const_iterator &matchLocOut) {
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
            matchLocOut = pathBegin;
            result = _CheckMatch(seg, runNthPredicate,
                                 prefixes.begin(), matchEnd, matchLocOut);
            if (result) {
                DEBUG_MSG("found match -> %s\n", Stringify(result));
                return result;
            }
        }
        DEBUG_MSG("no match found -> %s\n", Stringify(result));
        return result;
    };

    // For each segment:
    const size_t componentsSize = _components.size();
    for (_Segment const &segment: _segments) {
        // First segment must match at the beginning if non-stretch.
        if (!_stretchBegin && segment.StartsAt(0)) {
            const Result result =
                _CheckMatch(segment, runNthPredicate,
                            prefixes.begin(), matchEnd, matchLoc);
            DEBUG_MSG("segment %smatch at start -> %s\n",
                      result ? "" : "does not ", Stringify(result));
            if (!result) {
                return result;
            }
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
            if ((size_t)std::distance(matchLoc, matchEnd) <
                _SegmentMinMatchElts(segment)) {
                DEBUG_MSG("insufficient remaining path components for final "
                          "non-stretch match segment (%zd < %zu)\n",
                          std::distance(matchLoc, matchEnd),
                          _SegmentMinMatchElts(segment));
                return Result::MakeVarying(false);
            }
            matchLoc = matchEnd - segment.GetSize();
            const Result result = _CheckExactMatch(segment, runNthPredicate,
                                                   matchEnd, matchLoc);
            DEBUG_MSG("segment %smatch at end -> %s\n",
                      result ? "" : "does not ", Stringify(result));
            if (!result) {
                return result;
            }
        }
        // Interior segments search for a match within the range.
        else {
            // We can restrict the search range by considering how many
            // components we have remaining to match against.
            const Result result =
                searchMatch(segment, matchLoc, matchEnd, matchLoc);
            DEBUG_MSG("found %smatch in interior -> %s\n",
                      result ? "" : "no ", Stringify(result));
            if (!result) {
                return result;
            }
        }
    }

    // We've successfully completed matching.  If we end with a stretch '//'
    // component, we can mark the result constant over descendants.
    if (_stretchEnd) {
        DEBUG_MSG("_Match(<%s>) succeeds with trailing stretch -> "
                  "constant true\n", path.GetAsString().c_str());
        return Result::MakeConstant(true);
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
    using Segment = _PatternImplBase::_Segment;
    using Result = SdfPredicateFunctionResult;
    
    // If we're constant, return the constant value.
    if (search._constantDepth != -1) {
        Result res = Result::MakeConstant(search._constantValue);
        DEBUG_MSG("_Next(<%s>) has constant value at depth %d -> %s\n",
                  path.GetAsString().c_str(), search._constantDepth,
                  Stringify(res));
        return res;
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

    const size_t pathElemCount = path.GetPathElementCount();
    const size_t prefixElemCount = _prefix.GetPathElementCount();

    // Check prefix if we aren't into matching segments yet.  If we are into
    // segments, we have already checked the prefix.
    if (search._segmentMatchDepths.empty() && !path.HasPrefix(_prefix)) {
        // If this path is not a prefix of _prefix, then we can never match.
        if (!_prefix.HasPrefix(path)) {
            DEBUG_MSG("_Next(<%s>) outside of prefix <%s> -> constant false\n",
                      path.GetAsString().c_str(),
                      _prefix.GetAsString().c_str());
            search._constantDepth = prefixElemCount;
            search._constantValue = false;
            return Result::MakeConstant(false);
        }
        // Otherwise we might match once we traverse to _prefix.
        DEBUG_MSG("_Next(<%s>) not yet within prefix <%s> -> varying false\n",
                  path.GetAsString().c_str(),
                  _prefix.GetAsString().c_str());
        return Result::MakeVarying(false);
    }

    // If this pattern demands a either a prim or a property path then we can
    // early-out if the path in question is not the required type.
    const bool isPrimPropertyPath = path.IsPrimPropertyPath();
    if (_matchObjType == _MatchPropOnly && !isPrimPropertyPath) {
        DEBUG_MSG("_Next(<%s>) isn't a property path -> varying false\n",
                  path.GetAsString().c_str());
        return Result::MakeVarying(false);
    }
    if (_matchObjType == _MatchPrimOnly && isPrimPropertyPath) {
        DEBUG_MSG("_Next(<%s>) isn't a prim path -> constant false\n",
                  path.GetAsString().c_str());
        return Result::MakeConstant(false);
    }

    // If this pattern has no components, it matches if there's a stretch or if
    // it is the same length as the prefix (which means it is identical to the
    // prefix, since we've already done the has-prefix check above).
    if (_components.empty()) {
        if (_stretchBegin || _stretchEnd) {
            // The pattern allows arbitrary elements following the prefix. 
            DEBUG_MSG("_Next(<%s>) covered by stretch -> constant true\n",
                      path.GetAsString().c_str());
            search._constantDepth = prefixElemCount;
            search._constantValue = true;
            return Result::MakeConstant(search._constantValue);
        }
        else if (pathElemCount > prefixElemCount) {
            // The given path is descendant to the prefix, but the pattern
            // requires an exact match.
            DEBUG_MSG("_Next(<%s>) must match prefix <%s> exactly -> "
                      "constant false\n",
                      path.GetAsString().c_str(),
                      _prefix.GetAsString().c_str());
            search._constantDepth = prefixElemCount;
            search._constantValue = false;
            return Result::MakeConstant(search._constantValue);
        }
        // The path is exactly _prefix.
        DEBUG_MSG("_Next(<%s>) matches prefix <%s> -> varying true\n",
                  path.GetAsString().c_str(),
                  _prefix.GetAsString().c_str());
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
    
    if (search._segmentMatchDepths.size() == _segments.size()) {
        // We're looking for a rematch with the final segment.
        search._segmentMatchDepths.pop_back();
    }

    // Since segments that start with bare predicates can match the *preceding*
    // element, it means that segments can overlap, and we can match multiple
    // segments without "consuming" any of the path.  For example, the pattern
    // `//{capital}//{{primPath}}` can match `/World` (since it is a capitalized
    // prim path, and bare predicates can consume zero levels of hierarchy).
    // That's why we have a loop here, to accommodate this case.
    while (true) {
    
        const size_t curSegIdx = search._segmentMatchDepths.size();
        Segment const &curSeg = _segments[curSegIdx];
        const bool hasPrevSeg = static_cast<bool>(curSegIdx);
        const bool isFinalSeg = curSegIdx == _segments.size()-1;

        // If we are attempting to match the first segment, ensure we have
        // enough components (or exactly the right number if there is no stretch
        // begin).
        const size_t numMatchComponents = pathElemCount - (
            hasPrevSeg ? search._segmentMatchDepths.back() : prefixElemCount);

        if (numMatchComponents < _SegmentMinMatchElts(curSeg)) {
            // Not enough path components yet, but we could match once we
            // descend to a long enough path.
            DEBUG_MSG("_Next(<%s>) lacks enough matching components (%zu) for "
                      "current segment (%zu) -> varying false\n",
                      path.GetAsString().c_str(),
                      numMatchComponents, curSeg.GetSize());
            return Result::MakeVarying(false);
        }

        const bool hasStretch = hasPrevSeg || _stretchBegin;

        // If we're matching the first segment and there's no stretch begin, the
        // number of components must match exactly.
        if (!hasStretch && numMatchComponents > curSeg.GetSize()) {
            // Too many components; we cannot match this or any descendant path.
            search._constantDepth = pathElemCount;
            search._constantValue = false;
            DEBUG_MSG("_Next(<%s>) matching components (%zu) exceeds "
                      "required number (%zu) -> constant false\n",
                      path.GetAsString().c_str(), numMatchComponents,
                      curSeg.GetSize());
            return Result::MakeConstant(false);
        }

        // Split the path into prefixes.
        // XXX:TODO avoid heap if possible..
        const SdfPathVector prefixes = [&]() {
            // We need an extra prefix in 'prefixes' if this is a bare
            // predicate, and if this pattern's _prefix has elements.
            const int extraPrefix =
                static_cast<int>(hasStretch &&
                                 _IsBarePredicate(_components[curSeg.begin]) &&
                                 path.GetPathElementCount() != 0);
            size_t numPrefixes = numMatchComponents + extraPrefix;
            return numPrefixes ?
                path.GetPrefixes(numPrefixes) : SdfPathVector {};
        }();

        if (prefixes.empty()) {
            DEBUG_MSG("_Next(<%s>) no prefixes -> varying false\n",
                      path.GetAsString().c_str());
            return Result::MakeVarying(false);
        }

        // Check for a match here.  The final segment must match exactly at the
        // end.
        auto matchLoc = prefixes.end()-curSeg.GetSize();
        const Result result = isFinalSeg
            ? _CheckExactMatch(curSeg, runNthPredicate,
                               prefixes.end(), matchLoc)
            : _CheckMatch(curSeg, runNthPredicate,
                          prefixes.begin(), prefixes.end(), matchLoc);

        if (result) {
            // We have matched this component here, so push its match depth.
            search._segmentMatchDepths.push_back(
                matchLoc == prefixes.end() ? pathElemCount : pathElemCount-1);
        }

        if (!result || isFinalSeg) {
            break;
        }
    }
    
    // If we've completed matching all segments...
    if (search._segmentMatchDepths.size() == _segments.size()) {
        // We can mark ourselves constant if we end with stretch.
        if (_stretchEnd) {
            search._constantDepth = search._segmentMatchDepths.back();
            search._constantValue = true;
            DEBUG_MSG("_Next(<%s>) matches with trailing stretch -> "
                      "constant true\n", path.GetAsString().c_str());
            return Result::MakeConstant(true);
        }
        // Otherwise if the last match depth is the path elem count, we match
        // varying true.
        if (static_cast<size_t>(
                search._segmentMatchDepths.back()) == pathElemCount) {
            DEBUG_MSG("_Next(<%s>) matches -> varying true\n",
                      path.GetAsString().c_str());
            return Result::MakeVarying(true);
        }
        // Otherwise there are excess components, and we cannot match.
        search._constantDepth = search._segmentMatchDepths.back();
        search._constantValue = false;
        DEBUG_MSG("_Next(<%s>) has excess components -> constant false\n",
                  path.GetAsString().c_str());
        return Result::MakeConstant(false);
    }
    
    // We have taken the next step, but we have more matching to do.
    DEBUG_MSG("_Next(<%s>) partial yet incomplete match "
              "(%zd of %zd segments) -> varying false\n",
              path.GetAsString().c_str(),
              search._segmentMatchDepths.size(), _segments.size());
    
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
