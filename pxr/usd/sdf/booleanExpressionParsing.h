//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_BOOLEAN_EXPRESSION_PARSING_H
#define PXR_USD_SDF_BOOLEAN_EXPRESSION_PARSING_H

#include "pxr/usd/sdf/booleanExpression.h"

PXR_NAMESPACE_OPEN_SCOPE

// Builds an expression by parsing the provided string. The provided type
// callback will be invoked to aid in type resolution when parsing any constants
// found within comparison operations. If the string can't be parsed and the
// errorMessage parameter is non-null, a message describing the problem will be
// provided.
SdfBooleanExpression
Sdf_ParseBooleanExpression(std::string const& text,
    std::string* errorMessage = nullptr);

// Attempts to parse the provided string to determine if it is a valid
// expression. Returns true if the string is valid, false if it is not. If the
// string can't be parsed and the errorMessage parameter is non-null, a message
// describing the problem will be provided.
bool
Sdf_ValidateBooleanExpression(std::string const& text,
    std::string* errorMessage = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_BOOLEAN_EXPRESSION_PARSING_H
