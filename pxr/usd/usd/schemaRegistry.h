//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfAttributeSpec);
SDF_DECLARE_HANDLES(SdfRelationshipSpec);

class UsdSchemaBase;
class UsdPrimDefinition;

/// Schema versions are specified as a single unsigned integer value.
using UsdSchemaVersion = unsigned int;

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
class UsdSchemaRegistry : public TfWeakBase {
    UsdSchemaRegistry(const UsdSchemaRegistry&) = delete;
    UsdSchemaRegistry& operator=(const UsdSchemaRegistry&) = delete;
public:
    using TokenToTokenVectorMap = 
        std::unordered_map<TfToken, TfTokenVector, TfHash>;

    /// Structure that holds the information about a schema that is registered
    /// with the schema registry.
    struct SchemaInfo {

        /// The schema's identifier which is how the schema type is referred to
        /// in scene description and is also the key used to look up the 
        /// schema's prim definition.
        TfToken identifier;

        /// The schema's type as registered with the TfType registry. This will
        /// correspond to the C++ class of the schema if a class was generated
        /// for it.
        TfType type;

        /// The name of the family of schema's which the schema is a version
        /// of. This is the same as the schema identifier with the version 
        /// suffix removed (or exactly the same as the schema identifier in the
        /// case of version 0 of a schema which will not have a version suffix.)
        TfToken family;

        /// The version number of the schema within its schema family.
        UsdSchemaVersion version;

        /// The schema's kind: ConcreteTyped, SingleApplyAPI, etc.
        UsdSchemaKind kind;
    };

    USD_API
    static UsdSchemaRegistry& GetInstance() {
        return TfSingleton<UsdSchemaRegistry>::GetInstance();
    }

    /// Creates the schema identifier that would be used to define a schema of
    /// the given \p schemaFamily with the given \p schemaVersion.
    ///
    /// If the provided schema version is zero, the returned identifier will
    /// be the schema family itself. For all other versions, the returned 
    /// identifier will be the family followed by an underscore and the version
    /// number.
    ///
    /// If \p schemaFamily is not an 
    /// \ref IsAllowedSchemaFamily "allowed schema family", this function will
    /// append the appropriate version suffix, but the returned identifier will
    /// not be an \ref IsAllowedSchemaIdentifier "allowed schema identifier".
    USD_API
    static TfToken
    MakeSchemaIdentifierForFamilyAndVersion(
        const TfToken &schemaFamily, 
        UsdSchemaVersion schemaVersion);

    /// Parses and returns the schema family and version values from the given 
    /// \p schemaIdentifier.
    ///
    /// A schema identifier's version is indicated by a suffix consisting of an
    /// underscore followed by a positive integer which is its version. The
    /// schema family is the string before this suffix. If the identifier does
    /// not have a suffix matching this pattern, then the schema version is zero
    /// and the schema family is the identifier itself.
    ///
    /// For example:
    /// Identifier "FooAPI_1" returns ("FooAPI", 1)
    /// Identifier "FooAPI" returns ("FooAPI", 0)
    ///
    /// Note that this function only parses what the schema family and version
    /// would be for the given schema identifier and does not require that 
    /// \p schemaIdentifier be a registered schema itself or even an 
    /// \ref IsAllowedSchemaIdentifier "allowed schema identifier".
    USD_API
    static std::pair<TfToken, UsdSchemaVersion> 
    ParseSchemaFamilyAndVersionFromIdentifier(const TfToken &schemaIdentifier);

    /// Returns whether the given \p schemaFamily is an allowed schema family
    /// name.
    ///
    /// A schema family is allowed if it's a 
    /// \ref SdfPath::IsValidIdentifier "valid identifier" 
    /// and does not itself contain a 
    /// \ref ParseSchemaFamilyAndVersionFromIdentifier "version suffix".
    USD_API
    static bool
    IsAllowedSchemaFamily(const TfToken &schemaFamily);

    /// Returns whether the given \p schemaIdentifier is an allowed schema 
    /// identifier.
    ///
    /// A schema identifier is allowed if it can be  
    /// \ref ParseSchemaFamilyAndVersionFromIdentifier "parsed" into a 
    /// \ref IsAllowedSchemaFamily "allowed schema family" and schema version
    /// and it is the identifier that would be 
    /// \ref MakeSchemaIdentifierForFamilyAndVersion "created" from that parsed
    /// family and version.
    USD_API
    static bool
    IsAllowedSchemaIdentifier(const TfToken &schemaIdentifier);

    /// Finds and returns the schema info for a registered schema with the 
    /// given \p schemaType. Returns null if no registered schema with the 
    /// schema type exists.
    USD_API
    static const SchemaInfo *
    FindSchemaInfo(const TfType &schemaType);

    /// Finds and returns the schema info for a registered schema with the 
    /// C++ schema class \p SchemaType. 
    ///
    /// All generated C++ schema classes, i.e. classes that derive from 
    /// UsdSchemaBase, are expected to have their types registered with the
    /// schema registry and as such, the return value from this function should
    /// never be null. A null return value is indication of a coding error even
    /// though this function itself will not report an error.
    template <class SchemaType>
    static const SchemaInfo *
    FindSchemaInfo() {
        static_assert(std::is_base_of<UsdSchemaBase, SchemaType>::value,
            "Provided type must derive UsdSchemaBase.");
        return FindSchemaInfo(SchemaType::_GetStaticTfType());
    }

    /// Finds and returns the schema info for a registered schema with the 
    /// given \p schemaIdentifier. Returns null if no registered schema with the 
    /// schema identifier exists.
    USD_API
    static const SchemaInfo *
    FindSchemaInfo(const TfToken &schemaIdentifier);

    /// Finds and returns the schema info for a registered schema in the 
    /// given \p schemaFamily with the given \p schemaVersion. Returns null if 
    /// no registered schema in the schema family with the given version exists.
    USD_API
    static const SchemaInfo *
    FindSchemaInfo(const TfToken &schemaFamily, UsdSchemaVersion schemaVersion);

    /// A policy for filtering by schema version when querying for schemas in a
    /// particular schema family.
    enum class VersionPolicy {
        All,
        GreaterThan,
        GreaterThanOrEqual,
        LessThan,
        LessThanOrEqual
    };

    /// Finds all schemas in the given \p schemaFamily and returns their
    /// their schema info ordered from highest version to lowest version.
    USD_API
    static const std::vector<const SchemaInfo *> &
    FindSchemaInfosInFamily(
        const TfToken &schemaFamily);

    /// Finds all schemas in the given \p schemaFamily, filtered according to 
    /// the given \p schemaVersion and \p versionPolicy, and returns their
    /// their schema info ordered from highest version to lowest version.
    USD_API
    static std::vector<const SchemaInfo *>
    FindSchemaInfosInFamily(
        const TfToken &schemaFamily, 
        UsdSchemaVersion schemaVersion, 
        VersionPolicy versionPolicy);

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
    static const TokenToTokenVectorMap &GetAutoApplyAPISchemas();

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
        TokenToTokenVectorMap *autoApplyAPISchemas);

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
        return it != _concreteTypedPrimDefinitions.end() ? 
            it->second.get() : nullptr;
    }

    /// Finds the prim definition for the given \p typeName token if 
    /// \p typeName is a registered applied API schema type. Returns null if
    /// it is not.
    const UsdPrimDefinition *FindAppliedAPIPrimDefinition(
        const TfToken &typeName) const {
        const auto it = _appliedAPIPrimDefinitions.find(typeName);
        return it != _appliedAPIPrimDefinitions.end() ?
            it->second.primDef.get() : nullptr;
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

    using _FamilyAndInstanceToVersionMap = 
        std::unordered_map<std::pair<TfToken, TfToken>, UsdSchemaVersion, TfHash>;

    void _ComposeAPISchemasIntoPrimDefinition(
        UsdPrimDefinition *primDef, 
        const TfTokenVector &appliedAPISchemas,
        _FamilyAndInstanceToVersionMap *seenSchemaFamilyVersions) const;

    // Private class for helping initialize the schema registry. Defined 
    // entirely in the implementation. Declared here for private access to the
    // registry.
    class _SchemaDefInitHelper;

    std::vector<SdfLayerRefPtr> _schematicsLayers;

    std::unordered_map<TfToken, const std::unique_ptr<UsdPrimDefinition>,
         TfHash> _concreteTypedPrimDefinitions;

    struct _APISchemaDefinitionInfo {
        std::unique_ptr<UsdPrimDefinition> primDef;
        bool applyExpectsInstanceName;
    };
    std::unordered_map<TfToken, const _APISchemaDefinitionInfo, TfHash> 
        _appliedAPIPrimDefinitions;

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
    UsdSchemaRegistry::TokenToTokenVectorMap *autoApplyAPISchemasMap,
    UsdSchemaRegistry::TokenToTokenVectorMap *canOnlyApplyAPISchemasMap,
    TfHashMap<TfToken, TfToken::Set, TfHash> *allowedInstanceNamesMap);

// Utility for sorting a list of auto-applied API schemas. It is useful for 
// certain clients to be able to make sure they can perform this type of sort
// in the exact same way as UsdSchemaRegistry does.
void Usd_SortAutoAppliedAPISchemas(
    TfTokenVector *autoAppliedAPISchemas);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_SCHEMA_REGISTRY_H
