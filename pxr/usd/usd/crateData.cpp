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
#include "pxr/pxr.h"
#include "pxr/usd/usd/crateData.h"

#include "crateFile.h"

#include "pxr/base/tf/bitUtils.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/typeInfoMap.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/sdf/payload.h"
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

PXR_NAMESPACE_OPEN_SCOPE

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
        // Close file synchronously.  We don't want a race condition
        // on Windows due to the file being open for an indeterminate
        // amount of time.
        _crateFile.reset();

        // Tear down asynchronously.
        WorkMoveDestroyAsync(_flatTypes);
        WorkMoveDestroyAsync(_flatData);
        if (_hashData)
            WorkMoveDestroyAsync(_hashData);
    }

    string const &GetAssetPath() const { return _crateFile->GetAssetPath(); }

    bool CanIncrementalSave(string const &fileName) {
        return _crateFile->CanPackTo(fileName);
    }

    bool Save(string const &fileName) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Save");

        TF_DESCRIBE_SCOPE("Saving usd binary file @%s@", fileName.c_str());
        
        // Sort by path for better namespace-grouped data layout.
        vector<SdfPath> sortedPaths;
        sortedPaths.reserve(_hashData ? _hashData->size() : _flatData.size());
        if (_hashData) {
            for (auto const &p: *_hashData) {
                sortedPaths.push_back(p.first);
            }
        } else {
            for (auto const &p: _flatData) {
                sortedPaths.push_back(p.first);
            }
        }
        tbb::parallel_sort(
            sortedPaths.begin(), sortedPaths.end(),
            [](SdfPath const &p1, SdfPath const &p2) {
                // Prim paths before property paths, then property paths grouped
                // by property name.
                bool p1IsProperty = p1.IsPropertyPath();
                bool p2IsProperty = p2.IsPropertyPath();
                switch ((int)p1IsProperty + (int)p2IsProperty) {
                case 1:
                    return !p1IsProperty;
                case 2:
                    if (p1.GetName() != p2.GetName()) {
                        return p1.GetName() < p2.GetName();
                    }
                // Intentional fall-through
                default:
                case 0:
                    return p1 < p2;
                }
            });

        // Now pack all the specs.
        if (CrateFile::Packer packer = _crateFile->StartPacking(fileName)) {
            if (_hashData) {
                for (auto const &p: sortedPaths) {
                    auto iter = _hashData->find(p);
                    packer.PackSpec(
                        p, iter->second.specType, iter->second.fields.Get());
                }
            } else {
                for (auto const &p: sortedPaths) {
                    auto iter = _flatData.find(p);
                    packer.PackSpec(
                        p, _flatTypes[iter-_flatData.begin()].type,
                        iter->second.fields.Get());
                }
            }
            if (packer.Close()) {
                return _PopulateFromCrateFile();
            }
        }
        
        return false;
    }

    bool Open(string const &assetPath) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Open");

        TF_DESCRIBE_SCOPE("Opening usd binary asset @%s@", assetPath.c_str());
        
        if (auto newData = CrateFile::Open(assetPath)) {
            _crateFile = std::move(newData);
            return _PopulateFromCrateFile();
        }
        return false;
    }

    inline bool StreamsData() const {
        return true;
    }

    inline bool _GetTargetOrConnectionListOp(
        SdfPath const &path, SdfPathListOp *listOp) const {
        if (path.IsPrimPropertyPath()) {
            VtValue targetPaths;
            if ((Has(path, SdfFieldKeys->TargetPaths, &targetPaths) ||
                 Has(path, SdfFieldKeys->ConnectionPaths, &targetPaths)) &&
                targetPaths.IsHolding<SdfPathListOp>()) {
                targetPaths.UncheckedSwap(*listOp);
                return true;
            }
        }
        return false;
    }
    
    inline bool _HasTargetOrConnectionSpec(SdfPath const &path) const {
        // We don't store target specs to save space, since in Usd we don't have
        // any fields that may be set on them.  Their presence is determined by
        // whether or not they appear in their owning relationship's Added or
        // Explicit items.
        using std::find;
        SdfPath parentPath = path.GetParentPath();
        SdfPath targetPath = path.GetTargetPath();
        SdfPathListOp listOp;
        if (_GetTargetOrConnectionListOp(parentPath, &listOp)) {
            if (listOp.IsExplicit()) {
                auto const &items = listOp.GetExplicitItems();
                return find(
                    items.begin(), items.end(), targetPath) != items.end();
            } else {
                auto const &added = listOp.GetAddedItems();
                auto const &prepended = listOp.GetPrependedItems();
                auto const &appended = listOp.GetAppendedItems();
                return find(added.begin(),
                            added.end(), targetPath) != added.end() ||
                    find(prepended.begin(),
                         prepended.end(), targetPath) != prepended.end() ||
                    find(appended.begin(),
                         appended.end(), targetPath) != appended.end();
            }
        }
        return false;
    }

    inline bool HasSpec(const SdfPath &path) const {
        if (ARCH_UNLIKELY(path.IsTargetPath())) {
            return _HasTargetOrConnectionSpec(path);
        }
        return _hashData ?
            _hashData->find(path) != _hashData->end() :
            _flatData.find(path) != _flatData.end();
    }

    inline void EraseSpec(const SdfPath &path) {
        if (ARCH_UNLIKELY(path.IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }
        if (_MaybeMoveToHashTable()) {
            _hashLastSet = nullptr;
            TF_VERIFY(_hashData->erase(path), "%s", path.GetText());
        } else {
            auto iter = _flatData.find(path);
            size_t index = iter - _flatData.begin();
            if (TF_VERIFY(iter != _flatData.end(), "%s", path.GetText())) {
                _flatLastSet = nullptr;
                _flatData.erase(iter);
                _flatTypes.erase(_flatTypes.begin() + index);
            }
        }
    }

    inline void MoveSpec(const SdfPath& oldPath,
                         const SdfPath& newPath) {
        if (ARCH_UNLIKELY(oldPath.IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }

        if (_MaybeMoveToHashTable()) {
            auto oldIter = _hashData->find(oldPath);
            if (!TF_VERIFY(oldIter != _hashData->end()))
                return;
            _hashLastSet = nullptr;
            bool inserted = _hashData->emplace(newPath, oldIter->second).second;
            if (!TF_VERIFY(inserted))
                return;
            _hashData->erase(oldIter);
        } else {
            auto oldIter = _flatData.find(oldPath);
            if (!TF_VERIFY(oldIter != _flatData.end()))
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

    inline SdfSpecType GetSpecType(const SdfPath &path) const {
        if (path == SdfPath::AbsoluteRootPath()) {
            return SdfSpecTypePseudoRoot;
        }
        if (path.IsTargetPath()) {
            if (_HasTargetOrConnectionSpec(path)) {
                SdfPath parentPath = path.GetParentPath();
                SdfSpecType parentSpecType = GetSpecType(parentPath);
                if (parentSpecType == SdfSpecTypeRelationship) {
                    return SdfSpecTypeRelationshipTarget;
                }
                else if (parentSpecType == SdfSpecTypeAttribute) {
                    return SdfSpecTypeConnection;
                }
            }
            return SdfSpecTypeUnknown;
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
    CreateSpec(const SdfPath &path, SdfSpecType specType) {
        if (!TF_VERIFY(specType != SdfSpecTypeUnknown))
            return;
        if (path.IsTargetPath()) {
            // Do nothing, we do not store relationship target specs in usd.
            return;
        }
        if (_MaybeMoveToHashTable()) {
            // No need to blow the _hashLastSet cache here, since inserting into
            // the table won't invalidate existing references.
            (*_hashData)[path].specType = specType;
        } else {
            _flatLastSet = nullptr;
            auto iresult = _flatData.emplace(path, _FlatSpecData());
            auto index = iresult.first - _flatData.begin();
            if (iresult.second) {
                _flatTypes.insert(
                    _flatTypes.begin() + index, _SpecType(specType));
            } else {
                _flatTypes[index].type = specType;
            }
        }
    }



    inline void _VisitSpecs(SdfAbstractData const &data,
                            SdfAbstractDataSpecVisitor* visitor) const {

        // A helper function for spoofing target & connection spec existence --
        // we don't actually store those specs since we don't support fields on
        // them.
        auto doTargetAndConnectionSpecs =
            [this, &data, visitor](SdfPath const &path, SdfSpecType specType) {
            // Spoof existence of target & connection specs.
            if (specType == SdfSpecTypeAttribute ||
                specType == SdfSpecTypeRelationship) {
                SdfPathListOp listOp;
                SdfPathVector specs;
                if (_GetTargetOrConnectionListOp(path, &listOp)) {
                    if (listOp.IsExplicit()) {
                        specs = listOp.GetExplicitItems();
                    }
                    else {
                        auto const &added = listOp.GetAddedItems();
                        auto const &prepended = listOp.GetPrependedItems();
                        auto const &appended = listOp.GetAppendedItems();
                        specs.resize(
                            added.size() + prepended.size() + appended.size());
                        using std::copy;
                        copy(appended.begin(), appended.end(),
                             copy(prepended.begin(), prepended.end(),
                                  copy(added.begin(), added.end(),
                                       specs.begin())));
                        std::sort(specs.begin(), specs.end());
                        specs.erase(std::unique(specs.begin(), specs.end()),
                                    specs.end());
                    }
                    for (auto const &p: specs) {
                        SdfPath tp = path.AppendTarget(p);
                        if (!visitor->VisitSpec(data, tp)) {
                            return false;
                        }
                    }
                }
            }
            return true;
        };
        
        if (_hashData) {
            for (auto const &p: *_hashData) {
                if (!visitor->VisitSpec(data, p.first) ||
                    !doTargetAndConnectionSpecs(p.first, p.second.specType)) {
                    return;
                }
            }
        } else {
            size_t index = 0;
            for (auto const &p: _flatData) {
                if (!visitor->VisitSpec(data, p.first)) {
                    return;
                }
                SdfSpecType specType = _flatTypes[index++].type;
                if (!doTargetAndConnectionSpecs(p.first, specType)) {
                    return;
                }
            }
        }
    }

    inline bool Has(const SdfPath &path,
                    const TfToken &field,
                    SdfAbstractDataValue* value) const {
        if (VtValue const *fieldValue = _GetFieldValue(path, field)) {
            if (value) {
                VtValue val = _DetachValue(*fieldValue);
                if (field == SdfDataTokens->TimeSamples) {
                    // Special case, convert internal TimeSamples to
                    // SdfTimeSampleMap.
                    val = _MakeTimeSampleMap(val);
                } else if (field == SdfFieldKeys->Payload) {
                    // Special case, the payload field is expected to be a list
                    // op but can be represented in crate files as a single 
                    // SdfPayload to be compatible with older crate versions.
                    val = _ToPayloadListOpValue(val);
                }
                return value->StoreValue(val);
            }
            return true;
        }
        return false;
    }

    inline bool Has(const SdfPath& path,
                    const TfToken & field,
                    VtValue *value) const {
        // These are too expensive to do here, but could be uncommented for
        // debugging & tracking down corruption.
        //TF_DESCRIBE_SCOPE(GetAssetPath().c_str());
        //TfScopeDescription desc2(field.GetText());
        if (VtValue const *fieldValue = _GetFieldValue(path, field)) {
            if (value) {
                *value = _DetachValue(*fieldValue);
                if (field == SdfDataTokens->TimeSamples) {
                    // Special case, convert internal TimeSamples to
                    // SdfTimeSampleMap.
                    *value = _MakeTimeSampleMap(*value);
                } else if (field == SdfFieldKeys->Payload) {
                    // Special case, the payload field is expected to be a list
                    // op but can be represented in crate files as a single 
                    // SdfPayload to be compatible with older crate versions.
                    *value = _ToPayloadListOpValue(*value);
                }
            }
            return true;
        }
        return false;
    }

    inline VtValue Get(const SdfPath& path,
                       const TfToken & field) const {
        VtValue result;
        Has(path, field, &result);
        return result;
    }

    template <class Data>
    inline void _ListHelper(Data const &d, SdfPath const &path,
                            vector<TfToken> &out) const {
        auto i = d.find(path);
        if (i != d.end()) {
            auto const &fields = i->second.fields.Get();
            out.resize(fields.size());
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                out[j] = fields[j].first;
            }
        }
    }

    inline vector<TfToken> List(const SdfPath& path) const {
        vector<TfToken> names;
        _hashData ?
            _ListHelper(*_hashData, path, names) :
            _ListHelper(_flatData, path, names);
        return names;
    }

    template <class Data>
    inline void _SetHelper(
        Data &d, SdfPath const &path,
        typename Data::value_type *&lastSet,
        TfToken const &fieldName, VtValue const &value) {

        if (!lastSet || lastSet->first != path) {
            auto i = d.find(path);
            if (!TF_VERIFY(
                    i != d.end(),
                    "Tried to set field '%s' on nonexistent spec at <%s>",
                    path.GetText(), fieldName.GetText())) {
                return;
            }
            lastSet = &(*i);
        }
        
        VtValue const *valPtr = &value;
        VtValue convertedVal;
        if (fieldName == SdfDataTokens->TimeSamples) {
            convertedVal = _Make_TimeSamples(value);
            valPtr = &convertedVal;
        } else if (fieldName == SdfFieldKeys->Payload) {
            // Special case. Some payload list op values can be represented as
            // a single SdfPayload which is compatible with crate file software
            // version 0.7.0 and earlier. We always attempt to write the payload
            // field as old version compatible if possible in case we need to
            // write the file in a 0.7.0 compatible crate file.
            convertedVal = _FromPayloadListOpValue(value);
            valPtr = &convertedVal;
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

    inline void Set(const SdfPath& path,
                    const TfToken& fieldName,
                    const VtValue& value) {
        if (ARCH_UNLIKELY(value.IsEmpty())) {
            Erase(path, fieldName);
            return;
        }
        if (path.IsTargetPath()) {
            TF_CODING_ERROR("Cannot set fields on relationship target or "
                            "attribute connection specs: "
                            "<%s>:%s = %s", path.GetText(),
                            fieldName.GetText(), TfStringify(value).c_str());
            return;
        }
        _hashData ?
            _SetHelper(*_hashData, path, _hashLastSet, fieldName, value) :
            _SetHelper(_flatData, path, _flatLastSet, fieldName, value);
    }

    inline void Set(const SdfPath& path,
                    const TfToken& field,
                    const SdfAbstractDataConstValue& value) {
        VtValue val;
        TF_AXIOM(value.GetValue(&val));
        return Set(path, field, val);
    }

    template <class Data>
    inline void _EraseHelper(
        Data &d, const SdfPath& path, const TfToken & field) {
        auto i = d.find(path);
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

    inline void Erase(const SdfPath& path, const TfToken & field) {
        _hashData ?
            _EraseHelper(*_hashData, path, field) :
            _EraseHelper(_flatData, path, field);
    }

    inline std::set<double>
    ListAllTimeSamples() const {
        auto times = _ListAllTimeSamples();
        return std::set<double>(times.begin(), times.end());
    }

    inline std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const {
        auto const &times = _ListTimeSamplesForPath(path);
        return std::set<double>(times.begin(), times.end());
    }

    inline bool GetBracketingTimeSamples(
        double time, double* tLower, double* tUpper) const {
        return _GetBracketingTimes(_ListAllTimeSamples(), time, tLower, tUpper);
    }

    inline size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const {
        return _ListTimeSamplesForPath(path).size();
    }

    inline bool GetBracketingTimeSamplesForPath(
        const SdfPath& path,
        double time, double* tLower, double* tUpper) const {
        return _GetBracketingTimes(
            _ListTimeSamplesForPath(path), time, tLower, tUpper);
    }

    inline bool QueryTimeSample(const SdfPath& path, double time,
                                VtValue *value) const {
        // This is too expensive to do here, but could be uncommented to help
        // debugging or tracking down file corruption.
        //TF_DESCRIBE_SCOPE(GetAssetPath().c_str());
        if (VtValue const *fieldValue =
            _GetFieldValue(path, SdfDataTokens->TimeSamples)) {
            if (fieldValue->IsHolding<TimeSamples>()) {
                auto const &ts = fieldValue->UncheckedGet<TimeSamples>();
                auto const &times = ts.times.Get();
                auto iter = lower_bound(times.begin(), times.end(), time);
                if (iter == times.end() || *iter != time)
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

    inline bool QueryTimeSample(const SdfPath& path, double time,
                                SdfAbstractDataValue* value) const {
        if (!value)
            return QueryTimeSample(path, time, static_cast<VtValue *>(nullptr));
        VtValue vtVal;
        return QueryTimeSample(path, time, &vtVal) && value->StoreValue(vtVal);
    }

    inline void
    SetTimeSample(const SdfPath& path, double time,
                  const VtValue & value) {
        if (value.IsEmpty()) {
            EraseTimeSample(path, time);
            return;
        }

        TimeSamples newSamples;
        
        VtValue *fieldValue =
            _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);
        
        if (fieldValue && fieldValue->IsHolding<TimeSamples>()) {
            fieldValue->UncheckedSwap(newSamples);
        }
        
        // Insert or overwrite time into newTimes.
        auto iter = lower_bound(newSamples.times.Get().begin(),
                                newSamples.times.Get().end(), time);
        if (iter == newSamples.times.Get().end() || *iter != time) {
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
            Set(path, SdfDataTokens->TimeSamples, VtValue::Take(newSamples));
        }
    }
    
    inline void EraseTimeSample(const SdfPath& path, double time) {
        TimeSamples newSamples;
        
        VtValue *fieldValue =
            _GetMutableFieldValue(path, SdfDataTokens->TimeSamples);
        
        if (fieldValue && fieldValue->IsHolding<TimeSamples>()) {
            fieldValue->UncheckedSwap(newSamples);
        } else {
            return;
        }
        
        // Insert or overwrite time into newTimes.
        auto iter = lower_bound(newSamples.times.Get().begin(),
                                newSamples.times.Get().end(), time);
        if (iter == newSamples.times.Get().end() || *iter != time)
            return;

        // If we're removing the last sample, remove the entire field to be
        // consistent with SdfData's implementation.
        if (newSamples.times.Get().size() == 1) {
            Erase(path, SdfDataTokens->TimeSamples);
        } else {
            // Otherwise remove just the one sample.
            auto index = iter-newSamples.times.Get().begin();
            // Make the samples mutable, which may invalidate 'iter'.
            _crateFile->MakeTimeSampleTimesAndValuesMutable(newSamples);
            newSamples.times.GetMutable().erase(
                newSamples.times.GetMutable().begin() + index);
            newSamples.values.erase(newSamples.values.begin() + index);
            
            fieldValue->UncheckedSwap(newSamples);
        }
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
        if (rep.IsInlined() || rep.GetType() == TypeEnum::TimeSamples) {
            ret = _crateFile->UnpackValue(rep);
        } else {
            ret = rep;
        }
        return ret;
    }

    inline std::vector<double> const &
    _ListTimeSamplesForPath(const SdfPath &path) const {
        TF_DESCRIBE_SCOPE(GetAssetPath().c_str());
        if (const VtValue* fieldValue =
            _GetFieldValue(path, SdfDataTokens->TimeSamples)) {
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
            auto const &times = _ListTimeSamplesForPath(p.first);
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

    // Converts the value to a SdfPayloadListOp value if possible.
    inline VtValue _ToPayloadListOpValue(VtValue const &val) const {
        // Can convert if the value holds an SdfPayload. 
        if (val.IsHolding<SdfPayload>()) {
            const SdfPayload &payload = val.UncheckedGet<SdfPayload>();
            SdfPayloadListOp result;
            // Support for payload list ops and internal payloads were added 
            // at the same time. So semantically, a single SdfPayload with an
            // empty asset path was equivalent to setting the payload to be
            // explicitly none. We maintain this semantic meaning so that we
            // can continue to read older versions of crate files correctly.
            if (payload.GetAssetPath().empty()) {
                // Explicitly empty payload list
                result.ClearAndMakeExplicit();
            } else {
                // Explicit payload list containing the single payload.
                result.SetExplicitItems(SdfPayloadVector(1, payload));
            }
            return VtValue::Take(result);
        }    
        // Value is returned as is if it's already a payload list op or if it's
        // any other type.
        return val;
    }

    // Converts the value from a SdfPayloadListOp value to an SdfPayload only
    // if it can be semantically represented as a single SdfPayload
    inline VtValue _FromPayloadListOpValue(VtValue const &val) const {
        if (val.IsHolding<SdfPayloadListOp>()) {
            const SdfPayloadListOp &listOp = val.UncheckedGet<SdfPayloadListOp>();
            // The list must be explicit to be represented as a single 
            // SdfPayload.
            if (listOp.IsExplicit()) {
                if (listOp.GetExplicitItems().size() == 0) {
                    // If the list is explicitly empty, it is equivalent to a
                    // default SdfPayload.
                    return VtValue(SdfPayload());
                } else if (listOp.GetExplicitItems().size() == 1) {
                    // Otherwise an explicit list of one payload may be 
                    // convertible. Even if we have a single explicit payload, 
                    // we must check whether it is internal as an SdfPayload 
                    // with no asset path was used to represent "payload = none"
                    // in older versions and we need keep those semantics.
                    const SdfPayload &payload = listOp.GetExplicitItems().front();
                    if (!payload.GetAssetPath().empty()) {
                        return VtValue(payload);
                    }
                }
            }
        }
        // Fall through to the original value if no SdfPayload conversion is
        // possible.
        return val;
    }

    template <class Data>
    inline VtValue const *
    _GetFieldValueHelper(Data const &d,
                         SdfPath const &path,
                         TfToken const &field) const {
        auto i = d.find(path);
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
    _GetFieldValue(SdfPath const &path,
                   TfToken const &field) const {
        return _hashData ?
            _GetFieldValueHelper(*_hashData, path, field) :
            _GetFieldValueHelper(_flatData, path, field);
    }

    template <class Data>
    inline VtValue *
    _GetMutableFieldValueHelper(Data &d,
                                SdfPath const &path,
                                TfToken const &field) {
        auto i = d.find(path);
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
    _GetMutableFieldValue(const SdfPath& path,
                          const TfToken& field) {
        return _hashData ?
            _GetMutableFieldValueHelper(*_hashData, path, field) :
            _GetMutableFieldValueHelper(_flatData, path, field);
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
        if (!_hashData && _flatData.size() > FlatDataThreshold) {
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
Usd_CrateData::CanRead(string const &assetPath)
{
    return CrateFile::CanRead(assetPath);
}

bool
Usd_CrateData::Save(string const &fileName)
{
    if (fileName.empty()) {
        TF_CODING_ERROR("Tried to save to empty fileName");
        return false;
    }

    if (_impl->CanIncrementalSave(fileName)) {
        return _impl->Save(fileName);
    }
    else {
        // We copy to a temporary data and save that.
        Usd_CrateData tmp;
        tmp.CopyFrom(SdfAbstractDataConstPtr(this));
        return tmp.Save(fileName);
    }
}

bool
Usd_CrateData::Open(const std::string &assetPath)
{
    return _impl->Open(assetPath);
}

// ------------------------------------------------------------------------- //
// Abstract Data Implementation.
//

bool
Usd_CrateData::StreamsData() const
{
    return _impl->StreamsData();
}

bool
Usd_CrateData::HasSpec(const SdfPath &path) const
{
    return _impl->HasSpec(path);
}

void
Usd_CrateData::EraseSpec(const SdfPath &path)
{
    _impl->EraseSpec(path);
}

void
Usd_CrateData::MoveSpec(const SdfPath& oldPath,
                        const SdfPath& newPath)
{
    return _impl->MoveSpec(oldPath, newPath);
}

SdfSpecType
Usd_CrateData::GetSpecType(const SdfPath &path) const
{
    return _impl->GetSpecType(path);
}

void
Usd_CrateData::CreateSpec(const SdfPath &path, SdfSpecType specType)
{
    _impl->CreateSpec(path, specType);
}

void
Usd_CrateData::_VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    _impl->_VisitSpecs(*this, visitor);
}

bool
Usd_CrateData::Has(const SdfPath& path,
                   const TfToken & field,
                   SdfAbstractDataValue* value) const
{
    return _impl->Has(path, field, value);
}

bool
Usd_CrateData::Has(const SdfPath& path,
                   const TfToken & field,
                   VtValue *value) const
{
    return _impl->Has(path, field, value);
}

VtValue
Usd_CrateData::Get(const SdfPath& path, const TfToken & field) const
{
    return _impl->Get(path, field);
}

std::vector<TfToken>
Usd_CrateData::List(const SdfPath& path) const
{
    return _impl->List(path);
}

void
Usd_CrateData::Set(const SdfPath& path, const TfToken& fieldName,
                   const VtValue& value)
{
    return _impl->Set(path, fieldName, value);
}

void
Usd_CrateData::Set(const SdfPath& path, const TfToken& field,
                   const SdfAbstractDataConstValue& value)
{
    return _impl->Set(path, field, value);
}

void
Usd_CrateData::Erase(const SdfPath& path, const TfToken & field)
{
    return _impl->Erase(path, field);
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
Usd_CrateData::ListTimeSamplesForPath(const SdfPath& path) const
{
    return _impl->ListTimeSamplesForPath(path);
}

bool
Usd_CrateData::GetBracketingTimeSamples(
    double time, double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamples(time, tLower, tUpper);
}

size_t
Usd_CrateData::GetNumTimeSamplesForPath(const SdfPath& path) const
{
    return _impl->GetNumTimeSamplesForPath(path);
}

bool
Usd_CrateData::GetBracketingTimeSamplesForPath(
    const SdfPath& path,
    double time, double* tLower, double* tUpper) const
{
    return _impl->GetBracketingTimeSamplesForPath(path, time, tLower, tUpper);
}

bool
Usd_CrateData::QueryTimeSample(const SdfPath& path,
                               double time, VtValue *value) const
{
    return _impl->QueryTimeSample(path, time, value);
}

bool
Usd_CrateData::QueryTimeSample(const SdfPath& path,
                               double time, SdfAbstractDataValue* value) const
{
    return _impl->QueryTimeSample(path, time, value);
}

void
Usd_CrateData::SetTimeSample(const SdfPath& path,
                             double time, const VtValue &value)
{
    return _impl->SetTimeSample(path, time, value);
}

void
Usd_CrateData::EraseTimeSample(const SdfPath& path, double time)
{
    return _impl->EraseTimeSample(path, time);
}

PXR_NAMESPACE_CLOSE_SCOPE

