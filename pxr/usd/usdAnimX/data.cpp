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
    
    UsdAnimXData* data = new UsdAnimXData(params);

    // The pseudo-root spec must always exist in a layer's SdfData, so
    // add it here.
    data->CreateSpec(SdfPath::AbsoluteRootPath(), SdfSpecTypePseudoRoot);

    return TfCreateRefPtr(data);
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
    return false;
}

bool 
UsdAnimXData::IsEmpty() const
{
    return !_impl || _impl->IsEmpty();
}

bool
UsdAnimXData::HasSpec(const SdfPath& path) const
{
    return _data.find(path) != _data.end();
}

void
UsdAnimXData::EraseSpec(const SdfPath& path)
{
    _HashTable::iterator i = _data.find(path);
    if (!TF_VERIFY(i != _data.end(),
                   "No spec to erase at <%s>", path.GetText())) {
        return;
    }
    _data.erase(i);
}

void
UsdAnimXData::MoveSpec(const SdfPath& oldPath, 
                                      const SdfPath& newPath)
{
    _HashTable::iterator old = _data.find(oldPath);
    if (!TF_VERIFY(old != _data.end(),
            "No spec to move at <%s>", oldPath.GetString().c_str())) {
        return;
    }
    bool inserted = _data.insert(std::make_pair(newPath, old->second)).second;
    if (!TF_VERIFY(inserted)) {
        return;
    }
    _data.erase(old);
}

SdfSpecType
UsdAnimXData::GetSpecType(const SdfPath& path) const
{
    _HashTable::const_iterator i = _data.find(path);
    if (i == _data.end()) {
        return SdfSpecTypeUnknown;
    }
    return i->second.specType;
}

void
UsdAnimXData::CreateSpec(const SdfPath& path,
                                        SdfSpecType specType)
{
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
        return;
    }
    _data[path].specType = specType;
}

void
UsdAnimXData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    TF_FOR_ALL(it, _data) {
        if (!visitor->VisitSpec(*this, it->first)) {
            break;
        }
    }
}

void 
UsdAnimXData::Initialize(const std::string& filename)
{
  _impl->InitFromFile(filename);
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                  const TfToken &field,
                  SdfAbstractDataValue* value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field)) {
        if (value) {
            return value->StoreValue(*fieldValue);
        }
        return true;
    }
    return false;
}

bool 
UsdAnimXData::Has(const SdfPath& path, 
                  const TfToken & field, 
                  VtValue *value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(path, field)) {
        if (value) {
            *value = *fieldValue;
        }
        return true;
    }
    return false;
}

/*
bool
UsdAnimXData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        return !value || value->StoreValue(*v);
    }
    return false;
}

bool
UsdAnimXData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    VtValue *value, SdfSpecType *specType) const
{
    if (VtValue const *v =
        _GetSpecTypeAndFieldValue(path, fieldName, specType)) {
        if (value) {
            *value = *v;
        }
        return true;
    }
    return false;
}
*/
 
VtValue
UsdAnimXData::Get(const SdfPath& path, 
                                 const TfToken & field) const
{
    if (const VtValue *value = _GetFieldValue(path, field)) {
        return *value;
    }
    return VtValue();
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, const VtValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    if (value.IsEmpty()) {
        Erase(path, field);
        return;
    }

    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue) {
        *newValue = value;
    }
}

void 
UsdAnimXData::Set(const SdfPath& path, 
                                 const TfToken & field, 
                                 const SdfAbstractDataConstValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    VtValue* newValue = _GetOrCreateFieldValue(path, field);
    if (newValue) {
        value.GetValue(newValue);
    }
}

void 
UsdAnimXData::Erase(const SdfPath& path, 
                                   const TfToken & field)
{
    _HashTable::iterator i = _data.find(path);
    if (i == _data.end()) {
        return;
    }
    
    _SpecData &spec = i->second;
    for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
        if (spec.fields[j].first == field) {
            spec.fields.erase(spec.fields.begin()+j);
            return;
        }
    }
}

std::vector<TfToken>
UsdAnimXData::List(const SdfPath& path) const
{
    _HashTable::const_iterator i = _data.find(path);
    if (i != _data.end()) {
        const _SpecData & spec = i->second;

        std::vector<TfToken> names;
        names.reserve(spec.fields.size());
        for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
            names.push_back(spec.fields[j].first);
        }
        return names;
    }

    return std::vector<TfToken>();
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
    if (value.IsEmpty()) {
        EraseTimeSample(path, time);
        return;
    }

    SdfTimeSampleMap newSamples;

    // Attempt to get a pointer to an existing timeSamples field.
    VtValue *fieldValue =
        _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);

    // If we have one, swap it out so we can modify it.
    if (fieldValue && fieldValue->IsHolding<SdfTimeSampleMap>()) {
        fieldValue->UncheckedSwap(newSamples);
    }
    
    // Insert or overwrite into newSamples.
    newSamples[time] = value;

    // Set back into the field.
    if (fieldValue) {
        fieldValue->Swap(newSamples);
    } else {
        Set(path, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
    }
    //TF_RUNTIME_ERROR("UsdAnimX file SetTimeSample() not supported");
}

void
UsdAnimXData::EraseTimeSample(const SdfPath& path, double time)
{
    TF_RUNTIME_ERROR("UsdAnimX file EraseTimeSample() not supported");
}

bool
UsdAnimXData::Write(
    const SdfAbstractDataConstPtr& data,
    const std::string& filePath,
    const std::string& comment)
{
  
   return true;
}

const VtValue*
UsdAnimXData::_GetSpecTypeAndFieldValue(const SdfPath& path,
                                   const TfToken& field,
                                   SdfSpecType* specType) const
{
    _HashTable::const_iterator i = _data.find(path);
    if (i == _data.end()) {
        *specType = SdfSpecTypeUnknown;
    }
    else {
        const _SpecData &spec = i->second;
        *specType = spec.specType;
        for (auto const &f: spec.fields) {
            if (f.first == field) {
                return &f.second;
            }
        }
    }
    return nullptr;
}

const VtValue* 
UsdAnimXData::_GetFieldValue(const SdfPath &path,
                        const TfToken &field) const
{
    _HashTable::const_iterator i = _data.find(path);
    if (i != _data.end()) {
        const _SpecData & spec = i->second;
        for (auto const &f: spec.fields) {
            if (f.first == field) {
                return &f.second;
            }
        }
    }
    return nullptr;
}

VtValue*
UsdAnimXData::_GetMutableFieldValue(const SdfPath &path,
                               const TfToken &field)
{
    _HashTable::iterator i = _data.find(path);
    if (i != _data.end()) {
        _SpecData &spec = i->second;
        for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
            if (spec.fields[j].first == field) {
                return &spec.fields[j].second;
            }
        }
    }
    return NULL;
}

VtValue* 
UsdAnimXData::_GetOrCreateFieldValue(const SdfPath &path,
                                const TfToken &field)
{
    _HashTable::iterator i = _data.find(path);
    if (!TF_VERIFY(i != _data.end(),
                   "No spec at <%s> when trying to set field '%s'",
                   path.GetText(), field.GetText())) {
        return nullptr;
    }

    _SpecData &spec = i->second;
    for (auto &f: spec.fields) {
        if (f.first == field) {
            return &f.second;
        }
    }

    spec.fields.emplace_back(std::piecewise_construct,
                             std::forward_as_tuple(field),
                             std::forward_as_tuple());

    return &spec.fields.back().second;
}

PXR_NAMESPACE_CLOSE_SCOPE
