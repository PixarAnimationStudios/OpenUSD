//
// Copyright 2017 Pixar
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
#include "usdKatana/usdInPluginRegistry.h"
#include <FnAttributeFunction/plugin/FnAttributeFunctionPlugin.h>

extern void registerPrmanGeometryDecorator();

// XXX need to register a katana plug-in for katana to load the .so
//     If we were a plug-in which was already registering Ops, etc, this
//     wouldn't be necessary.
class DummyFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args)
    {
        return FnAttribute::Attribute();
    }
};
DEFINE_ATTRIBUTEFUNCTION_PLUGIN(DummyFnc);


void registerPlugins()
{
    // XXX need to register a katana plug-in for katana to load the .so
    REGISTER_PLUGIN(DummyFnc, "__noticeMe", 0, 1);
    
    
    
    registerPrmanGeometryDecorator();
}
