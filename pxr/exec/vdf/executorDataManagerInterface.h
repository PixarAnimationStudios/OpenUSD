//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_DATA_MANAGER_INTERFACE_H
#define PXR_EXEC_VDF_EXECUTOR_DATA_MANAGER_INTERFACE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorBufferData;
class VdfExecutorInvalidationData;
class VdfNetwork;
class VdfOutput;
class VdfSMBLData;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ExecutorDataManagerInterface
///
/// \brief The interface contract for the static polymorphism used by executor
/// data manager implementations.
///

template <typename DerivedClass, typename DataHandle>
class Vdf_ExecutorDataManagerInterface 
{
protected:

    /// Allow construction via derived classes, only.
    ///
    Vdf_ExecutorDataManagerInterface() = default;

    /// Prevent destruction via base class pointers (static polymorphism only).
    ///
    ~Vdf_ExecutorDataManagerInterface() = default;

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

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorBufferData * _GetBufferData(const DataHandle handle) const {
        return _Self()->GetBufferData(handle);
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfExecutorInvalidationData * _GetInvalidationData(
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

    /// Returns an existing \p VdfSMBLData associated with the given \p handle.
    /// Returns \c nullptr if there is no SMBL data associated with this
    /// data \p handle.
    ///
    /// Note that attempting to retrieve data at an invalid handle need not
    /// be supported.
    ///
    VdfSMBLData * _GetSMBLData(const DataHandle handle) const {
        return _Self()->GetSMBLData(handle);
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle
    /// or creates a new one of none exists.
    ///
    /// Note that this must always return a valid pointer to VdfSMBLData.
    ///
    VdfSMBLData * _GetOrCreateSMBLData(const DataHandle handle) const {
        return _Self()->GetOrCreateSMBLData(handle);
    }

    /// Returns \c true if the data at the given \p handle has been touched by
    /// evaluation.
    /// 
    /// Note that attempting to touch data at an invalid handle need not
    /// be supported.
    ///
    bool _IsTouched(const DataHandle handle) const {
        return _Self()->IsTouched(handle);
    }

    /// Marks the data at the given \p handle as having been touched by
    /// evaluation.
    ///
    /// Note that attempting to touch data at an invalid handle need not
    /// be supported.
    ///
    void _Touch(const DataHandle handle) const {
        _Self()->Touch(handle);
    }

    /// Marks the data at the given \p handle as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    /// Note that attempting to un-touch data at an invalid handle need not
    /// be supported.
    ///
    bool _Untouch(const DataHandle handle) {
        return _Self()->Untouch(handle);
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
