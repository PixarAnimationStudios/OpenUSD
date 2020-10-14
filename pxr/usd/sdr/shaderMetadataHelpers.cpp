//
// Copyright 2018 Pixar
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

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderProperty.h"

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
    IsTruthy(const TfToken& propName, const NdrTokenMap& metadata)
    {
        // Absence of the option implies false
        if (metadata.count(propName) == 0) {
            return false;
        }

        std::string boolStr = metadata.at(propName);

        // Presence of the option without a value implies true
        if (boolStr.empty()) {
            return true;
        }

        // Turn into a lower case string
        std::transform(boolStr.begin(), boolStr.end(), boolStr.begin(), ::tolower);

        if ((boolStr == "0") || (boolStr == "false") || (boolStr == "f")) {
            return false;
        }

        return true;
    }


    // -------------------------------------------------------------------------


    std::string
    StringVal(const TfToken& propName, const NdrTokenMap& metadata,
              const std::string& defaultValue)
    {
        const NdrTokenMap::const_iterator search = metadata.find(propName);

        if (search != metadata.end()) {
            return search->second;
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    TfToken
    TokenVal(const TfToken& propName, const NdrTokenMap& metadata,
             const TfToken& defaultValue)
    {
        const NdrTokenMap::const_iterator search = metadata.find(propName);

        if (search != metadata.end()) {
            return TfToken(search->second);
        }

        return defaultValue;
    }


    // -------------------------------------------------------------------------


    NdrStringVec
    StringVecVal(const TfToken& propName, const NdrTokenMap& metadata)
    {
        const NdrTokenMap::const_iterator search = metadata.find(propName);

        if (search != metadata.end()) {
            return TfStringSplit(search->second, "|");
        }

        return NdrStringVec();
    }


    // -------------------------------------------------------------------------


    NdrTokenVec
    TokenVecVal(const TfToken& propName, const NdrTokenMap& metadata)
    {
        const NdrStringVec untokenized = StringVecVal(propName, metadata);
        NdrTokenVec tokenized;

        for (const std::string& item : untokenized) {
            tokenized.emplace_back(TfToken(item));
        }

        return tokenized;
    }


    // -------------------------------------------------------------------------


    NdrOptionVec
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

        NdrOptionVec options;

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
    CreateStringFromStringVec(const NdrStringVec& stringVec)
    {
        return TfStringJoin(stringVec, "|");
    }


    // -------------------------------------------------------------------------


    bool
    IsPropertyAnAssetIdentifier(const NdrTokenMap& metadata)
    {
        const NdrTokenMap::const_iterator widgetSearch =
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
    IsPropertyATerminal(const NdrTokenMap& metadata)
    {
        const NdrTokenMap::const_iterator renderTypeSearch =
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
    GetRoleFromMetadata(const NdrTokenMap& metadata)
    {
        const NdrTokenMap::const_iterator roleSearch =
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
}

PXR_NAMESPACE_CLOSE_SCOPE
