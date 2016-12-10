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
#include "pxr/usd/usd/crateData.h"

#include "crateFile.h"

#include "pxr/base/tf/bitUtils.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/typeInfoMap.h"
#include "pxr/base/tracelite/trace.h"

#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/sdf/schema.h"

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

#include <boost/container/flat_map.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using namespace Usd_CrateFile;

static inline bool
_GetBracketingTimes(const vector<double> &times,
                    const double time, double* tLower, double* tUpper)
{
    if (times.empty()) {
        // No samples.
        return false;
    } else if (time <= times.front()) {
        // Time is at-or-before the first sample.
        *tLower = *tUpper = times.front();
    } else if (time >= times.back()) {
        // Time is at-or-after the last sample.
        *tLower = *tUpper = times.back();
    } else {
        auto i = lower_bound(times.begin(), times.end(), time);
        if (*i == time) {
            // Time is exactly on a sample.
            *tLower = *tUpper = *i;
        } else {
            // Time is in-between samples; return the bracketing times.
            *tUpper = *i;
            --i;
            *tLower = *i;
        }
    }
    return true;
}

class Usd_CrateDataImpl
{
    friend class Usd_CrateData;

public:

    Usd_CrateDataImpl() 
        : _flatLastSet(nullptr)
        , _hashLastSet(nullptr)
        , _crateFile(CrateFile::CreateNew()) {}
    
    ~Usd_CrateDataImpl() {
        // Tear down asynchronously.
        WorkMoveDestroyAsync(_crateFile);
        WorkMoveDestroyAsync(_flatTypes);
        WorkMoveDestroyAsync(_flatData);
        if (_hashData)
            WorkMoveDestroyAsync(_hashData);
    }

    string const &GetFileName() const { return _crateFile->GetFileName(); }

    bool Save(string const &fileName) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Save");
        
        auto dataFileName = _crateFile->GetFileName();
        if (not TF_VERIFY(fileName == dataFileName or dataFileName.empty()))
            return false;

        if (CrateFile::Packer packer = _crateFile->StartPacking(fileName)) {
            if (_hashData) {
                for (auto const &p: *_hashData) {
                    packer.PackSpec(
                        p.first, p.second.specType, p.second.fields.Get());
                }
            } else {
                for (size_t i = 0; i != _flatData.size(); ++i) {
                    auto const &p = _flatData.begin()[i];
                    packer.PackSpec(
                        p.first, _flatTypes[i].type, p.second.fields.Get());
                }
            }
            if (packer.Close()) {
                return _PopulateFromCrateFile();
            }
        }
        
        return false;
    }

    bool Open(string const &fileName) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Open");

        if (auto newData = CrateFile::Open(fileName)) {
            _crateFile = std::move(newData);
            return _PopulateFromCrateFile();
        }
        return false;
    }

    inline bool _HasTargetSpec(SdfPath const &path) const {
        // We don't store target specs to save space, since in Usd we don't have
        // any fields that may be set on them.  Their presence is determined by
        // whether or not they appear in their owning relationship's Added or
        // Explicit items.
        SdfPath parentPath = path.GetParentPath();
        if (parentPath.IsPrimPropertyPath()) {
            VtValue targetPaths;
            if (Has(SdfAbstractDataSpecId(&parentPath),
                    SdfFieldKeys->TargetPaths, &targetPaths) and
                targetPaths.IsHolding<SdfPathListOp>()) {
                auto const &listOp = targetPaths.UncheckedGet<SdfPathListOp>();
                if (listOp.IsExplicit()) {
                    auto const &items = listOp.GetExplicitItems();
                    return std::find(
                        items.begin(), items.end(), path) != items.end();
                } else {
                    auto const &items = listOp.GetAddedItems();
                    return std::find(
                        items.begin(), items.end(), path) != items.end();
                }
            }
        }
        return false;
    }

    inline bool HasSpec(const SdfAbstractDataSpecId &id) const {
        const SdfPath &path = id.GetFullSpecPath();
        if (ARCH_UNLIKELY(path.IsTargetPath())) {
            return _HasTargetSpec(path);
        }
        return _hashData ?
            _hashData->find(path) != _hashData->end() :
            _flatData.find(path) != _flatData.end();
    }

    inline void EraseSpec(const SdfAbstractDataSpecId &id) {
        if (ARCH_UNLIKELY(id.GetFullSpecPath().IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }
        if (_MaybeMoveToHashTable()) {
            _hashLastSet = nullptr;
            TF_VERIFY(_hashData->erase(id.GetFullSpecPath()),
                      id.GetString().c_str());
        } else {
            auto iter = _flatData.find(id.GetFullSpecPath());
            size_t index = iter - _flatData.begin();
            if (TF_VERIFY(iter != _flatData.end(), id.GetString().c_str())) {
                _flatLastSet = nullptr;
                _flatData.erase(iter);
                _flatTypes.erase(_flatTypes.begin() + index);
            }
        }
    }

    inline void MoveSpec(const SdfAbstractDataSpecId& oldId,
                         const SdfAbstractDataSpecId& newId) {
        if (ARCH_UNLIKELY(oldId.GetFullSpecPath().IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }

        SdfPath const &oldPath = oldId.GetFullSpecPath();
        SdfPath const &newPath = newId.GetFullSpecPath();

        if (_MaybeMoveToHashTable()) {
            auto oldIter = _hashData->find(oldPath);
            if (not TF_VERIFY(oldIter != _hashData->end()))
                return;
            _hashLastSet = nullptr;
            bool inserted = _hashData->emplace(newPath, oldIter->second).second;
            if (not TF_VERIFY(inserted))
                return;
            _hashData->erase(oldIter);
        } else {
            auto oldIter = _flatData.find(oldPath);
            if (not TF_VERIFY(oldIter != _flatData.end()))
                return;
            
            _flatLastSet = nullptr;

            const size_t index = oldIter - _flatData.begin();
            auto tmpFields(std::move(oldIter->second));
            auto tmpType = _flatTypes[index];
            
            _flatData.erase(oldIter);
            _flatTypes.erase(_flatTypes.begin() + index);
            
            auto iresult = _flatData.emplace(newPath, std::move(tmpFields));
            const size_t newIndex = iresult.first - _flatData.begin();
            _flatTypes.insert(_flatTypes.begin() + newIndex, tmpType);
            
            TF_VERIFY(iresult.second);
        }
    }

    inline SdfSpecType GetSpecType(const SdfAbstractDataSpecId &id) const {
        SdfPath const &path = id.GetFullSpecPath();
        if (path == SdfPath::AbsoluteRootPath()) {
            return SdfSpecTypePseudoRoot;
        }
        if (path.IsTargetPath()) {
            return _HasTargetSpec(path) ?
                SdfSpecTypeRelationshipTarget : SdfSpecTypeUnknown;
        }
        if (_hashData) {
            auto i = _hashData->find(path);
            return i == _hashData->end() ?
                SdfSpecTypeUnknown : i->second.specType;
        }
        auto i = _flatData.find(path);
        if (i == _flatData.end())
            return SdfSpecTypeUnknown;
        // Don't look up in the table if we can tell the type from the path.
        return path.IsPrimPath() ? SdfSpecTypePrim :
            _flatTypes[i - _flatData.begin()].type;
    }

    inline void
    CreateSpec(const SdfAbstractDataSpecId &id, SdfSpecType specType) {
        if (not TF_VERIFY(specType != SdfSpecTypeUnknown))
            return;
        if (id.GetFullSpecPath().IsTargetPath()) {
            // Do nothing, we do not store relationship target specs in usd.
            return;
        }
        if (_MaybeMoveToHashTable()) {
            // No need to blow the _hashLastSet cache here, since inserting into
            // the table won't invalidate existing references.
            (*_hashData)[id.GetFullSpecPath()].specType = specType;
        } else {
            _flatLastSet = nullptr;
            auto iresult = 
                _flatData.emplace(id.GetFullSpecPath(), _FlatSpecData());
            auto index = iresult.first - _flatData.begin();
            if (iresult.second) {
                _flatTypes.insert(
                    _flatTypes.begin() + index, _SpecType(specType));
            } else {
                TF_VERIFY(_flatTypes[index].type == specType);
            }
        }
    }

    inline void _VisitSpecs(SdfAbstractData const &data,
                            SdfAbstractDataSpecVisitor* visitor) const {
        // XXX: Is it important to present relationship target specs here?
        if (_hashData) {
            for (auto const &p: *_hashData) {
                if (not visitor->VisitSpec(
                        data, SdfAbstractDataSpecId(&p.first))) {
                    break;
                }
            }
        } else {
            for (auto const &p: _flatData) {
                if (not visitor->VisitSpec(
                        data, SdfAbstractDataSpecId(&p.first))) {
                    break;
                }
            }
        }
    }

    inline bool Has(const SdfAbstractDataSpecId& id,
                    const TfToken & field,
                    SdfAbstractDataValue* value) const {
        if (VtValue const *fieldValue = _GetFieldValue(id, field)) {
            if (value) {
                VtValue val = _DetachValue(*fieldValue);
                if (field == SdfDataTokens->TimeSamples) {
                    // Special case, convert internal TimeSamples to
                    // SdfTimeSampleMap.
                    val = _MakeTimeSampleMap(val);
                }
                return value->StoreValue(val);
            }
            return true;
        }
        return false;
    }

    inline bool Has(const SdfAbstractDataSpecId& id,
                    const TfToken & field,
                    VtValue *value) const {
        if (VtValue const *fieldValue = _GetFieldValue(id, field)) {
            if (value) {
                *value = _DetachValue(*fieldValue);
                if (field == SdfDataTokens->TimeSamples) {
                    // Special case, convert internal TimeSamples to
                    // SdfTimeSampleMap.
                    *value = _MakeTimeSampleMap(*value);
                }
            }
            return true;
        }
        return false;
    }

    inline VtValue Get(const SdfAbstractDataSpecId& id,
                       const TfToken & field) const {
        VtValue result;
        Has(id, field, &result);
        return result;
    }

    template <class Data>
    inline void _ListHelper(Data const &d, SdfAbstractDataSpecId const &id,
                            vector<TfToken> &out) const {
        auto i = d.find(id.GetFullSpecPath());
        if (i != d.end()) {
            auto const &fields = i->second.fields.Get();
            out.resize(fields.size());
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                out[j] = fields[j].first;
            }
        }
    }

    inline vector<TfToken> List(const SdfAbstractDataSpecId& id) const {
        vector<TfToken> names;
        _hashData ?
            _ListHelper(*_hashData, id, names) :
            _ListHelper(_flatData, id, names);
        return names;
    }

    template <class Data>
    inline void _SetHelper(
        Data &d, SdfAbstractDataSpecId const &id,
        typename Data::value_type *&lastSet,
        TfToken const &fieldName, VtValue const &value) {

        SdfPath const &path = id.GetFullSpecPath();

        if (not lastSet or lastSet->first != path) {
            auto i = d.find(id.GetFullSpecPath());
            if (not TF_VERIFY(
                    i != d.end(),
                    "Tried to set field '%s' on nonexistent spec at <%s>",
                    id.GetString().c_str(), fieldName.GetText())) {
                return;
            }
            lastSet = &(*i);
        }
        
        VtValue const *valPtr = &value;
        VtValue timeSamples;
        if (fieldName == SdfDataTokens->TimeSamples) {
            timeSamples = _Make_TimeSamples(value);
            valPtr = &timeSamples;
        }
        
        auto &spec = lastSet->second;
        spec.DetachIfNotUnique();
        auto &fields = spec.fields.GetMutable();
        for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
            if (fields[j].first == fieldName) {
                // Found existing field entry.
                fields[j].second = *valPtr;
                return;
            }
        }
        
        // No existing field entry.
        fields.emplace_back(fieldName, *valPtr);
    }

    inline void Set(const SdfAbstractDataSpecId& id,
                    const TfToken& fieldName,
                    const VtValue& value) {
        if (ARCH_UNLIKELY(value.IsEmpty())) {
            Erase(id, fieldName);
            return;
        }
        if (id.GetFullSpecPath().IsTargetPath()) {
            TF_CODING_ERROR("Cannot set fields on relationship target specs: "
                            "<%s>:%s = %s", id.GetFullSpecPath().GetText(),
                            fieldName.GetText(), TfStringify(value).c_str());
            return;
        }
        _hashData ?
            _SetHelper(*_hashData, id, _hashLastSet, fieldName, value) :
            _SetHelper(_flatData, id, _flatLastSet, fieldName, value);
    }

    inline void Set(const SdfAbstractDataSpecId& id,
                    const TfToken& field,
                    const SdfAbstractDataConstValue& value) {
        VtValue val;
        TF_AXIOM(value.GetValue(&val));
        return Set(id, field, val);
    }

    template <class Data>
    inline void _EraseHelper(
        Data &d, const SdfAbstractDataSpecId& id, const TfToken & field) {
        auto i = d.find(id.GetFullSpecPath());
        if (i == d.end())
            return;

        auto &spec = i->second;
        auto const &fields = spec.fields.Get();
        for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
            if (fields[j].first == field) {
                // Detach if not unique, and remove the j'th element.
                spec.DetachIfNotUnique();
                auto &mutableFields = spec.fields.GetMutable();
                mutableFields.erase(mutableFields.begin()+j);
                return;
            }
        }
    }

    inline void Erase(const SdfAbstractDataSpecId& id, const TfToken & field) {
        _hashData ?
            _EraseHelper(*_hashData, id, field) :
            _EraseHelper(_flatData, id, field);
    }

    inline std::set<double>
    ListAllTimeSamples() const {
        auto times = _ListAllTimeSamples();
        return std::set<double>(times.begin(), times.end());
    }

    inline std::set<double>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const {
        auto const &times = _ListTimeSamplesForPath(id);
        return std::set<double>(times.begin(), times.end());
    }

    inline bool GetBracketingTimeSamples(
        double time, double* tLower, double* tUpper) const {
        return _GetBracketingTimes(_ListAllTimeSamples(), time, tLower, tUpper);
    }

    inline size_t
    GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const {
        return _ListTimeSamplesForPath(id).size();
    }

    inline bool GetBracketingTimeSamplesForPath(
        const SdfAbstractDataSpecId& id,
        double time, double* tLower, double* tUpper) const {
        return _GetBracketingTimes(
            _ListTimeSamplesForPath(id), time, tLower, tUpper);
    }

    inline bool QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                                VtValue *value) const {
        if (VtValue const *fieldValue =
            _GetFieldValue(id, SdfDataTokens->TimeSamples)) {
            if (fieldValue->IsHolding<TimeSamples>()) {
                auto const &ts = fieldValue->UncheckedGet<TimeSamples>();
                auto const &times = ts.times.Get();
                auto iter = lower_bound(times.begin(), times.end(), time);
                if (iter == times.end() or *iter != time)
                    return false;
                if (value) {
                    auto index = iter - times.begin();
                    *value = _DetachValue(
                        _crateFile->GetTimeSampleValue(ts, index));
                }
                return true;
            }
        }
        return false;
    }

    inline bool QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                                SdfAbstractDataValue* value) const {
        if (not value)
            return QueryTimeSample(id, time, static_cast<VtValue *>(nullptr));
        VtValue vtVal;
        return QueryTimeSample(id, time, &vtVal) and value->StoreValue(vtVal);
    }

    inline void
    SetTimeSample(const SdfAbstractDataSpecId& id, double time,
                  const VtValue & value) {
        if (value.IsEmpty()) {
            EraseTimeSample(id, time);
            return;
        }

        TimeSamples newSamples;
        
        VtValue *fieldValue =
            _GetMutableFieldValue(id, SdfDataTokens->TimeSamples);
        
        if (fieldValue and fieldValue->IsHolding<TimeSamples>()) {
            fieldValue->UncheckedSwap(newSamples);
        }
        
        // Insert or overwrite time into newTimes.
        auto iter = lower_bound(newSamples.times.Get().begin(),
                                newSamples.times.Get().end(), time);
        if (iter == newSamples.times.Get().end() or *iter != time) {
            auto index = iter - newSamples.times.Get().begin();
            // Make the samples mutable, which may invalidate 'iter'.
            _crateFile->MakeTimeSampleTimesAndValuesMutable(newSamples);
            newSamples.times.GetMutable().
                insert(newSamples.times.GetMutable().begin() + index, time);
            newSamples.values.insert(newSamples.values.begin() + index, value);
        } else {
            // Make the values mutable, then modify.
            _crateFile->MakeTimeSampleValuesMutable(newSamples);
            newSamples.values[iter-newSamples.times.Get().begin()] = value;
        }
        
        if (fieldValue) {
            fieldValue->UncheckedSwap(newSamples);
        } else {
            Set(id, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
        }
    }
    
    inline void EraseTimeSample(const SdfAbstractDataSpecId& id, double time) {
        TimeSamples newSamples;
        
        VtValue *fieldValue =
            _GetMutableFieldValue(id, SdfDataTokens->TimeSamples);
        
        if (fieldValue and fieldValue->IsHolding<TimeSamples>()) {
            fieldValue->UncheckedSwap(newSamples);
        } else {
            return;
        }
        
        // Insert or overwrite time into newTimes.
        auto iter = lower_bound(newSamples.times.Get().begin(),
                                newSamples.times.Get().end(), time);
        if (iter == newSamples.times.Get().end() or *iter != time)
            return;
        
        auto index = iter-newSamples.times.Get().begin();
        // Make the samples mutable, which may invalidate 'iter'.
        _crateFile->MakeTimeSampleTimesAndValuesMutable(newSamples);
        newSamples.times.GetMutable().erase(
            newSamples.times.GetMutable().begin() + index);
        newSamples.values.erase(newSamples.values.begin() + index);
        
        fieldValue->UncheckedSwap(newSamples);
    }

    ////////////////////////////////////////////////////////////////////////
private:

    bool _PopulateFromCrateFile() {

        // Ensure we start from a clean slate.
        _ClearSpecData();

        WorkArenaDispatcher dispatcher;

        // Pull all the data out of the crate file structure that we'll consume.
        vector<CrateFile::Spec> specs;
        vector<CrateFile::Field> fields;
        vector<Usd_CrateFile::FieldIndex> fieldSets;
        _crateFile->RemoveStructuralData(specs, fields, fieldSets);
        
        // Remove any target specs, we do not store target specs in Usd, but old
        // files could contain them.
        specs.erase(
            remove_if(
                specs.begin(), specs.end(),
                [this](CrateFile::Spec const &spec) {
                    return _crateFile->GetPath(spec.pathIndex).IsTargetPath();
                }),
            specs.end());

        // Sort by path fast-less-than, need same order that _Table will
        // store.
        dispatcher.Run([this, &specs]() {
                tbb::parallel_sort(
                    specs.begin(), specs.end(),
                    [this](CrateFile::Spec const &l, CrateFile::Spec const &r) {
                        SdfPath::FastLessThan flt;
                        return flt(_crateFile->GetPath(l.pathIndex),
                                   _crateFile->GetPath(r.pathIndex));
                    });
            });
        dispatcher.Wait();
        
        // This function object just turns a CrateFile::Spec into the spec data
        // type that we want to store in _flatData.  It has to be a function
        // object instead of a lambda because boost::transform_iterator requires
        // the function object be copy/assignable.
        struct _SpecToPair {
            using result_type = _FlatMap::value_type;
            explicit _SpecToPair(CrateFile *crateFile) : crateFile(crateFile) {}
            result_type operator()(CrateFile::Spec const &spec) const {
                result_type r(crateFile->GetPath(spec.pathIndex),
                              _FlatSpecData(Usd_EmptySharedTag));
                TF_AXIOM(!r.first.IsTargetPath());
                return r;
            }
            CrateFile *crateFile;
        };

        {
            TfAutoMallocTag tag2("Usd_CrateDataImpl main hash table");
            _SpecToPair s2p(_crateFile.get());
            decltype(_flatData)(
                boost::container::ordered_unique_range,
                boost::make_transform_iterator(specs.begin(), s2p),
                boost::make_transform_iterator(specs.end(), s2p)).swap(
                    _flatData);
        }

        // Allocate all the spec data structures in the hashtable first, so we
        // can populate fields in parallel without locking.
        vector<_FlatSpecData *> specDataPtrs;

        // Create all the specData entries and store pointers to them.
        dispatcher.Run([this, &specs, &specDataPtrs]() {
                // XXX Won't need first two tags when bug #132031 is addressed
                TfAutoMallocTag2 tag("Usd", "Usd_CrateDataImpl::Open");
                TfAutoMallocTag tag2("Usd_CrateDataImpl main hash table");
                specDataPtrs.resize(specs.size());
                for (size_t i = 0; i != specs.size(); ++i) {
                    specDataPtrs[i] = &(_flatData.begin()[i].second);
                }
            });

        // Create the specType array.
        dispatcher.Run([this, &specs]() {
                // XXX Won't need first two tags when bug #132031 is addressed
                TfAutoMallocTag2 tag("Usd", "Usd_CrateDataImpl::Open");
                TfAutoMallocTag  tag2("Usd_CrateDataImpl main hash table");
                _flatTypes.resize(specs.size());
            });

        typedef Usd_Shared<_FieldValuePairVector> SharedFieldValuePairVector;

        unordered_map<
            FieldSetIndex, SharedFieldValuePairVector, _Hasher> liveFieldSets;

        for (auto fsBegin = fieldSets.begin(),
                 fsEnd = find(fsBegin, fieldSets.end(), FieldIndex());
             fsBegin != fieldSets.end();
             fsBegin = fsEnd + 1,
                 fsEnd = find(fsBegin, fieldSets.end(), FieldIndex())) {

            // Add this range to liveFieldSets.
            TfAutoMallocTag tag2("field data");
            auto &fieldValuePairs =
                liveFieldSets[FieldSetIndex(fsBegin-fieldSets.begin())];

            dispatcher.Run(
                [this, fsBegin, fsEnd, &fields, &fieldValuePairs]() mutable {
                    // XXX Won't need first two tags when bug #132031 is
                    // addressed
                    TfAutoMallocTag2 tag("Usd", "Usd_CrateDataImpl::Open");
                    TfAutoMallocTag tag2("field data");
                    auto &pairs = fieldValuePairs.GetMutable();
                    pairs.resize(fsEnd-fsBegin);
                    for (size_t i = 0; fsBegin != fsEnd; ++fsBegin, ++i) {
                        auto const &field = fields[fsBegin->value];
                        pairs[i].first = _crateFile->GetToken(field.tokenIndex);
                        pairs[i].second = _UnpackForField(field.valueRep);
                    }
                });
        }

        dispatcher.Wait();

        dispatcher.Run(
            [this, &specs, &specDataPtrs, &liveFieldSets]() {
                tbb::parallel_for(
                    static_cast<size_t>(0), static_cast<size_t>(specs.size()),
                    [this, &specs, &specDataPtrs, &liveFieldSets]
                    (size_t specIdx) {
                        auto const &s = specs[specIdx];
                        auto *specData = specDataPtrs[specIdx];
                        _flatTypes[specIdx].type = s.specType;
                        specData->fields =
                            liveFieldSets.find(s.fieldSetIndex)->second;
                    });
            });

        dispatcher.Wait();

        return true;
    }

    inline VtValue _UnpackForField(ValueRep rep) const {
        VtValue ret;
        if (rep.IsInlined() or rep.GetType() == TypeEnum::TimeSamples) {
            ret = _crateFile->UnpackValue(rep);
        } else {
            ret = rep;
        }
        return ret;
    }

    inline std::vector<double> const &
    _ListTimeSamplesForPath(const SdfAbstractDataSpecId &id) const {
        if (const VtValue* fieldValue =
            _GetFieldValue(id, SdfDataTokens->TimeSamples)) {
            if (fieldValue->IsHolding<TimeSamples>()) {
                return fieldValue->UncheckedGet<TimeSamples>().times.Get();
            }
        }
        static std::vector<double> empty;
        return empty;
    }

    template <class Data>
    inline vector<double> _ListAllTimeSamplesHelper(Data const &d) const {
        vector<double> allTimes, tmp; 
        for (auto const &p: d) {
            tmp.swap(allTimes);
            allTimes.clear();
            auto const &times =
                _ListTimeSamplesForPath(SdfAbstractDataSpecId(&p.first));
            set_union(tmp.begin(), tmp.end(), times.begin(), times.end(),
                      back_inserter(allTimes));
        }
        return allTimes;
    }

    inline vector<double> _ListAllTimeSamples() const {
        return _hashData ?
            _ListAllTimeSamplesHelper(*_hashData) :
            _ListAllTimeSamplesHelper(_flatData);
    }

    inline VtValue _MakeTimeSampleMap(VtValue const &val) const {
        if (val.IsHolding<TimeSamples>()) {
            SdfTimeSampleMap result;
            auto const &ts = val.UncheckedGet<TimeSamples>();
            for (size_t i = 0; i != ts.times.Get().size(); ++i) {
                result.emplace(
                    ts.times.Get()[i],
                    _DetachValue(_crateFile->GetTimeSampleValue(ts, i)));
            }
            return VtValue::Take(result);
        }
        return val;
    }

    inline VtValue _Make_TimeSamples(VtValue const &val) const {
        if (val.IsHolding<SdfTimeSampleMap>()) {
            TimeSamples result;
            auto const &tsm = val.UncheckedGet<SdfTimeSampleMap>();
            result.times.GetMutable().reserve(tsm.size());
            result.values.reserve(tsm.size());
            for (auto const &p: tsm) {
                result.times.GetMutable().push_back(p.first);
                result.values.push_back(p.second);
            }
            return VtValue::Take(result);
        }
        return val;
    }

    template <class Data>
    inline VtValue const *
    _GetFieldValueHelper(Data const &d,
                         SdfAbstractDataSpecId const &id,
                         TfToken const &field) const {
        auto i = d.find(id.GetFullSpecPath());
        if (i != d.end()) {
            auto const &fields = i->second.fields.Get();
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                if (fields[j].first == field) {
                    return &fields[j].second;
                }
            }
        }
        return nullptr;
    }        
        
    inline VtValue const *
    _GetFieldValue(SdfAbstractDataSpecId const &id,
                   TfToken const &field) const {
        return _hashData ?
            _GetFieldValueHelper(*_hashData, id, field) :
            _GetFieldValueHelper(_flatData, id, field);
    }

    template <class Data>
    inline VtValue *
    _GetMutableFieldValueHelper(Data &d,
                                SdfAbstractDataSpecId const &id,
                                TfToken const &field) {
        auto i = d.find(id.GetFullSpecPath());
        if (i != d.end()) {
            auto &spec = i->second;
            auto const &fields = spec.fields.Get();
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                if (fields[j].first == field) {
                    spec.DetachIfNotUnique();
                    return &spec.fields.GetMutable()[j].second;
                }
            }
        }
        return nullptr;
    }

    inline VtValue *
    _GetMutableFieldValue(const SdfAbstractDataSpecId& id,
                          const TfToken& field) {
        return _hashData ?
            _GetMutableFieldValueHelper(*_hashData, id, field) :
            _GetMutableFieldValueHelper(_flatData, id, field);
    }

    inline VtValue _DetachValue(VtValue const &val) const {
        return val.IsHolding<ValueRep>() ?
            _crateFile->UnpackValue(val.UncheckedGet<ValueRep>()) : val;
    }

    inline void _ClearSpecData() {
        _hashData.reset();
        TfReset(_flatData);
        TfReset(_flatTypes);
        _flatLastSet = nullptr;
        _hashLastSet = nullptr;
    }

    bool _MaybeMoveToHashTable() {
        // Arbitrary size threshold for flat_map data.
        constexpr size_t FlatDataThreshold = 1024;
        if (not _hashData and _flatData.size() > FlatDataThreshold) {
            // blow lastSet caches.
            _flatLastSet = nullptr;
            _hashLastSet = nullptr;

            // move to hash table.
            _hashData.reset(new decltype(_hashData)::element_type);
            auto &d = *_hashData;
            auto flatBeginIter = _flatData.begin();
            for (size_t i = 0; i != _flatData.size(); ++i) {
                auto const &p = flatBeginIter[i];
                _MapSpecData msd {
                    std::move(p.second.fields), _flatTypes[i].type };
                d.emplace(p.first, std::move(msd));
            }
            TfReset(_flatData);
            TfReset(_flatTypes);
        }
        return static_cast<bool>(_hashData);
    }

    // In-memory storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    typedef std::vector<_FieldValuePair> _FieldValuePairVector;

    struct _FlatSpecData {
        inline void DetachIfNotUnique() { fields.MakeUnique(); }
        _FlatSpecData() = default;
        explicit _FlatSpecData(Usd_EmptySharedTagType)
            : fields(Usd_EmptySharedTag) {}
        Usd_Shared<_FieldValuePairVector> fields;
    };

    struct _MapSpecData {
        inline void DetachIfNotUnique() { fields.MakeUnique(); }
        Usd_Shared<_FieldValuePairVector> fields;
        SdfSpecType specType;
    };

    // Flat map for storing _SpecData coming off disk.  We switch to a hash
    // table when we mutate the set of specs.
    typedef boost::container::flat_map<
        SdfPath, _FlatSpecData, SdfPath::FastLessThan> _FlatMap;

    typedef std::unordered_map<
        SdfPath, _MapSpecData, SdfPath::Hash> _HashMap;

    // In-memory data for specs.  If _hashData is not null, it holds the data,
    // otherwise _flatData does.
    _FlatMap _flatData;
    _FlatMap::value_type *_flatLastSet;

    std::unique_ptr<_HashMap> _hashData;
    _HashMap::value_type *_hashLastSet;

    // Packed spec types.
    struct _SpecType {
        _SpecType() = default;
        explicit _SpecType(SdfSpecType type) : type(type) {}
        SdfSpecType type:8;
        static_assert(TF_BITS_FOR_VALUES(SdfNumSpecTypes) <= 8,
                      "Must be able to pack a SdfSpecType in a byte.");
    };
    std::vector<_SpecType> _flatTypes;

    // Underlying file.
    std::unique_ptr<CrateFile> _crateFile;
};



////////////////////////////////////////////////////////////////////////
// Usd_CrateData

Usd_CrateData::Usd_CrateData() : _impl(new Usd_CrateDataImpl)
{
}

Usd_CrateData::~Usd_CrateData()
{
}

/* static */
TfToken const &
Usd_CrateData::GetSoftwareVersionToken()
{
    return CrateFile::GetSoftwareVersionToken();
}

/* static */
bool
Usd_CrateData::CanRead(string const &fileName)
{
    return CrateFile::CanRead(fileName);
}

bool
Usd_CrateData::Save(string const &fileName)
{
    if (fileName.empty()) {
        TF_CODING_ERROR("Tried to save to empty fileName");
        return false;
    }

    bool hasFile = not _impl->GetFileName().empty();
    bool saveToOtherFile = _impl->GetFileName() != fileName;
        
    if (hasFile and saveToOtherFile) {
        // We copy to a temporary data and save that.
        Usd_CrateData tmp;
        tmp.CopyFrom(SdfAbstractDataConstPtr(this));
        return tmp.Save(fileName);
    }

    return _impl->Save(fileName);
}

bool
Usd_CrateData::Open(const std::string &fileName)
{
    return _impl->Open(fileName);
}

// ------------------------------------------------------------------------- //
// Abstract DataImplementation.
//

bool
Usd_CrateData::HasSpec(const SdfAbstractDataSpecId &id) const
{
    return _impl->HasSpec(id);
}

void
Usd_CrateData::EraseSpec(const SdfAbstractDataSpecId &id)
{
    _impl->EraseSpec(id);
}

void
Usd_CrateData::MoveSpec(const SdfAbstractDataSpecId& oldId,
                        const SdfAbstractDataSpecId& newId)
{
    return _impl->MoveSpec(oldId, newId);
}

SdfSpecType
Usd_CrateData::GetSpecType(const SdfAbstractDataSpecId &id) const
{
    return _impl->GetSpecType(id);
}

void
Usd_CrateData::CreateSpec(const SdfAbstractDataSpecId &id, SdfSpecType specType)
{
    _impl->CreateSpec(id, specType);
}

void
Usd_CrateData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    _impl->_VisitSpecs(*this, visitor);
}

bool
Usd_CrateData::Has(const SdfAbstractDataSpecId& id,
                   const TfToken & field,
                   SdfAbstractDataValue* value) const
{
    return _impl->Has(id, field, value);
}

bool
Usd_CrateData::Has(const SdfAbstractDataSpecId& id,
                   const TfToken & field,
                   VtValue *value) const
{
    return _impl->Has(id, field, value);
}

VtValue
Usd_CrateData::Get(const SdfAbstractDataSpecId& id, const TfToken & field) const
{
    return _impl->Get(id, field);
}

std::vector<TfToken>
Usd_CrateData::List(const SdfAbstractDataSpecId& id) const
{
    return _impl->List(id);
}

void
Usd_CrateData::Set(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   const VtValue& value)
{
    return _impl->Set(id, fieldName, value);
}

void
Usd_CrateData::Set(const SdfAbstractDataSpecId& id, const TfToken& field,
                   const SdfAbstractDataConstValue& value)
{
    return _impl->Set(id, field, value);
}

void
Usd_CrateData::Erase(const SdfAbstractDataSpecId& id, const TfToken & field)
{
    return _impl->Erase(id, field);
}

// ------------------------------------------------------------------------- //
// Time Sample API.
//
std::set<double>
Usd_CrateData::ListAllTimeSamples() const
{
    return _impl->ListAllTimeSamples();
}

std::set<double>
Usd_CrateData::ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _impl->ListTimeSamplesForPath(id);
}

bool
Usd_CrateData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamples(time, tLower, tUpper);
}

size_t
Usd_CrateData::GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _impl->GetNumTimeSamplesForPath(id);
}

bool
Usd_CrateData::GetBracketingTimeSamplesForPath(
    const SdfAbstractDataSpecId& id,
    double time, double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamplesForPath(id, time, tLower, tUpper);
}

bool
Usd_CrateData::QueryTimeSample(const SdfAbstractDataSpecId& id,
                               double time, VtValue *value) const
{
    return _impl->QueryTimeSample(id, time, value);
}

bool
Usd_CrateData::QueryTimeSample(const SdfAbstractDataSpecId& id,
                               double time, SdfAbstractDataValue* value) const
{
    return _impl->QueryTimeSample(id, time, value);
}

void
Usd_CrateData::SetTimeSample(const SdfAbstractDataSpecId& id,
                             double time, const VtValue &value)
{
    return _impl->SetTimeSample(id, time, value);
}

void
Usd_CrateData::EraseTimeSample(const SdfAbstractDataSpecId& id, double time)
{
    return _impl->EraseTimeSample(id, time);
}
