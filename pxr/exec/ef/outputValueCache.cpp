//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/outputValueCache.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/exec/vdf/vector.h"

PXR_NAMESPACE_OPEN_SCOPE

Ef_OutputValueCache::Ef_OutputValueCache() :
    _oneOneMask(VdfMask::AllOnes(1))
{

}

Ef_OutputValueCache::~Ef_OutputValueCache()
{
    
}

bool
Ef_OutputValueCache::_ContainsOutput(const VdfOutput &output) const
{
    const VdfIndex outputIdx = VdfOutput::GetIndexFromId(output.GetId());
    return _outputSet.GetSize() > outputIdx && _outputSet.IsSet(outputIdx);
}

void
Ef_OutputValueCache::_AddOutput(const VdfOutput &output)
{
    // Make sure the bitset is sized sufficiently large to hold the
    // output index, and initialize any new entries with 0.
    const VdfIndex outputIdx = VdfOutput::GetIndexFromId(output.GetId());
    if (_outputSet.GetSize() <= outputIdx) {
        _outputSet.ResizeKeepContent(
            output.GetNode().GetNetwork().GetOutputCapacity());
    }

    _outputSet.Set(outputIdx);
}

void
Ef_OutputValueCache::_RemoveOutput(const VdfOutput &output)
{
    const VdfIndex outputIdx = VdfOutput::GetIndexFromId(output.GetId());
    _outputSet.Clear(outputIdx);
}

const VdfVector *
Ef_OutputValueCache::_GetValue(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    // Bail out if the output's node is not even referenced.
    if (!_ContainsOutput(output)) {
        return NULL;
    }

    _OutputsMap::const_iterator it = _outputs.find(&output);
    if (it != _outputs.end()) {
        return mask == _oneOneMask
            ? it->second.GetValue()
            : it->second.GetValue(mask);
    }

    return NULL;
}

size_t
Ef_OutputValueCache::_SetValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    // If this is a new output to be inserted into the map,
    // add a reference to the output's node .
    std::pair<_OutputsMap::iterator, bool> res =
        _outputs.insert(std::make_pair(&output, _Entry()));
    if (res.second) {
        _AddOutput(output);
    }

    // Merge in the value
    return res.first->second.SetValue(value, mask);
}

size_t
Ef_OutputValueCache::_Invalidate(const VdfOutput &output)
{
    // Bail out if the output's node is not even referenced.
    if (!_ContainsOutput(output)) {
        return 0;
    }

    _OutputsMap::iterator it = _outputs.find(&output);
    if (it != _outputs.end()) {
        // Invalidate the entry
        const size_t bytesInvalidated = it->second.Invalidate();

        // Remove the output entry
        _outputs.erase(it);
        _RemoveOutput(output);

        // Return the number of bytes invalidated
        return bytesInvalidated;
    }

    // Nothing invalidated
    return 0;
}

size_t
Ef_OutputValueCache::_Invalidate(const VdfMaskedOutputVector &outputs)
{
    size_t bytesInvalidated = 0;

    for (const VdfMaskedOutput &maskedOutput : outputs) {
        const VdfOutput &output = *maskedOutput.GetOutput();

        // Bail out if the output's node is not even referenced.
        if (!_ContainsOutput(output)) {
            continue;
        }

        _OutputsMap::iterator it = _outputs.find(&output);
        if (it != _outputs.end()) {
            // Invalidate the entry
            bytesInvalidated += it->second.Invalidate(maskedOutput.GetMask());

            // Remove the entry, if it has expired
            if (!it->second.GetValue()) {
                _outputs.erase(it);
                _RemoveOutput(output);
            }
        }
    }

    // Return the number of bytes invalidated
    return bytesInvalidated;
}

size_t
Ef_OutputValueCache::_Clear()
{
    TRACE_FUNCTION();

    // Compute the number of bytes cleared
    size_t bytesInvalidated = 0;
    TF_FOR_ALL (it, _outputs) {
        bytesInvalidated += it->second.GetNumBytes();
    }

    // Remove all entries from the map
    _outputs.clear();
    _outputSet.ClearAll();

    // Return the number of bytes free'd
    return bytesInvalidated;
}

bool
Ef_OutputValueCache::_IsEmpty() const
{
    return _outputs.empty();
}

bool
Ef_OutputValueCache::_IsUncached(
    const VdfRequest &request) const
{
    // If the request is empty there aren't any uncached values.
    if (request.IsEmpty()) {
        return false;
    }

    // If the cache is empty, everything in the request is uncached.
    if (_IsEmpty()) {
        return true;
    }

    TRACE_FUNCTION();
    
    // Find the first output that is not cached.    
    for (const VdfMaskedOutput &it : request) {
        if (!_GetValue(*it.GetOutput(), it.GetMask())) {
            return true;
        }
    }

    // Everything is cached.
    return false;
}

VdfRequest
Ef_OutputValueCache::_GetUncached(const VdfRequest &request) const
{
    TRACE_FUNCTION();

    // If the cache is empty, everything is uncached.
    if (_IsEmpty()) {
        return request;
    }

    // Find all uncached output, and return them with the result request.
    VdfRequest subRequest(request);
    for (VdfRequest::const_iterator it = request.begin(); 
         it != request.end();
         ++it) {
        if (_GetValue(*it->GetOutput(), it->GetMask())) {
            subRequest.Remove(it);
        }
    }

    return subRequest;
}

Ef_OutputValueCache::_Entry::_Entry() :
    _value(NULL)
{
}

Ef_OutputValueCache::_Entry::~_Entry()
{
    delete _value;
}

size_t
Ef_OutputValueCache::_Entry::GetNumBytes() const
{
    return _value ? _value->EstimateElementMemory() * _mask.GetSize() : 0;
}

const VdfVector *
Ef_OutputValueCache::_Entry::GetValue(const VdfMask &mask) const
{
    return _value && _mask.Contains(mask) ? _value : NULL;
}

size_t
Ef_OutputValueCache::_Entry::SetValue(
    const VdfVector &value,
    const VdfMask &mask)
{
    // If this entry already holds the exact same data, bail out right away.
    if (_value && mask == _mask) {
        return 0;
    }

    // This is an entirely new entry
    if (!_value) {
        // Create a new vector large enough for all the data that could possibly
        // be stored at this output (for thread safety.) Then, start off by
        // populating the new vector with the elements from value.
        _value = new VdfVector(value, mask.GetSize());
        _value->Merge(value, mask.GetBits());

        _mask = mask;

        // Return the number of bytes stored for this entry
        return _value->EstimateElementMemory() * mask.GetSize();
    }

    // Existing entry
    VdfMask::Bits uncachedBits = mask.GetBits() - _mask.GetBits();
    if (uncachedBits.IsAnySet()) {
        // Merge in the uncached data
        _value->Merge(value, uncachedBits);
        uncachedBits |= _mask.GetBits();
        _mask = VdfMask(uncachedBits);
    }

    // We did not allocate any additional memory.
    return 0;
}

size_t
Ef_OutputValueCache::_Entry::Invalidate()
{
    if (!_value) {
        return 0;
    }

    const size_t bytesInvalidated =
        _value->EstimateElementMemory() * _mask.GetSize();

    delete _value;
    _value = NULL;

    return bytesInvalidated;
}

size_t
Ef_OutputValueCache::_Entry::Invalidate(const VdfMask &mask)
{
    // If the invalidation mask is all-zeros, bail out.
    if (mask.IsAllZeros()) {
        return 0;
    }

    // If the invalidation mask is exactly equal to the stored mask, 
    // remove all data. No need to reset the stored mask, because the entry
    // will be removed from the output map.
    if (mask == _mask || mask.IsAllOnes()) {
        return Invalidate();
    }

    // Compute the new mask with the invalid entries removed
    VdfMask::Bits newBits = _mask.GetBits() - mask.GetBits();
    const size_t elementsInvalid = _mask.GetNumSet() - newBits.GetNumSet();

    // If nothing has been invalidated, bail out.
    if (elementsInvalid == 0) {
        return 0;
    }

    // Note, that for performance reasons, invalidation simply removes the
    // invalid bits from the stored mask, instead of actually freeing any
    // memory (unless the new mask is now all zeros).

    // Delete the value, if necessary
    if (newBits.AreAllUnset()) {
        return Invalidate();
    }

    // Otherwise, just update the current mask.
    else {
        _mask = VdfMask(newBits);
    }

    // We did not free any memory.
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
