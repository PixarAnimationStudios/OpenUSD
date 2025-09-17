//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_COMPRESSED_H
#define PXR_EXEC_VDF_VECTOR_IMPL_COMPRESSED_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/vectorDataTyped.h"
#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/compressedIndexMapping.h"
#include "pxr/exec/vdf/forEachCommonType.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vectorImpl_Dispatch.h"
#include "pxr/exec/vdf/vectorImpl_Boxed.h"

#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Implements a Vdf_VectorData storage that is holds a subset of a
///        vector. The subset is determined by a supplied VdfMask. Each bit
///        in the mask maps to an element in the given vector.  The storage
///        omits all elements associated with zeros in the mask.
///
/// See vdf/compressedIndexMapping.h for details on the mapping implementation.
///
template<typename TYPE>
class VDF_API_TYPE Vdf_VectorImplCompressed final
    : public Vdf_VectorDataTyped<TYPE>
{
    static_assert(
        !Vdf_IsBoxedContainer<TYPE>,
        "Only Vdf_VectorImplBoxed may hold boxed values");

public :

    /// Construct storage for the elements of \p data indicated by \p bits.
    Vdf_VectorImplCompressed(const TYPE *data, const VdfMask::Bits &bits) :
        _compressedIndexMapping(new Vdf_CompressedIndexMapping())
    {
        _AllocateSpace(bits);
        _Initialize(data, bits);
    }

    /// Construct enough storage to hold as many elements as \p bits has set.
    explicit Vdf_VectorImplCompressed(const VdfMask::Bits &bits) :
        _compressedIndexMapping(new Vdf_CompressedIndexMapping())
    {
        _AllocateSpace(bits);
        _Initialize(NULL, bits);
    }

    Vdf_VectorImplCompressed(const Vdf_VectorImplCompressed &rhs);

    // Move ctor
    Vdf_VectorImplCompressed(Vdf_VectorImplCompressed &&sourceData) :
        _data(sourceData._data),
        _logicalSize(sourceData._logicalSize),
        _compressedIndexMapping(sourceData._compressedIndexMapping)
    {
        sourceData._data = NULL;
        sourceData._logicalSize = 0;
        sourceData._compressedIndexMapping = NULL;
    }

    ~Vdf_VectorImplCompressed() override
    {
        if (_data)
            delete [] _data;

        if (_compressedIndexMapping)
            delete _compressedIndexMapping;
    }

    void MoveInto(Vdf_VectorData::DataHolder *destData) override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplCompressed >(std::move(*this));
    }

    void Clone(Vdf_VectorData::DataHolder *destData) const override
    {
        TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
        destData->Destroy();
        destData->New< Vdf_VectorImplCompressed >(*this);
    }

    /// Assigns the subset of \p data that is described by \p mask into this
    /// sparse vector.
    ///
    void Assign(const TYPE *data, const VdfMask &mask)
    {
        // If our current data isn't the exact right size to store the data
        // we are supposed to copy, delete our array and 'new' one of the
        // appropriate size.
        if (GetNumStoredElements() != mask.GetNumSet()) {
            delete [] _data;
            _data = new TYPE[mask.GetNumSet()];
        }

        _Initialize(data, mask.GetBits());
    }

    void CloneSubset(const VdfMask &mask,
                     Vdf_VectorData::DataHolder *destData) const override;

    void Box(const VdfMask::Bits &bits,
             Vdf_VectorData::DataHolder *destData) const override;

    void Merge(const VdfMask::Bits &bits,
               Vdf_VectorData::DataHolder *destData) const override;

    size_t GetSize() const override
    {
        return _logicalSize;
    }

    size_t GetNumStoredElements() const override
    {
        if (_compressedIndexMapping->_blockMappings.size() > 0) {
            return _compressedIndexMapping->_blockMappings.back().dataEndIndex;
        } else {
            return 0;
        }
    }

    Vdf_VectorData::Info GetInfo() override
    {
        return Vdf_VectorData::Info(
            /* data = */ _data,
            /* size = */ _logicalSize,
            /* first = */ _compressedIndexMapping->GetFirstIndex(),
            /* last = */ _compressedIndexMapping->GetLastIndex(),
            /* compressedIndexMapping = */ _compressedIndexMapping);
    }

private :

    /// Constructs a compressed vector containing the elements from \p data that
    /// are specified by \p mask.
    ///
    /// Note that the block of memory allocated by the sparse vector is 
    /// contiguous even if the mask contains holes.  Only the elements
    /// specified by the mask will be stored, and all other elements will be
    /// omitted.
    ///
    /// If \p data is NULL, the compressed vector is left uninitialized.
    ///
    void _Initialize(const TYPE *data, const VdfMask::Bits &bits);

    void _AllocateSpace(const VdfMask::Bits &bits)
    {
        _data = new TYPE[bits.GetNumSet()];
    }

private:

    TYPE *_data;
    size_t _logicalSize;
    Vdf_CompressedIndexMapping *_compressedIndexMapping;
};

////////////////////////////////////////////////////////////////////////////////

template<typename TYPE>
Vdf_VectorImplCompressed<TYPE>::Vdf_VectorImplCompressed(
    const Vdf_VectorImplCompressed &rhs) :
    _logicalSize(rhs._logicalSize),
    _compressedIndexMapping(new Vdf_CompressedIndexMapping(
        *rhs._compressedIndexMapping))
{
    if (rhs._data) {
        const size_t numUsed = rhs.GetNumStoredElements();

        _data = new TYPE[numUsed];
        TF_DEV_AXIOM(_data);
        Vdf_VectorImplDispatch<TYPE>::Copy(_data, rhs._data, numUsed);

    } else {

        _data = NULL;

    }

}

template<typename TYPE>
void
Vdf_VectorImplCompressed<TYPE>::CloneSubset(
    const VdfMask &mask, Vdf_VectorData::DataHolder *destData) const
{
    TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
    TF_DEV_AXIOM(_logicalSize == mask.GetSize());
    const VdfMask::Bits &bits = mask.GetBits();

    // Allocate space and build an index mapping, leaving data uninitialized.
    // This is a little more than necessary but very easy to accomplish.
    destData->Destroy();
    destData->New< Vdf_VectorImplCompressed >(bits);
    Vdf_VectorImplCompressed * destImpl =
        dynamic_cast<Vdf_VectorImplCompressed *>(destData->Get());

    // Copy the relevant data from me into the destination.  The mapping is
    // conveniently already set up.
    size_t srcBlockHint = 0;
    size_t destBlockHint = 0;
    using View = VdfMask::Bits::PlatformsView;
    View platforms = bits.GetPlatformsView();
    for (View::const_iterator it=platforms.begin(), e=platforms.end(); it != e;
         ++it) {
        if (it.IsSet()) {
            const size_t index = *it;
            const size_t srcDataIdx =
                _compressedIndexMapping->FindDataIndex(index, &srcBlockHint);
            const size_t destDataIdx = destImpl->
                _compressedIndexMapping->FindDataIndex(index, &destBlockHint);
            Vdf_VectorImplDispatch<TYPE>::Copy(
                destImpl->_data + destDataIdx, _data + srcDataIdx,
                it.GetPlatformSize());
        }
    }
}

template<typename TYPE>
void
Vdf_VectorImplCompressed<TYPE>::Box(
    const VdfMask::Bits &bits, Vdf_VectorData::DataHolder *destData) const
{
    TfAutoMallocTag tag("Vdf", __ARCH_PRETTY_FUNCTION__);
    TF_VERIFY(bits.GetFirstSet() >= _compressedIndexMapping->GetFirstIndex());
    TF_VERIFY(bits.GetLastSet() <= _compressedIndexMapping->GetLastIndex());

    Vdf_BoxedContainer<TYPE> v(bits.GetNumSet());

    size_t i = 0;
    size_t blockHint = 0;
    for (const uint32_t idx : bits.GetAllSetView()) {
        const size_t dataIdx =
            _compressedIndexMapping->FindDataIndex(idx, &blockHint);
        v[i++] = _data[dataIdx];
    }

    destData->Destroy();
    destData->New< Vdf_VectorImplBoxed<TYPE> >(std::move(v));
}

template<typename TYPE>
void
Vdf_VectorImplCompressed<TYPE>::Merge(
    const VdfMask::Bits &bits, Vdf_VectorData::DataHolder *destData) const
{
    // Retrieve the destination information
    Vdf_VectorData::Info info = destData->Get()->GetInfo();

    // The destination must be a dense vector
    if (!TF_VERIFY(
            info.size > 1 &&
            info.compressedIndexMapping == NULL &&
            info.data, "destData is not a Vdf_VectorImplDense.")) {
        return;
    }
        
    // Merge the requested data into the destination vector (must be
    // sparse or dense)
    size_t srcBlockHint = 0;
    TYPE *typedDest = static_cast<TYPE*>(info.data);

    // Copy in chunks.
    using View = VdfMask::Bits::PlatformsView;
    View platforms = bits.GetPlatformsView();
    for (View::const_iterator it=platforms.begin(), e=platforms.end(); it != e;
         ++it) {
        if (it.IsSet()) {
            const size_t index = *it;
            const size_t srcDataIdx =
                _compressedIndexMapping->FindDataIndex(index, &srcBlockHint);
            Vdf_VectorImplDispatch<TYPE>::Copy(
                typedDest + index - info.first, _data + srcDataIdx,
                it.GetPlatformSize());
        }
    }
}

template<typename TYPE>
void
Vdf_VectorImplCompressed<TYPE>::_Initialize(
    const TYPE *srcData, const VdfMask::Bits &bits)
{
    // Note the logical size of the vector is not the same
    // as how much data it stores.
    _logicalSize  = bits.GetSize();
    const size_t newStorageSize = bits.GetNumSet();

    // Check to make sure that we have elements to copy.
    if (newStorageSize == 0) {
        // We have nothing to store.
        return;
    }

    // Compute the compressed index mapping, which is only
    // dependent on the layout of the set bits, and
    // provides a mapping from logical indices to
    // packed stored data indices in _data.  This is
    // the key to compressed vector memory savings.
    _compressedIndexMapping->Initialize(bits);

    // If specified, initialize this vector's data, using the
    // compressed index mapping. Copy from the srcData in chunks
    // as specified by the mask.
    if (srcData) {
        size_t destDataIdx = 0;
        using View = VdfMask::Bits::PlatformsView;
        View platforms = bits.GetPlatformsView();
        for (View::const_iterator it=platforms.begin(), e=platforms.end();
             it != e; ++it) {
            if (it.IsSet()) {
                const size_t platformSize = it.GetPlatformSize();
                Vdf_VectorImplDispatch<TYPE>::Copy(
                    _data + destDataIdx, srcData + *it, platformSize);
                destDataIdx += platformSize;
            }
        }
    }    

    // A few sanity check axioms.
    TF_DEV_AXIOM(_logicalSize > 0);
    TF_DEV_AXIOM(_data);
}

#if !defined(ARCH_OS_WINDOWS)
#define VDF_DECLARE_EXTERN_VECTOR_IMPL_COMPRESSED(type)         \
    extern template class VDF_API_TYPE Vdf_VectorImplCompressed<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_DECLARE_EXTERN_VECTOR_IMPL_COMPRESSED)
#undef VDF_DECLARE_EXTERN_VECTOR_IMPL_COMPRESSED
#endif // !defined(ARCH_OS_WINDOWS)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
