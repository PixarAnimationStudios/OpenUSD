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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_MTEXT_PARSER_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_MTEXT_PARSER_H

#include "pxr/pxr.h"
#include "globals.h"
#include "textRunElement.h"

#define MTEXT_PARSER_NAME L"MTEXT"

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonParserMTextParseInstance
///
/// The implementation of MText parser.
///
class CommonParserMTextParseInstance
{
public:
    /// The constructor from a string and the environment.
    CommonParserMTextParseInstance(
        const CommonParserStRange sMarkup, 
        CommonParserEnvironment* pEnv) 
        : _markup(sMarkup)
        , _env(pEnv)
    {
    }

    /// Parse the string.
    CommonParserStatus Parse();

    /// Returns the ACI of the color given, or -1 if no match
    static int RgbToAci(CommonParserColor);

    /// Does a distance match to pick the nearest ACI to a given color.
    static int RgbToNearestAci(CommonParserColor);

    /// Set the color for a block.
    static void SetBlockColor(long value)
    {
        if ((value & 0xFF000000) == 0xFF000000)
            _aciColorTable[0] = value;
        else if ((value & 0xFF000000) == 0)
            _aciColorTable[0] = _aciColorTable[value & 0x00FFFFFF];
        else
            return;
    }

    /// Set the color for foreground.
    static void SetForeGroundColor(long value)
    {
        if ((value & 0xFF000000) == 0xFF000000)
            _aciColorTable[7] = value;
        else if ((value & 0xFF000000) == 0)
            _aciColorTable[7] = _aciColorTable[value & 0x00FFFFFF];
        else
            return;
    }

    /// Set the color for the layer.
    static void SetLayerColor(long value)
    {
        if ((value & 0xFF000000) == 0xFF000000)
            _aciColorTable[256] = value;
        else if ((value & 0xFF000000) == 0)
            _aciColorTable[256] = _aciColorTable[value & 0x00FFFFFF];
        else
            return;
    }

    /// Get the color at specified index.
    static long GetIndexedColor(int index)
    {
        return _aciColorTable[index];
    }

private:
    /// Constructor-provided values
    CommonParserStRange _markup;
    CommonParserEnvironment* _env;

    /// Internally-used values.
    /// Current location of parsing.
    CommonParserStRange _here;

    static long _aciColorTable[257];

private:
    /// Methods used internally
    void _UpdateContentsPointer(CommonParserTextRunElement& Run, int iAdvance = 1);

    /// The main process of parsing.
    CommonParserStatus _ParseContext(CommonParserTextRunElement* pOuter);

    /// Parse the "\A" markup.
    CommonParserStatus _Parse_A(CommonParserTextRunElement& Run);

    /// Parse the "\C" markup.
    CommonParserStatus _Parse_C(CommonParserTextRunElement& Run);

    /// Parse the "\c" markup.
    CommonParserStatus _Parse_c(CommonParserTextRunElement& Run);

    /// Parse the "\F" markup.
    CommonParserStatus _Parse_F(CommonParserTextRunElement& Run);

    /// Parse the "\f" markup.
    CommonParserStatus _Parse_f(CommonParserTextRunElement& Run);

    /// Parse the "\H" markup.
    CommonParserStatus _Parse_H(CommonParserTextRunElement& Run);

    /// Parse the "\L" markup.
    CommonParserStatus _Parse_L(CommonParserTextRunElement& Run);

    /// Parse the "\l" markup.
    CommonParserStatus _Parse_l(CommonParserTextRunElement& Run);

    /// Parse the "\N" markup.
    CommonParserStatus _Parse_N(CommonParserTextRunElement& Run);

    /// Parse the "\O" markup.
    CommonParserStatus _Parse_O(CommonParserTextRunElement& Run);

    /// Parse the "\o" markup.
    CommonParserStatus _Parse_o(CommonParserTextRunElement& Run);

    /// Parse the "\p" markup.
    CommonParserStatus _Parse_p(CommonParserTextRunElement& Run);

    /// Parse the "\Q" markup.
    CommonParserStatus _Parse_Q(CommonParserTextRunElement& Run);

    /// Parse the "\S" markup.
    CommonParserStatus _Parse_S(CommonParserTextRunElement& Run);

    /// Parse the "\T" markup.
    CommonParserStatus _Parse_T(CommonParserTextRunElement& Run);

    /// Parse the "\U" markup.
    CommonParserStatus _Parse_U(CommonParserTextRunElement& Run);

    /// Parse the "\W" markup.
    CommonParserStatus _Parse_W(CommonParserTextRunElement& Run);

    ///                         over
    /// Produces a traditional ----- fraction.
    ///                        under
    CommonParserStatus _Parse_S_OverUnder(CommonParserTextRunElement& Run,
                                          CommonParserStRange sNumer,
                                          CommonParserStRange sDenom);

    ///                                      1 /
    ///  Implements a "vulgar" fraction, ie,  /  and such.
    ///                                      / 2
    CommonParserStatus _Parse_S_Vulgar(CommonParserTextRunElement& Run,
                                       CommonParserStRange sNumer,
                                       CommonParserStRange sDenom);
    /// Produces a tolerance (two left-justified over/and-under arrangement, sans fraction line)
    CommonParserStatus _Parse_S_Tolerance(CommonParserTextRunElement& Run,
                                          CommonParserStRange sNumer, 
                                          CommonParserStRange sDenom);
    ///  This produces a decimal-aligned stack.     +99.09
    ///                                            +101.10
    CommonParserStatus _Parse_S_Decimal(CommonParserTextRunElement& Run,
                                        CommonParserStRange sNumer,
                                        CommonParserStRange sDenom,
                                        CHARTYPE chDecimal);
    ///
    CommonParserStatus _Parse_S_Decimal_Part(CommonParserTextRunElement& Run,
                                             CommonParserStRange sWhole, 
                                             CommonParserStRange sDecimal);

    /// Processes %< ... >% insertion, or complains if failed.
    CommonParserStatus _ParseFieldInsertion(CommonParserTextRunElement& Run);

    /// Handles all markup that isn't prefixed with a backslash
    /// Notably: nesting constructs { and }
    ///          newline character (immediate line break)
    ///          %% metacharacters
    ///          %< ... >% expansion notation
    CommonParserStatus _Parse_NonBackslash(CommonParserTextRunElement& Run);

    /// Gets an NUMBER from the parameter string indicated.
    CommonParserStatus _GetNumber(CommonParserStRange& sString,
                                  NUMBER& nNumber);

    /// Gets a large NUMBER from the parameter string indicated.
    CommonParserStatus _GetLargeNumber(CommonParserStRange& sString,
                                       double& nNumber);

    /// Parse the parameter.
    CommonParserStatus _ParseForParameter(CommonParserStRange& sParam);

    /// Handle meta character.
    CommonParserStatus _SendMetacharacter(CommonParserTextRunElement& Run,
                                          const CommonParserStRange& sMeta);

    /// Does the handiwork of dispatching a TextRun notification.
    CommonParserStatus _SendTextRunNotification(CommonParserTextRunElement& Run);
    /// Does the handiwork of dispatching a TextLine notification.
    CommonParserStatus _SendNewlineNotification(CommonParserTextRunElement& Run);
    /// Does the handiwork of dispatching a block notification.
    CommonParserStatus _SendStructureNotification(CommonParserTextRunElement& Run);

    /// Does the handiwork of dispatching an Abandon notification.
    CommonParserStatus _Abandon(CommonParserStatusType eReason, 
                               const CommonParserStRange sPos);

    /// Get the hex value of a character.
    int _HexChar(CHARTYPE ch);
};

/// \class CommonParserMTextParser
///
/// The MText Parser.
///
class CommonParserMTextParser : public CommonParserParser
{
private:
    friend class CommonParserMTextGenerator;

    /// The constructor from a generator.
    CommonParserMTextParser(CommonParserGenerator* pGenerator);
    CommonParserGenerator* _generator;

public:
    /// Implementations of the CommonParserParser interface
    /// Parse a markup string by creating an "environment" with all the
    /// ambient settings, then combine that with a string to parse, and
    /// give it to this method.
    ///
    /// Since MTEXT is a nested grammar, this just does boiler-plate administrative
    /// work, and defers the actual parsing to ParseContext.
    CommonParserStatus Parse(const CommonParserStRange sMarkup,
                             CommonParserEnvironment* pEnv) override;

    /// Get the generator.
    CommonParserGenerator* GetGenerator() override;

    /// Set the color of block.
    void SetBlockColor(long value) { CommonParserMTextParseInstance::SetBlockColor(value); }

    /// Set the color of foreground.
    void SetForeGroundColor(long value) { CommonParserMTextParseInstance::SetForeGroundColor(value); }

    /// Set the color of layer.
    void SetLayerColor(long value) { CommonParserMTextParseInstance::SetLayerColor(value); }

    /// Get the color of specified color.
    long GetIndexedColor(int index) { return CommonParserMTextParseInstance::GetIndexedColor(index); }

    /// The destructor.
    virtual ~CommonParserMTextParser() = default;
};

/// \class CommonParserMTextGenerator
///
/// The MTEXT Parser just needs to instance one of these as
/// a singleton in order to be automatically self registering
/// and unregistering.
///
class CommonParserMTextGenerator : public CommonParserGenerator
{
protected:
    /// Indicate whether the unregisteration is needed.
    bool _endRegister;

public:
    CommonParserUniverse* _universe;

public:
    /// Self-registering ctor and dtor.
    CommonParserMTextGenerator();
    ~CommonParserMTextGenerator();

public:
    /// The name of the markup this parser represents
    /// such as "SVG" or "RTF" or ...
    const CommonParserStRange Name() const override;

    /// Documentation of the parser/generator (for
    /// version reporting, etc.)  A human-readable string.
    const CommonParserStRange Description() const override;

    /// Creates an instance to a new Parser
    CommonParserStatus Create(CommonParserParser**) override;

    /// Takes a pointer to an existing parser and destroys it.
    CommonParserStatus Destroy(CommonParserParser*) override;

    bool HasSink() const override;

    /// Creates an instance to a new Parser
    CommonParserStatus Create(CommonParserSink**) override;

    /// Takes a pointer to an existing parser and destroys it.
    CommonParserStatus Destroy(CommonParserSink*) override;

    /// The universe is destroyed so that this generator
    /// needn't do unregistration.
    CommonParserStatus RegisterNull() override;
};

CommonParserUniverse* GetUniverse();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_MTEXT_PARSER_H
