//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_VARIABLE_EXPRESSION
#define PXR_USD_SDF_VARIABLE_EXPRESSION

/// \file sdf/variableExpression.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl {
    class Node;
}

/// \class SdfVariableExpression
///
/// Class responsible for parsing and evaluating variable expressions.
///
/// Variable expressions are written in a custom language and 
/// represented in scene description as a string surrounded by backticks.
/// These expressions may refer to "expression variables", which are key-value
/// pairs provided by clients. For example, when evaluating an expression like:
///
/// \code
/// `"a_${NAME}_string"`
/// \endcode
///
/// The "${NAME}" portion of the string with the value of expression variable
/// "NAME".
///
/// Expression variables may be any of these supported types:
/// 
/// - std::string
/// - int64_t (int is accepted but coerced to int64_t)
/// - bool
/// - VtArrays containing any of the above types.
/// - None (represented by an empty VtValue)
///
/// Expression variables are typically authored in scene description as layer
/// metadata under the 'expressionVariables' field. Higher levels of the system
/// (e.g., composition) are responsible for examining fields that support
/// variable expressions, evaluating them with the appropriate variables (via
/// this class) and consuming the results.
///
/// See \ref Sdf_Page_VariableExpressions "Variable Expressions"
/// or more information on the expression language and areas of the system
/// where expressions may be used.
class SdfVariableExpression
{
public:
    /// Construct using the expression \p expr. If the expression cannot be
    /// parsed, this object represents an invalid expression. Parsing errors
    /// will be accessible via GetErrors.
    SDF_API
    explicit SdfVariableExpression(const std::string& expr);

    SDF_API
    explicit SdfVariableExpression(std::string&& expr);

    /// Construct an object representing an invalid expression.
    SDF_API
    SdfVariableExpression();

    SDF_API
    ~SdfVariableExpression();

    /// Returns true if \p s is a variable expression, false otherwise.
    /// A variable expression is a string surrounded by backticks.
    ///
    /// A return value of true does not guarantee that \p s is a valid
    /// expression. This function is meant to be used as an initial check
    /// to determine if a string should be considered as an expression.
    SDF_API
    static bool IsExpression(const std::string& s);

    /// Returns true if \p value holds a type that is supported by
    /// variable expressions, false otherwise. If this function returns
    /// true, \p value may be used for an expression variable supplied to
    /// the Evaluate function. \p value may also be authored into the
    /// 'expressionVariables' dictionary, unless it is an empty VtValue
    /// representing the None value. See class documentation for list of
    /// supported types.
    SDF_API
    static bool IsValidVariableType(const VtValue& value);

    /// Returns true if this object represents a valid expression, false
    /// if it represents an invalid expression.
    ///
    /// A return value of true does not mean that evaluation of this
    /// expression is guaranteed to succeed. For example, an expression may
    /// refer to a variable whose value is an invalid expression.
    /// Errors like this can only be discovered by calling Evaluate.
    SDF_API
    explicit operator bool() const;

    /// Returns the expression string used to construct this object.
    SDF_API
    const std::string& GetString() const;

    /// Returns a list of errors encountered when parsing this expression.
    ///
    /// If the expression was parsed successfully, this list will be empty.
    /// However, additional errors may be encountered when evaluating the e
    /// expression.
    SDF_API
    const std::vector<std::string>& GetErrors() const;

    /// \name Evaluation
    /// @{

    /// \class EmptyList
    /// A result value representing an empty list.
    class EmptyList { };

    /// \class Result
    class Result
    {
    public:
        /// The result of evaluating the expression. This value may be
        /// empty if the expression yielded no value. It may also be empty
        /// if errors occurred during evaluation. In this case, the errors
        /// field will be populated with error messages.
        ///
        /// If the value is not empty, it will contain one of the supported
        /// types listed in the class documentation.
        VtValue value;

        /// Errors encountered while evaluating the expression.
        std::vector<std::string> errors;

        /// Set of variables that were used while evaluating
        /// the expression. For example, for an expression like
        /// `"example_${VAR}_expression"`, this set will contain "VAR".
        ///
        /// This set will also contain variables from subexpressions.
        /// In the above example, if the value of "VAR" was another
        /// expression like `"sub_${SUBVAR}_expression"`, this set will
        /// contain both "VAR" and "SUBVAR".
        std::unordered_set<std::string> usedVariables;
    };

    /// Evaluates this expression using the variables in
    /// \p variables and returns a Result object with the final
    /// value. If an error occurs during evaluation, the value field
    /// in the Result object will be an empty VtValue and error messages
    /// will be added to the errors field.
    ///
    /// If the expression evaluates to an empty list, the value field
    /// in the Result object will contain an EmptyList object instead
    /// of an empty VtArray<T>, as the expression language does not
    /// provide syntax for specifying the expected element types in
    /// an empty list.
    ///
    /// If this object represents an invalid expression, calling this
    /// function will return a Result object with an empty value and the
    /// errors from GetErrors().
    ///
    /// If any values in \p variables used by this expression
    /// are themselves expressions, they will be parsed and evaluated.
    /// If an error occurs while evaluating any of these subexpressions,
    /// evaluation of this expression fails and the encountered errors
    /// will be added in the Result's list of errors.
    SDF_API
    Result Evaluate(const VtDictionary& variables) const;

    /// Evaluates this expression using the variables in
    /// \p variables and returns a Result object with the final
    /// value.
    ///
    /// This is a convenience function that calls Evaluate and ensures that
    /// the value in the Result object is either an empty VtValue or is
    /// holding the specified ResultType. If this is not the case, the
    /// Result value will be set to an empty VtValue an error message
    /// indicating the unexpected type will be added to the Result's error
    /// list. Otherwise, the Result will be returned as-is.
    ///
    /// If the expression evaluates to an empty list and the ResultType
    /// is a VtArray<T>, the value in the Result object will be an empty
    /// VtArray<T>. This differs from Evaluate, which would return an
    /// untyped EmptyList object instead.
    ///
    /// ResultType must be one of the supported types listed in the
    /// class documentation.
    template <class ResultType>
    Result EvaluateTyped(const VtDictionary& variables) const
    {
        Result r = Evaluate(variables);

        if (VtIsArray<ResultType>::value && r.value.IsHolding<EmptyList>()) {
            r.value = VtValue(ResultType());
        }
        else if (!r.value.IsEmpty() && !r.value.IsHolding<ResultType>()) {
            r.errors.push_back(
                _FormatUnexpectedTypeError(r.value, VtValue(ResultType())));
            r.value = VtValue();
        }
        return r;
    }

    /// @}

    /// \name Building Expressions
    /// Utilities for programatically building a variable expression.
    /// These functions can be chained together to create complex
    /// expressions. For example:
    /// 
    /// \code
    /// const SdfVariableExpression containsExpr =
    ///    SdfVariableExpression::MakeFunction(
    ///        "contains",
    ///        SdfVariableExpression::MakeList(
    ///            SdfVariableExpression::MakeLiteral("foo"),
    ///            SdfVariableExpression::MakeLiteral("bar")),
    ///        SdfVariableExpression::MakeVariable("VAR"));
    /// \endcode
    ///
    /// This yields the expression `contains(["foo", "bar"], ${VAR})`.
    ///
    /// Note that these functions may yield invalid expressions that
    /// cannot be evaluated. For example, calling MakeFunction with
    /// an unrecognized function name will produce an SdfVariableExpression
    /// whose bool operator returns false. However, calling GetString on
    /// the returned SdfVariableExpression will still return the
    /// expression string for inspection.
    ///
    /// @{

    /// \class Builder
    /// Helper class for storing intermediate results when building
    /// a variable expression.
    class Builder
    {
    public:
        SDF_API operator SdfVariableExpression() const;

    private:
        friend class SdfVariableExpression;
        Builder(std::string&& expr) : _expr(std::move(expr)) { }
        std::string _expr;
    };

    /// \class FunctionBuilder
    /// Helper class for storing intermediate results when building
    /// a function variable expression.
    class FunctionBuilder
    {
    public:
        /// Add an expression as an argument to the function call.
        /// \see MakeFunction
        template <class Argument>
        FunctionBuilder& AddArgument(Argument&& arg);

        SDF_API operator SdfVariableExpression() const;
        SDF_API operator Builder() const &;
        SDF_API operator Builder() &&;

    private:
        friend class SdfVariableExpression;
        FunctionBuilder(const std::string& name) : _expr(name + '(') { }
        std::string _expr;
    };

    /// \class ListBuilder
    /// Helper class for storing intermediate results when building
    /// a list variable expression.
    class ListBuilder
    {
    public:
        /// Add an expression as an element to the list.
        /// \see MakeList
        template <class Element>
        ListBuilder& AddElement(Element&& elem);

        /// Add values in \p values as literal expressions to the list.
        /// \see MakeList
        template <class T>
        ListBuilder& AddLiteralValues(const std::vector<T>& values);

        SDF_API operator SdfVariableExpression() const;
        SDF_API operator Builder() const &;
        SDF_API operator Builder() &&;

    private:
        friend class SdfVariableExpression;
        ListBuilder() : _expr("[") { }

        std::string _expr;
    };

    /// Create a function expression that calls the function named \p fnName
    /// with \p fnArgs as arguments, i.e. `fnName(fnArgs1, fnArgs2, ...)`.
    ///
    /// \p fnArgs must be other SdfVariableExpression objects or the result
    /// of other expression builder functions.
    template <class... Arguments>
    static FunctionBuilder
    MakeFunction(const std::string& fnName, Arguments&&... fnArgs)
    {
        FunctionBuilder b(fnName);
        (b.AddArgument(std::forward<Arguments>(fnArgs)), ...);
        return b;
    }

    /// Create a list expression with \p listElems as elements, i.e.
    /// `[listElems1, listElems2, ...]`.
    ///
    /// \p elems must be other SdfVariableExpression objects or the result
    /// of other expression builder functions.
    template <class... Elements>
    static ListBuilder
    MakeList(Elements&&... elems)
    {
        ListBuilder b;
        (b.AddElement(std::forward<Elements>(elems)), ...);
        return b;
    }

    /// Create a list expression with the values in \p values as literal
    /// elements.
    ///
    /// \p values must hold types that can be represented by literal
    /// expressions, i.e. a type for which MakeLiteral is defined.
    template <class T>
    static ListBuilder
    MakeListOfLiterals(const std::vector<T>& values)
    {
        return ListBuilder().AddLiteralValues(values);
    }

    /// Create a literal expression for \p value.
    SDF_API static Builder MakeLiteral(int64_t value);
    SDF_API static Builder MakeLiteral(bool value);
    SDF_API static Builder MakeLiteral(const std::string& value);
    SDF_API static Builder MakeLiteral(const char* value);

    /// Create a "None" literal expression.
    SDF_API static Builder MakeNone();

    /// Create a variable reference expression for the variable named
    /// \p name, i.e. `${name}`.
    SDF_API static Builder MakeVariable(const std::string& name);

    /// Create a variable reference expression for the variable named
    /// \p name and \p fallbackValue, i.e. `${name:val}`.
    SDF_API static Builder MakeVariable(const std::string& name,
                                        const Builder& fallbackValue);

    /// Create a variable reference expression whose fallback is the
    /// expression \p fallbackValue. This is useful when the fallback is an
    /// already-built expression, e.g. the result of MakeList.
    SDF_API static Builder MakeVariable(const std::string& name,
                                        const SdfVariableExpression& fallbackValue);
    /// @}

private:
    SDF_API
    static std::string
    _FormatUnexpectedTypeError(const VtValue& got, const VtValue& expected);

    SDF_API
    static void
    _AppendExpression(
        std::string* expr, const SdfVariableExpression& arg, bool first);
    
    SDF_API
    static void
    _AppendBuilder(std::string* expr, const Builder& b, bool first);

    template <class Argument>
    static void
    _Append(std::string* expr, Argument&& arg, bool first)
    {
        // Avoid implicitly converting arg to an SdfVariableExpression
        // since that would incur unnecessary parsing costs.
        if constexpr (std::is_same_v<
            std::decay_t<Argument>, SdfVariableExpression>) {
            _AppendExpression(expr, std::forward<Argument>(arg), first);
        }
        else {
            _AppendBuilder(expr, std::forward<Argument>(arg), first);
        }
    }

    std::vector<std::string> _errors;
    std::shared_ptr<Sdf_VariableExpressionImpl::Node> _expression;
    std::string _expressionStr;
};

inline bool
operator==(
    const SdfVariableExpression::EmptyList&,
    const SdfVariableExpression::EmptyList&)
{
    return true;
}

inline bool
operator!=(
    const SdfVariableExpression::EmptyList&,
    const SdfVariableExpression::EmptyList&)
{
    return false;
}

template <class Argument>
inline SdfVariableExpression::FunctionBuilder&
SdfVariableExpression::FunctionBuilder::AddArgument(Argument&& arg)
{
    SdfVariableExpression::_Append(
        &_expr, std::forward<Argument>(arg), 
        /* first = */ *_expr.rbegin() == '(');
    return *this;
}

template <class Element>
inline SdfVariableExpression::ListBuilder&
SdfVariableExpression::ListBuilder::AddElement(Element&& arg)
{
    SdfVariableExpression::_Append(
        &_expr, std::forward<Element>(arg), 
        /* first = */ *_expr.rbegin() == '[');
    return *this;
}

template <class T>
inline SdfVariableExpression::ListBuilder&
SdfVariableExpression::ListBuilder::AddLiteralValues(
    const std::vector<T>& values)
{            
    for (const T& v : values) {
        AddElement(SdfVariableExpression::MakeLiteral(v));
    }
    return *this;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
