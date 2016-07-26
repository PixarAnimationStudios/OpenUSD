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
/// \file sdf/specType.cpp

#ifndef SDF_SPECTYPE_H
#define SDF_SPECTYPE_H

#include "pxr/usd/sdf/types.h"

#include <typeinfo>
#include <vector>

class SdfSpec;
class TfType;

/// \class SdfSpecTypeRegistration
/// Provides functions to register spec types with the runtime typing system
/// used to cast between C++ spec types. Implementations of C++ spec types
/// should use as follows:
///
/// For a concrete spec type that corresponds to a specific SdfSpecType:
/// TF_REGISTRY_FUNCTION(SdfSpecTypeRegistration) {
///    SdfSpecTypeRegistration::RegisterSpecType<MyPrimSpec>();
/// }
/// 
/// For an abstract spec type that has no corresponding SdfSpecType:
/// TF_REGISTRY_FUNCTION(SdfSpecTypeRegistration) {
///    SdfSpecTypeRegistration::RegisterAbstractSpecType<MyPropertySpec>();
/// }
class SdfSpecTypeRegistration
{
public:
    /// Registers the C++ type T as a concrete spec class.
    template <class T>
    static void RegisterSpecType()
    {
        _RegisterSpecType(typeid(T), T::GetStaticSpecType(),
                          T::GetSchemaType());
    }

    /// Registers the C++ type T as an abstract spec class.
    template <class T>
    static void RegisterAbstractSpecType()
    {
        _RegisterAbstractSpecType(typeid(T), T::GetSchemaType());
    }

private:
    static void _RegisterSpecType(
        const std::type_info& specCPPType, SdfSpecType specEnumType,
        const std::type_info& schemaType);
    static void _RegisterAbstractSpecType(
        const std::type_info& specCPPType,
        const std::type_info& schemaType);
};

// This class holds type information for specs.  It associates a
// spec type with the corresponding TfType.
class Sdf_SpecType {
public:
    // If \p spec can be represented by the C++ spec class \p to, returns
    // the TfType for \p to. This includes verifying that \p spec's schema
    // matches the schema associated with \p to.
    static TfType Cast(const SdfSpec& spec, const std::type_info& to);

    // Returns whether the \p spec can be represented by the C++ spec
    // class \p to. This includes verifying that \p spec's schema matches
    // the schema associated with \p to.
    static bool CanCast(const SdfSpec& spec, const std::type_info& to);

    // Returns whether a spec with spec type \p from can be represented by
    // the C++ spec class \p to, regardless of schema.
    static bool CanCast(SdfSpecType from, const std::type_info& to);
};

#endif
