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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_PARAGRAPH_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_PARAGRAPH_H

/// \file usdImaging/textParagraph.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textLine.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingTextParagraph
///
/// The representation of a text paragraph.
///
class UsdImagingTextParagraph
{
public:
    /// The default constructor
    USDIMAGING_API
    UsdImagingTextParagraph() = default;

    /// The constructor.
    USDIMAGING_API
    UsdImagingTextParagraph(const UsdImagingTextParagraphStyle& style, 
                            UsdImagingTextLineList::iterator firstIter, 
                            UsdImagingTextLineList::iterator lastIter)
        : _paragraphStyle(style)
        , _firstLineIter(firstIter)
        , _lastLineIter(lastIter)
    {
    }

    /// The copy constructor
    USDIMAGING_API
    UsdImagingTextParagraph(const UsdImagingTextParagraph& paragraph)
        : _paragraphStyle(paragraph._paragraphStyle)
        , _firstLineIter(paragraph._firstLineIter)
        , _lastLineIter(paragraph._lastLineIter)
    {
    }

    /// The destructor.
    USDIMAGING_API
    ~UsdImagingTextParagraph() = default;

    /// Set the pargraph style.
    USDIMAGING_API
    inline void Style(const UsdImagingTextParagraphStyle& value) { _paragraphStyle = value; }

    /// Get the pargraph style.
    USDIMAGING_API
    inline const UsdImagingTextParagraphStyle& Style() const { return _paragraphStyle; }

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

private:
    /// The style of the paragraph.
    UsdImagingTextParagraphStyle _paragraphStyle;
    /// The iterator of the first TextLine.
    UsdImagingTextLineList::iterator _firstLineIter;
    /// The iterator of the last TextLine.
    UsdImagingTextLineList::iterator _lastLineIter;
};

// A vector of text paragraphs.
typedef std::vector<UsdImagingTextParagraph> UsdImagingTextParagraphArray;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_PARAGRAPH_H
