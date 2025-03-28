//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_SPEC_TYPE_H
#define PXR_USD_SDF_SPEC_TYPE_H

/// \file sdf/specType.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"

#include <typeinfo>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfSpec;
class TfType;

/// \class SdfSpecTypeRegistration
///
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
///
class SdfSpecTypeRegistration
{
public:
    /// Registers the C++ type T as a concrete spec class.
    template <class SchemaType, class SpecType>
    static void RegisterSpecType(SdfSpecType specTypeEnum) {
        _RegisterSpecType(
            typeid(SpecType), specTypeEnum, typeid(SchemaType));
    }

    /// Registers the C++ type T as an abstract spec class.
    template <class SchemaType, class SpecType>
    static void RegisterAbstractSpecType() {
        _RegisterSpecType(
            typeid(SpecType), SdfSpecTypeUnknown, typeid(SchemaType));
    }

private:
    SDF_API
    static void _RegisterSpecType(
        const std::type_info& specCPPType,
        SdfSpecType specEnumType,
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_SPEC_TYPE_H
