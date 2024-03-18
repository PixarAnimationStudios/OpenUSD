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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MARKUP_PARSER_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MARKUP_PARSER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserMarkupParser
///
/// The markup parser interface.
///
class CommonParserMarkupParser
{
public:
    /// The default constructor.
    CommonParserMarkupParser() = default;

    /// The default destructor.
    virtual ~CommonParserMarkupParser() = default;

    /// Initialize the parser with a markupText.
    bool Initialize(std::shared_ptr<UsdImagingMarkupText> markupText);

    /// Parse the string in the _markupText.
    bool ParseText();

protected:

    /// Parse and generate the text structure in the Internal Representation.
    /// After parsing, the Internal Representation will be in FTS_PRELAYOUT state.
    /// \param The internal representation.
    /// \param The parser which will parse the markup string
    bool _ParseInternalRepresentation(CommonParserParser* parser);

    /// The post process will make every line end consistent with the next line start.
    bool _PostProcess();

    std::shared_ptr<UsdImagingMarkupText> _markupText = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MARKUP_PARSER_H
