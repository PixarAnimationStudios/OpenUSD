//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_COMPRESSED_INDEX_MAPPING_H
#define PXR_EXEC_VDF_COMPRESSED_INDEX_MAPPING_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"

#include <cstddef>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// A mapping that relates logical blocks of indices to actual stored data
/// when the data is compressed by eliding unset elements.
/// See Vdf_CompressedIndexMapping.
///
struct Vdf_IndexBlockMapping {
    // The first logical index of the block of contiguous elements.
    //
    size_t logicalStartIndex;

    // The index into packed raw storage of the element AFTER
    // the final element in this block of logically contiguous elements.
    //
    size_t dataEndIndex;

    // A less-than operator for performing binary search on a
    // vector of these objects.  It only considers the start index
    // becase that is sufficient to order the blocks.
    //
    bool operator<(const Vdf_IndexBlockMapping& rhs) const {
        return logicalStartIndex < rhs.logicalStartIndex;
    }
};

/// This collection of IndexBlockMappings is all the info required to
/// take a logical index into a compressed VdfVector and obtain a
/// raw index into packed storage.
///
/// An example vector with letters representing significant values:
///  logical vector:           [ A BC  DE ]
///  logical indices:           0123456789
///
///  raw storage:              [ABCDE]
///
///  Block mappings: (1,1), (3,3), (7,5)
///
/// The third block mapping (7,5) says the third contiguous block of data
/// (D,E) begins in the logical vector at logical index 7, and that in the
/// raw storage, it ends before raw index 5.  To find the raw storage index
/// corresponding to logical index 7, just look at the previous mapping's
/// end index and find it's stored at raw index 3.
///
/// Using this scheme, the total number of stored elements is available as
/// the end index of the last block mapping.
///
class Vdf_CompressedIndexMapping {
public:

    // Computes a mapping with block layout that matches the
    // bits set in the given VdfMask::Bits.
    VDF_API
    void Initialize(const VdfMask::Bits &bits);

    // Finds the raw data index corresponding to the given
    // logical element index.  The block specified by blockHint
    // is checked first as an optimization.  The containing
    // block is also returned in blockHint.
    //
    VDF_API
    size_t FindDataIndex(size_t logicalIdx, size_t *blockHint) const;

    // Returns the index of the block containing the given
    // logical element index.
    VDF_API
    size_t FindBlockIndex(size_t logicalIdx) const;

    // Returns whether logicalIndex is in the range of the given block.
    // if so, returns the data index also.
    VDF_API
    bool ComputeDataIndex(size_t blockIndex, size_t logicalIndex,
                          size_t *dataIndex) const;


    // Returns the first logical index mapped by the given block.
    VDF_API
    size_t GetBlockFirstIndex(size_t blockIdx) const;

    // Returns the last logical index mapped by the given block.
    VDF_API
    size_t GetBlockLastIndex(size_t blockIdx) const;

    VDF_API
    size_t GetBlockLength(size_t blockIdx) const;

    // Returns the range of blocks that intersect the given
    // logical index range.
    VDF_API
    void FindBlockRange(size_t first, size_t last,
        size_t *firstBlockIdx, size_t *lastBlockIdx) const;

    // Returns the first logical index in the entire mapping
    size_t GetFirstIndex() const {
        return GetBlockFirstIndex(0);
    }

    // Returns the last logical index in the entire mapping
    size_t GetLastIndex() const {
        return GetBlockLastIndex(_blockMappings.size() - 1);
    }

    // Computes a mask with bits turned on for each index contained in the
    // compressed index mapping
    VDF_API
    void ComputeStoredBits(VdfMask::Bits *bits, size_t num) const;

private:

    template <class T> friend class Vdf_VectorImplCompressed;

    typedef std::vector<Vdf_IndexBlockMapping> _BlockMappings;
    _BlockMappings _blockMappings;
   
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
