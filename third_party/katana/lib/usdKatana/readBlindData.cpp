//
// Copyright 2016 Pixar
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
#include "usdKatana/attrMap.h"
#include "usdKatana/blindDataObject.h"
#include "usdKatana/readBlindData.h"
#include "usdKatana/utils.h"

#include <FnLogging/FnLogging.h>

FnLogSetup("UsdKatanaReadBlindData");

void
PxrUsdKatanaReadBlindData(
        const UsdKatanaBlindDataObject& kbd,
        PxrUsdKatanaAttrMap& attrs)
{
    std::vector<UsdProperty> blindProps = kbd.GetKbdAttributes();
    TF_FOR_ALL(blindPropIter, blindProps) {
        UsdProperty blindProp = *blindPropIter;
        if (blindProp.Is<UsdAttribute>()) {
            UsdAttribute blindAttr = blindProp.As<UsdAttribute>();
            VtValue vtValue;
            if (not blindAttr.Get(&vtValue)) {
                continue;
            }

            std::string attrName = 
                UsdKatanaBlindDataObject::GetKbdAttributeNameSpace(blindProp).GetString();

            // If the attribute has no namespace, then it should be a
            // top-level attribute and the name is simply the property 
            // 'base name'. Otherwise, attrName is the group attribute 
            // name, and we need to append onto it the group builder key.
            //
            if (attrName.empty())
            {
                attrName = blindProp.GetBaseName();
            }
            else
            {
                attrName += ".";
                attrName += UsdKatanaBlindDataObject::GetGroupBuilderKeyForProperty(blindProp);
            }

            // we set asShaderParam=true because we want the attribute to be
            // generated "as is", we *do not* want the prmanStatement style
            // "type"/"value" declaration to be created.
            attrs.set(attrName, 
                PxrUsdKatanaUtils::ConvertVtValueToKatAttr(
                    vtValue, 
                    /* asShaderParam */ true));
        }
    }
}

