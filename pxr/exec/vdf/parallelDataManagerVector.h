//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_DATA_MANAGER_VECTOR_H
#define PXR_EXEC_VDF_PARALLEL_DATA_MANAGER_VECTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/parallelExecutorDataManager.h"
#include "pxr/exec/vdf/parallelExecutorDataVector.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorBufferData;
class VdfExecutorInvalidationData;
class VdfNetwork;
class VdfParallelDataManagerVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ParallelExecutorDataManagerTraits<VdfParallelDataManagerVector>
///
/// \brief Type traits specialization for the VdfParallelDataManagerVector.
///
template<>
struct Vdf_ParallelExecutorDataManagerTraits<VdfParallelDataManagerVector> {

    /// The data handle type. For the VdfParallelDataManagerVector this is an
    /// index into the vector.
    ///
    typedef Vdf_ParallelExecutorDataVector::DataHandle DataHandle;

};

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelDataManagerVector
///
/// \brief This is a data manager for executors that uses data stored in a
///        vector indexed by output ids. Note that all methods on this data
///        manager are thread-safe unless specifically called out to not
///        be thread-safe in their documentation.
///
class VDF_API_TYPE VdfParallelDataManagerVector : 
    public VdfParallelExecutorDataManager<VdfParallelDataManagerVector>
{
public:

    /// The base class
    ///
    typedef
        VdfParallelExecutorDataManager<VdfParallelDataManagerVector>
        Base;

    /// The data handle type from the type traits class.
    ///
    typedef
        typename Vdf_ParallelExecutorDataManagerTraits<
            VdfParallelDataManagerVector>::DataHandle
        DataHandle;

    /// Constructor.
    ///
    VdfParallelDataManagerVector() : _data(nullptr) {}

    /// Destructor.
    ///
    VDF_API
    ~VdfParallelDataManagerVector();

    /// Resize the data manager to accommodate the given network.
    ///
    /// This method is not thread-safe. It can only be called during quiescent
    /// state.
    ///
    VDF_API
    void Resize(const VdfNetwork &network);

    /// Returns \c true if the given data \p handle is valid, i.e. it is valid
    /// to ask for data for this given \p handle.
    ///
    /// Note that attempting to resolve data at an invalid handle results in
    /// undefined behavior.
    ///
    bool IsValidDataHandle(const DataHandle handle) const {
        return handle != Vdf_ParallelExecutorDataVector::InvalidHandle;
    }

    /// Returns an existing data handle, or creates a new one for the given
    /// \p outputId.
    ///
    /// This method is guaranteed to return a valid data handle.
    ///
    /// This method is not thread-safe when invoken with the same \p output
    /// parameter from multiple threads. It is safe to call this method with
    /// concurrently, with different \p output parameters.
    ///
    inline DataHandle GetOrCreateDataHandle(const VdfId outputId) const {
        return _data->GetOrCreateDataHandle(outputId);
    }

    /// Returns an existing data handle for the given \p outputId. This method
    /// will return an invalid data handle, if no handle has been created
    /// for the given \p output.
    ///
    inline DataHandle GetDataHandle(const VdfId outputId) const {
        return _data
            ? _data->GetDataHandle(outputId)
            : Vdf_ParallelExecutorDataVector::InvalidHandle;
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData *GetPrivateBufferData(const DataHandle handle) const {
        return _data->GetPrivateBufferData(handle);
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData *GetScratchBufferData(const DataHandle handle) const {
        return _data->GetScratchBufferData(handle);
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData *GetPublicBufferData(const DataHandle handle) const {
        return _data->GetPublicBufferData(handle);
    }

    /// Publishes the private VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData.
    /// After this method returns, clients may still read from the private data,
    /// but are no longer allowed to mutate it.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void PublishPrivateBufferData(const DataHandle handle) const {
        _data->PublishPrivateBufferData(handle);
    }

    /// Publishes the scratch VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData.
    /// After this method returns, clients may still read from the scratch data,
    /// but are no longer allowed to mutate it.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void PublishScratchBufferData(const DataHandle handle) const {
        _data->PublishScratchBufferData(handle);
    }

    /// Returns the transferred VdfExecutorBufferData associated with the given
    /// \p handle. This method will return nullptr, if no value has been written
    /// back to this output.
    ///
    /// Note it is undefined behavior to call this method with an invalid data
    /// \p handle.
    ///
    VdfExecutorBufferData *GetTransferredBufferData(
        const DataHandle handle) const {
        return _data->GetTransferredBufferData(handle);
    }

    /// Transfers ownership of the \p value to the output associated with
    /// \p handle. Returns \c true if the transfer of ownership was successful.
    /// If the transfer of ownership was successful, the responsibility of
    /// lifetime management for \p value transfers to this data manager.
    /// Otherwise, the call site maintains this responsibility.
    /// 
    /// Note that only one \p value can be transferred to each output.
    /// Subsequent attempts to transfer will fail for that output.
    /// 
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    bool TransferBufferData(
        const DataHandle handle,
        VdfVector *value,
        const VdfMask &mask) {
        return _data->TransferBufferData(handle, value, mask);
    }

    /// Resets the transferred buffer associated with the given \p handle. If
    /// any value has previously been written back to this output, its storage
    /// will be freed.
    ///
    /// Note it is undefined behavior to call this method with an invalid data
    /// \p handle.
    ///
    void ResetTransferredBufferData(const DataHandle handle) {
        _data->ResetTransferredBufferData(handle);
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorInvalidationData *GetInvalidationData(
        const DataHandle handle) const {
        return _data->GetInvalidationData(handle);
    }

    /// Un-hide the GetInvalidationTimestamp method declared in the base class.
    ///
    using Base::GetInvalidationTimestamp;

    /// Returns the VdfInvalidationTimestamp associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfInvalidationTimestamp GetInvalidationTimestamp(
        const DataHandle handle) const {
        return _data->GetInvalidationTimestamp(handle);
    }

    /// Sets the invalidation \p timestamp for the give data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void SetInvalidationTimestamp(
        const DataHandle handle,
        VdfInvalidationTimestamp timestamp) {
        _data->SetInvalidationTimestamp(handle, timestamp);
    }

    /// Returns \c true if the data at the given \p outputId has been touched by
    /// evaluation.
    ///
    bool IsTouched(const VdfId outputId) const {
        return _data->IsTouched(outputId);
    }

    /// Marks the data at the given \p outputId as having been touched by
    /// evaluation.
    ///
    void Touch(const VdfId outputId) const {
        _data->Touch(outputId);
    }

    /// Marks the data at the given \p outputId as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    bool Untouch(const VdfId outputId) {
        return _data->Untouch(outputId);
    }

    /// Clears the executor data for a specific output
    ///
    /// This method is not thread-safe. It must be invoked during quiescent
    /// state only.
    ///
    VDF_API
    void ClearDataForOutput(const VdfId outputId);

    /// Clears all the data from this manager.
    /// 
    /// This method is not thread-safe. It must be invoked during quiescent
    /// state only.
    ///
    VDF_API
    void Clear();

    /// Returns \c true if this data manager is empty.
    ///
    bool IsEmpty() const {
        return !_data || _data->GetNumData() == 0;
    }


private:

    // Pointer the the executor data vector.
    mutable Vdf_ParallelExecutorDataVector *_data;

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
