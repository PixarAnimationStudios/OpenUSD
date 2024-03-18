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
#ifndef PXR_USD_IMAGING_USD_IMAGING_MARKUP_TEXT_H
#define PXR_USD_IMAGING_USD_IMAGING_MARKUP_TEXT_H

/// \file usdImaging/markupText.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/textBlock.h"
#include "pxr/usdImaging/usdImaging/textLine.h"
#include "pxr/usdImaging/usdImaging/textParagraph.h"
#include "pxr/usdImaging/usdImaging/textRun.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingMarkupText
///
/// The representation of multiline multiple style text
///
class UsdImagingMarkupText
{
public:
    /// The default constructor.
    USDIMAGING_API
    UsdImagingMarkupText()
    {
        _paragraphStyleArray = std::make_shared<TextParagraphStyleArray>();
        _listOfTextRuns     = std::make_shared<UsdImagingTextRunList>();
        _textBlockArray     = std::make_shared<UsdImagingTextBlockArray>();
        _textParagraphArray = std::make_shared<UsdImagingTextParagraphArray>();
        _listOfTextLines    = std::make_shared<UsdImagingTextLineList>();
    }

    /// The constructor from a markup string.
    USDIMAGING_API
    UsdImagingMarkupText(const std::wstring& markup, 
                          const std::wstring& language = L"")
        : _markupString(markup)
        , _markupLanguage(language)
    {
        _paragraphStyleArray = std::make_shared<TextParagraphStyleArray>();
        _listOfTextRuns     = std::make_shared<UsdImagingTextRunList>();
        _textBlockArray     = std::make_shared<UsdImagingTextBlockArray>();
        _textParagraphArray = std::make_shared<UsdImagingTextParagraphArray>();
        _listOfTextLines    = std::make_shared<UsdImagingTextLineList>();
    }

    /// The destructor
    USDIMAGING_API
    ~UsdImagingMarkupText() = default;

    /// Set the markup string.
    USDIMAGING_API
    inline void MarkupString(const std::wstring& markup)
    {
        _markupString = markup;
    }

    /// Get the markup string.
    USDIMAGING_API
    inline const std::wstring& MarkupString() const { return _markupString; }

    /// Set the markup language.
    USDIMAGING_API
    inline void MarkupLanguage(const std::wstring& language)
    {
        _markupLanguage = language;
    }

    /// Get the markup language.
    USDIMAGING_API
    inline const std::wstring& MarkupLanguage() const { return _markupLanguage; }

    /// Set the default UsdImagingTextStyle.
    USDIMAGING_API
    inline void GlobalTextStyle(const UsdImagingTextStyle& style)
    {
        _globalTextStyle = style;
    }

    /// Get the default UsdImagingTextStyle.
    USDIMAGING_API
    inline const UsdImagingTextStyle& GlobalTextStyle() const { return _globalTextStyle; }

    /// Set the default paragraph style.
    USDIMAGING_API
    inline void GlobalParagraphStyle(const UsdImagingTextParagraphStyle& style)
    {
        _globalParagraphStyle = style;
    }

    /// Get the default paragraph style.
    USDIMAGING_API
    inline const UsdImagingTextParagraphStyle& GlobalParagraphStyle() const { return _globalParagraphStyle; }

    /// Set the default text color.
    USDIMAGING_API
    inline void DefaultTextColor(const UsdImagingTextColor& color) { _defaultTextColor = color; }

    /// Get the default text Color.
    USDIMAGING_API
    inline const UsdImagingTextColor& DefaultTextColor() const { return _defaultTextColor; }

    /// Get the ParagraphStyle array.
    USDIMAGING_API
    inline const std::shared_ptr<TextParagraphStyleArray> ParagraphStyleArray() const
    {
        return _paragraphStyleArray;
    }

    /// Get the TextRuns list.
    USDIMAGING_API
    inline const std::shared_ptr<UsdImagingTextRunList> ListOfTextRuns() const { return _listOfTextRuns; }

    /// Get the TextBlocks array.
    USDIMAGING_API
    inline const std::shared_ptr<UsdImagingTextBlockArray> TextBlockArray() const { return _textBlockArray; }

    /// Get the TextParagraphs array.
    USDIMAGING_API
    inline const std::shared_ptr<UsdImagingTextParagraphArray> TextParagraphArray() const
    {
        return _textParagraphArray;
    }

    /// Get the TextLines list.
    USDIMAGING_API
    inline const std::shared_ptr<UsdImagingTextLineList> ListOfTextLines() const { return _listOfTextLines; }

private:
    std::wstring _markupString;
    std::wstring _markupLanguage;

    UsdImagingTextStyle _globalTextStyle;
    UsdImagingTextParagraphStyle _globalParagraphStyle;
    UsdImagingTextColor _defaultTextColor;
    std::shared_ptr<TextParagraphStyleArray> _paragraphStyleArray;

    std::shared_ptr<UsdImagingTextRunList> _listOfTextRuns;
    std::shared_ptr<UsdImagingTextBlockArray> _textBlockArray;
    std::shared_ptr<UsdImagingTextParagraphArray> _textParagraphArray;
    std::shared_ptr<UsdImagingTextLineList> _listOfTextLines;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_USD_IMAGING_MARKUP_TEXT_H
