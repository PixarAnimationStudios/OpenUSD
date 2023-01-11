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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SIMPLE_LAYOUT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SIMPLE_LAYOUT_H

#include "definitions.h"
#include "fontDevice.h"
#include "globals.h"
#include "metrics.h"

PXR_NAMESPACE_OPEN_SCOPE
class CommonTextTrueTypeFontDevice;
class CommonTextSystem;
class CommonTextComplexScriptInfo;

/// \struct CommonTextMetrics
///
/// The metrics for a text primitive.
///
struct CommonTextMetrics
{
public:
    /// The semantically bounding box of the string.
    CommonTextBox2<GfVec2f> _semanticBound;
    /// The exactly bounding box of the geometry.
    CommonTextBox2<GfVec2f> _extentBound;
};

/// \struct CommonTextCharMetrics
///
/// The metrics for one character.
///
struct CommonTextCharMetrics
{
public:
    float _startPosition;
    float _endPosition;
    CommonTextBox2<GfVec2i> _boundBox;
};

/// \enum class CommonTextMetricsInfoAvailability
///
/// The availability of the text metrics in the simple text layout.
///
enum class CommonTextMetricsInfoAvailability
{
    /// The layout is empty.
    CommonTextMetricsInfoAvailabilityEmpty = 0x00,
    /// The character indices is valid
    CommonTextMetricsInfoAvailabilityIndicesValid = 0x01,
    /// The character indices are available
    CommonTextMetricsInfoAvailabilityIndicesAvailable = 0x02,
    /// The character metrics are available
    CommonTextMetricsInfoAvailabilityCharMetricsAvailable = 0x04,
    /// The text matrics are available.
    CommonTextMetricsInfoAvailabilityTextMetricsAvailable = 0x08
};

constexpr CommonTextMetricsInfoAvailability operator|(
    const CommonTextMetricsInfoAvailability selfValue, 
    const CommonTextMetricsInfoAvailability inValue)
{
    return (CommonTextMetricsInfoAvailability)(int(selfValue) | int(inValue));
}

constexpr CommonTextMetricsInfoAvailability operator&(
    const CommonTextMetricsInfoAvailability selfValue, 
    const CommonTextMetricsInfoAvailability inValue)
{
    return (CommonTextMetricsInfoAvailability)(int(selfValue) & int(inValue));
}

constexpr CommonTextMetricsInfoAvailability operator~(
    const CommonTextMetricsInfoAvailability value)
{
    return (CommonTextMetricsInfoAvailability)(~(int(value)));
}

/// \class CommonTextSimpleLayout
///
/// The layout of a single-line single-style text
///
class CommonTextSimpleLayout
{
private:
    CommonTextMetrics _fullMetrics;
    int _countOfRenderableChars = 0;
    std::vector<CommonTextCharMetrics> _arrayCharacterMetrics;
    std::vector<unsigned short> _arrayIndices;

    /// Whether the character indices, metrics, glyph positions, and extents are available.
    CommonTextMetricsInfoAvailability _metricsInfoAvailability = 
        CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityEmpty;

    std::shared_ptr<CommonTextComplexScriptMetrics> _complexScriptMetrics;

public:
    /// The constructor.
    CommonTextSimpleLayout() = default;

    /// The destructor.
    ~CommonTextSimpleLayout() = default;

    /// Reset the layout.
    void Reset()
    {
        _fullMetrics._semanticBound.Clear();
        _fullMetrics._extentBound.Clear();
        _countOfRenderableChars = 0;
        _arrayCharacterMetrics.clear();
        _arrayIndices.clear();
        _metricsInfoAvailability = CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityEmpty;
    }

    /// Get the metrics of the whole text primitive.
    CommonTextMetrics& FullMetrics() { return _fullMetrics; }

    /// Get the metrics of the whole text primitive.
    const CommonTextMetrics& FullMetrics() const { return _fullMetrics; }

    /// Get the count of renderable characters
    int CountOfRenderableChars() const { return _countOfRenderableChars; }

    /// Set the count of renderable characters
    void SetCountOfRenderableChars(int count)
    {
        _countOfRenderableChars = count;
        _arrayCharacterMetrics.resize(count);
        _arrayIndices.resize(count);
    }

    /// Get the metrics of a renderable char.
    CommonTextCharMetrics& CharacterMetrics(int index) { return _arrayCharacterMetrics.at(index); }

    /// Get the metrics of a renderable char.
    const CommonTextCharMetrics& CharacterMetrics(int index) const
    {
        return _arrayCharacterMetrics.at(index);
    }

    /// Get the indices of the renderable char in the font.
    std::vector<unsigned short>& CharacterIndices() { return _arrayIndices; }

    /// Get the indices of the renderable char in the font.
    const std::vector<unsigned short>& CharacterIndices() const { return _arrayIndices; }

    /// Get the information of complex script
    std::shared_ptr<CommonTextComplexScriptMetrics> GetComplexScriptMetrics() const
    {
        return _complexScriptMetrics;
    }

    /// Set the information of complex script
    void SetComplexScriptMetrics(std::shared_ptr<CommonTextComplexScriptMetrics> info)
    {
        _complexScriptMetrics = info;
    }

    /// Test if some bits of the _metricsInfoAvailability are set.
    /// \note The flag should be one enumeration or a set of enumerations in
    /// MetricsInfoAvailability.
    bool TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability flag) const
    {
        return ((_metricsInfoAvailability & flag) == flag);
    }

    /// Set or unset some bits of the _metricsInfoAvailability.
    /// \note The flag should be one enumeration or a set of enumerations in
    /// _metricsInfoAvailability.
    void SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability flag, bool value)
    {
        if (value)
            _metricsInfoAvailability = _metricsInfoAvailability | flag;
        else
            _metricsInfoAvailability = _metricsInfoAvailability & (~flag);
    }

    /// Scale the character metrics by a ratio.
    void Scale(float ratio)
    {
        // Scale the character metrics to current size.
        for (int i = 0; i < CountOfRenderableChars(); i++)
        {
            CommonTextCharMetrics& charMetrics = _arrayCharacterMetrics[i];
            charMetrics._startPosition *= ratio;
            charMetrics._endPosition *= ratio;
            charMetrics._boundBox = CommonTextBox2<GfVec2i>((int)(charMetrics._boundBox.Min()[0] * ratio),
                (int)(charMetrics._boundBox.Min()[1] * ratio),
                (int)(charMetrics._boundBox.Max()[0] * ratio),
                (int)(charMetrics._boundBox.Max()[1] * ratio));
        }
        CommonTextBox2<GfVec2f>& extentBound   = _fullMetrics._extentBound;
        CommonTextBox2<GfVec2f>& semanticBound = _fullMetrics._semanticBound;
        extentBound.Min(extentBound.Min() * ratio);
        extentBound.Max(extentBound.Max() * ratio);
        semanticBound.Min(semanticBound.Min() * ratio);
        semanticBound.Max(semanticBound.Max() * ratio);
    }
    /// Test if the glyph index is valid.
    /// Implement the virtual interface of ATextRun.
    bool IsGlyphIndexValidAt(int index) const
    {
        assert(index < (int)_arrayIndices.size());
        if (index >= (int)_arrayIndices.size())
            return false;
        return _arrayIndices[index] != TRUETYPE_MISSING_GLYPH_INDEX;
    }
};

/// \class CommonTextTrueTypeSimpleLayoutManager
///
/// Class which can generate the layout of a single-line single-style text.
///
class CommonTextTrueTypeSimpleLayoutManager
{
    friend class CommonTextSystem;

private:
    CommonTextTrueTypeFontDevicePtr _fontDevice;
    CommonTextSystem* _textSystem = nullptr;

protected:
    /// Acquire the indices of each character of each Ascii text.
    CommonTextStatus _QueryGlyphIndices(const std::string& asciiString, 
                                        CommonTextSimpleLayout& simpleLayout,
                                        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo = nullptr);

    /// Acquire the indices of each character of each Unicode text.
    CommonTextStatus _QueryGlyphIndices(const std::wstring& unicodeString, 
                                        CommonTextSimpleLayout& simpleLayout,
                                        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo = nullptr);

    /// Calculate the CharMetrics of CommonTextSimpleLayout.
    CommonTextStatus _CalculateCharMetrics(CommonTextSimpleLayout& simpleLayout);

public:
    /// The constructor.
    CommonTextTrueTypeSimpleLayoutManager(
        CommonTextSystem* textSystem, 
        const UsdImagingTextStyle& style) 
        : _textSystem(textSystem)
    {
        if (textSystem)
        {
            _fontDevice.Initialize(style);
        }
    }

    /// The copy constructor is not allowed.
    CommonTextTrueTypeSimpleLayoutManager(const CommonTextTrueTypeSimpleLayoutManager& from) = delete;

    /// The move constructor.
    CommonTextTrueTypeSimpleLayoutManager(CommonTextTrueTypeSimpleLayoutManager&& from) noexcept :
        _fontDevice(std::move(from._fontDevice)),
        _textSystem(from._textSystem)
    {
    }

    /// The destructor.
    ~CommonTextTrueTypeSimpleLayoutManager();

    /// The copy assignment is not allowed.
    CommonTextTrueTypeSimpleLayoutManager& operator=(CommonTextTrueTypeSimpleLayoutManager& from) = delete;

    /// The move assignment.
    CommonTextTrueTypeSimpleLayoutManager& operator=(CommonTextTrueTypeSimpleLayoutManager&& from) noexcept
    {
        _fontDevice = std::move(from._fontDevice);
        _textSystem = from._textSystem;
        return *this;
    }

    /// If the simpleLayoutManager is valid.
    bool IsValid() const { return _textSystem && _fontDevice.IsValid(); }

    /// Get the CharacterMetrics and CharacterIndices of full characters.
    /// Ascii string version.
    CommonTextStatus GenerateCharMetricsAndIndices(
        const std::string& asciiString,
        CommonTextSimpleLayout& simpleLayout,
        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo = nullptr);

    /// Get the CharacterMetrics and CharacterIndices of full characters.
    /// Unicode string version.
    CommonTextStatus GenerateCharMetricsAndIndices(
        const std::wstring& unicodeString,
        CommonTextSimpleLayout& simpleLayout,
        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo = nullptr);

    /// Get the layout of the text, including the position and box of each character.
    CommonTextStatus GenerateTextMetrics(CommonTextSimpleLayout& simpleLayout);

    /// Generate the CommonTextSimpleLayout.
    CommonTextStatus GenerateSimpleLayout(
        const std::string& asciiString,
        CommonTextSimpleLayout& simpleLayout,
        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo = nullptr)
    {
        CommonTextStatus status =
            GenerateCharMetricsAndIndices(asciiString, simpleLayout, complexScriptInfo);
        if (status == CommonTextStatus::CommonTextStatusSuccess)
        {
            // Generate the text metrics of the textrun.
            return GenerateTextMetrics(simpleLayout);
        }
        else
            return status;
    }

    /// Generate the CommonTextSimpleLayout.
    CommonTextStatus GenerateSimpleLayout(
        const std::wstring& unicodeString, 
        CommonTextSimpleLayout& simpleLayout,
        std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo)
    {
        CommonTextStatus status =
            GenerateCharMetricsAndIndices(unicodeString, simpleLayout, complexScriptInfo);
        if (status == CommonTextStatus::CommonTextStatusSuccess)
        {
            // Generate the text metrics of the textrun.
            return GenerateTextMetrics(simpleLayout);
        }
        else
            return status;
    }

    /// Get the rasterized data for the renderable character.
    CommonTextStatus GenerateRasterizedData(short glyphIndex, 
                                            CommonTextBox2<GfVec2i>& rasBox, 
                                            int& dataLength, 
                                            void* data);

    /// Get the triangular control points geometry for the renderable character.
    CommonTextStatus GenerateRawGlyph(short glyphIndex, 
                                        CommonTextBox2<GfVec2i>& rasBox, 
                                        UsdImagingTextRawGlyph& rawGlyph);
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_SIMPLE_LAYOUT_H
