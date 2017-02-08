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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/tf/hashmap.h"

#include <map>
#include <utility>
#include <vector>

using std::pair;
using std::make_pair;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static inline size_t
_GetBitmaskForSpecType(SdfSpecType specType)
{
    return (1 << specType);
}

struct Sdf_SpecTypeInfo
{
    static Sdf_SpecTypeInfo& GetInstance()
    { 
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
    
    // Cache of type_info -> TfType used during cast operations to avoid TfType
    // lookups.  This speeds up these operations, especially when run
    // concurrently since TfType has a global lock.
    typedef vector<pair<std::type_info const *, TfType> > SpecTypeInfoToTfType;
    SpecTypeInfoToTfType specTypeInfoToTfType;

    // Mapping from schema class to mapping from SdfSpecType to spec class.
    // In other words, for a given schema and spec type, what is the 
    // corresponding C++ spec class?
    typedef std::vector<TfType> SpecTypeToTfType;
    typedef TfHashMap<TfType, SpecTypeToTfType, TfHash> 
        SchemaTypeToSpecTypes;
    SchemaTypeToSpecTypes schemaTypeToSpecTypes;

    // Mapping from spec class to schema class. In other words, what schema
    // is associated with a given C++ spec class.
    typedef TfHashMap<TfType, TfType, TfHash> SpecTypeToSchemaType;
    SpecTypeToSchemaType specTypeToSchemaType;

    std::atomic<bool> registrationsCompleted;

    // Helper function for creating an entry in the specTypeToBitmask table.
    SpecTypeToBitmask::iterator
    CreateSpecTypeEntry(const std::type_info& specCPPType)
    {
        const TfType& specTfType = TfType::Find(specCPPType);
        if (specTfType.IsUnknown()) {
            TF_CODING_ERROR(
                "Spec type %s must be registered with the TfType system.",
                ArchGetDemangled(specCPPType).c_str());
            return specTypeToBitmask.end();
        }

        const std::pair<SpecTypeToBitmask::iterator, bool> mapStatus =
            specTypeToBitmask.insert(std::make_pair(specTfType, 0));
        if (!mapStatus.second) {
            TF_CODING_ERROR(
                "Duplicate registration for spec type %s.",
                specTfType.GetTypeName().c_str());
            return specTypeToBitmask.end();
        }

        // Add an entry into the specTypeInfoToTfType.
        specTypeInfoToTfType.push_back(make_pair(&specCPPType, specTfType));

        return mapStatus.first;
    }

    // Find the TfType corresponding to specCPPType.  This will look in the
    // specTypeInfoToTfType cache first to avoid hitting the TfType lock.
    inline TfType TfTypeFind(const std::type_info &specCPPtype) const {
        typedef pair<const std::type_info *, TfType> Pair;
        for (const auto& p : specTypeInfoToTfType) {
            if (p.first == &specCPPtype)
                return p.second;
        }
        return TfType::Find(specCPPtype);
    }        

private:
    friend class TfSingleton<Sdf_SpecTypeInfo>;

    Sdf_SpecTypeInfo()
        : specTypeToBitmask(0)
        , registrationsCompleted(false)
    { 
        TfSingleton<Sdf_SpecTypeInfo>::SetInstanceConstructed(*this);
        TfRegistryManager::GetInstance().SubscribeTo<SdfSpecTypeRegistration>();
        // Basic registration is complete.  Note, however that this does not
        // account for registrations from downstream libraries like Sd.  See bug
        // 111728.
        registrationsCompleted = true;
    }
};

TF_INSTANTIATE_SINGLETON(Sdf_SpecTypeInfo);

void
SdfSpecTypeRegistration::_RegisterSpecType(
    const std::type_info& specCPPType,
    SdfSpecType specEnumType,
    const std::type_info& schemaType)
{ 
    Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();
    
    const TfType& schemaTfType = specTypeInfo.TfTypeFind(schemaType);
    if (schemaTfType.IsUnknown()) {
        TF_CODING_ERROR(
            "Schema type %s must be registered with the TfType system.",
            ArchGetDemangled(schemaType).c_str());
    }

    Sdf_SpecTypeInfo::SpecTypeToBitmask::iterator specEntry = 
        specTypeInfo.CreateSpecTypeEntry(specCPPType);
    if (specEntry == specTypeInfo.specTypeToBitmask.end()) {
        // Error already emitted, bail out.
        return;
    }

    const TfType& specTfType = specEntry->first;
    size_t& specAllowedBitmask = specEntry->second;

    // Check every entry currently in the specTypeToBitmask (including the
    // one that was just added above) and indicate whether each spec type
    // can be created from the spec type we're registering.
    TF_FOR_ALL(it, specTypeInfo.specTypeToBitmask) {
        if (specTfType.IsA(it->first))
            it->second |= _GetBitmaskForSpecType(specEnumType);
        else if (it->first.IsA(specTfType))
            specAllowedBitmask |= it->second;
    }

    // XXX: See comments in Sdf_SpecType::Cast
    if (specEnumType == SdfSpecTypePrim) {
        specAllowedBitmask |= _GetBitmaskForSpecType(SdfSpecTypeVariant);
    }

    Sdf_SpecTypeInfo::SpecTypeToTfType& specTypeToTfType = 
        specTypeInfo.schemaTypeToSpecTypes[schemaTfType];
    if (specTypeToTfType.empty()) {
        specTypeToTfType.resize(SdfNumSpecTypes);
    }
    specTypeToTfType[specEnumType] = specTfType;

    specTypeInfo.specTypeToSchemaType[specTfType] = schemaTfType;
}

void
SdfSpecTypeRegistration::_RegisterAbstractSpecType(
    const std::type_info& specCPPType,
    const std::type_info& schemaType)
{
    Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();

    const TfType& schemaTfType = specTypeInfo.TfTypeFind(schemaType);
    if (schemaTfType.IsUnknown()) {
        TF_CODING_ERROR(
            "Schema type %s must be registered with the TfType system.",
            ArchGetDemangled(schemaType).c_str());
    }

    Sdf_SpecTypeInfo::SpecTypeToBitmask::iterator specEntry = 
        specTypeInfo.CreateSpecTypeEntry(specCPPType);
    if (specEntry == specTypeInfo.specTypeToBitmask.end()) {
        // Error already emitted, bail out.
        return;
    }

    const TfType& specTfType = specEntry->first;
    size_t& specAllowedBitmask = specEntry->second;

    // Check every entry currently in the specTypeToBitmask (including the
    // one that was just added above) and indicate whether each spec type
    // can be created from the spec type we're registering.
    TF_FOR_ALL(it, specTypeInfo.specTypeToBitmask) {
        if (it->first.IsA(specTfType))
            specAllowedBitmask |= it->second;
    }

    specTypeInfo.specTypeToSchemaType[specTfType] = schemaTfType;
}

// XXX: Note, this function must be invoked by all public API in order to wait
// on basic registry initialization before accessing the registry contents.
static bool
_CanCast(SdfSpecType fromType, const TfType& toType)
{
    if (toType.IsUnknown()) {
        return TfType();
    }

    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();
    
    while (!specTypeInfo.registrationsCompleted) {
        // spin until registration has completed.
    }

    const size_t allowedBitmask = 
        TfMapLookupByValue(specTypeInfo.specTypeToBitmask, toType, 0);
    return allowedBitmask & _GetBitmaskForSpecType(fromType);
}

TfType
Sdf_SpecType::Cast(const SdfSpec& from, const std::type_info& to)
{
    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();

    const SdfSpecType fromType = from.GetSpecType();
    const TfType& toType = specTypeInfo.TfTypeFind(to);
    if (!_CanCast(fromType, toType)) {
        return TfType();
    }

    const TfType& schemaType = TfType::Find(typeid(from.GetSchema()));
    if (!TF_VERIFY(!schemaType.IsUnknown())) {
        return TfType();
    }

    const Sdf_SpecTypeInfo::SpecTypeToTfType* specTypeToTfType = 
        TfMapLookupPtr(specTypeInfo.schemaTypeToSpecTypes, schemaType);

    // Allow casting to go through if we're trying to cast from a
    // variant spec to a prim spec. 
    //
    // XXX: This is required to allow variant specs to be treated as prim
    //      specs. However, if we're going to do that, shouldn't we just make
    //      variant specs derive from prim specs?
    if (fromType == SdfSpecTypeVariant) {
        const TfType primSpecType = (*specTypeToTfType)[SdfSpecTypePrim];
        if (toType == primSpecType) {
            return toType;
        }
    }

    return (*specTypeToTfType)[fromType];
}

bool 
Sdf_SpecType::CanCast(SdfSpecType fromType, const std::type_info& to)
{
    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();
    const TfType& toType = specTypeInfo.TfTypeFind(to);
    return _CanCast(fromType, toType);
}

bool 
Sdf_SpecType::CanCast(const SdfSpec& from, const std::type_info& to)
{
    const Sdf_SpecTypeInfo& specTypeInfo = Sdf_SpecTypeInfo::GetInstance();

    const SdfSpecType fromType = from.GetSpecType();
    const TfType& toType = specTypeInfo.TfTypeFind(to);
    if (!_CanCast(fromType, toType)) {
        return false;
    }

    const TfType& fromSchemaType = TfType::Find(typeid(from.GetSchema()));
    const TfType* toSchemaType = 
        TfMapLookupPtr(specTypeInfo.specTypeToSchemaType, toType);
    if (!toSchemaType || !fromSchemaType.IsA(*toSchemaType)) {
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
