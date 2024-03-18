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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_METRICS_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_METRICS_H

#include "definitions.h"
#include "textMath.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \struct CommonTextFontMetrics
///
/// The metrics of a specified font.
///
struct CommonTextFontMetrics
{
    /// The size of the em square
    int _emSquareSize;

    /// The height of the whole character based on the em square.
    int _emHeight;

    /// The capital height based on the em square.
    int _capHeight;

    /// Ascent plus descent.
    int _height;

    /// The maximum distance characters in this font extend above the base line.
    /// \note This is the typographic ascent for the font.
    /// This is in design units.
    int _typographicAscent;

    /// The maximum distance characters in this font extend below the base line.
    /// \note This is the typographic descent for the font.
    /// This is always below zero, in design units.
    int _typographicDescent;

    /// The ascent (units above the base line) of characters.
    int _ascent;

    /// The descent (units below the base line) of characters.
    /// \note This is always below zero.
    int _descent;

    /// The amount of leading inside the bounds.
    int _internalLeading;

    /// The amount of extra leading outside the bounds.
    int _externalLeading;

    /// The average character width.
    int _avgCharWidth;

    /// The maximum character width.
    int _maxCharWidth;

    /// Default Character defined in the font.
    wchar_t _defaultChar;

    /// Scale the font metrics.
    inline CommonTextFontMetrics& operator*=(float scaleRatio)
    {
        _emHeight           = (int)round(_emHeight * scaleRatio);
        _capHeight          = (int)round(_capHeight * scaleRatio);
        _height             = (int)round(_height * scaleRatio);
        _typographicAscent  = (int)round(_typographicAscent * scaleRatio);
        _typographicDescent = (int)round(_typographicDescent * scaleRatio);
        _ascent             = (int)round(_ascent * scaleRatio);
        _descent            = (int)round(_descent * scaleRatio);
        _internalLeading    = (int)round(_internalLeading * scaleRatio);
        _externalLeading    = (int)round(_externalLeading * scaleRatio);
        _avgCharWidth       = (int)round(_avgCharWidth * scaleRatio);
        _maxCharWidth       = (int)round(_maxCharWidth * scaleRatio);

        return *this;
    }
};

/// \struct CommonTextGlyphMetrics
///
/// The metrics of a specified glyph.
///
struct CommonTextGlyphMetrics
{
    /// The width of the black box.
    int _blackBoxX = 0;

    /// The height of the black box.
    int _blackBoxY = 0;

    /// The offset of the glyph origin in the x axis.
    int _glyphOriginX = 0;

    /// The offset of the glyph origin in the y axis.
    int _glyphOriginY = 0;

    /// The increment in the x axis after we add this glyph.
    int _cellIncX = 0;

    /// The increment in the y axis after we add this glyph.
    int _cellIncY = 0;

    /// The a part of the abc width. This is the offset in the baseline from the start of
    /// the character to the left of the character.
    int _abcA = 0;

    /// The b part of the abc width. This is the offset in the baseline from the left of
    /// the character to the right of the character.
    int _abcB = 0;

    /// The c part of the abc width. This is the offset in the baseline from the right of
    /// the character to the end of the character.
    int _abcC = 0;

    /// The default constructor.
    CommonTextGlyphMetrics() = default;

    /// The copy constructor
    CommonTextGlyphMetrics(const CommonTextGlyphMetrics& rhs)
        : _blackBoxX(rhs._blackBoxX)
        , _blackBoxY(rhs._blackBoxY)
        , _glyphOriginX(rhs._glyphOriginX)
        , _glyphOriginY(rhs._glyphOriginY)
        , _cellIncX(rhs._cellIncX)
        , _cellIncY(rhs._cellIncY)
        , _abcA(rhs._abcA)
        , _abcB(rhs._abcB)
        , _abcC(rhs._abcC)
    {
    }

    /// Test if two CommonTextGlyphMetrics are the same.
    inline bool operator==(const CommonTextGlyphMetrics& rhs) const
    {
        return abs(_blackBoxX - rhs._blackBoxX) < M_EPSILON &&
            abs(_blackBoxY - rhs._blackBoxY) < M_EPSILON &&
            abs(_glyphOriginX - rhs._glyphOriginX) < M_EPSILON &&
            abs(_glyphOriginY - rhs._glyphOriginY) < M_EPSILON &&
            abs(_cellIncX - rhs._cellIncX) < M_EPSILON && abs(_cellIncY - rhs._cellIncY) < M_EPSILON &&
            abs(_abcA - rhs._abcA) < M_EPSILON && abs(_abcB - rhs._abcB) < M_EPSILON &&
            abs(_abcC - rhs._abcC) < M_EPSILON;
    }

    /// Scale the glyph metrics.
    inline CommonTextGlyphMetrics& operator*=(int scaleRatio)
    {
        _blackBoxX *= scaleRatio;
        _blackBoxY *= scaleRatio;
        _glyphOriginX *= scaleRatio;
        _glyphOriginY *= scaleRatio;
        _cellIncX *= scaleRatio;
        _cellIncY *= scaleRatio;
        _abcA *= scaleRatio;
        _abcB *= scaleRatio;
        _abcC *= scaleRatio;
        return *this;
    }
};

/// \struct CommonTextUnicodeRange
///
/// A range of unicode.
///
struct CommonTextUnicodeRange
{
    /// Low Unicode code point in the range of supported Unicode code points.
    wchar_t _wcLow;

    /// Number of supported Unicode code points in this range.
    short _cGlyphs;
};

/// \struct CommonTextFontUnicodeRanges
///
/// The information of the ranges of unicode supported by a font
///
struct CommonTextFontUnicodeRanges
{
    /// The total number of Unicode code points supported in the font.
    int _cGlyphsSupported = 0;

    /// The total number of Unicode ranges in ranges.
    int _cRanges = 0;

    /// Array of Unicode ranges that are supported in the font.
    CommonTextUnicodeRange* _ranges = nullptr;

    /// The constructor.
    CommonTextFontUnicodeRanges() = default;

    /// The destructor.
    ~CommonTextFontUnicodeRanges()
    {
        if (_ranges)
        {
            delete _ranges;
            _ranges = nullptr;
        }
    }
};

/// \class CommonTextComplexScriptMetrics
///
/// The metrics of the complex script in the string.
///
class CommonTextComplexScriptMetrics
{
protected:
    int _glyphCount   = 0;
    int _clusterCount = 0;
    std::vector<short> _characterToGlyphMap;
    std::vector<short> _characterToClusterMap;
    int _lengthOfClusterAttributes = 0;
    char* _clusterAttributes       = nullptr;

public:
    /// Constructor.
    CommonTextComplexScriptMetrics() = default;

    /// Destructor.
    ~CommonTextComplexScriptMetrics()
    {
        if (_clusterAttributes != nullptr)
            delete[] _clusterAttributes;
    }

    /// Set the count of glyphs.
    inline void GlyphCount(int glyphCount) { _glyphCount = glyphCount; }

    /// Get the count of glyphs.
    inline int GlyphCount() { return _glyphCount; }

    /// Set the count of clusters.
    inline void ClusterCount(int clusterCount) { _clusterCount = clusterCount; }

    /// Get the count of clusters.
    inline int ClusterCount() { return _clusterCount; }

    /// Set the attributes of clusters.
    /// Only the multilanguage handler of the platform can understand the structure of
    /// attributes.
    inline void ClusterAttributes(int lengthOfClusterAttributes, 
                                  char* clusterAttributes)
    {
        if (_clusterAttributes != nullptr)
            delete[] _clusterAttributes;
        _lengthOfClusterAttributes = lengthOfClusterAttributes;
        _clusterAttributes         = clusterAttributes;
    }

    /// Get the attributes of clusters.
    /// Only the multilanguage handler of the platform can understand the structure of
    /// attributes.
    inline char* ClusterAttributes() { return _clusterAttributes; }

    /// Get the map from the character to glyph
    inline std::vector<short>& CharacterToGlyphMap() { return _characterToGlyphMap; }

    /// Get the map from the character to cluster
    inline std::vector<short>& CharacterToClusterMap() { return _characterToClusterMap; }
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_METRICS_H
