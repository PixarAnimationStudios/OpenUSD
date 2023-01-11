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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_H

#include "definitions.h"
#include "intermediateInfo.h"
#include "multiLanguageHandlerImpl.h"
#include "globalSetting.h"
#include "globals.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE
class CommonTextSimpleLayout;
class CommonTextComplexScriptInfo;

/// \class CommonTextMultiLanguageHandler
///
/// The multiple-language handler module
///
class CommonTextMultiLanguageHandler
{
private:
    std::shared_ptr<CommonTextFontMapCache> _trueTypeFontMapCache;

    std::shared_ptr<CommonTextStringArray> _defaultTrueTypeFontList;

    // The implementation implements some platform-specific interfaces,
    // which will be used in the font substitution process.
    std::shared_ptr<CommonTextMultiLanguageHandlerImpl> _multiLanguageHandlerImpl;

    bool _allowFromCache;
    bool _allowSystem;
    bool _allowPredefined;

public:
    /// The constructor
    CommonTextMultiLanguageHandler()
    {
        _trueTypeFontMapCache    = std::make_shared<CommonTextFontMapCache>();
        _defaultTrueTypeFontList = std::make_shared<CommonTextStringArray>();
    }

    /// The destructor
    ~CommonTextMultiLanguageHandler() = default;

    /// Do font substitution on the pTextRun
    /// Some characters in pTextRun is not supported by its font.
    /// In this function, the pTextRun is divided into several sub-textruns,
    /// and put them into a list.
    /// Each sub-textrun will contain only the characters supported by the current font,
    /// or contain only the characters not supported.
    /// For the latter case, we will find a new font and substitute the old font.
    /// And then the list of sub-textruns is returned in pSubstituteTextRunList.
    /// \param markupText the markup text.
    /// \param textRunIter The text run we will do font substitution.
    /// \param runTextStyle The text style of the text run
    /// \param textLineIter The text line that the text run belongs to.
    /// \param simpleLayout The CommonTextSimpleLayout of the text run.
    /// \param lastSubRunIter The last subtextrun after the textrun is divided into a list
    /// of subTextRuns and each subTextRun is substituted with a supported font.
    CommonTextStatus SubstituteFont(std::shared_ptr<UsdImagingMarkupText> markupText,
                                    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                    UsdImagingTextRunList::iterator textRunIter,
                                    const UsdImagingTextStyle& runTextStyle,
                                    UsdImagingTextLineList::iterator textLineIter,
                                    const CommonTextSimpleLayout& simpleLayout,
                                    UsdImagingTextRunList::iterator& lastSubRunIter);

    /// Acquire the CommonTextFontMapCache.
    std::shared_ptr<CommonTextFontMapCache> GetFontMapCache();

    /// Acquire the list of default TrueType font.
    std::shared_ptr<CommonTextStringArray> GetDefaultTTFontList() { return _defaultTrueTypeFontList; }

    /// Acquire the implementation
    std::shared_ptr<CommonTextMultiLanguageHandlerImpl> AcquireImplementation();

    /// The default initialization of CommonTextFontMapCache
    /// Return true to indicate a success, false to indicate a failure.
    CommonTextStatus AddDefaultFontToFontMapCache();

    /// Get if the string require complex script handling.
    bool RequireComplexScriptHandling(const wchar_t* start, 
                                      int length);

    /// Divide the string by scripts.
    /// If the text string contain complex script, the text run which contains complex script
    /// will be assigned with CommonTextComplexScriptInfo.
    /// \param markupText The markupText.
    /// \param textRunIter The iterator of the textRun.
    /// \param textLineIter The line that the textRun belongs to.
    /// \param lastSubRunIter If the textRun is divided, this will point to the last subTextRun.
    /// Or else, it is the same as the textRunIter.
    CommonTextStatus DivideStringByScripts(std::shared_ptr<UsdImagingMarkupText> markupText,
                                           std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                           UsdImagingTextRunList::iterator textRunIter,
                                           UsdImagingTextLineList::iterator textLineIter,
                                           UsdImagingTextRunList::iterator& lastSubRunIter);

    /// Get the indices of the complex string.
    /// If the text string contain CommonTextComplexScriptInfo, the cluster information will also be
    /// generated.
    /// \param unicodeString The string we will get glyph indices.
    /// \param pSimpleLayout The CommonTextSimpleLayout of UsdImagingTextRun.
    /// \param textStyle The text style.
    /// \param isAllSupported If all the characters are supported in the font.
    /// \param indices The indices of the glyphs.
    /// \param pComplexScriptInfo The CommonTextComplexScriptInfo of UsdImagingTextRun.
    CommonTextStatus AcquireComplexIndices(std::wstring unicodeString, 
                                           CommonTextSimpleLayout& pSimpleLayout,
                                           const UsdImagingTextStyle& textStyle, 
                                           bool& isAllSupported, 
                                           std::vector<unsigned short>& indices,
                                           std::shared_ptr<CommonTextComplexScriptInfo> pComplexScriptInfo);

    /// Check if all the characters are supported in the font.
    /// indices is only for checking if the character is supported. Don't use them as the
    /// final indices of the characters.
    /// \param style The text style.
    /// \param unicodeString The string we will get glyph indices.
    /// \param isAllSupported If all the characters are supported in the font.
    /// \param indices The indices of the glyphs.
    CommonTextStatus IsAllCharactersSupported(const UsdImagingTextStyle& style, 
                                              std::wstring unicodeString,
                                              bool& isAllSupported, 
                                              unsigned short* indices = nullptr);

protected:
    /// Find a font supported the code page in the cache.
    /// The characters in a text run may be in not only one code page.
    /// For example, the Chinese character is supported in 936(Simplified Chinese),
    /// 950(Traditional Chinese), 932(Japanese), and 949(Korean).
    /// So the different codepages are represented in a single variable dwCodePages.
    ///
    /// This function will get one code page from the dwCodePages, test if
    /// we can find a Font in the cache support this code page, and remove the
    /// code page from dwCodePages. If the font still doesn't support all the characters,
    /// in the text run, we will try this function again with the remain dwCodePages.
    /// Return true to indicate a success, false to indicate a failure.
    /// \param originalTypeface Original type face of the text run. We should find a different font.
    /// \param newTypeFace The new type face if we successfully find it.
    /// \param dwCodePages The remain codepages. Please see the remarks.
    /// \param fontMapCache The font map cache.
    bool _FindNewFontFaceFromCache(const std::string& originalTypeface, 
                                   std::string& newTypeFace,
                                   unsigned long& dwCodePages, 
                                   std::shared_ptr<CommonTextFontMapCache> fontMapCache);

    /// Find a font supported the code page in the cache.
    /// Using Mlang function MapFont to get the system default font for the codepages.
    /// Return true to indicate a success, false to indicate a failure.
    /// \param originalTypeface Original type face of the text run. We should find a different font.
    /// \param newTypeFace The new type face if we successfully find it.
    /// \param dwCodePages The codepages. Please see the remarks.
    bool _FindDefaultFontFace(const std::string& originalTypeface, 
                              std::string& newTypeFace, 
                              long dwCodePages);

    /// The _SystemSubstituteFont includes the font substitution with
    /// the user-defined font and the font substitution with the
    /// system default font.
    /// \param unicodeString The string we will get glyph indices.
    /// \param isComplex If the script of the string is complex.
    /// \param originalTypeface Original type face of the text run. We should find a different font.
    /// \param dwCodePages The codepages of this text string.
    /// \param pFontMapCacheIterator The font map cache iterator.
    /// \param newTypeFaceFind If we successfully find the new type face.
    /// \param newTypeFace The new type face if we successfully find it.
    CommonTextStatus _SystemSubstituteFont(const std::wstring unicodeString,
                                        bool isComplex,
                                        const std::string& originalTypeface, 
                                        long dwCodePages,
                                        std::shared_ptr<CommonTextFontMapCache> fontMapCache, 
                                        bool& newTypeFaceFind,
                                        std::string& newTypeFace);

/// Disable _PredefinedSubstituteFont on mac temporarily to avoid build error,
#ifdef _WIN32
    /// Do the font substitution using predefined character to charset mapping.
    /// \param wch The first character of the text string
    /// \param textRunType The font type of the text run
    /// \param isComplex If the script of the string is complex.
    /// \param newTypeFaceFind If we successfully find the new type face.
    /// \param The typeface of the font.
    CommonTextStatus _PredefinedSubstituteFont(wchar_t wch,
                                             bool isComplex,
                                             bool& newTypeFaceFind, 
                                             std::string& newTypeFace);
#endif
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_H
