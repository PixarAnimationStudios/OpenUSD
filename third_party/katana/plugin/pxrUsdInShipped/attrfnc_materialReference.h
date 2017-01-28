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
#include <FnAttributeFunction/plugin/FnAttributeFunctionPlugin.h>
#include <FnGeolib/util/AttributeKeyedCache.h>


// Cache for the UsdMaterialReference AttributeFunction
class MaterialReferenceAttrFncCache : public FnGeolibUtil::AttributeKeyedCache<
FnAttribute::GroupAttribute >
{
public:
    MaterialReferenceAttrFncCache() : FnGeolibUtil::AttributeKeyedCache<
        FnAttribute::GroupAttribute>() {}
protected:
    IMPLPtr createValue(const FnAttribute::Attribute & attr);
};


class MaterialReferenceAttrFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args);
    static void flush();
};



// Cache for the LibraryMaterialNames AttributeFunction
class LibraryMaterialNamesAttrFncCache : public FnGeolibUtil::AttributeKeyedCache<
FnAttribute::StringAttribute >
{
public:
    LibraryMaterialNamesAttrFncCache() : FnGeolibUtil::AttributeKeyedCache<
        FnAttribute::StringAttribute>() {}
protected:
    IMPLPtr createValue(const FnAttribute::Attribute & attr);
};


class LibraryMaterialNamesAttrFnc : public Foundry::Katana::AttributeFunction
{
public:
    static FnAttribute::Attribute run(FnAttribute::Attribute args);
    static void flush();
};
