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
#ifndef SDF_VALUETYPEREGISTRY_H
#define SDF_VALUETYPEREGISTRY_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include <boost/noncopyable.hpp>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TfType;

/// \class Sdf_ValueTypeRegistry
///
/// A registry of value type names used by a schema.
///
class Sdf_ValueTypeRegistry : boost::noncopyable {
public:
    Sdf_ValueTypeRegistry();
    ~Sdf_ValueTypeRegistry();

    /// Returns all registered value type names.
    std::vector<SdfValueTypeName> GetAllTypes() const;

    /// Returns a value type name by name.
    SdfValueTypeName FindType(const std::string& name) const;

    /// Returns the value type name for the type and role if any, otherwise
    /// returns the invalid value type name.  This returns the first
    /// registered value type name for a given type/role pair if there are
    /// aliases
    SdfValueTypeName FindType(const TfType& type,
                              const TfToken& role = TfToken()) const;

    /// Returns the value type name for the held value and given role if
    /// any, otherwise returns the invalid value type.  This returns the
    /// first registered name for a given type/role pair if there are
    /// aliases.
    SdfValueTypeName FindType(const VtValue& value,
                              const TfToken& role = TfToken()) const;

    /// Returns a value type name by name.  If a type with that name is
    /// registered it returns the object for that name.  Otherwise a
    /// temporary type name is created and returned.  This name will match
    /// other temporary value type names that use the exact same name.  Use
    /// this function when you need to ensure that the name isn't lost even
    /// if the type isn't registered, typically when writing the name to a
    /// file or log.
    SdfValueTypeName FindOrCreateTypeName(const std::string& name) const;

    /// \class Type
    /// Named parameter object for specifying an SdfValueTypeName to
    /// be added to the registry.
    class Type
    {
    public:
        // Specify a type with the given name, default value, and default
        // array value.
        Type(const std::string& name, 
             const VtValue& defaultValue, 
             const VtValue& defaultArrayValue)
            : _name(name)
            , _defaultValue(defaultValue)
            , _defaultArrayValue(defaultArrayValue)
        { }

        // Specify a type with the given name, default value, and default
        // array value of VtArray<T>.
        template <class T>
        Type(const std::string& name, const T& defaultValue)
            : Type(name, VtValue(defaultValue), VtValue(VtArray<T>()))
        { }

        // Specify a type with the given name and underlying C++ type.
        // No default value or array value will be registered.
        Type(const std::string& name, const TfType& type)
            : _name(name)
            , _type(type)
        { }

        // Set C++ type name string for this type. Defaults to type name
        // from TfType.
        Type& CPPTypeName(const std::string& cppTypeName)
        {
            _cppTypeName = cppTypeName;
            if (!_defaultArrayValue.IsEmpty()) {
                _arrayCppTypeName = "VtArray<" + cppTypeName + ">";
            }
            return *this;
        }

        // Set shape for this type. Defaults to shapeless.
        Type& Dimensions(const SdfTupleDimensions& dims)
        { _dimensions = dims; return *this; }

        // Set default unit for this type. Defaults to dimensionless unit.
        Type& DefaultUnit(TfEnum unit) { _unit = unit; return *this; }

        // Set role for this type. Defaults to no role.
        Type& Role(const TfToken& role) { _role = role; return *this; }

        // Indicate that arrays of this type are not supported.
        Type& NoArrays() 
        { 
            _defaultArrayValue = VtValue(); 
            _arrayCppTypeName = std::string();
            return *this; 
        }

    private:
        friend class Sdf_ValueTypeRegistry;

        std::string _name;
        TfType _type;
        VtValue _defaultValue, _defaultArrayValue;
        std::string _cppTypeName, _arrayCppTypeName;
        TfEnum _unit;
        TfToken _role;
        SdfTupleDimensions _dimensions;
    };

    /// Register the value type specified by \p type.
    /// \see Type
    void AddType(const Type& type);

    /// Register a value type and it's corresponding array value type.
    void AddType(const std::string& name,
                 const VtValue& defaultValue,
                 const VtValue& defaultArrayValue,
                 const std::string& cppName, const std::string& cppArrayName,
                 TfEnum defaultUnit, const TfToken& role,
                 const SdfTupleDimensions& dimensions);

    /// Register a value type and it's corresponding array value type.
    /// In this case the default values are empty.  This is useful for types 
    /// provided by plugins;  you don't need to load the plugin just to 
    /// register the type.  However, there is no default value.
    void AddType(const std::string& name,
                 const TfType& type, const TfType& arrayType,
                 const std::string& cppName, const std::string& cppArrayName,
                 TfEnum defaultUnit, const TfToken& role,
                 const SdfTupleDimensions& dimensions);

    /// Empties out the registry.  Any existing types, roles or their names
    /// become invalid and must not be used.
    void Clear();

private:
    class _Impl;
    std::unique_ptr<_Impl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_VALUETYPEREGISTRY_H
