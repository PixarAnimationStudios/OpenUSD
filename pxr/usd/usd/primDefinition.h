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
#ifndef PXR_USD_USD_PRIM_DEFINITION_H
#define PXR_USD_USD_PRIM_DEFINITION_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/schemaRegistry.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Class representing the builtin definition of a prim given the schemas 
/// registered in the schema registry. It provides access to the the builtin 
/// properties of a prim whose type is defined by this definition. 
/// 
/// Instances of this class can only be created by the UsdSchemaRegistry.
class UsdPrimDefinition
{
public:
    ~UsdPrimDefinition() = default;

    /// Return the list of names of builtin properties for this prim definition.
    const TfTokenVector &GetPropertyNames() const { return _properties; }

    /// Return the list of names of the API schemas that have been applied to
    /// this prim definition in order.
    const TfTokenVector& GetAppliedAPISchemas() const { 
        return _appliedAPISchemas; 
    }

    /// Return the SdfSpecType for \p propName if it is a builtin property of
    /// the prim type represented by this prim definition. Otherwise return 
    /// SdfSpecTypeUnknown.
    SdfSpecType GetSpecType(const TfToken &propName) const    
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, propName)) {
            return _GetSchematics()->GetSpecType(*path);
        }
        return SdfSpecTypeUnknown;
    }

    /// Returns the prim spec in the registered schematics that represents this 
    /// prim definition's prim type. This will be null for invalid prim types
    /// or definitions composed for a prim with applied API schemas.
    SdfPrimSpecHandle GetSchemaPrimSpec() const { return _primSpec; }

    /// Return the property spec that defines the fallback for the property
    /// named \a propName on prims of this prim definition's type. Return null 
    /// if there is no such property spec.
    SdfPropertySpecHandle GetSchemaPropertySpec(const TfToken& propName) const
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, propName)) {
            return _GetSchematics()->GetPropertyAtPath(*path);
        }
        return TfNullPtr;
    }

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfAttributeSpecHandle>(
    ///     GetSchemaPropertySpec(primType, attrName));
    SdfAttributeSpecHandle GetSchemaAttributeSpec(const TfToken& attrName) const
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, attrName)) {
            return _GetSchematics()->GetAttributeAtPath(*path);
        }
        return TfNullPtr;
    }

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfRelationshipSpecHandle>(
    ///     GetSchemaPropertySpec(primType, relName));
    SdfRelationshipSpecHandle GetSchemaRelationshipSpec(const TfToken& relName) const
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, relName)) {
            return _GetSchematics()->GetRelationshipAtPath(*path);
        }
        return TfNullPtr;
    }

    /// Retrieves the fallback value for the attribute named \p attrName and
    /// stores it in \p value if possible. 
    /// 
    /// Returns true if the attribute exists in this prim definition and it has
    /// a fallback value defined. Returns false otherwise. 
    template <class T>
    bool GetAttributeFallbackValue(const TfToken &attrName, T *value) const
    {
        return _HasField(attrName, SdfFieldKeys->Default, value);
    }

    /// Returns the documentation for this prim definition
    /// This is based on the documentation metadata specified for this schema.
    std::string GetDocumentation() const {
        return _primSpec ? _primSpec->GetDocumentation() : std::string();
    }

private:
    // Only the UsdSchemaRegistry can construct prim definitions.
    friend class UsdSchemaRegistry;

    // Friended private accessor for use by UsdStage when composing metadata 
    // values for value resolution. The public GetMetadata functions perform
    // the extra step of filtering out disallowed or private metadata fields
    // from the SdfSpecs before retrieving metadata. Value resolution does not
    // want to pay that extra cost so uses this function instead.
    template <class T>
    friend bool Usd_GetFallbackValue(const UsdPrimDefinition &primDef,
                                     const TfToken &propName,
                                     const TfToken &fieldName,
                                     const TfToken &keyPath,
                                     T *value)
    {
        // Try to read fallback value.
        return keyPath.IsEmpty() ?
            primDef._HasField(propName, fieldName, value) :
            primDef._HasFieldDictKey(propName, fieldName, keyPath, value);
    }

    /// It is preferable to use the _HasField and _HasFieldDictKey methods to 
    /// access property field values, as opposed to getting a spec handle from 
    /// the GetSchemaXXXSpec functions, as these methods are faster.
    template <class T>
    bool _HasField(const TfToken& propName,
                   const TfToken& fieldName,
                   T* value) const
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, propName)) {
            return _GetSchematics()->HasField(*path, fieldName, value);
        }
        return false;
    }

    template <class T>
    bool _HasFieldDictKey(const TfToken& propName,
                          const TfToken& fieldName,
                          const TfToken& keyPath,
                          T* value) const
    {
        if (const SdfPath *path = TfMapLookupPtr(_propPathMap, propName)) {
            return _GetSchematics()->HasFieldDictKey(
                *path, fieldName, keyPath, value);
        }
        return false;
    }

    UsdPrimDefinition() = default;

    UsdPrimDefinition(const SdfPrimSpecHandle &primSpec);

    // Access to the schema registry's schematics.
    const SdfLayerRefPtr &_GetSchematics() const {
        return UsdSchemaRegistry::GetInstance()._schematics;
    }

    SdfPrimSpecHandle _primSpec;

    // Map for caching the paths to each property spec in the schematics by 
    // property name.
    using _PrimTypePropNameToPathMap = 
        TfHashMap<TfToken, SdfPath, TfToken::HashFunctor>;
    _PrimTypePropNameToPathMap _propPathMap;
    TfTokenVector _appliedAPISchemas;

    // Cached list of property names.
    TfTokenVector _properties;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_DEFINITION_H
