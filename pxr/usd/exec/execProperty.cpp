//
// unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/exec/debugCodes.h"
#include "pxr/usd/exec/execMetadataHelpers.h"
#include "pxr/usd/exec/execProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    EXEC_DEFAULT_VALUE_AS_SDF_DEFAULT_VALUE, true,
    "This is set to true, until all the internal codesites using "
    "GetDefaultValue() are updated to use GetDefaultValueAsSdfType(). As "
    "previous implementation for type conformance code would update "
    "_defaultValue, for backward compatibility we need to set _defaultValue to "
    "_sdfTypeDefaultValue. Following needs to be removed or set to false once "
    "appropriate GetDefaultValue() codesite changes are made. ");

TF_DEFINE_PUBLIC_TOKENS(ExecPropertyTypes, EXEC_PROPERTY_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(ExecPropertyMetadata, EXEC_PROPERTY_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(ExecPropertyRole, EXEC_PROPERTY_ROLE_TOKENS);

using ExecNodeMetadataHelpers::GetRoleFromMetadata;
using ExecNodeMetadataHelpers::IsTruthy;
using ExecNodeMetadataHelpers::StringVal;
using ExecNodeMetadataHelpers::TokenVal;
using ExecNodeMetadataHelpers::TokenVecVal;

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor>
        TokenToSdfTypeMap;

namespace {
    // This only establishes EXACT mappings. If a mapping is not included here,
    // a one-to-one mapping isn't possible.
    const TokenToSdfTypeMap& _GetTokenTypeToSdfType()
    {
        static const TokenToSdfTypeMap tokenTypeToSdfType  = {
            {ExecPropertyTypes->Bool,       SdfValueTypeNames->Bool},
            {ExecPropertyTypes->Int,        SdfValueTypeNames->Int},
            {ExecPropertyTypes->String,     SdfValueTypeNames->String},
            {ExecPropertyTypes->Float,      SdfValueTypeNames->Float},
            {ExecPropertyTypes->Color,      SdfValueTypeNames->Float3},
            {ExecPropertyTypes->Vector2,    SdfValueTypeNames->Float2},
            {ExecPropertyTypes->Vector3,    SdfValueTypeNames->Float3},
            {ExecPropertyTypes->Vector4,    SdfValueTypeNames->Float4},
            {ExecPropertyTypes->Quaternion, SdfValueTypeNames->Float4},
            {ExecPropertyTypes->Rotation,   SdfValueTypeNames->Float4},
            {ExecPropertyTypes->Matrix3,    SdfValueTypeNames->Matrix3d},
            {ExecPropertyTypes->Matrix4,    SdfValueTypeNames->Matrix4d}
        };
        return tokenTypeToSdfType;
    }

    // The array equivalent of the above map
    const TokenToSdfTypeMap& _GetTokenTypeToSdfArrayType()
    { 
        static const TokenToSdfTypeMap tokenTypeToSdfArrayType  = {
            {ExecPropertyTypes->Bool,       SdfValueTypeNames->BoolArray},
            {ExecPropertyTypes->Int,        SdfValueTypeNames->IntArray},
            {ExecPropertyTypes->String,     SdfValueTypeNames->StringArray},
            {ExecPropertyTypes->Float,      SdfValueTypeNames->FloatArray},
            {ExecPropertyTypes->Color,      SdfValueTypeNames->Float3Array},
            {ExecPropertyTypes->Vector2,    SdfValueTypeNames->Float2Array},
            {ExecPropertyTypes->Vector3,    SdfValueTypeNames->Float3Array},
            {ExecPropertyTypes->Vector4,    SdfValueTypeNames->Float4Array},
            {ExecPropertyTypes->Quaternion, SdfValueTypeNames->Float4Array},
            {ExecPropertyTypes->Rotation,   SdfValueTypeNames->Float4Array},
            {ExecPropertyTypes->Matrix3,    SdfValueTypeNames->Matrix3dArray},
            {ExecPropertyTypes->Matrix4,    SdfValueTypeNames->Matrix4dArray}
        };
        return tokenTypeToSdfArrayType;
    }

    // Map of SdfValueTypeName's aliases to corresponding SdfValueTypeName
    // Refer SdfValueTypeName::GetAliasesAsTokens.
    // This is used to determine SdfValueTypeName from the ExecUsdDefinitionType
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

    SdfValueTypeName
    _GetExecUsdDefinitionType(const NdrTokenMap &metadata)
    {
        const TfToken &execUsdDefinitionType = 
            TfToken(StringVal(
                        ExecPropertyMetadata->ExecUsdDefinitionType, metadata));

        if (execUsdDefinitionType.IsEmpty()) {
            return SdfValueTypeName();
        }

        const TokenToSdfTypeMap &aliasesTokensToSdfValueTypeNames =
            _GetAliasesTokensToSdfValueTypeNames();

        if (aliasesTokensToSdfValueTypeNames.find(execUsdDefinitionType) == 
                aliasesTokensToSdfValueTypeNames.end()) {
            TF_WARN("Invalid SdfValueTypeName or alias provided for "
                    "execUsdDefinitionType metadata: %s", 
                    execUsdDefinitionType.GetText());
            return SdfValueTypeName();
        }

        return aliasesTokensToSdfValueTypeNames.at(execUsdDefinitionType);
    }

    // Returns true if the arraySize or the metadata indicate that the property
    // has an array type
    bool
    _IsArray(size_t arraySize, const NdrTokenMap &metadata)
    {
        bool isDynamicArray =
            IsTruthy(ExecPropertyMetadata->IsDynamicArray, metadata);
        return arraySize > 0 || isDynamicArray;
    }

    // Determines if the metadata contains a key identifying the property as an
    // asset identifier
    bool
    _IsAssetIdentifier(const NdrTokenMap& metadata)
    {
        return metadata.count(ExecPropertyMetadata->IsAssetIdentifier);
    }

    // Returns true is this property is a default input on the node
    bool
    _IsDefaultInput(const NdrTokenMap &metadata)
    {
        return metadata.count(ExecPropertyMetadata->DefaultInput);
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

    // Helper to convert the type to an Sdf type
    const NdrSdfTypeIndicator
    _GetTypeAsSdfType(
        const TfToken& type, size_t arraySize, const NdrTokenMap& metadata)
    {
        bool isArray = _IsArray(arraySize, metadata);
        return _GetTypeIndicatorFromDefaultMapping(type, isArray);
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


    // This methods conforms the given default value's type with the property's
    // SdfValueTypeName.  This step is important because a Exec parser should not
    // care about what SdfValueTypeName the parsed property will eventually map
    // to, and a parser will just return the value it sees with the type that
    // most closely matches the type in the file.  Any special type
    // 'transformations' that make use of metadata and other knowledge should
    // happen in this conformance step when the ExecProperty is
    // instantiated.
    VtValue
    _ConformSdfTypeDefaultValue(
        const VtValue& execDefaultValue,
        const TfToken& execType,
        size_t arraySize,
        const NdrTokenMap& metadata)
    {
        // Return early if there is no value to conform
        if (execDefaultValue.IsEmpty()) {
            return execDefaultValue;
        }

        // Return early if no conformance issue
        const NdrSdfTypeIndicator sdfTypeIndicator = _GetTypeAsSdfType(
            execType, arraySize, metadata);
        const SdfValueTypeName sdfType = sdfTypeIndicator.first;

        if (execDefaultValue.GetType() == sdfType.GetType()) {
            return execDefaultValue;
        }

        // Special conformance for when ExecUsdDefinitionType is provided, we
        // want to set the sdfTypeDefaultValue as the original parsed default 
        // value. This assumes that the node writer has provided 
        // SdfValueTypeName corresponding default value in the node since the 
        // node provides an explicit SdfValueTypeName by specifying a
        // ExecUsdDefinitionType metadata, if not its possible the type and value
        // could mismatch.
        if (metadata.find(ExecPropertyMetadata->ExecUsdDefinitionType) !=
                metadata.end()) {
            // Make sure the types match, or try to extract the correct typed
            // vtvalue from the default
            VtValue sdfTypeValue = VtValue::CastToTypeid(execDefaultValue,
                    sdfType.GetType().GetTypeid());
            if (!sdfTypeValue.IsEmpty()) {
                return sdfTypeValue;
            }
        }

        bool isArray = _IsArray(arraySize, metadata);

        // ASSET and ASSET ARRAY
        // ---------------------------------------------------------------------
        if (execType == ExecPropertyTypes->String &&
            _IsAssetIdentifier(metadata)) {
            if (isArray) {
                VtStringArray arrayVal;
                _GetValue(execDefaultValue, &arrayVal);

                VtArray<SdfAssetPath> array;
                array.reserve(arrayVal.size());

                for (const std::string& val : arrayVal) {
                    array.push_back(SdfAssetPath(val));
                }
                return VtValue::Take(array);
            } else {
                std::string val;
                _GetValue(execDefaultValue, &val);
                return VtValue(SdfAssetPath(val));
            }
        }

        // FLOAT ARRAY (FIXED SIZE 2, 3, 4)
        // ---------------------------------------------------------------------
        else if (execType == ExecPropertyTypes->Float &&
                 isArray) {
            VtFloatArray arrayVal;
            _GetValue(execDefaultValue, &arrayVal);

            if (arrayVal.size() != arraySize) {
                TF_DEBUG(EXEC_TYPE_CONFORMANCE).Msg(
                    "Default value for fixed size float array type does not "
                    "have the right length (%zu vs expected %zu)",
                    arrayVal.size(), arraySize);
                return execDefaultValue;
            } 

            // We return a fixed-size array for arrays with size 2, 3, or 4
            // because ExecProperty::GetTypeAsSdfType returns a specific
            // size type (Float2, Float3, Float4).  If in the future we want to
            // return a VtFloatArray instead, we need to change the logic in
            // ExecProperty::GetTypeAsSdfType
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

ExecProperty::ExecProperty(
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
        type,
        // Note, that the default value might be modified after creation in
        // ExecNode::_PostProcessProperties. Hence we check and conform the
        // default value in _FinalizeProperty.
        defaultValue,
        isOutput,
        arraySize,
        /* isDynamicArray= */false,
        metadata),

      _hints(hints),
      _options(options)
{
    _isDynamicArray =
        IsTruthy(ExecPropertyMetadata->IsDynamicArray, _metadata);

    // Note that outputs are always connectable. If "connectable" metadata is
    // found on outputs, ignore it.
    if (isOutput) {
        _isConnectable = true;
    } else {
        _isConnectable = _metadata.count(ExecPropertyMetadata->Connectable)
            ? IsTruthy(ExecPropertyMetadata->Connectable, _metadata)
            : true;
    }

    // Indicate a "default" widget if one was not assigned
    _metadata.insert({ExecPropertyMetadata->Widget, "default"});

    // Tokenize metadata
    _label = TokenVal(ExecPropertyMetadata->Label, _metadata);
    _page = TokenVal(ExecPropertyMetadata->Page, _metadata);
    _widget = TokenVal(ExecPropertyMetadata->Widget, _metadata);
}

std::string
ExecProperty::GetHelp() const
{
    return StringVal(ExecPropertyMetadata->Help, _metadata);
}

std::string
ExecProperty::GetImplementationName() const
{
    return StringVal(ExecPropertyMetadata->ImplementationName, _metadata,
                     GetName().GetString());
}

bool
ExecProperty::CanConnectTo(const NdrProperty& other) const
{
    return true;
    /*
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
        _GetTypeAsSdfType(inputType, inputArraySize, inputMetadata);
    const NdrSdfTypeIndicator& sdfOutputTypeInd =
        _GetTypeAsSdfType(outputType, outputArraySize, outputMetadata);
    const SdfValueTypeName& sdfInputType = sdfInputTypeInd.first;
    const SdfValueTypeName& sdfOutputType = sdfOutputTypeInd.first;

    bool inputIsFloat3 =
        (inputType == ExecPropertyTypes->Color)  ||
        (inputType == ExecPropertyTypes->Point)  ||
        (inputType == ExecPropertyTypes->Normal) ||
        (inputType == ExecPropertyTypes->Vector) ||
        (sdfInputType == SdfValueTypeNames->Float3);

    bool outputIsFloat3 =
        (outputType == ExecPropertyTypes->Color)  ||
        (outputType == ExecPropertyTypes->Point)  ||
        (outputType == ExecPropertyTypes->Normal) ||
        (outputType == ExecPropertyTypes->Vector) ||
        (sdfOutputType == SdfValueTypeNames->Float3);

    // Connections between float-3 types are possible
    if (inputIsFloat3 && outputIsFloat3) {
        return true;
    }

    return false;
    */
}

const NdrSdfTypeIndicator
ExecProperty::GetTypeAsSdfType() const
{
    return _GetTypeAsSdfType(_type, _arraySize, _metadata);
}

bool
ExecProperty::IsAssetIdentifier() const
{
    return _IsAssetIdentifier(_metadata);
}

bool
ExecProperty::IsDefaultInput() const
{
    return _IsDefaultInput(_metadata);
}


void
ExecProperty::_FinalizeProperty()
{
    // XXX: Note that until all the codesites using GetDefaultValue() are
    // updated, we need to set the _defaultValue to _sdfTypeDefaultValue.
    // Following needs to be removed once appropriate GetDefaultValue() 
    // codesite changes are made. (This is for backward compatibility)
    if (TfGetEnvSetting(EXEC_DEFAULT_VALUE_AS_SDF_DEFAULT_VALUE)) {
        _defaultValue = _sdfTypeDefaultValue;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
