//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/compressedIndexMapping.h"

#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/tf.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

void
Vdf_CompressedIndexMapping::Initialize(const VdfMask::Bits &bits)
{
    if (!TF_VERIFY(bits.GetFirstSet() < bits.GetSize())) {
        return;
    }

    // Resize the compressed index mapping here to avoid incrementally
    // resizing it later at greater cost.
    _blockMappings.resize(bits.GetNumSetPlatforms());

    // Compute the contents of the mapping blocks.
    // The general idea is to iterate over only the set indices in
    // bits, and if we notice a gap in the indices, record
    // what we've seen since the last gap as a block mapping.
    size_t currentBlockIdx = 0;
    size_t currentDataIdx = 0;
    using View = VdfMask::Bits::PlatformsView;
    View platforms = bits.GetPlatformsView();
    for (View::const_iterator it=platforms.begin(), e=platforms.end();
         it != e; ++it) {
        // Skip unset platforms
        if (!it.IsSet()) {
            continue;
        }

        // Set the logical start index to the first bit set in the platform
        _blockMappings[currentBlockIdx].logicalStartIndex = *it;

        // Increment our count of the total number of elements
        currentDataIdx += it.GetPlatformSize();
        _blockMappings[currentBlockIdx].dataEndIndex = currentDataIdx;

        // Proceed to the next platform
        ++currentBlockIdx;
    }

    // Some sanity check axioms on the structure of the mapping 
    // constructed above.
    if (TF_DEV_BUILD) {
        // Check that logical and storage indices are in expected ranges.
        for (size_t i=0; i<_blockMappings.size(); i++) {
            size_t lsi = _blockMappings[i].logicalStartIndex;
            TF_DEV_AXIOM(lsi >= bits.GetFirstSet());
            TF_DEV_AXIOM(lsi <= bits.GetLastSet());

            size_t dei = _blockMappings[i].dataEndIndex;
            TF_DEV_AXIOM(dei > 0);
            TF_DEV_AXIOM(dei <= bits.GetNumSet());
        }
    }
}

size_t
Vdf_CompressedIndexMapping::FindDataIndex(
    size_t logicalIdx, size_t *blockHint)const
{

    size_t blockIdx, dataIdx;

    // Try the block given in blockHint, to avoid a costlier call to
    // FindBlockIndex.
    blockIdx = *blockHint;
    if (ComputeDataIndex(blockIdx, logicalIdx, &dataIdx)) {
        return dataIdx;
    }

    // The hint failed, but hey, there's a very good chance the caller is
    // just iterating monotonically through the indices, so we might as well
    // try the *next* block, if there is one.
    if (blockIdx + 1 < _blockMappings.size()) {
        if (ComputeDataIndex(blockIdx + 1, logicalIdx, &dataIdx)) {
            *blockHint = blockIdx + 1;
            return dataIdx;
        }
    }

    // The blockHint block didn't contain the given index, so search for
    // the right block using a binary search.
    *blockHint = blockIdx = FindBlockIndex(logicalIdx);

    TF_VERIFY(ComputeDataIndex(blockIdx, logicalIdx, &dataIdx));

    return dataIdx;
}

size_t
Vdf_CompressedIndexMapping::FindBlockIndex(size_t logicalIdx) const
{

    TF_DEV_AXIOM(!_blockMappings.empty());

    // Search for the matching block using upper_bound.
    // insertIt will be the greatest block iter where we could
    // insert the query without violating the order.
    Vdf_IndexBlockMapping query;
    query.logicalStartIndex = logicalIdx;
    _BlockMappings::const_iterator insertIt =
       std::upper_bound(_blockMappings.begin(),
       _blockMappings.end(), query);

    // Compute the containing block's index.
    return insertIt - _blockMappings.begin() - 1;
}


bool
Vdf_CompressedIndexMapping::ComputeDataIndex(
    size_t blockIdx, size_t logicalIdx, size_t *dataIdxRet) const
{

    size_t blockStartIdx = GetBlockFirstIndex(blockIdx);
    size_t dataStartIdx = (blockIdx == 0) ? 0
        : _blockMappings[blockIdx-1].dataEndIndex;
    size_t dataEndIdx = _blockMappings[blockIdx].dataEndIndex;
    size_t dataIdx = dataStartIdx + logicalIdx - blockStartIdx;

    if (dataIdx >= dataStartIdx && dataIdx < dataEndIdx) {
        *dataIdxRet = dataIdx;
        return true;
    }

    return false;
   
}

size_t
Vdf_CompressedIndexMapping::GetBlockFirstIndex(size_t blockIdx) const
{
    return _blockMappings[blockIdx].logicalStartIndex;
}

size_t
Vdf_CompressedIndexMapping::GetBlockLastIndex(size_t blockIdx) const
{
    return 
        // The logical start of the block
        GetBlockFirstIndex(blockIdx)
        // Plus the length of the block
      + GetBlockLength(blockIdx)
        // Minus one, to return the logical index of the last element in
        // the block.
      - 1;
}

size_t
Vdf_CompressedIndexMapping::GetBlockLength(size_t blockIdx) const
{
    return _blockMappings[blockIdx].dataEndIndex
         - (blockIdx == 0 ? 0 : _blockMappings[blockIdx-1].dataEndIndex);
}


void 
Vdf_CompressedIndexMapping::FindBlockRange(
    size_t first, size_t last,
    size_t *firstBlockIdx, size_t *lastBlockIdx) const
{

    // Search for the matching block using upper_bound.
    // insertIt will be the greatest block iter where we could
    // insert the query without violating the order.

    Vdf_IndexBlockMapping query;

    query.logicalStartIndex = first;
    _BlockMappings::const_iterator insertIt =
       std::upper_bound(_blockMappings.begin(),
       _blockMappings.end(), query);
    // The result of upper_bound could be either the block we want
    // or the one right after it, so check to make sure the
    // block before doesn't contain "first", and if it does,
    // it's the block we want.
    *firstBlockIdx = insertIt - _blockMappings.begin() - 1;
    if (first > GetBlockLastIndex(*firstBlockIdx)) {
        (*firstBlockIdx)++;
    }

    query.logicalStartIndex = last;
    insertIt = std::upper_bound(_blockMappings.begin(),
       _blockMappings.end(), query);
    // The result of upper_bound is always the correct
    // last block that intersects the range.
    *lastBlockIdx = insertIt - _blockMappings.begin() - 1;
}

void
Vdf_CompressedIndexMapping::ComputeStoredBits(
    VdfMask::Bits *bits, size_t num) const
{
    // XXX: Computing the mask from the index mapping can be pretty slow. If
    //      this turns out to be a performance hotspot, we can store the mask
    //      locally, since the bits are already being passed to the Initialize
    //      method.
    //      The only problem with this approach is that we have to make sure
    //      that the mask stays up-to-day, even after operations like
    //      Slice or CopySubset on VdfVectorImpl_Compressed!
    //
    size_t lastIndex = 0;
    for (size_t b = 0; b < _blockMappings.size(); ++b) {
        const size_t first = GetBlockFirstIndex(b);
        const size_t len = GetBlockLength(b);
        bits->Append(first - lastIndex, false);
        bits->Append(len, true);
        lastIndex = first + len;
    }
    bits->Append(num - lastIndex, false);
}

PXR_NAMESPACE_CLOSE_SCOPE
