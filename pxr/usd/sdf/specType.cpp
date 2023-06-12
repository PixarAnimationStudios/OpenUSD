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
/// \file SpecType.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/specType.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/spec.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/bigRWMutex.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/tf/hashmap.h"

#include <map>
#include <thread>
#include <utility>
#include <vector>

using std::pair;
using std::make_pair;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static constexpr size_t
_GetBitmaskForSpecType(SdfSpecType specType)
{
    return (size_t(1) << specType);
}

struct Sdf_SpecTypeInfo
{
    // GetInstance() waits until initial registration has completed to support
    // query operations.  Call GetInstanceForRegistration() to do registrations.
    static Sdf_SpecTypeInfo& GetInstance() {
        Sdf_SpecTypeInfo &instance =
            TfSingleton<Sdf_SpecTypeInfo>::GetInstance();
        while (!instance.initialRegistrationCompleted) {
            std::this_thread::yield();
        }
        return instance;
    }

    // Return the instance directly, without waiting for initial registrations.
    // Use this form for doing registrations, and GetInstance() for queries.
    static Sdf_SpecTypeInfo& GetInstanceForRegistration() { 
        return TfSingleton<Sdf_SpecTypeInfo>::GetInstance(); 
    }

    // Mapping from C++ spec type to bitmask indicating the possible source
    // spec types. This table lets us answer the question, "If I have an
    // spec whose SdfSpecType is X, can I create the C++ spec type Y from
    // that?" For example, a possible entry in this table could be 
    // (SdfPrimSpec, SdfSpecTypePrim), indicating that consumers can create
    // an SdfPrimSpec from any spec whose spec type is SdfSpecTypePrim.
    typedef TfHashMap<TfType, size_t, TfHash> SpecTypeToBitmask;
    SpecTypeToBitmask specTypeToBitmask;
    
    // Mapping from schema class to mapping from SdfSpecType to spec class.
    // In other words, for a given schema and spec type, what is the 
    // corresponding C++ spec class?
    typedef std::vector<TfType> SpecTypeToTfType;
    typedef TfHashMap<TfType, SpecTypeToTfType, TfHash> SchemaTypeToSpecTypes;
    SchemaTypeToSpecTypes schemaTypeToSpecTypes;

    // Mapping from spec class to schema classes. In other words, what schemas
    // are associated with a given C++ spec class.
    typedef std::vector<TfType> SchemaTypes;
    typedef TfHashMap<TfType, SchemaTypes, TfHash> SpecTypeToSchemaTypes;
    SpecTypeToSchemaTypes specTypeToSchemaTypes;

    std::atomic<bool> initialRegistrationCompleted;
    mutable TfBigRWMutex mutex;

private:
    friend class TfSingleton<Sdf_SpecTypeInfo>;

    Sdf_SpecTypeInfo()
        : specTypeToBitmask(0)
        , initialRegistrationCompleted(false)
    { 
        TfSingleton<Sdf_SpecTypeInfo>::SetInstanceConstructed(*this);
        
        TfRegistryManager::GetInstance().SubscribeTo<SdfSpecTypeRegistration>();
        // Basic registration is complete.  Note, however that this does not
        // account for registrations from downstream libraries like Sd.  See bug
        // 111728.
        initialRegistrationCompleted = true;
    }
};

TF_INSTANTIATE_SINGLETON(Sdf_SpecTypeInfo);

// In abstract registrations, specEnumType is SdfSpecTypeUnknown.
void
SdfSpecTypeRegistration::_RegisterSpecType(
    const std::type_info& specCPPType,
    SdfSpecType specEnumType,
    const std::type_info& schemaType)
{
    const bool isConcrete = specEnumType != SdfSpecTypeUnknown;
    Sdf_SpecTypeInfo& specTypeInfo =
        Sdf_SpecTypeInfo::GetInstanceForRegistration();
    
    const TfType& schemaTfType = TfType::Find(schemaType);
    if (schemaTfType.IsUnknown()) {
        TF_CODING_ERROR(
            "Schema type %s must be registered with the TfType system.",
            ArchGetDemangled(schemaType).c_str());
        return;
    }

    const TfType& specTfType = TfType::Find(specCPPType);
    if (specTfType.IsUnknown()) {
        TF_CODING_ERROR(
            "Spec type %s must be registered with the TfType system.",
            ArchGetDemangled(specCPPType).c_str());
        return;
    }

    TfBigRWMutex::ScopedLock lock(specTypeInfo.mutex);
    
    Sdf_SpecTypeInfo::SpecTypeToBitmask::iterator specEntry =
        specTypeInfo.specTypeToBitmask.insert({specTfType, 0}).first;
    
    size_t &specAllowedBitmask = specEntry->second;

    // Check every entry currently in the specTypeToBitmask (including the
    // one that was just added above) and indicate whether each spec type
    // can be created from the spec type we're registering.
    TF_FOR_ALL(it, specTypeInfo.specTypeToBitmask) {
        if (isConcrete && specTfType.IsA(it->first)) {
            it->second |= _GetBitmaskForSpecType(specEnumType);
        } else if (it->first.IsA(specTfType)) {
            specAllowedBitmask |= it->second;
        }
    }

    // XXX: See comments in Sdf_SpecType::Cast
    if (specEnumType == SdfSpecTypePrim) {
        specAllowedBitmask |= _GetBitmaskForSpecType(SdfSpecTypeVariant);
    }

    if (isConcrete) {
        Sdf_SpecTypeInfo::SpecTypeToTfType& specTypeToTfType = 
            specTypeInfo.schemaTypeToSpecTypes[schemaTfType];
        if (specTypeToTfType.empty()) {
            specTypeToTfType.resize(SdfNumSpecTypes);
        }
        specTypeToTfType[specEnumType] = specTfType;
    }

    Sdf_SpecTypeInfo::SchemaTypes& schemaTypesForSpecType = 
        specTypeInfo.specTypeToSchemaTypes[specTfType];
    if (std::find(schemaTypesForSpecType.begin(),
                  schemaTypesForSpecType.end(), schemaTfType) 
            == schemaTypesForSpecType.end()) {
        schemaTypesForSpecType.push_back(schemaTfType);
    }
    else {
        TF_CODING_ERROR(
            "Spec type %s already registered for schema type %s",
            specTfType.GetTypeName().c_str(), 
            schemaTfType.GetTypeName().c_str());
    }        
}

static bool
_CanCast(const Sdf_SpecTypeInfo &specTypeInfo,
         SdfSpecType fromType, const TfType& toType)
{
    if (toType.IsUnknown()) {
        return TfType();
    }
    const size_t allowedBitmask = 
        TfMapLookupByValue(specTypeInfo.specTypeToBitmask, toType, size_t(0));
    return allowedBitmask & _GetBitmaskForSpecType(fromType);
}

TfType
Sdf_SpecType::Cast(const SdfSpec& from, const std::type_info& to)
{
    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();

    const TfType& schemaType = TfType::Find(typeid(from.GetSchema()));
    if (!TF_VERIFY(!schemaType.IsUnknown())) {
        return TfType();
    }

    const SdfSpecType fromType = from.GetSpecType();
    const TfType& toType = TfType::Find(to);

    TfBigRWMutex::ScopedLock lock(specTypeInfo.mutex, /*write=*/false);

    if (!_CanCast(specTypeInfo, fromType, toType)) {
        return TfType();
    }

    const Sdf_SpecTypeInfo::SpecTypeToTfType &specTypeToTfType = 
        *TfMapLookupPtr(specTypeInfo.schemaTypeToSpecTypes, schemaType);

    // Allow casting to go through if we're trying to cast from a
    // variant spec to a prim spec. 
    //
    // XXX: This is required to allow variant specs to be treated as prim
    //      specs. However, if we're going to do that, shouldn't we just make
    //      variant specs derive from prim specs?
    if (fromType == SdfSpecTypeVariant) {
        const TfType primSpecType = specTypeToTfType[SdfSpecTypePrim];
        if (toType == primSpecType) {
            return toType;
        }
    }

    return specTypeToTfType[fromType];
}

bool 
Sdf_SpecType::CanCast(SdfSpecType fromType, const std::type_info& to)
{
    const Sdf_SpecTypeInfo &specTypeInfo = Sdf_SpecTypeInfo::GetInstance();
    TfType toType = TfType::Find(to);
    TfBigRWMutex::ScopedLock lock(specTypeInfo.mutex, /*write=*/false);
    return _CanCast(specTypeInfo, fromType, toType);
}

bool 
Sdf_SpecType::CanCast(const SdfSpec& from, const std::type_info& to)
{
    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();

    const SdfSpecType fromType = from.GetSpecType();
    const TfType& toType = TfType::Find(to);

    const TfType& fromSchemaType = TfType::Find(typeid(from.GetSchema()));

    TfBigRWMutex::ScopedLock lock(specTypeInfo.mutex, /*write=*/false);

    if (!_CanCast(specTypeInfo, fromType, toType)) {
        return false;
    }

    const Sdf_SpecTypeInfo::SchemaTypes* toSchemaTypes = 
        TfMapLookupPtr(specTypeInfo.specTypeToSchemaTypes, toType);
    if (!toSchemaTypes) {
        return false;
    }

    for (const TfType& toSchemaType : *toSchemaTypes) {
        if (fromSchemaType.IsA(toSchemaType)) {
            return true;
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
