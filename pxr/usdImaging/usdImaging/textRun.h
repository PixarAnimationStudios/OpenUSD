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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_RUN_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_RUN_H

/// \file usdImaging/textRun.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textColor.h"
#include "pxr/usdImaging/usdImaging/textStyleChange.h"
#include <forward_list>

#if !defined(_WIN32)
#include <cstring>
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// \enum class UsdImagingTextRunType
///
/// The type of TextRun.
///
enum class UsdImagingTextRunType
{
    UsdImagingTextRunTypeString,
    UsdImagingTextRunTypeTab,
    UsdImagingTextRunTypeSymbol
};

/// \class UsdImagingTextRun
///
/// A single line single style text. It can be either a text string or a tab string.
///
class UsdImagingTextRun
{
public:
    /// The default constructor.
    USDIMAGING_API
    UsdImagingTextRun() = default;

    /// The constructor.
    USDIMAGING_API
    UsdImagingTextRun(UsdImagingTextRunType type, 
                      int start, 
                      int count)
        : _type(type)
        , _startIndex(start)
        , _length(count)
    {
    }

    /// The copy constructor
    USDIMAGING_API
    UsdImagingTextRun(const UsdImagingTextRun& run)
        : _type(run._type)
        , _startIndex(run._startIndex)
        , _length(run._length)
    {
        _styleChangeArray = run._styleChangeArray;
        if (run._textColor)
            _textColor = std::make_unique<UsdImagingTextColor>(*run._textColor);
    }

    /// The destructor
    USDIMAGING_API
    ~UsdImagingTextRun() = default;

    /// Set the UsdImagingTextRunType.
    USDIMAGING_API
    inline void Type(UsdImagingTextRunType value) { _type = value; }

    /// Get the UsdImagingTextRunType.
    USDIMAGING_API
    inline UsdImagingTextRunType Type() const { return _type; }

    /// Set the position in the Markup String of UsdImagingMarkupText that this TextRun starts.
    USDIMAGING_API
    inline void StartIndex(int index) { _startIndex = index; }

    /// Get the position in the Markup String of UsdImagingMarkupText that this TextRun starts.
    USDIMAGING_API
    inline int StartIndex() const { return _startIndex; }

    /// Set the length of the TextRun.
    USDIMAGING_API
    inline void Length(int length) { _length = length; }

    /// Get the length of the TextRun.
    USDIMAGING_API
    inline int Length() const
    {
    return _type == UsdImagingTextRunType::UsdImagingTextRunTypeTab ? 1 : _length; 
    }

    /// Add a style change to the TextRun.
    USDIMAGING_API
    void AddStyleChange(const UsdImagingTextStyleChange& change);

    /// Provide a UsdImagingTextStyle, then get the changed UsdImagingTextStyle
    /// of this UsdImagingTextRun.
    USDIMAGING_API
    UsdImagingTextStyle GetStyle(const UsdImagingTextStyle& parentStyle) const;

    /// Set the text color of the TextRun.
    USDIMAGING_API
    inline void SetTextColor(const UsdImagingTextColor& color)
    {
        if (!_textColor)
            _textColor = std::make_unique<UsdImagingTextColor>(color);
        else
            *_textColor = color;
    }

    /// Get the text color of the TextRun.
    USDIMAGING_API
    inline UsdImagingTextColor GetTextColor(const UsdImagingTextColor& defaultColor) const
    {
        if (!_textColor)
            return defaultColor;
        else
            return *_textColor;
    }

    /// The equal operator.
    USDIMAGING_API
    UsdImagingTextRun& operator=(const UsdImagingTextRun& other)
    {
        _type             = other._type;
        _startIndex       = other._startIndex;
        _length           = other._length;
        _styleChangeArray = other._styleChangeArray;
        if (other._textColor)
            _textColor = std::make_unique<UsdImagingTextColor>(*other._textColor);

        return *this;
    }

    /// Copy part of the text run.
    USDIMAGING_API
    bool CopyPartOfRun(const UsdImagingTextRun& fromRun, 
                       int startOffset, 
                       int length);

    /// Copy part of the text data from the fromRun.
    /// \param fromRun The data is copied from this TextRun.
    /// \param startOffset We copy the data from this index.
    /// \param length We copy the data until startOffset + length.
    USDIMAGING_API
    bool CopyPartOfData(const UsdImagingTextRun& fromRun, 
                        int startOffset, 
                        int length);

    /// Copy the text style from the fromRun.
    USDIMAGING_API
    bool CopyStyle(const UsdImagingTextRun& fromRun);

    /// Resize the TextRun to a shorter length.
    USDIMAGING_API
    void Shorten(int newLength);
private:
    /// The type of the TextRun.
    UsdImagingTextRunType _type = UsdImagingTextRunType::UsdImagingTextRunTypeString;

    /// The index to the markup string in UsdImagingTextMarkupText, which marks the start of the TextRun.
    int _startIndex = 0;

    /// The length of the text string.
    int _length = 0;

    /// An array of Style Change, which marks the difference from the default text style.
    std::vector<UsdImagingTextStyleChange> _styleChangeArray;

    /// The color of the TextRun, if it is different from the default color.
    std::unique_ptr<UsdImagingTextColor> _textColor = nullptr;
};

/// A list of TextRuns.
typedef std::forward_list<UsdImagingTextRun> UsdImagingTextRunList;

/// \class UsdImagingRangeOfTextRuns
///
/// The TextRunRange includes the TextRuns from the _firstRun until the _lastRun.
/// If _isEmpty is true, the range is empty.
class UsdImagingTextRunRange
{
public:
    /// The iterator point to the first TextRun.
    UsdImagingTextRunList::iterator _firstRun;
    /// The iterator point to the last TextRun.
    UsdImagingTextRunList::iterator _lastRun;
    /// If the range is empty, the _isEmpty is true.
    bool _isEmpty = true;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_RUN_H
