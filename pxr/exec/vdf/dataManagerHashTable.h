//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_HASH_TABLE_H
#define PXR_EXEC_VDF_DATA_MANAGER_HASH_TABLE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorDataManager.h"
#include "pxr/exec/vdf/executorInvalidationData.h"
#include "pxr/exec/vdf/smblData.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stl.h"

#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class VdfDataManagerHashTable;
class VdfOutput;
class VdfNetwork;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ExecutorDataManagerTraits<VdfDataManagerHashTable>
///
/// \brief Type traits specialization for the VdfDataManagerHashTable.
///
template<>
struct Vdf_ExecutorDataManagerTraits<VdfDataManagerHashTable> {

    /// The output data stored at each entry in the hash table.
    ///
    struct OutputData {
        OutputData() :
            invalidationTimestamp(
                VdfExecutorInvalidationData::InitialInvalidationTimestamp),
            touched(false)
        {}

        VdfExecutorBufferData bufferData;
        VdfExecutorInvalidationData invalidationData;
        VdfInvalidationTimestamp invalidationTimestamp;
        std::unique_ptr<VdfSMBLData> smblData;
        bool touched;
    };

    /// The data handle type. For the VdfDataManagerHashTable this is simply
    /// a pointer to value stored in the hash table.
    ///
    typedef OutputData * DataHandle;

};

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfDataManagerHashTable
///
/// \brief This is a data manager for executors that uses data stored in an
/// external hash table.
///
class VdfDataManagerHashTable : 
    public VdfExecutorDataManager<VdfDataManagerHashTable>
{

    // The output data stored at each entry.
    typedef
        typename Vdf_ExecutorDataManagerTraits<
            VdfDataManagerHashTable>::OutputData
        _OutputData;

    // Data type for map from outputs to their executor data.
    using _DataMap = std::unordered_map<VdfId, _OutputData, TfHash>;

public:

    /// The base class type.
    ///
    typedef
        VdfExecutorDataManager<VdfDataManagerHashTable>
        Base;

    /// The data handle type from the type traits class.
    ///
    typedef
        typename Vdf_ExecutorDataManagerTraits<
            VdfDataManagerHashTable>::DataHandle
        DataHandle;

    /// Resize the data manager to accommodate all the outputs in the given
    /// network.
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
        return handle != nullptr;
    }

    /// Returns an existing data handle, or creates a new one for the given
    /// \p outputId.
    ///
    /// This method is guaranteed to return a valid data handle.
    ///
    DataHandle GetOrCreateDataHandle(const VdfId outputId) const {
        _DataMap::iterator it = _outputData.find(outputId);
        if (it == _outputData.end()) {
            it = _outputData.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(outputId),
                std::forward_as_tuple())
                    .first;
        }
        return &it->second;
    }

    /// Returns an existing data handle for the given \p outputId. This method
    /// will return an invalid data handle, if no handle has been created
    /// for the given \p outputId.
    ///
    DataHandle GetDataHandle(const VdfId outputId) const {
        return TfMapLookupPtr(_outputData, outputId);
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData * GetBufferData(const DataHandle handle) const {
        return &handle->bufferData;
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorInvalidationData * GetInvalidationData(
        const DataHandle handle) const {
        return &handle->invalidationData;
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
        return handle->invalidationTimestamp;
    }

    /// Sets the invalidation \p timestamp for the give data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void SetInvalidationTimestamp(
        const DataHandle handle,
        VdfInvalidationTimestamp timestamp) {
        handle->invalidationTimestamp = timestamp;
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle.
    /// Returns \c nullptr if there is no SMBL data associated with this
    /// data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetSMBLData(const DataHandle handle) const {
        return handle->smblData.get();
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle
    /// or creates a new one of none exists.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetOrCreateSMBLData(const DataHandle handle) const {
        if (!handle->smblData) {
            handle->smblData.reset(new VdfSMBLData());
        }      
        return handle->smblData.get();
    }

    /// Returns \c true if the data at the given \p handle has been touched by
    /// evaluation.
    /// 
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    bool IsTouched(const DataHandle handle) const {
        return handle->touched;
    }

    /// Marks the data at the given \p handle as having been touched by
    /// evaluation.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    void Touch(const DataHandle handle) const {
        handle->touched = true;
    }

    /// Marks the data at the given \p handle as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    bool Untouch(const DataHandle handle) {
        const bool wasTouched = handle->touched;
        handle->touched = false;
        return wasTouched;
    }

    /// Clears the executor data for a specific output
    /// 
    void ClearDataForOutput(const VdfId outputId) {
        _outputData.erase(outputId);
    }

    /// Clears all the data from this manager.
    /// 
    void Clear() {
        TfReset(_outputData);
    }

    /// Returns \c true if this data manager is empty.
    ///
    bool IsEmpty() const {
        return _outputData.empty();
    }


private:

    // Map from outputs to their executor data.
    mutable _DataMap _outputData;

};

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
