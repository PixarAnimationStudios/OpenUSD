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

