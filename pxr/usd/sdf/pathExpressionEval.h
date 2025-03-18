//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PATH_EXPRESSION_EVAL_H
#define PXR_USD_SDF_PATH_EXPRESSION_EVAL_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/usd/sdf/predicateLibrary.h"
#include "pxr/usd/sdf/predicateProgram.h"

#include "pxr/base/arch/regex.h"
#include "pxr/base/tf/functionRef.h"

#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// fwd decl
template <class DomainType>
class SdfPathExpressionEval;

// fwd decl
template <class DomainType>
SdfPathExpressionEval<DomainType>
SdfMakePathExpressionEval(SdfPathExpression const &expr,
                          SdfPredicateLibrary<DomainType> const &lib);

// fwd decl
class Sdf_PathExpressionEvalBase;

// Helper base class for SdfPathExpressionEval.  This factors out as much
// template-parameter independent code as possible to reduce bloat & compile
// times.
class Sdf_PathExpressionEvalBase
{
public:
    friend SDF_API bool
    Sdf_MakePathExpressionEvalImpl(
        Sdf_PathExpressionEvalBase &eval,
        SdfPathExpression const &expr,
        TfFunctionRef<
        void (SdfPathExpression::PathPattern const &)> translatePattern);

    /// Return true if this is the empty evalutator.  The empty evaluator always
    /// returns false from operator().
    bool IsEmpty() const {
        return _ops.empty();
    }        

    /// Return true if this is not the empty evaluator, false otherwise.
    explicit operator bool() const {
        return !IsEmpty();
    }

protected:
    class _PatternImplBase;
    
    class _PatternIncrSearchState {
        friend class _PatternImplBase;
    public:
        SDF_API void Pop(int newDepth);
    private:
        std::vector<int> _segmentMatchDepths;
        int _constantDepth = -1;
        bool _constantValue = false;
    };

    class _PatternImplBase {
    protected:
        using _RunNthPredFn =
            TfFunctionRef<SdfPredicateFunctionResult (int, SdfPath const &)>;
        
        // This is not a constructor because the subclass wants to invoke this
        // from its ctor, TfFunctionRef currently requires an lvalue, which is
        // hard to conjure in a ctor initializer list.
        SDF_API
        void _Init(SdfPathExpression::PathPattern const &pattern,
                   TfFunctionRef<
                   int (SdfPredicateExpression const &)> linkPredicate);

        SDF_API
        SdfPredicateFunctionResult
        _Match(SdfPath const &path, _RunNthPredFn runNthPredicate) const;

        SDF_API
        SdfPredicateFunctionResult
        _Next(_PatternIncrSearchState &searchState,
              SdfPath const &path, _RunNthPredFn runNthPredicate) const;


        enum _ComponentType {
            ExplicitName,      // an explicit name (not a glob pattern).
            Regex              // a glob pattern (handled via regex).
        };

        struct _Component {
            _ComponentType type;
            int patternIndex; // into either _explicitNames or _regexes
            int predicateIndex;  // into _predicates or -1 if no predicate.
        };

        struct _Segment {
            // A _Segment is a half-open interval [begin, end) in _components.
            bool IsEmpty() const { return begin == end; }
            bool StartsAt(size_t idx) const { return begin == idx; }
            bool EndsAt(size_t idx) const { return end == idx; }
            size_t GetSize() const { return end - begin; }
            size_t begin, end;
        };
        
        bool _IsBarePredicate(_Component const &comp) const {
            // An empty explicit name component with a predicate is a "bare
            // predicate".
            return comp.type == ExplicitName &&
                _explicitNames[comp.patternIndex].empty() &&
                comp.predicateIndex != -1;
        }

        size_t _SegmentMinMatchElts(_Segment const &seg) const {
            // If a segment starts with a bare predicate, it may match the bare
            // predicate to the *prior* path element, so it requires one fewer
            // path element to match than the number of segment components.
            return seg.GetSize() - (_IsBarePredicate(
                                        _components[seg.begin]) ? 1 : 0);
        }

        // Check if \p seg matches at exactly \p pathIterInOut.  Update \p
        // pathIterInOut to the position past this match if there is a match and
        // return a truthy result.  Otherwise leave \p pathIterInOut untouched
        // and return a falsey result.
        SDF_API
        SdfPredicateFunctionResult
        _CheckExactMatch(_Segment const &seg,
                         _RunNthPredFn runNthPredicate,
                         SdfPathVector::const_iterator pathIterEnd,
                         SdfPathVector::const_iterator &pathIterInOut) const;

        // Check if \p seg matches at \p pathIterInOut, or at
        // `std::prev(pathIterInOut)` if \p seg starts with a bare predicate
        // (like //{predicate}).  Update \p pathIterInOut to the position past
        // the match if there is a match and return a truthy result.  Otherwise
        // leave \p pathIterInOut untouched and return a falsey result.  The key
        // difference between _CheckMatch() and _CheckExactMatch() is that if \p
        // seg begins with a bare predicate, it can potentially match at
        // `std::prev(pathIterInOut)`.
        SDF_API
        SdfPredicateFunctionResult
        _CheckMatch(_Segment const &seg, 
                    _RunNthPredFn runNthPredicate,
                    SdfPathVector::const_iterator pathIterBegin,
                    SdfPathVector::const_iterator pathIterEnd,
                    SdfPathVector::const_iterator &pathIterInOut) const;
        
        
        SdfPath _prefix;
        std::vector<_Component> _components;
        std::vector<_Segment> _segments;
        std::vector<std::string> _explicitNames;
        std::vector<ArchRegex> _regexes;

        bool _stretchBegin;
        bool _stretchEnd;
        enum : uint8_t {
            // The kind of objects this pattern is capable of matching.
            _MatchPrimOrProp, _MatchPrimOnly, _MatchPropOnly
        } _matchObjType;
    };


    // The passed \p invokePattern function must do two things: 1, if \p skip is
    // false, test the current pattern for a match (otherwise skip it) and 2,
    // advance to be ready to test the next pattern for a match on the next call
    // to \p invokePattern.
    SDF_API
    SdfPredicateFunctionResult
    _EvalExpr(TfFunctionRef<
              SdfPredicateFunctionResult (bool /*skip*/)> invokePattern) const;
    
    enum _Op { EvalPattern, Not, Open, Close, Or, And };
    
    std::vector<_Op> _ops;
};

/// \class SdfPathExpressionEval
///
/// Objects of this class evaluate complete SdfPathExpressions.  See
/// SdfMakePathExpressionEval() to create instances of this class, passing the
/// expression to evaluate and an SdfPredicateLibrary to evaluate any embedded
/// predicates.
///
///
template <class DomainType>
class SdfPathExpressionEval : public Sdf_PathExpressionEvalBase
{
    // This object implements matching against a single path pattern.
    class _PatternImpl : public _PatternImplBase {
    public:
        _PatternImpl() = default;

        _PatternImpl(SdfPathExpression::PathPattern const &pattern,
                     SdfPredicateLibrary<DomainType> const &predLib) {
            auto linkPredicate =
                [this, &predLib](SdfPredicateExpression const &predExpr) {
                    _predicates.push_back(
                        SdfLinkPredicateExpression(predExpr, predLib));
                    return _predicates.size()-1;
                };
            _Init(pattern, linkPredicate);
        }

        // Check objPath for a match against this pattern.
        template <class PathToObject>
        SdfPredicateFunctionResult
        Match(SdfPath const &objPath,
              PathToObject const &pathToObj) const {
            auto runNthPredicate =
                [this, &pathToObj](int i, SdfPath const &path) {
                    return _predicates[i](pathToObj(path));
                };
            return _Match(objPath, runNthPredicate);
        }

        // Perform the next incremental search step against this pattern.
        template <class PathToObject>
        SdfPredicateFunctionResult
        Next(SdfPath const &objPath,
             _PatternIncrSearchState &search,
             PathToObject const &pathToObj) const {
            auto runNthPredicate =
                [this, &pathToObj](int i, SdfPath const &path) {
                    return _predicates[i](pathToObj(path));
                };
            return _Next(search, objPath, runNthPredicate);
        }
        
    private:
        std::vector<SdfPredicateProgram<DomainType>> _predicates;
    };

public:
    /// Make an SdfPathExpressionEval object to evaluate \p expr using \p lib to
    /// link any embedded predicate expressions.
    friend SdfPathExpressionEval
    SdfMakePathExpressionEval<DomainType>(
        SdfPathExpression const &expr,
        SdfPredicateLibrary<DomainType> const &lib);

    bool IsEmpty() const {
        return _patternImpls.empty();
    }

    /// Test \p objPath for a match with this expression.
    template <class PathToObject>
    SdfPredicateFunctionResult
    Match(SdfPath const &objPath,
          PathToObject const &pathToObj) const {
        if (IsEmpty()) {
            return SdfPredicateFunctionResult::MakeConstant(false);
        }
        auto patternImplIter = _patternImpls.cbegin();
        auto evalPattern = [&](bool skip) {
            return skip ? (++patternImplIter, SdfPredicateFunctionResult()) :
                (*patternImplIter++).Match(objPath, pathToObj);
        };
        return _EvalExpr(evalPattern);
    }

    /// \class IncrementalSearcher
    ///
    /// This class implements stateful incremental search over DomainType
    /// objects in depth-first order.  See Next() for more info.  This class is
    /// copyable, and may be copied to parallelize searches over domain
    /// subtrees, where one copy is invoked with a child, and the other with the
    /// next sibling.
    template <class PathToObject>
    class IncrementalSearcher {
    public:
        IncrementalSearcher() : _eval(nullptr), _lastPathDepth(0) {}
        
        IncrementalSearcher(SdfPathExpressionEval const *eval,
                            PathToObject const &p2o)
            : _eval(eval)
            , _incrSearchStates(_eval->_patternImpls.size())
            , _pathToObj(p2o)
            , _lastPathDepth(0) {}

        IncrementalSearcher(SdfPathExpressionEval const *eval,
                            PathToObject &&p2o)
            : _eval(eval)
            , _incrSearchStates(_eval->_patternImpls.size())
            , _pathToObj(std::move(p2o))
            , _lastPathDepth(0) {}

        /// Advance the search to the next \p objPath, and return the result of
        /// evaluating the expression on it.
        /// 
        /// The passed \p objPath must possibly succeed the previous object's
        /// path in a valid depth-first ordering.  That is, it must be a direct
        /// child, a sibling, or the sibling of an ancestor.  For example, the
        /// following paths are in a valid order:
        ///
        /// /foo, /foo/bar, /foo/bar/baz, /foo/bar/qux, /oof, /oof/zab /oof/xuq
        ///
        SdfPredicateFunctionResult
        Next(SdfPath const &objPath) {
            auto patternImplIter = _eval->_patternImpls.begin();
            auto stateIter = _incrSearchStates.begin();
            const int newDepth = objPath.GetPathElementCount();
            const bool pop = newDepth <= _lastPathDepth;
            auto patternStateNext = [&](bool skip) {
                if (pop) {
                    stateIter->Pop(newDepth);
                }
                auto const &patternImpl = *patternImplIter++;
                auto &state = *stateIter++;
                return skip
                    ? SdfPredicateFunctionResult {}
                    : patternImpl.Next(objPath, state, _pathToObj);
            };
            _lastPathDepth = newDepth;
            return _eval->_EvalExpr(patternStateNext);
        }

        /// Reset this object's incremental search state so that a new round of
        /// searching may begin.
        void Reset() {
            *this = IncrementalSearcher { _eval, std::move(_pathToObj) };
        }
        
    private:
        SdfPathExpressionEval const *_eval;
        std::vector<_PatternIncrSearchState> _incrSearchStates;
        
        PathToObject _pathToObj;

        int _lastPathDepth;
    };

    /// Create an IncrementalSearcher object, using \p pathToObject to map
    /// DomainType instances to their paths.
    template <class PathToObject>
    IncrementalSearcher<std::decay_t<PathToObject>>
    MakeIncrementalSearcher(PathToObject &&pathToObj) const {
        return IncrementalSearcher<std::decay_t<PathToObject>>(
            this, std::forward<PathToObject>(pathToObj));
    }

private:
    std::vector<_PatternImpl> _patternImpls;
};

/// Create an SdfPathExpressionEval object that can evaluate the complete
/// SdfPathExpression \p expr on DomainType instances, using \p lib, an
/// SdfPredicateLibrary<DomainType> to evaluate any embedded predicates.
///
/// Note that \p expr must be "complete", meaning that
/// SdfPathExpression::IsComplete() must return true.  If an evaluator cannot
/// succesfully be made, possibly because the passed \expr is not complete, or
/// if any embedded SdfPredicateExpression s cannot be successfully linked with
/// \p lib, or another reason, issue an error and return the empty
/// SdfPathExpressionEval object.  See SdfPathExpressionEval::IsEmpty().
///
template <class DomainType>
SdfPathExpressionEval<DomainType>
SdfMakePathExpressionEval(SdfPathExpression const &expr,
                          SdfPredicateLibrary<DomainType> const &lib)
{
    using Expr = SdfPathExpression;
    using Eval = SdfPathExpressionEval<DomainType>;

    Eval eval;

    auto translatePattern = [&](Expr::PathPattern const &pattern) {
        // Add a _PatternImpl object that tests a DomainType object against
        // pattern.
        eval._patternImpls.emplace_back(pattern, lib);
        eval._ops.push_back(Eval::EvalPattern);
    };

    if (!Sdf_MakePathExpressionEvalImpl(eval, expr, translatePattern)) {
        eval = {};
    }

    return eval;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_EXPRESSION_EVAL_H
