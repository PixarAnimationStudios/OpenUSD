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

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

/// Class representing the builtin definition of a prim given the schemas 
/// registered in the schema registry. It provides access to the the builtin 
/// properties and metadata of a prim whose type is defined by this definition. 
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
        if (const _LayerAndPath *layerAndPath = 
                _GetPropertyLayerAndPath(propName)) {
            return layerAndPath->GetSpecType();
        }
        return SdfSpecTypeUnknown;
    }

    /// Return the property spec that defines the fallback for the property
    /// named \a propName on prims of this prim definition's type. Return null 
    /// if there is no such property spec.
    USD_API
    SdfPropertySpecHandle GetSchemaPropertySpec(
        const TfToken& propName) const;

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfAttributeSpecHandle>(
    ///     GetSchemaPropertySpec(primType, attrName));
    USD_API
    SdfAttributeSpecHandle GetSchemaAttributeSpec(
        const TfToken& attrName) const;

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfRelationshipSpecHandle>(
    ///     GetSchemaPropertySpec(primType, relName));
    USD_API
    SdfRelationshipSpecHandle GetSchemaRelationshipSpec(
        const TfToken& relName) const;

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

    /// Returns the list of names of metadata fields that are defined by this 
    /// prim definition for the prim itself.
    TfTokenVector ListMetadataFields() const 
    {
        return _ListMetadataFields(TfToken());
    }

    /// Retrieves the fallback value for the metadata field named \p key, that
    /// is defined by this prim definition for the prim itself and stores it in
    /// \p value if possible. 
    /// 
    /// Returns true if a fallback value is defined for the given metadata 
    /// \p key. Returns false otherwise. 
    template <class T>
    bool GetMetadata(const TfToken &key, T* value) const 
    {
        if (UsdSchemaRegistry::IsDisallowedField(key)) {
            return false;
        }
        return _HasField(TfToken(), key, value);
    }

    /// Retrieves the value at \p keyPath from the fallback dictionary value 
    /// for the dictionary metadata field named \p key, that is defined by this 
    /// prim definition for the prim itself, and stores it in \p value if 
    /// possible.
    /// 
    /// Returns true if a fallback dictionary value is defined for the given 
    /// metadata \p key and it contains a value at \p keyPath. Returns false 
    /// otherwise. 
    template <class T>
    bool GetMetadataByDictKey(const TfToken &key, 
                              const TfToken &keyPath, T* value) const 
    {
        if (UsdSchemaRegistry::IsDisallowedField(key)) {
            return false;
        }
        return _HasFieldDictKey(TfToken(), key, keyPath, value);
    }

    /// Returns the documentation metadata defined by the prim definition for 
    /// the prim itself.
    USD_API
    std::string GetDocumentation() const;

    /// Returns the list of names of metadata fields that are defined by this 
    /// prim definition for property \p propName if a property named \p propName
    /// exists.
    TfTokenVector ListPropertyMetadataFields(const TfToken &propName) const 
    {
        return propName.IsEmpty() ? 
            TfTokenVector() : _ListMetadataFields(propName);
    }

    /// Retrieves the fallback value for the metadata field named \p key, that
    /// is defined by this prim definition for the property named \p propName, 
    /// and stores it in \p value if possible.
    /// 
    /// Returns true if a fallback value is defined for the given metadata 
    /// \p key for the named property. Returns false otherwise. 
    template <class T>
    bool GetPropertyMetadata(
        const TfToken &propName, const TfToken &key, T* value) const
    {
        if (propName.IsEmpty() || UsdSchemaRegistry::IsDisallowedField(key)) {
            return false;
        }
        return _HasField(propName, key, value);
    }

    /// Retrieves the value at \p keyPath from the fallback dictionary value 
    /// for the dictionary metadata field named \p key, that is defined by this 
    /// prim definition for the property named \p propName, and stores it in 
    /// \p value if possible.
    /// 
    /// Returns true if a fallback dictionary value is defined for the given 
    /// metadata \p key for the named property and it contains a value at 
    /// \p keyPath. Returns false otherwise. 
    template <class T>
    bool GetPropertyMetadataByDictKey(
        const TfToken &propName, const TfToken &key, 
        const TfToken &keyPath, T* value) const
    {
        if (propName.IsEmpty() || UsdSchemaRegistry::IsDisallowedField(key)) {
            return false;
        }
        return _HasFieldDictKey(propName, key, keyPath, value);
    }

    /// Returns the documentation metadata defined by the prim definition for 
    /// the property named \p propName if it exists.
    USD_API
    std::string GetPropertyDocumentation(const TfToken &propName) const;

    /// Copies the contents of this prim definition to a prim spec on the 
    /// given \p layer at the given \p path. This includes the entire property
    /// spec for each of this definition's built-in properties as well as all of
    /// this definition's prim metadata. 
    /// 
    /// If the prim definition represents a concrete prim type, the type name 
    /// of the prim spec is set to the the type name of this prim definition. 
    /// Otherwise the type name is set to empty. The 'apiSchemas' metadata
    /// on the prim spec will always be explicitly set to the combined list 
    /// of all API schemas applied to this prim definition, i.e. the list 
    /// returned by UsdPrimDefinition::GetAppliedAPISchemas. Note that if this 
    /// prim definition is an API schema prim definition 
    /// (see UsdSchemaRegistry::FindAppliedAPIPrimDefinition) then 'apiSchemas'
    /// will be empty as this prim definition does not "have" an applied API 
    /// because instead it "is" an applied API.
    /// 
    /// If there is no prim spec at the given \p path, a new prim spec is 
    /// created at that path with the specifier \p newSpecSpecifier. Any 
    /// necessary ancestor specs will be created as well but they will always 
    /// be created as overs. If a spec does exist at \p path, then all of its 
    /// properties and 
    /// \ref UsdSchemaRegistry::IsDisallowedField "schema allowed metadata" are 
    /// cleared before it is populated from the prim definition.
    USD_API
    bool FlattenTo(const SdfLayerHandle &layer, 
                   const SdfPath &path,
                   SdfSpecifier newSpecSpecifier = SdfSpecifierOver) const;

    /// \overload
    /// Copies the contents of this prim definition to a prim spec at the 
    /// current edit target for a prim with the given \p name under the prim 
    /// \p parent.
    USD_API
    UsdPrim FlattenTo(const UsdPrim &parent, 
                      const TfToken &name,
                      SdfSpecifier newSpecSpecifier = SdfSpecifierOver) const;

    /// \overload
    /// Copies the contents of this prim definition to a prim spec at the 
    /// current edit target for the given \p prim.
    USD_API
    UsdPrim FlattenTo(const UsdPrim &prim, 
                      SdfSpecifier newSpecSpecifier = SdfSpecifierOver) const;


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

    // Prim definitions store property access via a pointer to the schematics
    // layer and a path to the property spec on that layer.
    struct _LayerAndPath {
        // Note that we use a raw pointer to the layer (for efficiency) as only
        // the schema registry can create a UsdPrimDefinition and is responsible
        // for making sure any schematics layers are alive throughout the
        // life-time of any UsdPrimDefinition it creates.
        const SdfLayer *layer = nullptr;
        SdfPath path;

        // Accessors for the common data we extract from the schematics, inline
        // for efficiency during value resolution (with the exception of 
        // ListMetadataFields).
        SdfSpecType GetSpecType() const {
            return layer->GetSpecType(path);
        }

        template <class T>
        bool HasField(const TfToken& fieldName, T* value) const {
            return layer->HasField(path, fieldName, value);
        }

        template <class T>
        bool HasFieldDictKey(
            const TfToken& fieldName, const TfToken& keyPath, T* value) const {
            return layer->HasFieldDictKey(path, fieldName, keyPath, value);
        }

        USD_API
        TfTokenVector ListMetadataFields() const;
    };

    /// It is preferable to use the _HasField and _HasFieldDictKey methods to 
    /// access property field values, as opposed to getting a spec handle from 
    /// the GetSchemaXXXSpec functions, as these methods are faster.
    template <class T>
    bool _HasField(const TfToken& propName,
                   const TfToken& fieldName,
                   T* value) const
    {
        if (const _LayerAndPath *layerAndPath = 
                _GetPropertyLayerAndPath(propName)) {
            return layerAndPath->HasField(fieldName, value);
        }
        return false;
    }

    template <class T>
    bool _HasFieldDictKey(const TfToken& propName,
                          const TfToken& fieldName,
                          const TfToken& keyPath,
                          T* value) const
    {
        if (const _LayerAndPath *layerAndPath = 
                _GetPropertyLayerAndPath(propName)) {
            return layerAndPath->HasFieldDictKey(fieldName, keyPath, value);
        }
        return false;
    }

    UsdPrimDefinition() = default;

    USD_API
    void _IntializeForTypedSchema(
        const SdfLayerHandle &schematicsLayer,
        const SdfPath &schematicsPrimPath, 
        const VtTokenArray &propertiesToIgnore);

    USD_API
    void _IntializeForAPISchema(
        const TfToken &apiSchemaName,
        const SdfLayerHandle &schematicsLayer,
        const SdfPath &schematicsPrimPath, 
        const VtTokenArray &propertiesToIgnore);

    // Only used by the two _Initialize methods.
    bool _MapSchematicsPropertyPaths(
        const VtTokenArray &propertiesToIgnore);

    // Accessors for looking property spec paths by name.
    const _LayerAndPath *_GetPropertyLayerAndPath(const TfToken& propName) const
    {
        return TfMapLookupPtr(_propLayerAndPathMap, propName);
    }

    _LayerAndPath *_GetPropertyLayerAndPath(const TfToken& propName)
    {
        return TfMapLookupPtr(_propLayerAndPathMap, propName);
    }

    USD_API
    TfTokenVector _ListMetadataFields(const TfToken &propName) const;

    // Helpers for constructing the prim definition.
    USD_API
    void _ComposePropertiesFromPrimDef(
        const UsdPrimDefinition &weakerPrimDef, 
        bool useWeakerPropertyForTypeConflict = false);

    USD_API
    void _ComposePropertiesFromPrimDefInstance(
        const UsdPrimDefinition &weakerPrimDef, 
        const std::string &instanceName);

    USD_API
    void _ComposeOverAndReplaceExistingProperty(
        const TfToken &propName,
        const SdfLayerRefPtr &overLayer,
        const SdfPath &overPrimPath);

    using _FamilyAndInstanceToVersionMap = 
        std::unordered_map<std::pair<TfToken, TfToken>, UsdSchemaVersion, TfHash>;

    USD_API
    bool _ComposeWeakerAPIPrimDefinition(
        const UsdPrimDefinition &apiPrimDef,
        const TfToken &instanceName,
        _FamilyAndInstanceToVersionMap *alreadyAppliedSchemaFamilyVersions,
        bool allowDupes = false);

    USD_API
    static bool _PropertyTypesMatch(
        const _LayerAndPath &strongerProp,
        const _LayerAndPath &weakerProp);

    // Path to the prim in the schematics for this prim definition.
    _LayerAndPath _primLayerAndPath;

    // Map for caching the paths to each property spec in the schematics by 
    // property name.
    using _PrimTypePropNameToPathMap = 
        std::unordered_map<TfToken, _LayerAndPath, TfToken::HashFunctor>;
    _PrimTypePropNameToPathMap _propLayerAndPathMap;
    TfTokenVector _appliedAPISchemas;

    // Cached list of property names.
    TfTokenVector _properties;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_DEFINITION_H
