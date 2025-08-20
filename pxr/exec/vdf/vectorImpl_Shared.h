//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_VECTOR_IMPL_SHARED_H
#define PXR_EXEC_VDF_VECTOR_IMPL_SHARED_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/vectorData.h"

#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/vt/array.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Vdf_VectorImplShared
///
/// \brief Implements a Vdf_VectorData storage the supports reference counted 
///        sharing of other vector implementations.
///
class VDF_API_TYPE Vdf_VectorImplShared final
    : public Vdf_VectorData
{
public:
    // This class only stores a TfDelegatedCountPtr to _SharedSource, so most
    // of the constructor, destructor, and operator heavy lifting is
    // implemented in the equivalent methods on TfDelegatedCountPtr.
    //
    VDF_API
    explicit Vdf_VectorImplShared(DataHolder* srcData);
    VDF_API
    Vdf_VectorImplShared(const Vdf_VectorImplShared &o);
    VDF_API
    Vdf_VectorImplShared(Vdf_VectorImplShared &&o) noexcept;
    VDF_API
    ~Vdf_VectorImplShared() override;

    // Returns the type_info for the held data.
    //
    VDF_API
    const std::type_info& GetTypeInfo() const override;
    
    // Sets \p destData to an empty data of this data's held type.
    //
    VDF_API
    void NewEmpty(size_t size, DataHolder* destData) const override;

    // Sets \p destData to a single element of this data's held type.
    //
    VDF_API
    void NewSingle(DataHolder* destData) const override;

    // Sets \p destData to a sparse vector of this data's held type.
    //
    VDF_API
    void NewSparse(size_t size, size_t first, size_t last, 
        DataHolder* destData) const override;

    // Sets \p destData to a dense vector of this data's held type.
    //
    VDF_API
    void NewDense(size_t size, DataHolder* destData) const override;

    // Moves this vector's held data into the \p destData.
    //
    VDF_API
    void MoveInto(Vdf_VectorData::DataHolder* destData) override;

    // Clones this vector's data object into \p destData. Causes the _refCount 
    // to increase.
    //
    VDF_API
    void Clone(Vdf_VectorData::DataHolder* destData) const override;

    // This is like the clone method, only it uses a mask to potentially
    // copy a smaller set of this vector into \p destData. This does not 
    // cause the _refCount to increase.
    //
    VDF_API
    void CloneSubset(const VdfMask& mask, 
        Vdf_VectorData::DataHolder* destData) const override;

    // Boxes the stored data. As a result of this, \p destData will store a
    // single element, which is a Vdf_BoxedContainer containing all the
    // elements stored in this vector data instance.
    //
    // Only the data elements specified in \p bits will be pushed into the
    // boxed container.
    //
    VDF_API
    void Box(const VdfMask::Bits& bits,
        Vdf_VectorData::DataHolder* destData) const override;
    
    // Merges the held data into \p destData.
    //
    VDF_API
    void Merge(const VdfMask::Bits& bits,
        Vdf_VectorData::DataHolder* destData) const override;

    // Expand the storage capabilities of the underlying vector
    // implementation, if necessary.
    //
    // Note, this should never get called on VectorImpl_Shared as it mutates
    // the held data.
    //
    VDF_API
    void Expand(size_t first, size_t last) override;

    // Returns the size of the vector. Note that there may not be storage
    // allocated for all the elements in the vector size.
    //
    VDF_API
    size_t GetSize() const override;

    // Returns the number of elements stored in the vector implementation.
    //
    VDF_API
    size_t GetNumStoredElements() const override;

    // Returns a pointer to the SharedSource data structure for copyless
    // value extraction.
    //
    VDF_API
    Vt_ArrayForeignDataSource* GetSharedSource() const override;

    // Returns the vector implementation details.
    //
    VDF_API
    Info GetInfo() override;

    // Returns the estimated size of the allocated memory for a single
    // element stored in this vector.
    //
    VDF_API
    size_t EstimateElementMemory() const override;

    // Detaches \p data from its shared source data.
    //
    // It is the caller's responsibility to ensure that \p *data is holding a
    // Vdf_VectorImplShared.
    //
    // If the ref count of the shared data is one then it is not safe to 
    // make copies of the shared source and detach at the same time. This
    // is not an issue in practice because the executor data manager will 
    // always hold onto the last instance of the shared source. Meaning that
    // if the ref count is one then either no clients are holding onto a copy
    // or there is no more data manager (meaning that the vector cannot be
    // written to). Also it is generally not thread-safe to try to make 
    // copies of something being written to concurrently so you would already
    // have to be in a bad place for this to occur.
    //
    // An alternate approach would be to not optimize the refcount equals one
    // case inside this method. Instead copies would always occur during
    // detachment and the data manager could "unshare" previously shared
    // vectors when they are reused. The performance benefits would be similar.
    // 
    VDF_API
    static void Detach(Vdf_VectorData::DataHolder* data);

private:
    // Implement a foreign data source for VtArray, it shares in the 
    // lifetime of the held DataHolder.
    class _SharedSource: public Vt_ArrayForeignDataSource
    {
    public:
        explicit _SharedSource(DataHolder* srcData);

        ~_SharedSource();

        // Return the held DataHolder for use in detachment.
        Vdf_VectorData::DataHolder* GetHolder() {
            return &_data;
        }

        // Returns true if there is only one last outstanding reference to the
        // shared data.
        bool IsUnique() const {
            return _refCount.load(std::memory_order_acquire) == 1;
        }

        // Specialized TfDelegatedCountPtr operations for _SharedSource.
        friend inline void
        TfDelegatedCountIncrement(_SharedSource* s) noexcept {
            s->_refCount.fetch_add(1, std::memory_order_relaxed);
        }
        
        friend inline void
        TfDelegatedCountDecrement(_SharedSource* s) noexcept {
            if (s->_refCount.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete s;
            }
        }

    private:
        // Callback for VtArray foreign data source.
        static void _Detached(Vt_ArrayForeignDataSource* selfBase);

        Vdf_VectorData::DataHolder _data;
    };

    TfDelegatedCountPtr<_SharedSource> _source;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
