//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_VECTOR_H
#define PXR_EXEC_VDF_DATA_MANAGER_VECTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorDataManager.h"
#include "pxr/exec/vdf/executorDataVector.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// Enum representing the deallocation strategy for the data manager.
///
enum class VdfDataManagerDeallocationMode
{
    /// Deallocate in the background
    ///
    Background = 0,

    /// Deallocate immediately
    ///
    Immediate
};

////////////////////////////////////////////////////////////////////////////////

template<VdfDataManagerDeallocationMode DeallocationMode>
class VdfDataManagerVector;
class VdfExecutorBufferData;
class VdfExecutorInvalidationData;
class VdfNetwork;
class VdfOutput;
class VdfSMBLData;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ExecutorDataManagerTraits<VdfDataManagerVector>
///
/// \brief Type traits specialization for the VdfDataManagerVector.
///
template<VdfDataManagerDeallocationMode Mode>
struct Vdf_ExecutorDataManagerTraits< VdfDataManagerVector<Mode> > {

    /// The data handle type. For the VdfDataManagerVector this is an index
    /// into the vector.
    ///
    typedef Vdf_ExecutorDataVector::DataHandle DataHandle;
};


///////////////////////////////////////////////////////////////////////////////
///
/// Private functions for memory management strategies
///

// Allocates an executor data vector for the given \p network.
VDF_API
Vdf_ExecutorDataVector *
Vdf_DataManagerVectorAllocate(const VdfNetwork &network);

// Deallocates the executor data vector \p data that was alloacted with
// Vdf_DataManagerVectorAllocate, and does so immediately.
VDF_API
void
Vdf_DataManagerVectorDeallocateNow(Vdf_ExecutorDataVector *data);

// Queues up deallocation of the executor data vector \p data that was 
// alloacted with Vdf_DataManagerVectorAllocate.  The actually memory
// will be deallocated at an unspecified time in the future.
VDF_API
void 
Vdf_DataManagerVectorDeallocateLater(Vdf_ExecutorDataVector *data);


///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfDataManagerVector
///
/// \brief This is a data manager for executors that uses data stored in a
///        vector indexed by output ids.
///
template<VdfDataManagerDeallocationMode DeallocationMode>
class VdfDataManagerVector : 
    public VdfExecutorDataManager<VdfDataManagerVector<DeallocationMode> >
{
public:

    /// The base class.
    ///
    typedef
        VdfExecutorDataManager<VdfDataManagerVector>
        Base;

    /// The data handle type from the type traits class.
    ///
    typedef
        typename Vdf_ExecutorDataManagerTraits<
            VdfDataManagerVector>::DataHandle
        DataHandle;

    /// Constructor.
    ///
    VdfDataManagerVector() : _data(nullptr) { }

    /// Destructor
    ///
    ~VdfDataManagerVector();

    /// Resize the data manager to accommodate the given network.
    ///
    void Resize(const VdfNetwork &network);

    /// Returns \c true if the given data \p handle is valid, i.e. it is valid
    /// to ask for data for this given \p handle.
    ///
    /// Note that attempting to resolve data at an invalid handle results in
    /// undefined behavior.
    ///
    bool IsValidDataHandle(const DataHandle handle) const {
        return handle != Vdf_ExecutorDataVector::InvalidHandle;
    }

    /// Returns an existing data handle, or creates a new one for the given
    /// \p outputId.
    ///
    /// This method is guaranteed to return a valid data handle.
    ///
    DataHandle GetOrCreateDataHandle(const VdfId outputId) const {
        return _data->GetOrCreateDataHandle(outputId);
    }

    /// Returns an existing data handle for the given \p outputId. This method
    /// will return an invalid data handle, if no handle has been created
    /// for the given \p outputId.
    ///
    DataHandle GetDataHandle(const VdfId outputId) const {
        return _data
            ? _data->GetDataHandle(outputId)
            : Vdf_ExecutorDataVector::InvalidHandle;
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData * GetBufferData(const DataHandle handle) const {
        return _data->GetBufferData(handle);
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorInvalidationData * GetInvalidationData(
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

    /// Returns an existing \p VdfSMBLData associated with the given \p handle.
    /// Returns \c nullptr if there is no SMBL data associated with this
    /// data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetSMBLData(const DataHandle handle) const {
        return _data->GetSMBLData(handle);
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle
    /// or creates a new one of none exists.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetOrCreateSMBLData(const DataHandle handle) const {
        return _data->GetOrCreateSMBLData(handle);
    }

    /// Returns \c true if the data at the given \p handle has been touched by
    /// evaluation.
    /// 
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    bool IsTouched(const DataHandle handle) const {
        return _data->IsTouched(handle);
    }

    /// Marks the data at the given \p handle as having been touched by
    /// evaluation.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    void Touch(const DataHandle handle) const {
        _data->Touch(handle);
    }

    /// Marks the data at the given \p handle as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    bool Untouch(const DataHandle handle) {
        return _data->Untouch(handle);
    }

    /// Clears the executor data for a specific output
    /// 
    void ClearDataForOutput(const VdfId outputId);

    /// Clears all the data from this manager.
    /// 
    void Clear();

    /// Returns \c true if this data manager is empty.
    ///
    bool IsEmpty() const {
        return !_data || _data->GetNumData() == 0;
    }


private:

    // The Vdf_ExecutorDataVector instance that holds the data.
    mutable Vdf_ExecutorDataVector *_data;

};

///////////////////////////////////////////////////////////////////////////////

template<VdfDataManagerDeallocationMode DeallocationMode>
VdfDataManagerVector<DeallocationMode>::~VdfDataManagerVector()
{
    if (DeallocationMode == VdfDataManagerDeallocationMode::Immediate) {
        Vdf_DataManagerVectorDeallocateNow(_data);
    } else {
        Vdf_DataManagerVectorDeallocateLater(_data);
    }
}

template<VdfDataManagerDeallocationMode DeallocationMode>
void
VdfDataManagerVector<DeallocationMode>::Resize(const VdfNetwork &network)
{
    // Allocate a new Vdf_ExecutorDataVector if necessary. 
    if (!_data) {
        _data = Vdf_DataManagerVectorAllocate(network);
    }

    // Otherwise, make sure to resize our current instance.
    else {
        _data->Resize(network);
    }
}

template<VdfDataManagerDeallocationMode DeallocationMode>
void
VdfDataManagerVector<DeallocationMode>::ClearDataForOutput(
    const VdfId outputId)
{
    // Clear the data associated with the given output (if it exists).
    if (_data) {
        const DataHandle dataHandle = _data->GetDataHandle(outputId);
        if (IsValidDataHandle(dataHandle)) {
            _data->Reset(dataHandle, outputId);
        }
    }
}

template<VdfDataManagerDeallocationMode DeallocationMode>
void
VdfDataManagerVector<DeallocationMode>::Clear()
{
    // Clear all data.
    if (_data) {
        _data->Clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
