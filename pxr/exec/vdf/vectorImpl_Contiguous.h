//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_CONTIGUOUS_H
#define PXR_EXEC_VDF_VECTOR_IMPL_CONTIGUOUS_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vectorDataTyped.h"
#include "pxr/exec/vdf/vectorImpl_Boxed.h"
#include "pxr/exec/vdf/vectorImpl_Compressed.h"
#include "pxr/exec/vdf/vectorImpl_Dispatch.h"

#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Implements Vdf_VectorData storage that holds a contiguous range of
/// elements, which may be a subrange of the logical vector size.
///
template <typename TYPE>
class VDF_API_TYPE Vdf_VectorImplContiguous final
    : public Vdf_VectorDataTyped<TYPE>
{
    static_assert(
        !Vdf_IsBoxedContainer<TYPE>,
        "Only Vdf_VectorImplBoxed may hold boxed values");

public:

    /// Dense vector constructor.
    ///
    /// Constructs a vector of whose size is \p size.  Storage is allocated
    /// for the range [0, size-1].
    ///
    explicit Vdf_VectorImplContiguous(size_t size)
        : _data(size ? new TYPE[size] : nullptr)
        , _size(size)
        , _first(0)
        , _last(size ? size-1 : 0)
    {
    }

    /// Sparse vector constructor.
    ///
    /// Constructs a vector whose size is \p size.  Storage is allocated for
    /// the range [first, last].
    ///
    Vdf_VectorImplContiguous(size_t size, size_t first, size_t last)
    {
        _AllocateSpace(size, first, last);
    }

    /// Sparse vector constructor.
    ///
    /// Constructs a vector whose size is the width of \p bits.  Storage is
    /// allocated for the range [bits.GetFirstSet(), bits.GetLastSet()].
    ///
    explicit Vdf_VectorImplContiguous(const VdfMask::Bits &bits)
    {
        _AllocateSpace(bits);
    }

    /// Sparse vector constructor.
    ///
    /// Constructs a vector whose size is \p size.  Storage is allocated for
    /// the range [bits.GetFirstSet(), bits.GetLastSet()].  Elements are
    /// copied from \p data for each set bit in \p bits.
    ///
    /// Note that the block of memory allocated is contiguous even if the mask
    /// contains holes.  Only the elements specified by the mask will be
    /// copied, all other elements will remain uninitialized.
    ///
    Vdf_VectorImplContiguous(const TYPE *data, const VdfMask::Bits &bits)
    {
        _AllocateSpace(bits);

        if (_data) {
            // We can allow to copy only elements that are needed, since _data[]
            // is default initialized.
            const size_t firstSet = bits.GetFirstSet();
            Vdf_VectorImplDispatch<TYPE>::Copy(_data - firstSet, data, bits);
        }
    }

    Vdf_VectorImplContiguous(const Vdf_VectorImplContiguous &rhs) :
        _size(rhs._size),
        _first(rhs._first),
        _last(rhs._last)
    {
        if (rhs._data) {
            const size_t numUsed = _last - _first + 1;
            _data = new TYPE[numUsed];
            TF_DEV_AXIOM(_data);

            Vdf_VectorImplDispatch<TYPE>::Copy(_data, rhs._data, numUsed);
        } else {
            _data = NULL;
        }
    }

    Vdf_VectorImplContiguous(Vdf_VectorImplContiguous &&sourceData) :
        _data(sourceData._data),
        _size(sourceData._size),
        _first(sourceData._first),
        _last(sourceData._last)
    {
        sourceData._data = NULL;
        sourceData._size = 0;
        sourceData._first = 0;
        sourceData._last = 0;
    }

    ~Vdf_VectorImplContiguous() override
    {
        delete [] _data;
    }

    void MoveInto(Vdf_VectorData::DataHolder *destData) override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplContiguous >(std::move(*this));
    }

    void Clone(Vdf_VectorData::DataHolder *destData) const override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplContiguous >(*this);
    }

    void CloneSubset(const VdfMask &mask,
                     Vdf_VectorData::DataHolder *destData) const override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        TF_DEV_AXIOM(_size == mask.GetSize());

        // We're a contiguous vector and there is potential for more subsetting.
        if (mask.IsAllZeros()) {

            // In this case we want to store an empty vector that indicates
            // that we have the correct size, but there are no stored values.
            destData->Destroy();
            destData->New< Vdf_VectorImplEmpty<TYPE> >(mask.GetSize());

        } else if (Vdf_VectorData::ShouldStoreCompressed(
                   mask.GetBits(), sizeof(TYPE)) ) {
            // Check to see if the destination data is already a compressed
            // vector.  If it is, we'll try to avoid reallocating by simply
            // copying a subset of our data into the existing compressed vector.
            // The allocation is avoided only if the mask numset is exactly
            // the same as the destination's raw storage size.
            Vdf_VectorImplCompressed<TYPE> *destCompressed = 
                dynamic_cast<Vdf_VectorImplCompressed<TYPE> *>(destData->Get());
            if (destCompressed) {
                destCompressed->Assign(&(_data[-_first]), mask);
            } else {
                destData->Destroy();
                destData->New< Vdf_VectorImplCompressed<TYPE> >(
                    &(_data[-_first]), mask.GetBits() );
            }

        } else {

            // Here's where we can create a subset vector.
            size_t firstSet = mask.GetFirstSet();
            size_t lastSet  = mask.GetLastSet();

            if (firstSet == _first && lastSet == _last) {

                // In this case we want all of this already subset vector.
                Clone(destData);

            } else {

                // Here we're splitting up the already contiguous vector a 
                // little more.
                destData->Destroy();
                destData->New< Vdf_VectorImplContiguous >(
                    _data - _first, mask.GetBits());
            }
        }
    }

    void Box(const VdfMask::Bits &bits,
             Vdf_VectorData::DataHolder *destData) const override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        TF_VERIFY(bits.GetFirstSet() >= _first);
        TF_VERIFY(bits.GetLastSet() <= _last);

        Vdf_BoxedContainer<TYPE> v(bits.GetNumSet());
        if (bits.AreContiguouslySet()) {
            const size_t offset = bits.GetFirstSet() - _first;
            for (size_t i = 0; i < v.size(); ++i) {
                v[i] = _data[offset + i];
            }
        } else {
            size_t i = 0;
            for (const uint32_t idx : bits.GetAllSetView()) {
                v[i++] = _data[idx - _first];
            }
        }

        destData->Destroy();
        destData->New< Vdf_VectorImplBoxed<TYPE> >(std::move(v));
    }

    void Merge(const VdfMask::Bits &bits,
               Vdf_VectorData::DataHolder *destData) const override
    {
        // Retrieve the destination information
        Vdf_VectorData::Info info = destData->Get()->GetInfo();

        // The destination must be a contiguous vector
        if (!TF_VERIFY(
                info.size > 1 &&
                info.compressedIndexMapping == NULL &&
                info.data, "destData is not a Vdf_VectorImplContiguous.")) {
            return;
        }
        
        // Merge the requested data into the destination vector
        TYPE *typedDest = static_cast<TYPE*>(info.data); 
        Vdf_VectorImplDispatch<TYPE>::Copy(
            typedDest - info.first, _data - _first, bits);
    }

    void Expand(size_t first, size_t last) override
    {
        // Make sure that the storage grows, but never shrinks. If the storage
        // is currently empty, use the passed in range.
        const size_t newFirst = _data ? std::min(_first, first) : first;
        const size_t newLast = _data ? std::max(_last, last) : last;

        // If the storage space is already big enough, there is nothing
        // to do here.
        if (_first == newFirst && _last == newLast) {
            return;
        }

        // Retain a pointer to the old data
        TYPE *oldData = _data;

        // Allocate a new data section
        const size_t newSize = newLast - newFirst + 1;
        _data = new TYPE[newSize];
        TF_DEV_AXIOM(_data);

        // Copy the old data, if available
        if (oldData) {
            Vdf_VectorImplDispatch<TYPE>::Copy(
                _data + _first - newFirst, oldData, _last - _first + 1);
            delete[] oldData;
        }

        // Set the new info
        _first = newFirst;
        _last = newLast;
    }

    size_t GetSize() const override
    {
        return _size;
    }

    size_t GetNumStoredElements() const override
    {
        if (!_data) {
            return 0;
        }
        return _last - _first + 1;
    }

    bool IsSharable() const override
    {
        return _size >= Vdf_VectorData::_VectorSharingSize;
    }

    Vdf_VectorData::Info GetInfo() override
    {
        return Vdf_VectorData::Info(
            /* data = */ _data,
            /* size = */ _size,
            /* first = */ _first,
            /* last = */ _last);
    }

private:

    void _AllocateSpace(const VdfMask::Bits &bits)
    {
        if (bits.AreAllUnset()) {
            _data = nullptr;
            _size = bits.GetSize();
            _first = 0;
            _last = 0;
        } else {
            _AllocateSpace(bits.GetSize(),
                           bits.GetFirstSet(),
                           bits.GetLastSet());
        }
    }

    void _AllocateSpace(size_t size, size_t firstIndex, size_t lastIndex) 
    {
        _size = size;

        _first = firstIndex;
        TF_DEV_AXIOM(_first < _size);

        _last = lastIndex;
        TF_DEV_AXIOM(_last >= _first);

        const size_t numUsed = _last - _first + 1;
        _data = new TYPE[numUsed];
        TF_DEV_AXIOM(_data);
    }

private:

    TYPE   *_data;
    size_t _size;
    size_t _first;
    size_t _last;
};

#define VDF_DECLARE_EXTERN_VECTOR_IMPL_CONTIGUOUS(type)         \
    extern template class VDF_API_TYPE Vdf_VectorImplContiguous<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_IMPL_CONTIGUOUS)
#undef VDF_DECLARE_EXTERN_VECTOR_IMPL_CONTIGUOUS

PXR_NAMESPACE_CLOSE_SCOPE

#endif
