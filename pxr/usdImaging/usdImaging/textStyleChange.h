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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_CHANGE_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_CHANGE_H

/// \file usdImaging/textStyleChange.h

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// A unordered_map for key and UsdImagingTextStyle.
typedef std::unordered_map<int, std::shared_ptr<UsdImagingTextStyle>> TextStyleMap;

/// \enum class UsdImagingTextProperty
///
/// The type of text properties.
///
enum class UsdImagingTextProperty
{
    UsdImagingTextPropertyTypeface,
    UsdImagingTextPropertyBold,
    UsdImagingTextPropertyItalic,
    UsdImagingTextPropertyHeight,
    UsdImagingTextPropertyWidthFactor,
    UsdImagingTextPropertyObliqueAngle,
    UsdImagingTextPropertyCharacterSpaceFactor,
    UsdImagingTextPropertyUnderlineType,
    UsdImagingTextPropertyOverlineType,
    UsdImagingTextPropertyStrikethroughType
};

/// \struct UsdImagingTextStyleChange
///
/// The style change of text.
///
struct UsdImagingTextStyleChange
{
public:
    UsdImagingTextProperty _changeType = UsdImagingTextProperty::UsdImagingTextPropertyTypeface;
    union
    {
        bool _boolValue;
        int _intValue = 0;
        float _floatValue;
    };
    std::shared_ptr<std::string> _stringValue;

    /// The constructor
    USDIMAGING_API
    UsdImagingTextStyleChange() = default;

    /// The copy constructor
    USDIMAGING_API
    UsdImagingTextStyleChange(const UsdImagingTextStyleChange& other) { *this = other; }

    /// The destructor
    USDIMAGING_API
    ~UsdImagingTextStyleChange() = default;

    /// operator =
    USDIMAGING_API
    UsdImagingTextStyleChange& operator=(const UsdImagingTextStyleChange& other)
    {
        _changeType = other._changeType;
        switch (_changeType)
        {
        case UsdImagingTextProperty::UsdImagingTextPropertyTypeface:
        case UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType:
        case UsdImagingTextProperty::UsdImagingTextPropertyOverlineType:
        case UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType:
        {
            _stringValue = other._stringValue;
            break;
        }
        case UsdImagingTextProperty::UsdImagingTextPropertyBold:
        case UsdImagingTextProperty::UsdImagingTextPropertyItalic:
            _boolValue = other._boolValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyHeight:
            _intValue = other._intValue;
            break;
        case UsdImagingTextProperty::UsdImagingTextPropertyWidthFactor:
        case UsdImagingTextProperty::UsdImagingTextPropertyObliqueAngle:
        case UsdImagingTextProperty::UsdImagingTextPropertyCharacterSpaceFactor:
            _floatValue = other._floatValue;
            break;
        default:
            break;
        }
        return *this;
    }

    /// operator ==.
    USDIMAGING_API
    bool operator==(const UsdImagingTextStyleChange& other) const
    {
        if (_changeType != other._changeType)
            return false;
        else
        {
            switch (_changeType)
            {
            case UsdImagingTextProperty::UsdImagingTextPropertyTypeface:
            case UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType:
            case UsdImagingTextProperty::UsdImagingTextPropertyOverlineType:
            case UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType:
                return _stringValue == other._stringValue ||
                    (_stringValue && other._stringValue &&
                        _stringValue->compare(*(other._stringValue.get())));
            case UsdImagingTextProperty::UsdImagingTextPropertyBold:
            case UsdImagingTextProperty::UsdImagingTextPropertyItalic:
                return _boolValue == other._boolValue;
            case UsdImagingTextProperty::UsdImagingTextPropertyHeight:
                return _intValue == other._intValue;
            case UsdImagingTextProperty::UsdImagingTextPropertyWidthFactor:
            case UsdImagingTextProperty::UsdImagingTextPropertyObliqueAngle:
            case UsdImagingTextProperty::UsdImagingTextPropertyCharacterSpaceFactor:
            {
                static const float epsilon = 1e-10f;
                return GfIsClose(_floatValue, other._floatValue, epsilon);
            }
            default:
                return false;
            }
        }
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_STYLE_CHANGE_H
