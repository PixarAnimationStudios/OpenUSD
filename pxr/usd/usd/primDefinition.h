//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

private:
    // Forward declaration required by Property.
    struct _LayerAndPath;

public:
    /// Accessor to a property's definition in the prim definition.
    /// 
    /// These are returned by calls to UsdPrimDefinition::GetPropertyDefinition 
    /// and can be used check the existence of a property (via conversion to 
    /// bool) and get field values that a defined for a property in the prim 
    /// definition.
    ///
    /// This class is just a thin wrapper around the property representation in
    /// the UsdPrimDefinition that creates it and cannot be stored or accessed
    /// beyond the lifetime of the prim definition itself.
    class Property {
    public:
        /// Default constructor returns an invalid property.
        Property() = default;

        /// Returns the name of the requested property.
        /// Note that the return value of GetName gives no indication as to 
        /// whether this is a valid property.
        USD_API
        const TfToken &GetName() const;

        /// Conversion to bool returns true if this represents a valid property
        /// in the prim definition, and false otherwise.
        explicit operator bool() const 
        {
            return _layerAndPath;
        }

        /// Return true if the property is a valid is a valid property in the 
        /// prim definition and is an attribute.
        USD_API
        bool IsAttribute() const;

        /// Return true if the property is a valid is a valid property in the 
        /// prim definition and is a relationship.
        USD_API
        bool IsRelationship() const;

        /// \name Field Access Methods 
        /// These methods help get values for fields defined on a property in 
        /// a prim definition.
        ///
        /// None of the data access methods check that the property is valid 
        /// before trying to access the property. I.e. they all assume the 
        /// property is already known to be valid when called. 
        /// 
        /// Client code is on the hook for verifying the validity of the 
        /// Property before calling any of these methods. The validity can
        /// be determined by converting the Property to bool.
        ///
        /// @{

        /// Returns the spec type of this property in the prim definition.
        USD_API
        SdfSpecType GetSpecType() const;

        /// Returns the list of names of metadata fields that are defined for 
        /// this property in the prim definition. 
        USD_API
        TfTokenVector ListMetadataFields() const;

        /// Retrieves the fallback value for the metadata field named \p key, 
        /// that is defined for this property in the prim definition, and stores
        /// it in \p value if possible.
        /// 
        /// Returns true if a value is defined for the given metadata \p key for
        /// this property. Returns false otherwise. 
        template <class T>
        bool GetMetadata(const TfToken &key, T* value) const;

        /// Retrieves the value at \p keyPath from the dictionary value for the
        /// dictionary metadata field named \p key, that is defined for this 
        /// property in the prim definition, and stores it in \p value if 
        /// possible.
        /// 
        /// Returns true if a dictionary value is defined for the given metadata
        /// \p key for this property and it contains a value at \p keyPath. 
        /// Returns false otherwise. 
        template <class T>
        bool GetMetadataByDictKey(
            const TfToken &key, const TfToken &keyPath, T* value) const;

        /// Returns the variability of this property in the prim definition.
        USD_API
        SdfVariability GetVariability() const;

        /// Returns the documentation metadata defined by the prim definition 
        /// for this property.
        USD_API
        std::string GetDocumentation() const;

        /// @}

    protected:
        // Only the prim definition can create real property accessors.
        friend class UsdPrimDefinition;
        Property(const TfToken &name, const _LayerAndPath *layerAndPath):
            _name(name), _layerAndPath(layerAndPath) {}
        Property(const _LayerAndPath *layerAndPath):
            _layerAndPath(layerAndPath) {}

        TfToken _name;
        const _LayerAndPath *_layerAndPath = nullptr;
    };

    /// Accessor to a attribute's definition in the prim definition.
    /// 
    /// These are returned by calls to UsdPrimDefinition::GetAttributeDefinition 
    /// and can be freely converted to from a Property accessor. These can be 
    /// used to check that a property exists and is an attribute (via conversion
    /// to bool) and to get attribute relevant field values that are defined for
    /// a property in the prim definition.
    ///
    /// This class is just a thin wrapper around the property representation in
    /// the UsdPrimDefinition that creates it and cannot be stored or accessed
    /// beyond the lifetime of the prim definition itself.
    class Attribute : public Property {
    public:
        /// Default constructor returns an invalid attribute.
        Attribute() = default;

        /// Copy constructor from a Property to allow implicit conversion.
        USD_API
        Attribute(const Property &property);

        /// Move constructor from a Property to allow implicit conversion.
        USD_API
        Attribute(Property &&property);

        /// Conversion to bool returns true if this represents a valid property
        /// in the prim definition that is an attribute, and false otherwise.
        explicit operator bool() const {
            return IsAttribute();
        }

        /// \name Field Access Methods 
        /// These methods help get values for additional fields defined on a 
        /// attribute in a prim definition.
        ///
        /// None of the data access methods check that the attribute is valid 
        /// before trying to access the attribute. I.e. they all assume the 
        /// attribute is already known to be valid when called. 
        /// 
        /// Client code is on the hook for verifying the validity of the 
        /// Attribute before calling any of these methods. The validity can
        /// be determined by converting the Attribute to bool.
        ///
        /// @{

        /// Returns the value type name of this attribute in the prim 
        /// definition.
        USD_API
        SdfValueTypeName GetTypeName() const;

        /// Returns the token value of the type name of this attribute in the
        /// prim definition.
        USD_API
        TfToken GetTypeNameToken() const;

        /// Retrieves the fallback value of type \p T for this attribute and 
        /// stores it in \p value if possible. 
        /// 
        /// Returns true if this attribute has a fallback value defined with 
        /// the expected type. Returns false otherwise. 
        template <class T>
        bool GetFallbackValue(T *value) const;

        /// @}
    };

    /// Accessor to a relationship's definition in the prim definition.
    /// 
    /// These are returned by calls to 
    /// UsdPrimDefinition::GetRelationshipDefinition and can be freely converted
    /// to from a Property accessor. These can be used to check that a property
    /// exists and is a relationship (via conversion to bool) and to get 
    /// relationship relevant field values that are defined for a property in
    /// the prim definition.
    ///
    /// This class is just a thin wrapper around the property representation in
    /// the UsdPrimDefinition that creates it and cannot be stored or accessed
    /// beyond the lifetime of the prim definition itself.
    class Relationship : public Property {
    public:
        /// Default constructor returns an invalid relationship.
        Relationship() = default;

        /// Copy constructor from a Property to allow implicit conversion.
        USD_API
        Relationship(const Property &property);

        /// Move constructor from a Property to allow implicit conversion.
        USD_API
        Relationship(Property &&property);

        /// Conversion to bool returns true if this represents a valid property
        /// in the prim definition that is a relationship, and false otherwise.
        explicit operator bool() const{
            return IsRelationship();
        }
    };

    /// Returns a property accessor the property named \p propName if it is 
    /// defined by this this prim definition. If a property with the given name
    /// doesn't exist, this will return an invalid Property.
    USD_API
    Property GetPropertyDefinition(const TfToken& propName) const;

    /// Returns an attribute accessor the property named \p attrName if it is 
    /// defined by this this prim definition and is an attribute. If a property
    /// with the given name doesn't exist or exists but isn't an attribute, 
    /// this will return an invalid Attribute.
    USD_API
    Attribute GetAttributeDefinition(const TfToken& attrName) const;

    /// Returns a relationship accessor the property named \p relName if it is 
    /// defined by this this prim definition and is a relationship. If a 
    /// property with the given name doesn't exist or exists but isn't a 
    /// relationship, this will return an invalid Relationship.
    USD_API
    Relationship GetRelationshipDefinition(const TfToken& relName) const;

    /// Return the SdfSpecType for \p propName if it is a builtin property of
    /// the prim type represented by this prim definition. Otherwise return 
    /// SdfSpecTypeUnknown.
    USD_API
    SdfSpecType GetSpecType(const TfToken &propName) const;

    /// \deprecated Use GetPropertyDefinition instead.
    ///
    /// Return the property spec that defines the fallback for the property
    /// named \a propName on prims of this prim definition's type. Return null 
    /// if there is no such property spec.
    USD_API
    SdfPropertySpecHandle GetSchemaPropertySpec(
        const TfToken& propName) const;

    /// \deprecated Use GetAttributeDefinition instead.
    ///
    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfAttributeSpecHandle>(
    ///     GetSchemaPropertySpec(primType, attrName));
    USD_API
    SdfAttributeSpecHandle GetSchemaAttributeSpec(
        const TfToken& attrName) const;

    /// \deprecated Use GetRelationshipDefinition instead.
    ///
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
    USD_API
    TfTokenVector ListMetadataFields() const;

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
    USD_API
    TfTokenVector ListPropertyMetadataFields(const TfToken &propName) const;

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
        if (Property prop = GetPropertyDefinition(propName)) {
            return prop.GetMetadata(key, value);
        }
        return false;
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
        if (Property prop = GetPropertyDefinition(propName)) {
            return prop.GetMetadataByDictKey(key, keyPath, value);
        }
        return false;
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
        // for efficiency during value resolution
        template <class T>
        bool HasField(const TfToken& fieldName, T* value) const {
            return layer->HasField(path, fieldName, value);
        }

        template <class T>
        bool HasFieldDictKey(
            const TfToken& fieldName, const TfToken& keyPath, T* value) const {
            return layer->HasFieldDictKey(path, fieldName, keyPath, value);
        }
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
    UsdPrimDefinition(const UsdPrimDefinition &) = default;

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

    // Helpers for constructing the prim definition.
    void _ComposePropertiesFromPrimDef(
        const UsdPrimDefinition &weakerPrimDef);

    void _ComposePropertiesFromPrimDefInstance(
        const UsdPrimDefinition &weakerPrimDef, 
        const std::string &instanceName);

    void _AddOrComposeProperty(
        const TfToken &propName,
        const _LayerAndPath &layerAndPath);

    SdfPropertySpecHandle _FindOrCreatePropertySpecForComposition(
        const TfToken &propName,
        const _LayerAndPath &srcLayerAndPath);

    SdfPropertySpecHandle _CreateComposedPropertyIfNeeded(
        const TfToken &propName,
        const _LayerAndPath &strongProp, 
        const _LayerAndPath &weakProp);

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
        _FamilyAndInstanceToVersionMap *alreadyAppliedSchemaFamilyVersions);

    static bool _PropertyTypesMatch(
        const Property &strongProp,
        const Property &weakProp);

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

    // Layer that may be created for this prim definition if it's necessary to
    // compose any new property specs for this definition from multiple 
    // property specs from other definitions.
    SdfLayerRefPtr _composedPropertyLayer;
};

template <class T>
bool 
UsdPrimDefinition::Property::GetMetadata(const TfToken &key, T* value) const
{
    if (UsdSchemaRegistry::IsDisallowedField(key)) {
        return false;
    }
    return _layerAndPath->HasField(key, value);
}

template <class T>
bool 
UsdPrimDefinition::Property::GetMetadataByDictKey(
    const TfToken &key, const TfToken &keyPath, T* value) const
{
    if (UsdSchemaRegistry::IsDisallowedField(key)) {
        return false;
    }
    return _layerAndPath->HasFieldDictKey(key, keyPath, value);
}

template <class T>
bool 
UsdPrimDefinition::Attribute::GetFallbackValue(T *value) const
{
    return _layerAndPath->HasField(SdfFieldKeys->Default, value);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_DEFINITION_H
