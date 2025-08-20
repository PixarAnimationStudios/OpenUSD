//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_BOOLEAN_EXPRESSION_H
#define PXR_USD_SDF_BOOLEAN_EXPRESSION_H

#include "pxr/usd/sdf/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfBooleanExpression
///
/// Objects of this class represent expressions that can be evaluated to produce
/// a boolean value.
/// See \ref Sdf_Page_BooleanExpressions for more details.

class SdfBooleanExpression {
public:
    /// Constructs an empty expression.
    SdfBooleanExpression() = default;

    /// Constructs an expression by parsing a string representation. If an error
    /// occurs while parsing the string, the result will be an empty expression.
    /// See also GetParseError().
    SDF_API explicit SdfBooleanExpression(std::string const& text);

    /// An expression is empty if it was default constructed or if there
    /// was a problem parsing its string representation.
    SDF_API bool IsEmpty() const;

    /// Provides a string representation that can be parsed by
    /// SdfBooleanExpression(std::string const&).
    /// If the expression was constructed from a string, the existing formatting
    /// will be preserved.
    SDF_API std::string GetText() const;

    /// Return parsing errors as a string if this expression was constructed
    /// from a string and parse errors were encountered.
    SDF_API std::string const& GetParseError() const;

    /// Provides the collection of variable names referenced by the expression.
    SDF_API std::set<TfToken> GetVariableNames() const;

    /// Constructs an expression representing a variable.
    ///
    /// \see VariableVisitor
    SDF_API static SdfBooleanExpression
    MakeVariable(TfToken const& variableName);

    /// Constructs an expression wrapping a constant value.
    ///
    /// \see ConstantVisitor
    SDF_API static SdfBooleanExpression
    MakeConstant(VtValue const& value);

    /// Operators for combining two subexpressions.
    /// \see MakeBinaryOp
    enum class BinaryOperator {
        /// The `==` operator.
        EqualTo,

        /// The `!=` operator.
        NotEqualTo,

        /// The `<` operator.
        LessThan,

        /// The `<=` operator.
        LessThanOrEqualTo,

        /// The `>` operator.
        GreaterThan,

        /// The `>=` operator.
        GreaterThanOrEqualTo,

        /// The `&&` operator.
        And,

        /// The `||` operator.
        Or,
    };

    /// Constructs an expression that applies the provided operator to the
    /// result of the two provided subexpressions.
    ///
    /// The expression `width > 10.0` would be constructed with the arguments:
    /// |argument|value                                                  |
    /// |--------|-------------------------------------------------------|
    /// |lhs     |`SdfBooleanExpression::MakeVariable(TfToken("width"));`|
    /// |op      |BinaryOperator::GreaterThan                            |
    /// |rhs     |`SdfBooleanExpression::MakeConstant(VtValue(10.0));`   |
    /// \see BinaryVisitor
    SDF_API static SdfBooleanExpression
    MakeBinaryOp(SdfBooleanExpression lhs,
                 BinaryOperator op,
                 SdfBooleanExpression rhs);

    /// Operators applied to a single subexpression.
    /// \see MakeUnaryOp
    enum class UnaryOperator {
        /// The `!` operator.
        Not,
    };

    /// Constructs an expression that applies the provided operator to the
    /// result of the provided subexpression.
    ///
    /// The expression `!(width > 10.0)` would be constructed with the
    /// arguments:
    /// |argument  |value                                           |
    /// |----------|------------------------------------------------|
    /// |expression|SdfBooleanExpression representing `width > 10.0`|
    /// |op        | UnaryOp::Not                                   |
    /// \see UnaryVisitor
    SDF_API static SdfBooleanExpression
    MakeUnaryOp(SdfBooleanExpression expression, UnaryOperator op);

    /// The Visit() method will invoke this callback if the receiver represents
    /// a binary operator applied to two subexpressions.
    ///
    /// Given the expression `width > 10.0`, the callback would be invoked with
    /// the arguments:
    /// |argument|value                                    |
    /// |--------|-----------------------------------------|
    /// |lhs     |SdfBooleanExpression representing `width`|
    /// |op      |BinaryOperator::GreaterThan              |
    /// |rhs     |SdfBooleanExpression representing `10.0` |
    /// \see MakeBinaryOp()
    using BinaryVisitor = TfFunctionRef<void(SdfBooleanExpression const&,
                                             BinaryOperator,
                                             SdfBooleanExpression const&)>;

    /// The Visit() method will invoke this callback if the receiver represents
    /// an operator applied to a single subexpression
    ///
    /// Given the expression `!(width > 10.0)`, the callback would be invoked
    /// with the arguments:
    /// |argument  |value                                           |
    /// |----------|------------------------------------------------|
    /// |expression|SdfBooleanExpression representing `width > 10.0`|
    /// |op        | UnaryOp::Not                                   |
    /// \see MakeUnaryOp()
    using UnaryVisitor = TfFunctionRef<void(SdfBooleanExpression const&,
                                            UnaryOperator)>;

    /// The Visit() method will invoke this callback if the receiver represents
    /// a variable.
    ///
    /// \see MakeVariable()
    using VariableVisitor = TfFunctionRef<void(TfToken const&)>;

    /// The Visit() method will invoke this callback if the receiver represents
    /// a constant.
    ///
    /// \see MakeConstant()
    using ConstantVisitor = TfFunctionRef<void(VtValue const&)>;

    /// Invokes one of the given callbacks based on the type of the expression.
    SDF_API void Visit(VariableVisitor variable,
                       ConstantVisitor constant,
                       BinaryVisitor binary,
                       UnaryVisitor unary) const;

    /// Provides the current value for a given variable. Used by Evaluate()
    /// when evaluating the expression.
    using VariableCallback = TfFunctionRef<VtValue (TfToken const&)>;

    /// Evaluates the expression. If the expression contains any variables,
    /// \p variableCallback will be invoked to determine their current values.
    SDF_API bool Evaluate(VariableCallback const& variableCallback) const;

    /// Encapsulates a transformation that may be applied to a variable name.
    /// Used by RenameVariables().
    using NameTransform = TfFunctionRef<TfToken(TfToken const&)>;

    /// Applies the provided transform to each variable name and returns the
    /// resulting expression.
    SDF_API SdfBooleanExpression
    RenameVariables(NameTransform const& transform) const;

    /// Determines if the provided string can be parsed as an expression.
    /// Returns `true` if the expression is valid, otherwise returns `false`.
    /// If the string is not a valid expression, \p errorMessage (if non-null)
    /// will be filled with an explanatory error message.
    SDF_API static bool
    Validate(std::string const& expression, std::string* errorMessage = nullptr);

    // Note that the internal _Node class is public to simplify details of the
    // implementation.
    class _Node;
    TF_DECLARE_REF_PTRS(_Node);

private:
    SdfBooleanExpression(_NodeRefPtr const& node);

    SDF_API friend std::ostream&
    operator<<(std::ostream&, SdfBooleanExpression const&);

    std::string _text;
    std::string _parseError;
    _NodeRefPtr _rootNode;
};

SDF_API std::ostream&
operator<<(std::ostream& os, SdfBooleanExpression::BinaryOperator const& rhs);

SDF_API std::ostream&
operator<<(std::ostream& os, SdfBooleanExpression::UnaryOperator const& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_BOOLEAN_EXPRESSION_H
