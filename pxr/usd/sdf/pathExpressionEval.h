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
    class _PatternMatchBase {
    protected:
        // This is not a constructor because the subclass wants to invoke this
        // from its ctor, TfFunctionRef currently requires an lvalue, which is
        // hard to conjure in a ctor initializer list.
        SDF_API
        void _Init(SdfPathExpression::PathPattern const &pattern,
                   TfFunctionRef<
                   int (SdfPredicateExpression const &)> linkPredicate);

        SDF_API
        bool _Match(SdfPath const &path,
                    TfFunctionRef<
                    bool (int, SdfPath const &)> runNthPredicate) const;

        enum _ComponentType {
            Stretch,           // the "//", arbitrary levels of hierarchy.
            ExplicitName,      // an explicit name (not a glob pattern).
            Regex              // a glob pattern (handled via regex).
        };

        struct _Component {
            _ComponentType type;
            int patternIndex; // into either _explicitNames or _regexes
            int predicateIndex;  // into _predicates or -1 if no predicate.
        };
        
        SdfPath _prefix;
        std::vector<_Component> _components;
        std::vector<std::string> _explicitNames;
        std::vector<ArchRegex> _regexes;
        size_t _numMatchingComponents; // of type ExplicitName or Regex.
        bool _isProperty; // true if this pattern matches only properties.
    };

    // The passed \p patternMatch function must do two things: 1, if \p skip is
    // false, test the current pattern for a match (otherwise skip it) and 2,
    // advance to be ready to test the next pattern for a match on the next call
    // to \p patternMatch.
    SDF_API
    bool _Match(TfFunctionRef<bool (bool /*skip*/)> patternMatch) const;
    
    enum _Op { PatternMatch, Not, Open, Close, Or, And };
    
    std::vector<_Op> _ops;
};


/// \class SdfPathExpressionEval
///
/// Objects of this class evaluate complete SdfPathExpressions on instances of
/// DomainType.  See SdfMakePathExpressionEval() to create instances of this
/// class, passing the expression to evaluate and an
/// SdfPredicateLibrary<DomainType> to evaluate any embedded predicates.
///
/// This class must be able to find the DomainType instance associated with an
/// SdfPath and vice-versa. There are two ways to do this; either explicitly by
/// passing function objects to Match(), or by providing ADL-found overloads of
/// the following two functions:
///
/// \code
/// SdfPath SdfPathExpressionObjectToPath(DomainType);
/// DomainType SdfPathExpressionPathToObject(SdfPath const &,
///                                          DomainType const *);
/// \endcode
///
/// The function SdfPathExpressionPathToObject()'s second argument is always
/// nullptr. It exists to disambiguate overloads, since functions cannot be
/// overloaded on return-type. It is also an argument rather than an explicit
/// template parameter since function calls with explicit template arguments are
/// not found via ADL.
///
template <class DomainType>
class SdfPathExpressionEval : public Sdf_PathExpressionEvalBase
{
    struct _DefaultToPath {
        auto operator()(DomainType obj) const {
            return SdfPathExpressionObjectToPath(obj);
        }
    };
    
    struct _DefaultToObj {
        auto operator()(SdfPath const &path) const {
            return SdfPathExpressionPathToObject(
                path, static_cast<std::decay_t<DomainType> const *>(nullptr));
        }
    };
    
public:
    /// Make an SdfPathExpressionEval object to evaluate \p expr using \p lib to
    /// link any embedded predicate expressions.
    friend SdfPathExpressionEval
    SdfMakePathExpressionEval<DomainType>(
        SdfPathExpression const &expr,
        SdfPredicateLibrary<DomainType> const &lib);

    /// Test \p obj for a match with this expression. Overloads of
    /// SdfPathExpressionPathToObject() and SdfPathExpressionObjectToPath() for
    /// \a DomainType must be found by ADL.
    bool Match(DomainType obj) {
        return Match(obj, _DefaultToPath {}, _DefaultToObj {});
    }
    
    /// Test \p obj for a match with this expression. Use \p objToPath and \p
    /// pathToObj to map between corresponding SdfPath and DomainType instances.
    template <class ObjectToPath, class PathToObject>
    bool Match(DomainType obj,
               ObjectToPath const &objToPath,
               PathToObject const &pathToObj) const {
        auto matchIter = _matches.cbegin();
        auto patternMatch = [&](bool skip) {
            if (skip) {
                ++matchIter;
                return false;
            }
            return (*matchIter++)(obj, objToPath, pathToObj);
        };
        return _Match(patternMatch);
    }

private:

    // This object implements matching against a single path pattern.
    class _PatternMatch : public _PatternMatchBase {
    public:
        _PatternMatch(SdfPathExpression::PathPattern const &pattern,
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
        bool operator()(DomainType obj,
                        ObjectToPath const &objToPath,
                        PathToObject const &pathToObj) const {
            auto runNthPredicate =
                [this, &pathToObj](int i, SdfPath const &path) {
                    return _predicates[i](pathToObj(path));
                };
            return _Match(objToPath(obj), runNthPredicate);
        }
        
    private:
        std::vector<SdfPredicateProgram<DomainType>> _predicates;
    };
    
    std::vector<_PatternMatch> _matches;
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
        // Add a _PatternMatch object that tests a DomainType object against
        // pattern.
        eval._matches.emplace_back(pattern, lib);
        eval._ops.push_back(Eval::PatternMatch);
    };

    if (!Sdf_MakePathExpressionEvalImpl(eval, expr, translatePattern)) {
        eval = {};
    }

    return eval;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PATH_EXPRESSION_EVAL_H
