//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_MANAGER_INTERFACE_H
#define PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_MANAGER_INTERFACE_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorBufferData;
class VdfExecutorInvalidationData;
class VdfNetwork;
class VdfOutput;
class VdfVector;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ParallelExecutorDataManagerInterface
///
/// \brief The interface contract for the static polymorphism used by parallel
///        executor data manager implementations.
///
template <typename DerivedClass, typename DataHandle>
class Vdf_ParallelExecutorDataManagerInterface 
{
protected:

    /// Allow construction via derived classes, only.
    ///
    Vdf_ParallelExecutorDataManagerInterface() {}

    /// Prevent destruction via base class pointers (static polymorphism only).
    ///
    virtual ~Vdf_ParallelExecutorDataManagerInterface() {}

    /// Resize the data manager to accommodate all the outputs in the given
    /// network.
    ///
    void _Resize(const VdfNetwork &network) {
        _Self()->Resize(network);
    }

    /// Returns \c true if the given data \p handle is valid, i.e. it is valid
    /// to ask for data for this given \p handle.
    ///
    /// Note that attempting to resolve data at an invalid handle need not be
    /// supported.
    ///
    bool _IsValidDataHandle(const DataHandle handle) const {
        return _Self()->IsValidDataHandle(handle);
    }

    /// Returns an existing data handle, or creates a new one for the given
    /// \p outputId.
    ///
    /// This method must always return a valid data handle.
    ///
    DataHandle _GetOrCreateDataHandle(const VdfId outputId) const {
        return _Self()->GetOrCreateDataHandle(outputId);
    }

    /// Returns an existing data handle for the given \p outputId. This method
    /// must return an invalid data handle, if no handle has been created
    /// for the given \p outputId.
    ///
    DataHandle _GetDataHandle(const VdfId outputId) const {
        return _Self()->GetDataHandle(outputId);
    }

    /// Returns the private VdfExecutorBufferData associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorBufferData *_GetPrivateBufferData(
        const DataHandle handle) const {
        return _Self()->GetPrivateBufferData(handle);
    }

    /// Returns the scratch VdfExecutorBufferData associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorBufferData * _GetScratchBufferData(
        const DataHandle handle) const {
        return _Self()->GetScratchBufferData(handle);
    }

    /// Returns the public VdfExecutorBufferData associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorBufferData *_GetPublicBufferData(
        const DataHandle handle) const {
        return _Self()->GetPublicBufferData(handle);
    }

    /// Publishes the private VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData as private data.
    /// After this method returns, clients may still read from the private data,
    /// but are no longer allowed to mutate it.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void _PublishPrivateBufferData(const DataHandle handle) const {
        _Self()->PublishPrivateBufferData(handle);
    }

    /// Publishes the scratch VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData as scratch data.
    /// After this method returns, clients may still read from the scratch data,
    /// but are no longer allowed to mutate it.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void _PublishScratchBufferData(const DataHandle handle) const {
        _Self()->PublishScratchBufferData(handle);
    }

    /// Returns the transferred VdfExecutorBufferData associated with the given
    /// \p handle. This method will return nullptr, if no value has been written
    /// back to this output.
    ///
    /// Note it is undefined behavior to call this method with an invalid data
    /// \p handle.
    ///
    VdfExecutorBufferData *_GetTransferredBufferData(
        const DataHandle handle) const {
        return _Self()->GetTransferredBufferData(handle);
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
    bool _TransferBufferData(
        const DataHandle handle,
        VdfVector *value,
        const VdfMask &mask) {
        return _Self()->TransferBufferData(handle, value, mask);
    }

    /// Resets the transferred buffer associated with the given \p handle. If
    /// any value has previously been written back to this output, its storage
    /// will be freed.
    ///
    /// Note it is undefined behavior to call this method with an invalid data
    /// \p handle.
    ///
    void _ResetTransferredBufferData(const DataHandle handle) {
        _Self()->ResetTransferredBufferData(handle);
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorInvalidationData *_GetInvalidationData(
        const DataHandle handle) const {
        return _Self()->GetInvalidationData(handle);
    }

    /// Returns the VdfInvalidationTimestamp associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfInvalidationTimestamp _GetInvalidationTimestamp(
        const DataHandle handle) const {
        return _Self()->GetInvalidationTimestamp(handle);
    }

    /// Sets the invalidation \p timestamp for the give data \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    void _SetInvalidationTimestamp(
        const DataHandle handle,
        VdfInvalidationTimestamp ts) {
        _Self()->SetInvalidationTimestamp(handle, ts);
    }

    /// Returns \c true if the data at the given \p output has been touched by
    /// evaluation.
    ///
    bool _IsTouched(const VdfId outputId) const {
        return _Self()->IsTouched(outputId);
    }

    /// Marks the data at the given \p output as having been touched by
    /// evaluation.
    ///
    void _Touch(const VdfId outputId) const {
        _Self()->Touch(outputId);
    }

    /// Marks the data at the given \p output as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    bool _Untouch(const VdfId outputId) {
        return _Self()->Untouch(outputId);
    }

    /// Clears the executor data for a specific output
    ///
    void _ClearDataForOutput(const VdfOutput &output) {
        return _Self()->ClearDataForOutput(output);
    }

private:

    // Returns the constant this pointer to the derived class.
    const DerivedClass * _Self() const {
        return static_cast<const DerivedClass *>(this);
    }


    // Returns the mutable this pointer to the derived class.
    DerivedClass * _Self() {
        return static_cast<DerivedClass *>(this);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
