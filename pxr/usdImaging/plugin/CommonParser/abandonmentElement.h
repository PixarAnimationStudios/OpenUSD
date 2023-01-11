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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ABANDONMENT_ELEMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ABANDONMENT_ELEMENT_H

#include "pxr/pxr.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserSink
///
/// The sink implementation in CommonParser module
///
class CommonParserAbandonmentElement: public CommonParserAbandonment 
{
public:
    /// The constructor from status.
    CommonParserAbandonmentElement(CommonParserStatus);

    /// Set the string being parsed.
    void SetMarkup(const CommonParserStRange& sEntireString);

    /// Set local context of the string being parsed.
    void SetContext(const CommonParserStRange& sCurrentLine);

    /// Set the specific location where the error Occurred.
    void SetPosition(const CommonParserStRange& sCurrentLine);

    /// Implement CommonParserAbandonment::Reason().
    const CommonParserStatus   Reason() override;

    /// Implement CommonParserAbandonment::Markup().
    const CommonParserStRange& Markup() override;

    /// Implement CommonParserAbandonment::Context().
    const CommonParserStRange& Context() override;

    /// Implement CommonParserAbandonment::Position().
    const CommonParserStRange& Position() override;

private:
    CommonParserStatus    _oStatus;

    CommonParserStRange   _sMarkupString;      // generally, the entire string.
    CommonParserStRange   _sMarkupContext;     // the context surrounding the abandonment
    CommonParserStRange   _sAbandonedPosition; // The position at which abandonment occurred.
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ABANDONMENT_ELEMENT_H
