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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_LINE_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_LINE_H

/// \file usdImaging/textLine.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textRun.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum class UsdImagingTextLineBreak
///
/// The line break type.
///
enum class UsdImagingTextLineBreak
{
    UsdImagingTextLineBreakNoType,
    UsdImagingTextLineBreakTextStart,
    UsdImagingTextLineBreakTextEnd,
    UsdImagingTextLineBreakLineBreak,
    UsdImagingTextLineBreakBlockBreak,
    UsdImagingTextLineBreakWrapBreak
};

/// \enum class UsdImagingTextLineType
///
/// The line type.
///
enum class UsdImagingTextLineType
{
    UsdImagingTextLineTypeNormal,
    UsdImagingTextLineTypeZero,
    UsdImagingTextLineTypeInvalid
};

/// \class UsdImagingTextLine
///
/// The representation of a line of text.
///
class UsdImagingTextLine
{
public:
    /// The default constructor.
    USDIMAGING_API
    UsdImagingTextLine() = default;

    /// The constructor from a range.
    USDIMAGING_API
    UsdImagingTextLine(const UsdImagingTextRunRange& range)
        : _range(range) 
    {
        if (!_range._isEmpty)
        {
            _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeNormal;
        }
        else
            _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeZero;
    }

    /// The copy constructor.
    USDIMAGING_API
    UsdImagingTextLine(const UsdImagingTextLine& line)
    {
        _lineType        = line._lineType;
        _range._firstRun = line._range._firstRun;
        _range._lastRun  = line._range._lastRun;
        _range._isEmpty = line._range._isEmpty;
        _paragraphStart  = line._paragraphStart;
        _paragraphEnd    = line._paragraphEnd;
        _startBreak      = line._startBreak;
        _endBreak        = line._endBreak;
    }

    /// The destructor.
    USDIMAGING_API
    ~UsdImagingTextLine() = default;

    /// Set the range of TextRuns.
    USDIMAGING_API
    inline void Range(const UsdImagingTextRunRange& range)
    {
        _range = range;
        if (!_range._isEmpty)
        {
            _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeNormal;
        }
        else
            _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeZero;
    }

    /// Get the range of TextRuns.
    USDIMAGING_API
    inline const UsdImagingTextRunRange& Range() const { return _range; }

    /// Get The type of the line.
    USDIMAGING_API
    inline UsdImagingTextLineType LineType() const { return _lineType; }

    /// Set the type of the line.
    USDIMAGING_API
    inline void LineType(UsdImagingTextLineType value) { _lineType = value; }

    /// Get the line break at the start.
    USDIMAGING_API
    inline UsdImagingTextLineBreak StartBreak() const { return _startBreak; }

    /// Set the line break at the start.
    USDIMAGING_API
    inline void StartBreak(UsdImagingTextLineBreak startBreak) { _startBreak = startBreak; }

    /// Get the line break at the end.
    USDIMAGING_API
    inline UsdImagingTextLineBreak EndBreak() const { return _endBreak; }

    /// Set the line break at the end.
    USDIMAGING_API
    inline void EndBreak(UsdImagingTextLineBreak endBreak) { _endBreak = endBreak; }

    /// Get if the line is the start of a paragraph.
    USDIMAGING_API
    inline bool ParagraphStart() const { return _paragraphStart; }

    /// Set if the line is the start of a paragraph.
    USDIMAGING_API
    inline bool ParagraphEnd() const { return _paragraphEnd; }

    /// Get if the line is the end of a paragraph
    USDIMAGING_API
    inline void ParagraphStart(bool value) { _paragraphStart = value; }

    /// Set if the line is the end of a paragraph
    USDIMAGING_API
    inline void ParagraphEnd(bool value) { _paragraphEnd = value; }

    /// Add a TextRun to the end of this line.
    USDIMAGING_API
    bool AddTextRun(const UsdImagingTextRunList::iterator& textRun)
    {
        if (_lineType == UsdImagingTextLineType::UsdImagingTextLineTypeInvalid)
        {
            return false;
        }

        if (_range._isEmpty)
        {
            _range._firstRun = textRun;
            _range._isEmpty = false;
        }
        _range._lastRun = textRun;

        if (_lineType == UsdImagingTextLineType::UsdImagingTextLineTypeZero && textRun->Length() != 0)
            _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeNormal;

        return true;
    }

private:
    /// The type of this line.
    UsdImagingTextLineType _lineType = UsdImagingTextLineType::UsdImagingTextLineTypeZero;
    /// The range of TextRuns that this line includes.
    UsdImagingTextRunRange _range;
    /// If this is the paragraph start line
    bool _paragraphStart = false;
    /// If this is the paragraph last line
    bool _paragraphEnd = false;
    /// The break type at the start
    UsdImagingTextLineBreak _startBreak = UsdImagingTextLineBreak::UsdImagingTextLineBreakNoType;
    /// The break type at the end
    UsdImagingTextLineBreak _endBreak = UsdImagingTextLineBreak::UsdImagingTextLineBreakNoType;
};

// A list of TextLines.
typedef std::list<UsdImagingTextLine> UsdImagingTextLineList;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_LINE_H
