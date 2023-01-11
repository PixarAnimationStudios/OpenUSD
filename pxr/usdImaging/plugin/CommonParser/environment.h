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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_H

#include "globals.h"
#include "styleElement.h"
#include "transformElement.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum class CommonParserRunStatus
///
/// The action type of the sink.
///
enum class CommonParserRunStatus
{
    CommonParserRunStatusNotSet,
    CommonParserRunStatusDoNothing,
    CommonParserRunStatusConsume,
    CommonParserRunStatusReceive,
    CommonParserRunStatusFinish,
};

/// \class CommonParserAmbient
///
/// Global attributes of the text.
///
class CommonParserAmbient
{
private:
    float _definedWidth  = 0.0f;
    float _definedHeight = 0.0f;

    bool _ifVertical = false;

public:
    /// Constructor
    CommonParserAmbient() = default;

    /// Get the width of the text box.
    float DefinedWidth() const { return _definedWidth; }

    /// Get the height of the text box.
    float DefinedHeight() const { return _definedHeight; }

    /// Set the width of the text box.
    void DefinedWidth(float width) { _definedWidth = width; }

    /// Set the height of the text box.
    void DefinedHeight(float height) { _definedHeight = height; }

    /// Get if the text is vertical.
    bool IfVertical() const { return _ifVertical; }

    /// Set if the text is vertical.
    void IfVertical(bool value) { _ifVertical = value; }
};

/// \class CommonParserMarkupEnvironment
///
/// The CommonParserMarkupEnvironment contains the text display attributes
/// which may not be saved in CommonParserTextRun structure.
/// This class is a bridge of the parser and the sink.
///
class CommonParserMarkupEnvironment : public CommonParserEnvironment
{
private:
    CommonParserStyleDescriptionElement _ambientStyle;
    CommonParserTransformElement _ambientTransform;
    CommonParserStyleTable* _styleTable;
    CommonParserSink* _sink;
    CommonParserReferenceResolver* _resolver;
    CommonParserColor _rgbaCanvas;
    CommonParserAmbient* _textAmbient;
    // This state will tell the sink what it should do.
    CommonParserRunStatus _consumeState;

public:
    /// Constructor.
    CommonParserMarkupEnvironment(CommonParserSink* pSink,
                          CommonParserStyleTable* pStyleTable, 
                          CommonParserColor rgbaCanvas, 
                          CommonParserAmbient* ambient,
                          CommonParserReferenceResolver* resolver = nullptr);

public: // interface implementation
    /// Derived from parent interface.
    /// Get the global attribute.
    const CommonParserStyleDescription* AmbientStyle() const;

    /// Derived from parent interface.
    /// Get the global transform.
    const CommonParserTransform* AmbientTransform() const;

    /// Derived from parent interface.
    /// Get the style table.
    const CommonParserStyleTable* StyleDictionary() const;

    /// Derived from parent interface.
    /// Get the sink.
    CommonParserSink* Sink() const;

    /// Derived from parent interface.
    /// Get the resolver.
    CommonParserReferenceResolver* References() const;

    /// Derived from parent interface.
    /// Get the default color.
    CommonParserColor CanvasColor() const;

    /// Set the CommonParserAmbient.
    /// This is some attribute addtional to the ambient particle.
    CommonParserStatus SetTextAmbient(CommonParserAmbient* environment);

    /// Get the CommonParserAmbient.
    CommonParserAmbient* GetTextAmbient() const;

    /// Get the consume state.
    CommonParserRunStatus ConsumeState() const;

    /// Set the consume state.
    void ConsumeState(CommonParserRunStatus);
}; // end,TextEnvironment
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_ENVIRONMENT_H
