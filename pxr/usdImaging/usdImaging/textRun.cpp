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

#include "pxr/usdImaging/usdImaging/textRun.h"

PXR_NAMESPACE_OPEN_SCOPE

void 
UsdImagingTextRun::AddStyleChange(const UsdImagingTextStyleChange& change)
{
    for (auto&& oldChange : _styleChangeArray)
    {
        // Find if the kind of StyleChange is already in the vector. If yes, update the item.
        if (oldChange._changeType == change._changeType)
        {
            switch (change._changeType)
            {
            case UsdImagingTextProperty::UsdImagingTextPropertyTypeface:
            case UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType:
            case UsdImagingTextProperty::UsdImagingTextPropertyOverlineType:
            case UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType:
                oldChange._stringValue = change._stringValue;
                return;
            case UsdImagingTextProperty::UsdImagingTextPropertyBold:
            case UsdImagingTextProperty::UsdImagingTextPropertyItalic:
                oldChange._boolValue = change._boolValue;
                return;
            case UsdImagingTextProperty::UsdImagingTextPropertyHeight:
                oldChange._intValue = change._intValue;
                return;
            case UsdImagingTextProperty::UsdImagingTextPropertyWidthFactor:
            case UsdImagingTextProperty::UsdImagingTextPropertyObliqueAngle:
            case UsdImagingTextProperty::UsdImagingTextPropertyCharacterSpaceFactor:
                oldChange._floatValue = change._floatValue;
                return;
            default:
                return;
            }
        }
    }
    // If there is no such kind of StyleChange, add it.
    _styleChangeArray.push_back(change);
}

UsdImagingTextStyle 
UsdImagingTextRun::GetStyle(const UsdImagingTextStyle& parentStyle) const
{
    // First clone the parentStyle, and then use the StyleChange array to modify the style.
    UsdImagingTextStyle style = parentStyle;
    for (auto&& change : _styleChangeArray)
    {
        switch (change._changeType)
        {
        case UsdImagingTextProperty::UsdImagingTextPropertyTypeface:
        {
            if (change._stringValue && !change._stringValue->empty())
                style._typeface = *(change._stringValue);
            break;
        }
        case UsdImagingTextProperty::UsdImagingTextPropertyBold:
            style._bold = change._boolValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyItalic:
            style._italic = change._boolValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyHeight:
            style._height = change._intValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyWidthFactor:
            style._widthFactor = change._floatValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyObliqueAngle:
            style._obliqueAngle = change._floatValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyCharacterSpaceFactor:
            style._characterSpaceFactor = change._floatValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType:
            style._underlineType = TfToken(*change._stringValue);
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyOverlineType:
            style._overlineType = TfToken(*change._stringValue);
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType:
            style._strikethroughType = TfToken(*change._stringValue);
            break;
        default:
            break;
        };
    }
    return style;
}

bool
UsdImagingTextRun::CopyPartOfRun(const UsdImagingTextRun& fromRun, 
                                 int startOffset, 
                                 int length)
{
    // Copy the text string data.
    if (!CopyPartOfData(fromRun, startOffset, length))
        return false;

    // Copy the text style.
    if (!CopyStyle(fromRun))
        return false;
    return true;
}

bool
UsdImagingTextRun::CopyPartOfData(const UsdImagingTextRun& fromRun,
                                  int startOffset,
                                  int length)
{
    _type = fromRun._type;
    _startIndex  = fromRun._startIndex + startOffset;
    _length      = length;

    return true;
}

bool
UsdImagingTextRun::CopyStyle(const UsdImagingTextRun& fromRun)
{
    _styleChangeArray.clear();

    for (int i = 0; i < fromRun._styleChangeArray.size(); ++i)
    {
        UsdImagingTextStyleChange pStyleChange = fromRun._styleChangeArray.at(i);
        _styleChangeArray.push_back(std::move(pStyleChange));
    }

    if (fromRun._textColor)
        _textColor = std::make_unique<UsdImagingTextColor>(*fromRun._textColor);
    else if (_textColor)
        _textColor = nullptr;

    return true;
}

void 
UsdImagingTextRun::Shorten(int newLength)
{
    assert(newLength <= _length);
    _length = newLength;
}
PXR_NAMESPACE_CLOSE_SCOPE
