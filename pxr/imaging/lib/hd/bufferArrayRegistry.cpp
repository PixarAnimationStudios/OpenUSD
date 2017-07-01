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
#include "pxr/imaging/hd/bufferArrayRegistry.h"
#include "pxr/imaging/hd/bufferArray.h"

PXR_NAMESPACE_OPEN_SCOPE


HdBufferArrayRegistry::HdBufferArrayRegistry()
 : _entries()
{
}


HdBufferArrayRangeSharedPtr HdBufferArrayRegistry::AllocateRange(
        HdAggregationStrategy *strategy,
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // early out for empty source
    if (!TF_VERIFY(!bufferSpecs.empty())) {
        return HdBufferArrayRangeSharedPtr();
    }

    if (!strategy) {
        TF_CODING_ERROR("Aggregation strategy is set to null");
        return HdBufferArrayRangeSharedPtr();
    }

    // compute an aggregation Id on current aggregation strategy
    HdAggregationStrategy::AggregationId aggrId =
        strategy->ComputeAggregationId(bufferSpecs);

    // We use insert to do a find and insert operation
    std::pair<_BufferArrayIndex::iterator, bool> result =_entries.insert(std::make_pair(aggrId, _Entry()));
    _Entry &entry = (result.first)->second;

    if (result.second) {
        // We just created a new entry so make sure it has a buffer in it.
        _InsertNewBufferArray(entry, HdBufferArraySharedPtr(), strategy, role, bufferSpecs);
    } else {

        // There's a protential multi-thread race condition where
        // another thread has created the entry and is still in the process of adding
        // the first buffer to it, therefore the list could be empty, so wait for it
        _EntryIsNotEmpty pred(entry);
        std::unique_lock<std::mutex> lock(entry.lock);
        entry.emptyCondition.wait(lock, pred);
    }

    
    HdBufferArrayRangeSharedPtr range = strategy->CreateBufferArrayRange();
    
    // Try to find where to insert the range.
    // while no new slots can free up during allocate,
    // garbage collection may create empty slots in entries.
    // So we have to go through the list to find slots.
    // Tough as this is Multi-thread, entries maybe added to the list.
    // This doesn't invalidate the interator, but need to be careful
    // on check end condition.

    _HdBufferArraySharedPtrList::iterator it   = entry.bufferArrays.begin();
    do {
        HdBufferArraySharedPtr currentArray = *it;

        if (!currentArray->TryAssignRange(range)) {
            _HdBufferArraySharedPtrList::iterator prev = it;
            ++it;

            if (it == entry.bufferArrays.end()) {
                // Reached end of buffer list, so try to insert new buffer
                // Only one thread will win and add the buffer
                // however, by the time we get back multiple buffers may have
                // been added, so rewind iterator.
                
                _InsertNewBufferArray(entry, currentArray, strategy, role, bufferSpecs);
                it = prev;
                ++it;
            }
        }        
    } while (!range->IsAssigned());

    return range;
}


void
HdBufferArrayRegistry::ReallocateAll(HdAggregationStrategy *strategy)
{
    for (auto& entry : _entries) {
        for (auto bufferIt = entry.second.bufferArrays.begin(), 
                         e = entry.second.bufferArrays.end();
             bufferIt != e; ++bufferIt) {

            HdBufferArraySharedPtr const &bufferArray = *bufferIt;
            if (!bufferArray->NeedsReallocation()) continue;

            // in case of over aggregation, split the buffer

            bufferArray->RemoveUnusedRanges();

            size_t maxTotalElements = bufferArray->GetMaxNumElements();
            size_t numTotalElements = 0;

            size_t rangeCount = bufferArray->GetRangeCount();
            std::vector<HdBufferArrayRangeSharedPtr> ranges;
            ranges.reserve(rangeCount);

            for (size_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
                HdBufferArrayRangeSharedPtr range =
                    bufferArray->GetRange(rangeIdx).lock();

                if (!range) continue; // shouldn't exist

                size_t numElements = range->GetNumElements();

                // numElements in each range should not exceed maxTotalElements
                if (!TF_VERIFY(numElements < maxTotalElements,
                                  "%lu >= %lu", numElements, maxTotalElements))
                    continue;

                // over aggregation check of non-uniform buffer
                if (numTotalElements + numElements > maxTotalElements) {
                    // create new BufferArray with same specification
                    HdBufferSpecVector bufferSpecs = 
                        strategy->GetBufferSpecs(bufferArray);
                    HdBufferArraySharedPtr newBufferArray =
                        strategy->CreateBufferArray(bufferArray->GetRole(),
                                                    bufferSpecs);
                    newBufferArray->Reallocate(ranges, bufferArray);

                    // bufferArrays is std::list
                    entry.second.bufferArrays.insert(bufferIt, newBufferArray);

                    numTotalElements = 0;
                    ranges.clear();
                }

                numTotalElements += numElements;
                ranges.push_back(range);
            }

            bufferArray->Reallocate(ranges, bufferArray);
        }
    }
}

void
HdBufferArrayRegistry::GarbageCollect()
{
    _BufferArrayIndex::iterator entryIt = _entries.begin();

    while (entryIt != _entries.end()) {
        _Entry &entry = entryIt->second;

        _HdBufferArraySharedPtrList::iterator bufferIt = entry.bufferArrays.begin();

        while (bufferIt != entry.bufferArrays.end()) {
            if ((*bufferIt)->GarbageCollect()) {
                bufferIt = entry.bufferArrays.erase(bufferIt);
            } else {
                ++bufferIt;
            }
        }

        if (entry.bufferArrays.empty()) {
            entryIt = _entries.unsafe_erase(entryIt);
        } else {
            ++entryIt;
        }
    }
}

size_t
HdBufferArrayRegistry::GetResourceAllocation(HdAggregationStrategy *strategy,
                                             VtDictionary &result) const
{
    size_t gpuMemoryUsed = 0;
    TF_FOR_ALL (entryIt, _entries) {
        TF_FOR_ALL(bufferIt, entryIt->second.bufferArrays) {
            gpuMemoryUsed += strategy->GetResourceAllocation(
                                                *bufferIt, result);
        }
    }

    return gpuMemoryUsed;
}

void
HdBufferArrayRegistry::_InsertNewBufferArray(_Entry &entry,
                                             const HdBufferArraySharedPtr &expectedTail,
                                             HdAggregationStrategy *strategy,
                                             TfToken const &role,
                                             HdBufferSpecVector const &bufferSpecs)
{
    {
        std::lock_guard<std::mutex> lock(entry.lock);

        // Check state of list, still matches what is expected.
        // If not another thread won and inserted a new buffer.
        if (!entry.bufferArrays.empty()) {
            if (entry.bufferArrays.back() != expectedTail) {
                return;  // Lock_guard will unlock entry
            }
        } else {
            // This shouldn't ever happen, because where did the expected tail
            // come from if it wasn't in the list???
            TF_VERIFY(!expectedTail);
        }

        entry.bufferArrays.emplace_back(strategy->CreateBufferArray(role, bufferSpecs));
    }  // Lock_guard will unlock

    // Notify any threads waiting on an empty list (unlock must happen first).
    entry.emptyCondition.notify_all();
}


HD_API
std::ostream &
operator <<(std::ostream &out, const HdBufferArrayRegistry& self)
{
    out << "HdBufferArrayRegistry " << &self << " :\n";
    TF_FOR_ALL (entryIt, self._entries) {
        out << "  _Entry aggrId = " << entryIt->first << ": \n";
        
        size_t bufferNum = 0;
        TF_FOR_ALL(bufferIt, entryIt->second.bufferArrays) {
            out << "HdBufferArray " << bufferNum << "\n";
        }
    }


    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE

