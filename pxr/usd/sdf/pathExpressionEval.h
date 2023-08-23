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

// fwd decl
SDF_API
bool
Sdf_MakePathExpressionEvalImpl(
    Sdf_PathExpressionEvalBase &eval,
    SdfPathExpression const &expr,
    TfFunctionRef<
    void (SdfPathExpression::PathPattern const &)> translatePattern);

// Helper base class for SdfPathExpressionEval.  This factors out as much
// template-parameter independent code as possible to reduce bloat & compile
// times.
class Sdf_PathExpressionEvalBase
{
public:
    friend bool
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
        void Pop(int newDepth) {
            while (!_segmentMatchDepths.empty() &&
                   _segmentMatchDepths.back() >= newDepth) {
                _segmentMatchDepths.pop_back();
            }
            if (newDepth <= _constantDepth) {
                _constantDepth = -1;
            }
        }
    private:
        std::vector<int> _segmentMatchDepths;
        int _constantDepth = -1; // 0 means constant at the _prefix level.
        bool _constantValue = false;
    };

    class _PatternImplBase {
    protected:
        // This is not a constructor because the subclass wants to invoke this
        // from its ctor, TfFunctionRef currently requires an lvalue, which is
        // hard to conjure in a ctor initializer list.
        SDF_API
        void _Init(SdfPathExpression::PathPattern const &pattern,
                   TfFunctionRef<
                   int (SdfPredicateExpression const &)> linkPredicate);

        SDF_API
        SdfPredicateFunctionResult
        _Match(
            SdfPath const &path,
            TfFunctionRef<SdfPredicateFunctionResult (int, SdfPath const &)>
            runNthPredicate) const;

        SDF_API
        SdfPredicateFunctionResult
        _Next(_PatternIncrSearchState &searchState,
              SdfPath const &path,
              TfFunctionRef<SdfPredicateFunctionResult (int, SdfPath const &)>
              runNthPredicate) const;

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
        
        SdfPath _prefix;
        std::vector<_Component> _components;
        std::vector<_Segment> _segments;
        std::vector<std::string> _explicitNames;
        std::vector<ArchRegex> _regexes;

        bool _stretchBegin;
        bool _stretchEnd;
        bool _isProperty; // true if this pattern matches only properties.
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

        // Check obj for a match against this pattern.
        template <class ObjectToPath, class PathToObject>
        SdfPredicateFunctionResult
        Match(DomainType const &obj,
              ObjectToPath const &objToPath,
              PathToObject const &pathToObj) const {
            auto runNthPredicate =
                [this, &pathToObj](int i, SdfPath const &path) {
                    return _predicates[i](pathToObj(path));
                };
            return _Match(objToPath(obj), runNthPredicate);
        }

        // Perform the next incremental search step against this pattern.
        template <class ObjectToPath, class PathToObject>
        SdfPredicateFunctionResult
        Next(DomainType const &obj,
             _PatternIncrSearchState &search,
             ObjectToPath const &objToPath,
             PathToObject const &pathToObj) const {
            auto runNthPredicate =
                [this, &pathToObj](int i, SdfPath const &path) {
                    return _predicates[i](pathToObj(path));
                };
            return _Next(search, objToPath(obj), runNthPredicate);
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

    /// Test \p obj for a match with this expression.
    template <class ObjectToPath, class PathToObject>
    SdfPredicateFunctionResult
    Match(DomainType const &obj,
          ObjectToPath const &objToPath,
          PathToObject const &pathToObj) const {
        if (IsEmpty()) {
            return SdfPredicateFunctionResult::MakeConstant(false);
        }
        auto patternImplIter = _patternImpls.cbegin();
        auto evalPattern = [&](bool skip) {
            return skip ? (++patternImplIter, SdfPredicateFunctionResult()) :
                (*patternImplIter++).Match(obj, objToPath, pathToObj);
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
    template <class ObjectToPath, class PathToObject>
    class IncrementalSearcher {
    public:
        IncrementalSearcher() : _eval(nullptr), _lastPathDepth(0) {}
        
        IncrementalSearcher(SdfPathExpressionEval const *eval,
                            ObjectToPath const &o2p,
                            PathToObject const &p2o)
            : _eval(eval)
            , _incrSearchStates(_eval->_patternImpls.size())
            , _objToPath(o2p)
            , _pathToObj(p2o)
            , _lastPathDepth(0) {}

        IncrementalSearcher(SdfPathExpressionEval const *eval,
                            ObjectToPath &&o2p,
                            PathToObject &&p2o)
            : _eval(eval)
            , _incrSearchStates(_eval->_patternImpls.size())
            , _objToPath(std::move(o2p))
            , _pathToObj(std::move(p2o))
            , _lastPathDepth(0) {}

        /// Advance the search to the next \p object, and return the result of
        /// evaluating the expression on it.
        /// 
        /// The passed \p obj must have a path that could succeed the previous
        /// object's path in a valid depth-first ordering.  That is, it must be
        /// a direct child, a sibling, or the sibling of an ancestor.  For
        /// example, the following paths are in a valid order:
        ///
        /// /foo, /foo/bar, /foo/bar/baz, /foo/bar/qux, /oof, /oof/zab /oof/xuq
        ///
        SdfPredicateFunctionResult
        Next(DomainType const &obj) {
            auto patternImplIter = _eval->_patternImpls.begin();
            auto stateIter = _incrSearchStates.begin();
            int newDepth = _objToPath(obj).GetPathElementCount();
            const int popLevel = (newDepth <= _lastPathDepth) ? newDepth : 0;
            auto patternStateNext = [&](bool skip) {
                if (popLevel) {
                    stateIter->Pop(popLevel);
                }
                return skip ? (++patternImplIter, SdfPredicateFunctionResult())
                    : (*patternImplIter++).Next(
                        obj, *stateIter++, _objToPath, _pathToObj);
            };
            _lastPathDepth = newDepth;
            return _eval->_EvalExpr(patternStateNext);
        }

        /// Reset this object's incremental search state so that a new round of
        /// searching may begin.
        void Reset() {
            *this = IncrementalSearcher {
                _eval, std::move(_objToPath), std::move(_pathToObj)
            };
        }
        
    private:
        SdfPathExpressionEval const *_eval;
        std::vector<_PatternIncrSearchState> _incrSearchStates;
        
        ObjectToPath _objToPath;
        PathToObject _pathToObj;

        int _lastPathDepth;
    };

    /// Create an IncrementalSearcher object, using \p objToPath and \p
    /// pathToObject to map DomainType instances to their paths and vice-versa.
    template <class ObjectToPath, class PathToObject>
    IncrementalSearcher<std::decay_t<ObjectToPath>,
                        std::decay_t<PathToObject>>
    MakeIncrementalSearcher(ObjectToPath &&objToPath,
                            PathToObject &&pathToObj) const {
        return IncrementalSearcher<
            std::decay_t<ObjectToPath>,
            std::decay_t<PathToObject>>(
                this, std::forward<ObjectToPath>(objToPath),
                std::forward<PathToObject>(pathToObj));
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
