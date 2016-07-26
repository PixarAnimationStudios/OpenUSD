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
#include "pxr/usd/sdf/valueTypeRegistry.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/valueTypePrivate.h"
#include "pxr/usd/sdf/types.h" // For SdfDimensionlessUnitDefault
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include <boost/functional/hash.hpp>

#include <tbb/spin_rw_mutex.h>

namespace {

const Sdf_ValueTypePrivate::CoreType*
GetEmptyCoreType()
{
    static Sdf_ValueTypePrivate::CoreType empty((Sdf_ValueTypePrivate::Empty()));
    return &empty;
}

} // anonymous namespace


//
// Sdf_ValueTypePrivate
//

Sdf_ValueTypePrivate::CoreType::CoreType(Sdf_ValueTypePrivate::Empty)
{
    aliases.push_back(TfToken());
    unit = SdfDimensionlessUnitDefault;
}

SdfValueTypeName
Sdf_ValueTypePrivate::MakeValueTypeName(const Sdf_ValueTypeImpl* impl)
{
    return SdfValueTypeName(impl);
}

const Sdf_ValueTypeImpl*
Sdf_ValueTypePrivate::GetEmptyTypeName()
{
    static Sdf_ValueTypeImpl empty;
    return &empty;
}

//
// Sdf_ValueTypeImpl
//

Sdf_ValueTypeImpl::Sdf_ValueTypeImpl()
{
    type   = GetEmptyCoreType();
    scalar = this;
    array  = this;
}

namespace {

//
// Registry -- The implementation of the value type name registry.
//

class Registry : boost::noncopyable {
public:
    typedef Sdf_ValueTypePrivate::CoreType CoreType;

    Registry();
    ~Registry();

    // Add a type.
    void AddType(const std::string& name,
                 const VtValue& defaultValue,
                 const VtValue& defaultArrayValue,
                 TfEnum defaultUnit,
                 const TfToken& role,
                 const SdfTupleDimensions& dimensions);

    // Add a type with a C++ implementation that may not yet be available.
    void AddType(const std::string& name,
                 const TfType& type, const TfType& arrayType,
                 TfEnum defaultUnit, const TfToken& role,
                 const SdfTupleDimensions& dimensions);

    // Find a type by name.  Returns the empty type if not found.
    const Sdf_ValueTypeImpl* FindType(const std::string& name) const;

    // Find a type by TfType and role.  Returns the empty type if not found.
    const Sdf_ValueTypeImpl* FindType(const TfType&, const TfToken& role) const;

    // Find a type name by name.  Creates a new type name if not found.
    const Sdf_ValueTypeImpl* FindOrCreateTypeName(const std::string& name);

    // Returns all registered types.
    std::vector<SdfValueTypeName> GetAllTypes() const;

    // Clears all roles, types and their names.
    void Clear();

private:
    // Find a type by name.  Returns the empty type if not found.
    const Sdf_ValueTypeImpl* _FindType(const std::string& name) const;

    // Find a type by TfType and role.  Returns the empty type if not found.
    const Sdf_ValueTypeImpl* _FindType(const TfType&, const TfToken& role)const;

    // Find a type name by name.  Creates a new type name if not found.
    const Sdf_ValueTypeImpl* _FindOrCreateTypeName(const std::string& name);

    // Add a type.
    bool _AddType(Sdf_ValueTypeImpl** scalar,
                  Sdf_ValueTypeImpl** array,
                  const std::string& name,
                  const TfType& defaultValueType,
                  const TfType& defaultArrayValueType,
                  const TfToken& role,
                  const SdfTupleDimensions& dimensions,
                  const VtValue& defaultValue,
                  const VtValue& defaultArrayValue,
                  TfEnum defaultUnit);
    const CoreType* _AddCoreType(const std::string& name,
                                 const TfType& tfType,
                                 const TfToken& role,
                                 const SdfTupleDimensions& dimensions,
                                 const VtValue& value,
                                 TfEnum unit);

private:
    // Core types are a TfType and role pair.  That is, a single TfType
    // can be the core type for multiple roles but all types that have
    // the same TfType and role are aliases of each other.
    typedef std::pair<TfType, TfToken> CoreTypeKey;
    struct CoreTypeKeyHash {
        size_t operator()(const CoreTypeKey& x) const
        {
            size_t hash = 0;
            boost::hash_combine(hash, TfHash()(x.first));
            boost::hash_combine(hash, x.second.Hash());
            return hash;
        }
    };

    typedef TfHashMap<CoreTypeKey, CoreType, CoreTypeKeyHash> CoreTypeMap;
    typedef TfHashMap<std::string, Sdf_ValueTypeImpl, TfHash> TypeMap;
    typedef TfHashMap<std::string, CoreType, TfHash> TemporaryCoreTypeMap;
    typedef TfHashMap<std::string, Sdf_ValueTypeImpl, TfHash> TemporaryNameMap;

    mutable tbb::spin_rw_mutex _mutex;

    CoreTypeMap _coreTypes;
    TypeMap _types;
    std::vector<SdfValueTypeName> _allTypes;

    // Temporary names.
    TemporaryCoreTypeMap _temporaryCoreTypes;
    TemporaryNameMap _temporaryNames;
};

Registry::Registry()
{
    // Do nothing
}

Registry::~Registry()
{
    // Do nothing
}

void
Registry::Clear()
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex);

    // Clear the maps.
    _coreTypes.clear();
    _types.clear();
    _allTypes.clear();
    _temporaryCoreTypes.clear();
    _temporaryNames.clear();
}

void
Registry::AddType(
    const std::string& name,
    const VtValue& defaultValue,
    const VtValue& defaultArrayValue,
    TfEnum defaultUnit,
    const TfToken& role,
    const SdfTupleDimensions& dimensions)
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex);

    // Get the value types.  An empty VtValue has the void type but we
    // want it use the unknown type.
    const TfType type      = defaultValue.GetType();
    const TfType arrayType = defaultArrayValue.GetType();
    Sdf_ValueTypeImpl* scalar, *array;
    if (not _AddType(&scalar, &array, name,
                     type != TfType::Find<void>() ? type : TfType(),
                     arrayType != TfType::Find<void>() ? arrayType : TfType(),
                     role, dimensions,
                     defaultValue, defaultArrayValue, defaultUnit)) {
        // Error already reported.
    }
}

void
Registry::AddType(
    const std::string& name,
    const TfType& type,
    const TfType& arrayType,
    TfEnum defaultUnit,
    const TfToken& role,
    const SdfTupleDimensions& dimensions)
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex);

    Sdf_ValueTypeImpl* scalar, *array;
    if (not _AddType(&scalar, &array, name,
                     type, arrayType, role, dimensions,
                     VtValue(), VtValue(), defaultUnit)) {
        // Error already reported.
    }
}

bool
Registry::_AddType(
    Sdf_ValueTypeImpl** scalar,
    Sdf_ValueTypeImpl** array,
    const std::string& name,
    const TfType& type,
    const TfType& arrayType,
    const TfToken& role,
    const SdfTupleDimensions& dimensions,
    const VtValue& defaultValue,
    const VtValue& defaultArrayValue,
    TfEnum defaultUnit)
{
    // Construct the array name.
    const std::string arrayName = name + "[]";

    // Preconditions.
    if (not TF_VERIFY(not name.empty(), "Types must have names")) {
        return false;
    }
    if (not TF_VERIFY(not type.IsUnknown() or not arrayType.IsUnknown(),
                      "Type '%s' must have a C++ type", name.c_str())) {
        return false;
    }
    const Sdf_ValueTypeImpl* existing = _FindType(name);
    if (not TF_VERIFY(existing == Sdf_ValueTypePrivate::GetEmptyTypeName(),
                      "Type '%s' already exists", name.c_str())) {
        return false;
    }
    existing = _FindType(arrayName);
    if (not TF_VERIFY(existing == Sdf_ValueTypePrivate::GetEmptyTypeName(),
                      "Type '%s' already exists", arrayName.c_str())) {
        return false;
    }

    // Use the default dimensionless unit if the given default unit is
    // the default constructed TfEnum.
    if (defaultUnit == TfEnum()) {
        defaultUnit = SdfDimensionlessUnitDefault;
    }

    // Get the core types.
    const CoreType* coreType = NULL, *coreArrayType = NULL;
    if (not type.IsUnknown()) {
        coreType =
            _AddCoreType(name, type, role, dimensions,
                         defaultValue, defaultUnit);
        if (not coreType) {
            // Error already reported.
            return false;
        }
    }
    if (not arrayType.IsUnknown()) {
        coreArrayType =
            _AddCoreType(arrayName, arrayType, role, dimensions,
                         defaultArrayValue, defaultUnit);
        if (not coreArrayType) {
            // Error already reported.
            return false;
        }
    }

    // Add the scalar type.
    if (coreType) {
        *scalar          = &_types[name];
        (*scalar)->type  = coreType;
        (*scalar)->name  = TfToken(name);
    }
    else {
        *scalar = NULL;
    }

    // Add the array type.
    if (coreArrayType) {
        *array           = &_types[arrayName];
        (*array)->type   = coreArrayType;
        (*array)->name   = TfToken(arrayName);
    }
    else {
        *array = NULL;
    }

    // Scalar/array references.
    if (*scalar) {
        (*scalar)->scalar = *scalar;
        (*scalar)->array  = *array ? *array : Sdf_ValueTypePrivate::GetEmptyTypeName();
        _allTypes.push_back(Sdf_ValueTypePrivate::MakeValueTypeName(*scalar));
    }
    if (*array) {
        (*array)->scalar = *scalar ? *scalar : Sdf_ValueTypePrivate::GetEmptyTypeName();
        (*array)->array  = *array;
        _allTypes.push_back(Sdf_ValueTypePrivate::MakeValueTypeName(*array));
    }

    return true;
}

const Sdf_ValueTypeImpl*
Registry::FindType(const std::string& name) const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /*write=*/false);
    return _FindType(name);
}

const Sdf_ValueTypeImpl*
Registry::FindType(const TfType& type, const TfToken& role) const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /*write=*/false);
    return _FindType(type, role);
}

const Sdf_ValueTypeImpl*
Registry::FindOrCreateTypeName(const std::string& name)
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex);

    // Look up by type.
    TypeMap::const_iterator i = _types.find(name);
    if (i != _types.end()) {
        return &i->second;
    }

    // Not found.  Find or create in the temporary pool.
    return _FindOrCreateTypeName(name);
}

std::vector<SdfValueTypeName>
Registry::GetAllTypes() const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /*write=*/false);
    return _allTypes;
}

const Sdf_ValueTypeImpl*
Registry::_FindType(const std::string& name) const
{
    TypeMap::const_iterator i = _types.find(name);
    return i != _types.end() ? &i->second : Sdf_ValueTypePrivate::GetEmptyTypeName();
}

const Sdf_ValueTypeImpl*
Registry::_FindType(const TfType& type, const TfToken& role) const
{
    const CoreTypeKey key(type, role);
    CoreTypeMap::const_iterator i = _coreTypes.find(key);
    return i != _coreTypes.end() ? _FindType(i->second.aliases.front())
                                 : Sdf_ValueTypePrivate::GetEmptyTypeName();
}

const Sdf_ValueTypeImpl*
Registry::_FindOrCreateTypeName(const std::string& name)
{
    // Return any already saved temporary name.
    TemporaryNameMap::const_iterator i = _temporaryNames.find(name);
    if (i != _temporaryNames.end()) {
        return &i->second;
    }

    // Create a new temporary name.  These need their own core types.
    CoreType& coreType = _temporaryCoreTypes[name];
    coreType.aliases.push_back(TfToken(name));
    Sdf_ValueTypeImpl& impl = _temporaryNames[name];
    impl.type = &coreType;
    impl.name = coreType.aliases.back();
    return &impl;
}

const Registry::CoreType*
Registry::_AddCoreType(
    const std::string& name,
    const TfType& tfType,
    const TfToken& role,
    const SdfTupleDimensions& dimensions,
    const VtValue& value,
    TfEnum unit)
{
    if (not TF_VERIFY(not tfType.IsUnknown(),
                      "Internal error: unknown TfType for '%s'",
                      name.c_str())) {
        return NULL;
    }
    if (not TF_VERIFY(tfType != TfType::Find<void>(),
                      "Internal error: TfType<void> for '%s'",
                      name.c_str())) {
        return NULL;
    }

    // Find or create the core type.
    const CoreTypeKey key(tfType, role);
    CoreType& coreType = _coreTypes[key];
    if (coreType.type.IsUnknown()) {
        // Create.
        coreType.type  = tfType;
        coreType.role  = role;
        coreType.dim   = dimensions;
        coreType.value = value;
        coreType.unit  = unit;
    }
    else {
        // Found.  Preconditions.
        if (not TF_VERIFY(coreType.type == tfType,
                          "Internal error: unexpected core type for '%s'",
                          name.c_str())) {
            return NULL;
        }
        if (not TF_VERIFY(coreType.role == role,
                          "Mismatched roles '%s' and '%s' for core type '%s'",
                          coreType.role.GetText(),
                          role.GetText(),
                          tfType.GetTypeName().c_str())) {
            return NULL;
        }
        if (not TF_VERIFY(coreType.dim == dimensions,
                          "Mismatched dimensions for core type '%s'",
                          tfType.GetTypeName().c_str())) {
            return NULL;
        }
        if (not TF_VERIFY(coreType.value == value,
                          "Mismatched default value for core type '%s'",
                          tfType.GetTypeName().c_str())) {
            return NULL;
        }
        if (not TF_VERIFY(coreType.unit == unit,
                          "Mismatched unit for core type '%s'",
                          tfType.GetTypeName().c_str())) {
            return NULL;
        }
    }

    // Add alias.
    coreType.aliases.push_back(TfToken(name));

    return &coreType;
}

} // anonymous namespace


//
// Sdf_ValueTypeRegistry
//

class Sdf_ValueTypeRegistry::_Impl : public Registry { };

Sdf_ValueTypeRegistry::Sdf_ValueTypeRegistry() : _impl(new _Impl)
{
    // Do nothing
}

Sdf_ValueTypeRegistry::~Sdf_ValueTypeRegistry()
{
    // Do nothing
}

std::vector<SdfValueTypeName>
Sdf_ValueTypeRegistry::GetAllTypes() const
{
    return _impl->GetAllTypes();
}

SdfValueTypeName
Sdf_ValueTypeRegistry::FindType(const std::string& name) const
{
    return SdfValueTypeName(_impl->FindType(name));
}

SdfValueTypeName
Sdf_ValueTypeRegistry::FindType(const TfType& type, const TfToken& role) const
{
    return SdfValueTypeName(_impl->FindType(type, role));
}

SdfValueTypeName
Sdf_ValueTypeRegistry::FindType(const VtValue& value, const TfToken& role) const
{
    return SdfValueTypeName(_impl->FindType(value.GetType(), role));
}

SdfValueTypeName
Sdf_ValueTypeRegistry::FindOrCreateTypeName(const std::string& name) const
{
    return SdfValueTypeName(_impl->FindOrCreateTypeName(name));
}

void
Sdf_ValueTypeRegistry::AddType(
    const std::string& name,
    const VtValue& defaultValue,
    const VtValue& defaultArrayValue,
    TfEnum defaultUnit,
    const TfToken& role,
    const SdfTupleDimensions& dimensions)
{
    _impl->AddType(name, defaultValue, defaultArrayValue,
                   defaultUnit, role, dimensions);
}

void
Sdf_ValueTypeRegistry::AddType(
    const std::string& name,
    const TfType& type,
    const TfType& arrayType,
    TfEnum defaultUnit, const TfToken& role,
    const SdfTupleDimensions& dimensions)
{
    _impl->AddType(name, type, arrayType, defaultUnit, role, dimensions);
}

void
Sdf_ValueTypeRegistry::Clear()
{
    _impl->Clear();
}
