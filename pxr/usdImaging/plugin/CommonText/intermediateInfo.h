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
#ifndef PXR_USD_IMAGING_COMMON_TEXT_INTERMEDIATE_INFO_H
#define PXR_USD_IMAGING_COMMON_TEXT_INTERMEDIATE_INFO_H

#include "definitions.h"
#include "globals.h"

namespace std
{
    /// The std::hash implementation for UsdImagingTextRunList::iterator.
    template <>
    struct hash<PXR_INTERNAL_NS::UsdImagingTextRunList::iterator>
    {
        std::size_t operator()(const PXR_INTERNAL_NS::UsdImagingTextRunList::iterator& s) const noexcept
        {
            const PXR_INTERNAL_NS::UsdImagingTextRun& textRun = *s;
            return textRun.StartIndex();
        }
    };

    /// The std::hash implementation for UsdImagingTextLineList::iterator.
    template <>
    struct hash<PXR_INTERNAL_NS::UsdImagingTextLineList::iterator>
    {
        std::size_t operator()(const PXR_INTERNAL_NS::UsdImagingTextLineList::iterator& s) const noexcept
        {
            const PXR_INTERNAL_NS::UsdImagingTextLine& textLine = *s;
            const PXR_INTERNAL_NS::UsdImagingTextRunRange& range = textLine.Range();
            assert(!range._isEmpty);
            const PXR_INTERNAL_NS::UsdImagingTextRun& textRun = *(range._firstRun);
            return textRun.StartIndex();
        }
    };
} // namespace std

PXR_NAMESPACE_OPEN_SCOPE
class CommonTextTrueTypeGenericLayoutManager;

/// \class CommonTextComplexScriptInfo
///
/// The information of the complex script in the string.
///
class CommonTextComplexScriptInfo
{
protected:
    // The complex script attributes.
    // There can be no attributes when there is no complex script.
    int _lengthOfAttributes = 0;
    char* _attributes = nullptr;

public:
    /// Constructor.
    CommonTextComplexScriptInfo() = default;

    /// Copy constructor.
    CommonTextComplexScriptInfo(const CommonTextComplexScriptInfo& fromInfo)
    {
        // Copy mAttributes
        _lengthOfAttributes = fromInfo._lengthOfAttributes;
        if (_lengthOfAttributes != 0)
        {
            _attributes = new char[fromInfo._lengthOfAttributes];
            memcpy(_attributes, fromInfo._attributes,
                sizeof(char) * fromInfo._lengthOfAttributes);
        }
        else
            _attributes = nullptr;
    }

    /// Destructor.
    ~CommonTextComplexScriptInfo()
    {
        if (_attributes != nullptr)
            delete[] _attributes;
    }

    /// Set the attributes of script.
    /// Only the multilanguage handler of the platform can understand the structure of
    /// attributes.
    void Attributes(int lengthOfAttributes, char* attributes)
    {
        if (_attributes != nullptr)
            delete[] _attributes;
        _lengthOfAttributes = lengthOfAttributes;
        _attributes = attributes;
    }

    /// Get the attributes of script.
    /// Only the multilanguage handler of the platform can understand the structure of
    /// attributes.
    inline char* Attributes() { return _attributes; }
};

/// \class CommonTextRunInfo
///
/// The information of TextRun when we generate layouts.
///
struct CommonTextRunInfo
{
public:
    /// The default constructor.
    CommonTextRunInfo() = default;

    /// The copy constructor
    CommonTextRunInfo(const CommonTextRunInfo& info) :
        _complexScriptInfo(info._complexScriptInfo),
        _scriptInfoArray(info._scriptInfoArray)
    {
    }

    /// The destructor
    ~CommonTextRunInfo() = default;

    /// Get the array which saves the indices of characters who are the start of a new script.
    inline std::vector<CommonTextScriptInfo>& GetScriptInfo() { return _scriptInfoArray; }

    /// Get the information of complex script
    inline std::shared_ptr<CommonTextComplexScriptInfo> ComplexScriptInformation()
    {
        return _complexScriptInfo;
    }

    /// Set the information of complex script
    inline void ComplexScriptInformation(std::shared_ptr<CommonTextComplexScriptInfo> info)
    {
        _complexScriptInfo = info;
    }

    /// Copy part of the TextRunInfo from the fromInfo.
    void CopyPartOfData(const CommonTextRunInfo& fromInfo,
        int startOffset,
        int length);

    /// Resize the TextRunInfo to a shorter length.
    void Shorten(int newLength)
    {
        int count = (int)_scriptInfoArray.size();
        for (int i = 0; i < count; i++)
        {
            if (_scriptInfoArray[i]._indexOfFirstCharacter >= newLength)
            {
                _scriptInfoArray.resize(i);
                break;
            }
        }
    }
private:
    /// The information of the complex script.
    std::shared_ptr<CommonTextComplexScriptInfo> _complexScriptInfo;

    /// The characters that is the start of a new script.
    std::vector<CommonTextScriptInfo> _scriptInfoArray;
};

typedef std::unordered_map<UsdImagingTextRunList::iterator, CommonTextRunInfo> TextRunToInfoMap;

/// \struct CommonTextWordBreakIndex
///
/// The word break indices in the text run.
///
struct CommonTextWordBreakIndex
{
    std::vector<int> _breakIndexInTextRun;
    std::vector<int> _addJustifyIndexInTextRun;

    /// The default constructor.
    CommonTextWordBreakIndex() = default;

    /// The copy constructor.
    CommonTextWordBreakIndex(const CommonTextWordBreakIndex & from)
    {
        _breakIndexInTextRun = from._breakIndexInTextRun;
        _addJustifyIndexInTextRun = from._addJustifyIndexInTextRun;
    }
};

typedef std::forward_list<CommonTextWordBreakIndex> WordBreakIndexList;
typedef std::unordered_map<UsdImagingTextLineList::iterator, WordBreakIndexList> TextLineToWordBreakListMap;

/// \class CommonTextIntermediateInfo
///
/// The intermediate information when generate the layout.
///
class CommonTextIntermediateInfo {

    friend CommonTextTrueTypeGenericLayoutManager;
public:
    /// The default constructor.
    CommonTextIntermediateInfo(std::shared_ptr<UsdImagingMarkupText> markupText);

    /// The destructor
    ~CommonTextIntermediateInfo() = default;

    /// Get the information for the TextRun..
    CommonTextRunInfo& GetTextRunInfo(UsdImagingTextRunList::iterator textRunIter)
    {
        TextRunToInfoMap::iterator textRunInfoIter = _textRunToInfoMap.find(textRunIter);
        assert(textRunInfoIter != _textRunToInfoMap.end());
        if (textRunInfoIter != _textRunToInfoMap.end())
            return textRunInfoIter->second;
        else
            return _defaultTextRunInfo;
    }

    /// Get the WordBreakIndexList for the line.
    WordBreakIndexList& GetWordBreakIndexList(UsdImagingTextLineList::iterator textLineIter)
    {
        TextLineToWordBreakListMap::iterator wordBreakIndexIter = _textLineToWordBreakListMap.find(textLineIter);
        assert(wordBreakIndexIter != _textLineToWordBreakListMap.end());
        if (wordBreakIndexIter != _textLineToWordBreakListMap.end())
            return wordBreakIndexIter->second;
        else
            return _defaultWordBreakIndexList;
    }

private:
    /// Add the information for the TextRun.
    void _AddTextRunInfo(UsdImagingTextRunList::iterator textRunIter, 
                        const CommonTextRunInfo& scriptInfo)
    {
        _textRunToInfoMap.emplace(textRunIter, scriptInfo);
    }

    /// Add the WordBreakIndexList for the line.
    void _AddWordBreakIndexList(UsdImagingTextLineList::iterator textLineIter, 
                                const WordBreakIndexList& wordBreakIndexList)
    {
        _textLineToWordBreakListMap.emplace(textLineIter, wordBreakIndexList);
    }

    TextRunToInfoMap _textRunToInfoMap;
    TextLineToWordBreakListMap _textLineToWordBreakListMap;

    static CommonTextRunInfo _defaultTextRunInfo;
    static WordBreakIndexList _defaultWordBreakIndexList;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_COMMON_TEXT_INTERMEDIATE_INFO_H
