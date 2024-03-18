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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_BLOCK_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_BLOCK_H

/// \file usdImaging/textBlock.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textLine.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingTextBlock
///
/// The representation of a block of text.
///
class UsdImagingTextBlock
{
public:
    /// The default constructor.
    USDIMAGING_API
    UsdImagingTextBlock() = default;

    /// The constructor from TextLine index.
    USDIMAGING_API
    UsdImagingTextBlock(UsdImagingTextLineList::iterator firstIter, 
                        UsdImagingTextLineList::iterator lastIter)
        : _firstLineIter(firstIter)
        , _lastLineIter(lastIter)
    {}

    /// The constructor from an blockStyle.
    USDIMAGING_API
    UsdImagingTextBlock(const UsdImagingTextBlockStyle& value) 
        : _style(value) 
    {}

    /// The copy constructor
    USDIMAGING_API
    UsdImagingTextBlock(const UsdImagingTextBlock& value) = default;

    /// The destructor.
    USDIMAGING_API
    ~UsdImagingTextBlock() = default;

    /// Reset the column to default state.
    USDIMAGING_API
    inline void Reset()
    {
    }

    /// Set the first line iterator.
    USDIMAGING_API
    inline void FirstLineIter(UsdImagingTextLineList::iterator iter) { _firstLineIter = iter; }

    /// Get the first line iterator.
    USDIMAGING_API
    inline UsdImagingTextLineList::iterator FirstLineIter() const { return _firstLineIter; }

    /// Set the last line iterator.
    USDIMAGING_API
    inline void LastLineIter(UsdImagingTextLineList::iterator iter) { _lastLineIter = iter; }

    /// Get the last line iterator.
    USDIMAGING_API
    inline UsdImagingTextLineList::iterator LastLineIter() const { return _lastLineIter; }

    /// Set the width.
    USDIMAGING_API
    inline void Width(float value) { _style.Width(value); }

    /// Get the width.
    USDIMAGING_API
    inline float Width() const { return _style.Width(); }

    /// Set the height.
    USDIMAGING_API
    inline void Height(float value) { _style.Height(value); }

    /// Get the first line index.
    USDIMAGING_API
    inline float Height() const { return _style.Height(); }

    /// Get if there is constraint in Width.
    USDIMAGING_API
    inline float WidthConstraint() const { return _style.WidthConstraint(); }

    /// Get if there is constraint in height.
    USDIMAGING_API
    inline float HeightConstraint() const { return _style.HeightConstraint(); }

    /// Set the topMargin.
    USDIMAGING_API
    inline void TopMargin(float value) { _style.TopMargin(value); }

    /// Get the topMargin.
    USDIMAGING_API
    inline float TopMargin() const { return _style.TopMargin(); }

    /// Set the first line index.
    USDIMAGING_API
    inline void BottomMargin(float value) { _style.BottomMargin(value); }

    /// Get the first line index.
    USDIMAGING_API
    inline float BottomMargin() const { return _style.BottomMargin(); }

    /// Set the leftMargin.
    USDIMAGING_API
    inline void LeftMargin(float value) { _style.LeftMargin(value); }

    /// Get the leftMargin.
    USDIMAGING_API
    inline float LeftMargin() const { return _style.LeftMargin(); }

    /// Set the rightMargin.
    USDIMAGING_API
    inline void RightMargin(float value) { _style.RightMargin(value); }

    /// Get the rightMargin.
    USDIMAGING_API
    inline float RightMargin() const { return _style.RightMargin(); }

    /// Set the alignment.
    USDIMAGING_API
    inline void Alignment(UsdImagingBlockAlignment value) { _style.Alignment(value); }

    /// Get the alignment.
    USDIMAGING_API
    inline UsdImagingBlockAlignment Alignment() const { return _style.Alignment(); }

    /// Set the offset.
    USDIMAGING_API
    inline void Offset(const GfVec2f& value) { _style.Offset(value); }

    /// Get the offset.
    USDIMAGING_API
    inline GfVec2f Offset() const { return _style.Offset(); }

private:
    /// The iterator of the first TextLine.
    UsdImagingTextLineList::iterator _firstLineIter;
    /// The iterator of the last TextLine.
    UsdImagingTextLineList::iterator _lastLineIter;

    UsdImagingTextBlockStyle _style;
};

// A vector of text blocks.
typedef std::vector<UsdImagingTextBlock> UsdImagingTextBlockArray;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_BLOCK_H
