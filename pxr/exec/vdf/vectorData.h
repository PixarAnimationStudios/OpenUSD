//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_DATA_H
#define PXR_EXEC_VDF_VECTOR_DATA_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/fixedSizePolymorphicHolder.h"
#include "pxr/exec/vdf/mask.h"

#include <iosfwd>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

class Vdf_CompressedIndexMapping;
class Vt_ArrayForeignDataSource;

/// \brief Abstract base class for storing data in a VdfVector.
///
class VDF_API_TYPE Vdf_VectorData
{
protected:

    // The smallest buffer size we want is the size of our biggest 
    // subclass, which happens to be Vdf_VectorImplContiguous which is made up
    // of 3 size_ts and a pointer.  Note that if this ever changes we will get
    // a compilation error (unless it gets smaller in which case we would
    // have to realize that independently and adjust accordingly to maximize
    // storage).
    static const size_t _SmallBufferSize = sizeof(size_t)*3 + sizeof(void *);

    // The holder also needs space for the vtable pointer.
    static const size_t _DataHolderSize = _SmallBufferSize + sizeof(void*);

    // The size a vector needs to be to enable sharing. Used in IsSharable().
    static constexpr size_t _VectorSharingSize = 5000;

    Vdf_VectorData() = default;

public:

    typedef Vdf_FixedSizePolymorphicHolder<Vdf_VectorData, _DataHolderSize> 
        DataHolder;

    Vdf_VectorData(const Vdf_VectorData &) = delete;
    Vdf_VectorData& operator=(const Vdf_VectorData &) = delete;

    Vdf_VectorData(Vdf_VectorData &&) = delete;
    Vdf_VectorData& operator=(Vdf_VectorData &&) = delete;

    VDF_API
    virtual ~Vdf_VectorData();

    // Returns the type_info for this data.
    //
    virtual const std::type_info &GetTypeInfo() const = 0;

    // Sets \p destData to an empty data of with this data's type.
    //
    // Note that \p destData must have never been initialized or freed
    // before this call.
    //
    virtual void NewEmpty(size_t size, DataHolder *destData) const = 0;

    // Sets \p destData to a single element vector of this data's type.
    //
    // Note that \p destData must have never been initialized or freed
    // before this call.
    //
    virtual void NewSingle(DataHolder *destData) const = 0;

    // Sets \p destData to a sparse vector of this data's type.
    //
    // Note that \p destData must have never been initialized or freed
    // before this call.
    // 
    virtual void NewSparse(size_t size, size_t first, size_t last,
                           DataHolder *destData) const = 0;

    // Sets \p destData to a dense vector of this data's type.
    // 
    // Note that \p destData must have never been initialized or freed
    // before this call.
    //
    virtual void NewDense(size_t size, DataHolder *destData) const = 0;

    // Moves this vector into the \p destData.
    //
    // Note that after the opertion, this vector data object is no longer
    // valid and may only be destroyed.
    //
    virtual void MoveInto(DataHolder *destData) = 0;

    // Clones this data data object into \p destData.
    //
    // Note that \p destData must point to valid memory.
    //
    virtual void Clone(DataHolder *destData) const = 0;

    // This is like the clone method, only it uses a mask to potentially
    // copy a smaller set of this vector into \p destData.
    //
    // Note that \p destData must point to valid memory.
    //
    virtual void CloneSubset(const VdfMask &mask,
                             DataHolder *destData) const = 0;

    // Boxes the stored data into a container. As a result of this,
    // \p destData will contain a single element holding all the elements
    // stored in this vector data instance.
    //
    // Only the data elements specified in \p bits will be pushed into the
    // boxed container.
    //
    // Note that \p destData must point to valid memory.
    //
    virtual void Box(const VdfMask::Bits &bits,
                     DataHolder *destData) const = 0;

    // Merges this data into \p destData.
    //
    // Note, that \p destData must point to valid memory.
    //
    virtual void Merge(const VdfMask::Bits& bits,
                       DataHolder *destData) const = 0;

    // Expand the storage capabilities of the underlying vector
    // implementation, if necessary.  By default, expansion is not supported.
    //
    VDF_API
    virtual void Expand(size_t first, size_t last);

    // Returns the size of the vector. Note that there may not be storage
    // allocated for all the elements in the vector size.
    //
    virtual size_t GetSize() const = 0;

    // Returns the number of elements stored in the vector implementation.
    //
    virtual size_t GetNumStoredElements() const = 0;

    // Returns a pointer to the SharedSource data structure for copyless
    // value extraction. Disabled for all implementations but 
    // Vdf_VectorImplShared.
    //
    VDF_API
    virtual Vt_ArrayForeignDataSource* GetSharedSource() const;

    // Returns true if the vector's data is sharable. Defaults to false.
    //
    VDF_API
    virtual bool IsSharable() const;

    // The vector implementation details returned by the GetInfo() method
    // as a named parameter object.
    //
    struct Info {
        enum class Layout : uint8_t {
            Unboxed,
            Boxed,
        };

        enum class Ownership : uint8_t {
            Exclusive,
            Shared
        };

        // Constructor.
        explicit Info(
            void *data_,
            size_t size_,
            size_t first_ = 0,
            size_t last_ = 0,
            Vdf_CompressedIndexMapping *compressedIndexMapping_ = NULL,
            Layout layout_ = Info::Layout::Unboxed,
            Ownership ownership_ = Info::Ownership::Exclusive) :
                data(data_),
                size(size_),
                first(first_),
                last(last_),
                compressedIndexMapping(compressedIndexMapping_),
                layout(layout_),
                ownership(ownership_)
        {}

        // Backing storage.
        void *data;

        // Size of the vector implementation.
        size_t size;

        // First element stored.
        size_t first;

        // Last element stored.
        size_t last;

        // The compressed index mapping.
        Vdf_CompressedIndexMapping *compressedIndexMapping;

        // The data layout (boxed vs unboxed)
        Layout layout;

        // If the vector implementation is shared.
        Ownership ownership;
    };

    // Returns the vector implementation details.
    //
    virtual Info GetInfo() = 0;

    // Returns the estimated size of the allocated memory for a single
    // element stored in this vector.
    //
    virtual size_t EstimateElementMemory() const = 0;

    // Returns whether a vector characterized by the given bitmask and
    // element size in bytes should be stored using a compressed
    // block layout, as opposed to a single-block sparse layout or
    // dense layout.
    //
    // The method implements a heuristic to decide whether
    // compression is appropriate.
    //
    VDF_API
    static bool ShouldStoreCompressed(
        const VdfMask::Bits &bits, int elementSize);

    // Prints the data held in this class.
    //
    // Only vectors holding a select list of types can be printed.  To see the
    // list of these types or to add to them, see the .cpp file.
    //
    VDF_API
    void DebugPrint(const VdfMask &mask, std::ostream *o) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
