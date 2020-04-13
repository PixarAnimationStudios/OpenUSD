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

    /// Return the type name in the USD schema for prims of the given registered
    /// \p primType.
    TfToken GetSchemaTypeName(const TfType &schemaType) const {
        auto iter = _typeToUsdTypeNameMap.find(schemaType);
        return iter != _typeToUsdTypeNameMap.end() ? iter->second : TfToken();
    }

    /// Return the type name in the USD schema for prims of the given
    /// \p SchemaType.
    template <class SchemaType>
    TfToken GetSchemaTypeName() const {
        return GetSchemaTypeName(SchemaType::_GetStaticTfType());
    }

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

    /// Returns true if the prim type \p primType is instantiable
    /// in scene description.
    USD_API
    bool IsConcrete(const TfType& primType) const;

    /// Returns true if the prim type \p primType is instantiable
    /// in scene description.
    USD_API
    bool IsConcrete(const TfToken& primType) const;

    /// Returns true if \p apiSchemaType is an applied API schema type.
    USD_API
    bool IsAppliedAPISchema(const TfType& apiSchemaType) const;

    /// Returns true if \p apiSchemaType is an applied API schema type.
    USD_API
    bool IsAppliedAPISchema(const TfToken& apiSchemaType) const;

    /// Returns true if \p apiSchemaType is a multiple-apply API schema type.
    USD_API
    bool IsMultipleApplyAPISchema(const TfType& apiSchemaType) const;
    
    /// Returns true if \p apiSchemaType is a multiple-apply API schema type.
    USD_API
    bool IsMultipleApplyAPISchema(const TfToken& apiSchemaType) const;
        
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
    /// and list of \p applieSchemas. This prim definition will contain a union
    /// of properties from the registered prim definitions of each of the 
    /// provided types. 
    USD_API
    std::unique_ptr<UsdPrimDefinition>
    BuildComposedPrimDefinition(
        const TfToken &primType, const TfTokenVector &appliedAPISchemas) const;

private:
    friend class TfSingleton<UsdSchemaRegistry>;

    UsdSchemaRegistry();

    void _FindAndAddPluginSchema();

    void _ApplyAPISchemasToPrimDefinition(
        UsdPrimDefinition *primDef, const TfTokenVector &appliedAPISchemas) const;

    SdfLayerRefPtr _schematics;

    // Registered map of schema class type -> Usd schema type name token.
    typedef TfHashMap<TfType, TfToken, TfHash> _TypeToTypeNameMap;
    _TypeToTypeNameMap _typeToUsdTypeNameMap;

    typedef TfHashMap<TfToken, UsdPrimDefinition *, 
                      TfToken::HashFunctor> _TypeNameToPrimDefinitionMap;

    _TypeNameToPrimDefinitionMap _concreteTypedPrimDefinitions;
    _TypeNameToPrimDefinitionMap _appliedAPIPrimDefinitions;
    UsdPrimDefinition *_emptyPrimDefinition;

    TfHashMap<TfToken, TfToken, TfToken::HashFunctor> 
        _multipleApplyAPISchemaNamespaces;

    friend class UsdPrimDefinition;
};

USD_API_TEMPLATE_CLASS(TfSingleton<UsdSchemaRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_SCHEMA_REGISTRY_H
