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
#ifndef USD_SCHEMAREGISTRY_H
#define USD_SCHEMAREGISTRY_H

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

/// \class UsdSchemaRegistry
///
/// Singleton registry that provides access to prim and property definition
/// information for registered Usd "IsA" schema types.
///
/// The data contained herein comes from the processed (by \em usdGenSchema)
/// schema.usda files of each schema-defining module.  The data is returned
/// in the form of SdfSpec's of the appropriate subtype.
///
/// It is used by the Usd core to determine how to create scene description
/// for un-instantiated "builtin" properties of schema classes, and also
/// to enumerate all properties for a given schema class, and finally to
/// provide fallback values for unauthored builtin properties.
///
class UsdSchemaRegistry : public TfSingleton<UsdSchemaRegistry> {
public:
    USD_API
    static UsdSchemaRegistry& GetInstance() {
        return TfSingleton<UsdSchemaRegistry>::GetInstance();
    }

    static const SdfLayerRefPtr & GetSchematics() {
        return GetInstance()._schematics;
    }

    /// Return the PrimSpec that contains all the builtin metadata and
    /// properties for the given \a primType.  Return null if there is no such
    /// prim definition.
    USD_API
    static SdfPrimSpecHandle GetPrimDefinition(const TfToken &primType);

    /// Return the PrimSpec that contains all the bulitin metadata and
    /// properties for the given \a primType.  Return null if there is no such
    /// prim definition.
    USD_API
    static SdfPrimSpecHandle GetPrimDefinition(const TfType &primType);

    /// Return the PrimSpec that contains all the builtin metadata and
    /// properties for the given \p SchemaType.  Return null if there is no such
    /// prim definition.
    template <class SchemaType>
    static SdfPrimSpecHandle GetPrimDefinition() {
        return GetPrimDefinition(SchemaType::_GetStaticTfType());
    }

    /// Return the property spec that defines the fallback for the property
    /// named \a propName on prims of type \a primType.  Return null if there is
    /// no such property definition.
    USD_API
    static SdfPropertySpecHandle
    GetPropertyDefinition(const TfToken& primType,
                          const TfToken& propName);

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfAttributeSpecHandle>(
    ///     GetPropertyDefinition(primType, attrName));
    USD_API
    static SdfAttributeSpecHandle
    GetAttributeDefinition(const TfToken& primType,
                           const TfToken& attrName);

    /// This is a convenience method. It is shorthand for
    /// TfDynamic_cast<SdfRelationshipSpecHandle>(
    ///     GetPropertyDefinition(primType, relName));
    USD_API
    static SdfRelationshipSpecHandle
    GetRelationshipDefinition(const TfToken& primType, const TfToken& relName);

    /// Return the SdfSpecType for \p primType and \p propName if those identify
    /// a builtin property.  Otherwise return SdfSpecTypeUnknown.
    static SdfSpecType GetSpecType(const TfToken &primType,
                                   const TfToken &propName) {
        const UsdSchemaRegistry &self = GetInstance();
        if (const SdfAbstractDataSpecId *specId =
            self._GetSpecId(primType, propName)) {
            return self._schematics->GetSpecType(*specId);
        }
        return SdfSpecTypeUnknown;
    }

    /// Return in \p value the field for the property named \p propName
    /// under the prim for type \p primType or for the prim if \p propName
    /// is empty.  Returns \c true if the value exists, \c false otherwise.
    // XXX: Getting these fields via the methods that return spec
    //      handles will be slower than using this method.  It's
    //      questionable if those methods should exist at all.
    template <class T>
    static bool HasField(const TfToken& primType,
                         const TfToken& propName,
                         const TfToken& fieldName, T* value)
    {
        const UsdSchemaRegistry &self = GetInstance();
        if (const SdfAbstractDataSpecId *specId =
            self._GetSpecId(primType, propName)) {
            if (self._schematics->HasField(*specId, fieldName, value))
                return true;
        }
        return false;
    }

    template <class T>
    static bool HasFieldDictKey(const TfToken& primType,
                                const TfToken& propName,
                                const TfToken& fieldName,
                                const TfToken& keyPath,
                                T* value)
    {
        const UsdSchemaRegistry &self = GetInstance();
        if (const SdfAbstractDataSpecId *specId =
            self._GetSpecId(primType, propName)) {
            if (self._schematics->HasFieldDictKey(
                    *specId, fieldName, keyPath, value)) {
                return true;
            }
        }
        return false;
    }

    /// Returns list of fields that cannot have fallback values
    /// specified in schemas. 
    /// 
    /// Fields are generally in this list because their fallback values
    /// aren't used. For instance, fallback values for composition arcs
    /// aren't used during composition, so allowing them to be set in
    /// schemas would be misleading.
    USD_API
    static std::vector<TfToken> GetDisallowedFields();

    /// Returns true if the prim type \p primType inherits from \ref UsdTyped. 
    USD_API
    static bool IsTyped(const TfType& primType);

    /// Returns true if the prim type \p primType is instantiable
    /// in scene description.
    USD_API
    static bool IsConcrete(const TfType& primType);

    /// Returns true if \p apiSchemaType is an applied API schema type.
    USD_API
    bool IsAppliedAPISchema(const TfType& apiSchemaType);

    /// Returns true if \p apiSchemaType is a multiple-apply API schema type.
    USD_API
    bool IsMultipleApplyAPISchema(const TfType& apiSchemaType);
    

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

private:
    friend class TfSingleton<UsdSchemaRegistry>;

    UsdSchemaRegistry();

    // Helper for template GetPrimDefinition.
    static SdfPrimSpecHandle
    _GetPrimDefinitionAtPath(const SdfPath &path);

    // Helper for looking up the prim definition path for a given primType.
    const SdfPath& _GetSchemaPrimPath(const TfToken &primType) const;

    // Helper for looking up the prim definition path for a given primType.
    const SdfPath& _GetSchemaPrimPath(const TfType &primType) const;

    USD_API
    const SdfAbstractDataSpecId *_GetSpecId(const TfToken &primType,
                                            const TfToken &propName) const;

    void _FindAndAddPluginSchema();

    void _BuildPrimTypePropNameToSpecIdMap(const TfToken &typeName,
                                           const SdfPath &primPath);

    SdfLayerRefPtr _schematics;

    // Registered map of schema class type -> definition prim path.
    // XXX: Should drop this in favor of _TypeNameToPathMap but
    //      TfType should have a GetTypeNameToken() method so we
    //      don't have to construct a TfToken from a std::string.
    typedef TfHashMap<TfType, SdfPath, TfHash> _TypeToPathMap;
    _TypeToPathMap _typeToPathMap;

    typedef TfHashMap<TfToken, SdfPath, TfToken::HashFunctor>
        _TypeNameToPathMap;
    _TypeNameToPathMap _typeNameToPathMap;

    struct _TokenPairHash {
        inline size_t operator()(const std::pair<TfToken, TfToken> &p) const {
            size_t hash = p.first.Hash();
            boost::hash_combine(hash, p.second);
            return hash;
        }
    };

    // Cache of primType/propName to specId.
    typedef TfHashMap<
        std::pair<TfToken, TfToken>,
        const SdfAbstractDataSpecId *,
        _TokenPairHash> _PrimTypePropNameToSpecIdMap;
    _PrimTypePropNameToSpecIdMap _primTypePropNameToSpecIdMap;

    TfToken::HashSet _appliedAPISchemaNames;
    TfToken::HashSet _multipleApplyAPISchemaNames;
};

USD_API_TEMPLATE_CLASS(TfSingleton<UsdSchemaRegistry>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USD_SCHEMAREGISTRY_H
