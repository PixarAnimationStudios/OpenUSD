//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/trace/trace.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/schema.h"

#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

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

    struct _SpecData;

public:

    Usd_CrateDataImpl(bool detached) 
        : _lastSet(_data.end())
        , _crateFile(CrateFile::CreateNew(detached)) {}
    
    ~Usd_CrateDataImpl() {
        // Close file synchronously.  We don't want a race condition
        // on Windows due to the file being open for an indeterminate
        // amount of time.
        _crateFile.reset();

        // Tear down asynchronously.
        WorkMoveDestroyAsync(_data);
    }

    string const &GetAssetPath() const { return _crateFile->GetAssetPath(); }

    bool Save(string const &fileName) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Save");

        TF_DESCRIBE_SCOPE("Saving usd binary file @%s@", fileName.c_str());
        
        // Sort by path for better namespace-grouped data layout.
        vector<SdfPath> sortedPaths;
        sortedPaths.reserve(_data.size());
        for (auto const &p: _data) {
            sortedPaths.push_back(p.first);
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
            for (auto const &p: sortedPaths) {
                auto iter = _data.find(p);
                packer.PackSpec(
                    p, iter->second.specType, iter->second.fields.Get());
            }
            if (packer.Close()) {
                return _PopulateFromCrateFile();
            }
        }
        
        return false;
    }

    template <class ...Args>
    bool Open(string const& assetPath, Args&&... args) {
        TfAutoMallocTag tag("Usd_CrateDataImpl::Open");

        TF_DESCRIBE_SCOPE("Opening usd binary asset @%s@", assetPath.c_str());
        
        if (auto newData = 
                CrateFile::Open(assetPath, std::forward<Args>(args)...)) {
            _crateFile = std::move(newData);
            return _PopulateFromCrateFile();
        }
        return false;
    }

    inline bool StreamsData() const {
        return _crateFile && !_crateFile->IsDetached();
    }

    // Return either TargetPaths or ConnectionPaths as a VtValue.  If
    // specTypeOut is not null, set it to SdfSpecTypeRelationship if we find
    // TargetPaths, otherwise to SdfSpecTypeAttribute if we find
    // ConnectionPaths, otherwise SdfSpecTypeUnknown.
    inline VtValue
    _GetTargetOrConnectionListOpValue(
        SdfPath const &path, SdfSpecType *specTypeOut = nullptr) const {
        VtValue targetPaths;
        SdfSpecType specType = SdfSpecTypeUnknown;
        if (path.IsPrimPropertyPath()) {
            if (Has(path, SdfFieldKeys->TargetPaths, &targetPaths)) {
                specType = SdfSpecTypeRelationship;
            }
            else if (Has(path, SdfFieldKeys->ConnectionPaths, &targetPaths)) {
                specType = SdfSpecTypeAttribute;
            }
            if (!targetPaths.IsHolding<SdfPathListOp>()) {
                specType = SdfSpecTypeUnknown;
                targetPaths = VtValue();
            }
        }
        if (specTypeOut) {
            *specTypeOut = specType;
        }
        return targetPaths;
    }
    
    inline bool _HasTargetOrConnectionSpec(SdfPath const &path) const {
        // We don't store target specs to save space, since in Usd we don't have
        // any fields that may be set on them.  Their presence is determined by
        // whether or not they appear in their owning relationship's Added or
        // Explicit items.
        using std::find;
        SdfPath parentPath = path.GetParentPath();
        SdfPath targetPath = path.GetTargetPath();
        VtValue listOpVal = _GetTargetOrConnectionListOpValue(parentPath);
        if (!listOpVal.IsEmpty()) {
            SdfPathListOp const &listOp =
                listOpVal.UncheckedGet<SdfPathListOp>();
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
        return _GetSpecData(path) != nullptr;
    }

    inline void EraseSpec(const SdfPath &path) {
        if (ARCH_UNLIKELY(path.IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }
        _lastSet = _data.end();
        TF_VERIFY(_data.erase(path), "%s", path.GetText());
    }

    inline void MoveSpec(const SdfPath& oldPath,
                         const SdfPath& newPath) {
        if (ARCH_UNLIKELY(oldPath.IsTargetPath())) {
            // Do nothing, we do not store target specs.
            return;
        }
        auto oldIter = _data.find(oldPath);
        if (!TF_VERIFY(oldIter != _data.end())) {
            return;
        }
        _lastSet = _data.end();
        auto tmpFields(std::move(oldIter->second));
        _data.erase(oldIter);
        auto iresult = _data.emplace(newPath, std::move(tmpFields));
        TF_VERIFY(iresult.second);
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
        if (_SpecData const *specData = _GetSpecData(path)) {
            return specData->specType;
        }
        return SdfSpecTypeUnknown;
    }

    inline void
    CreateSpec(const SdfPath &path, SdfSpecType specType) {
        if (!TF_VERIFY(specType != SdfSpecTypeUnknown)) {
            return;
        }
        if (path.IsTargetPath()) {
            // Do nothing, we do not store relationship target specs in usd.
            return;
        }
        // Need to blow/reset the _lastSet cache here, since inserting
        // into the table will invalidate existing references.
        auto iter = _data.emplace(path, _SpecData()).first;
        iter.value().specType = specType;
        _lastSet = iter;
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
                VtValue listOpVal = _GetTargetOrConnectionListOpValue(path);
                if (!listOpVal.IsEmpty()) {
                    SdfPathListOp const &listOp =
                        listOpVal.UncheckedGet<SdfPathListOp>();
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
        
        for (auto const &p: _data) {
            if (!visitor->VisitSpec(data, p.first) ||
                !doTargetAndConnectionSpecs(p.first, p.second.specType)) {
                return;
            }
        }
    }

    inline bool Has(const SdfPath &path,
                    const TfToken &field,
                    SdfAbstractDataValue* value,
                    SdfSpecType *specType=nullptr) const {
        
        if (VtValue const *fieldValue = _GetFieldValue(path, field, specType)) {
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
                return value->StoreValue(std::move(val));
            }
            return true;
        }
        else if (ARCH_UNLIKELY(
                     field == SdfChildrenKeys->ConnectionChildren ||
                     field == SdfChildrenKeys->RelationshipTargetChildren)) {
            return _HasConnectionOrTargetChildren(path, field, value);
        }
        return false;
    }

    inline bool Has(const SdfPath& path,
                    const TfToken & field,
                    VtValue *value,
                    SdfSpecType *specType=nullptr) const {
        // These are too expensive to do here, but could be uncommented for
        // debugging & tracking down corruption.
        //TF_DESCRIBE_SCOPE(GetAssetPath().c_str());
        //TfScopeDescription desc2(field.GetText());
        if (VtValue const *fieldValue = _GetFieldValue(path, field, specType)) {
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
        else if (ARCH_UNLIKELY(
                     field == SdfChildrenKeys->ConnectionChildren ||
                     field == SdfChildrenKeys->RelationshipTargetChildren)) {
            return _HasConnectionOrTargetChildren(path, field, value);
        }
        return false;
    }

    bool _HasConnectionOrTargetChildren(const SdfPath &path,
                                        const TfToken &field,
                                        SdfAbstractDataValue *value) const {
        VtValue listOpVal = _GetTargetOrConnectionListOpValue(path);
        if (!listOpVal.IsEmpty()) {
            if (value) {
                SdfPathListOp const &plo =
                    listOpVal.UncheckedGet<SdfPathListOp>();
                SdfPathVector paths;
                plo.ApplyOperations(&paths);
                value->StoreValue(paths);
            }
            return true;
        }
        return false;
    }

    bool _HasConnectionOrTargetChildren(const SdfPath &path,
                                        const TfToken &field,
                                        VtValue *value) const {
        VtValue listOpVal = _GetTargetOrConnectionListOpValue(path);
        if (!listOpVal.IsEmpty()) {
            if (value) {
                SdfPathListOp const &plo =
                    listOpVal.UncheckedGet<SdfPathListOp>();
                SdfPathVector paths;
                plo.ApplyOperations(&paths);
                *value = paths;
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

    inline std::type_info const &GetTypeid(const SdfPath& path,
                                           const TfToken& field) const {
        if (VtValue const *fieldValue = _GetFieldValue(path, field)) {
            return fieldValue->IsHolding<ValueRep>() ?
                _crateFile->GetTypeid(fieldValue->UncheckedGet<ValueRep>()) :
                fieldValue->GetTypeid();
        }
        return typeid(void);
    }

    inline vector<TfToken> List(const SdfPath& path) const {
        vector<TfToken> names;
        if (_SpecData const *specData = _GetSpecData(path)) {
            auto const &fields = specData->fields.Get();
            names.resize(fields.size());
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                names[j] = fields[j].first;
            }
            // If 'path' is a property path, we may have to "spoof" the
            // existence of connectionChildren or targetChildren.
            if (path.IsPrimPropertyPath()) {
                SdfSpecType specType = SdfSpecTypeUnknown;
                VtValue listOpVal = 
                    _GetTargetOrConnectionListOpValue(path, &specType);
                if (specType == SdfSpecTypeRelationship) {
                    names.push_back(SdfChildrenKeys->RelationshipTargetChildren);
                }
                else if (specType == SdfSpecTypeAttribute) {
                    names.push_back(SdfChildrenKeys->ConnectionChildren);
                }
            }
        }
        return names;
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
        if (_lastSet == _data.end() || _lastSet->first != path) {
            auto i = _data.find(path);
            if (!TF_VERIFY(
                    i != _data.end(),
                    "Tried to set field '%s' on nonexistent spec at <%s>",
                    path.GetText(), fieldName.GetText())) {
                return;
            }
            _lastSet = i;
        }

        if (fieldName == SdfChildrenKeys->ConnectionChildren ||
            fieldName == SdfChildrenKeys->RelationshipTargetChildren) {
            // Silently do nothing -- we synthesize these fields from the list
            // ops.
            return;
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
        
        auto &spec = _lastSet.value();
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
                    const TfToken& field,
                    const SdfAbstractDataConstValue& value) {
        VtValue val;
        TF_AXIOM(value.GetValue(&val));
        return Set(path, field, val);
    }

    inline void Erase(const SdfPath& path, const TfToken & field) {
        auto i = _data.find(path);
        if (i == _data.end())
            return;

        auto &spec = i.value();
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

        TfErrorMark m;

        WorkDispatcher dispatcher;

        // Pull all the data out of the crate file structure that we'll
        // consume.
        vector<CrateFile::Spec> specs;
        vector<CrateFile::Field> fields;
        vector<Usd_CrateFile::FieldIndex> fieldSets;
        _crateFile->RemoveStructuralData(specs, fields, fieldSets);
                
        // Remove any target specs, we do not store target specs in Usd, but old
        // files could contain them.  We stopped writing target specs in version
        // 0.1.0, so skip this step if the version is newer or equal to that.
        if (_crateFile->GetFileVersion() < CrateFile::Version(0, 1, 0)) {
            specs.erase(
                remove_if(
                    specs.begin(), specs.end(),
                    [this](CrateFile::Spec const &spec) {
                        return _crateFile->GetPath(
                            spec.pathIndex).IsTargetPath();
                    }),
                specs.end());
        }

        CrateFile const * const crateFile = _crateFile.get();

        // Reserving the space in the _data table is pretty expensive, so start
        // that upfront as a task and overlap it with building up all the live
        // field sets.
        dispatcher.Run([this, &specs, crateFile]() {
            TfAutoMallocTag tag("Usd", "Usd_CrateDataImpl::Open",
                                "Usd_CrateDataImpl main hash table");
            // over-reserve by 25% to help ensure no rehashes.
            _data.reserve(specs.size() + (specs.size() >> 2));
            
            // Do all the insertions first, since inserting can invalidate
            // references.
            for (size_t i = 0; i != specs.size(); ++i) {
                _data.emplace(crateFile->GetPath(specs[i].pathIndex),
                              Usd_EmptySharedTag);
            }
        });

        // XXX robin_map ?
        typedef Usd_Shared<_FieldValuePairVector> SharedFieldValuePairVector;
        unordered_map<
            FieldSetIndex, SharedFieldValuePairVector, _Hasher> liveFieldSets;

        for (auto fsBegin = fieldSets.begin(),
                 fsEnd = find(fsBegin, fieldSets.end(), FieldIndex());
             fsBegin != fieldSets.end();
             fsBegin = fsEnd + 1,
                 fsEnd = find(fsBegin, fieldSets.end(), FieldIndex())) {
                    
            // Add this range to liveFieldSets.
            TfAutoMallocTag tag("field data");
            auto &fieldValuePairs =
                liveFieldSets[FieldSetIndex(fsBegin-fieldSets.begin())];
                    
            dispatcher.Run(
                [this, fsBegin, fsEnd, &fields, &fieldValuePairs]()  {
                    try{
                        // XXX Won't need first two tags when bug #132031 is
                        // addressed
                        TfAutoMallocTag tag(
                            "Usd", "Usd_CrateDataImpl::Open", "field data");
                        auto &pairs = fieldValuePairs.GetMutable();
                        pairs.resize(fsEnd-fsBegin);
                        for (size_t i = 0; i < size_t(std::distance(fsBegin,fsEnd)); ++i) {
                            auto const &field = fields[fsBegin[i].value];
                            pairs[i].first = 
                                _crateFile->GetToken(field.tokenIndex);
                            pairs[i].second = _UnpackForField(field.valueRep);
                        } 
                    } catch (const std::exception &e){
                        TF_RUNTIME_ERROR("Encountered exception: %s %s", 
                            e.what(), _crateFile->GetAssetPath().c_str());

                    } catch (...) {
                        TF_RUNTIME_ERROR("Encountered unknown exception");
                    }
                });
        }
                
        dispatcher.Wait();

        if (!m.IsClean()) {
            return false;
        }

        // Create all the specData entries and store pointers to them.
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, specs.size()),
            [this, crateFile, &liveFieldSets, &specs](
                tbb::blocked_range<size_t> const &r) {
                for (size_t i = r.begin(),
                         end = r.end(); i != end; ++i) {
                    
                    CrateFile::Spec const &spec = specs[i];
                    _SpecData &specData = _data.find(
                        crateFile->GetPath(spec.pathIndex)).value();
                    
                    specData.specType = spec.specType;
                    specData.fields =
                        liveFieldSets.find(spec.fieldSetIndex)->second;
                    
                }
            },
            tbb::static_partitioner());

        _lastSet = _data.end();

        return true;
    }

    inline VtValue _UnpackForField(ValueRep rep) const {
        VtValue ret;
        if (rep.IsInlined() ||
            rep.GetType() == TypeEnum::TimeSamples ||
            rep.GetType() == TypeEnum::TokenVector) {
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

    inline vector<double> _ListAllTimeSamples() const {
        vector<double> allTimes, tmp; 
        for (auto const &p: _data) {
            tmp.swap(allTimes);
            allTimes.clear();
            auto const &times = _ListTimeSamplesForPath(p.first);
            set_union(tmp.begin(), tmp.end(), times.begin(), times.end(),
                      back_inserter(allTimes));
        }
        return allTimes;
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

    inline _SpecData const *
    _GetSpecData(SdfPath const &path) const {
        _SpecData const *specData = nullptr;
        auto i = _data.find(path);
        if (i != _data.end()) {
            specData = &i.value();
        }
        return specData;
    }

    inline VtValue const *
    _GetFieldValue(SdfPath const &path,
                   TfToken const &field,
                   SdfSpecType *specType=nullptr) const {
        if (_SpecData const *specData = _GetSpecData(path)) {
            if (specType) {
                *specType = specData->specType;
            }
            auto const &fields = specData->fields.Get();
            for (size_t j=0, jEnd = fields.size(); j != jEnd; ++j) {
                if (fields[j].first == field) {
                    return &fields[j].second;
                }
            }
        } else if (specType) {
            *specType = SdfSpecTypeUnknown;
        }
        return nullptr;
    }

    inline VtValue *
    _GetMutableFieldValue(const SdfPath& path,
                          const TfToken& field) {
        auto i = _lastSet != _data.end() && _lastSet->first == path ?
            _lastSet : _data.find(path);
        if (i != _data.end()) {
            auto &spec = i.value();
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

    inline VtValue _DetachValue(VtValue const &val) const {
        return val.IsHolding<ValueRep>() ?
            _crateFile->UnpackValue(val.UncheckedGet<ValueRep>()) : val;
    }

    inline void _ClearSpecData() {
        TfReset(_data);
        _lastSet = _data.end();
    }

    // In-memory storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    typedef std::vector<_FieldValuePair> _FieldValuePairVector;

    struct _SpecData {
        _SpecData() = default;
        explicit _SpecData(Usd_EmptySharedTagType) noexcept
            : fields(Usd_EmptySharedTag) {}
        inline void DetachIfNotUnique() { fields.MakeUnique(); }

        friend inline void swap(_SpecData &l, _SpecData &r) {
            std::swap(l.specType, r.specType);
            l.fields.swap(r.fields);
        }
        
        Usd_Shared<_FieldValuePairVector> fields;
        SdfSpecType specType;
    };

    using _HashMap = pxr_tsl::robin_map<
        SdfPath, _SpecData, SdfPath::Hash, std::equal_to<SdfPath>,
        std::allocator<std::pair<SdfPath, _SpecData>>,
        /*StoreHash=*/false>;

    // In-memory data for specs.
    _HashMap _data;
    _HashMap::iterator _lastSet; // cached last authored spec.

    // Underlying file.
    std::unique_ptr<CrateFile> _crateFile;
};



////////////////////////////////////////////////////////////////////////
// Usd_CrateData

Usd_CrateData::Usd_CrateData(bool detached) 
    : _impl(new Usd_CrateDataImpl(detached))
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

/* static */
bool
Usd_CrateData::CanRead(string const &assetPath,
                       std::shared_ptr<ArAsset> const &asset)
{
    return CrateFile::CanRead(assetPath, asset);
}

bool
Usd_CrateData::Save(string const &fileName)
{
    if (fileName.empty()) {
        TF_CODING_ERROR("Tried to save to empty fileName");
        return false;
    }

    return _impl->Save(fileName);
}

bool
Usd_CrateData::Export(string const &fileName)
{
    if (fileName.empty()) {
        TF_CODING_ERROR("Tried to save to empty fileName");
        return false;
    }

    // To Export, we copy to a temporary data and save that, since we need this
    // CrateData object to stay associated with its existing backing store.
    //
    // Usd_CrateData currently reloads the underlying asset to reinitialize its
    // internal members after a save. We use a non-detached Usd_CrateData here
    // to avoid any expense associated with detaching from the asset.
    Usd_CrateData tmp(/* detached = */ false);
    tmp.CopyFrom(SdfAbstractDataConstPtr(this));
    return tmp.Save(fileName);
}

bool
Usd_CrateData::Open(const std::string &assetPath,
                    bool detached)
{
    return _impl->Open(assetPath, detached);
}

bool
Usd_CrateData::Open(const std::string &assetPath,
                    const std::shared_ptr<ArAsset> &asset,
                    bool detached)
{
    return _impl->Open(assetPath, asset, detached);
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

bool
Usd_CrateData
::HasSpecAndField(const SdfPath &path, const TfToken &field,
                  SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    return _impl->Has(path, field, value, specType);
}

bool
Usd_CrateData
::HasSpecAndField(const SdfPath &path, const TfToken &field,
                  VtValue *value, SdfSpecType *specType) const
{
    return _impl->Has(path, field, value, specType);
}

VtValue
Usd_CrateData::Get(const SdfPath& path, const TfToken & field) const
{
    return _impl->Get(path, field);
}

std::type_info const &
Usd_CrateData::GetTypeid(const SdfPath& path, const TfToken& field) const
{
    return _impl->GetTypeid(path, field);
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

