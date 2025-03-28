//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PREDICATE_EXPRESSION_H
#define PXR_USD_SDF_PREDICATE_EXPRESSION_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/vt/value.h"

#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPredicateExpression
///
/// Represents a logical expression syntax tree consisting of predicate function
/// calls joined by the logical operators 'and', 'or', 'not', and an implied-and
/// operator that represents two subexpressions joined by only whitespace.
///
/// An SdfPredicateExpression can be constructed with a string, which will parse
/// an expression.  The syntax for an expression is as follows:
///
/// The fundamental building blocks are calls to predicate functions.  There are
/// three syntaxes for function calls.
///
/// \li Bare call: just a function name: `isDefined`
/// \li Colon call: name, colon, positional arguments: `isa:mammal,bird`
/// \li Paren call: name and parenthesized positional and keyword args:
/// `isClose(1.23, tolerance=0.01)`
///
/// Colon call arguments are all positional and must be separated by commas with
/// no spaces between arguments. In paren calls, positional arguments must
/// precede keyword arguments, and whitespace is allowed between arguments.
///
/// The string parser supports argument values of the following types:
/// double-quoted "strings", unquoted strings, integers, floating-point numbers,
/// and boolean values 'true' and 'false'.
///
/// The unary operator 'not' may appear preceding a function call, or a
/// subexpresion enclosed in parentheses. The binary operators 'and' and 'or'
/// may appear between subexpressions. If subexpressions appear adjacent to each
/// other (other than possible whitespace), this is considered an implied 'and'
/// operator.
///
/// Operator precedence in order from highest to lowest is: 'not',
/// <implied-and>, 'and', 'or'.
///
/// Here are some examples of valid predicate expression syntax:
///
/// \li `foo` (call "foo" with no arguments)
/// \li `foo bar` (implicit 'and' of "foo" and "bar")
/// \li `foo not bar` (implicit 'and' of "foo" and "not bar")
/// \li `color:red (shiny or matte)`
/// \li `animal or mineral or vegetable`
/// \li `(mammal or bird) and (tame or small)`
/// \li `isClose(100, tolerance=3.0) or negative`
///
class SdfPredicateExpression
{
public:

    /// \class FnArg
    ///
    /// Represents a function argument name and value.  Positional arguments
    /// have empty names.
    struct FnArg {
        static FnArg Positional(VtValue const &val) {
            return { std::string(), val };
        }
        static FnArg Keyword(std::string const &name, VtValue const &val) {
            return { name, val };
        }
        std::string argName;
        VtValue value;

        template <class HashState>
        friend void TfHashAppend(HashState &h, FnArg const &arg) {
            h.Append(arg.argName, arg.value);
        }

        friend bool operator==(FnArg const &l, FnArg const &r) {
            return std::tie(l.argName, l.value) == std::tie(r.argName, r.value);
        }
        friend bool operator!=(FnArg const &l, FnArg const &r) {
            return !(l == r);
        }

        friend void swap(FnArg &l, FnArg &r) {
            swap(l.argName, r.argName);
            swap(l.value, r.value);
        }
    };

    /// \class FnCall
    ///
    /// Represents a function call in an expression with calling style, function
    /// name, and arguments.
    struct FnCall {
        enum Kind {
            BareCall,  ///< no-arg call like 'active'
            ColonCall, ///< colon-separated pos args, like 'isa:Imageable'
            ParenCall  ///< paren/comma & pos/kw args like 'foo(23, bar=baz)'
        };
        
        Kind kind;
        std::string funcName;
        std::vector<FnArg> args;

        template <class HashState>
        friend void TfHashAppend(HashState &h, FnCall const &c) {
            h.Append(c.kind, c.funcName, c.args);
        }

        friend bool operator==(FnCall const &l, FnCall const &r) {
            return std::tie(l.kind, l.funcName, l.args) ==
                std::tie(r.kind, r.funcName, r.args);
        }
        friend bool operator!=(FnCall const &l, FnCall const &r) {
            return !(l == r);
        }
        friend void swap(FnCall &l, FnCall &r) {
            auto lt = std::tie(l.kind, l.funcName, l.args);
            auto rt = std::tie(r.kind, r.funcName, r.args);
            swap(lt, rt);
        }        
    };

    /// Construct the empty expression whose bool-operator returns false.
    SdfPredicateExpression() = default;

    /// Copy construct from another expression.
    SdfPredicateExpression(SdfPredicateExpression const &) = default;

    /// Move construct from another expression.
    SdfPredicateExpression(SdfPredicateExpression &&) = default;

    /// Construct an expression by parsing \p expr.  If provided, \p context
    /// appears in a parse error, if one is generated.  See GetParseError().
    /// See the class documentation for details on expression syntax.
    SDF_API
    explicit SdfPredicateExpression(std::string const &expr,
                                    std::string const &context = {});

    /// Copy assign from another expression.
    SdfPredicateExpression &
    operator=(SdfPredicateExpression const &) = default;

    /// Move assign from another expression.
    SdfPredicateExpression &
    operator=(SdfPredicateExpression &&) = default;

    /// Enumerant describing a subexpression operation.
    enum Op { Call, Not, ImpliedAnd, And, Or };

    /// Produce a new expression by prepending the 'not' operator onto \p right.
    SDF_API
    static SdfPredicateExpression
    MakeNot(SdfPredicateExpression &&right);

    /// Produce a new expression by combining \p left and \p right with the
    /// operator \p op.  The \p op must be one of ImpliedAnd, And, or Or.
    SDF_API
    static SdfPredicateExpression
    MakeOp(Op op,
           SdfPredicateExpression &&left,
           SdfPredicateExpression &&right);

    /// Produce a new expression containing just a the function call \p call.
    SDF_API
    static SdfPredicateExpression
    MakeCall(FnCall &&call);

    /// Walk this expression's syntax tree in depth-first order, calling \p call
    /// with the current function call when a function call is encountered, and
    /// calling \p logic multiple times for each logical operation encountered.
    /// When calling \p logic, the logical operation is passed as the \p Op
    /// parameter, and an integer indicating "where" we are in the set of
    /// operands is passed as the int parameter. For a 'not', call \p
    /// logic(Op=Not, int=0) to start, then after the subexpression that the
    /// 'not' applies to is walked, call \p logic(Op=Not, int=1).  For the
    /// binary operators like 'and' and 'or', call \p logic(Op, 0) before the
    /// first argument, then \p logic(Op, 1) after the first subexpression, then
    /// \p logic(Op, 2) after the second subexpression.  For a concrete example,
    /// consider the following expression:
    ///
    ///     (foo or bar) and not baz
    ///
    /// The sequence of calls from Walk() will be:
    ///
    ///     logic(And, 0)
    ///     logic(Or, 0)
    ///     call("foo")
    ///     logic(Or, 1)
    ///     call("bar")
    ///     logic(Or, 2)
    ///     logic(And, 1)
    ///     logic(Not, 0)
    ///     call("baz")
    ///     logic(Not, 1)
    ///     logic(And, 2)
    /// 
    SDF_API
    void Walk(TfFunctionRef<void (Op, int)> logic,
              TfFunctionRef<void (FnCall const &)> call) const;

    /// Equivalent to Walk(), except that the \p logic function is called with a
    /// const reference to the current Op stack instead of just the top of it.
    /// The top of the Op stack is the vector's back.  This is useful in case
    /// the processing code needs to understand the context in which an Op
    /// appears.
    SDF_API
    void WalkWithOpStack(
        TfFunctionRef<void (std::vector<std::pair<Op, int>> const &)> logic,
        TfFunctionRef<void (FnCall const &)> call) const;

    /// Return a text representation of this expression that parses to the same
    /// expression.
    SDF_API
    std::string GetText() const;

    /// Return true if this is the empty expression; i.e. default-constructed or
    /// constructed from a string with invalid syntax.
    bool IsEmpty() const {
        return _ops.empty();
    }
    
    /// Return true if this expression contains any operations, false otherwise.
    explicit operator bool() const {
        return !IsEmpty();
    }

    /// Return parsing errors as a string if this function was constructed from
    /// a string and parse errors were encountered.
    std::string const &GetParseError() const & {
        return _parseError;
    }

    /// Return parsing errors as a string if this function was constructed from
    /// a string and parse errors were encountered.
    std::string GetParseError() && {
        return std::move(_parseError);
    }

private:
    template <class HashState>
    friend void TfHashAppend(HashState &h, SdfPredicateExpression const &expr) {
        h.Append(expr._ops, expr._calls, expr._parseError);
    }

    friend bool
    operator==(SdfPredicateExpression const &l,
               SdfPredicateExpression const &r) {
        return std::tie(l._ops, l._calls, l._parseError) ==
               std::tie(r._ops, r._calls, r._parseError);
    }

    friend bool
    operator!=(SdfPredicateExpression const &l,
               SdfPredicateExpression const &r) {
        return !(l == r);
    }

    SDF_API
    friend std::ostream &
    operator<<(std::ostream &, SdfPredicateExpression const &);

    // The expression is represented in function-call style, but *in reverse* to
    // facilitate efficient assembly.  For example, an expression like "a and b"
    // would be represented as { Call(b), Call(a), And } rather than { And,
    // Call(a), Call(b) }.  This way, joining two expressions like "a" 'and' "b"
    // can be done by appending to a vector, avoiding having to shift all the
    // elements down to insert the new operation at the head.  See the
    // implementation of Walk() for guidance.
    std::vector<Op> _ops;

    // On the contrary, the elements in _calls are in forward-order, so the last
    // Call in _ops corresponds to the first element of _calls.
    std::vector<FnCall> _calls;

    // This member holds a parsing error string if this expression was
    // constructed by the parser and errors were encountered during the parsing.
    std::string _parseError;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PREDICATE_EXPRESSION_H
