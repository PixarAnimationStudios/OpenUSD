//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/sdf/textParserUtils.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Values for "widget" metadata that indicate the property is an
    // asset identifier
    ((filename, "filename"))            // OSL spec
    ((fileInput, "fileInput"))          // Args spec
    ((assetIdInput, "assetIdInput"))    // Pixar convention

    // Values for "renderType" metadata that indicate the property is a
    // SdrPropertyTypes->Terminal
    ((terminal, "terminal"))
);

namespace ShaderMetadataHelpers
{
    bool
    IsTruthy(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        // Absence of the option implies false
        if (search == metadata.end()) {
            return false;
        }

        // Presence of the option without a value implies true
        if (search->second.empty()) {
            return true;
        }

        // Copy string for modification below
        std::string boolStr = search->second;

        // Turn into a lower case string
        std::transform(boolStr.begin(), boolStr.end(), boolStr.begin(), ::tolower);

        if ((boolStr == "0") || (boolStr == "false") || (boolStr == "f")) {
            return false;
        }

        return true;
    }


    // -------------------------------------------------------------------------


    std::string
    StringVal(const TfToken& key, const SdrTokenMap& metadata,
              const std::string& defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return search->second;
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    TfToken
    TokenVal(const TfToken& key, const SdrTokenMap& metadata,
             const TfToken& defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return TfToken(search->second);
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    int
    IntVal(const TfToken& key, const SdrTokenMap& metadata,
           int defaultValue)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search == metadata.end()) {
            return defaultValue;
        }

        try {
            return std::stoi(search->second);
        } catch (...) {
            return defaultValue;
        }
    }


    // -------------------------------------------------------------------------


    SdrStringVec
    StringVecVal(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator search = metadata.find(key);

        if (search != metadata.end()) {
            return TfStringSplit(search->second, "|");
        }

        return SdrStringVec();
    }


    // -------------------------------------------------------------------------


    SdrTokenVec
    TokenVecVal(const TfToken& key, const SdrTokenMap& metadata)
    {
        const SdrStringVec untokenized = StringVecVal(key, metadata);
        SdrTokenVec tokenized;

        for (const std::string& item : untokenized) {
            tokenized.emplace_back(TfToken(item));
        }

        return tokenized;
    }


    // -------------------------------------------------------------------------


    SdrOptionVec
    OptionVecVal(const std::string& optionStr)
    {
        std::vector<std::string> tokens = TfStringSplit(optionStr, "|");

        // The input string should be formatted as one of the following:
        //
        //     list:   "option1|option2|option3|..."
        //     mapper: "key1:value1|key2:value2|..."
        //
        // If it's a mapper, return the result as a list of key-value tuples to
        // preserve order.

        SdrOptionVec options;

        for (const std::string& token : tokens) {
            size_t colonPos = token.find(':');

            if (colonPos != std::string::npos) {
                options.emplace_back(std::make_pair(
                    TfToken(token.substr(0, colonPos)),
                    TfToken(token.substr(colonPos + 1)))
                );
            } else {
                options.emplace_back(std::make_pair(
                    TfToken(token),
                    TfToken())
                );
            }
        }

        return options;
    }


    // -------------------------------------------------------------------------


    std::string
    CreateStringFromStringVec(const SdrStringVec& stringVec)
    {
        return TfStringJoin(stringVec, "|");
    }


    // -------------------------------------------------------------------------


    bool
    IsPropertyAnAssetIdentifier(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator widgetSearch =
            metadata.find(SdrPropertyMetadata->Widget);

        if (widgetSearch != metadata.end()) {
            const TfToken widget = TfToken(widgetSearch->second);

            if ((widget == _tokens->assetIdInput) ||
                (widget == _tokens->filename) ||
                (widget == _tokens->fileInput)) {
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------

    bool
    IsPropertyATerminal(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator renderTypeSearch =
            metadata.find(SdrPropertyMetadata->RenderType);

        if (renderTypeSearch != metadata.end()) {
            // If the property is a SdrPropertyTypes->Terminal, then the
            // renderType value will be "terminal <terminalName>", where the
            // <terminalName> is the specific kind of terminal.  To identify
            // the property as a terminal, we only need to check that the first
            // string in the renderType value specifies "terminal"
            if (TfStringStartsWith(
                renderTypeSearch->second, _tokens->terminal)) {
                return true;
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------

    TfToken
    GetRoleFromMetadata(const SdrTokenMap& metadata)
    {
        const SdrTokenMap::const_iterator roleSearch =
            metadata.find(SdrPropertyMetadata->Role);

        if (roleSearch != metadata.end()) {
            // If the value found is an allowed value, then we can return it
            const TfToken role = TfToken(roleSearch->second);
            if (std::find(SdrPropertyRole->allTokens.begin(),
                          SdrPropertyRole->allTokens.end(),
                          role) != SdrPropertyRole->allTokens.end()) {
                return role;
            }
        }
        // Return an empty token if no "role" metadata or acceptable value found
        return TfToken();
    }

    // -------------------------------------------------------------------------

    VtValue
    ParseSdfValue(const std::string& valueStr,
                  const SdrShaderPropertyConstPtr& property,
                  std::string* err)
    {
        const SdrSdfTypeIndicator indicator = property->GetTypeAsSdfType();
        const TfToken sdrType = property->GetType();
        const SdfValueTypeName sdfType = indicator.GetSdfType();

        std::string normalizedStr = valueStr;
        if (property->IsDynamicArray() ||
                sdrType == SdrPropertyTypes->Vector) {
            normalizedStr = TfStringTrim(normalizedStr);
            normalizedStr = '[' + normalizedStr + ']';
        } else if (property->IsArray() ||
                   sdrType == SdrPropertyTypes->Color  ||
                   sdrType == SdrPropertyTypes->Color4 ||
                   sdrType == SdrPropertyTypes->Point  ||
                   sdrType == SdrPropertyTypes->Normal) {
            normalizedStr = TfStringTrim(normalizedStr);
            normalizedStr = '(' + normalizedStr + ')';
        } else if (sdfType == SdfValueTypeNames->String ||
                   sdfType == SdfValueTypeNames->Token) {
            normalizedStr = Sdf_QuoteString(normalizedStr);
        } else if (sdfType == SdfValueTypeNames->Asset) {
            if (normalizedStr.empty()) {
                *err = "Attempted parse of invalid empty asset path";
                return VtValue();
            }
            normalizedStr = Sdf_QuoteAssetPath(normalizedStr);
        } else {
            normalizedStr = TfStringTrim(normalizedStr);
        }

        // We transform any TfErrors into messages that are concatenated to the
        // provided err output, so that Python users don't need to deal with
        // a thrown exception.
        TfErrorMark m;
        VtValue outputValue = Sdf_ParseValueFromString(normalizedStr, sdfType);
        if (!m.IsClean()) {
            for (TfError const &tfErr: m) {
                if (err->empty()) {
                    *err = tfErr.GetCommentary();
                } else {
                    *err = TfStringPrintf("%s; %s", err->c_str(),
                                          tfErr.GetCommentary().c_str());
                }
            }
            m.Clear();
        }
        return outputValue;
    }

    // Mapping from Katana conditional visibility operator names to the
    // equivalent operators from SdfBooleanExpression::BinaryOperator.
    using BinaryOperator = SdfBooleanExpression::BinaryOperator;
    TF_MAKE_STATIC_DATA((std::map<std::string, BinaryOperator>), operators) {
        (*operators)["and"] = BinaryOperator::And;
        (*operators)["or"] = BinaryOperator::Or;
        (*operators)["equalTo"] = BinaryOperator::EqualTo;
        (*operators)["notEqualTo"] = BinaryOperator::NotEqualTo;
        (*operators)["greaterThan"] = BinaryOperator::GreaterThan;
        (*operators)["lessThan"] = BinaryOperator::LessThan;
        (*operators)["greaterThanOrEqualTo"] = BinaryOperator::GreaterThanOrEqualTo;
        (*operators)["lessThanOrEqualTo"] = BinaryOperator::LessThanOrEqualTo;
    }

    // Finds a property from a Katana-style path, relative to fromProperty.
    // Given:
    //   path = '../../Advanced/traceLightPaths'
    //   fromProperty->GetPage() = 'Shadows'
    //   fromProperty->GetImplementationName() = 'enableShadows'
    // A full path will be constructed and normalized:
    //   full path = 'Shadows/enableShadows/../../Advanced/traceLightPaths'
    //   normalized = 'Advanced/traceLightPaths'
    // The normalized path is split into a page and implementation name:
    //   page = 'Advanced'
    //   implementationName = 'traceLightPaths'
    // If a property is found in allProperties that matches both the page and
    // implementation name, it will be returned. If a full match is not found
    // but a property matches just the implementation name, it will be returned.
    // If no such property is found, returns nullptr.
    SdrShaderPropertyConstPtr
    _FindSiblingProperty(const std::string& path,
        SdrShaderPropertyConstPtr fromProperty,
        const SdrShaderPropertyUniquePtrVec& allProperties)
    {
        // Construct a full path starting from the given property.
        std::string fullPath;
        if (!fromProperty->GetPage().IsEmpty()) {
            // Convert the property's page from a namedspaced identifier into
            // a path.
            const std::vector<std::string> pageParts =
                SdfPath::TokenizeIdentifier(fromProperty->GetPage());
            fullPath = TfStringJoin(pageParts, "/");
            fullPath += '/';
        }

        // Append the property's implementation name to the path and concatenate
        // the provided path.
        fullPath += fromProperty->GetImplementationName();
        fullPath += '/';
        fullPath += path;

        // Normalize the path and split it.
        const std::string normPath = TfNormPath(fullPath);
        std::vector<std::string> parts = TfStringSplit(normPath, "/");
        if (parts.empty()) {
            return nullptr;
        }

        // Split the resolved path into the property name and page
        const std::string name = parts.back();
        parts.pop_back();
        const std::string page = SdfPath::JoinIdentifier(parts);

        // Look for a property that matches both the page and name
        for (const SdrShaderPropertyUniquePtr& property : allProperties) {
            if (property->GetPage() == page &&
                property->GetImplementationName() == name) {
                return property.get();
            }
        }

        // Fall back to looking for a property that matches just the name
        for (const SdrShaderPropertyUniquePtr& property : allProperties) {
            if (property->GetImplementationName() == name) {
                return property.get();
            }
        }

        return nullptr;
    }

    // Extracts an expression from the metadata at the given prefix, recursing
    // as necessary.
    SdfBooleanExpression
    _ExtractExpression(const SdrTokenMap& metadata,
        const std::string& prefix,
        SdrShaderPropertyConstPtr property,
        const SdrShaderPropertyUniquePtrVec& allProperties,
        SdrShaderNodeConstPtr shader)
    {
        TRACE_FUNCTION();

        // Try to find an operator for the given prefix
        const SdrTokenMap::const_iterator it =
            metadata.find(TfToken(prefix + "Op"));
        if (it == metadata.end()) {
            return {};
        }

        // Check if it's a boolean operator
        const std::string opString = it->second;
        const auto opIt = operators->find(opString);
        if (opIt == operators->end()) {
            // Only boolean and comparison operators are supported
            TF_WARN("Unknown conditional visibility op: '%s'",
                opString.c_str());

            return {};
        }

        BinaryOperator op = opIt->second;
        if (op == BinaryOperator::And || op == BinaryOperator::Or) {
            // Boolean operator, which takes two subexpressions.
            // Get the prefixes for the left and right branches.
            const SdrTokenMap::const_iterator lhsIt =
                metadata.find(TfToken(prefix + "Left"));
            const SdrTokenMap::const_iterator rhsIt =
                metadata.find(TfToken(prefix + "Right"));
            if (lhsIt == metadata.end() || rhsIt == metadata.end()) {
                return {};
            }

            // Recurse on the left and right halves
            const SdfBooleanExpression lhs = _ExtractExpression(metadata,
                lhsIt->second, property, allProperties, shader);
            const SdfBooleanExpression rhs = _ExtractExpression(metadata,
                rhsIt->second, property, allProperties, shader);
            if (lhs.IsEmpty() || rhs.IsEmpty()) {
                return {};
            }

            // Combine the two subexpressions with the boolean operator
            return SdfBooleanExpression::MakeBinaryOp(lhs, op, rhs);
        }

        // Not a boolean operator, so it's a comparison operator.
        // Get the attribute path and value for the comparison
        const SdrTokenMap::const_iterator path =
            metadata.find(TfToken(prefix + "Path"));
        const SdrTokenMap::const_iterator value =
            metadata.find(TfToken(prefix + "Value"));
        if (path == metadata.end() || value == metadata.end()) {
            return {};
        }

        // Try to find the property referenced by the path.
        SdrShaderPropertyConstPtr otherProperty = _FindSiblingProperty(
            path->second, property, allProperties);
        if (!otherProperty) {
            TF_WARN("Unable to find referenced property '%s' "
                "(from property '%s' in %s)",
                path->second.c_str(), property->GetName().GetText(),
                shader->GetResolvedDefinitionURI().c_str());
            return {};
        }

        // Use the property's type to decode the value.
        const SdfValueTypeName& type =
            otherProperty->GetTypeAsSdfType().GetSdfType();
        VtValue parsedValue;
        if (type == SdfValueTypeNames->String) {
            parsedValue = value->second;
        } else {
            TfErrorMark mark;
            mark.SetMark();

            std::string error;
            parsedValue = ParseSdfValue(value->second, otherProperty, &error);
            mark.Clear();

            if (parsedValue.IsEmpty()) {
                TF_WARN("Unable to parse '%s' as %s: %s "
                    "(from property '%s' in %s)", value->second.c_str(),
                    type.GetAsToken().GetText(), error.c_str(),
                    property->GetName().GetText(),
                    shader->GetResolvedDefinitionURI().c_str());
                return {};
            }
        }

        // Construct the comparison operation.
        return SdfBooleanExpression::MakeBinaryOp(
            SdfBooleanExpression::MakeVariable(otherProperty->GetName()),
            op,
            SdfBooleanExpression::MakeConstant(parsedValue));
    }

    std::string
    ComputeShownIfFromMetadata(SdrShaderPropertyConstPtr property,
        const SdrShaderPropertyUniquePtrVec& allProperties,
        SdrShaderNodeConstPtr shader)
    {
        const std::string initialPrefix{"conditionalVis"};
        const SdfBooleanExpression expr = _ExtractExpression(
            property->GetMetadata(), initialPrefix, property, allProperties,
            shader);
        return expr.GetText();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
