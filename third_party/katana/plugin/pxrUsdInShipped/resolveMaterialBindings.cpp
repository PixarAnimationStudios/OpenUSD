//
// Copyright 2019 Pixar
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
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnAttribute/FnGroupBuilder.h>

class PxrUsdInResolveMaterialBindingsOp : public FnKat::GeolibOp
{
public:

    static void setup(FnKat::GeolibSetupInterface &interface)
    {
        interface.setThreading(
                FnKat::GeolibSetupInterface::ThreadModeConcurrent);   
    }

    static void cook(FnKat::GeolibCookInterface &interface)
    {
        static const char * kAttrPathArgName = "a";
        static const char * kOmitIfSameArgName = "o";
        static const char * kParentValueArgName = "p";
        
        FnAttribute::StringAttribute bindingAttrPathAttr;
        
        if (interface.atRoot())
        {
            // transfer the raw opArgs into traversal form
            
            std::string purposeValue = FnAttribute::StringAttribute(
                    interface.getOpArg("purpose")).getValue(
                            "allPurpose", false);
            if (purposeValue.empty())
            {
                purposeValue = "allPurpose";
            }

            bindingAttrPathAttr = FnAttribute::StringAttribute(
                    "usd.materialBindings." + purposeValue);

            
            interface.replaceChildTraversalOp("",
                FnAttribute::GroupBuilder()
                    .update(interface.getOpArg())
                    .set(kAttrPathArgName, bindingAttrPathAttr)
                    .set(kOmitIfSameArgName,
                            interface.getOpArg("omitIfParentValueMatches"))
                    .build());
            
            // Katana scene root should never have a USD binding so exit early
            // here to avoid needing to potentially combine child traveral op
            // args based on the pareent value
            return;
        }
        else
        {
            bindingAttrPathAttr = interface.getOpArg(kAttrPathArgName);
        }

        FnAttribute::StringAttribute bindingValue =
                interface.getAttr(bindingAttrPathAttr.getValueCStr());
        if (bindingValue.getNumberOfValues() != 1)
        {
            return;
        }

        if (FnAttribute::IntAttribute(
                interface.getOpArg(kOmitIfSameArgName)).getValue(0, false))
        {
            // value the same? We don't need to set it and we don't need to
            // replace it in the op args
            if (bindingValue == interface.getOpArg(kParentValueArgName))
            {
                return;
            }

            interface.replaceChildTraversalOp("",
                FnAttribute::GroupBuilder()
                    .update(interface.getOpArg())
                    .set(kParentValueArgName, bindingValue)
                    .build());
        }
        
        // TODO: potentially consider what to do if "materialAssign" already
        //       exists locally.

        // if we've reached here, set the attr
        interface.setAttr("materialAssign", bindingValue);
    }


};


DEFINE_GEOLIBOP_PLUGIN(PxrUsdInResolveMaterialBindingsOp)

void registerPxrUsdInResolveMaterialBindingsOp()
{
    REGISTER_PLUGIN(PxrUsdInResolveMaterialBindingsOp,
            "PxrUsdInResolveMaterialBindings", 0, 1);
}
