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
#ifndef PXR_USD_SDF_PREDICATE_PROGRAM_H
#define PXR_USD_SDF_PREDICATE_PROGRAM_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/functionTraits.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/predicateExpression.h"
#include "pxr/usd/sdf/predicateLibrary.h"
#include "pxr/usd/sdf/invoke.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// fwd decl
template <class DomainType>
class SdfPredicateProgram;

// fwd decl
template <class DomainType>
SdfPredicateProgram<DomainType>
SdfLinkPredicateExpression(SdfPredicateExpression const &expr,
                           SdfPredicateLibrary<DomainType> const &lib);

/// \class SdfPredicateProgram
///
/// Represents a callable "program", the result of linking an
/// SdfPredicateExpression with an SdfPredicateLibrary via
/// SdfLinkPredicateExpression().
///
/// The main public interface this class exposes is the function-call
/// operator(), accepting a single argument of type `DomainType`, as it is
/// specified to the template.  Consider using `const Type &` as the
/// `DomainType` for both SdfPredicateProgram and SdfPredicateLibrary if it's
/// important that domain type instances aren't passed by-value.
///
template <class DomainType>
class SdfPredicateProgram
{
public:
    using PredicateFunction =
        typename SdfPredicateLibrary<DomainType>::PredicateFunction;
    
    friend SdfPredicateProgram
    SdfLinkPredicateExpression<DomainType>(
        SdfPredicateExpression const &expr,
        SdfPredicateLibrary<DomainType> const &lib);

    /// Return true if this program has any ops, false otherwise.
    explicit operator bool() const {
        return !_ops.empty();
    }

    /// Run the predicate program on \p obj, and return the result.
    SdfPredicateFunctionResult
    operator()(DomainType const &obj) const {
        SdfPredicateFunctionResult result =
            SdfPredicateFunctionResult::MakeConstant(false);
        int nest = 0;
        auto funcIter = _funcs.cbegin();
        auto opIter = _ops.cbegin(), opEnd = _ops.cend();

        // The current implementation favors short-circuiting over constance
        // propagation.  It might be beneficial to avoid short-circuiting when
        // constancy isn't known, in hopes of establishing constancy.  For
        // example, if we have 'A or B', and 'A' evaluates to 'true' with
        // MayVaryOverDescendants, we will skip evaluating B
        // (short-circuit). This means we would miss the possibility of
        // upgrading the constancy in case B returned 'true' with
        // ConstantOverDescendants.  This isn't a simple switch to flip though;
        // we'd have to do some code restructuring here.
        //
        // For posterity, the rules for propagating constancy are the following,
        // where A and B are the truth-values, and c(A), c(B), are whether or
        // not the constancy is ConstantOverDescendants for A, B, respectively:
        //
        // c(A  or B) =  (A and c(A)) or  (B and c(B)) or (c(A) and c(B))
        // c(A and B) = (!A and c(A)) or (!B and c(B)) or (c(A) and c(B))
        
        // Helper for short-circuiting "and" and "or" operators.  Advance,
        // ignoring everything until we reach the next Close that brings us to
        // the starting nest level.
        auto shortCircuit = [&]() {
            const int origNest = nest;
            for (; opIter != opEnd; ++opIter) {
                switch(*opIter) {
                case Call: ++funcIter; break; // Skip calls.
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

        // Evaluate the predicate expression by processing operations and
        // invoking predicate functions.
        for (; opIter != opEnd; ++opIter) {
            switch (*opIter) {
            case Call:
                result.SetAndPropagateConstancy((*funcIter++)(obj));
                break;
            case Not: result = !result; break;
            case And: case Or: {
                const bool decidingValue = *opIter != And;
                // If the and/or result is already the deciding value,
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
    
private:
    enum _Op { Call, Not, Open, Close, And, Or };
    std::vector<_Op> _ops;
    std::vector<PredicateFunction> _funcs;
};


/// Link \p expr with \p lib and return a callable program that evaluates \p
/// expr on given objects of the \p DomainType.  If linking \p expr and \p lib
/// fails, issue a TF_RUNTIME_ERROR with a message, and return an empty program.
template <class DomainType>
SdfPredicateProgram<DomainType>
SdfLinkPredicateExpression(SdfPredicateExpression const &expr,
                           SdfPredicateLibrary<DomainType> const &lib)
{
    using Expr = SdfPredicateExpression;
    using Program = SdfPredicateProgram<DomainType>;
    
    // Walk expr and populate prog, binding calls with lib.

    Program prog;
    std::string errs;

    auto exprToProgramOp = [](Expr::Op op) {
        switch (op) {
        case Expr::Call: return Program::Call;
        case Expr::Not: return Program::Not;
        case Expr::ImpliedAnd: case Expr::And: return Program::And;
        case Expr::Or: return Program::Or;
        };
        return static_cast<typename Program::_Op>(-1);
    };

    auto translateLogic = [&](Expr::Op op, int argIndex) {
        switch (op) {
        case Expr::Not: // Not is postfix, RPN-style.
            if (argIndex == 1) {
                prog._ops.push_back(Program::Not);
            }
            break;
        case Expr::ImpliedAnd: // Binary logic ops are infix to facilitate
        case Expr::And:        // short-circuiting.
        case Expr::Or:
            if (argIndex == 1) {
                prog._ops.push_back(exprToProgramOp(op));
                prog._ops.push_back(Program::Open);
            }
            else if (argIndex == 2) {
                prog._ops.push_back(Program::Close);
            }
            break;
        case Expr::Call:
            break; // do nothing, handled in translateCall.
        };
    };

    auto translateCall = [&](Expr::FnCall const &call) {
        // Try to bind the call against library overloads.  If successful,
        // insert a call op and the function.
        if (auto fn = lib._BindCall(call.funcName, call.args)) {
            prog._funcs.push_back(std::move(fn));
            prog._ops.push_back(Program::Call);
        }
        else {
            if (!errs.empty()) {
                errs += ", ";
            }
            errs += "Failed to bind call of " + call.funcName;
        }
    };

    // Walk the expression and build the "compiled" program.
    expr.Walk(translateLogic, translateCall);

    if (!errs.empty()) {
        prog = {};
        TF_RUNTIME_ERROR(errs);
    }
    return prog;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PREDICATE_PROGRAM_H
