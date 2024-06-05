//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdr/debugCodes.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdrPropertyTypes, SDR_PROPERTY_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyMetadata, SDR_PROPERTY_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyRole, SDR_PROPERTY_ROLE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyTokens, SDR_PROPERTY_TOKENS);

using ShaderMetadataHelpers::GetRoleFromMetadata;
using ShaderMetadataHelpers::IsTruthy;
using ShaderMetadataHelpers::StringVal;
using ShaderMetadataHelpers::TokenVal;
using ShaderMetadataHelpers::TokenVecVal;

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor>
        TokenToSdfTypeMap;

namespace {
    // This only establishes EXACT mappings. If a mapping is not included here,
    // a one-to-one mapping isn't possible.
    const TokenToSdfTypeMap& _GetTokenTypeToSdfType()
    {
        static const TokenToSdfTypeMap tokenTypeToSdfType  = {
            {SdrPropertyTypes->Int,     SdfValueTypeNames->Int},
            {SdrPropertyTypes->String,  SdfValueTypeNames->String},
            {SdrPropertyTypes->Float,   SdfValueTypeNames->Float},
            {SdrPropertyTypes->Color,   SdfValueTypeNames->Color3f},
            {SdrPropertyTypes->Color4,  SdfValueTypeNames->Color4f},
            {SdrPropertyTypes->Point,   SdfValueTypeNames->Point3f},
            {SdrPropertyTypes->Normal,  SdfValueTypeNames->Normal3f},
            {SdrPropertyTypes->Vector,  SdfValueTypeNames->Vector3f},
            {SdrPropertyTypes->Matrix,  SdfValueTypeNames->Matrix4d}
        };
        return tokenTypeToSdfType;
    }

    // The array equivalent of the above map
    const TokenToSdfTypeMap& _GetTokenTypeToSdfArrayType()
    { 
        static const TokenToSdfTypeMap tokenTypeToSdfArrayType  = {
            {SdrPropertyTypes->Int,     SdfValueTypeNames->IntArray},
            {SdrPropertyTypes->String,  SdfValueTypeNames->StringArray},
            {SdrPropertyTypes->Float,   SdfValueTypeNames->FloatArray},
            {SdrPropertyTypes->Color,   SdfValueTypeNames->Color3fArray},
            {SdrPropertyTypes->Color4,  SdfValueTypeNames->Color4fArray},
            {SdrPropertyTypes->Point,   SdfValueTypeNames->Point3fArray},
            {SdrPropertyTypes->Normal,  SdfValueTypeNames->Normal3fArray},
            {SdrPropertyTypes->Vector,  SdfValueTypeNames->Vector3fArray},
            {SdrPropertyTypes->Matrix,  SdfValueTypeNames->Matrix4dArray}
        };
        return tokenTypeToSdfArrayType;
    }

    // Map of SdfValueTypeName's aliases to corresponding SdfValueTypeName
    // Refer SdfValueTypeName::GetAliasesAsTokens.
    // This is used to determine SdfValueTypeName from the SdrUsdDefinitionType
    // metadata.
    const TokenToSdfTypeMap& _GetAliasesTokensToSdfValueTypeNames()
    {
        auto MapBuilder = []() {
            TokenToSdfTypeMap result;
            for (const auto& typeName : SdfSchema::GetInstance().GetAllTypes()) {
                // Insert typeName itself as an alias
                result.emplace(typeName.GetAsToken(), typeName);
                // Insert all other aliases for the type
                for (const auto& aliasToken : typeName.GetAliasesAsTokens()) {
                    result.emplace(aliasToken, typeName);
                }
            }
            return result;
        };
        static const TokenToSdfTypeMap &aliasesTokensToSdfValueTypeNames =
            MapBuilder();
        return aliasesTokensToSdfValueTypeNames;
    }


    // -------------------------------------------------------------------------

    // The following typedefs are only needed to support the table below that
    // indicates how to convert an SdrPropertyType given a particular "role"
    // value
    typedef std::unordered_map<
            TfToken, std::pair<TfToken, size_t>, TfToken::HashFunctor>
        TokenToPairTable;

    typedef std::unordered_map<TfToken, TokenToPairTable, TfToken::HashFunctor>
            TokenToMapTable;

    // Establishes exact mappings for converting SdrPropertyTypes using "role"
    // The keys are original SdrPropertyTypes, and the value is another map,
    // keyed by the "role" metadata value. The value of that map is the
    // converted SdrPropertyType and array size.
    const TokenToMapTable& _GetConvertedSdrTypes()
    {
        static const TokenToMapTable convertedSdrTypes = {
            {SdrPropertyTypes->Color,
                {
                    {SdrPropertyRole->None, {SdrPropertyTypes->Float, 3}}
                }
            },
            {SdrPropertyTypes->Color4,
                {
                    {SdrPropertyRole->None, {SdrPropertyTypes->Float, 4}}
                }
            },
            {SdrPropertyTypes->Point,
                {
                    {SdrPropertyRole->None, {SdrPropertyTypes->Float, 3}}
                }
            },
            {SdrPropertyTypes->Normal,
                {
                    {SdrPropertyRole->None, {SdrPropertyTypes->Float, 3}}
                }
            },
            {SdrPropertyTypes->Vector,
                {
                    {SdrPropertyRole->None, {SdrPropertyTypes->Float, 3}}
                }
            }
        };
        return convertedSdrTypes;
    }

    // -------------------------------------------------------------------------

    SdfValueTypeName
    _GetSdrUsdDefinitionType(const NdrTokenMap &metadata)
    {
        const TfToken &sdrUsdDefinitionType = 
            TfToken(StringVal(
                        SdrPropertyMetadata->SdrUsdDefinitionType, metadata));

        if (sdrUsdDefinitionType.IsEmpty()) {
            return SdfValueTypeName();
        }

        const TokenToSdfTypeMap &aliasesTokensToSdfValueTypeNames =
            _GetAliasesTokensToSdfValueTypeNames();

        if (aliasesTokensToSdfValueTypeNames.find(sdrUsdDefinitionType) == 
                aliasesTokensToSdfValueTypeNames.end()) {
            TF_WARN("Invalid SdfValueTypeName or alias provided for "
                    "sdrUsdDefinitionType metadata: %s", 
                    sdrUsdDefinitionType.GetText());
            return SdfValueTypeName();
        }

        return aliasesTokensToSdfValueTypeNames.at(sdrUsdDefinitionType);
    }

    // Returns true if the arraySize or the metadata indicate that the property
    // has an array type
    bool
    _IsArray(size_t arraySize, const NdrTokenMap &metadata)
    {
        bool isDynamicArray =
            IsTruthy(SdrPropertyMetadata->IsDynamicArray, metadata);
        return arraySize > 0 || isDynamicArray;
    }

    // Determines if the metadata contains a key identifying the property as an
    // asset identifier
    bool
    _IsAssetIdentifier(const NdrTokenMap& metadata)
    {
        return metadata.count(SdrPropertyMetadata->IsAssetIdentifier);
    }

    // Returns true is this property is a default input on the shader node
    bool
    _IsDefaultInput(const NdrTokenMap &metadata)
    {
        return metadata.count(SdrPropertyMetadata->DefaultInput);
    }

    // Returns the type indicator based on the type mappings defined in
    // _GetTokenTypeToSdfType and _GetTokenTypeToSdfArrayType. If the type can't
    // be found the SdfType will be returned as Token with the original type as
    // a hint.
    const NdrSdfTypeIndicator
    _GetTypeIndicatorFromDefaultMapping(const TfToken& type, bool isArray)
    {
        const TokenToSdfTypeMap& tokenTypeToSdfType =
            isArray ? _GetTokenTypeToSdfArrayType() : _GetTokenTypeToSdfType();

        TokenToSdfTypeMap::const_iterator it = tokenTypeToSdfType.find(type);
        if (it != tokenTypeToSdfType.end()) {
            return std::make_pair(it->second, TfToken());
        }

        // If there is no clean mapping, it defaults to the 'Token' type
        return std::make_pair(SdfValueTypeNames->Token, type);
    }

    // -------------------------------------------------------------------------

    // Encoding 0: this is the mapping from Sdr types to Sdf types that was used
    //             internally at the inception of the Sdr at Pixar Animation
    //             Studios
    namespace _Encoding_0 {
        const NdrSdfTypeIndicator
        GetTypeAsSdfType(
            const TfToken& type, size_t arraySize, const NdrTokenMap& metadata)
        {
            bool isArray = _IsArray(arraySize, metadata);

            // There is one Sdf type (Asset) that is not included in the type
            // mapping because it is determined dynamically
            if (_IsAssetIdentifier(metadata)) {
                return std::make_pair(isArray ? SdfValueTypeNames->StringArray
                                              : SdfValueTypeNames->String,
                                      TfToken());
            }

            if (type == SdrPropertyTypes->Terminal) {
                return std::make_pair(SdfValueTypeNames->Token, type);
            }

            if (type == SdrPropertyTypes->Struct) {
                return std::make_pair(SdfValueTypeNames->String, type);
            }

            if (type == SdrPropertyTypes->Vstruct) {
                return std::make_pair(isArray ? SdfValueTypeNames->FloatArray 
                                              : SdfValueTypeNames->Float,
                                      type);
            }

            return _GetTypeIndicatorFromDefaultMapping(type, isArray);
        }
    }

    // Encoding 1: this is the original mapping from Sdr types to Sdf types that
    //             is used to store attributes in USD.
    namespace _Encoding_1 {
        const NdrSdfTypeIndicator
        GetTypeAsSdfType(
            const TfToken& type, size_t arraySize, const NdrTokenMap& metadata)
        {
            const SdfValueTypeName& sdfValueTypeName = 
                _GetSdrUsdDefinitionType(metadata);
            if (sdfValueTypeName) {
                return std::make_pair(sdfValueTypeName, TfToken());
            }

            bool isArray = _IsArray(arraySize, metadata);

            // There is one Sdf type (Asset) that is not included in the type
            // mapping because it is determined dynamically
            if (_IsAssetIdentifier(metadata)) {
                return std::make_pair(isArray ? SdfValueTypeNames->AssetArray
                                              : SdfValueTypeNames->Asset,
                                      TfToken());
            }

            // We have several special SdrPropertyTypes that we want to map to
            // 'token', which is the type we otherwise reserve for unknown types.
            // We call out this conversion here so it is explicitly documented
            // rather than happening implicitly.
            if (type == SdrPropertyTypes->Terminal ||
                type == SdrPropertyTypes->Struct ||
                type == SdrPropertyTypes->Vstruct) {
                return std::make_pair(isArray ? SdfValueTypeNames->TokenArray
                                              : SdfValueTypeNames->Token, 
                                      type);
            }

            // We prefer more specific types, so if the arraySize is 2, 3, or 4,
            // then try to convert to a fixed-dimension int or float array.
            // In the future if we change this to not return a fixed-size array,
            // all the parsers need to be updated to not return a fixed-size
            // array as well.
            if (type == SdrPropertyTypes->Int) {
                if (arraySize == 2) {
                    return std::make_pair(SdfValueTypeNames->Int2, TfToken());
                } else if (arraySize == 3) {
                    return std::make_pair(SdfValueTypeNames->Int3, TfToken());
                } else if (arraySize == 4) {
                    return std::make_pair(SdfValueTypeNames->Int4, TfToken());
                }
            }
            if (type == SdrPropertyTypes->Float) {
                if (arraySize == 2) {
                    return std::make_pair(SdfValueTypeNames->Float2, TfToken());
                } else if (arraySize == 3) {
                    return std::make_pair(SdfValueTypeNames->Float3, TfToken());
                } else if (arraySize == 4) {
                    return std::make_pair(SdfValueTypeNames->Float4, TfToken());
                }
            }

            return _GetTypeIndicatorFromDefaultMapping(type, isArray);
        }
    }

    enum _UsdEncodingVersions : int {
        _UsdEncodingVersions0 = 0,
        _UsdEncodingVersions1,

        _UsdEncodingVersionsCurrent = _UsdEncodingVersions1
    };

    // -------------------------------------------------------------------------

    // Helper to convert the type to an Sdf type
    const NdrSdfTypeIndicator
    _GetTypeAsSdfType(
        const TfToken& type, size_t arraySize, const NdrTokenMap& metadata,
        int usdEncodingVersion)
    {
        switch (usdEncodingVersion) {
        case _UsdEncodingVersions0:
            return _Encoding_0::GetTypeAsSdfType(type, arraySize, metadata);
        case _UsdEncodingVersions1:
            return _Encoding_1::GetTypeAsSdfType(type, arraySize, metadata);
        default:
            TF_DEBUG(NDR_PARSING).Msg(
                "Invalid/unsupported usdEncodingVersion %d. "
                "Current version is %d.",
                usdEncodingVersion, _UsdEncodingVersionsCurrent);
            return std::make_pair(SdfValueTypeNames->Token, TfToken());
        }
    }

    // -------------------------------------------------------------------------

    // This method converts a given SdrPropertyType to a new SdrPropertyType
    // and appropriate array size if the metadata indicates that such a
    // conversion is necessary.  The conversion is based on the value of the
    // "role" metadata
    std::pair<TfToken, size_t>
    _ConvertSdrPropertyTypeAndArraySize(
        const TfToken& type,
        const size_t& arraySize,
        const NdrTokenMap& metadata)
    {
        TfToken role = GetRoleFromMetadata(metadata);

        if (!type.IsEmpty() && !role.IsEmpty()) {
            // Look up using original type and role declaration
            const TokenToMapTable::const_iterator& typeSearch =
                _GetConvertedSdrTypes().find(type);
            if (typeSearch != _GetConvertedSdrTypes().end()) {
                const TokenToPairTable::const_iterator& roleSearch =
                    typeSearch->second.find(role);
                if (roleSearch != typeSearch->second.end()) {
                    // Return converted type and size
                    return roleSearch->second;
                }
            }
        }

        // No conversion needed or found
        return std::pair<TfToken, size_t>(type, arraySize);
    }

    // -------------------------------------------------------------------------

    template <class T>
    bool
    _GetValue(const VtValue& defaultValue, T* val)
    {
        if (defaultValue.IsHolding<T>()) {
            *val = defaultValue.UncheckedGet<T>();
            return true;
        }
        return false;
    }

    // This function checks if the authored _defaultValue and the sdrType 
    // conform, without actually modifying the _defaultValue, except in the case
    // of VtFloatArray in which case it appropriately returns GfVec2, GfVec3, 
    // GfVec4 values depending on size of the VtFloatArray.
    // Such a mismatch should have been handled appropriately in the parser, and
    // hence provide a debug diagnotics for these.
    VtValue
    _ConformSdrDefaultValue(
            const VtValue &sdrDefaultValue,
            const TfToken &sdrType,
            size_t arraySize,
            const NdrTokenMap &metadata,
            const TfToken &name)
    {
        bool isSdrValueConformed = true;
        bool isArray = _IsArray(arraySize, metadata);
        VtValue defaultValue = sdrDefaultValue;
        
        if (sdrType == SdrPropertyTypes->Int) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<int>();
            } else {
                isSdrValueConformed = sdrDefaultValue.IsHolding<VtArray<int>>();
            }
        }
        else if (sdrType == SdrPropertyTypes->String) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<std::string>();
            } else {
                isSdrValueConformed = 
                    sdrDefaultValue.IsHolding<VtStringArray>();
            }
        }
        else if (sdrType == SdrPropertyTypes->Float) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<float>();
            } else {
                // If the held value is a VtFloatArray We will conform float 
                // array values to GfVec#, else specify isSdrValueConformed as
                // False.
                //
                // If array size is NOT 2,3,4 we specify isSdrValueConformed as
                // False, else its True and explicitly conformed to appropriate
                // GfVec#
                VtFloatArray arrayVal;
                isSdrValueConformed = _GetValue(sdrDefaultValue, &arrayVal);
                if (arrayVal.size() != arraySize) {
                    isSdrValueConformed = false;
                    TF_DEBUG(SDR_TYPE_CONFORMANCE).Msg(
                        "Default value for fixed size float array type does not "
                        "have the right length (%zu vs expected %zu)",
                        arrayVal.size(), arraySize);
                }
                if (isSdrValueConformed) {
                    switch (arraySize) {
                        case 2:
                            defaultValue = VtValue(
                                    GfVec2f(arrayVal[0], 
                                        arrayVal[1]));
                            break;
                        case 3:
                            defaultValue = VtValue(
                                    GfVec3f(arrayVal[0], 
                                        arrayVal[1],
                                        arrayVal[2]));
                            break;
                        case 4:
                            defaultValue = VtValue(
                                    GfVec4f(arrayVal[0], 
                                        arrayVal[1],
                                        arrayVal[2],
                                        arrayVal[3]));
                            break;
                        default:
                            TF_DEBUG(SDR_TYPE_CONFORMANCE).Msg("Invalid "
                                    "arraySize provided. Expects 2/3/4 but %zu "
                                    " provided.", arraySize);
                            isSdrValueConformed = false;
                    }
                }
            }
        }
        else if (sdrType == SdrPropertyTypes->Color ||
                sdrType == SdrPropertyTypes->Point ||
                sdrType == SdrPropertyTypes->Normal ||
                sdrType == SdrPropertyTypes->Vector) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<GfVec3f>();
            } else {
                isSdrValueConformed = sdrDefaultValue.IsHolding<VtArray<GfVec3f>>();
            }
        } else if (sdrType == SdrPropertyTypes->Color4) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<GfVec4f>();
            } else {
                isSdrValueConformed = sdrDefaultValue.IsHolding<VtArray<GfVec4f>>();
            }
        } else if (sdrType == SdrPropertyTypes->Matrix) {
            if (!isArray) {
                isSdrValueConformed = sdrDefaultValue.IsHolding<GfMatrix4d>();
            } else {
                isSdrValueConformed = 
                    sdrDefaultValue.IsHolding<VtArray<GfMatrix4d>>();
            }
        } else {
            // malformed sdrType
            isSdrValueConformed = false;
        }

        if (!isSdrValueConformed) {
            TF_DEBUG(SDR_TYPE_CONFORMANCE).Msg(
                    "Expected type for defaultValue for property: %s is %s, "
                    "but %s was provided.", name.GetText(), sdrType.GetText(), 
                    defaultValue.GetTypeName().c_str());

        }
        return defaultValue;
    }

    // This methods conforms the given default value's type with the property's
    // SdfValueTypeName.  This step is important because a Sdr parser should not
    // care about what SdfValueTypeName the parsed property will eventually map
    // to, and a parser will just return the value it sees with the type that
    // most closely matches the type in the shader file.  Any special type
    // 'transformations' that make use of metadata and other knowledge should
    // happen in this conformance step when the SdrShaderProperty is
    // instantiated.
    VtValue
    _ConformSdfTypeDefaultValue(
        const VtValue& sdrDefaultValue,
        const TfToken& sdrType,
        size_t arraySize,
        const NdrTokenMap& metadata,
        int usdEncodingVersion)
    {
        // Return early if there is no value to conform
        if (sdrDefaultValue.IsEmpty()) {
            return sdrDefaultValue;
        }

        // Return early if no conformance issue
        const NdrSdfTypeIndicator sdfTypeIndicator = _GetTypeAsSdfType(
            sdrType, arraySize, metadata, usdEncodingVersion);
        const SdfValueTypeName sdfType = sdfTypeIndicator.first;

        if (sdrDefaultValue.GetType() == sdfType.GetType()) {
            return sdrDefaultValue;
        }

        // Special conformance for when SdrUsdDefinitionType is provided, we
        // want to set the sdfTypeDefaultValue as the original parsed default 
        // value. This assumes that the shader writer has provided 
        // SdfValueTypeName corresponding default value in the shader since the 
        // shader provides an explicit SdfValueTypeName by specifying a
        // SdrUsdDefinitionType metadata, if not its possible the type and value
        // could mismatch.
        if (metadata.find(SdrPropertyMetadata->SdrUsdDefinitionType) !=
                metadata.end()) {
            // Make sure the types match, or try to extract the correct typed
            // vtvalue from the default
            VtValue sdfTypeValue = VtValue::CastToTypeid(sdrDefaultValue,
                    sdfType.GetType().GetTypeid());
            if (!sdfTypeValue.IsEmpty()) {
                return sdfTypeValue;
            }
        }

        bool isArray = _IsArray(arraySize, metadata);

        // ASSET and ASSET ARRAY
        // ---------------------------------------------------------------------
        if (sdrType == SdrPropertyTypes->String &&
            _IsAssetIdentifier(metadata)) {
            if (isArray) {
                VtStringArray arrayVal;
                _GetValue(sdrDefaultValue, &arrayVal);

                VtArray<SdfAssetPath> array;
                array.reserve(arrayVal.size());

                for (const std::string& val : arrayVal) {
                    array.push_back(SdfAssetPath(val));
                }
                return VtValue::Take(array);
            } else {
                std::string val;
                _GetValue(sdrDefaultValue, &val);
                return VtValue(SdfAssetPath(val));
            }
        }

        // FLOAT ARRAY (FIXED SIZE 2, 3, 4)
        // ---------------------------------------------------------------------
        else if (sdrType == SdrPropertyTypes->Float &&
                 isArray) {
            VtFloatArray arrayVal;
            _GetValue(sdrDefaultValue, &arrayVal);

            if (arrayVal.size() != arraySize) {
                TF_DEBUG(SDR_TYPE_CONFORMANCE).Msg(
                    "Default value for fixed size float array type does not "
                    "have the right length (%zu vs expected %zu)",
                    arrayVal.size(), arraySize);
                return sdrDefaultValue;
            } 

            // We return a fixed-size array for arrays with size 2, 3, or 4
            // because SdrShaderProperty::GetTypeAsSdfType returns a specific
            // size type (Float2, Float3, Float4).  If in the future we want to
            // return a VtFloatArray instead, we need to change the logic in
            // SdrShaderProperty::GetTypeAsSdfType
            if (arraySize == 2) {
                return VtValue(
                        GfVec2f(arrayVal[0], 
                            arrayVal[1]));
            } else if (arraySize == 3) {
                return VtValue(
                        GfVec3f(arrayVal[0],
                            arrayVal[1],
                            arrayVal[2]));
            } else if (arraySize == 4) {
                return VtValue(
                        GfVec4f(arrayVal[0],
                            arrayVal[1],
                            arrayVal[2],
                            arrayVal[3]));
            }
        }

        // Default value's type was not conformant, but no special translation
        // step was found. So we use the default value of the SdfTypeName, which
        // is guaranteed to match
        return sdfType.GetDefaultValue();
    }
}

SdrShaderProperty::SdrShaderProperty(
    const TfToken& name,
    const TfToken& type,
    const VtValue& defaultValue,
    bool isOutput,
    size_t arraySize,
    const NdrTokenMap& metadata,
    const NdrTokenMap& hints,
    const NdrOptionVec& options)
    : NdrProperty(
        name,
        /* type= */ _ConvertSdrPropertyTypeAndArraySize(
            type, arraySize, metadata).first,
        // Note, that the default value might be modified after creation in
        // SdrShaderNode::_PostProcessProperties. Hence we check and conform the
        // default value in _FinalizeProperty.
        defaultValue,
        isOutput,
        /* arraySize= */ _ConvertSdrPropertyTypeAndArraySize(
            type, arraySize, metadata).second,
        /* isDynamicArray= */false,
        metadata),

      _hints(hints),
      _options(options),
      _usdEncodingVersion(_UsdEncodingVersionsCurrent)
{
    _isDynamicArray =
        IsTruthy(SdrPropertyMetadata->IsDynamicArray, _metadata);

    // Note that outputs are always connectable. If "connectable" metadata is
    // found on outputs, ignore it.
    if (isOutput) {
        _isConnectable = true;
    } else {
        _isConnectable = _metadata.count(SdrPropertyMetadata->Connectable)
            ? IsTruthy(SdrPropertyMetadata->Connectable, _metadata)
            : true;
    }

    // Indicate a "default" widget if one was not assigned
    _metadata.insert({SdrPropertyMetadata->Widget, "default"});

    // Tokenize metadata
    _label = TokenVal(SdrPropertyMetadata->Label, _metadata);
    _page = TokenVal(SdrPropertyMetadata->Page, _metadata);
    _widget = TokenVal(SdrPropertyMetadata->Widget, _metadata);
    _vstructMemberOf = TokenVal(
        SdrPropertyMetadata->VstructMemberOf, _metadata);
    _vstructMemberName = TokenVal(
        SdrPropertyMetadata->VstructMemberName, _metadata);
    _vstructConditionalExpr = TokenVal(
        SdrPropertyMetadata->VstructConditionalExpr, _metadata);
    _validConnectionTypes = TokenVecVal(
        SdrPropertyMetadata->ValidConnectionTypes, _metadata);
}

std::string
SdrShaderProperty::GetHelp() const
{
    return StringVal(SdrPropertyMetadata->Help, _metadata);
}

std::string
SdrShaderProperty::GetImplementationName() const
{
    return StringVal(SdrPropertyMetadata->ImplementationName, _metadata,
                     GetName().GetString());
}

bool
SdrShaderProperty::CanConnectTo(const NdrProperty& other) const
{
    NdrPropertyConstPtr input = !_isOutput ? this : &other;
    NdrPropertyConstPtr output = _isOutput ? this : &other;

    // Outputs cannot connect to outputs and vice versa
    if (_isOutput == other.IsOutput()) {
        return false;
    }

    const TfToken & inputType = input->GetType();
    size_t inputArraySize = input->GetArraySize();
    const NdrTokenMap& inputMetadata = input->GetMetadata();

    const TfToken& outputType = output->GetType();
    size_t outputArraySize = output->GetArraySize();
    const NdrTokenMap& outputMetadata = output->GetMetadata();

    // Connections are always possible if the types match exactly and the
    // array size matches
    if ((inputType == outputType) && (inputArraySize == outputArraySize)) {
        return true;
    }

    // Connections are also possible if the types match exactly and the input
    // is a dynamic array
    if ((inputType == outputType) && !output->IsArray()
            && input->IsDynamicArray()) {
        return true;
    }

    // Convert input/output types to Sdf types
    const NdrSdfTypeIndicator& sdfInputTypeInd =
        _GetTypeAsSdfType(inputType, inputArraySize, inputMetadata,
                          _usdEncodingVersion);
    const NdrSdfTypeIndicator& sdfOutputTypeInd =
        _GetTypeAsSdfType(outputType, outputArraySize, outputMetadata,
                          _usdEncodingVersion);
    const SdfValueTypeName& sdfInputType = sdfInputTypeInd.first;
    const SdfValueTypeName& sdfOutputType = sdfOutputTypeInd.first;

    bool inputIsFloat3 =
        (inputType == SdrPropertyTypes->Color)  ||
        (inputType == SdrPropertyTypes->Point)  ||
        (inputType == SdrPropertyTypes->Normal) ||
        (inputType == SdrPropertyTypes->Vector) ||
        (sdfInputType == SdfValueTypeNames->Float3);

    bool outputIsFloat3 =
        (outputType == SdrPropertyTypes->Color)  ||
        (outputType == SdrPropertyTypes->Point)  ||
        (outputType == SdrPropertyTypes->Normal) ||
        (outputType == SdrPropertyTypes->Vector) ||
        (sdfOutputType == SdfValueTypeNames->Float3);

    // Connections between float-3 types are possible
    if (inputIsFloat3 && outputIsFloat3) {
        return true;
    }

    bool inputIsFloat4 =
        (inputType == SdrPropertyTypes->Color4) ||
        (sdfInputType == SdfValueTypeNames->Float4);

    bool outputIsFloat4 =
        (outputType == SdrPropertyTypes->Color4) ||
        (sdfOutputType == SdfValueTypeNames->Float4);

    // Connections between float-4 types are possible
    if (inputIsFloat4 && outputIsFloat4) {
        return true;
    }

    // Special cases
    if ((outputType == SdrPropertyTypes->Vstruct)
        && (inputType == SdrPropertyTypes->Float)) {
        // vstruct -> float is accepted because vstruct seems to be an output
        // only type
        return true;
    }

    return false;
}

bool
SdrShaderProperty::IsVStructMember() const
{
    return _metadata.count(SdrPropertyMetadata->VstructMemberName);
}

bool
SdrShaderProperty::IsVStruct() const
{
    return _type == SdrPropertyTypes->Vstruct;
}

const NdrSdfTypeIndicator
SdrShaderProperty::GetTypeAsSdfType() const
{
    return _GetTypeAsSdfType(_type, _arraySize, _metadata,
                             _usdEncodingVersion);
}

bool
SdrShaderProperty::IsAssetIdentifier() const
{
    return _IsAssetIdentifier(_metadata);
}

bool
SdrShaderProperty::IsDefaultInput() const
{
    return _IsDefaultInput(_metadata);
}

void
SdrShaderProperty::_SetUsdEncodingVersion(int usdEncodingVersion)
{
    _usdEncodingVersion = usdEncodingVersion;
}

void
SdrShaderProperty::_ConvertToVStruct()
{
    _type = SdrPropertyTypes->Vstruct;

    // The default value should match the resulting Sdf type
    NdrSdfTypeIndicator typeIndicator = GetTypeAsSdfType();
    SdfValueTypeName typeName = typeIndicator.first;
    _defaultValue = typeName.GetDefaultValue();
}

void
SdrShaderProperty::_FinalizeProperty()
{
    _sdfTypeDefaultValue = _ConformSdfTypeDefaultValue(_defaultValue, 
            _type, _arraySize, _metadata, _usdEncodingVersion);

    _defaultValue = _ConformSdrDefaultValue(_defaultValue, _type, _arraySize, 
            _metadata, _name);
}

PXR_NAMESPACE_CLOSE_SCOPE
