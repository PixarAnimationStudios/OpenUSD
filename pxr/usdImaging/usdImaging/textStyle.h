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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_H

/// \file usdImaging/textStyle.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include <string>

#define NO_CONSTRAINT_VALUE -1.0f

PXR_NAMESPACE_OPEN_SCOPE

/// \struct UsdImagingTextStyle
///
/// The style of text.
/// 
struct UsdImagingTextStyle
{
public:
    /// Font typeface.
    std::string _typeface;

    /// Bold style.
    bool _bold = false;

    /// Italic style.
    bool _italic = false;

    /// Character height.
    int _height = 1;

    /// The factor to increase the width.
    float _widthFactor = 1.0f;

    /// The oblique angle.
    float _obliqueAngle = 0.0f;

    /// The factor to increase the characer space.
    float _characterSpaceFactor = 1.0f;

    /// The line type of underline.
    TfToken _underlineType = UsdImagingTextTokens->none;

    /// The line type of overline.
    TfToken _overlineType = UsdImagingTextTokens->none;

    /// The line type of strike through.
    TfToken _strikethroughType = UsdImagingTextTokens->none;

    /// operator ==.
    USDIMAGING_API
    inline bool operator==(const UsdImagingTextStyle& other) const
    {
        static const float epsilon = 1e-10f;
        return _typeface == other._typeface && _bold == other._bold && _italic == other._italic &&
            _height == other._height && GfIsClose(_widthFactor, other._widthFactor, epsilon) &&
            GfIsClose(_obliqueAngle, other._obliqueAngle, epsilon) &&
            GfIsClose(_characterSpaceFactor, other._characterSpaceFactor, epsilon) &&
            _underlineType == other._underlineType && _overlineType == other._overlineType &&
            _strikethroughType == other._strikethroughType;
    }
    /// Has underline decoration in this UsdImagingTextStyle.
    USDIMAGING_API
    inline bool HasUnderline() const { return _underlineType != UsdImagingTextTokens->none; }
    /// Has overline decoration in this UsdImagingTextStyle.
    USDIMAGING_API
    inline bool HasOverline() const { return _overlineType != UsdImagingTextTokens->none; }
    /// Has strike through decoration in this UsdImagingTextStyle.
    USDIMAGING_API
    inline bool HasStrikethrough() const { return _strikethroughType != UsdImagingTextTokens->none; }
};

/// \enum UsdImagingBlockAlignment
///
/// The enumeration of the block alignment.
/// 
enum class UsdImagingBlockAlignment
{
    /// Align at the top.
    UsdImagingBlockAlignmentTop,
    /// Align at the bottom.
    UsdImagingBlockAlignmentBottom,
    /// Align at the center.
    UsdImagingBlockAlignmentCenter
};

/// \class UsdImagingTextBlockStyle
///
/// The representation of the block attribute.
/// 
class UsdImagingTextBlockStyle
{
public:
    /// The default constructor.
    USDIMAGING_API
    UsdImagingTextBlockStyle() = default;

    /// The constructor from an offset.
    USDIMAGING_API
    UsdImagingTextBlockStyle(const GfVec2f& value) : _offset(value) {}

    /// The constructor from width, height, and margins.
    USDIMAGING_API
    UsdImagingTextBlockStyle(float width, 
                             float height, 
                             float topMargin, 
                             float bottomMargin,
                             float leftMargin,
                             float rightMargin)
        : _width(width)
        , _height(height)
        , _topMargin(topMargin)
        , _bottomMargin(bottomMargin)
        , _leftMargin(leftMargin)
        , _rightMargin(rightMargin)
    {
    }

    /// The constructor for a block that has width constraint but no height constraint.
    USDIMAGING_API
    UsdImagingTextBlockStyle(float width, 
                             float topMargin, 
                             float leftMargin, 
                             float rightMargin)
        :_width(width)
        , _topMargin(topMargin)
        , _leftMargin(leftMargin)
        , _rightMargin(rightMargin)
    {
    }

    /// The constructor for a block that has height constraint but no width constraint.
    USDIMAGING_API
    UsdImagingTextBlockStyle(float height, 
                             float topMargin, 
                             float bottomMargin,
                             float leftMargin, 
                             int)
        : _height(height)
        , _topMargin(topMargin)
        , _bottomMargin(bottomMargin)
        , _leftMargin(leftMargin)
    {
    }

    /// The constructor that has no constraint in width and height
    USDIMAGING_API
    UsdImagingTextBlockStyle(float topMargin, 
                             float leftMargin)
        : _topMargin(topMargin)
        , _leftMargin(leftMargin)
    {
    }

    /// The constructor from alignment.
    USDIMAGING_API
    UsdImagingTextBlockStyle(UsdImagingBlockAlignment alignment) 
        : _alignment(alignment) 
    {}

    /// The copy constructor
    USDIMAGING_API
    UsdImagingTextBlockStyle(const UsdImagingTextBlockStyle& value) = default;

    /// The destructor.
    USDIMAGING_API
    ~UsdImagingTextBlockStyle() = default;

    /// Set the width.
    USDIMAGING_API
    inline void Width(float value) { _width = value; }

    /// Get the width.
    USDIMAGING_API
    inline float Width() const { return _width; }

    /// Set the height.
    USDIMAGING_API
    inline void Height(float value) { _height = value; }

    /// Get the first line index.
    USDIMAGING_API
    inline float Height() const { return _height; }

    /// Get if there is constraint in Width.
    USDIMAGING_API
    inline float WidthConstraint() const { return _width - _leftMargin - _rightMargin; }

    /// Get if there is constraint in height.
    USDIMAGING_API
    inline float HeightConstraint() const { return _height - _topMargin - _bottomMargin; }

    /// Set the topMargin.
    USDIMAGING_API
    inline void TopMargin(float value) { _topMargin = value; }

    /// Get the topMargin.
    USDIMAGING_API
    inline float TopMargin() const { return _topMargin; }

    /// Set the first line index.
    USDIMAGING_API
    inline void BottomMargin(float value) { _bottomMargin = value; }

    /// Get the first line index.
    USDIMAGING_API
    inline float BottomMargin() const { return _bottomMargin; }

    /// Set the leftMargin.
    USDIMAGING_API
    inline void LeftMargin(float value) { _leftMargin = value; }

    /// Get the leftMargin.
    USDIMAGING_API
    inline float LeftMargin() const { return _leftMargin; }

    /// Set the rightMargin.
    USDIMAGING_API
    inline void RightMargin(float value) { _rightMargin = value; }

    /// Get the rightMargin.
    USDIMAGING_API
    inline float RightMargin() const { return _rightMargin; }

    /// Set the alignment.
    USDIMAGING_API
    inline void Alignment(UsdImagingBlockAlignment value) { _alignment = value; }

    /// Get the alignment.
    USDIMAGING_API
    inline UsdImagingBlockAlignment Alignment() const { return _alignment; }

    /// Set the offset.
    USDIMAGING_API
    inline void Offset(const GfVec2f& value) { _offset = value; }

    /// Get the offset.
    USDIMAGING_API
    inline GfVec2f Offset() const { return _offset; }

private:
    /// The width of the block.
    float _width = NO_CONSTRAINT_VALUE;
    /// The height of the block.
    float _height = NO_CONSTRAINT_VALUE;
    /// The offset from the previous block.
    GfVec2f _offset;
    /// The margin at the top.
    float _topMargin = 0.0f;
    /// The margin at the bottom.
    float _bottomMargin = 0.0f;
    /// The margin at the left.
    float _leftMargin = 0.0f;
    /// The margin at the right.
    float _rightMargin = 0.0f;
    /// The alignment in vertical in this block.
    UsdImagingBlockAlignment _alignment = UsdImagingBlockAlignment::UsdImagingBlockAlignmentTop;
};

/// \enum UsdImagingParagraphAlignment
///
/// The enumeration of the paragraph alignment.
/// 
enum class UsdImagingParagraphAlignment
{
    /// No alignment is specified.
    UsdImagingParagraphAlignmentNo = 0,

    /// Align to the left.
    UsdImagingParagraphAlignmentLeft,

    /// Align to the right.
    UsdImagingParagraphAlignmentRight,

    /// Align to the center.
    UsdImagingParagraphAlignmentCenter,

    /// Distribute the words evenly between left and right.
    UsdImagingParagraphAlignmentJustify,

    /// Distribute the characters evenly between left and right.
    UsdImagingParagraphAlignmentDistribute,
};

/// \enum class UsdImagingLineSpaceType
///
/// The enumeration of the type of line space.
///
enum class UsdImagingLineSpaceType
{
    /// The line space is exactly in this value.
    UsdImagingLineSpaceTypeExactly = 0,
    /// The line space is at least in this value.
    UsdImagingLineSpaceTypeAtLeast,
    /// The line space is some ratio to the default single line space.
    UsdImagingLineSpaceTypeMulti
};

/// \enum UsdImagingTabStopType
///
/// The enumeration of the type of tab stop
/// 
enum class UsdImagingTabStopType
{
    /// The tabstop is invalid.
    UsdImagingTabStopTypeInvalid,

    /// A left tabstop.
    UsdImagingTabStopTypeLeft,

    /// A center tabstop.
    UsdImagingTabStopTypeCenter,

    /// A right tabstop.
    UsdImagingTabStopTypeRight,

    /// A decimal tabstop.
    UsdImagingTabStopTypeDecimal
};

/// \class UsdImagingTabStop
///
/// The representation of the tabstop.
/// 
class UsdImagingTabStop
{
public:
    /// The type of the tabstop.
    UsdImagingTabStopType _type = UsdImagingTabStopType::UsdImagingTabStopTypeLeft;

    /// _position is for saving the position of the tabstop of a paragraph.
    /// _width is for saving the width of a tab.
    union
    {
        float _position = 0.0f;
        float _width;
    };

    /// Equal operator overloading.
    USDIMAGING_API
    inline bool operator==(const UsdImagingTabStop& rhs) const
    {
        static const float epsilon = 1e-10f;
        return _type == rhs._type && GfIsClose(_position, rhs._position, epsilon);
    }
};

typedef std::vector<UsdImagingTabStop> TabStopArray;

/// \class UsdImagingTextParagraphStyle
///
/// The representation of the paragraph attribute.
/// 
class UsdImagingTextParagraphStyle
{
public:
    /// The alignment of the paragraph.
    UsdImagingParagraphAlignment _alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentLeft;

    /// The indent on the left of the paragraph.
    float _leftIndent = 0.0f;

    /// The indent on the right of the paragraph.
    float _rightIndent = 0.0f;

    /// The indent on the left of the first line. By default, we use leftIndent as the indent of
    /// first line.
    float _firstLineIndent = -1.0f;

    /// The space after the paragraph.
    float _paragraphSpace = 0.0f;

    /// The linespace type.
    UsdImagingLineSpaceType _lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast;

    /// The space between lines.
    float _lineSpace = 0.0f;

    /// A list of tabstops in this paragraph.
    TabStopArray _tabStopList;

    /// operator ==.
    USDIMAGING_API
    inline bool operator==(const UsdImagingTextParagraphStyle& other) const
    {
        static const float epsilon = 1e-10f;
        return _alignment == other._alignment && GfIsClose(_leftIndent, other._leftIndent, epsilon) &&
            GfIsClose(_rightIndent, other._rightIndent, epsilon) &&
            GfIsClose(_firstLineIndent, other._firstLineIndent, epsilon) &&
            GfIsClose(_paragraphSpace, other._paragraphSpace, epsilon) && 
            _lineSpaceType == other._lineSpaceType &&
            GfIsClose(_lineSpace, other._lineSpace, epsilon) &&
            _tabStopList == other._tabStopList;
    }

    /// The clone function
    USDIMAGING_API
    inline std::shared_ptr<UsdImagingTextParagraphStyle> clone() 
    { return std::make_shared<UsdImagingTextParagraphStyle>(*this); }
};

// A vector of text blocks.
typedef std::vector<UsdImagingTextBlockStyle> TextBlockStyleArray;
// A vector of text paragraphs.
typedef std::vector<UsdImagingTextParagraphStyle> TextParagraphStyleArray;

PXR_NAMESPACE_CLOSE_SCOPE

/// The hash function for a class T.
template <class T>
USDIMAGING_API
inline void hash_combine(std::size_t& s, const T& v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

/// The hash function for TfToken.
template <>
USDIMAGING_API
inline void hash_combine<class PXR_INTERNAL_NS::TfToken>(std::size_t& s, const PXR_INTERNAL_NS::TfToken& v)
{
    s ^= v.Hash() + 0x9e3779b9 + (s << 6) + (s >> 2);
}

namespace std
{
    /// The std::hash implementation for UsdImagingTextStyle.
    template <>
    struct hash<PXR_INTERNAL_NS::UsdImagingTextStyle>
    {
        std::size_t operator()(const PXR_INTERNAL_NS::UsdImagingTextStyle& s) const noexcept
        {
            std::size_t res = 0;
            hash_combine(res, s._typeface);
            hash_combine(res, s._bold);
            hash_combine(res, s._italic);
            hash_combine(res, s._height);
            hash_combine(res, s._widthFactor);
            hash_combine(res, s._obliqueAngle);
            hash_combine(res, s._characterSpaceFactor);
            hash_combine(res, s._underlineType);
            hash_combine(res, s._overlineType);
            hash_combine(res, s._strikethroughType);
            return res; // or use boost::hash_combine
        }
    };
} // namespace std

#endif  // PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_H
