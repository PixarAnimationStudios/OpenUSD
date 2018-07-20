//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_ADAPTOR_H
#define PXRUSDMAYA_ADAPTOR_H

/// \file adaptor.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/schemaRegistry.h"

#include <maya/MDGModifier.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrUsdMayaAdaptor
/// The PxrUsdMayaAdaptor transparently adapts the interface for a Maya object
/// to a UsdPrim-like interface, allowing you to get and set Maya attributes as
/// VtValues. Via this mechanism, the USD importer can automatically adapt USD
/// data into Maya attributes, and the USD exporter can adapt Maya data back
/// into USD attributes. This is particularly useful for USD- or site-specific
/// data that is not natively handled by Maya. For example, you can use the
/// adaptor to set UsdGeomModelAPI's model draw mode attributes from within
/// Maya, and the exported USD prims will conform to the API schema.
///
/// PxrUsdMayaAdaptor determines the conversion between Maya and USD types by
/// consulting registered metadata fields and schemas. In order to use it with
/// any custom metadata or schemas, you must ensure that they are registered
/// via a plugInfo.json file and loaded by the USD system. If you need to
/// store and retrieve custom/blind data _without_ registering it beforehand,
/// you can use User-Exported Attributes instead.
///
/// The advantage of registering your metadata and schemas is that you can
/// configure the USD importer and exporter to handle known metadata and
/// schemas, enabling round-tripping of site-specific data between USD and Maya
/// without per-file configuration. See the _metadata_ and _apiSchema_ flags on
/// the _usdImport_ command.
///
/// If you are using the C++ API, then some functions will take an MDGModifier,
/// allowing you to undo the function's operations at a later time.
/// If you're using the Python API, there is no direct access to the overloads
/// taking an MDGModifier, but you can get undo functionality by registering and
/// loading the \c usdUndoHelperCmd command in Maya. If \c usdUndoHelperCmd is
/// available, Python adaptor operations will automatically write to the undo
/// stack.
///
/// \section UsdMaya_Adaptor_examples Examples
/// If you are familiar with the USD API, then this will be familiar, although
/// not entirely the same. Here are some examples of how you might do things in
/// the USD API versus using the PxrUsdMayaAdaptor.
///
/// \subsection UsdMaya_Adaptor_metadata Metadata
/// In USD:
/// \code{.py}
/// prim = stage.GetPrimAtPath('/pCube1')
/// prim.SetMetadata('comment', 'This is quite a nice cube.')
/// prim.GetMetadata('comment') # Returns: 'This is quite a nice cube.'
/// \endcode
/// In Maya:
/// \code{.py}
/// prim = UsdMaya.Adaptor('|pCube1')
/// prim.SetMetadata('comment', 'This is quite a nice cube.')
/// prim.GetMetadata('comment') # Returns: 'This is quite a nice cube.'
/// \endcode
///
/// \subsection UsdMaya_Adaptor_schemas Applying schemas
/// In USD:
/// \code{.py}
/// prim = stage.GetPrimAtPath('/pCube1')
/// schema = UsdGeom.ModelAPI.Apply(prim)
/// schema = UsdGeom.ModelAPI(prim)
/// \endcode
/// In Maya:
/// \code{.py}
/// prim = UsdMaya.Adaptor('|pCube1')
/// schema = prim.ApplySchema(UsdGeom.ModelAPI)
/// schema = prim.GetSchema(UsdGeom.ModelAPI)
/// \endcode
///
/// \subsection UsdMaya_Adaptor_attributes Setting/getting schema attributes
/// \code{.py}
/// prim = stage.GetPrimAtPath('/pCube1')
/// schema = UsdGeom.ModelAPI(prim)
/// schema.CreateModelCardTextureXPosAttr().Set(Sdf.AssetPath('card.png))
/// schema.GetModelCardTextureXPosAttr().Get()
/// # Returns: Sdf.AssetPath('card.png')
/// \endcode
/// In Maya:
/// \code{.py}
/// prim = UsdMaya.Adaptor('|pCube1')
/// schema = prim.GetSchema(UsdGeom.ModelAPI)
/// schema.CreateAttribute(UsdGeom.Tokens.modelCardTextureXPos).Set(
///     Sdf.AssetPath('card.png'))
/// schema.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos).Get()
/// # Returns: Sdf.AssetPath('card.png')
/// \endcode
/// Note that in the Maya API, CreateAttribute/GetAttribute won't accept
/// arbitrary attribute names; you can only pass attributes that belong to the
/// current schema. So this won't work:
/// \code{.py}
/// schema = prim.GetSchema(UsdGeom.ModelAPI)
/// schema.CreateAttribute('fakeAttributeName')
/// # Error: ErrorException
/// \endcode
class PxrUsdMayaAdaptor {
public:
    /// The AttributeAdaptor stores a mapping between a USD schema attribute and
    /// a Maya plug, enabling conversions between the two.
    ///
    /// \note There is not a one-to-one correspondence between USD and Maya
    /// types. For example, USD asset paths, tokens, and strings are all stored
    /// as plain strings in Maya. Thus, it is always important to go through the
    /// AttributeAdaptor when converting between USD and Maya values.
    ///
    /// \note One major difference between an AttributeAdaptor and a
    /// UsdAttribute is that there is no Clear() method. Since an
    /// AttributeAdaptor is designed to be a wrapper around some underlying Maya
    /// attribute, and Maya attributes always have values, it's not possible to
    /// Clear() the authored value. You can, however, completely remove the
    /// attribute by using PxrUsdMayaAdaptor::SchemaAdaptor::RemoveAttribute().
    class AttributeAdaptor {
        MPlug _plug;
        MObjectHandle _node;
        MObjectHandle _attr;
        SdfAttributeSpecHandle _attrDef;

    public:
        PXRUSDMAYA_API
        AttributeAdaptor();

        PXRUSDMAYA_API
        AttributeAdaptor(const MPlug& plug, SdfAttributeSpecHandle attrDef);

        PXRUSDMAYA_API
        explicit operator bool() const;

        /// Gets the adaptor for the node that owns this attribute.
        PXRUSDMAYA_API
        PxrUsdMayaAdaptor GetNodeAdaptor() const;

        /// Gets the name of the attribute in the bound USD schema.
        /// Returns the empty token if this attribute adaptor is invalid.
        PXRUSDMAYA_API
        TfToken GetName() const;

        /// Gets the value of the underlying Maya plug and adapts it back into
        /// a the requested type. This is simply a convenience function: values
        /// are retrieved internally as VtValues and then converted into the
        /// requested type. Returns false if the value could not be converted to
        /// the requested type, or if this attribute adaptor is invalid.
        /// \warning Unlike UsdAttribute::Get(), this function never performs
        /// fallback value resolution, since Maya attributes always have values.
        template <typename T>
        bool Get(T* value) const {
            VtValue v;
            if (Get(&v) && v.IsHolding<T>()) {
                *value = v.Get<T>();
                return true;
            }
            return false;
        }

        /// Gets the value of the underlying Maya plug and adapts it back into
        /// a VtValue suitable for use with USD. Returns true if the value was
        /// successfully retrieved. Returns false if the value could not be
        /// converted to a VtValue, or if this attribute adaptor is invalid.
        /// \warning Unlike UsdAttribute::Get(), this function never performs
        /// fallback value resolution, since Maya attributes always have values.
        PXRUSDMAYA_API
        bool Get(VtValue* value) const;

        /// Adapts the value to a Maya-compatible representation and sets it on
        /// the underlying Maya plug. Raises a coding error if the value cannot
        /// be adapted or is incompatible with this attribute's definition in
        /// the schema.
        PXRUSDMAYA_API
        bool Set(const VtValue& newValue);

        /// Adapts the value to a Maya-compatible representation and sets it on
        /// the underlying Maya plug. Raises a coding error if the value cannot
        /// be adapted or is incompatible with this attribute's definition in
        /// the schema.
        /// \note This overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        bool Set(const VtValue& newValue, MDGModifier& modifier);

        /// Gets the defining spec for this attribute from the schema registry.
        /// Returns a null handle if this attribute adaptor is invalid.
        PXRUSDMAYA_API
        const SdfAttributeSpecHandle GetAttributeDefinition() const;
    };

    /// The SchemaAdaptor is a wrapper around a Maya object associated with a
    /// particular USD schema. You can use it to query for adapted attributes
    /// stored on the Maya object, which include attributes previously set
    /// using an adaptor and attributes automatically adapted from USD during
    /// import.
    class SchemaAdaptor {
        MObjectHandle _handle;
        SdfPrimSpecHandle _schemaDef;

    public:
        PXRUSDMAYA_API
        SchemaAdaptor();

        PXRUSDMAYA_API
        SchemaAdaptor(const MObjectHandle& object, SdfPrimSpecHandle schemaDef);

        PXRUSDMAYA_API
        explicit operator bool() const;

        /// Gets the root adaptor for the underlying Maya node.
        PXRUSDMAYA_API
        PxrUsdMayaAdaptor GetNodeAdaptor() const;

        /// Gets the name of the bound schema.
        /// Returns the empty token if this schema adaptor is invalid.
        PXRUSDMAYA_API
        TfToken GetName() const;

        /// Gets the Maya attribute adaptor for the given schema attribute if it
        /// already exists. Returns an invalid adaptor if \p attrName doesn't
        /// exist yet on this Maya object, or if this schema adaptor is invalid.
        /// Raises a coding error if \p attrName does not exist on the schema.
        /// \warning When dealing with *typed* schema attributes, this function
        /// won't behave like a \c GetXXXAttr() function. In USD,
        /// \c GetXXXAttr() returns a valid attribute even if the attribute
        /// isn't defined in the current edit target (because the attribute is
        /// already defined by the prim type), but in Maya, you must first
        /// use CreateAttribute() to define the attribute on the Maya node
        /// (since the attribute is *not* already defined anywhere in Maya).
        PXRUSDMAYA_API
        AttributeAdaptor GetAttribute(const TfToken& attrName) const;

        /// Creates a Maya attribute corresponding to the given schema attribute
        /// and returns its adaptor. Raises a coding error if \p attrName
        /// does not exist on the schema, or if this schema adaptor is invalid.
        /// \note The Maya attribute name used by the adaptor will be
        /// different from the USD schema attribute name for technical reasons.
        /// You cannot depend on the Maya attribute having a specific name; this
        /// is all managed internally by the attribute adaptor.
        PXRUSDMAYA_API
        AttributeAdaptor CreateAttribute(const TfToken& attrName);

        /// Creates a Maya attribute corresponding to the given schema attribute
        /// and returns its adaptor. Raises a coding error if \p attrName
        /// does not exist on the schema, or if this schema adaptor is invalid.
        /// \note The Maya attribute name used by the adaptor will be
        /// different from the USD schema attribute name for technical reasons.
        /// You cannot depend on the Maya attribute having a specific name; this
        /// is all managed internally by the attribute adaptor.
        /// \note This overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        AttributeAdaptor CreateAttribute(
                const TfToken& attrName,
                MDGModifier& modifier);

        /// Removes the named attribute adaptor from this Maya object. Raises a
        /// coding error if \p attrName does not exist on the schema, or if
        /// this schema adaptor is invalid.
        PXRUSDMAYA_API
        void RemoveAttribute(const TfToken& attrName);

        /// Removes the named attribute adaptor from this Maya object. Raises a
        /// coding error if \p attrName does not exist on the schema, or if
        /// this schema adaptor is invalid.
        /// \note This overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        void RemoveAttribute(const TfToken& attrName, MDGModifier& modifier);

        /// Returns the names of only those schema attributes that are present
        /// on the Maya object, i.e., have been created via CreateAttribute().
        /// Returns an empty vector if this schema adaptor is invalid.
        PXRUSDMAYA_API
        TfTokenVector GetAuthoredAttributeNames() const;

        /// Returns the name of all schema attributes, including those that are
        /// unauthored on the Maya object.
        /// Returns an empty vector if this schema adaptor is invalid.
        PXRUSDMAYA_API
        TfTokenVector GetAttributeNames() const;

        /// Gets the prim spec for this schema from the schema registry.
        /// Returns a null handle if this schema adaptor is invalid.
        PXRUSDMAYA_API
        const SdfPrimSpecHandle GetSchemaDefinition() const;

    private:
        /// Gets the name of the adapted Maya attribute for the given attribute
        /// definition. The name may come from the registered aliases if one
        /// exists and is already present on the node.
        std::string _GetMayaAttrNameOrAlias(
                const SdfAttributeSpecHandle& attrSpec) const;
    };

    PXRUSDMAYA_API
    PxrUsdMayaAdaptor(const MObject& obj);

    PXRUSDMAYA_API
    explicit operator bool() const;

    /// Gets the full name of the underlying Maya node.
    /// An empty string is returned if the adaptor is invalid.
    PXRUSDMAYA_API
    std::string GetMayaNodeName() const;

    /// Gets the corresponding USD type name for this Maya node.
    /// An empty token is returned if the type could not be determined.
    PXRUSDMAYA_API
    TfToken GetUsdTypeName() const;

    /// Gets the corresponding USD (Tf) type for this Maya node based on its
    /// Maya type and registered mappings from Maya to Tf type.
    /// An empty type is returned if the type could not be determined.
    PXRUSDMAYA_API
    TfType GetUsdType() const;

    /// Returns a vector containing the names of USD API schemas applied via
    /// adaptors on this Maya object, using the ApplySchema() or
    /// ApplySchemaByName() methods.
    /// An empty vector is returned if the adaptor is invalid.
    PXRUSDMAYA_API
    TfTokenVector GetAppliedSchemas() const;

    /// Returns a schema adaptor for this Maya object, bound to the given USD
    /// schema type. Returns an invalid schema adaptor if this adaptor is
    /// invalid or if the schema type does not correspond to any USD schema.
    ///
    /// This function requires an exact match for any typed schema due to
    /// current API limitations. For example, if the current PxrUsdMayaAdaptor
    /// wraps a transform node (<tt>GetUsdTypeName() = "Xform"</tt>), you can
    /// use <tt>GetSchema(TfType::Find<UsdGeomXform>())</tt> but not
    /// <tt>GetSchema(TfType::Find<UsdGeomXformable>())</tt>, even though the
    /// Xform type inherits from Xformable. (We expect to be able to remove this
    /// limitation in the future.)
    /// \sa GetSchemaOrInheritedSchema()
    PXRUSDMAYA_API
    SchemaAdaptor GetSchema(const TfType& ty) const;

    /// Returns a schema adaptor for this Maya object, bound to the named USD
    /// schema. Returns an invalid schema adaptor if this adaptor is
    /// invalid or if the schema type does not correspond to any USD schema.
    ///
    /// This function requires an exact match for any typed schema name due to
    /// current API limitations. For example, if the current PxrUsdMayaAdaptor
    /// wraps a transform node (<tt>GetUsdTypeName() = "Xform"</tt>), you can
    /// use <tt>GetSchemaByName("Xform")</tt> but not
    /// <tt>GetSchemaByName("Xformable")</tt>, even though the
    /// Xform type inherits from Xformable. (We expect to be able to remove this
    /// limitation in the future.)
    /// \sa GetSchemaOrInheritedSchema()
    PXRUSDMAYA_API
    SchemaAdaptor GetSchemaByName(const TfToken& schemaName) const;

    template <typename T>
    SchemaAdaptor GetSchemaOrInheritedSchema() const {
        return GetSchemaOrInheritedSchema(TfType::Find<T>());
    }

    /// This function is intended to be temporary until the API limitations
    /// involving GetSchema() and GetSchemaByName() have been resolved.
    /// Returns a schema adaptor bound to the given USD schema type _or_ some
    /// type inherited from it. This avoids having to exactly match the
    /// concrete type, at the expense of returning a schema adaptor that is
    /// more powerful than (i.e., a superset of) the one that you requested.
    ///
    /// For example, suppose that you have a PxrUsdMayaAdaptor that wraps a
    /// Maya transform, and <tt>GetUsdTypeName() = "Xform"</tt>.
    /// <tt>GetSchemaOrInheritedSchema(TfType::Find<UsdGeomImageable>())</tt>,
    /// <tt>GetSchemaOrInheritedSchema(TfType::Find<UsdGeomXformable>())</tt>,
    /// and <tt>GetSchemaOrInheritedSchema(TfType::Find<UsdGeomXform>()</tt>
    /// will all be equivalent to <tt>GetSchemaByName("Xform")</tt>.
    /// And <tt>GetSchemaOrInheritedSchema(TfType::Find<UsdGeomMesh>())</tt>
    /// will return an invalid schema.
    ///
    /// Once we are able to implement the expected polymorphic behavior for
    /// GetSchema() and GetSchemaByName(), this function will be deprecated.
    PXRUSDMAYA_API
    SchemaAdaptor GetSchemaOrInheritedSchema(const TfType& ty) const;

    /// Applies the given API schema type on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if the type does not correspond to any known
    /// USD schema, or if it is not an API schema, or if it is a non-applied API
    /// schema, or if the adaptor is invalid.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchema(const TfType& ty);

    /// Applies the given API schema type on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if the type does not correspond to any known
    /// USD schema, or if it is not an API schema, or if it is a non-applied API
    /// schema, or if the adaptor is invalid.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchema(const TfType& ty, MDGModifier& modifier);

    /// Applies the named API schema on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if there is no known USD schema with this name, or
    /// if it is not an API schema, or if it is a non-applied API schema, or if
    /// the adaptor is invalid.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchemaByName(const TfToken& schemaName);

    /// Applies the named API schema on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if there is no known USD schema with this name, or
    /// if it is not an API schema, or if the adaptor is invalid.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchemaByName(
            const TfToken& schemaName,
            MDGModifier& modifier);

    /// Removes the given API schema from the adaptor's apiSchemas metadata.
    /// Raises a coding error if the adaptor is invalid.
    PXRUSDMAYA_API
    void UnapplySchema(const TfType& ty);

    /// Removes the given API schema from the adaptor's apiSchemas metadata.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    /// Raises a coding error if the adaptor is invalid.
    PXRUSDMAYA_API
    void UnapplySchema(const TfType& ty, MDGModifier& modifier);

    /// Removes the named API schema from the adaptor's apiSchemas metadata.
    /// Raises a coding error if the adaptor is invalid.
    PXRUSDMAYA_API
    void UnapplySchemaByName(const TfToken& schemaName);

    /// Removes the named API schema from the adaptor's apiSchemas metadata.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    /// Raises a coding error if the adaptor is invalid.
    PXRUSDMAYA_API
    void UnapplySchemaByName(const TfToken& schemaName, MDGModifier& modifier);

    /// Returns all metadata authored via the adaptor on this Maya object.
    /// Only registered metadata (i.e. the metadata fields included in
    /// GetPrimMetadataFields()) will be returned.
    /// Returns an empty map if the adaptor is invalid.
    PXRUSDMAYA_API
    UsdMetadataValueMap GetAllAuthoredMetadata() const;

    /// Retrieves the requested metadatum if it has been authored on this Maya
    /// object, returning true on success. Raises a coding error if the
    /// metadata key is not registered. Returns false if the metadata is not
    /// authored, or if the adaptor is invalid.
    /// \warning This function does not behave exactly like
    /// UsdObject::GetMetadata; it won't return the registered fallback value if
    /// the metadatum is unauthored.
    PXRUSDMAYA_API
    bool GetMetadata(const TfToken& key, VtValue* value) const;

    /// Sets the metadatum \p key's value to \p value on this Maya object,
    /// returning true on success. Raises a coding error if the metadata key
    /// is not registered, or if the value is the wrong type for the metadatum,
    /// or if the adaptor is invalid.
    PXRUSDMAYA_API
    bool SetMetadata(const TfToken& key, const VtValue& value);

    /// Sets the metadatum \p key's value to \p value on this Maya object,
    /// returning true on success. Raises a coding error if the metadata key
    /// is not registered, or if the value is the wrong type for the metadatum,
    /// or if the adaptor is invalid.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    bool SetMetadata(
            const TfToken& key,
            const VtValue& value,
            MDGModifier& modifier);

    /// Clears the authored \p key's value on this Maya object.
    /// Raises a coding error if the adaptor is invalid.
    PXRUSDMAYA_API
    void ClearMetadata(const TfToken& key);

    /// Clears the authored \p key's value on this Maya object.
    /// Raises a coding error if the adaptor is invalid.
    /// \note This overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    void ClearMetadata(const TfToken& key, MDGModifier& modifier);

    /// Gets the names of all prim metadata fields registered in Sdf.
    PXRUSDMAYA_API
    static TfTokenVector GetPrimMetadataFields();

    /// Gets the names of all known API schemas.
    PXRUSDMAYA_API
    static TfToken::Set GetRegisteredAPISchemas();

    /// Gets the names of all known typed schemas.
    PXRUSDMAYA_API
    static TfToken::Set GetRegisteredTypedSchemas();

    /// Registers the given Maya plugin type with a USD typed schema.
    /// Each Maya type is associated with only one TfType; re-registering
    /// the same Maya type again will overwrite the previous registration.
    /// However, multiple Maya types may map to the same TfType.
    PXRUSDMAYA_API
    static void RegisterTypedSchemaConversion(
            const std::string& nodeTypeName, const TfType& usdType);

    /// For backwards compatibility only: when upgrading any pre-existing code
    /// to use the adaptor mechanism, you can instruct the adaptor to recognize
    /// your existing Maya attribute names for corresponding USD schema
    /// attributes. (By default, adaptors will auto-generate a Maya attribute
    /// name based on the attribute definition in the schema.)
    ///
    /// Adds an \p alias for the given USD \p attributeName to the adaptor
    /// system. When the adaptor system searches for adaptor attributes on a
    /// Maya node, it searches for the default generated name first, and then
    /// looks through the aliases in the order in which they were registered.
    /// When the system needs to create a new Maya attribute (because it
    /// cannot find any attributes with the default name or the alias names),
    /// it always uses the generated name.
    /// \sa PxrUsdMayaAdaptor::SchemaAdaptor::CreateAttribute()
    PXRUSDMAYA_API
    static void RegisterAttributeAlias(
            const TfToken& attributeName, const std::string& alias);

    /// Gets the name of all possible Maya attribute names for the given USD
    /// schema \p attributeName, in the order in which the aliases were
    /// registered. The default generated name is always the zeroth item in the
    /// returned vector.
    PXRUSDMAYA_API
    static std::vector<std::string> GetAttributeAliases(
            const TfToken& attributeName);

private:
    MObjectHandle _handle;

    /// Mapping of Maya type name to TfType's for typed schemas.
    /// API (untyped) schemas should never be included in this mapping.
    static std::map<std::string, TfType> _schemaLookup;

    /// Attribute aliases for backwards compatibility.
    static std::map<TfToken, std::vector<std::string>> _attributeAliases;
};

/// Registers the given \p mayaTypeName with the given USD \p schemaType
/// so that those Maya nodes can be used with the given typed schema in the
/// adaptor system. \p mayaTypeName is a Maya node name. Each \p fnOrPluginType
/// is mapped to a single \p schemaType; the last registration wins.
///
/// The convention in the UsdMaya library is to place the registration macro
/// in the prim writer that exports \p mayaTypeName nodes as \p schemaType
/// prims. This will ensure that the registrations are properly invoked by the
/// UsdMaya adaptor system.
///
/// Example usage:
/// \code
/// PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(myTypeName, MySchemaType);
/// \endcode
///
/// \sa PxrUsdMayaAdaptor::RegisterTypedSchemaConversion()
#define PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mayaTypeName, schemaType)\
TF_REGISTRY_FUNCTION(PxrUsdMayaAdaptor)\
{\
    PxrUsdMayaAdaptor::RegisterTypedSchemaConversion(\
            #mayaTypeName, TfType::Find<schemaType>());\
}

/// Registers an \p alias string for the given \p attrName token or string.
///
/// You should invoke this macro in the same place that you invoke any
/// PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA macros for your type. This will ensure
/// that all the aliases are registered at the correct time.
///
/// \sa PxrUsdMayaAdaptor::RegisterAttributeAlias()
#define PXRUSDMAYA_REGISTER_ADAPTOR_ATTRIBUTE_ALIAS(attrName, alias)\
TF_REGISTRY_FUNCTION(PxrUsdMayaAdaptor)\
{\
    PxrUsdMayaAdaptor::RegisterAttributeAlias(TfToken(attrName), alias);\
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
