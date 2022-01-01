//
// Copyright 2019 Pixar
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

#include "pxr/pxr.h"
#include "data.h"
#include "dataImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    UsdAnimXDataParamsTokens, 
    USD_ANIMX_DATA_PARAMS_TOKENS);

////////////////////////////////////////////////////////////////////////
// UsdAnimXDataParams

namespace {

// Sets an arbitrary param type value from a string arg.
template <class T>
static void
_SetParamFromArg(T *param, const std::string &arg)
{
    *param = TfUnstringify<T>(arg);
}

// Specialization for TfToken which doesn't have an istream method for 
// TfUnstringify.
template <>
void
_SetParamFromArg<TfToken>(TfToken *param, const std::string &arg)
{
    *param = TfToken(arg);
}

// Helper for setting a parameter value from a VtValue, casting if the value type
// is not an exact match.
template <class T>
static void
_SetParamFromValue(T *param, const VtValue &dictVal)
{
    if (dictVal.IsHolding<T>()) {
        *param = dictVal.UncheckedGet<T>();
    } else if (dictVal.CanCast<T>()) {
        VtValue castVal = VtValue::Cast<T>(dictVal);
        *param = castVal.UncheckedGet<T>();
    }
}

};

/*static*/
UsdAnimXDataParams 
UsdAnimXDataParams::FromArgs(
    const SdfFileFormat::FileFormatArguments & args)
{
    UsdAnimXDataParams params;

    // For each param in the struct, try to find an arg with the same name 
    // and convert its string value to a new value for the param. Falls back 
    // to leaving the param as its default value if the arg isn't there.
    #define xx(UNUSED_1, NAME, UNUSED_2) \
        if (const std::string *argValue = TfMapLookupPtr( \
                args, UsdAnimXDataParamsTokens->NAME)) { \
            _SetParamFromArg(&params.NAME, *argValue); \
        }
    USD_ANIMX_DATA_PARAMS_X_FIELDS   
    #undef xx

    return params;
}

/*static*/
UsdAnimXDataParams 
UsdAnimXDataParams::FromDict(const VtDictionary& dict)
{
    UsdAnimXDataParams params;

    // Same as FromArgs, but values are extracted from a VtDictionary.
    #define xx(UNUSED_1, NAME, UNUSED_2) \
        if (const VtValue *dictVal = TfMapLookupPtr( \
                dict, UsdAnimXDataParamsTokens->NAME)) { \
            _SetParamFromValue(&params.NAME, *dictVal); \
        }
    USD_ANIMX_DATA_PARAMS_X_FIELDS
    #undef xx
    return params;
}

SdfFileFormat::FileFormatArguments 
UsdAnimXDataParams::ToArgs() const
{
    SdfFileFormat::FileFormatArguments args;

    // Convert each param in the struct to string argument with the same name.
    #define xx(UNUSED_1, NAME, UNUSED_2) \
        args[UsdAnimXDataParamsTokens->NAME] = TfStringify(NAME);
    USD_ANIMX_DATA_PARAMS_X_FIELDS
    #undef xx
    return args;
}

////////////////////////////////////////////////////////////////////////
// UsdAnimXData

/*static*/
UsdAnimXDataRefPtr 
UsdAnimXData::New(const UsdAnimXDataParams &params)
{
    return TfCreateRefPtr(new UsdAnimXData(params));
}

UsdAnimXData::UsdAnimXData(
    const UsdAnimXDataParams &params) :
    _impl(new UsdAnimXDataImpl(params))
{
}

UsdAnimXData::~UsdAnimXData()
{
}

bool
UsdAnimXData::StreamsData() const
{
    // We say this data object streams data because the implementation generates
    // most of its queries on demand.
    return true;
}

bool 
UsdAnimXData::IsEmpty() const
{
    return !_impl || _impl->IsEmpty();
}

bool
UsdAnimXData::HasSpec(const SdfPath& path) const
{
    return GetSpecType(path) != SdfSpecTypeUnknown;
}

void
UsdAnimXData::EraseSpec(const SdfPath& path)
{
    TF_RUNTIME_ERROR("UsdAnimX file EraseSpec() not supported");
}

void
UsdAnimXData::MoveSpec(const SdfPath& oldPath, 
                                      const SdfPath& newPath)
{
    TF_RUNTIME_ERROR("UsdAnimX file MoveSpec() not supported");
}

SdfSpecType
UsdAnimXData::GetSpecType(const SdfPath& path) const
{
    return _impl->GetSpecType(path);
}

void
UsdAnimXData::CreateSpec(const SdfPath& path,
                                        SdfSpecType specType)
{
    TF_RUNTIME_ERROR("UsdAnimX file CreateSpec() not supported");
}

void
UsdAnimXData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    _impl->VisitSpecs(*this, visitor);
}

void 
UsdAnimXData::Initialize()
{
  _impl->InitFromFile("fuck");
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                                 const TfToken &field,
                                 SdfAbstractDataValue* value) const
{
    if (value) {
        VtValue val;
        if (_impl->Has(path, field, &val)) {
            return value->StoreValue(val);
        }
        return false;
    } else {
        return _impl->Has(path, field, nullptr);
    }
    return false;
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                                 const TfToken & field, 
                                 VtValue *value) const
{
    return _impl->Has(path, field, value);
}
 
VtValue
UsdAnimXData::Get(const SdfPath& path, 
                                 const TfToken & field) const
{
    VtValue value;
    _impl->Has(path, field, &value);
    return value;
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, const VtValue& value)
{
    TF_RUNTIME_ERROR("UsdAnimX file Set() not supported");
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, 
                                 const SdfAbstractDataConstValue& value)
{
    TF_RUNTIME_ERROR("UsdAnimX file Set() not supported");
}

void 
UsdAnimXData::Erase(const SdfPath& path, 
                                   const TfToken & field)
{
    TF_RUNTIME_ERROR("UsdAnimX file Erase() not supported");
}

std::vector<TfToken>
UsdAnimXData::List(const SdfPath& path) const
{
    return _impl->List(path);
}

std::set<double>
UsdAnimXData::ListAllTimeSamples() const
{
    return _impl->ListAllTimeSamples();
}

std::set<double>
UsdAnimXData::ListTimeSamplesForPath(const SdfPath& path) const
{
    return _impl->ListTimeSamplesForPath(path);
}

bool
UsdAnimXData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamples(time, tLower, tUpper);
}

size_t
UsdAnimXData::GetNumTimeSamplesForPath(
    const SdfPath& path) const
{
    return _impl->GetNumTimeSamplesForPath(path);
}

bool
UsdAnimXData::GetBracketingTimeSamplesForPath(
    const SdfPath& path, double time,
    double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamplesForPath(path, time, tLower, tUpper);
}

bool
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, VtValue *value) const
{
    return _impl->QueryTimeSample(path, time, value);
}

bool 
UsdAnimXData::QueryTimeSample(const SdfPath& path, 
                                             double time, 
                                             SdfAbstractDataValue* value) const
{ 
    if (value) {
        VtValue val;
        if (_impl->QueryTimeSample(path, time, &val)) {
            return value->StoreValue(val);
        }
        return false;
    } else {
        return _impl->QueryTimeSample(path, time, nullptr);
    }
}

void
UsdAnimXData::SetTimeSample(const SdfPath& path, 
                                           double time, const VtValue& value)
{
    TF_RUNTIME_ERROR("UsdAnimX file SetTimeSample() not supported");
}

void
UsdAnimXData::EraseTimeSample(const SdfPath& path, double time)
{
    TF_RUNTIME_ERROR("UsdAnimX file EraseTimeSample() not supported");
}

PXR_NAMESPACE_CLOSE_SCOPE
