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
#include "pxr/usd/sdf/predicateExpression.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/vt/value.h"

#include "fileIO_Common.h"
#include "predicateExpressionParser.h"

#include <functional>
#include <iterator>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    // SdfPredicateExpression::FnCall::Kind
    TF_ADD_ENUM_NAME(SdfPredicateExpression::FnCall::BareCall);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::FnCall::ColonCall);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::FnCall::ParenCall);

    // SdfPredicateExpression::Op
    TF_ADD_ENUM_NAME(SdfPredicateExpression::Call);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::Not);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::ImpliedAnd);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::And);
    TF_ADD_ENUM_NAME(SdfPredicateExpression::Or);
}

SdfPredicateExpression
SdfPredicateExpression::MakeNot(SdfPredicateExpression &&right)
{
    // Move over the ops and calls, then push back 'Not'.
    SdfPredicateExpression ret;
    ret._ops = std::move(right._ops);
    ret._calls = std::move(right._calls);
    ret._ops.push_back(Not);
    return ret;
}

SdfPredicateExpression
SdfPredicateExpression::MakeOp(
    Op op, SdfPredicateExpression &&left, SdfPredicateExpression &&right)
{
    SdfPredicateExpression ret;
    // Move the right ops, ensure we have enough space, then insert left.
    // Finally push back this new op.
    ret._ops = std::move(right._ops);
    ret._ops.reserve(ret._ops.size() + left._ops.size() + 1);
    ret._ops.insert(
        ret._ops.end(), left._ops.begin(), left._ops.end());
    ret._ops.push_back(op);

    // Move the left calls, then move-insert the right calls.
    ret._calls = std::move(left._calls);
    ret._calls.insert(ret._calls.end(),
                      make_move_iterator(right._calls.begin()),
                      make_move_iterator(right._calls.end()));
    return ret;
}

SdfPredicateExpression
SdfPredicateExpression::MakeCall(FnCall &&call)
{
    /// Just push back a 'Call' op and the call itself.
    SdfPredicateExpression ret;
    ret._ops.push_back(Call);
    ret._calls.push_back(std::move(call));
    return ret;
}

void
SdfPredicateExpression::WalkWithOpStack(
    TfFunctionRef<void (std::vector<std::pair<Op, int>> const &)> logic,
    TfFunctionRef<void (FnCall const &)> call) const
{
    // Do nothing if this is the empty expression.
    if (IsEmpty()) {
        return;
    }

    // Operations are stored in reverse order.
    using OpIter = std::vector<Op>::const_reverse_iterator;
    OpIter curOp = _ops.rbegin();

    // Calls are stored in forward order.
    using CallIter = std::vector<FnCall>::const_iterator;
    CallIter curCall = _calls.begin();

    // A stack of ops and indexes tracks where we are in the expression.  The
    // indexes delimit the operands while processing an operation:
    //
    // index ----->    0     1      2
    // operation -> And(<lhs>, <rhs>)
    std::vector<std::pair<Op, int>> stack {1, {*curOp, 0}};

    while (!stack.empty()) {
        Op stackOp = stack.back().first;
        int &operandIndex = stack.back().second;
        int operandIndexEnd = 0;

        // Invoke 'call' for Call operations, otherwise 'logic'.
        if (stackOp == Call) {
            call(*curCall++);
        } else {
            logic(stack);
            ++operandIndex;
            operandIndexEnd = stackOp == Not ? 2 : 3; // only 'not' is unary.
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
SdfPredicateExpression::Walk(
    TfFunctionRef<void (Op, int)> logic,
    TfFunctionRef<void (FnCall const &)> call) const
{
    auto adaptLogic = [&logic](std::vector<std::pair<Op, int>> const &stack) {
        return logic(stack.back().first, stack.back().second);
    };
    return WalkWithOpStack(adaptLogic, call);
}

std::string
SdfPredicateExpression::GetText() const
{
    std::string result;
    if (IsEmpty()) {
        return result;
    }

    auto opName = [](Op k) {
        switch (k) {
        case Not: return "not";
        case ImpliedAnd: return " ";
        case And: return "and";
        case Or: return "or";
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
        if (op == Not ? argIndex == 0 : argIndex == 1) {
            result += opName(op);
        }
        if (parenthesize && (op == Not ? argIndex == 1 : argIndex == 2)) {
            result += ')';
        }                
    };

    auto printCall = [&result](FnCall const &call) {
        result += call.funcName;
        switch (call.kind) {
        case FnCall::BareCall: break;
        case FnCall::ColonCall: {
            std::vector<std::string> argStrs;
        for (auto const &arg: call.args) {
                argStrs.push_back(
                    Sdf_FileIOUtility::StringFromVtValue(arg.value));
        }
            if (!argStrs.empty()) {
                result += ":" + TfStringJoin(argStrs, ",");
            }
        } break;
        case FnCall::ParenCall: {
            std::vector<std::string> argStrs;
            for (auto const &arg: call.args) {
                argStrs.push_back(
                    TfStringPrintf(
                        "%s%s%s",
                        arg.argName.empty() ? "" : arg.argName.c_str(),
                        arg.argName.empty() ? "" : "=",
                        Sdf_FileIOUtility
                        ::StringFromVtValue(arg.value).c_str()));
            }
            if (!argStrs.empty()) {
                result += "(" + TfStringJoin(argStrs, ", ") + ")";
            }
        } break;
        };
    };
    
    WalkWithOpStack(printLogic, printCall);
    
    return result;
}

SdfPredicateExpression::SdfPredicateExpression(
    std::string const &input, std::string const &context)
{
    using namespace tao::TAO_PEGTL_NAMESPACE;
    
    Analyze<PredExpr>();
    try {
        SdfPredicateExprBuilder builder;
        // Uncomment the 'tracer' bit below for debugging.
        parse<must<seq<PredExpr, eolf>>, PredAction/*, tracer*/>(
            string_input<> {
                input, context.empty() ? "<input>" : context.c_str()
            }, builder);
        *this = builder.Finish();
    }
    catch (parse_error const &err) {
        // Failed to parse -- make an err msg.
        std::string errMsg = err.what();
        errMsg += " -- ";
        bool first = true;
        for (position const &p: err.positions) {
            if (!first) {
                errMsg += ", ";
            }
            first = false;
            errMsg += to_string(p);
        }
        _parseError = std::move(errMsg);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

