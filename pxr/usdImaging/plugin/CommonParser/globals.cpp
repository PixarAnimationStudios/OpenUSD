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


#include "globals.h"
#include <stdlib.h>
#define _CRT_ATOF_DEFINED // needed to get rid of a stupid MS error concerning unneeded _locale_t
#include <math.h>

PXR_NAMESPACE_OPEN_SCOPE
//
//  Base CommonParserStyleParticle implementation.
//

void 
CommonParserParticle::_Append(const CommonParserParticle* pEnd)
{
    if(this->_pNext)
        this->_pNext->_Append(pEnd);
    else
        this->_pNext = const_cast<CommonParserParticle*>(pEnd);
}


CommonParserStatus 
CommonParserParticle::_AddToList(CommonParserParticle*& pList, 
                                 const CommonParserParticle& oParticle)
{

    CommonParserParticle* pParticle = oParticle.Clone();
    if(!pParticle)
        return CommonParserStatusTypeNoMemory;

    // Start a new list, or add to the end.
    if(pList == NULL)
        pList = pParticle;
    else 
        pList->_Append(pParticle);

    return CommonParserStatusTypeOk;
}



// Dummy assignment; do nothing at this level.
CommonParserStyleParticle& 
CommonParserStyleParticle::operator= (const CommonParserStyleParticle&) 
{
    return *this;
}

// Derivations implemented via macro.
ATOM_STYLE_PARTICLE_IMPL(Typeface,            CommonParserStRange)
ATOM_STYLE_PARTICLE_IMPL(IsSHX,               bool)
ATOM_STYLE_PARTICLE_IMPL(AltTypefaces,        CommonParserStRange)           // Alternate typefaces, to be tried if Typeface isn't found.
ATOM_STYLE_PARTICLE_IMPL(PitchFamily,         CommonParserPitchFamilyType) // Font-matching heuristics if font isn't known.
ATOM_STYLE_PARTICLE_IMPL(CharacterSet,        int)               // Font-matching heuristics for which character set.
ATOM_STYLE_PARTICLE_IMPL(Size,                CommonParserMeasure)
ATOM_STYLE_PARTICLE_IMPL(CapSize,             CommonParserMeasure)
ATOM_STYLE_PARTICLE_IMPL(FontWeight,          CommonParserFontWeightType)
ATOM_STYLE_PARTICLE_IMPL(Italic,              bool)              // Italic variation of font?
ATOM_STYLE_PARTICLE_IMPL(FillColor,           CommonParserColor)
ATOM_STYLE_PARTICLE_IMPL(StrokeColor,         CommonParserColor)
ATOM_STYLE_PARTICLE_IMPL(StrokeWeight,        CommonParserMeasure)           // a line weight measure
ATOM_STYLE_PARTICLE_IMPL(StrokeBehind,        bool)              // for SFA vs SVG: does the stroke get rendered behind fill?
ATOM_STYLE_PARTICLE_IMPL(Underline,           CommonParserTextLineType)    // None/Single/Double/Dotted.  Sink may support only
ATOM_STYLE_PARTICLE_IMPL(Overline,            CommonParserTextLineType)    // ..                          a limited subset
ATOM_STYLE_PARTICLE_IMPL(Strikethrough,       CommonParserTextLineType)    // ..                          and modify rendering.
ATOM_STYLE_PARTICLE_IMPL(CaseShift,           CommonParserCaseShiftType)   // Small Caps, etc.
ATOM_STYLE_PARTICLE_IMPL(TrackingAugment,     CommonParserMeasure)
ATOM_STYLE_PARTICLE_IMPL(Justification,       CommonParserJustificationType)
ATOM_STYLE_PARTICLE_IMPL(VerticalAlignment,   CommonParserVerticalAlignmentType)
ATOM_STYLE_PARTICLE_IMPL(HorizontalAlignment, CommonParserHorizontalAlignmentType)
ATOM_STYLE_PARTICLE_IMPL(AdvanceAlignment,    CommonParserMeasure)
ATOM_STYLE_PARTICLE_IMPL(ReferenceExpansion,  CommonParserReferenceExpansionType)// Reference Expansion
ATOM_STYLE_PARTICLE_IMPL(AfterPara,           CommonParserMeasure)           // Multi-line support: extra space after a "paragraph" unit.
ATOM_STYLE_PARTICLE_IMPL(LineHeight,          CommonParserLineHeightMeasure)           // Multi-line support: distance from base-line to base-line
ATOM_STYLE_PARTICLE_IMPL(BeforePara,          CommonParserMeasure)           // Multi-line support: extra space before a "paragraph" unit
ATOM_STYLE_PARTICLE_IMPL(BackgroundColor,     CommonParserColor)             // CommonParserColor (with Alpha) of the background.

//
//  Base CommonParserTransformParticle implementation.
//

// Derivations (except for each SetMatrix method)
// implemented by macro
TRANSFORM_PARTICLE_XY_IMPL(Scale,NUMBER)
void 
CommonParserScaleTransformParticle::SetMatrix(CommonParserMatrix& m) const
{
    m[0][0] = this->_x;
    m[1][1] = this->_y;
}

bool 
CommonParserScaleTransformParticle::IsIdentity() const
{
    return this->_x == 1 && this->_y == 1;
}

TRANSFORM_PARTICLE_XY_IMPL(Skew,CommonParserRadialMeasure)
void 
CommonParserSkewTransformParticle::SetMatrix(CommonParserMatrix& m) const
{
    m[0][1] = tan(this->_x.Radians());
    m[1][0] = tan(this->_y.Radians());
}

bool 
CommonParserSkewTransformParticle::IsIdentity() const
{
    return this->_x.Radians() == 0 && this->_y.Radians() == 0;
}

TRANSFORM_PARTICLE_XY_IMPL(Translation,NUMBER)
void 
CommonParserTranslationTransformParticle::SetMatrix(CommonParserMatrix& m) const
{
    m[0][2] = this->_x;
    m[1][2] = this->_y;
}

bool 
CommonParserTranslationTransformParticle::IsIdentity() const
{
    return this->_x == 0 && this->_y == 0;
}

TRANSFORM_PARTICLE_IMPL(Rotation, CommonParserRadialMeasure)
void 
CommonParserRotationTransformParticle::SetMatrix(CommonParserMatrix& m) const
{
    // Assumes this->_v is in radians.
    NUMBER c = cos(this->_v.Radians());
    NUMBER s = sin(this->_v.Radians());

    m[0][0] = c;   m[0][1] = -s;
    m[1][0] = s;   m[1][1] =  c;
}

bool 
CommonParserRotationTransformParticle::IsIdentity() const
{
    return this->_v.Radians() == 0;
}

TRANSFORM_PARTICLE_IMPL(Arbitrary, CommonParserMatrix)

void 
CommonParserArbitraryTransformParticle::SetMatrix(CommonParserMatrix& m) const
{
    m = this->_v;
}

bool 
CommonParserArbitraryTransformParticle::IsIdentity() const
{
    NUMBER* pMat = (NUMBER*)this->_v;
    return pMat[e_00] == 1 && pMat[e_01] == 0 && pMat[e_02] == 0
        && pMat[e_10] == 0 && pMat[e_11] == 1 && pMat[e_12] == 0
        && pMat[e_20] == 0 && pMat[e_21] == 0 && pMat[e_22] == 1;
}

/// ==================================================


ATOM_PARTICLE_IMPL(Location,Bookmark)
CommonParserBookmarkLocationParticle::CommonParserBookmarkLocationParticle(
    int iIndex)
    : _iIndex(iIndex)
{
}

CommonParserLocationParticle*
CommonParserBookmarkLocationParticle::Clone() const
{
    return new CommonParserBookmarkLocationParticle(_iIndex);
}

CommonParserLocationParticle&
CommonParserBookmarkLocationParticle::operator= (
    const CommonParserLocationParticle& o)      
{
    if(Type() == o.Type()) {
        this->_iIndex = ((const CommonParserBookmarkLocationParticle&)o)._iIndex;
    }

    return *this;
}

int
CommonParserBookmarkLocationParticle::Index() const
{
    return _iIndex;
}

bool
CommonParserBookmarkLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    if(Type() != o.Type())
        return false;
    else
        return this->_iIndex == ((CommonParserBookmarkLocationParticle&)o)._iIndex;
}

ATOM_PARTICLE_IMPL(Location,ReturnToBookmark)
CommonParserReturnToBookmarkLocationParticle::CommonParserReturnToBookmarkLocationParticle(
    int iIndex)
    : _iIndex(iIndex)
{
}

CommonParserLocationParticle*
CommonParserReturnToBookmarkLocationParticle::Clone() const
{
    return new CommonParserReturnToBookmarkLocationParticle(_iIndex);
}

CommonParserLocationParticle&
CommonParserReturnToBookmarkLocationParticle::operator= (
    const CommonParserLocationParticle& o)      
{
    if(Type() == o.Type()) {
        this->_iIndex = ((const CommonParserReturnToBookmarkLocationParticle&)o)._iIndex;
    }

    return *this;
}

bool
CommonParserReturnToBookmarkLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    if(Type() != o.Type())
        return false;
    else
        return this->_iIndex == ((CommonParserReturnToBookmarkLocationParticle&)o)._iIndex;
}

int 
CommonParserReturnToBookmarkLocationParticle::Index() const
{
    return _iIndex;
}


ATOM_PARTICLE_IMPL(Location,ConditionalReturnToBookmark)
CommonParserConditionalReturnToBookmarkLocationParticle::CommonParserConditionalReturnToBookmarkLocationParticle(
    int iIndex, 
    CommonParserConditionType eType)
    : _iIndex(iIndex)
    , _eCondition(eType)
{
}

CommonParserLocationParticle* 
CommonParserConditionalReturnToBookmarkLocationParticle::Clone() const
{
    return new CommonParserConditionalReturnToBookmarkLocationParticle(Index(),_eCondition);
}

CommonParserLocationParticle& 
CommonParserConditionalReturnToBookmarkLocationParticle::operator= (
    const CommonParserLocationParticle& o)
{
    if(Type() == o.Type()) {
        this->_iIndex     = ((const CommonParserConditionalReturnToBookmarkLocationParticle&)o)._iIndex;
        this->_eCondition = ((const CommonParserConditionalReturnToBookmarkLocationParticle&)o)._eCondition;
    }
    return *this;
}

bool 
CommonParserConditionalReturnToBookmarkLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    if(Type() != o.Type())
        return false;
    else
        return (this->_iIndex     == ((CommonParserConditionalReturnToBookmarkLocationParticle&)o)._iIndex
            &&  this->_eCondition == ((CommonParserConditionalReturnToBookmarkLocationParticle&)o)._eCondition);
}

int 
CommonParserConditionalReturnToBookmarkLocationParticle::Index() const
{
    return _iIndex;
}

CommonParserConditionType 
CommonParserConditionalReturnToBookmarkLocationParticle::Condition() const
{
    return this->_eCondition;
}

ATOM_PARTICLE_IMPL(Location,Relative)
CommonParserRelativeLocationParticle::CommonParserRelativeLocationParticle(
    CommonParserMeasure mAdvance, 
    CommonParserMeasure mRise)
    : _mAdvance(mAdvance)
    , _mRise(mRise)
    , _semantic(CommonParserSubSemanticTypeUndefined)
{
}

CommonParserLocationParticle*
CommonParserRelativeLocationParticle::Clone() const
{
    return new CommonParserRelativeLocationParticle(_mAdvance,_mRise);
}

CommonParserLocationParticle&
CommonParserRelativeLocationParticle::operator= (
    const CommonParserLocationParticle& o)      
{
    if(Type() == o.Type()) {
          this->_mAdvance = ((const CommonParserRelativeLocationParticle&)o)._mAdvance;
          this->_mRise    = ((const CommonParserRelativeLocationParticle&)o)._mRise;
    }

    return *this;
}

bool
CommonParserRelativeLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    if(Type() != o.Type())
        return false;
    else
        return this->_mAdvance == ((CommonParserRelativeLocationParticle&)o)._mAdvance 
            && this->_mRise    == ((CommonParserRelativeLocationParticle&)o)._mRise;
}

CommonParserMeasure 
CommonParserRelativeLocationParticle::Advance() const
{
    return _mAdvance;
}

CommonParserMeasure 
CommonParserRelativeLocationParticle::Rise() const
{
    return _mRise;
}

ATOM_PARTICLE_IMPL(Location,Point)
CommonParserPointLocationParticle::CommonParserPointLocationParticle(
    NUMBER x, 
    NUMBER y)
    : _x(x)
    , _y(y)
{
}

CommonParserLocationParticle*
CommonParserPointLocationParticle::Clone() const
{
    return new CommonParserPointLocationParticle(_x,_y);
}

CommonParserLocationParticle&
CommonParserPointLocationParticle::operator= (
    const CommonParserLocationParticle& o)      
{
    if(Type() == o.Type()) {
          this->_x = ((const CommonParserPointLocationParticle&)o)._x;
          this->_x = ((const CommonParserPointLocationParticle&)o)._y;
    }

    return *this;
}

bool
CommonParserPointLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    if(Type() != o.Type())
        return false;
    else
        return this->_x == ((CommonParserPointLocationParticle&)o)._x 
            && this->_y == ((CommonParserPointLocationParticle&)o)._y;
}

NUMBER 
CommonParserPointLocationParticle::X() const
{
    return this->_x;

}

NUMBER 
CommonParserPointLocationParticle::Y() const
{
    return this->_y;
}

ATOM_PARTICLE_IMPL(Location,LineBreak)
CommonParserLineBreakLocationParticle::CommonParserLineBreakLocationParticle()
{
}

CommonParserLocationParticle*
CommonParserLineBreakLocationParticle::Clone() const
{
    return new CommonParserLineBreakLocationParticle();
}

CommonParserLocationParticle&
CommonParserLineBreakLocationParticle::operator= (
    const CommonParserLocationParticle& o)      
{
    return *this;
}

bool
CommonParserLineBreakLocationParticle::operator==(
    const CommonParserLocationParticle& o) const
{
    return (Type() == o.Type());
}

void 
CommonParserMatrix::operator*= (const NUMBER* o)
{
    NUMBER m[9];

    m[e_00] = _pElements[e_00]*o[e_00]
           + _pElements[e_01]*o[e_10]
           + _pElements[e_02]*o[e_20];
    m[e_01] = _pElements[e_00]*o[e_01]
           + _pElements[e_01]*o[e_11]
           + _pElements[e_02]*o[e_21];
    m[e_02] = _pElements[e_00]*o[e_02]
           + _pElements[e_01]*o[e_12]
           + _pElements[e_02]*o[e_22];

    m[e_10] = _pElements[e_10]*o[e_00]
           + _pElements[e_11]*o[e_10]
           + _pElements[e_12]*o[e_20];
    m[e_11] = _pElements[e_10]*o[e_01]
           + _pElements[e_11]*o[e_11]
           + _pElements[e_12]*o[e_21];
    m[e_12] = _pElements[e_10]*o[e_02]
           + _pElements[e_11]*o[e_12]
           + _pElements[e_12]*o[e_22];

    m[e_20] = _pElements[e_20]*o[e_00]
           + _pElements[e_21]*o[e_10]
           + _pElements[e_22]*o[e_20];
    m[e_21] = _pElements[e_20]*o[e_01]
           + _pElements[e_21]*o[e_11]
           + _pElements[e_22]*o[e_21];
    m[e_22] = _pElements[e_20]*o[e_02]
           + _pElements[e_21]*o[e_12]
           + _pElements[e_22]*o[e_22];

   *this = m;
}


void
CommonParserMatrix::SetZero()
{
    _pElements[0] = 0, _pElements[1] = 0, _pElements[2] = 0,
    _pElements[3] = 0, _pElements[4] = 0, _pElements[5] = 0,
    _pElements[6] = 0, _pElements[7] = 0, _pElements[8] = 0;
}
void 
CommonParserMatrix::SetIdentity()
{
    _pElements[0] = 1, _pElements[1] = 0, _pElements[2] = 0,
    _pElements[3] = 0, _pElements[4] = 1, _pElements[5] = 0,
    _pElements[6] = 0, _pElements[7] = 0, _pElements[8] = 1;
}

PXR_NAMESPACE_CLOSE_SCOPE
