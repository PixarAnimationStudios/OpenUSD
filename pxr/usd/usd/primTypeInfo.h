//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_USD_PRIM_TYPE_INFO_H
#define PXR_USD_USD_PRIM_TYPE_INFO_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/base/tf/token.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// Class that holds the full type information for a prim. It holds the type
/// name, applied API schema names, and possibly a mapped schema type name which
/// represent a unique full type.
/// The info this holds is used to cache and provide the "real" schema type for 
/// the prim's type name regardless of whether it is a recognized prim type or 
/// not. The optional "mapped schema type name" is used to obtain a valid schema 
/// type for an unrecognized prim type name if the stage provides a fallback 
/// type for the unrecognized type. This class also provides access to the prim
/// definition that defines all the built-in properties and metadata of a prim 
/// of this type.
class UsdPrimTypeInfo 
{
public:
    /// Returns the concrete prim type name.
    const TfToken &GetTypeName() const { return _typeId.primTypeName; }

    /// Returns the list of applied API schemas, directly authored on the prim,
    /// that impart additional properties on its prim definition. This does NOT
    /// include the applied API schemas that may be defined in the conrete prim
    /// type's prim definition..
    const TfTokenVector &GetAppliedAPISchemas() const { 
        return _typeId.appliedAPISchemas; 
    }

    /// Returns the TfType of the actual concrete schema that prims of this 
    /// type will use to create their prim definition. Typically, this will
    /// be the type registered in the schema registry for the concrete prim type
    /// returned by GetTypeName. But if the stage provided this type info with
    /// a fallback type because the prim type name is not a recognized schema, 
    /// this will return the provided fallback schema type instead.
    /// 
    /// \sa \ref Usd_OM_FallbackPrimTypes
    const TfType &GetSchemaType() const { return _schemaType; }

    /// Returns the type name associated with the schema type returned from 
    /// GetSchemaType. This will always be equivalent to calling 
    /// UsdSchemaRegistry::GetConcreteSchemaTypeName on the type returned by 
    /// GetSchemaType and will typically be the same as GetTypeName as long as
    /// the prim type name is a recognized prim type.
    ///
    /// \sa \ref Usd_OM_FallbackPrimTypes
    const TfToken &GetSchemaTypeName() const { return _schemaTypeName; }

    /// Returns the prim definition associated with this prim type's schema 
    /// type and applied API schemas.
    const UsdPrimDefinition &GetPrimDefinition() const {
        // First check if we've already cached the prim definition pointer; 
        // we can just return it. Note that we use memory_order_acquire for
        // the case wher _FindOrCreatePrimDefinition needs to build its own
        // prim definition.
        if (const UsdPrimDefinition *primDef = 
                _primDefinition.load(std::memory_order_acquire)) {
            return *primDef;
        }
        return *_FindOrCreatePrimDefinition();
    }

    bool operator==(const UsdPrimTypeInfo &other) const { 
        // Only need to compare typeId as a typeId is expected to always produce
        // the same schema type and prim definition.
        return _typeId == other._typeId; 
    }

    bool operator!=(const UsdPrimTypeInfo &other) const { 
        return !(*this == other); 
    }

    /// Returns the empty prim type info.
    USD_API
    static const UsdPrimTypeInfo &GetEmptyPrimType();

private:
    // Only the PrimTypeInfoCache can create the PrimTypeInfo prims.
    // These are cached, one for each unique, prim type/applied schema list 
    // encountered. This provides the PrimData with lazy access to the unique 
    // prim definition for this exact prim type in a thread safe way.
    friend class Usd_PrimTypeInfoCache;

    // This struct holds the information used to uniquely identify the type of a 
    // UsdPrimTypeInfo and can be used to key each prim type info in the type
    // info cache.
    struct _TypeId 
    {
        // Authored type name of the prim.
        TfToken primTypeName;

        // Optional type name that the type name should be mapped to instead.
        // Will be used typically to provide a fallback schema type for an 
        // unrecognized prim type name.
        TfToken mappedTypeName;

        // The list of applied API schemas authored on the prim.
        TfTokenVector appliedAPISchemas;

        _TypeId() = default;

        // Have both move and copy constructors to minimize the number vector
        // copies when possible.
        _TypeId(const _TypeId &typeId) = default;
        _TypeId(_TypeId &&typeId) = default;

        // Explicit constructor from just a prim type name.        
        explicit _TypeId(const TfToken &primTypeName_) 
            : primTypeName(primTypeName_) {}

        // Is empty type
        bool IsEmpty() const {
            return primTypeName.IsEmpty() &&
                   mappedTypeName.IsEmpty() &&
                   appliedAPISchemas.empty();
        }

        // Hash function for hash map keying.
        size_t Hash() const {
            size_t hash = primTypeName.Hash();
            if (!mappedTypeName.IsEmpty()) {
                boost::hash_combine(hash, mappedTypeName.Hash());
            }
            if (!appliedAPISchemas.empty()) {
                size_t appliedHash = appliedAPISchemas.size();
                for (const TfToken &apiSchema : appliedAPISchemas) {
                    boost::hash_combine(appliedHash, apiSchema);
                }
                boost::hash_combine(hash, appliedHash);
            }
            return hash;
        }

        bool operator==(const _TypeId &other) const {
            return primTypeName == other.primTypeName &&
                   mappedTypeName == other.mappedTypeName &&
                   appliedAPISchemas == other.appliedAPISchemas;
        }

        bool operator!=(const _TypeId &other) const {
            return !(*this == other);
        }
    };

    // Default constructor. Empty type.
    UsdPrimTypeInfo() : _primDefinition(nullptr) {}

    // Move constructor from a _TypeId.
    UsdPrimTypeInfo(_TypeId &&typeId);

    // Returns the full type ID.
    const _TypeId &_GetTypeId() const { return _typeId; }

    // Finds the prim definition, creating it if it doesn't already exist. This
    // cache access must be thread safe.
    USD_API
    const UsdPrimDefinition *_FindOrCreatePrimDefinition() const;

    _TypeId _typeId;
    TfType _schemaType;
    TfToken _schemaTypeName;

    // Cached pointer to the prim definition.
    mutable std::atomic<const UsdPrimDefinition *> _primDefinition;

    // When there are applied API schemas, _FindOrCreatePrimDefinition will
    // build a custom prim definition that it will own for its lifetime. This
    // is here to make sure it is explicit when the prim type info owns the
    // prim definition.
    // Note that we will always return the prim definition via the atomic 
    // _primDefinition pointer regardless of whether the _ownedPrimDefinition 
    // is set.
    mutable std::unique_ptr<UsdPrimDefinition> _ownedPrimDefinition;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_TYPE_INFO_H
