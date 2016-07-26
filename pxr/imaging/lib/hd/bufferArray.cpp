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
#include "pxr/imaging/hd/bufferArray.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/base/tf/iterator.h"

static std::atomic_size_t _uniqueVersion(0);

HdBufferArray::HdBufferArray(TfToken const &role,
                             TfToken const garbageCollectionPerfToken) 
    : _needsReallocation(false),
      _rangeList(),
      _rangeCount(0),
      _rangeListLock(),
      _role(role), 
      _garbageCollectionPerfToken(garbageCollectionPerfToken),
      _version(_uniqueVersion++),   // Atomic
      _maxNumRanges(1)
{
    /*NOTHING*/
}

HdBufferArray::~HdBufferArray()
{
    /*NOTHING*/
}

void
HdBufferArray::IncrementVersion()
{
    _version = _uniqueVersion++;  // Atomic
}

HdBufferResourceSharedPtr
HdBufferArray::GetResource() const
{
    HD_TRACE_FUNCTION();

    if (_resourceList.empty()) return HdBufferResourceSharedPtr();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // make sure this buffer array has only one resource.
        GLuint id = _resourceList.begin()->second->GetId();
        TF_FOR_ALL (it, _resourceList) {
            if (it->second->GetId() != id) {
                TF_CODING_ERROR("GetResource(void) called on"
                                "HdBufferArray having multiple GL resources");
            }
        }
    }

    // returns the first item
    return _resourceList.begin()->second;
}

HdBufferResourceSharedPtr
HdBufferArray::GetResource(TfToken const& name)
{
    HD_TRACE_FUNCTION();

    // linear search.
    // The number of buffer resources should be small (<10 or so).
    for (HdBufferResourceNamedList::iterator it = _resourceList.begin();
         it != _resourceList.end(); ++it) {
        if (it->first == name) return it->second;
    }
    return HdBufferResourceSharedPtr();
}

HdBufferSpecVector
HdBufferArray::GetBufferSpecs() const
{
    HdBufferSpecVector result;
    result.reserve(_resourceList.size());
    TF_FOR_ALL (it, _resourceList) {
        HdBufferResourceSharedPtr const &bres = it->second;
        HdBufferSpec spec(it->first, bres->GetGLDataType(), bres->GetNumComponents());
        result.push_back(spec);
    }
    return result;
}

HdBufferResourceSharedPtr
HdBufferArray::_AddResource(TfToken const& name,
                            int glDataType,
                            short numComponents,
                            int arraySize,
                            int offset,
                            int stride)
{
    HD_TRACE_FUNCTION();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // duplication check
        HdBufferResourceSharedPtr bufferRes = GetResource(name);
        if (not TF_VERIFY(not bufferRes)) {
            return bufferRes;
        }
    }

    HdBufferResourceSharedPtr bufferRes = HdBufferResourceSharedPtr(
        new HdBufferResource(GetRole(), glDataType,
                             numComponents, arraySize, offset, stride));

    _resourceList.push_back(std::make_pair(name, bufferRes));
    return bufferRes;
}

bool
HdBufferArray::TryAssignRange(HdBufferArrayRangeSharedPtr &range)
{
    // Garbage collection should make sure range list is
    // contiguous, so we only ever need to insert at end.
    size_t allocIdx = _rangeCount++;

    if (allocIdx >= _maxNumRanges) {
        // Make sure out range count remains clamp at _maxNumRanges.
        // It's ok if multiple threads race to set this to the same value
        // (other than the cache line bouncing)
        _rangeCount.store(_maxNumRanges);

        return false;
    }

     size_t newSize = allocIdx + 1;

    // As we might grow the array (which would result in a copy)
    // we need to lock around the whole insert into _rangeList.
    //
    // An optomisation could be to change into a read/write lock.
    {
        std::lock_guard<std::mutex> lock(_rangeListLock);

        if (newSize > _rangeList.size()) {
            _rangeList.resize(newSize);
        }
        _rangeList[allocIdx] = range;
    }

    range->SetBufferArray(this);
    
    _needsReallocation = true;  // Multiple threads may try to set this to true at once, which is ok.

    return true;
}

void
HdBufferArray::RemoveUnusedRanges()
{
    // Local copy, because we don't want to perform atomic ops
    size_t numRanges = _rangeCount;
    size_t idx = 0;
    while (idx < numRanges) {
        if (_rangeList[idx].expired()) {
            // Order of range objects doesn't matter so use range at end to fill gap.
            _rangeList[idx] = _rangeList[numRanges - 1];
            _rangeList[numRanges - 1].reset();
            --numRanges;

            HD_PERF_COUNTER_INCR(_garbageCollectionPerfToken);
            // Don't increament idx as we need to check the value we just moved into the slot.
        } else {
            ++idx;
        }
    }

    // Now update atomic copy with new size.
    _rangeCount = numRanges;
}

HdBufferArrayRangePtr
HdBufferArray::GetRange(size_t idx) const
{
    // XXX: This would need a lock on range list
    // if run in parrelel to TryAssignRange

    TF_VERIFY(idx < _rangeCount); // Note this maybe lower than the actual array
    return _rangeList[idx];
}

void
HdBufferArray::_SetRangeList(
    std::vector<HdBufferArrayRangeSharedPtr> const &ranges)
{
    std::lock_guard<std::mutex> lock(_rangeListLock);

    _rangeList.clear();
    _rangeList.assign(ranges.begin(), ranges.end());
    _rangeCount = _rangeList.size();
    TF_FOR_ALL(it, ranges) {
        (*it)->SetBufferArray(this);
    }
}

/*virtual*/
size_t
HdBufferArray::GetMaxNumElements() const
{
    // 1 element per range is allowed by default (for uniform buffers)
    return _maxNumRanges;
}

std::ostream &operator <<(std::ostream &out,
                          const HdBufferArray &self)
{
    out << "    HdBufferArray:\n";
    int resourceIndex = 0;
    TF_FOR_ALL (bresIt, self.GetResources()) {
        out << "      " << resourceIndex << " : " << bresIt->first.GetText() << "\n";
        resourceIndex++;
    }
    self.DebugDump(out);
    return out;
}

