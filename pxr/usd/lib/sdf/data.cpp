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
#include "pxr/usd/sdf/data.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/work/utils.h"
#include <iostream>

SdfData::~SdfData()
{
    // Clear out _data in parallel, since it can get big.
    WorkSwapDestroyAsync(_data);
}

bool
SdfData::HasSpec(const SdfAbstractDataSpecId& id) const
{
    return (_data.find(id.GetFullSpecPath()) != _data.end());
}

void
SdfData::EraseSpec(const SdfAbstractDataSpecId& id)
{
    _HashTable::iterator i = _data.find(id.GetFullSpecPath());
    if (!TF_VERIFY(i != _data.end(), 
            "No spec to erase at <%s>", id.GetString().c_str())) {
        return;
    }
    _data.erase(i);
}

void
SdfData::MoveSpec(const SdfAbstractDataSpecId& oldId, 
                  const SdfAbstractDataSpecId& newId)
{
    const SdfPath oldPath = oldId.GetFullSpecPath();
    const SdfPath newPath = newId.GetFullSpecPath();

    _HashTable::iterator old = _data.find(oldPath);
    if (!TF_VERIFY(old != _data.end(),
            "No spec to move at <%s>", oldPath.GetString().c_str())) {
        return;
    }
    bool inserted = _data.insert(std::make_pair(newPath,old->second)).second;
    if (!TF_VERIFY(inserted)) {
        return;
    }
    _data.erase(old);
}

SdfSpecType
SdfData::GetSpecType(const SdfAbstractDataSpecId& id) const
{
    _HashTable::const_iterator i = _data.find(id.GetFullSpecPath());
    if (i == _data.end()) {
        return SdfSpecTypeUnknown;
    }
    return i->second.specType;
}

void
SdfData::CreateSpec(const SdfAbstractDataSpecId& id, SdfSpecType specType)
{
    if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
        return;
    }
    _data[id.GetFullSpecPath()].specType = specType;
}

void
SdfData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    TF_FOR_ALL(it, _data) {
        if (!visitor->VisitSpec(*this, SdfAbstractDataSpecId(&it->first))) {
            break;
        }
    }
}

bool 
SdfData::Has(const SdfAbstractDataSpecId& id, const TfToken &field,
             SdfAbstractDataValue* value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(id, field)) {
        if (value) {
            return value->StoreValue(*fieldValue);
        }
        return true;
    }
    return false;
}

bool 
SdfData::Has(const SdfAbstractDataSpecId& id, const TfToken & field, 
             VtValue *value) const
{
    if (const VtValue* fieldValue = _GetFieldValue(id, field)) {
        if (value) {
            *value = *fieldValue;
        }
        return true;
    }
    return false;
}

const VtValue* 
SdfData::_GetFieldValue(const SdfAbstractDataSpecId& id,
                        const TfToken& field) const
{
    _HashTable::const_iterator i = _data.find(id.GetFullSpecPath());
    if (i != _data.end()) {
        const _SpecData & spec = i->second;
        for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
            if (spec.fields[j].first == field) {
                return &spec.fields[j].second;
            }
        }
    }
    return NULL;
}
 
VtValue
SdfData::Get(const SdfAbstractDataSpecId& id, const TfToken & field) const
{
    if (const VtValue *value = _GetFieldValue(id, field))
        return *value;
    return VtValue();
}

void 
SdfData::Set(const SdfAbstractDataSpecId& id, const TfToken & field, 
             const VtValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    if (value.IsEmpty()) {
        Erase(id, field);
        return;
    }

    VtValue* newValue = _GetOrCreateFieldValue(id, field);
    if (newValue) {
        *newValue = value;
    }
}

void 
SdfData::Set(const SdfAbstractDataSpecId& id, const TfToken & field, 
            const SdfAbstractDataConstValue& value)
{
    TfAutoMallocTag2 tag("Sdf", "SdfData::Set");

    VtValue* newValue = _GetOrCreateFieldValue(id, field);
    if (newValue) {
        value.GetValue(newValue);
    }
}

VtValue* 
SdfData::_GetOrCreateFieldValue(const SdfAbstractDataSpecId& id,
                                const TfToken& field)
{
    _HashTable::iterator i = _data.find(id.GetFullSpecPath());
    if (!TF_VERIFY(i != _data.end(),
                      "No spec at <%s> when trying to set field '%s'",
                      id.GetString().c_str(), field.GetText())) {
        return NULL;
    }

    _SpecData & spec = i->second;
    for (size_t j=0, jEnd = spec.fields.size(); j != jEnd; ++j) {
        if (spec.fields[j].first == field) {
            return &spec.fields[j].second;
        }
    }

    spec.fields.push_back( _FieldValuePair(field, VtValue()) );
    return &spec.fields.back().second;
}

void 
SdfData::Erase(const SdfAbstractDataSpecId& id, const TfToken & field)
{
    _HashTable::iterator i = _data.find(id.GetFullSpecPath());
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
SdfData::List(const SdfAbstractDataSpecId& id) const
{
    _HashTable::const_iterator i = _data.find(id.GetFullSpecPath());
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


////////////////////////////////////////////////////////////////////////
// This is a basic prototype implementation of the time-sampling API
// for in-memory, non cached presto layers.

std::set<double>
SdfData::ListAllTimeSamples() const
{
    // Use a set to determine unique times.
    std::set<double> times;

    TF_FOR_ALL(i, _data) {
        std::set<double> timesForPath = 
            ListTimeSamplesForPath(SdfAbstractDataSpecId(&i->first));
        times.insert(timesForPath.begin(), timesForPath.end());
    }

    return times;
}

std::set<double>
SdfData::ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    std::set<double> times;
    
    VtValue value = Get(id, SdfDataTokens->TimeSamples);
    if (value.IsHolding<SdfTimeSampleMap>()) {
        const SdfTimeSampleMap & timeSampleMap =
            value.UncheckedGet<SdfTimeSampleMap>();
        TF_FOR_ALL(j, timeSampleMap) {
            times.insert(j->first);
        }
    }

    return times;
}

template <class Container, class GetTime>
static bool
_GetBracketingTimeSamplesImpl(
    const Container &samples, const GetTime &getTime,
    const double time, double* tLower, double* tUpper)
{
    if (samples.empty()) {
        // No samples.
        return false;
    } else if (time <= getTime(*samples.begin())) {
        // Time is at-or-before the first sample.
        *tLower = *tUpper = getTime(*samples.begin());
    } else if (time >= getTime(*samples.rbegin())) {
        // Time is at-or-after the last sample.
        *tLower = *tUpper = getTime(*samples.rbegin());
    } else {
        auto iter = samples.lower_bound(time);
        if (getTime(*iter) == time) {
            // Time is exactly on a sample.
            *tLower = *tUpper = getTime(*iter);
        } else {
            // Time is in-between samples; return the bracketing times.
            *tUpper = getTime(*iter);
            --iter;
            *tLower = getTime(*iter);
        }
    }
    return true;
}

static bool
_GetBracketingTimeSamples(const std::set<double> &samples, double time,
                          double *tLower, double *tUpper)
{
    return _GetBracketingTimeSamplesImpl(samples, [](double t) { return t; },
                                         time, tLower, tUpper);
}

static bool
_GetBracketingTimeSamples(const SdfTimeSampleMap &samples, double time,
                          double *tLower, double *tUpper)
{
    return _GetBracketingTimeSamplesImpl(
        samples, [](SdfTimeSampleMap::value_type const &p) { return p.first; },
        time, tLower, tUpper);
}

bool
SdfData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    return _GetBracketingTimeSamples(
        ListAllTimeSamples(), time, tLower, tUpper);
}

size_t
SdfData::GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    if (const VtValue *fval = _GetFieldValue(id, SdfDataTokens->TimeSamples)) {
        if (fval->IsHolding<SdfTimeSampleMap>()) {
            return fval->UncheckedGet<SdfTimeSampleMap>().size();
        }
    }
    return 0;
}

bool
SdfData::GetBracketingTimeSamplesForPath(
    const SdfAbstractDataSpecId& id, double time,
    double* tLower, double* tUpper) const
{
    const VtValue *fval = _GetFieldValue(id, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        return _GetBracketingTimeSamples(tsmap, time, tLower, tUpper);
    }
    return false;
}

bool
SdfData::QueryTimeSample(const SdfAbstractDataSpecId& id, double time, 
                         VtValue *value) const
{
    const VtValue *fval = _GetFieldValue(id, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end()) {
            if (value)
                *value = iter->second;
            return true;
        }
    }
    return false;
}

bool 
SdfData::QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                         SdfAbstractDataValue* value) const
{ 
    const VtValue *fval = _GetFieldValue(id, SdfDataTokens->TimeSamples);
    if (fval && fval->IsHolding<SdfTimeSampleMap>()) {
        auto const &tsmap = fval->UncheckedGet<SdfTimeSampleMap>();
        auto iter = tsmap.find(time);
        if (iter != tsmap.end()) {
            return !value || value->StoreValue(iter->second);
        }
    }
    return false;
}

void
SdfData::SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                       const VtValue& value)
{
    if (value.IsEmpty()) {
        EraseTimeSample(id, time);
        return;
    }

    SdfTimeSampleMap samples =
        GetAs<SdfTimeSampleMap>(id, SdfDataTokens->TimeSamples);

    samples[time] = value;

    Set(id, SdfDataTokens->TimeSamples, VtValue(samples));
}

void
SdfData::EraseTimeSample(const SdfAbstractDataSpecId& id, double time)
{
    SdfTimeSampleMap samples =
        GetAs<SdfTimeSampleMap>(id, SdfDataTokens->TimeSamples);

    samples.erase(time);
    if (samples.empty())
        Erase(id, SdfDataTokens->TimeSamples);
    else
        Set(id, SdfDataTokens->TimeSamples, VtValue(samples));
}

