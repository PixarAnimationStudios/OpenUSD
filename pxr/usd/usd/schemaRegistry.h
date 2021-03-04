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
#ifndef PXR_USD_USD_SCHEMA_REGISTRY_H
#define PXR_USD_USD_SCHEMA_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfAttributeSpec);
SDF_DECLARE_HANDLES(SdfRelationshipSpec);

class UsdPrimDefinition;

/// \class UsdSchemaRegistry
///
/// Singleton registry that provides access to schema type information and
/// the prim definitions for registered Usd "IsA" and applied API schema 
/// types. It also contains the data from the generated schemas that is used
/// by prim definitions to provide properties and fallbacks.
///
/// The data contained herein comes from the generatedSchema.usda file
/// (generated when a schema.usda file is processed by \em usdGenSchema)
/// of each schema-defining module. The registry expects each schema type to
/// be represented as a single prim spec with its inheritance flattened, i.e.
/// the prim spec contains a union of all its local and class inherited property
/// specs and metadata fields.
///
/// It is used by the Usd core, via UsdPrimDefinition, to determine how to 
/// create scene description for unauthored "built-in" properties of schema
/// classes, to enumerate all properties for a given schema class, and finally 
/// to provide fallback values for unauthored built-in properties.
///
class UsdSchemaRegistry : public TfWeakBase, boost::noncopyable {
public:
    USD_API
    static UsdSchemaRegistry& GetInstance() {
        return TfSingleton<UsdSchemaRegistry>::GetInstance();
    }

    /// Return the type name in the USD schema for prims or API schemas of the 
    /// given registered \p schemaType.
    USD_API
    static TfToken GetSchemaTypeName(const TfType &schemaType);

    /// Return the type name in the USD schema for prims or API schemas of the 
    /// given registered \p SchemaType.
    template <class SchemaType>
    static
    TfToken GetSchemaTypeName() {
        return GetSchemaTypeName(SchemaType::_GetStaticTfType());
    }

    /// Return the type name in the USD schema for concrete prim types only from
    /// the given registered \p schemaType.
    USD_API
    static TfToken GetConcreteSchemaTypeName(const TfType &schemaType);

    /// Return the type name in the USD schema for API schema types only from
    /// the given registered \p schemaType.
    USD_API
    static TfToken GetAPISchemaTypeName(const TfType &schemaType);

    /// Return the TfType of the schema corresponding to the given prim or API 
    /// schema name \p typeName. This the inverse of GetSchemaTypeName.
    USD_API
    static TfType GetTypeFromSchemaTypeName(const TfToken &typeName);

    /// Return the TfType of the schema corresponding to the given concrete prim
    /// type name \p typeName. This the inverse of GetConcreteSchemaTypeName.
    USD_API
    static TfType GetConcreteTypeFromSchemaTypeName(const TfToken &typeName);

    /// Return the TfType of the schema corresponding to the given API schema
    /// type name \p typeName. This the inverse of GetAPISchemaTypeNAme.
    USD_API
    static TfType GetAPITypeFromSchemaTypeName(const TfToken &typeName);

    /// Returns true if the field \p fieldName cannot have fallback values 
    /// specified in schemas. 
    /// 
    /// Fields are generally disallowed because their fallback values
    /// aren't used. For instance, fallback values for composition arcs
    /// aren't used during composition, so allowing them to be set in
    /// schemas would be misleading.
    USD_API
    static bool IsDisallowedField(const TfToken &fieldName);

    /// Returns true if the prim type \p primType inherits from \ref UsdTyped. 
    USD_API
    static bool IsTyped(const TfType& primType);

    /// Returns the kind of the schema the given \p schemaType represents.
    ///
    /// This returns UsdSchemaKind::Invalid if \p schemaType is not a valid 
    /// schema type or if the kind cannot be determined from type's plugin 
    /// information.
    USD_API 
    static UsdSchemaKind GetSchemaKind(const TfType &schemaType);

    /// Returns the kind of the schema the given \p typeName represents.
    ///
    /// This returns UsdSchemaKind::Invalid if \p typeName is not a valid 
    /// schema type name or if the kind cannot be determined from type's plugin 
    /// information.
    USD_API 
    static UsdSchemaKind GetSchemaKind(const TfToken &typeName);

    /// Returns true if the prim type \p primType is instantiable
    /// in scene description.
    USD_API
    static bool IsConcrete(const TfType& primType);

    /// Returns true if the prim type \p primType is instantiable
    /// in scene description.
    USD_API
    static bool IsConcrete(const TfToken& primType);

    /// Returns true if \p apiSchemaType is an applied API schema type.
    USD_API
    static bool IsAppliedAPISchema(const TfType& apiSchemaType);

    /// Returns true if \p apiSchemaType is an applied API schema type.
    USD_API
    static bool IsAppliedAPISchema(const TfToken& apiSchemaType);

    /// Returns true if \p apiSchemaType is a multiple-apply API schema type.
    USD_API
    static bool IsMultipleApplyAPISchema(const TfType& apiSchemaType);
    
    /// Returns true if \p apiSchemaType is a multiple-apply API schema type.
    USD_API
    static bool IsMultipleApplyAPISchema(const TfToken& apiSchemaType);
        
    /// Finds the TfType of a schema with \p typeName
    ///
    /// This is primarily for when you have been provided Schema typeName
    /// (perhaps from a User Interface or Script) and need to identify
    /// if a prim's type inherits/is that typeName. If the type name IS known,
    /// then using the schema class is preferred.
    ///
    /// \code{.py}
    /// # This code attempts to match all prims on a stage to a given
    /// # user specified type, making the traditional schema based idioms not
    /// # applicable.
    /// data = parser.parse_args()
    /// tfType = UsdSchemaRegistry.GetTypeFromName(data.type)
    /// matchedPrims = [p for p in stage.Traverse() if p.IsA(tfType)] 
    /// \endcode
    ///
    /// \note It's worth noting that
    /// GetTypeFromName("Sphere") == GetTypeFromName("UsdGeomSphere"), as
    /// this function resolves both the Schema's C++ class name and any
    /// registered aliases from a libraries plugInfo.json file. However,
    /// GetTypeFromName("Boundable") != GetTypeFromName("UsdGeomBoundable")
    /// because type aliases don't get registered for abstract schema types.
    USD_API
    static TfType GetTypeFromName(const TfToken& typeName);

    /// Returns the schema type name and the instance name parsed from the 
    /// given \p apiSchemaName
    ///
    /// \p apiSchemaName is the name of an applied schema as it appears in 
    /// the list of applied schemas on a prim. For single-apply API schemas 
    /// the name will just be the schema type name. For multiple-apply schemas 
    /// the name should include the schema type name and the applied instance
    /// name separated by a namespace delimiter, for example 
    /// 'CollectionAPI:plasticStuff'.
    ///
    /// This function returns the separated schema type name and instance name 
    /// component tokens if possible, otherwise it returns the \p apiSchemaName 
    /// as the type name and an empty instance name.
    ///
    /// \sa UsdPrim::AddAppliedSchema(const TfToken&) const
    /// \sa UsdPrim::GetAppliedSchemas() const
    USD_API
    static std::pair<TfToken, TfToken> GetTypeAndInstance(
            const TfToken &apiSchemaName);

    /// Returns a map of the names of all registered auto apply API schemas
    /// to the list of type names each is registered to be auto applied to.
    ///
    /// The list of type names to apply to will directly match what is specified
    /// in the plugin metadata for each schema type. While auto apply schemas do
    /// account for the existence and validity of the type names and expand to 
    /// include derived types of the listed types, the type lists returned by 
    /// this function do not. 
    USD_API
    static const std::map<TfToken, TfTokenVector> &GetAutoApplyAPISchemas();

    /// Collects all the additional auto apply schemas that can be defined in 
    /// a plugin through "AutoApplyAPISchemas" metadata and adds the mappings
    /// to \p autoApplyAPISchemas. 
    /// 
    /// These are separate from the auto-apply schemas that are built in to the 
    /// applied API schema types themselves and can be defined in any plugin to 
    /// map any applied API schema to any concrete prim type.
    ///
    /// Note that GetAutoApplyAPISchemas will already include API schemas 
    /// collected from this method; this function is provided for clients that
    /// may want to collect just these plugin API schema mappings.
    USD_API
    static void CollectAddtionalAutoApplyAPISchemasFromPlugins(
        std::map<TfToken, TfTokenVector> *autoApplyAPISchemas);

    /// Returns the namespace prefix that is prepended to all properties of
    /// the given \p multiApplyAPISchemaName.
    USD_API
    TfToken GetPropertyNamespacePrefix(
        const TfToken &multiApplyAPISchemaName) const;

    /// Finds the prim definition for the given \p typeName token if 
    /// \p typeName is a registered concrete typed schema type. Returns null if
    /// it is not.
    const UsdPrimDefinition* FindConcretePrimDefinition(
        const TfToken &typeName) const {
        auto it = _concreteTypedPrimDefinitions.find(typeName);
        return it != _concreteTypedPrimDefinitions.end() ? it->second : nullptr;
    }

    /// Finds the prim definition for the given \p typeName token if 
    /// \p typeName is a registered applied API schema type. Returns null if
    /// it is not.
    const UsdPrimDefinition *FindAppliedAPIPrimDefinition(
        const TfToken &typeName) const {
        auto it = _appliedAPIPrimDefinitions.find(typeName);
        return it != _appliedAPIPrimDefinitions.end() ? it->second : nullptr;
    }

    /// Returns the empty prim definition.
    const UsdPrimDefinition *GetEmptyPrimDefinition() const {
        return _emptyPrimDefinition;
    }

    /// Composes and returns a new UsdPrimDefinition from the given \p primType
    /// and list of \p appliedSchemas. This prim definition will contain a union
    /// of properties from the registered prim definitions of each of the 
    /// provided types. 
    USD_API
    std::unique_ptr<UsdPrimDefinition>
    BuildComposedPrimDefinition(
        const TfToken &primType, const TfTokenVector &appliedAPISchemas) const;

    /// Returns a dictionary mapping concrete schema prim type names to a 
    /// VtTokenArray of fallback prim type names if fallback types are defined
    /// for the schema type in its registered schema.
    /// 
    /// The standard use case for this to provide schema defined metadata that
    /// can be saved with a stage to inform an older version of USD - that 
    /// may not have some schema types - as to which types it can used instead 
    /// when encountering a prim of one these types.
    ///
    /// \sa UsdStage::WriteFallbackPrimTypes
    /// \sa \ref Usd_OM_FallbackPrimTypes
    const VtDictionary &GetFallbackPrimTypes() const {
        return _fallbackPrimTypes;
    }

private:
    friend class TfSingleton<UsdSchemaRegistry>;

    UsdSchemaRegistry();

    // Functions for backwards compatibility which old generated schemas. If
    // usdGenSchema has not been run to regenerate schemas so that the schema
    // kind is designated in the plugInfo, these functions are used to inquire
    // about kind through the registered prim definitions.
    bool _HasConcretePrimDefinition(const TfToken& primType) const;
    bool _HasAppliedAPIPrimDefinition(const TfToken& apiSchemaType) const;
    bool _HasMultipleApplyAPIPrimDefinition(const TfToken& apiSchemaType) const;

    void _FindAndAddPluginSchema();

    void _ApplyAPISchemasToPrimDefinition(
        UsdPrimDefinition *primDef, const TfTokenVector &appliedAPISchemas) const;

    SdfLayerRefPtr _schematics;
    typedef TfHashMap<TfToken, UsdPrimDefinition *, 
                      TfToken::HashFunctor> _TypeNameToPrimDefinitionMap;

    _TypeNameToPrimDefinitionMap _concreteTypedPrimDefinitions;
    _TypeNameToPrimDefinitionMap _appliedAPIPrimDefinitions;
    UsdPrimDefinition *_emptyPrimDefinition;

    TfHashMap<TfToken, TfToken, TfToken::HashFunctor> 
        _multipleApplyAPISchemaNamespaces;

    VtDictionary _fallbackPrimTypes;

    friend class UsdPrimDefinition;
};

USD_API_TEMPLATE_CLASS(TfSingleton<UsdSchemaRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_SCHEMA_REGISTRY_H
