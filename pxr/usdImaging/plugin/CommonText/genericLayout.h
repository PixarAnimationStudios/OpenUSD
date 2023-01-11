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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GENERIC_LAYOUT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GENERIC_LAYOUT_H

#include "definitions.h"
#include "intermediateInfo.h"
#include "simpleLayout.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::vector<std::pair<float, float>> CommonTextPosition2DArray;
typedef std::function<CommonTextStatus(std::shared_ptr<UsdImagingMarkupText>,
                                       std::shared_ptr<CommonTextIntermediateInfo>,
                                       UsdImagingTextRunList::iterator,
                                       UsdImagingTextLineList::iterator,
                                       UsdImagingTextRunList::iterator&)> CommonTextDivideTextRunFunc;

/// \class CommonTextRunLayout
///
/// The layout for a text run.
///
class CommonTextRunLayout
{
public:
    /// The constructor.
    CommonTextRunLayout() = default;

    /// Constructor from layout and start end index.
    CommonTextRunLayout(CommonTextSimpleLayout& layout)
        : _layout(layout)
    {
    }

    /// The copy constructor.
    CommonTextRunLayout(const CommonTextRunLayout& other) = default;

    /// Get the layout of the textrun.
    inline const CommonTextSimpleLayout& SimpleLayout() const { return _layout; }

    /// Set the layout of the textrun.
    inline CommonTextSimpleLayout& SimpleLayout() { return _layout; }

private:
    CommonTextSimpleLayout _layout;
};

/// A list of textRuns.
typedef std::forward_list<CommonTextRunLayout> CommonTextRunLayoutList;

/// \class CommonTextRunLayoutRange
///
/// The TextRunLayoutRange includes the layout from the _firstRun until the _lastRun.
/// If _isEmpty is true, the range is empty.
///
class CommonTextRunLayoutRange
{
public:
    /// The iterator point to the first textRun.
    CommonTextRunLayoutList::iterator _firstRunLayout;
    /// The iterator point to the layout of last textRun.
    CommonTextRunLayoutList::iterator _lastRunLayout;
    /// If the range is empty, the _isEmpty is true.
    bool _isEmpty = true;
};

/// \class CommonTextDecorationLayout
///
/// The layout of a text-line-decoration(underline,overline).
/// The DecorationLayout is composed of some contiguous line sections.
///
struct CommonTextDecorationLayout
{
public:
    /// The type of the decoration
    UsdImagingTextProperty _decoration = UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType;
    /// Type of the line.
    TfToken _type = UsdImagingTextTokens->none;

    /// Section data is computed from contiguous textRuns which
    /// have text-line-decoration and same
    struct CommonTextSection
    {
        pxr::UsdImagingTextColor _lineColor;
        float _endXPosition = 0.0f;
    };

    /// Start X position of the line.
    float _startXPosition;
    /// Line sections with different color.
    std::vector<CommonTextSection> _sections;
    /// Y position of the line.
    float _yPosition = 0.0f;
    /// The line is valid or not. kNONE always means the line is empty.
    inline bool IsValid() const { return _type != UsdImagingTextTokens->none; }

    CommonTextDecorationLayout(UsdImagingTextProperty property)
        : _decoration(property)
    {}
};

/// \class CommonTextLineLayout
///
/// The layout of a line of text.
///
class CommonTextLineLayout
{
public:
    /// The default constructor.
    CommonTextLineLayout() { Reset(); }

    /// The copy constructor.
    CommonTextLineLayout(const CommonTextLineLayout& other)
    {
        _arrayOfTextRunPositions = other._arrayOfTextRunPositions;
        _range._firstRunLayout   = other._range._firstRunLayout;
        _range._lastRunLayout    = other._range._lastRunLayout;
        _lineAscent              = other._lineAscent;
        _lineDescent             = other._lineDescent;
        _decorations             = other._decorations;
    }

    /// The destructor.
    ~CommonTextLineLayout() = default;

    /// Reset the layout.
    inline void Reset()
    {
        _arrayOfTextRunPositions.clear();
        _lineAscent           = 0.0f;
        _lineDescent          = 0.0f;
        _decorations.clear();
    }

    /// Get the array of TextRun positions.
    inline std::vector<std::pair<float, float>>& ArrayOfTextRunPositions()
    {
        return _arrayOfTextRunPositions;
    }

    /// Get the array of TextRun positions.
    inline const std::vector<std::pair<float, float>>& ArrayOfTextRunPositions() const
    {
        return _arrayOfTextRunPositions;
    }

    /// Get the range of the textrun layout belong to the line layout.
    inline const CommonTextRunLayoutRange& Range() const { return _range; }

    /// Get the range of the textrun layout belong to the line layout.
    inline CommonTextRunLayoutRange& Range() { return _range; }

    /// Get the ascent of the line.
    inline float LineAscent() const { return _lineAscent; }

    /// Set the ascent of the line.
    inline void LineAscent(float value) { _lineAscent = value; }

    /// Get the descent of the line.
    inline float LineDescent() const { return _lineDescent; }

    /// Set the descent of the line.
    inline void LineDescent(float value) { _lineDescent = value; }

    /// Get the decorations of the line.
    inline std::vector<CommonTextDecorationLayout>& Decorations() { return _decorations; }

    /// Get the line layout array.
    inline const std::vector<CommonTextDecorationLayout>& Decorations() const { return _decorations; }

private:
    std::vector<std::pair<float, float>> _arrayOfTextRunPositions;
    CommonTextRunLayoutRange _range;
    std::vector<CommonTextDecorationLayout> _decorations;

    float _lineAscent  = 0.0f;
    float _lineDescent = 0.0f;
};

/// A list of text Lines.
typedef std::list<CommonTextLineLayout> CommonTextLineLayoutList;

/// \class CommonTextBlockLayout
///
/// The layout of a block of text.
///
class CommonTextBlockLayout
{
public:
    /// The default constructor.
    CommonTextBlockLayout() { Reset(); }

    /// The copy constructor.
    CommonTextBlockLayout(const CommonTextBlockLayout& other) = default;

    /// The destructor.
    ~CommonTextBlockLayout() = default;

    /// Reset the layout.
    inline void Reset()
    {
        _arrayOfLinePositions.clear();
    }

    /// Set the first line iterator.
    inline void FirstLineLayoutIter(CommonTextLineLayoutList::iterator iter) 
    {
        _firstLineLayoutIter = iter;
    }

    /// Get the first line iterator.
    inline CommonTextLineLayoutList::iterator FirstLineLayoutIter() const { return _firstLineLayoutIter; }

    /// Set the last line iterator.
    inline void LastLineLayoutIter(CommonTextLineLayoutList::iterator iter) { _lastLineLayoutIter = iter; }

    /// Get the last line iterator.
    inline CommonTextLineLayoutList::iterator LastLineLayoutIter() const { return _lastLineLayoutIter; }

    /// Get the array of line positions.
    inline std::vector<std::pair<float, float>>& ArrayOfLinePositions() { return _arrayOfLinePositions; }

    /// Get the array of line positions.
    inline const std::vector<std::pair<float, float>>& ArrayOfLinePositions() const
    {
        return _arrayOfLinePositions;
    }

private:
    /// The iterator of the first text line.
    CommonTextLineLayoutList::iterator _firstLineLayoutIter;
    /// The iterator of the last text line.
    CommonTextLineLayoutList::iterator _lastLineLayoutIter;

    std::vector<std::pair<float, float>> _arrayOfLinePositions;
};

/// \class CommonTextGenericLayout
///
/// The layout of a multiple line multiple style text primitive.
///
class CommonTextGenericLayout
{
public:
    /// The default constructor.
    CommonTextGenericLayout() { Reset(); }

    /// The copy constructor.
    CommonTextGenericLayout(const CommonTextGenericLayout& /*layout*/) = default;

    /// The destructor.
    ~CommonTextGenericLayout() = default;

    /// Reset the layout.
    inline void Reset()
    {
        _arrayOfBlockPositions.clear();
        _arrayBlockLayouts.clear();
        _listOfTextLineLayouts.clear();
        _listOfTextRunLayouts.clear();
    }

    /// Get the block positions array
    inline CommonTextPosition2DArray& ArrayOfBlockPositions() { return _arrayOfBlockPositions; }

    /// Get the block positions array
    inline const CommonTextPosition2DArray& ArrayOfBlockPositions() const { return _arrayOfBlockPositions; }

    /// Get the block layouts array.
    inline std::vector<CommonTextBlockLayout>& ArrayBlockLayouts() { return _arrayBlockLayouts; }

    /// Get the block layouts array.
    inline const std::vector<CommonTextBlockLayout>& ArrayBlockLayouts() const { return _arrayBlockLayouts; }

    /// Get the line layout list.
    inline CommonTextLineLayoutList& ListOfTextLineLayouts() { return _listOfTextLineLayouts; }

    /// Get the line layout list.
    inline const CommonTextLineLayoutList& ListOfTextLineLayouts() const { return _listOfTextLineLayouts; }

    /// Get the simple layout list.
    inline CommonTextRunLayoutList& ListOfTextRunLayouts() { return _listOfTextRunLayouts; }

    /// Get the simple layout list.
    inline const CommonTextRunLayoutList& ListOfTextRunLayouts() const
    {
        return _listOfTextRunLayouts;
    }

private:
    CommonTextPosition2DArray _arrayOfBlockPositions;

    std::vector<CommonTextBlockLayout> _arrayBlockLayouts;
    CommonTextLineLayoutList _listOfTextLineLayouts;
    CommonTextRunLayoutList _listOfTextRunLayouts;
};

/// \class CommonTextTrueTypeGenericLayoutManager
///
/// Class which can generate the layout of a multiple line multiple style text primitive.
///
class CommonTextTrueTypeGenericLayoutManager
{
    friend class CommonTextSystem;

public:
    /// The constructor.
    CommonTextTrueTypeGenericLayoutManager(CommonTextSystem* textSystem)
        : _textSystem(textSystem)
    {
    }

    /// The copy constructor is not allowed.
    CommonTextTrueTypeGenericLayoutManager(const CommonTextTrueTypeGenericLayoutManager& /*from*/) = delete;

    /// The move constructor.
    CommonTextTrueTypeGenericLayoutManager(CommonTextTrueTypeGenericLayoutManager&& from) noexcept :
        _textSystem(from._textSystem)
    {
    }

    /// The copy assignment is not allowed.
    CommonTextTrueTypeGenericLayoutManager& operator=(CommonTextTrueTypeGenericLayoutManager& /*from*/) = delete;

    /// The move assignment.
    CommonTextTrueTypeGenericLayoutManager& operator=(CommonTextTrueTypeGenericLayoutManager&& from) noexcept
    {
        _textSystem    = from._textSystem;
        _markupText   = nullptr;
        _genericLayout = nullptr;
        return *this;
    }

    /// The destructor.
    ~CommonTextTrueTypeGenericLayoutManager() = default;

    /// If the genericLayoutManager is valid.
    inline bool IsValid() const { return _textSystem && _markupText && _genericLayout; }

    /// Initialize the layout manager
    void Initialize(std::shared_ptr<UsdImagingMarkupText> markupText,
                    std::shared_ptr<CommonTextGenericLayout> genericLayout,
                    bool useFullSizeToGenerateLayout = false,
                    bool allowLineBreakInWord = false,
                    bool allowLineBreakBetweenScripts = true)
    {
        _markupText                  = markupText;
        _genericLayout                = genericLayout;
        _useFullSizeToGenerateLayout  = useFullSizeToGenerateLayout;
        _allowLineBreakInWord         = allowLineBreakInWord;
        _allowLineBreakBetweenScripts = allowLineBreakBetweenScripts;
        _intermediateInfo = nullptr;
    }

    /// Generate the layout of a multiple line multiple style text primitive.
    CommonTextStatus GenerateGenericLayout();

    /// Get if we use the full size font to generate the layout of each character.
    /// \details
    /// The text shown on the screen can be zoom in/out. But the layout may not be proportional
    /// with the text size. To keep the text not jumping in the screen, we need to use a unified
    /// layout in different size. So it may be required to use full size to generate the layout
    /// and then scale to current size.
    inline bool UseFullSizeToGenerateLayout() const { return _useFullSizeToGenerateLayout; }

    /// Get if we allow line break in word.
    inline bool AllowLineBreakInWord() const { return _allowLineBreakInWord; }

    /// Get if we allow line between different scripts.
    inline bool AllowLineBreakBetweenScripts() const { return _allowLineBreakBetweenScripts; }

    /// Get the absolute positions for all textruns relative to the origin of the text.
    CommonTextStatus GetAbsolutePositionForAllTextRuns(CommonTextPosition2DArray& positionArray);

    /// Collect decorations of all lines from GenericLayout.
    CommonTextStatus CollectDecorations(std::vector<CommonTextDecorationLayout>& decorationsArray);


    /// Divide the textRun into a list of subTextRuns. The dividePosInTextRun is the
    /// position that we divide the subTextRuns. The lastSubRunIter is the last subTextRun.
    static CommonTextStatus DivideTextRun(std::shared_ptr<UsdImagingMarkupText> markupText,
                                          std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                          UsdImagingTextRunList::iterator textRunIter,
                                          std::vector<int> dividePosInTextRun,
                                          UsdImagingTextLineList::iterator textLineIter,
                                          UsdImagingTextRunList::iterator& lastSubRunIter);

    /// Add a TextRun into the markupText, and add its information to the intermediateInfo.
    static UsdImagingTextRunList::iterator AddTextRun(
        std::shared_ptr<UsdImagingMarkupText> markupText,
        std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
        UsdImagingTextRunList::iterator insertPos,
        const UsdImagingTextRun& addedRun,
        const CommonTextRunInfo& textRunInfo);

    /// Add a TextLine into the markupText, and add its WordBreakIndexList to the intermediateInfo.
    static UsdImagingTextLineList::iterator AddTextLine(
        std::shared_ptr<UsdImagingMarkupText> markupText,
        std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
        UsdImagingTextLineList::iterator insertPos,
        const UsdImagingTextLine& addedLine,
        const WordBreakIndexList& wordBreakIndexList);

private:
    /// Generate the layout from a UsdImagingMarkupText who is in pre-layout state.
    CommonTextStatus _GenerateLayoutFromPreLayout();

    /// Generate the layout from a UsdImagingMarkupText who is in layout state.
    CommonTextStatus _GenerateLayoutFromLayout();

    /// Generate the simple layout for all the text runs.
    CommonTextStatus _GenerateSimpleLayoutForAllRuns(
        const UsdImagingTextStyle& globalTextStyle, 
        const std::wstring& markupString,
        std::shared_ptr<UsdImagingTextRunList>& textRunList,
        CommonTextRunLayoutList& listOfTextRunLayouts);

    /// Initialize the line layouts.
    CommonTextStatus _InitializeLineLayouts(
        const std::shared_ptr<UsdImagingTextRunList>& textRunList,
        CommonTextRunLayoutList& listOfTextRunLayouts,
        const std::shared_ptr<UsdImagingTextLineList> textLineArray, 
        CommonTextLineLayoutList& listOfTextLineLayouts);

    /// Divide a text run if it is composed with different scripts.
    static CommonTextStatus _DivideTextRunByScripts(std::shared_ptr<UsdImagingMarkupText> markupText,
                                                    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                                    UsdImagingTextRunList::iterator textRunIter,
                                                    UsdImagingTextLineList::iterator textLineIter,
                                                    UsdImagingTextRunList::iterator& lastSubRunIter);

    /// Divide a text run if it contains tabstops.
    static CommonTextStatus _DivideTextRunByTabs(std::shared_ptr<UsdImagingMarkupText> markupText,
                                                 std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                                 UsdImagingTextRunList::iterator textRunIter,
                                                 UsdImagingTextLineList::iterator textLineIter,
                                                 UsdImagingTextRunList::iterator& lastSubRunIter);

    /// Divide all the text runs if they are in different scripts.
    /// If devideFunc is DivideTextRunByScripts, after this function, each text run should
    /// be either in non-complex script, or in the same complex script.
    CommonTextStatus _DivideTextRun(CommonTextDivideTextRunFunc devideFunc);

    bool _useFullSizeToGenerateLayout  = false;
    bool _allowLineBreakInWord         = false;
    bool _allowLineBreakBetweenScripts = true;

    CommonTextSystem* _textSystem = nullptr;
    std::shared_ptr<UsdImagingMarkupText> _markupText = nullptr;
    std::shared_ptr<CommonTextGenericLayout> _genericLayout = nullptr;
    std::shared_ptr<CommonTextIntermediateInfo> _intermediateInfo = nullptr;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GENERIC_LAYOUT_H
