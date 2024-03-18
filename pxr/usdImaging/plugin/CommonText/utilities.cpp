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

#include "utilities.h"
#include "fontDevice.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE
/// Convert the height to full size of the font, and get the scale ratio.
bool
CommonTextUtilities::GetFullSizeStyle(
    UsdImagingTextStyle& style,
    float& scaleRatio)
{
    // Get the fullSize of the font.
    CommonTextTrueTypeFontDevicePtr fontDevice(style);
    if (fontDevice.IsValid())
    {
        int fullSize = TRUETYPE_COMMON_FONT_FULL_SIZE;
        if (fontDevice->QueryFullSize(fullSize) != CommonTextStatus::CommonTextStatusSuccess)
        {
            fullSize = TRUETYPE_COMMON_FONT_FULL_SIZE;
        }
        // The scale is the ratio between textHeight and fullSize.
        scaleRatio = (float)style._height / fullSize;
        // Set the fullSize of the font to the height.
        style._height = fullSize;
        return true;
    }
    else
        return false;
}
PXR_NAMESPACE_CLOSE_SCOPE
