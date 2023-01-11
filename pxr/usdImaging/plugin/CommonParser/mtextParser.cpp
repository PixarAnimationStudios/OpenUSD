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

#include "mtextParser.h"
#include "abandonmentElement.h"
#include "portableUtils.h"

// Reserve indices for specific purposes.
#define STACK_LEFT_BOOKMARK_INDEX 0
#define STACK_RIGHT_BOOKMARK_INDEX 1

/*

This is the minimum set of MTEXT opcodes that need to be supported,
and is derived from the Topobase requirements.

\A#;  Vertical Alignment
\C#;  Autodesk color index,
\c#;  True color in decimal
\ffont[b#][i#][p#][cN]  TrueType font override
\Ffontfile[,bigfontfile][|c#]; SHX font override
\H#;  Text height
\L  Underline on
\l  Underline off
\N  Column end
\O  Overline off
\o  Overline off
\Q#;  Obliquing Angle
\p[x]l#,i#,r#,q{*lcrjd},s{*eam}[#],b#,a#,t[z][#,c#,r#,d#]; Advanced Paragraph settings.
\S[numer]sep[char][denom];  Stack
\T#;  Tracking factor
\U+xxxx  Unicode codepoint
\W#;  character width
\\ (double backslash)  produces a backslash
{  Initiate MText override.
}  Terminate MTEXT override
\~  Non-breaking space
Carriage Return  Line end (soft break)
%%c or %%C  Diameter symbol

This parser also supports

\U+xxxx CIF notation
%< .. >% field notation (delegating the interpretation of the contents
to the CommonParserReferenceResolver)

General Format:
%<\EvalId FieldCode \f "Format" \href "Link"%

Example of use of field notation:
Drawing created by %<\AcVar author>% on %<\AcVar SaveDate \f "M/d/yyyy">%

*/

PXR_NAMESPACE_OPEN_SCOPE
CommonParserMTextParser::CommonParserMTextParser(CommonParserGenerator* pGenerator) 
    : _generator(pGenerator) {}

// Parse a markup string by creating an "environment" with all the
// ambient settings, then combine that with a string to parse, and
// give it to this method.
CommonParserStatus 
CommonParserMTextParser::Parse(
    const CommonParserStRange sMarkup, 
    CommonParserEnvironment* pEnv)
{
    // Nice stack-based implementation; keeps things simple.
    // gee, and it's thread-safe, too!
    CommonParserMTextParseInstance instance(sMarkup, pEnv);

    return instance.Parse();
}

CommonParserGenerator* 
CommonParserMTextParser::GetGenerator()
{
    return _generator;
}

// Parse a markup string by creating an "environment" with all the
// ambient settings, then combine that with a string to parse, and
// give it to this method.
CommonParserStatus 
CommonParserMTextParseInstance::Parse()
{
    CommonParserStatus eRet = _env->Sink()->Initialize(_env);
    if (eRet.Succeeded())
    {
        _here.Set(_markup.Start(), 1);
        eRet = _ParseContext(/* No outer TextRun context*/ nullptr);
        _env->Sink()->Terminate(_env);
    }
    return eRet;
}

CommonParserStatus 
CommonParserMTextParseInstance::_SendMetacharacter(
    CommonParserTextRunElement& Run, 
    const CommonParserStRange& sMeta)
{
    // Meta-characters... we're not keeping a working buffer, so we
    // break the run around these characters.
    CommonParserStatus eRet = _SendTextRunNotification(Run);
    if (!eRet.Succeeded())
        return eRet;

    Run.Contents() = sMeta;

    return _SendTextRunNotification(Run);
}

// Only extends the Run.Length, unless reset.
void 
CommonParserMTextParseInstance::_UpdateContentsPointer(
    CommonParserTextRunElement& Run,
    int iAdvance)
{
    if (Run.IsReset())
    {
        Run.Contents().SetStart(_here.Start());
        Run.Contents().SetLength(iAdvance);
    }
    else
        Run.Contents().AddLength(iAdvance);
}

// Implements the \A#; command
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_A(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    if (!this->_ParseForParameter(parm).Succeeded())
        return _Abandon(CommonParserStatusTypeUnexpectedCharacter, parm);

    NUMBER nAlign;
    switch (parm[0])
    {
    default:
    case '0':
        nAlign = 0.0f;
        break;
    case '1':
        nAlign = 0.5f;
        break;
    case '2':
        nAlign = 1.0f;
        break;
    }

    Run.Style().AddDelta(CommonParserAdvanceAlignmentStyleParticle(
        CommonParserMeasure(nAlign, CommonParserMeasureUnitUnitless, &parm)));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \C#; command.
CommonParserStatus
CommonParserMTextParseInstance::_Parse_C(CommonParserTextRunElement& Run)
{
    _here.Move(1);

    CommonParserStRange parm = _here;
    NUMBER nACI;
    if (!this->_GetNumber(parm, nACI).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    int iACI = (int)nACI;
    // Out of bounds, or not an int?
    if (iACI < 0 || iACI > 256)
    {
        // Invalid Argument
        // change to foreground color
        iACI = 7;
        nACI = 7.0;
    }

    Run.Style().AddDelta(CommonParserFillColorStyleParticle(
        CommonParserColor(static_cast<long>(_aciColorTable[iACI]))));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \c#; command.
CommonParserStatus
CommonParserMTextParseInstance::_Parse_c(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    double nColor;
    if (!_GetLargeNumber(parm, nColor).Succeeded())
        return _Abandon(CommonParserStatusTypeUnexpectedCharacter, parm);

    int r = (int)(((long)nColor) & 0x000000FF);
    int g = (int)(((long)nColor) & 0x0000FF00) >> 8;
    int b = (int)(((long)nColor) & 0x00FF0000) >> 16;
    Run.Style().AddDelta(CommonParserFillColorStyleParticle(CommonParserColor(r, g, b)));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_F(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    if (!_ParseForParameter(parm).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    CommonParserStRange s = parm.Split('|');
    Run.Style().AddDelta(CommonParserTypefaceStyleParticle(s));
    Run.Style().AddDelta(CommonParserIsSHXStyleParticle(true));
    do
    {
        s = parm.Split('|');

        if (s.Length() == 0)
            break;
        switch (s[0])
        {
        case 'c':
            Run.Style().AddDelta(
                CommonParserCharacterSetStyleParticle(PXR_INTERNAL_NS::wtoi(s.Start() + 1)));
            break;

        default:
            return _Abandon(CommonParserStatusTypeUnknownMarkup, parm);
        }
    } while (parm.Length() > 0);

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \fname|b#|i#|p#|c#; command.
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_f(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    if (!_ParseForParameter(parm).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    CommonParserStRange s = parm.Split('|');

    // Trim the white space before or after the typeface.
    int moveStart = 0;
    while (iswspace(s[moveStart]))
    {
        moveStart++;
    }
    s.MoveStart(moveStart);

    int moveEnd = s.Length();
    while (iswspace(s[moveEnd - 1]))
    {
        moveEnd--;
    }
    s.SetLength(moveEnd);

    Run.Style().AddDelta(CommonParserTypefaceStyleParticle(s));
    Run.Style().AddDelta(CommonParserIsSHXStyleParticle(false));
    do
    {
        s = parm.Split('|');
        if (s.Length() == 0)
            break;
        switch (s[0])
        {
        case 'b':
            Run.Style().AddDelta(CommonParserFontWeightStyleParticle(s[1] == '1'
                    ? CommonParserFontWeightTypeBold
                    : CommonParserFontWeightTypeNormal));
            break;

        case 'i':
            Run.Style().AddDelta(CommonParserItalicStyleParticle(s[1] == '1'));
            break;

        case 'c':
            Run.Style().AddDelta(
                CommonParserCharacterSetStyleParticle(PXR_INTERNAL_NS::wtoi(s.Start() + 1)));
            break;

        case 'p':
            Run.Style().AddDelta(CommonParserPitchFamilyStyleParticle(
                (CommonParserPitchFamilyType)PXR_INTERNAL_NS::wtoi(s.Start() + 1)));
            break;

        default:
            return _Abandon(CommonParserStatusTypeUnknownMarkup, parm);
        }
    } while (parm.Length() > 0);

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \H#; and \H#x; command variations.
CommonParserStatus
CommonParserMTextParseInstance::_Parse_H(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    NUMBER nHeight;
    if (!_GetNumber(parm, nHeight).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    if (parm.Last() == L'x')
    {
        Run.Style().AddDelta(CommonParserSizeStyleParticle(
            CommonParserMeasure(nHeight, CommonParserMeasureUnitProportion, &parm)));
    }
    else
    {
        Run.Style().AddDelta(CommonParserSizeStyleParticle(
            CommonParserMeasure(nHeight, CommonParserMeasureUnitModel, &parm)));
    }

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \L command.
CommonParserStatus
CommonParserMTextParseInstance::_Parse_L(CommonParserTextRunElement& Run)
{
    Run.Style().AddDelta(
        CommonParserUnderlineStyleParticle(CommonParserTextLineTypeSingle));

    return CommonParserStatusTypeOk;
}

// Implements the \l command.
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_l(CommonParserTextRunElement& Run)
{
    Run.Style().AddDelta(CommonParserUnderlineStyleParticle(CommonParserTextLineTypeNone));

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_N(CommonParserTextRunElement& Run)
{
    Run.Location().SetSemantics(CommonParserSemanticTypeFlowColumn);
    Run.Location().AddOperation(CommonParserLineBreakLocationParticle());

    // Parameterless opcode, so no need to advance
    CommonParserStatus eRet = _SendNewlineNotification(Run);
    if (!eRet.Succeeded())
        return eRet;
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_O(CommonParserTextRunElement& Run)
{
    Run.Style().AddDelta(CommonParserOverlineStyleParticle(CommonParserTextLineTypeSingle));
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_o(CommonParserTextRunElement& Run)
{
    Run.Style().AddDelta(CommonParserOverlineStyleParticle(CommonParserTextLineTypeNone));

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_p(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    if (!_ParseForParameter(parm).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    bool proportionFlag = false;
    if (parm[0] == 'x')
    {
        proportionFlag = true;
        parm.MoveStart(1);
    }
    while (parm.Length() > 0)
    {
        CommonParserStRange s = parm.Split(',');
        switch (s[0])
        {
        case 'l':
            break;
        case 'i':
            break;
        case 'r':
            break;
        case 'q':
            break;
        case 's':
        {
            wchar_t lineSpaceType = s[1];
            s.MoveStart(2);
            if (proportionFlag == false)
            {
                if (lineSpaceType != L'*')
                {
                    NUMBER nHeight;
                    if (!_GetNumber(s, nHeight).Succeeded())
                        return _Abandon(CommonParserStatusTypeIncompleteString, s);
                    switch (lineSpaceType)
                    {
                    case L'a':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitModel, &s),
                                CommonParserLineHeightMeasureTypeAtLeast)));
                        break;
                    case L'e':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitModel, &s),
                                CommonParserLineHeightMeasureTypeExactly)));
                        break;
                    case L'm':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitModel, &s),
                                CommonParserLineHeightMeasureTypeMultiple)));
                        break;
                    default:
                        break;
                        // TODO add warning: "bad MText parsing, line space type not 'a', 'e', or
                        // 'm'."
                    }
                }
            }
            else
            {
                if (lineSpaceType != L'*')
                {
                    NUMBER nHeight;
                    if (!_GetNumber(s, nHeight).Succeeded())
                        return _Abandon(CommonParserStatusTypeIncompleteString, s);
                    switch (lineSpaceType)
                    {
                    case L'a':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitProportion, &s),
                                CommonParserLineHeightMeasureTypeAtLeast)));
                        break;
                    case L'e':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitProportion, &s),
                                CommonParserLineHeightMeasureTypeExactly)));
                        break;
                    case L'm':
                        Run.Style().AddDelta(CommonParserLineHeightStyleParticle(
                            CommonParserLineHeightMeasure(
                                CommonParserMeasure(
                                nHeight, CommonParserMeasureUnitProportion, &s),
                                CommonParserLineHeightMeasureTypeMultiple)));
                        break;
                    default:
                        break;
                        // TODO add warning: "bad MText parsing, line space type not 'a', 'e', or
                        // 'm'."
                    }
                }
            }
            break;
        }
        case 'b':
            break;
        case 'a':
            break;
        case 't':
            break;
        default:
            return _Abandon(CommonParserStatusTypeUnknownMarkup, parm);
        }
    }
    // Account for the trailRelativeLocationParticleing semicolon
    _here.SetStart(parm.Beyond());
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_Q(CommonParserTextRunElement& Run)
{
    _here.Move(1);

    CommonParserStRange parm = _here;
    NUMBER nAng;
    if (!_GetNumber(parm, nAng).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    if (nAng == 0.0)
        Run.Transform().RemoveSameTypeTransform(CommonParserSkewTransformParticle(
            0, 0, CommonParserTransformParticleSemanticsOblique));
    else
        Run.Transform().ReplaceTransform(
            CommonParserSkewTransformParticle(CommonParserDegreeRadialMeasure(nAng), 0,
                CommonParserTransformParticleSemanticsOblique));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

//                         over
// Produces a traditional ----- fraction.
//                        under
CommonParserStatus
CommonParserMTextParseInstance::_Parse_S_OverUnder(
    CommonParserTextRunElement& Run,
    CommonParserStRange sNumer,
    CommonParserStRange sDenom)
{
    // This guy is left justified, regardless of what's going on outside.
    // (But let's remember what that (outside) justification is.)
    CommonParserJustificationStyleParticle* pOldJustification =
        (CommonParserJustificationStyleParticle*)(Run.Style().GetDescriptionParticle(
            CommonParserStyleParticleTypeJustification));
    CommonParserJustificationType eOldJustification = pOldJustification != nullptr
        ? pOldJustification->Value()
        : CommonParserJustificationTypeLeft;

    if (eOldJustification != CommonParserJustificationTypeCentered)
        Run.Style().AddDelta(
            CommonParserJustificationStyleParticle(CommonParserJustificationTypeCentered));

    bool bHasNumer = sNumer.Length() > 0;
    bool bHasDenom = sDenom.Length() > 0;

    // Hack: read the BorderLineStyleParticle stuff below.
    bool bNumerLine = false;
    bool bDenomLine = false;

    //
    // Process the "numerator"
    //
    if (bHasNumer)
    {
        // Full stack (not fake superscript) so add some additional info.
        if (bHasDenom)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeInlineBlock);
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // We have to come back to this place... remember it.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));

            // Short-term hack, instead of relying on BorderLineStyleParticle.
            // See the OverlineStyleParticle in the corresponding code below for notes.
            if (sDenom.Length() < sNumer.Length())
            {
                Run.Style().AddDelta(
                    CommonParserUnderlineStyleParticle(CommonParserTextLineTypeSingle));
                bNumerLine = true;
            }
        }

        // Indicate that this is the "superscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSuperscript);

        Run.Contents() = sNumer;

        // This location stuff is subject to some fiddling (part 1)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        // Send the Numerator along
        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of superscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSuperscript);
        // This location stuff is subject to some fiddling (part 2)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));
    }

    //
    //  Process the "denominator"
    //
    if (bHasDenom)
    {
        // Full stack (not fake subscript) so add some additional info.
        if (bHasNumer)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // Remember how far out we came, because we need to come back here.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_RIGHT_BOOKMARK_INDEX));
            // Now, back to the left edge in preparation for the denom.
            Run.Location().AddOperation(
                CommonParserReturnToBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));

            // We assume, based on some use-case knowledge
            // of the stacked fraction, that these are likely to be numeric, and numbers are usually
            // fixed-width, even in variable-width fonts.
            if (sDenom.Length() >= sNumer.Length())
            {
                Run.Style().AddDelta(
                    CommonParserOverlineStyleParticle(CommonParserTextLineTypeSingle));
                bDenomLine = true;
            }
        }

        // Indicate that this is the "subscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSubscript);

        Run.Contents() = sDenom;

        // This location stuff is subject to some fiddling (part 3)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));

        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of subscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSubscript);
        // This location stuff is subject to some fiddling (part 4)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        if (bHasNumer)
        {
            // No longer in the "stack" region
            Run.Location().AddSemantic(CommonParserSemanticTypeEndInlineBlock);
            // Go as far as the
            Run.Location().AddOperation(CommonParserConditionalReturnToBookmarkLocationParticle(
                STACK_RIGHT_BOOKMARK_INDEX,
                CommonParserConditionTypeFarthestAdvance));
            // End the line
            if (bNumerLine)
                Run.Style().AddDelta(
                    CommonParserUnderlineStyleParticle(CommonParserTextLineTypeNone));
            if (bDenomLine)
                Run.Style().AddDelta(
                    CommonParserOverlineStyleParticle(CommonParserTextLineTypeNone));
        }
    }

    if (eOldJustification != CommonParserJustificationTypeCentered)
        Run.Style().AddDelta(CommonParserJustificationStyleParticle(eOldJustification));

    // Account for the trailing semicolon
    _here.SetStart(sDenom.Beyond());

    return CommonParserStatusTypeOk;
}

//                                      1 /
//  Implements a "vulgar" fraction, ie,  /  and such.
//                                      / 2
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_S_Vulgar(
    CommonParserTextRunElement& Run, 
    CommonParserStRange sNumer,
    CommonParserStRange sDenom)
{
    bool bHasNumer = sNumer.Length() > 0;
    bool bHasDenom = sDenom.Length() > 0;

    //
    //  Process the "numerator"
    //
    if (bHasNumer)
    {
        // Full stack (not fake superscript) so add some additional info.
        if (bHasDenom)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeInlineBlock);
        }

        // Indicate that this is the "superscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSuperscript);

        Run.Contents() = sNumer;

        // This location stuff is subject to some fiddling (part 1)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        // Send the Numerator along
        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of superscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSuperscript);
        // This location stuff is subject to some fiddling (part 2)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5f, CommonParserMeasureUnitEm, nullptr)));

        if (bHasDenom)
        {
            // A bit of "hand" kerning to tuck the slash under the numerator.
            Run.Location().AddOperation(CommonParserRelativeLocationParticle(
                CommonParserMeasure(-0.4f, CommonParserMeasureUnitEm, nullptr),
                CommonParserMeasure()));

            // The slash.
            Run.Contents() = L"/";

            // Send the slash along
            eRet = _SendTextRunNotification(Run);
            if (!eRet.Succeeded())
                return eRet;

            // Ditto: more hand kerning so the denominator starts under the slash.
            // (But this is against the slashe's em size.)
            Run.Location().AddOperation(CommonParserRelativeLocationParticle(
                CommonParserMeasure(-0.4f, CommonParserMeasureUnitEm, nullptr),
                CommonParserMeasure()));
        }
    }

    //
    //  Process the "denominator"
    //
    if (bHasDenom)
    {
        // Indicate that this is the "subscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSubscript);

        Run.Contents() = sDenom;

        // This location stuff is subject to some fiddling (part 3)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));

        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of subscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSubscript);
        // This location stuff is subject to some fiddling (part 4)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        if (bHasNumer)
        {
            // No longer in the "stack" region
            Run.Location().AddSemantic(CommonParserSemanticTypeEndInlineBlock);
        }
    }

    // Account for the trailing semicolon
    _here.SetStart(sDenom.Beyond());

    return CommonParserStatusTypeOk;
}

//
// Produces a tolerance (two left-justified over/and-under arrangement, sans fraction line)
//
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_S_Tolerance(
    CommonParserTextRunElement& Run, 
    CommonParserStRange sNumer,
    CommonParserStRange sDenom)
{
    // This guy is left justified, regardless of what's going on outside.
    // (But let's remember what that (outside) justification is.)
    CommonParserJustificationStyleParticle* pOldJustification =
        (CommonParserJustificationStyleParticle*)(Run.Style().GetDescriptionParticle(
            CommonParserStyleParticleTypeJustification));
    CommonParserJustificationType eOldJustification = pOldJustification != nullptr
        ? pOldJustification->Value()
        : CommonParserJustificationTypeLeft;

    if (eOldJustification != CommonParserJustificationTypeLeft)
        Run.Style().AddDelta(
            CommonParserJustificationStyleParticle(CommonParserJustificationTypeLeft));

    bool bHasNumer = sNumer.Length() > 0;
    bool bHasDenom = sDenom.Length() > 0;

    //
    //  Process the "numerator"
    //
    if (bHasNumer)
    {
        // Full stack (not fake superscript) so add some additional info.
        if (bHasDenom)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeInlineBlock);
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // We have to come back to this place... remember it.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));
        }

        // Indicate that this is the "superscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSuperscript);

        Run.Contents() = sNumer;

        // This location stuff is subject to some fiddling (part 1)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        // Send the Numerator along
        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of superscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSuperscript);
        // This location stuff is subject to some fiddling (part 2)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));
    }

    //
    //  Process the "denominator"
    //
    if (bHasDenom)
    {
        // Full stack (not fake subscript) so add some additional info.
        if (bHasNumer)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // Remember how far out we came, because we need to come back here.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_RIGHT_BOOKMARK_INDEX));
            // Now, back to the left edge in preparation for the denom.
            Run.Location().AddOperation(
                CommonParserReturnToBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));
        }

        // Indicate that this is the "subscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSubscript);

        Run.Contents() = sDenom;

        // This location stuff is subject to some fiddling (part 3)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));

        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of subscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSubscript);
        // This location stuff is subject to some fiddling (part 4)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        if (bHasNumer)
        {
            // No longer in the "stack" region
            Run.Location().AddSemantic(CommonParserSemanticTypeEndInlineBlock);
            // Go as far as the
            Run.Location().AddOperation(CommonParserConditionalReturnToBookmarkLocationParticle(
                STACK_RIGHT_BOOKMARK_INDEX,
                CommonParserConditionTypeFarthestAdvance));
        }
    }

    if (eOldJustification != CommonParserJustificationTypeLeft)
        Run.Style().AddDelta(CommonParserJustificationStyleParticle(eOldJustification));

    // Account for the trailing semicolon
    _here.SetStart(sDenom.Beyond());

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_S_Decimal_Part(
    CommonParserTextRunElement&,
    CommonParserStRange,
    CommonParserStRange)
{
    return CommonParserStatusTypeNotImplemented;
}

//
//  This produces a decimal-aligned stack.     +99.09
//                                            +101.10
//
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_S_Decimal(
    CommonParserTextRunElement& Run,
    CommonParserStRange sNumerDeci,
    CommonParserStRange sDenomDeci,
    CHARTYPE chDecimal)
{
    // We attack the problem by dividing the numerator and denominator into
    // whole and decimal parts, respectively.  These four pieces will be presented
    // in a 2x2 table, where the whole parts are right justified and the decimal
    // are left justified.  In most other respects, this follows ordinary tolerance stacks.

    // We remember what that (outside) justification is.
    CommonParserJustificationStyleParticle* pOldJustification =
        (CommonParserJustificationStyleParticle*)(Run.Style().GetDescriptionParticle(
            CommonParserStyleParticleTypeJustification));
    CommonParserJustificationType eOldJustification = pOldJustification != nullptr
        ? pOldJustification->Value()
        : CommonParserJustificationTypeLeft;

    bool bHasNumer = sNumerDeci.Length() > 0;
    bool bHasDenom = sDenomDeci.Length() > 0;

    //
    //  Process the "numerator"
    //
    if (bHasNumer)
    {
        // Full stack (not fake superscript) so add some additional info.
        if (bHasDenom)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeTable);
            Run.Location().AddSemantic(CommonParserSemanticTypeInlineBlock);
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // We have to come back to this place... remember it.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));

            // Whole number part is right-justified
            Run.Style().AddDelta(
                CommonParserJustificationStyleParticle(CommonParserJustificationTypeRight));
        }

        // Indicate that this is the "superscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSuperscript);

        CommonParserStRange sNumerWhole =
            sNumerDeci.Split(chDecimal); // sNumerDeci reduced to what follows decimal point.
        sNumerDeci.MoveStart(-1);        // back up, to include the point.

        Run.Contents() = sNumerWhole;

        // This location stuff is subject to some fiddling (part 1)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        // Send the Numerator Whole along
        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        if (bHasDenom)
        {
            // Okay, we move on to the next item.
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);
        }

        // The decimal part is left-justified
        Run.Style().AddDelta(
            CommonParserJustificationStyleParticle(CommonParserJustificationTypeLeft));

        Run.Contents() = sNumerDeci;

        // Send the Numerator decimal along
        eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of superscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSuperscript);
        // This location stuff is subject to some fiddling (part 2)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));
    }

    //
    //  Process the "denominator"
    //
    if (bHasDenom)
    {
        // Full stack (not fake subscript) so add some additional info.
        if (bHasNumer)
        {
            Run.Location().AddSemantic(CommonParserSemanticTypeRow);
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);

            // Remember how far out we came, because we need to come back here.
            Run.Location().AddOperation(
                CommonParserBookmarkLocationParticle(STACK_RIGHT_BOOKMARK_INDEX));
            // Now, back to the left edge in preparation for the denom.
            Run.Location().AddOperation(
                CommonParserReturnToBookmarkLocationParticle(STACK_LEFT_BOOKMARK_INDEX));
            // Whole number part is right-justified
            Run.Style().AddDelta(
                CommonParserJustificationStyleParticle(CommonParserJustificationTypeRight));
        }

        // Indicate that this is the "subscript" part of the stack
        Run.Location().AddSemantic(CommonParserSemanticTypeSubscript);

        // sDenomDeci reduced ...
        CommonParserStRange sDenomWhole = sDenomDeci.Split(chDecimal);
        sDenomDeci.MoveStart(-1); // back up, to include the point.

        Run.Contents() = sDenomWhole;

        // This location stuff is subject to some fiddling (part 3)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(-0.5, CommonParserMeasureUnitEm, nullptr)));

        // Send the Denominator Whole along
        CommonParserStatus eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        if (bHasDenom)
        {
            // Okay, we move on to the next item.
            Run.Location().AddSemantic(CommonParserSemanticTypeCell);
        }

        // The decimal part is left-justified
        Run.Style().AddDelta(
            CommonParserJustificationStyleParticle(CommonParserJustificationTypeLeft));

        Run.Contents() = sDenomDeci;

        // Send the Numerator decimal along
        eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        //
        // Prepare for what follows
        //

        // End of subscript
        Run.Location().AddSemantic(CommonParserSemanticTypeEndSubscript);

        // This location stuff is subject to some fiddling (part 4)
        Run.Location().AddOperation(
            CommonParserRelativeLocationParticle(CommonParserMeasure(),
                CommonParserMeasure(0.5, CommonParserMeasureUnitEm, nullptr)));

        if (bHasNumer)
        {
            // No longer in the "stack" region
            Run.Location().AddSemantic(CommonParserSemanticTypeEndInlineBlock);
            Run.Location().AddSemantic(CommonParserSemanticTypeEndTable);
            // Go as far as the right edge, unless we're farther.
            Run.Location().AddOperation(CommonParserConditionalReturnToBookmarkLocationParticle(
                STACK_RIGHT_BOOKMARK_INDEX,
                CommonParserConditionTypeFarthestAdvance));
        }
    }

    Run.Style().AddDelta(CommonParserJustificationStyleParticle(eOldJustification));
    // Account for the trailing semicolon
    _here.SetStart(sDenomDeci.Beyond());

    return CommonParserStatusTypeOk;
}

// Implements the \Snumer/denom; command. (stack)
// Formally, this may be:                                 1
//   \Snumer/denom;  -- traditional over/under fraction: ---
//   \Snumer#denom;  -- vulgar fraction  1/2              2
//   \Snumer^denom;  -- tolerance, numbers left-aligned
//   \Snumer~.denom; -- decimal-aligned stack, where . represents the decimal char
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_S(CommonParserTextRunElement& Run)
{
    _here.Move(1);

    CommonParserStRange parm = _here;
    if (!_ParseForParameter(parm).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    // Okay, we know we've got a reasonably-well formed stack.
    // Let's flush what's before it.
    CommonParserStatus eRet = _SendTextRunNotification(Run);
    if (!eRet.Succeeded())
        return eRet;

    // Mark a recursive block surrounding the stack.

    // Now, with a clean slate, let's move forward:
    CommonParserStRange numer;
    const CHARTYPE* pSep = nullptr;
    CommonParserStRange denom;

    for (int i = 0; i < parm.Length(); i++)
    {
        switch (parm[i])
        {
        case '/':
        case '#':
        case '^':
            numer.Set(parm.Start(), i);
            pSep  = parm.Start() + i;
            denom = parm.Part(i + 1);
            break;

        case '~':
            numer.Set(parm.Start(), i);
            pSep  = parm.Start() + i;
            denom = parm.Part(i + 2);
            break;
        }
        if (pSep != nullptr)
            break;
    }

    if (pSep != nullptr)
    {
        switch (*pSep)
        {
        case '/':
            return _Parse_S_OverUnder(Run, numer, denom);
        case '#':
            return _Parse_S_Vulgar(Run, numer, denom);
        case '^':
            return _Parse_S_Tolerance(Run, numer, denom);
        case '~':
            return _Parse_S_Decimal(Run, numer, denom, pSep[1]);
        default:
            return _Abandon(
                CommonParserStatusTypeInternalError, CommonParserStRange(pSep, 2));
        }
    }
    else
        return _Abandon(CommonParserStatusTypeInvalidArg, CommonParserStRange(pSep, 2));
}

CommonParserStatus 
CommonParserMTextParseInstance::_Parse_T(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    NUMBER nTrack;
    if (!_GetNumber(parm, nTrack).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    Run.Style().AddDelta(CommonParserTrackingAugmentStyleParticle(
        CommonParserMeasure(nTrack, CommonParserMeasureUnitProportion, &parm)));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

int
CommonParserMTextParseInstance::_HexChar(CHARTYPE ch)
{
    switch (ch)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return ch - '0';
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return ch - 'a' + 10;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return ch - 'A' + 10;
    }

    return -1;
}

// Implements the \U+xxxx command. (Unicode codepoint)
CommonParserStatus 
CommonParserMTextParseInstance::_Parse_U(CommonParserTextRunElement& Run)
{
    _here.Move(1);

    CommonParserStRange parm(_here.Start(), 5);

    // Future: implement a variant based on single-octet character type.
    // (presumably UTF-8 encoding)
    int char_size = sizeof(CHARTYPE);
    if (char_size == 1)
        return _Abandon(CommonParserStatusTypeNotSupported, parm);

    if (parm[0] != '+')
        return _Abandon(CommonParserStatusTypeUnexpectedCharacter, parm);

    int hd, iCodePoint = 0;
    for (int i = 1; i < 5; i++)
    {
        hd = _HexChar(parm[i]);
        if (hd < 0)
            return _Abandon(CommonParserStatusTypeInvalidArg, parm);
        iCodePoint <<= 4;
        iCodePoint += hd;
    }
    CHARTYPE sz[2];
    sz[0] = (CHARTYPE)iCodePoint;
    sz[1] = 0;

    // Flush what's not been sent yet
    CommonParserStatus eRet = _SendTextRunNotification(Run);
    if (!eRet.Succeeded())
        return eRet;

    // Send the unicode character
    // Set the text environment to indicate that the memory should be created and held by the text
    // run.
    Run.Contents() = sz;

    Run.OwnText(true);

    eRet = _SendTextRunNotification(Run);

    if (!eRet.Succeeded())
        return eRet;

    // Now, get ready to continue parsing.
    _here.SetStart(parm.End());

    return CommonParserStatusTypeOk;
}

CommonParserStatus
CommonParserMTextParseInstance::_Parse_W(CommonParserTextRunElement& Run)
{
    _here.Move(1);
    CommonParserStRange parm = _here;
    NUMBER nWid;
    if (!_GetNumber(parm, nWid).Succeeded())
        return _Abandon(CommonParserStatusTypeIncompleteString, parm);

    if (nWid == 1.0)
        Run.Transform().RemoveSameTypeTransform(CommonParserScaleTransformParticle(
            nWid, 1.0, CommonParserTransformParticleSemanticsWidth));
    else
        Run.Transform().ReplaceTransform(CommonParserScaleTransformParticle(
            nWid, 1.0, CommonParserTransformParticleSemanticsWidth));

    // Account for the trailing semicolon
    _here.SetStart(parm.Beyond());

    return CommonParserStatusTypeOk;
}

// Processes %< ... >% insertion, or complains if failed.
CommonParserStatus
CommonParserMTextParseInstance::_ParseFieldInsertion(CommonParserTextRunElement& Run)
{
    CommonParserStRange sField = _here;

    // Read forward looking for a semicolon, which is the
    // marker for end of parameter.
    for (;;)
    {
        // Make sure it's a complete reference; if we're over the buffer's EOS
        // then we've run out of characters and thus need to _Abandon.
        if (!sField.Last())
            return _Abandon(CommonParserStatusTypeUnmatchedConstruct, _here.Part(0, 2));

        // Okay, do we have the last two characters?
        if (sField.Last() == '%' && sField.Last(2) == '>')
            break;

        // No news; let's expand our look by one.
        sField.AddLength(1);
    }

    // We now have sField covering the full %< ... >% markup.
    // Let's break it down...

    // First, we need to send the unprocessed text run preceding the field:
    CommonParserStatus eRet = _SendTextRunNotification(Run);
    if (!eRet.Succeeded())
        return eRet;

    // Delete the markup from around the field.
    // Take two off the front for %<
    sField.MoveStart(2);
    // and  two off the end for >%
    sField.AddLength(-2);

    // Now, ask the Environment to expand this field.
    CommonParserStRange sResolution;
    CommonParserStatus eStatus =
        _env->References()->Resolve(MTEXT_PARSER_NAME, sField, sResolution, _env);

    // If the resolver figured it out, let's put what it figured out.
    // Otherwise, let's pass the field through unchanged.
    if (eStatus.Result() == CommonParserStatusTypeUnchanged || !eStatus.Succeeded())
        Run.Style().AddDelta(CommonParserReferenceExpansionStyleParticle(
            // the original is there
            CommonParserReferenceExpansionTypeSource));
    else
        Run.Style().AddDelta(CommonParserReferenceExpansionStyleParticle(
            // it's been swapped.
            CommonParserReferenceExpansionTypeExpanded));

    // Let's push through the reference, expanded or not.
    Run.Contents() = eStatus.Succeeded() ? sResolution : sField;
    eRet           = _SendTextRunNotification(Run);
    if (!eRet.Succeeded())
        return eRet;

    // Finally, let's reset for normal operation.
    Run.Style().AddDelta(CommonParserReferenceExpansionStyleParticle(
        CommonParserReferenceExpansionTypeNotReference));
    // Get past the >% in the markup.
    _here.Set(sField.Beyond(1), 1);

    return CommonParserStatusTypeOk;
}

// Handles all markup that isn't prefixed with a backslash
// Notably: nesting constructs { and }
//          newline character (immediate line break)
//          %% metacharacters
//          %< ... >% expansion notation
CommonParserStatus
CommonParserMTextParseInstance::_Parse_NonBackslash(CommonParserTextRunElement& Run)
{
    CommonParserStatus eRet;
    if (_here[0] == '{')
    {
        eRet = _SendStructureNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        // Enter nested context...
        _here.Move(1);
        eRet = _ParseContext(&Run);
        if (!eRet.Succeeded())
            return eRet;
    }
    else if (_here[0] == '}')
    {
        // Take care of unfinished business inside of this context
        eRet = _SendTextRunNotification(Run);
        if (!eRet.Succeeded())
            return eRet;

        if (Run.Structure()->Depth() == 0)
            return _Abandon(CommonParserStatusTypeUnexpectedCharacter, _here);

        // Special secret handshake to tell the outside
        // world that we're Okay, but we're exiting a nested
        // construct.
        return CommonParserStatusTypeDone;
    }
    else if (_here[0] == '\n')
    {
        Run.Location().SetSemantics(CommonParserSemanticTypeLine);
        Run.Location().AddOperation(CommonParserLineBreakLocationParticle());
        eRet = _SendNewlineNotification(Run);
        if (!eRet.Succeeded())
            return eRet;
    }
    else if (_here[0] == '%' && *_here.Beyond() == '%')
    { 
        // %%C %%D or %%P metachars?
        switch (_here.Start()[2])
        {
        case 'C':
            // Diameter
        case 'c':
            // Diameter symbol
            eRet = _SendMetacharacter(Run, /*MSG0*/ L"\x00D8");
            if (!eRet.Succeeded())
                return eRet;
            _here.Move(2);
            break;

        case 'D':
            // Degree
        case 'd':
            eRet = _SendMetacharacter(Run, /*MSG0*/ L"\x00b0");
            // Degree symbol
            if (!eRet.Succeeded())
                return eRet;
            _here.Move(2);
            break;

        case 'P':
            // Plus/Minus
        case 'p':
            eRet = _SendMetacharacter(Run, /*MSG0*/ L"\x00b1");
            // Plus/Minus symbol
            if (!eRet.Succeeded())
                return eRet;
            _here.Move(2);
            break;

        default:
            // nothing... just pass through the % sign.
            _UpdateContentsPointer(Run);
        }
    }
    else if (_here[0] == '%' && *_here.Beyond() == '<')
    {
        // %< ... >%?
        eRet = _ParseFieldInsertion(Run);
        if (!eRet.Succeeded())
            return eRet;
    }
    else
    {
        _UpdateContentsPointer(Run);
    }

    return CommonParserStatusTypeOk;
}

// Main workhorse.  Reentrant to handle nesting constructs { and }
CommonParserStatus
CommonParserMTextParseInstance::_ParseContext(CommonParserTextRunElement* pOuter)
{
    CommonParserStatus eRet = CommonParserStatusTypeOk;

    CommonParserTextRunElement Run;

    if (pOuter != nullptr)
        Run.InitFrom(pOuter);
    else
    {
        Run.InitFrom(_env);
        // Now, remove stuff from the style that might be confusing/contradictory.
        Run.Style().RemoveFromDescription(CommonParserStyleParticleTypeSize);
    }

    // for diagnostics, if necessary.
    const CHARTYPE* pEntryPosition = _here.Start();

    bool needSemicolon = true;

    while (_here[0])
    {
        if (_here[0] == '\\')
        {
            // Transition to Markup
            eRet = _SendTextRunNotification(Run);
            if (!eRet.Succeeded())
                return eRet;

            // Advance past the slash to the opcode.
            _here.Move(1);

            needSemicolon = true;

            switch (_here[0])
            {
            case '\0':
                // End of string in the middle of a backslash sequence?  That ain't good.
                return _Abandon(CommonParserStatusTypeIncompleteString,
                    CommonParserStRange(_here.Start() - 1, 1));
            case 'A':
                // \A#; -- Vertical alignment. # = 0, 1, 2
                eRet = _Parse_A(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;
            case 'C':
                // \C##;  -- ACI color
                eRet = _Parse_C(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;
            case 'c':
                // \c###; -- RGB color (in decimal) (R + G<<8 + B<<16)
                eRet = _Parse_c(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;
            case 'F':
                //  \ftxt,bigfontfile|c0; -- SHX Font change: CJK font, charset
                eRet = _Parse_F(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;
            case 'f':
                //  \fArial|b0|i0|p34|c0; -- Font change:
                //  typeface,bold,italic,pitchfam,charset
                eRet = _Parse_f(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case 'H':
                // either \H###; or \H###x; the latter being a relative scale: \H2x; => twice
                // size.
                eRet = _Parse_H(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case 'L':
                // \L (no extra syntax) -- begin Underline.
                eRet = _Parse_L(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'l':
                // \l (no extra syntax) -- end Underline.
                eRet = _Parse_l(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'N':
                // \N (no extra syntax) -- Next Column
                eRet = _Parse_N(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'O':
                // \O (no extra syntax) -- begin Overline.
                eRet = _Parse_O(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'o':
                // \o (no extra syntax) -- end Overline.
                eRet = _Parse_o(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'P':
                // \P (no extra syntax) -- end Paragraph.
                Run.Location().AddSemantic(CommonParserSemanticTypeParagraph);
                // Go to the next line.
                Run.Location().AddOperation(CommonParserLineBreakLocationParticle());
                eRet = _SendNewlineNotification(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'p':
                // \p
                eRet = _Parse_p(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'Q':
                // \Q##; -- Set obliquing angle
                eRet = _Parse_Q(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case 'S':
                // \Snum/denom;  (where / could be ^ or %, too)  -- Stack fraction.
                eRet = _Parse_S(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case 'T':
                // \T##; -- Tracking factor.
                eRet = _Parse_T(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case 'U':
                // \U+xxxx -- Unicode codepoint.
                eRet = _Parse_U(Run);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case 'W':
                // \W##; -- Width factor: horizontally stretch text by multiplier given.
                eRet = _Parse_W(Run);
                if (!eRet.Succeeded())
                    return eRet;
                break;

            case '~':
                // \~ -- Non-breaking space
                // non-breaking space
                eRet = _SendMetacharacter(Run, /*MSG0*/ L"\x00a0");
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;

            case '\\':
            case '{':
            case '}':
                // The escaped character, \, {, or }, is what we
                // send (that is, we omit the esaping \ in the
                // string.
                eRet = _SendMetacharacter(Run, _here);
                if (!eRet.Succeeded())
                    return eRet;
                needSemicolon = false;
                break;
            default:
            {
                // Unrecognized tag
                // Parse as it is not a tag
                // Move back to the backslash
                _here.Move(-1);

                needSemicolon = false;

                eRet = _Parse_NonBackslash(Run);

                // Our queue to exit from a nested context:
                if (eRet == CommonParserStatusTypeDone)
                    return CommonParserStatusTypeOk;

                if (!eRet.Succeeded())
                    return eRet;
            }
            }
            if (needSemicolon)
                while (_here[0] != '\0' && _here[0] != ';')
                    _here.Move(1);
            if (_here[0] == '\0')
                return CommonParserStatusTypeOk;
        }
        else
        {
            eRet = _Parse_NonBackslash(Run);

            // Our queue to exit from a nested context:
            if (eRet == CommonParserStatusTypeDone)
                return CommonParserStatusTypeOk;

            if (!eRet.Succeeded())
                return eRet;
        }
        // Advance to the next character.
        if (_here[0] != '\0')
            _here.Move(1);
    }

    // Process whatever's left over...
    eRet = _SendNewlineNotification(Run);

    // Have we prematurely reached the end of string?
    if (Run.Structure()->Depth() != 0)
        return _Abandon(CommonParserStatusTypeUnmatchedConstruct,
            CommonParserStRange(pEntryPosition - 1, 1));

    return eRet;
}

CommonParserStatus 
CommonParserMTextParseInstance::_ParseForParameter(CommonParserStRange& sParam)
{
    // Read forward looking for a semicolon, which is the
    // marker for end of parameter.
    while (sParam.Last() && sParam.Last() != ';')
    {
        // Looking for runaway command: do we find another
        // backslash inside?
        if (sParam.Last() == '\\')
        {
            sParam.AddLength(1);
            // It's only a problem if the following character
            // isn't also a backslash (that is, escaped backslash)
            if (sParam.Last(1) != '\\')
            {
                sParam.AddLength(-1);
                break;
            }
        }
        sParam.AddLength(1);
    }

    // If we're out of the loop and we're not over
    // a semicolon, then back off to not include that
    // unexpected character, and indicate that we think
    // the parameter isn't there.
    if (sParam.Last() != ';')
    {
        sParam.AddLength(-1);
        return _Abandon(CommonParserStatusTypeUnexpectedCharacter, sParam);
    }

    // Come to think of it, we shouldn't include the semicolon, either.
    sParam.AddLength(-1);
    return CommonParserStatusTypeOk;
}

// Gets an NUMBER from the parameter string indicated.
CommonParserStatus
CommonParserMTextParseInstance::_GetNumber(
    CommonParserStRange& sNumber,
    NUMBER& nNumber)
{
    if (_ParseForParameter(sNumber).Succeeded())
    {
        nNumber = (NUMBER)PXR_INTERNAL_NS::wtof(sNumber.Start());
        return CommonParserStatusTypeOk;
    }
    return CommonParserStatusTypeNotPresent;
}

CommonParserStatus 
CommonParserMTextParseInstance::_GetLargeNumber(
    CommonParserStRange& sNumber,
    double& nNumber)
{
    if (_ParseForParameter(sNumber).Succeeded())
    {
        nNumber = PXR_INTERNAL_NS::wtof(sNumber.Start());
        return CommonParserStatusTypeOk;
    }
    return CommonParserStatusTypeNotPresent;
}

// Does the dirty work of talking TextRun to the sink.
CommonParserStatus
CommonParserMTextParseInstance::_SendTextRunNotification(CommonParserTextRunElement& Run)
{
    if (!Run.IsReset())
    {
        CommonParserStatus eRet = _env->Sink()->TextRun(&Run, _env);
        if (!eRet.Succeeded())
            eRet = _Abandon(eRet.Result(), Run.Contents());
        Run.Reset();
        return eRet;
    }
    Run.Style().Reset();
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMTextParseInstance::_SendNewlineNotification(CommonParserTextRunElement& Run)
{
    CommonParserStatus eRet = _env->Sink()->TextRun(&Run, _env);
    if (!eRet.Succeeded())
        eRet = _Abandon(eRet.Result(), Run.Contents());
    Run.Reset();
    return eRet;
}

CommonParserStatus 
CommonParserMTextParseInstance::_SendStructureNotification(CommonParserTextRunElement& Run)
{
    CommonParserStatus eRet = _env->Sink()->TextRun(&Run, _env);
    if (!eRet.Succeeded())
        eRet = _Abandon(eRet.Result(), Run.Contents());
    Run.Reset();
    return eRet;
}

// Does the handiwork of dispatching an _Abandon notification.
CommonParserStatus 
CommonParserMTextParseInstance::_Abandon(
    CommonParserStatusType eReason,
    const CommonParserStRange sPos)
{
    CommonParserAbandonmentElement a(eReason);
    a.SetMarkup(_markup);

    // Let's backtrack to find the start of the line (or buffer)
    const CHARTYPE* pLineStart = sPos.Start();
    while (pLineStart != nullptr && pLineStart > _markup.Start())
    {
        if (*pLineStart == '\n')
        {
            // get back on our side of the line.
            pLineStart++;
            break;
        }
        pLineStart--;
    }

    CommonParserStRange sContext(pLineStart, sPos.End());
    // And go forward to find end of line (or buffer)
    while (pLineStart != nullptr && sContext.Last() != '\0' && sContext.Last() != '\n')
        sContext.AddLength(1);
    sContext.AddLength(-1);

    a.SetContext(sContext);
    a.SetPosition(sPos);
    _env->Sink()->Abandon(&a, _env);
    return CommonParserStatusTypeAbandoned;
}

int 
CommonParserMTextParseInstance::RgbToAci(CommonParserColor rgb)
{
    long l = static_cast<long>(rgb.LongARGB());
    // no alpha? force alpha to maximum.
    if (!(l & ATOM_COLOR_A_BITS))
        l |= ATOM_COLOR_A_BITS;

    for (size_t i = 0; i < sizeof(_aciColorTable) / sizeof(long); i++)
    {
        if (l == _aciColorTable[i])
            return int(i);
    }
    // not found.
    return -1;
}

// Just minimizes cartesian distance (squared) to
int 
CommonParserMTextParseInstance::RgbToNearestAci(CommonParserColor rgb)
{
    // huge;
    double dDistSquaredToNearest = 1e308;
    int iIndexNearest            = -1;

    for (size_t i = 0; i < sizeof(_aciColorTable) / sizeof(long); i++)
    {
        long lAci = _aciColorTable[i];
        long lRgb = static_cast<long>(rgb.LongARGB());

        // B
        double dDist = ((lAci & 0xFF) - (lRgb & 0xFF)) * ((lAci & 0xFF) - (lRgb & 0xFF));

        // G
        lAci >>= 8;
        lRgb >>= 8;
        dDist += ((lAci & 0xFF) - (lRgb & 0xFF)) * ((lAci & 0xFF) - (lRgb & 0xFF));

        // R
        lAci >>= 8;
        lRgb >>= 8;
        dDist += ((lAci & 0xFF) - (lRgb & 0xFF)) * ((lAci & 0xFF) - (lRgb & 0xFF));

        // A
        lAci >>= 8;
        lRgb >>= 8;
        dDist += ((lAci & 0xFF) - (lRgb & 0xFF)) * ((lAci & 0xFF) - (lRgb & 0xFF));

        if (dDist < dDistSquaredToNearest)
        {
            dDistSquaredToNearest = dDist;
            iIndexNearest         = (int)i;
            // Hmm, exact match; don't get any closer than this.
            if (dDist == 0)
                break;
        }
    }

    // not found.
    return iIndexNearest;
}

// generated using data from http://bitsy.sub-atomic.com/~moses/acadcolors.html
long CommonParserMTextParseInstance::_aciColorTable[257] = {
    // aaRRGGBB  <- note that it's RGB, not BGR.
    (long)0xff000000, // 0
    (long)0xffFF0000, // 1
    (long)0xffFFFF00, // 2
    (long)0xff00FF00, // 3
    (long)0xff00FFFF, // 4
    (long)0xff0000FF, // 5
    (long)0xffFF00FF, // 6
    (long)0xffFFFFFF, // 7
    (long)0xff414141, // 8
    (long)0xff808080, // 9
    (long)0xffFF0000, // 10
    (long)0xffFFAAAA, // 11
    (long)0xffBD0000, // 12
    (long)0xffBD7E7E, // 13
    (long)0xff810000, // 14
    (long)0xff815656, // 15
    (long)0xff680000, // 16
    (long)0xff684545, // 17
    (long)0xff4F0000, // 18
    (long)0xff4F3535, // 19
    (long)0xffFF3F00, // 20
    (long)0xffFFBFAA, // 21
    (long)0xffBD2E00, // 22
    (long)0xffBD8D7E, // 23
    (long)0xff811F00, // 24
    (long)0xff816056, // 25
    (long)0xff681900, // 26
    (long)0xff684E45, // 27
    (long)0xff4F1300, // 28
    (long)0xff4F3B35, // 29
    (long)0xffFF7F00, // 30
    (long)0xffFFD4AA, // 31
    (long)0xffBD5E00, // 32
    (long)0xffBD9D7E, // 33
    (long)0xff814000, // 34
    (long)0xff816B56, // 35
    (long)0xff683400, // 36
    (long)0xff685645, // 37
    (long)0xff4F2700, // 38
    (long)0xff4F4235, // 39
    (long)0xffFFBF00, // 40
    (long)0xffFFEAAA, // 41
    (long)0xffBD8D00, // 42
    (long)0xffBDAD7E, // 43
    (long)0xff816000, // 44
    (long)0xff817656, // 45
    (long)0xff684E00, // 46
    (long)0xff685F45, // 47
    (long)0xff4F3B00, // 48
    (long)0xff4F4935, // 49
    (long)0xffFFFF00, // 50
    (long)0xffFFFFAA, // 51
    (long)0xffBDBD00, // 52
    (long)0xffBDBD7E, // 53
    (long)0xff818100, // 54
    (long)0xff818156, // 55
    (long)0xff686800, // 56
    (long)0xff686845, // 57
    (long)0xff4F4F00, // 58
    (long)0xff4F4F35, // 59
    (long)0xffBFFF00, // 60
    (long)0xffEAFFAA, // 61
    (long)0xff8DBD00, // 62
    (long)0xffADBD7E, // 63
    (long)0xff608100, // 64
    (long)0xff768156, // 65
    (long)0xff4E6800, // 66
    (long)0xff5F6845, // 67
    (long)0xff3B4F00, // 68
    (long)0xff494F35, // 69
    (long)0xff7FFF00, // 70
    (long)0xffD4FFAA, // 71
    (long)0xff5EBD00, // 72
    (long)0xff9DBD7E, // 73
    (long)0xff408100, // 74
    (long)0xff6B8156, // 75
    (long)0xff346800, // 76
    (long)0xff566845, // 77
    (long)0xff274F00, // 78
    (long)0xff424F35, // 79
    (long)0xff3FFF00, // 80
    (long)0xffBFFFAA, // 81
    (long)0xff2EBD00, // 82
    (long)0xff8DBD7E, // 83
    (long)0xff1F8100, // 84
    (long)0xff608156, // 85
    (long)0xff196800, // 86
    (long)0xff4E6845, // 87
    (long)0xff134F00, // 88
    (long)0xff3B4F35, // 89
    (long)0xff00FF00, // 90
    (long)0xffAAFFAA, // 91
    (long)0xff00BD00, // 92
    (long)0xff7EBD7E, // 93
    (long)0xff008100, // 94
    (long)0xff568156, // 95
    (long)0xff006800, // 96
    (long)0xff456845, // 97
    (long)0xff004F00, // 98
    (long)0xff354F35, // 99
    (long)0xff00FF3F, // 100
    (long)0xffAAFFBF, // 101
    (long)0xff00BD2E, // 102
    (long)0xff7EBD8D, // 103
    (long)0xff00811F, // 104
    (long)0xff568160, // 105
    (long)0xff006819, // 106
    (long)0xff45684E, // 107
    (long)0xff004F13, // 108
    (long)0xff354F3B, // 109
    (long)0xff00FF7F, // 110
    (long)0xffAAFFD4, // 111
    (long)0xff00BD5E, // 112
    (long)0xff7EBD9D, // 113
    (long)0xff008140, // 114
    (long)0xff56816B, // 115
    (long)0xff006834, // 116
    (long)0xff456856, // 117
    (long)0xff004F27, // 118
    (long)0xff354F42, // 119
    (long)0xff00FFBF, // 120
    (long)0xffAAFFEA, // 121
    (long)0xff00BD8D, // 122
    (long)0xff7EBDAD, // 123
    (long)0xff008160, // 124
    (long)0xff568176, // 125
    (long)0xff00684E, // 126
    (long)0xff45685F, // 127
    (long)0xff004F3B, // 128
    (long)0xff354F49, // 129
    (long)0xff00FFFF, // 130
    (long)0xffAAFFFF, // 131
    (long)0xff00BDBD, // 132
    (long)0xff7EBDBD, // 133
    (long)0xff008181, // 134
    (long)0xff568181, // 135
    (long)0xff006868, // 136
    (long)0xff456868, // 137
    (long)0xff004F4F, // 138
    (long)0xff354F4F, // 139
    (long)0xff00BFFF, // 140
    (long)0xffAAEAFF, // 141
    (long)0xff008DBD, // 142
    (long)0xff7EADBD, // 143
    (long)0xff006081, // 144
    (long)0xff567681, // 145
    (long)0xff004E68, // 146
    (long)0xff455F68, // 147
    (long)0xff003B4F, // 148
    (long)0xff35494F, // 149
    (long)0xff007FFF, // 150
    (long)0xffAAD4FF, // 151
    (long)0xff005EBD, // 152
    (long)0xff7E9DBD, // 153
    (long)0xff004081, // 154
    (long)0xff566B81, // 155
    (long)0xff003468, // 156
    (long)0xff455668, // 157
    (long)0xff00274F, // 158
    (long)0xff35424F, // 159
    (long)0xff003FFF, // 160
    (long)0xffAABFFF, // 161
    (long)0xff002EBD, // 162
    (long)0xff7E8DBD, // 163
    (long)0xff001F81, // 164
    (long)0xff566081, // 165
    (long)0xff001968, // 166
    (long)0xff454E68, // 167
    (long)0xff00134F, // 168
    (long)0xff353B4F, // 169
    (long)0xff0000FF, // 170
    (long)0xffAAAAFF, // 171
    (long)0xff0000BD, // 172
    (long)0xff7E7EBD, // 173
    (long)0xff000081, // 174
    (long)0xff565681, // 175
    (long)0xff000068, // 176
    (long)0xff454568, // 177
    (long)0xff00004F, // 178
    (long)0xff35354F, // 179
    (long)0xff3F00FF, // 180
    (long)0xffBFAAFF, // 181
    (long)0xff2E00BD, // 182
    (long)0xff8D7EBD, // 183
    (long)0xff1F0081, // 184
    (long)0xff605681, // 185
    (long)0xff190068, // 186
    (long)0xff4E4568, // 187
    (long)0xff13004F, // 188
    (long)0xff3B354F, // 189
    (long)0xff7F00FF, // 190
    (long)0xffD4AAFF, // 191
    (long)0xff5E00BD, // 192
    (long)0xff9D7EBD, // 193
    (long)0xff400081, // 194
    (long)0xff6B5681, // 195
    (long)0xff340068, // 196
    (long)0xff564568, // 197
    (long)0xff27004F, // 198
    (long)0xff42354F, // 199
    (long)0xffBF00FF, // 200
    (long)0xffEAAAFF, // 201
    (long)0xff8D00BD, // 202
    (long)0xffAD7EBD, // 203
    (long)0xff600081, // 204
    (long)0xff765681, // 205
    (long)0xff4E0068, // 206
    (long)0xff5F4568, // 207
    (long)0xff3B004F, // 208
    (long)0xff49354F, // 209
    (long)0xffFF00FF, // 210
    (long)0xffFFAAFF, // 211
    (long)0xffBD00BD, // 212
    (long)0xffBD7EBD, // 213
    (long)0xff810081, // 214
    (long)0xff815681, // 215
    (long)0xff680068, // 216
    (long)0xff684568, // 217
    (long)0xff4F004F, // 218
    (long)0xff4F354F, // 219
    (long)0xffFF00BF, // 220
    (long)0xffFFAAEA, // 221
    (long)0xffBD008D, // 222
    (long)0xffBD7EAD, // 223
    (long)0xff810060, // 224
    (long)0xff815676, // 225
    (long)0xff68004E, // 226
    (long)0xff68455F, // 227
    (long)0xff4F003B, // 228
    (long)0xff4F3549, // 229
    (long)0xffFF007F, // 230
    (long)0xffFFAAD4, // 231
    (long)0xffBD005E, // 232
    (long)0xffBD7E9D, // 233
    (long)0xff810040, // 234
    (long)0xff81566B, // 235
    (long)0xff680034, // 236
    (long)0xff684556, // 237
    (long)0xff4F0027, // 238
    (long)0xff4F3542, // 239
    (long)0xffFF003F, // 240
    (long)0xffFFAABF, // 241
    (long)0xffBD002E, // 242
    (long)0xffBD7E8D, // 243
    (long)0xff81001F, // 244
    (long)0xff815660, // 245
    (long)0xff680019, // 246
    (long)0xff68454E, // 247
    (long)0xff4F0013, // 248
    (long)0xff4F353B, // 249
    (long)0xff333333, // 250
    (long)0xff505050, // 251
    (long)0xff696969, // 252
    (long)0xff828282, // 253
    (long)0xffBEBEBE, // 254
    (long)0xffFFFFFF, // 255
    (long)0xff000000, // 256
};

/**********************************************************************
 *
 *  PARSER GENERATOR and its singleton instance.
 *
 **********************************************************************/

CommonParserMTextGenerator::CommonParserMTextGenerator()
{
    _universe = BigBang();
    _universe->Register(this);
    _endRegister = false;
}

CommonParserMTextGenerator::~CommonParserMTextGenerator()
{
    if (_endRegister == false)
    {
        _universe->Unregister(this);
        _endRegister = true;
    }
}

// The name of the markup this parser represents
// such as "SVG" or "RTF" or ...
const 
CommonParserStRange CommonParserMTextGenerator::Name() const
{
    // This is an internal string, not subject to localization.
    return MTEXT_PARSER_NAME;
}

// Documentation of the parser/generator (for
// version reporting, etc.)  A human-readable string.
const 
CommonParserStRange CommonParserMTextGenerator::Description() const
{
    return L"MTEXT CommonParserParser v1.0";
}

// Creates an instance to a new Parser
CommonParserStatus 
CommonParserMTextGenerator::Create(CommonParserParser** ppNewParser)
{
    if (ppNewParser == nullptr)
        return CommonParserStatusTypeInvalidArg;

    *ppNewParser = new CommonParserMTextParser(this);

    return CommonParserStatusTypeOk;
}

// Takes a pointer to an existing parser and destroys it.
CommonParserStatus 
CommonParserMTextGenerator::Destroy(CommonParserParser* pOldParser)
{
    if (pOldParser == nullptr)
        return CommonParserStatusTypeInvalidArg;

    // Since CommonParserParser doesn't define virtual dtor, MSVC 2019 report an warning. Use static_cast to
    // suppress the warning.
    delete (static_cast<CommonParserMTextParser*>(pOldParser));
    return CommonParserStatusTypeOk;
}

bool 
CommonParserMTextGenerator::HasSink() const
{
    return false;
}

// Creates an instance to a new Parser
CommonParserStatus 
CommonParserMTextGenerator::Create(CommonParserSink** ppNewSink)
{
    if (ppNewSink == nullptr)
        return CommonParserStatusTypeInvalidArg;

    return CommonParserStatusTypeOk;
}

// Takes a pointer to an existing parser and destroys it.
CommonParserStatus 
CommonParserMTextGenerator::Destroy(CommonParserSink* pOldSink)
{
    if (pOldSink == nullptr)
        return CommonParserStatusTypeInvalidArg;

    return CommonParserStatusTypeOk;
}

CommonParserStatus
CommonParserMTextGenerator::RegisterNull()
{
    _endRegister = true;
    return CommonParserStatusTypeOk;
}

// Instance the generator, which does
// all the self-registration with the CommonParserUniverse.
// well, until it's dead.
CommonParserMTextGenerator LongLiveMText;

CommonParserUniverse* 
getUniverse()
{
    return LongLiveMText._universe;
}

PXR_NAMESPACE_CLOSE_SCOPE

