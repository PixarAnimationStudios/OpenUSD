//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vector.h"

PXR_NAMESPACE_OPEN_SCOPE

static_assert(
    sizeof(VdfVector::ReadWriteAccessor<int>) == sizeof(Vdf_VectorAccessor<int>),
    "VdfVector::ReadWriteAccessor should be composed of a Vdf_VectorAccessor only.");

static_assert(
    sizeof(VdfVector::ReadAccessor<int>) == sizeof(Vdf_VectorAccessor<int>),
    "VdfVector::ReadAccessor should be composed of a Vdf_VectorAccessor only.");

// Ostream operator for printing selected elements in a vector.
std::ostream &operator<<(std::ostream &o, const VdfVector::DebugPrintable &v)
{
    v._data->DebugPrint(v._mask, &o);
    return o;
}

void
VdfVector::Merge(const VdfVector &rhs, const VdfMask::Bits &bits)
{
    _CheckType(rhs);

    // Can't do a self-merge
    if (&rhs == this) {
        TF_CODING_ERROR("cannot self-Merge() (this == &rhs)");
        return;
    }

    const size_t size = GetSize();

    // Bail out if there is nothing to merge from rhs
    if (rhs.IsEmpty() || bits.AreAllUnset())
        return;

    // Vector sizes must be equal to the bits size
    if (size != rhs.GetSize()) {
        TF_CODING_ERROR(
            "size mismatch: this->GetSize() (%zu) != rhs.GetSize() (%zu)",
            size, rhs.GetSize());
        return;
    }
    if (size != bits.GetSize()) {
        TF_CODING_ERROR(
            "size mismatch: this->GetSize() (%zu) != bits.GetSize() (%zu)",
            size, bits.GetSize());
        return;
    }

    // Collect the destination vector info
    Vdf_VectorData::Info info = _data.Get()->GetInfo();

    // Need to detach before mutating the data if shared.
    if (ARCH_UNLIKELY(info.ownership == 
            Vdf_VectorData::Info::Ownership::Shared)) {
        Vdf_VectorImplShared::Detach(&_data);

        // Update the info after detaching.
        info = _data.Get()->GetInfo();
    }

    // Deal with sparse and compressed vectors
    if (GetNumStoredElements() < size) {
        const size_t newFirst =
            std::min<size_t>(bits.GetFirstSet(), info.first);
        const size_t newLast =
            std::max<size_t>(bits.GetLastSet(), info.last);

        // Uncompress, if necessary. This can become expensive, so we
        // uncompress the vector once, i.e. all subsequent Merge operations
        // will target a now sparse vector. 
        if (info.compressedIndexMapping) {
            VdfMask::Bits storedBits;
            info.compressedIndexMapping->ComputeStoredBits(
                &storedBits, info.size);

            Vdf_VectorData::DataHolder tmp;
            _data.Get()->NewSparse(info.size, newFirst, newLast, &tmp);
            _data.Get()->Merge(storedBits, &tmp);
            tmp.Get()->MoveInto(&_data);
            tmp.Destroy();
        }

        // Make sure the storage space in the destination vector is
        // sufficiently large.
        else if (newFirst < info.first || newLast > info.last) {
            _data.Get()->Expand(newFirst, newLast);
        }
    }

    // Merge from the rhs implementation to _data
    rhs._data.Get()->Merge(bits, &_data);
}

bool
VdfVector::_ComputeCompressedExtractionIndex(
    const Vdf_CompressedIndexMapping &indexMapping,
    size_t size,
    int offset,
    size_t *dataIdx)
{
    if (!TF_VERIFY(dataIdx)) {
        return false;
    }

    size_t block = 0;
    *dataIdx = indexMapping.FindDataIndex(offset, &block);
    const size_t blockStart = indexMapping.GetBlockFirstIndex(block);
    const size_t blockLen = indexMapping.GetBlockLength(block);

    return TF_VERIFY((offset + size) <= (blockStart + blockLen),
                     "Extraction range (idx=%d, len=%zu) outside "
                     "block %zu range (idx=%zu, len=%zu)",
                     offset, size, block, blockStart, blockLen);
}

void
VdfVector::_PostTypeError(
    const std::type_info &typeInfo,
    const std::type_info &otherTypeInfo)
{
    // CODE_COVERAGE_OFF - should not get here
    TF_CODING_ERROR(
        "Invalid type.  Vector is holding " +
        ArchGetDemangled(typeInfo) +
        ", tried to use as " +
        ArchGetDemangled(otherTypeInfo));
    // CODE_COVERAGE_ON
}

PXR_NAMESPACE_CLOSE_SCOPE
