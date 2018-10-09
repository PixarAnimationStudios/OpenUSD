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
#ifndef SDF_SCHEMA_H
#define SDF_SCHEMA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include "pxr/base/plug/notice.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/vt/value.h"

#include <boost/function.hpp>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;
class SdfPath;
class SdfPayload;
class SdfReference;
class Sdf_ValueTypeRegistry;

TF_DECLARE_WEAK_PTRS(PlugPlugin);

/// \class SdfSchemaBase
///
/// Generic class that provides information about scene description fields
/// but doesn't actually provide any fields.
///
class SdfSchemaBase : public TfWeakBase, public boost::noncopyable {

protected:
    class _SpecDefiner;

public:
    /// \class FieldDefinition
    ///
    /// Class defining various attributes for a field.
    ///
    class FieldDefinition {
    public:
        FieldDefinition(
            const SdfSchemaBase& schema,
            const TfToken& name, 
            const VtValue& fallbackValue);

        typedef std::vector< std::pair<TfToken, JsValue> > InfoVec;

        SDF_API const TfToken& GetName() const;
        SDF_API const VtValue& GetFallbackValue() const;
        SDF_API const InfoVec& GetInfo() const;

        SDF_API bool IsPlugin() const;
        SDF_API bool IsReadOnly() const;
        SDF_API bool HoldsChildren() const;
        
        /// Validation functions that return true if a given value passes
        /// the registered validator or if no validator has been set.
        /// @{
        
        template <class T>
        SdfAllowed IsValidValue(const T& value) const
        {
            return (_valueValidator ? 
                    _valueValidator(_schema, VtValue(value)) : 
                    SdfAllowed(true));
        }

        template <class T>
        SdfAllowed IsValidListValue(const T& value) const
        {
            return (_listValueValidator ? 
                    _listValueValidator(_schema, VtValue(value)) : 
                    SdfAllowed(true));
        }

        template <class T>
        SdfAllowed IsValidMapKey(const T& value) const
        {
            return (_mapKeyValidator ? 
                    _mapKeyValidator(_schema, VtValue(value)) : 
                    SdfAllowed(true));
        }

        template <class T>
        SdfAllowed IsValidMapValue(const T& value) const
        {
            return (_mapValueValidator ? 
                    _mapValueValidator(_schema, VtValue(value)) : 
                    SdfAllowed(true));
        }

        /// @}

        /// Functions for setting field attributes during registration.
        /// @{

        FieldDefinition& FallbackValue(const VtValue& v);

        FieldDefinition& Plugin();
        FieldDefinition& Children();
        FieldDefinition& ReadOnly();
        FieldDefinition& AddInfo(const TfToken& tok, const JsValue& val);

        typedef boost::function<
            SdfAllowed(const SdfSchemaBase&, const VtValue&)> Validator;
        FieldDefinition& ValueValidator(const Validator& v);
        FieldDefinition& ListValueValidator(const Validator& v);
        FieldDefinition& MapKeyValidator(const Validator& v);
        FieldDefinition& MapValueValidator(const Validator& v);

        /// @}

    private:
        const SdfSchemaBase& _schema;
        TfToken _name;
        VtValue _fallbackValue;
        InfoVec _info;

        bool _isPlugin;
        bool _isReadOnly;
        bool _holdsChildren;

        Validator _valueValidator;
        Validator _listValueValidator;
        Validator _mapKeyValidator;
        Validator _mapValueValidator;
    };

    // Structure containing information about a field as it pertains to the
    // spec this object defines.
    struct _FieldInfo {
        _FieldInfo(): required(false), metadata(false) { }
        bool required;
        bool metadata;
        TfToken metadataDisplayGroup;
    };

    class SpecDefinition;

    /// \class SpecDefinition
    ///
    /// Class representing fields and other information for a spec type.
    ///
    class SpecDefinition {
    public:
        /// Returns all fields for this spec.
        SDF_API TfTokenVector GetFields() const;

        /// Returns all value fields marked as required for this spec.
        SDF_API TfTokenVector GetRequiredFields() const;

        /// Returns all value fields marked as metadata for this spec.
        SDF_API TfTokenVector GetMetadataFields() const;

        /// Returns whether the given field is valid for this spec.
        SDF_API bool IsValidField(const TfToken& name) const;

        /// Returns whether the given field is metadata for this spec.
        SDF_API bool IsMetadataField(const TfToken& name) const;

        /// Returns the display group for this metadata field.  Returns the
        /// empty token if this field is not a metadata field or if this
        /// metadata field has no display group.
        SDF_API 
        TfToken GetMetadataFieldDisplayGroup(const TfToken& name) const;

        /// Returns whether the given field is required for this spec.
        SDF_API bool IsRequiredField(const TfToken& name) const;


    private:
        typedef TfHashMap<TfToken, _FieldInfo, TfToken::HashFunctor> 
            _FieldMap;
        _FieldMap _fields;

    private:
        friend class _SpecDefiner;
        void _AddField(const TfToken& name, const _FieldInfo& fieldInfo);
    };

    /// Returns the field definition for the given field. 
    /// Returns NULL if no definition exists for given field.
    SDF_API 
    const FieldDefinition* GetFieldDefinition(const TfToken &fieldKey) const;
    
    /// Returns the spec definition for the given spec type.
    /// Returns NULL if no definition exists for the given spec type.
    SDF_API 
    const SpecDefinition* GetSpecDefinition(SdfSpecType type) const;

    /// Convenience functions for accessing specific field information.
    /// @{

    /// Return whether the specified field has been registered. Also
    /// optionally return the fallback value.
    SDF_API 
    bool IsRegistered(const TfToken &fieldKey, VtValue *fallback=NULL) const;

    /// Returns whether the given field is a 'children' field -- that is, it
    /// indexes certain children beneath the owning spec.
    SDF_API 
    bool HoldsChildren(const TfToken &fieldKey) const;

    /// Return the fallback value for the specified \p fieldKey or the
    /// empty value if \p fieldKey is not registered.
    SDF_API 
    const VtValue& GetFallback(const TfToken &fieldKey) const;

    /// Coerce \p value to the correct type for the specified field.
    SDF_API 
    VtValue CastToTypeOf(const TfToken &fieldKey, const VtValue &value) const;

    /// Return whether the given field is valid for the given spec type.
    SDF_API 
    bool IsValidFieldForSpec(const TfToken &fieldKey, SdfSpecType specType) const;

    /// Returns all fields registered for the given spec type.
    SDF_API TfTokenVector GetFields(SdfSpecType specType) const;

    /// Returns all metadata fields registered for the given spec type.
    SDF_API TfTokenVector GetMetadataFields(SdfSpecType specType) const;

    /// Return the metadata field display group for metadata \a metadataField on
    /// \a specType.  Return the empty token if \a metadataField is not a
    /// metadata field, or if it has no display group.
    SDF_API 
    TfToken GetMetadataFieldDisplayGroup(SdfSpecType specType,
                                         TfToken const &metadataField) const;

    /// Returns all required fields registered for the given spec type.
    SDF_API TfTokenVector GetRequiredFields(SdfSpecType specType) const;

    /// Return true if \p fieldName is a required field name for at least one
    /// spec type, return false otherwise.  The main use of this function is to
    /// quickly rule out field names that aren't required (and thus don't need
    /// special handling).
    inline bool IsRequiredFieldName(const TfToken &fieldName) const {
        for (size_t i = 0; i != _requiredFieldNames.size(); ++i) {
            if (_requiredFieldNames[i] == fieldName)
                return true;
        }
        return false;
    }

    /// @}

    /// Specific validation functions for various fields. These are internally
    /// registered as validators for the associated field, but can also be
    /// used directly.
    /// @{

    static SdfAllowed IsValidAttributeConnectionPath(const SdfPath& path);
    static SdfAllowed IsValidIdentifier(const std::string& name);
    static SdfAllowed IsValidNamespacedIdentifier(const std::string& name);
    static SdfAllowed IsValidInheritPath(const SdfPath& path);
    static SdfAllowed IsValidPayload(const SdfPayload& payload);
    static SdfAllowed IsValidReference(const SdfReference& ref);
    static SdfAllowed IsValidRelationshipTargetPath(const SdfPath& path);
    static SdfAllowed IsValidRelocatesPath(const SdfPath& path);
    static SdfAllowed IsValidSpecializesPath(const SdfPath& path);
    static SdfAllowed IsValidSubLayer(const std::string& sublayer);
    static SdfAllowed IsValidVariantIdentifier(const std::string& name);

    /// @}

    /// Scene description value types
    /// @{

    /// Given a value, check if it is a valid value type.
    /// This function only checks that the type of the value is valid
    /// for this schema. It does not imply that the value is valid for
    /// a particular field -- the field's validation function must be
    /// used for that.
    SdfAllowed IsValidValue(const VtValue& value) const;

    /// Returns all registered type names.
    std::vector<SdfValueTypeName> GetAllTypes() const;

    /// Return the type name object for the given type name string.
    SDF_API 
    SdfValueTypeName FindType(const std::string& typeName) const;

    /// Return the type name object for the given type and optional role.
    SDF_API 
    SdfValueTypeName FindType(const TfType& type,
                              const TfToken& role = TfToken()) const;

    /// Return the type name object for the value's type and optional role.
    SDF_API 
    SdfValueTypeName FindType(const VtValue& value,
                              const TfToken& role = TfToken()) const;

    /// Return the type name object for the given type name string if it
    /// exists otherwise create a temporary type name object.  Clients
    /// should not normally need to call this.
    SDF_API
    SdfValueTypeName FindOrCreateType(const std::string& typeName) const;

    /// @}

protected:
    /// \class _SpecDefiner
    ///
    /// Class that defines fields for a spec type.
    ///
    class _SpecDefiner {
    public:
        /// Functions for setting spec attributes during registration
        /// @{

        _SpecDefiner& Field(
            const TfToken& name, bool required = false);
        _SpecDefiner& MetadataField(
            const TfToken& name, bool required = false);
        _SpecDefiner& MetadataField(
            const TfToken& name, const TfToken& displayGroup,
            bool required = false);

        _SpecDefiner &CopyFrom(const SpecDefinition &other);

        /// @}
    private:
        friend class SdfSchemaBase;
        explicit _SpecDefiner(SdfSchemaBase *schema, SpecDefinition *definition)
            : _schema(schema)
            , _definition(definition)
            {}
        SdfSchemaBase *_schema;
        SpecDefinition *_definition;
    };

    /// A helper for registering value types.
    class _ValueTypeRegistrar {
    public:
        explicit _ValueTypeRegistrar(Sdf_ValueTypeRegistry*);

        class Type
        {
        public:
            // Specify a type with the given name, default value, and default
            // array value of VtArray<T>.
            template <class T>
            Type(const std::string& name, const T& defaultValue)
                : _name(name)
                , _defaultValue(defaultValue)
                , _defaultArrayValue(VtArray<T>())
            { }

            // Specify a type with the given name and underlying C++ type.
            // No default value or array value will be registered.
            Type(const std::string& name, const TfType& type)
                : _name(name)
                , _type(type)
            { }

            // Set C++ type name string for this type. Defaults to type name
            // from TfType.
            Type& CPPTypeName(const std::string& cppTypeName)
            {
                _cppTypeName = cppTypeName;
                if (!_defaultArrayValue.IsEmpty()) {
                    _arrayCppTypeName = "VtArray<" + cppTypeName + ">";
                }
                return *this;
            }

            // Set shape for this type. Defaults to shapeless.
            Type& Dimensions(const SdfTupleDimensions& dims)
            { _dimensions = dims; return *this; }

            // Set default unit for this type. Defaults to dimensionless unit.
            Type& DefaultUnit(TfEnum unit) { _unit = unit; return *this; }

            // Set role for this type. Defaults to no role.
            Type& Role(const TfToken& role) { _role = role; return *this; }

            // Indicate that arrays of this type are not supported.
            Type& NoArrays() 
            { 
                _defaultArrayValue = VtValue(); 
                _arrayCppTypeName = std::string();
                return *this; 
            }

        private:
            friend class _ValueTypeRegistrar;

            std::string _name;
            TfType _type;
            VtValue _defaultValue, _defaultArrayValue;
            std::string _cppTypeName, _arrayCppTypeName;
            TfEnum _unit;
            TfToken _role;
            SdfTupleDimensions _dimensions;
        };

        /// Register a value type and its corresponding array value type.
        void AddType(const Type& type);

    private:
        Sdf_ValueTypeRegistry* _registry;
    };

    SdfSchemaBase();
    virtual ~SdfSchemaBase();

    /// Creates and registers a new field named \p fieldKey with the fallback
    /// value \p fallback. If \p plugin is specified, it indicates that this
    /// field is not a built-in field from this schema, but rather a field
    /// that was externally registered.
    ///
    /// It is a fatal error to call this function with a key that has already
    /// been used for another field.
    template <class T>
    FieldDefinition& _RegisterField(
        const TfToken &fieldKey, const T &fallback, bool plugin = false)
    {
        return _CreateField(fieldKey, VtValue(fallback), plugin);
    }

    /// Returns the SpecDefinition for the given spec type. Subclasses may
    /// then extend this definition by specifying additional fields.
    _SpecDefiner _ExtendSpecDefinition(SdfSpecType specType);

    /// Registers the standard fields.
    void _RegisterStandardFields();

    /// Returns a type registrar.
    _ValueTypeRegistrar _GetTypeRegistrar() const;

    /// Factory function for creating a default value for a metadata
    /// field. The parameters are the value type name and default
    /// value (if any) specified in the defining plugin.
    typedef std::function<VtValue(const std::string&, const JsValue&)>
        _DefaultValueFactoryFn;

    /// Registers all metadata fields specified in the given plugins
    /// under the given metadata tag.
    const std::vector<const SdfSchemaBase::FieldDefinition *>
    _UpdateMetadataFromPlugins(const PlugPluginPtrVector& plugins,
                                    const std::string& metadataTag = 
                                        std::string(),
                                    const _DefaultValueFactoryFn& defFactory = 
                                        _DefaultValueFactoryFn());

private:
    friend class _SpecDefiner;

    // Return a _SpecDefiner for the internal definition associated with \p type.
    _SpecDefiner _Define(SdfSpecType type) {
        return _SpecDefiner(this, &_specDefinitions[type]);
    }

    // Return a _SpecDefiner for an existing spec definition, \p local.
    _SpecDefiner _Define(SpecDefinition *local) {
        return _SpecDefiner(this, local);
    }

    void _AddRequiredFieldName(const TfToken &name);

    const SpecDefinition* _CheckAndGetSpecDefinition(SdfSpecType type) const;

    friend struct Sdf_SchemaFieldTypeRegistrar;
    FieldDefinition& _CreateField(
        const TfToken &fieldKey, const VtValue &fallback, bool plugin = false);

    template <class T>
    FieldDefinition& _DoRegisterField(const TfToken &fieldKey, const T &fallback)
    {
        return _DoRegisterField(fieldKey, VtValue(fallback));
    }

    FieldDefinition& _DoRegisterField(
        const TfToken &fieldKey, const VtValue &fallback);

private:
    typedef TfHashMap<TfToken, SdfSchemaBase::FieldDefinition, 
                                 TfToken::HashFunctor> 
        _FieldDefinitionMap;
    _FieldDefinitionMap _fieldDefinitions;
    
    typedef TfHashMap<SdfSpecType, SdfSchemaBase::SpecDefinition, 
                                 TfHash> 
        _SpecDefinitionMap;
    _SpecDefinitionMap _specDefinitions;

    std::unique_ptr<Sdf_ValueTypeRegistry> _valueTypeRegistry;
    TfTokenVector _requiredFieldNames;
};

/// \class SdfSchema
///
/// Class that provides information about the various scene description 
/// fields.
///
class SdfSchema : public SdfSchemaBase {
public:
    SDF_API
    static const SdfSchema& GetInstance()
    {
        return TfSingleton<SdfSchema>::GetInstance();
    }

private:
    friend class TfSingleton<SdfSchema>;
    SdfSchema();
    virtual ~SdfSchema();

    static void _RegisterTypes(_ValueTypeRegistrar registry);

    void _OnDidRegisterPlugins(const PlugNotice::DidRegisterPlugins& n);

    const Sdf_ValueTypeNamesType* _NewValueTypeNames() const;
    friend struct Sdf_ValueTypeNamesType::_Init;
};

SDF_API_TEMPLATE_CLASS(TfSingleton<SdfSchema>);

///
/// The following fields are pre-registered by Sdf. 
/// \showinitializer
#define SDF_FIELD_KEYS                                       \
    ((Active, "active"))                                     \
    ((AllowedTokens, "allowedTokens"))                       \
    ((AssetInfo, "assetInfo"))                               \
    ((ColorConfiguration, "colorConfiguration"))             \
    ((ColorManagementSystem, "colorManagementSystem"))       \
    ((ColorSpace, "colorSpace"))                             \
    ((Comment, "comment"))                                   \
    ((ConnectionPaths, "connectionPaths"))                   \
    ((Custom, "custom"))                                     \
    ((CustomData, "customData"))                             \
    ((CustomLayerData, "customLayerData"))                   \
    ((Default, "default"))                                   \
    ((DefaultPrim, "defaultPrim"))                           \
    ((DisplayGroup, "displayGroup"))                         \
    ((DisplayName, "displayName"))                           \
    ((DisplayUnit, "displayUnit"))                           \
    ((Documentation, "documentation"))                       \
    ((EndTimeCode, "endTimeCode"))                           \
    ((FramePrecision, "framePrecision"))                     \
    ((FramesPerSecond, "framesPerSecond"))                   \
    ((Hidden, "hidden"))                                     \
    ((HasOwnedSubLayers, "hasOwnedSubLayers"))               \
    ((InheritPaths, "inheritPaths"))                         \
    ((Instanceable, "instanceable"))                         \
    ((Kind, "kind"))                                         \
    ((MapperArgValue, "value"))                              \
    ((Marker, "marker"))                                     \
    ((PrimOrder, "primOrder"))                               \
    ((NoLoadHint, "noLoadHint"))                             \
    ((Owner, "owner"))                                       \
    ((Payload, "payload"))                                   \
    ((Permission, "permission"))                             \
    ((Prefix, "prefix"))                                     \
    ((PrefixSubstitutions, "prefixSubstitutions"))           \
    ((PropertyOrder, "propertyOrder"))                       \
    ((References, "references"))                             \
    ((Relocates, "relocates"))                               \
    ((Script, "script"))                                     \
    ((SessionOwner, "sessionOwner"))                         \
    ((Specializes, "specializes"))                           \
    ((Specifier, "specifier"))                               \
    ((StartTimeCode, "startTimeCode"))                       \
    ((SubLayers, "subLayers"))                               \
    ((SubLayerOffsets, "subLayerOffsets"))                   \
    ((Suffix, "suffix"))                                     \
    ((SuffixSubstitutions, "suffixSubstitutions"))           \
    ((SymmetricPeer, "symmetricPeer"))                       \
    ((SymmetryArgs, "symmetryArgs"))                         \
    ((SymmetryArguments, "symmetryArguments"))               \
    ((SymmetryFunction, "symmetryFunction"))                 \
    ((TargetPaths, "targetPaths"))                           \
    ((TimeSamples, "timeSamples"))                           \
    ((TimeCodesPerSecond, "timeCodesPerSecond"))             \
    ((TypeName, "typeName"))                                 \
    ((VariantSelection, "variantSelection"))                 \
    ((Variability, "variability"))                           \
    ((VariantSetNames, "variantSetNames"))                   \
                                                             \
    /* XXX: These fields should move into Sd. See bug 123508. */ \
    ((EndFrame, "endFrame"))                                 \
    ((StartFrame, "startFrame"))

#define SDF_CHILDREN_KEYS                                    \
    ((ConnectionChildren, "connectionChildren"))             \
    ((ExpressionChildren, "expressionChildren"))             \
    ((MapperArgChildren, "mapperArgChildren"))               \
    ((MapperChildren, "mapperChildren"))                     \
    ((PrimChildren, "primChildren"))                         \
    ((PropertyChildren, "properties"))                       \
    ((RelationshipTargetChildren, "targetChildren"))         \
    ((VariantChildren, "variantChildren"))                   \
    ((VariantSetChildren, "variantSetChildren"))

TF_DECLARE_PUBLIC_TOKENS(SdfFieldKeys, SDF_API, SDF_FIELD_KEYS);
TF_DECLARE_PUBLIC_TOKENS(SdfChildrenKeys, SDF_API, SDF_CHILDREN_KEYS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_SCHEMA_H
