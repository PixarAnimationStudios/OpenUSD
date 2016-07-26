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
#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>
#include <FnPluginSystem/FnPlugin.h>
#include <FnDefaultAttributeProducer/plugin/FnDefaultAttributeProducerPlugin.h>
#include <FnDefaultAttributeProducer/plugin/FnDefaultAttributeProducerUtil.h>



namespace
{

// Allows for attr hints to be described via attrs. This is used by
// PxrUsdInVariantSelect to populate its pop-up menus with contextually
// relevant values.
class PxrUsdInUtilExtraHintsDap :
        public FnDefaultAttributeProducer::DefaultAttributeProducer
{
public:
    static FnAttribute::GroupAttribute cook(
            const FnGeolibOp::GeolibCookInterface & interface,
            const std::string & attrRoot,
            const std::string & inputLocationPath,
            int inputIndex)
    {
        FnAttribute::GroupAttribute entries =
                interface.getAttr("__pxrUsdInExtraHints");
        
        if (!entries.isValid() || entries.getNumberOfChildren() == 0)
        {
            return FnAttribute::GroupAttribute();
        }
        
        // encoding is attrPath -> groupAttr
        // attrPath is encoded via DelimiterEncode
        
        FnAttribute::GroupBuilder gb;
        for (int64_t i = 0, e = entries.getNumberOfChildren(); i < e; ++i)
        {
            FnAttribute::GroupAttribute hintsAttr = entries.getChildByIndex(i);
            if (!hintsAttr.isValid())
            {
                continue;
            }
            
            
            FnDefaultAttributeProducer::DapUtil::SetAttrHints(
                    gb, FnAttribute::DelimiterDecode(entries.getChildName(i)),
                            hintsAttr);
        }
        return gb.build();
    }
};

DEFINE_DEFAULTATTRIBUTEPRODUCER_PLUGIN(PxrUsdInUtilExtraHintsDap)

}

void registerPxrUsdInShippedUiUtils()
{
    REGISTER_PLUGIN(PxrUsdInUtilExtraHintsDap,
            "PxrUsdInUtilExtraHintsDap", 0, 1);
}