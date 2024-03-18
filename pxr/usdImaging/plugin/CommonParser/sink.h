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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_SINK_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_SINK_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "globals.h"
#include <stack>

#define TEXT_ATOM_GENERATOR_NAME L"COMMONTEXT"
#define TEXT_ATOM_GENERATOR_DESC                                                                   \
    L"Convert to text \
intermediate structure.\nVersion: 1.0\nParser: Unsupport\nSink: Support\n"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserMarkupGenerator
///
/// The generator for generate the CommonParserSink
///
class CommonParserMarkupGenerator : public CommonParserGenerator
{
protected:
    // Indicate whether the unregisteration is needed.
    bool _endRegister;

public:
    /// The default constructor.
    CommonParserMarkupGenerator();

    /// The destructor.
    virtual ~CommonParserMarkupGenerator();

public:
    /// The name of the markup this parser represents.
    const CommonParserStRange Name() const;

    /// Documentation of the parser/generator (for
    /// version reporting, etc.)  A human-readable string.
    const CommonParserStRange Description() const;

    /// Takes a pointer to an existing parser and destroys it.
    /// Unimplemented for this generator.
    CommonParserStatus Destroy(CommonParserParser*);

    /// Creates an instance to a new Sink
    CommonParserStatus Create(CommonParserSink**);

    /// Creates an instance to a new Parser.
    /// Unimplemented for this generator.
    CommonParserStatus Create(CommonParserParser**);

    /// Whether the generator has a sink or not
    bool HasSink() const;

    /// Takes a pointer to an existing sink and destroys it.
    CommonParserStatus Destroy(CommonParserSink*);

    /// The universe is destroyed so that this generator
    /// needn't do unregistration.
    CommonParserStatus RegisterNull();
};

/// \enum class CommonParserHeightChange
///
/// 0: No height change. 1: Proportional height change. 2, Inproportional height change.
///
enum class CommonParserHeightChange
{
    CommonParserHeightChangeNoChange       = 0,
    CommonParserHeightChangeProportional   = 1,
    CommonParserHeightChangeInproportional = 2
};

/// \class CommonParserSink
///
/// The sink implementation in CommonParser module
///
class CommonParserMarkupSink : public CommonParserSink
{
    friend class CommonParserMarkupGenerator;

protected:
    std::shared_ptr<UsdImagingMarkupText> _markupText;
    CommonParserGenerator* _generator = nullptr;
    CommonParserSinkStateType _sinkState = CommonParserSinkStateTypeWaiting;

    // The current text height is saved, because sometimes
    // the text height is defined as proportional of the previous height.
    std::shared_ptr<UsdImagingTextStyle> _currentTextStyle;

    UsdImagingTextParagraphStyle _currentParagraphStyle;

    TextStyleMap _textStyleMap;
    std::stack<int> _textStyleStack;

    int _currentDepth = 0;

    UsdImagingTextRunList::iterator _currentTextRunIter;
    int _currentColumnIndex    = 0;
    UsdImagingTextLineList::iterator _currentTextLineIter;
    int _currentParagraphIndex = -1;

public:
    /// The constructor needs the FullText.
    void InternalRepresentation(std::shared_ptr<UsdImagingMarkupText> value)
    {
        assert(value != nullptr);
        _markupText = value;
    }

    /// The destructor.
    std::shared_ptr<UsdImagingMarkupText> InternalRepresentation() { return _markupText; }

    /// The state of the sink.
    CommonParserSinkStateType SinkState() override;

    /// The initialization.
    CommonParserStatus Initialize(CommonParserEnvironment*) override;

    /// This method is the main process of the sink.
    /// It will receive a text run and layout it or cache it.
    /// \param The text run that the layout manager will manipulate.
    /// \param The manipulation environment, which may contain some global attributes.
    CommonParserStatus TextRun(CommonParserTextRun*, 
                               CommonParserEnvironment*) override;

    /// The sink is in the abandon state.
    /// [in] The manipulation environment, which may contain some global attributes.
    CommonParserStatus Abandon(CommonParserAbandonment* abandonment, 
                               CommonParserEnvironment* env) override;

    /// Derived from parent interface.
    /// The sink is terminated,
    /// \param The manipulation environment, which may contain some global attributes.
    CommonParserStatus Terminate(CommonParserEnvironment* env) override;

    /// Get the generator that can generate the sink.
    CommonParserGenerator* GetGenerator() override;

private:
    /// The sink can be generated and destructed only by the generator
    /// So the constructor and destructor are all private.
    /// This is the default constructor.
    CommonParserMarkupSink(CommonParserGenerator* pGenerator);

    /// The destructor
    virtual ~CommonParserMarkupSink();

    /// Generate a ATextRun from the CommonParserTextRun
    /// and add it to the TextRun list in the Internal Representation
    /// \param The ATextRun is created from this.
    /// \param[in, out]The output ATextRun.
    bool _AddTextRun(CommonParserTextRun* pITextRun, 
                     std::shared_ptr<UsdImagingTextRun>& pTextRun);

    /// Handle the structure particle.
    bool _HandleStructure(CommonParserTextRun* pITextRun);

    /// Handle the location particle. Generate columns, paragraphs and lines if needed.
    bool _HandleLocation(CommonParserTextRun* pITextRun);

    /// Handle the transform particle.
    /// The Oblique angle and character width features require this.
    bool _HandleTransform(CommonParserTextRun* pITextRun,
                          std::shared_ptr<UsdImagingTextRun> pTextRun);
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_SINK_H
