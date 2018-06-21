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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdrPropertyTypes, SDR_PROPERTY_TYPE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyMetadata, SDR_PROPERTY_METADATA_TOKENS);

using ShaderMetadataHelpers::IsTruthy;
using ShaderMetadataHelpers::StringVal;
using ShaderMetadataHelpers::StringVecVal;
using ShaderMetadataHelpers::TokenVal;
using ShaderMetadataHelpers::TokenVecVal;

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor>
        TokenToSdfTypeMap;

namespace {
    // This only establishes EXACT mappings. If a mapping is not included here,
    // a one-to-one mapping isn't possible.
    const TokenToSdfTypeMap& GetTokenTypeToSdfType()
    {
        static const TokenToSdfTypeMap tokenTypeToSdfType  = {
            {SdrPropertyTypes->Int,     SdfValueTypeNames->Int},
            {SdrPropertyTypes->String,  SdfValueTypeNames->String},
            {SdrPropertyTypes->Float,   SdfValueTypeNames->Float},
            {SdrPropertyTypes->Color,   SdfValueTypeNames->Color3f},
            {SdrPropertyTypes->Point,   SdfValueTypeNames->Point3f},
            {SdrPropertyTypes->Normal,  SdfValueTypeNames->Normal3f},
            {SdrPropertyTypes->Vector,  SdfValueTypeNames->Vector3f},
            {SdrPropertyTypes->Matrix,  SdfValueTypeNames->Matrix4d}
        };
        return tokenTypeToSdfType;
    }

    // The array equivalent of the above map
    const TokenToSdfTypeMap& GetTokenTypeToSdfArrayType()
    { 
        static const TokenToSdfTypeMap tokenTypeToSdfArrayType  = {
            {SdrPropertyTypes->Int,     SdfValueTypeNames->IntArray},
            {SdrPropertyTypes->String,  SdfValueTypeNames->StringArray},
            {SdrPropertyTypes->Float,   SdfValueTypeNames->FloatArray},
            {SdrPropertyTypes->Color,   SdfValueTypeNames->Color3fArray},
            {SdrPropertyTypes->Point,   SdfValueTypeNames->Point3fArray},
            {SdrPropertyTypes->Normal,  SdfValueTypeNames->Normal3fArray},
            {SdrPropertyTypes->Vector,  SdfValueTypeNames->Vector3fArray},
            {SdrPropertyTypes->Matrix,  SdfValueTypeNames->Matrix4dArray}
        };
        return tokenTypeToSdfArrayType;
    }


    // -------------------------------------------------------------------------


    // Determines if the metadata contains a key identifying the property as an
    // asset identifier
    bool _IsAssetIdentifier(const NdrTokenMap& metadata)
    {
        return metadata.count(SdrPropertyMetadata->IsAssetIdentifier);
    }


    // -------------------------------------------------------------------------


    // Helper to convert array types to Sdf types. Shouldn't be used directly;
    // use `_GetTypeAsSdfType()` instead
    const SdfTypeIndicator _GetTypeAsSdfArrayType(
        const TfToken& type, size_t arraySize)
    {
        SdfValueTypeName convertedType = SdfValueTypeNames->Token;
        bool conversionSuccessful = false;

        // First try to convert to a fixed-dimension float array
        if (type == SdrPropertyTypes->Float) {
            if (arraySize == 2) {
                convertedType = SdfValueTypeNames->Float2;
                conversionSuccessful = true;
            } else if (arraySize == 3) {
                convertedType = SdfValueTypeNames->Float3;
                conversionSuccessful = true;
            } else if (arraySize == 4) {
                convertedType = SdfValueTypeNames->Float4;
                conversionSuccessful = true;
            }
        }

        // If no float array conversion was done, try converting to an array
        // type without a fixed dimension
        if (!conversionSuccessful) {
            const TokenToSdfTypeMap& tokenTypeToSdfArrayType = 
                GetTokenTypeToSdfArrayType();
            TokenToSdfTypeMap::const_iterator it =
                tokenTypeToSdfArrayType.find(type);

            if (it != tokenTypeToSdfArrayType.end()) {
                convertedType = it->second;
                conversionSuccessful = true;
            }
        }

        return std::make_pair(
            convertedType,
            conversionSuccessful ? TfToken() : type
        );
    }


    // -------------------------------------------------------------------------


    // Helper to convert the type to an Sdf type (this will call
    // `_GetTypeAsSdfArrayType()` if an array type is detected)
    const SdfTypeIndicator _GetTypeAsSdfType(
        const TfToken& type, size_t arraySize, const NdrTokenMap& metadata)
    {
        // There is one Sdf type (Asset) that is not included in the type
        // mapping because it is determined dynamically
        if (_IsAssetIdentifier(metadata)) {
            return std::make_pair(SdfValueTypeNames->Asset, TfToken());
        }

        bool conversionSuccessful = false;

        // If the conversion can't be made, it defaults to the 'Token' type
        SdfValueTypeName convertedType = SdfValueTypeNames->Token;

        if (arraySize > 0) {
            return _GetTypeAsSdfArrayType(type, arraySize);
        }

        const TokenToSdfTypeMap& tokenTypeToSdfType = GetTokenTypeToSdfType();
        TokenToSdfTypeMap::const_iterator it = tokenTypeToSdfType.find(type);
        if (it != tokenTypeToSdfType.end()) {
            conversionSuccessful = true;
            convertedType = it->second;
        }

        return std::make_pair(
            convertedType,
            conversionSuccessful ? TfToken() : type
        );
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
    : NdrProperty(name, type, defaultValue, isOutput, arraySize,
               /* isDynamicArray= */false, metadata),

      _hints(hints),
      _options(options)
{
    _isDynamicArray =
        IsTruthy(SdrPropertyMetadata->IsDynamicArray, _metadata);

    _isConnectable = _metadata.count(SdrPropertyMetadata->Connectable)
        ? IsTruthy(SdrPropertyMetadata->Connectable, _metadata)
        : true;

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
    _validConnectionTypes = TokenVecVal(
        SdrPropertyMetadata->ValidConnectionTypes, _metadata);
}

const std::string&
SdrShaderProperty::GetHelp() const
{
    return StringVal(SdrPropertyMetadata->Help, _metadata);
}

const std::string&
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

    // Connections are always possible if the types match exactly
    if ((inputType == outputType) && (inputArraySize == outputArraySize)) {
        return true;
    }

    // Convert input/output types to Sdf types
    const SdfTypeIndicator& sdfInputTypeInd =
        _GetTypeAsSdfType(inputType, inputArraySize, inputMetadata);
    const SdfTypeIndicator& sdfOutputTypeInd =
        _GetTypeAsSdfType(outputType, outputArraySize, outputMetadata);
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

const SdfTypeIndicator
SdrShaderProperty::GetTypeAsSdfType() const
{
    return _GetTypeAsSdfType(_type, _arraySize, _metadata);
}

bool
SdrShaderProperty::IsAssetIdentifier() const
{
    return _IsAssetIdentifier(_metadata);
}

PXR_NAMESPACE_CLOSE_SCOPE
