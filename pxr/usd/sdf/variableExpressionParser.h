//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_VARIABLE_EXPRESSION_PARSER_H
#define PXR_USD_SDF_VARIABLE_EXPRESSION_PARSER_H

#include "pxr/pxr.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_VariableExpressionImpl
{
    class Node;
}

/// \class Sdf_VariableExpressionParserResult
/// Object containing results of parsing an expression.
class Sdf_VariableExpressionParserResult
{
public:
    std::unique_ptr<Sdf_VariableExpressionImpl::Node> expression;
    std::vector<std::string> errors;
};

/// Parse the given expression.
Sdf_VariableExpressionParserResult
Sdf_ParseVariableExpression(const std::string& expr);

/// Returns true if \p s is recognized as a variable expression.
/// This does not check the syntax of the expression.
bool Sdf_IsVariableExpression(const std::string& s);

PXR_NAMESPACE_CLOSE_SCOPE

#endif

