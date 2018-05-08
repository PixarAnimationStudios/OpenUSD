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
/// allowing you to undo the function's operations at a later time. You can use
/// this to implement undo/redo in commands. If you're using the Python API,
/// the overloads taking MDGModifier are not currently wrapped, so undo is
/// unsupported.
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
    /// Note that there is not a one-to-one correspondence between USD and Maya
    /// types. For example, USD asset paths, tokens, and strings are all stored
    /// as plain strings in Maya. Thus, it is always important to go through the
    /// AttributeAdaptor when converting between USD and Maya values.
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
        PXRUSDMAYA_API
        TfToken GetName() const;

        /// Gets the value of the underlying Maya plug and adapts it back into
        /// a VtValue suitable for use with USD. Returns true if the value was
        /// successfully retrieved.
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
        /// Note that this overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        bool Set(const VtValue& newValue, MDGModifier& modifier);

        /// Gets the defining spec for this attribute from the schema registry.
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
        PXRUSDMAYA_API
        TfToken GetName() const;

        /// Gets the Maya attribute adaptor for the given schema attribute if it
        /// already exists. Returns an invalid adaptor if \p attrName doesn't
        /// exist yet on this Maya object. Raises a coding error if \p attrName
        /// does not exist on the schema.
        PXRUSDMAYA_API
        AttributeAdaptor GetAttribute(const TfToken& attrName) const;

        /// Creates a Maya attribute corresponding to the given schema attribute
        /// and returns its adaptor. Raises a coding error if \p attrName
        /// does not exist on the schema.
        /// Note that the Maya attribute name used by the adaptor will be
        /// different from the USD schema attribute name for technical reasons.
        /// You cannot depend on the Maya attribute having a specific name; this
        /// is all managed internally by the attribute adaptor.
        PXRUSDMAYA_API
        AttributeAdaptor CreateAttribute(const TfToken& attrName);

        /// Creates a Maya attribute corresponding to the given schema attribute
        /// and returns its adaptor. Raises a coding error if \p attrName
        /// does not exist on the schema.
        /// Note that the Maya attribute name used by the adaptor will be
        /// different from the USD schema attribute name for technical reasons.
        /// You cannot depend on the Maya attribute having a specific name; this
        /// is all managed internally by the attribute adaptor.
        /// Note that this overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        AttributeAdaptor CreateAttribute(
                const TfToken& attrName,
                MDGModifier& modifier);

        /// Removes the named attribute adaptor from this Maya object. Raises a
        /// coding error if \p attrName does not exist on the schema.
        PXRUSDMAYA_API
        void RemoveAttribute(const TfToken& attrName);

        /// Removes the named attribute adaptor from this Maya object. Raises a
        /// coding error if \p attrName does not exist on the schema.
        /// Note that this overload will call doIt() on the MDGModifier; thus
        /// any actions will have been committed when the function returns.
        PXRUSDMAYA_API
        void RemoveAttribute(const TfToken& attrName, MDGModifier& modifier);

        /// Returns the names of only those schema attributes that are present
        /// on the Maya object, i.e., have been created via CreateAttribute().
        PXRUSDMAYA_API
        TfTokenVector GetAuthoredAttributeNames() const;

        /// Returns the name of all schema attributes, including those that are
        /// unauthored on the Maya object.
        PXRUSDMAYA_API
        TfTokenVector GetAttributeNames() const;

        /// Gets the prim spec for this schema from the schema registry.
        PXRUSDMAYA_API
        const SdfPrimSpecHandle GetSchemaDefinition() const;
    };

    PXRUSDMAYA_API
    PxrUsdMayaAdaptor(const MObject& obj);

    PXRUSDMAYA_API
    explicit operator bool() const;

    /// Gets the full name of the underlying Maya node.
    PXRUSDMAYA_API
    std::string GetMayaNodeName() const;

    /// Returns a vector containing the names of USD API schemas applied via
    /// adaptors on this Maya object, using the ApplySchema() or
    /// ApplySchemaByName() methods.
    PXRUSDMAYA_API
    TfTokenVector GetAppliedSchemas() const;

    /// Returns a schema adaptor for this Maya object, bound to the given USD
    /// schema type. Raises a coding error if the type does not correspond to 
    /// any known USD schema.
    /// Currently this only returns valid SchemaAdaptor's for API schemas, but
    /// may be extended in the future with support for typed schemas.
    PXRUSDMAYA_API
    SchemaAdaptor GetSchema(const TfType& ty) const;

    /// Returns a schema adaptor for this Maya object, bound to the named USD
    /// schema. Raises a coding error if the schema name is not registered in
    /// USD.
    /// Currently this only returns valid SchemaAdaptor's for API schemas, but
    /// may be extended in the future with support for typed schemas.
    PXRUSDMAYA_API
    SchemaAdaptor GetSchemaByName(const TfToken& schemaName) const;

    /// Applies the given API schema type on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if the type does not correspond to any known
    /// USD schema, or if it is not an API schema.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchema(const TfType& ty);

    /// Applies the named API schema on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if there is no known USD schema with this name, or
    /// if it is not an API schema.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchemaByName(const TfToken& schemaName);

    /// Applies the named API schema on this Maya object via the adaptor
    /// mechanism. The schema's name is added to the adaptor's apiSchemas
    /// metadata, and the USD exporter will recognize the API schema when
    /// exporting this node to a USD prim.
    /// Raises a coding error if there is no known USD schema with this name, or
    /// if it is not an API schema.
    /// Note that this overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    SchemaAdaptor ApplySchemaByName(
            const TfToken& schemaName,
            MDGModifier& modifier);

    /// Removes the given API schema from the adaptor's apiSchemas metadata.
    PXRUSDMAYA_API
    void UnapplySchema(const TfType& ty);

    /// Removes the named API schema from the adaptor's apiSchemas metadata.
    PXRUSDMAYA_API
    void UnapplySchemaByName(const TfToken& schemaName);

    /// Removes the named API schema from the adaptor's apiSchemas metadata.
    /// Note that this overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    void UnapplySchemaByName(const TfToken& schemaName, MDGModifier& modifier);

    /// Returns all metadata authored via the adaptor on this Maya object.
    /// Only registered metadata (i.e. the metadata fields included in
    /// GetPrimMetadataFields()) will be returned.
    PXRUSDMAYA_API
    UsdMetadataValueMap GetAllAuthoredMetadata() const;

    /// Retrieves the requested metadatum if it has been authored on this Maya
    /// object, returning true on success. Raises a coding error if the
    /// metadata key is not registered.
    /// Note that this function does not behave exactly like
    /// UsdObject::GetMetadata; it won't return the registered fallback value if
    /// the metadatum is unauthored.
    PXRUSDMAYA_API
    bool GetMetadata(const TfToken& key, VtValue* value) const;

    /// Sets the metadatum \p key's value to \p value on this Maya object,
    /// returning true on success. Raises a coding error if the metadata key
    /// is not registered, or if the value is the wrong type for the metadatum.
    PXRUSDMAYA_API
    bool SetMetadata(const TfToken& key, const VtValue& value);

    /// Sets the metadatum \p key's value to \p value on this Maya object,
    /// returning true on success. Raises a coding error if the metadata key
    /// is not registered, or if the value is the wrong type for the metadatum.
    /// Note that this overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    bool SetMetadata(
            const TfToken& key,
            const VtValue& value,
            MDGModifier& modifier);

    /// Clears the authored \p key's value on this Maya object.
    PXRUSDMAYA_API
    void ClearMetadata(const TfToken& key);

    /// Clears the authored \p key's value on this Maya object.
    /// Note that this overload will call doIt() on the MDGModifier; thus any
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    void ClearMetadata(const TfToken& key, MDGModifier& modifier);

    /// Gets the names of all prim metadata fields registered in Sdf.
    PXRUSDMAYA_API
    static TfTokenVector GetPrimMetadataFields();

    /// Gets the names of all known API schemas.
    PXRUSDMAYA_API
    static TfToken::Set GetRegisteredAPISchemas();

private:
    MObjectHandle _handle;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif