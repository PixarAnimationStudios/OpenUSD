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

#include <unordered_map>

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

    /// Returns true if the prim type \p primType is an abstract schema type 
    /// and, unlike a concrete type, is not instantiable in scene description.
    USD_API
    static bool IsAbstract(const TfType& primType);

    /// Returns true if the prim type \p primType is an abstract schema type 
    /// and, unlike a concrete type, is not instantiable in scene description.
    USD_API
    static bool IsAbstract(const TfToken& primType);

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
    /// Note that no validation is done on the returned tokens. Clients are
    /// advised to use GetTypeFromSchemaTypeName() to validate the typeName 
    /// token.
    ///
    /// \sa UsdPrim::AddAppliedSchema(const TfToken&) const
    /// \sa UsdPrim::GetAppliedSchemas() const
    USD_API
    static std::pair<TfToken, TfToken> GetTypeNameAndInstance(
            const TfToken &apiSchemaName);

    /// Returns true if the given \p instanceName is an allowed instance name
    /// for the multiple apply API schema named \p apiSchemaName. 
    /// 
    /// Any instance name that matches the name of a property provided by the 
    /// API schema is disallowed and will return false. If the schema type
    /// has plugin metadata that specifies allowed instance names, then only
    /// those specified names are allowed for the schema type.
    /// If the instance name is empty or the API is not a multiple apply schema,
    /// this will return false.
    USD_API
    static bool IsAllowedAPISchemaInstanceName(
        const TfToken &apiSchemaName,
        const TfToken &instanceName);

    /// Returns a list of prim type names that the given \p apiSchemaName can
    /// only be applied to. 
    /// 
    /// A non-empty list indicates that the API schema can only be applied to 
    /// prim that are or derive from prim type names in the list. If the list
    /// is empty, the API schema can be applied to prims of any type.
    /// 
    /// If a non-empty \p instanceName is provided, this will first look for
    /// a list of "can only apply to" names specific to that instance of the API
    /// schema and return that if found. If a list is not found for the specific
    /// instance, it will fall back to looking for a "can only apply to" list
    /// for just the schema name itself.
    USD_API
    static const TfTokenVector &GetAPISchemaCanOnlyApplyToTypeNames(
        const TfToken &apiSchemaName, 
        const TfToken &instanceName = TfToken());

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

    /// Creates a name template that can represent a property or API schema that
    /// belongs to a multiple apply schema and will therefore have multiple 
    /// instances with different names.
    ///
    /// The name template is created by joining the \p namespacePrefix, 
    /// the instance name placeholder "__INSTANCE_NAME__", and the 
    /// \p baseName using the namespace delimiter. Therefore the 
    /// returned name template will be of one of the following forms depending 
    /// on whether either of the inputs is empty:
    /// 1. namespacePrefix:__INSTANCE_NAME__:baseName
    /// 2. namespacePrefix:__INSTANCE_NAME__
    /// 3. __INSTANCE_NAME__:baseName
    /// 4. __INSTANCE_NAME__
    /// 
    /// Name templates can be passed to MakeMultipleApplyNameInstance along with
    /// an instance name to create the name for a particular instance.
    /// 
    USD_API
    static TfToken MakeMultipleApplyNameTemplate(
        const std::string &namespacePrefix, 
        const std::string &baseName);

    /// Returns an instance of a multiple apply schema name from the given 
    /// \p nameTemplate for the given \p instanceName.
    ///
    /// The returned name is created by replacing the instance name placeholder 
    /// "__INSTANCE_NAME__" in the name template with the given instance name.
    /// If the instance name placeholder is not found in \p nameTemplate, then 
    /// the name template is not multiple apply name template and is returned as
    /// is.
    /// 
    /// Note that the instance name placeholder must be found as an exact full
    /// word match with one of the tokenized components of the name template, 
    /// when tokenized by the namespace delimiter, in order for it to be treated
    /// as a placeholder and substituted with the instance name.
    ///
    USD_API
    static TfToken MakeMultipleApplyNameInstance(
        const std::string &nameTemplate,
        const std::string &instanceName);

    /// Returns the base name for the multiple apply schema name template 
    /// \p nameTemplate.
    ///
    /// The base name is the substring of the given name template that comes
    /// after the instance name placeholder and the subsequent namespace 
    /// delimiter. If the given property name does not contain the instance name
    /// placeholder, it is not a name template and the name template is returned
    /// as is.
    ///
    USD_API
    static TfToken GetMultipleApplyNameTemplateBaseName(
        const std::string &nameTemplate);

    /// Returns true if \p nameTemplate is a multiple apply schema name 
    /// template.
    ///
    /// The given \p nameTemplate is a name template if and only if it 
    /// contains the instance name place holder "__INSTANCE_NAME__" as an exact
    /// match as one of the tokenized components of the name tokenized by
    /// the namespace delimiter.
    ///
    USD_API
    static bool IsMultipleApplyNameTemplate(
        const std::string &nameTemplate);

    /// Finds the prim definition for the given \p typeName token if 
    /// \p typeName is a registered concrete typed schema type. Returns null if
    /// it is not.
    const UsdPrimDefinition* FindConcretePrimDefinition(
        const TfToken &typeName) const {
        const auto it = _concreteTypedPrimDefinitions.find(typeName);
        return it != _concreteTypedPrimDefinitions.end() ? it->second.get() : nullptr;
    }

    /// Finds the prim definition for the given \p typeName token if 
    /// \p typeName is a registered applied API schema type. Returns null if
    /// it is not.
    const UsdPrimDefinition *FindAppliedAPIPrimDefinition(
        const TfToken &typeName) const {
        // Check the single apply API schemas first then check for multiple
        // apply schemas. This function will most often be used to find a 
        // single apply schema's prim definition as the prim definitions for
        // multiple apply schemas aren't generally useful.
        const auto it = _appliedAPIPrimDefinitions.find(typeName);
        if (it != _appliedAPIPrimDefinitions.end()) {
            return it->second.get();
        }
        const auto multiIt = _multiApplyAPIPrimDefinitions.find(typeName);
        return multiIt != _multiApplyAPIPrimDefinitions.end() ? 
            multiIt->second : nullptr;
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

    // For the given full API schema name (which may be "type:instance" for 
    // multiple apply API schemas), finds and returns the prim definition for 
    // the API schema type. If the API schema is an instance of a multiple 
    // apply API, the instance name will be set in instanceName.
    const UsdPrimDefinition *_FindAPIPrimDefinitionByFullName(
        const TfToken &apiSchemaName, 
        TfToken *instanceName) const;

    void _ComposeAPISchemasIntoPrimDefinition(
        UsdPrimDefinition *primDef, 
        const TfTokenVector &appliedAPISchemas) const;

    // Private class for helping initialize the schema registry. Defined 
    // entirely in the implementation. Declared here for private access to the
    // registry.
    class _SchemaDefInitHelper;

    using _TypeNameToPrimDefinitionMap = std::unordered_map<
        TfToken, const std::unique_ptr<UsdPrimDefinition>, TfToken::HashFunctor>;

    SdfLayerRefPtr _schematics;

    _TypeNameToPrimDefinitionMap _concreteTypedPrimDefinitions;
    _TypeNameToPrimDefinitionMap _appliedAPIPrimDefinitions;

    // This is a mapping from multiple apply API schema name (e.g. 
    // "CollectionAPI") to the template prim definition stored for it in
    // _appliedAPIPrimDefinitions as the template prim definition is actually 
    // mapped to its template name (e.g. "CollectionAPI:__INSTANCE_NAME__") in
    // that map.
    std::unordered_map<TfToken, const UsdPrimDefinition *, TfToken::HashFunctor> 
        _multiApplyAPIPrimDefinitions;
    UsdPrimDefinition *_emptyPrimDefinition;

    VtDictionary _fallbackPrimTypes;

    friend class UsdPrimDefinition;
};

USD_API_TEMPLATE_CLASS(TfSingleton<UsdSchemaRegistry>);

// Utility function for extracting the metadata about applying API schemas from
// the plugin metadata for the schema's type. It is useful for certain clients
// to be able to access this plugin data in the same way that the 
// UsdSchemaRegistry does.
void Usd_GetAPISchemaPluginApplyToInfoForType(
    const TfType &apiSchemaType,
    const TfToken &apiSchemaName,
    std::map<TfToken, TfTokenVector> *autoApplyAPISchemasMap,
    TfHashMap<TfToken, TfTokenVector, TfHash> *canOnlyApplyAPISchemasMap,
    TfHashMap<TfToken, TfToken::Set, TfHash> *allowedInstanceNamesMap);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_SCHEMA_REGISTRY_H
