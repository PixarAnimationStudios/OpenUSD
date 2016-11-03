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
#include "usdMaya/AttributeConverterRegistry.h"

#include "pxr/base/tf/registryManager.h" 

static std::vector<AttributeConverter*> _reg;

/* static */
void
AttributeConverterRegistry::Register(AttributeConverter* converter) {
    _reg.push_back(converter);
}

/* static */
std::vector<const AttributeConverter*>
AttributeConverterRegistry::GetAllConverters() {
    TfRegistryManager::GetInstance().SubscribeTo<AttributeConverterRegistry>();
    std::vector<const AttributeConverter*> ret;
    for (AttributeConverter* converter : _reg) {
        ret.push_back(converter);
    }
    return ret;
}
