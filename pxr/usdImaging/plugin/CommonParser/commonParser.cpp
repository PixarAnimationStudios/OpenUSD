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
#include "pxr/usdImaging/usdImaging/markupParser.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usd/usdText/tokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include "globals.h"
#include "markupParser.h"
#include "portableUtils.h"

#include <regex>
#include <codecvt>
#include <locale>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingCommonParser
///
/// The common markup parser plugin.
///
class UsdImagingCommonParser : public UsdImagingMarkupParser
{
public:
    using Base = UsdImagingMarkupParser;

    /// The constructor.
    UsdImagingCommonParser();

    /// The destructor.
    ~UsdImagingCommonParser() override;

private:
    /// Initialize the markup parser plugin using a parser setting.
    bool _Initialize(const ParserSettingMap&) override;

    /// Parse the markup string in the MarkupText.
    bool _ParseText(std::shared_ptr<UsdImagingMarkupText> markupText) override;

    /// If a specified markup language is supported.
    bool _IsSupported(const std::wstring& language) override;

};

// Register the parser.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<UsdImagingCommonParser, TfType::Bases<UsdImagingCommonParser::Base> >();
    t.SetFactory< UsdImagingMarkupParserFactory<UsdImagingCommonParser> >();
}

UsdImagingCommonParser::UsdImagingCommonParser()
{
}

/* virtual */
UsdImagingCommonParser::~UsdImagingCommonParser() = default;

/// Return a lower case version of a wstring
inline std::wstring to_lower(std::wstring str) {
    std::transform(std::begin(str), std::end(str), std::begin(str), [](const std::wstring::value_type& x) {
        return std::tolower(x, std::locale());
        });
    return str;
}

bool 
UsdImagingCommonParser::_Initialize(const ParserSettingMap& parserSettingMap)
{
    // Set the font folder.
    auto supportLanguagesIter = parserSettingMap.find(UsdImagingTextTokens->supportLanguages);
    if (supportLanguagesIter != parserSettingMap.end())
    {
        CommonParserUniverse* pUniverse = BigBang();
        if (!pUniverse)
            return false;

        // Get the count of registered generators.
        int countOfGenerators = pUniverse->RegisteredCount();
        // If there is no generator registered, we can not parse any markup. So return false.
        if (countOfGenerators <= 0)
            return false;
        else
        {
            // Collect the markup languages that are supported by the parser.
            std::vector<std::wstring> supportedLanguages;
            for (int i = 0; i < countOfGenerators; i++)
            {
                CommonParserStRange generatorName = pUniverse->GetGenerator(i)->Name();
                std::wstring strName(generatorName.Start(), generatorName.Length());
                strName = to_lower(strName);
                supportedLanguages.push_back(std::move(strName));
            }

            // Get the languages that require to support.
            // The input will be like "MTEXT;RTF;SVG". Each language is divided by ';'.
            std::string requiredLanguages = supportLanguagesIter->second;
            std::regex reg("[;]");
            std::vector<std::string> strLanguages(std::sregex_token_iterator(requiredLanguages.begin(),
                requiredLanguages.end(), reg, -1), std::sregex_token_iterator());
            for (auto strLlanguage : strLanguages)
            {
                // Check if the required language is in the vector of supported languages.
                // If it is not, return false.
                std::wstring requiredLanguage = s2w(strLlanguage);
                requiredLanguage = to_lower(requiredLanguage);
                if (std::find(supportedLanguages.begin(), supportedLanguages.end(), requiredLanguage)
                    == supportedLanguages.end())
                    return false;
            }
        }
    }

    return true;
}

bool 
UsdImagingCommonParser::_ParseText(
    std::shared_ptr<UsdImagingMarkupText> markupText)
{
    // Initialize a parser and then parse the string.
    CommonParserMarkupParser parser;
    if (!parser.Initialize(markupText))
        return false;
    else
    {
        return parser.ParseText();
    }
}

bool
UsdImagingCommonParser::_IsSupported(const std::wstring& requiredLanguage)
{
    CommonParserUniverse* pUniverse = BigBang();
    if (!pUniverse)
        return false;

    // Get the count of registered generators.
    int countOfGenerators = pUniverse->RegisteredCount();
    // If there is no generator registered, we can not parse any markup. So return false.
    if (countOfGenerators <= 0)
        return false;
    else
    {
        // Collect the markup languages that are supported by the parser.
        std::vector<std::wstring> supportedLanguages;
        for (int i = 0; i < countOfGenerators; i++)
        {
            CommonParserStRange generatorName = pUniverse->GetGenerator(i)->Name();
            std::wstring strName(generatorName.Start(), generatorName.Length());
            strName = to_lower(strName);
            supportedLanguages.push_back(std::move(strName));
        }

        // Check if the language is supported.
        if (std::find(supportedLanguages.begin(), supportedLanguages.end(), 
            to_lower(requiredLanguage)) == supportedLanguages.end())
            return false;
        else
            return true;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

