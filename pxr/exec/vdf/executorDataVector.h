//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_DATA_VECTOR_H
#define PXR_EXEC_VDF_EXECUTOR_DATA_VECTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorInvalidationData.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/smblData.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_vector.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ExecutorDataVector
///
/// \brief A vector-like container for executor data used by the
/// VdfDataManagerVector. Unlike a hash-map, the executor data storage laid out
/// somewhat contiguously in memory, and may therefore be quicker to access.
/// The access pattern during the first round of data access determines the
/// memory layout.
///
class Vdf_ExecutorDataVector
{
public:

    /// The data handle type is an index into the internal data vectors.
    ///
    typedef size_t DataHandle;

    /// This sentinel index denotes an invalid handle.
    ///
    static const size_t InvalidHandle = size_t(-1);

    /// Destructor.
    ///
    VDF_API
    ~Vdf_ExecutorDataVector();

    /// Resize the vector to be able to accommodate all outputs in the given
    /// network.
    ///
    VDF_API
    void Resize(const VdfNetwork &network);

    /// Returns an existing data handle, or creates a new one for the given
    /// \p output.
    ///
    /// This method is guaranteed to return a valid data handle.
    ///
    inline DataHandle GetOrCreateDataHandle(const VdfId outputId);

    /// Returns an existing data handle for the given \p output. This method
    /// will return an invalid data handle, if no handle has been created
    /// for the given \p output.
    ///
    inline DataHandle GetDataHandle(const VdfId outputId) const;

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData * GetBufferData(const DataHandle handle) {
        return &_bufferData[handle];
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorInvalidationData * GetInvalidationData(const DataHandle handle) {
        return &_invalidationData[handle];
    }

    /// Returns the VdfInvalidationTimestamp associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfInvalidationTimestamp GetInvalidationTimestamp(
        const DataHandle handle) const {
        return _outputData[handle].invalidationTimestamp;
    }

    /// Sets the invalidation \p timestamp for the give data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void SetInvalidationTimestamp(
        const DataHandle &handle,
        VdfInvalidationTimestamp ts) {
        _outputData[handle].invalidationTimestamp = ts;
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle.
    /// Returns \c nullptr if there is no SMBL data associated with this
    /// data \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetSMBLData(const DataHandle handle) const {
        return _smblData[handle].get();
    }

    /// Returns an existing \p VdfSMBLData associated with the given \p handle
    /// or creates a new one of none exists.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfSMBLData * GetOrCreateSMBLData(const DataHandle handle) {
        if (!_smblData[handle]) {
            _smblData[handle].reset(new VdfSMBLData());
        }
        return _smblData[handle].get();
    }

    /// Returns \c true if the data at the given \p handle has been touched by
    /// evaluation.
    /// 
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    bool IsTouched(const DataHandle handle) {
        return _outputData[handle].touched;
    }

    /// Marks the data at the given \p handle as having been touched by
    /// evaluation.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    void Touch(const DataHandle handle) {   
        _outputData[handle].touched = true;
    }

    /// Marks the data at the given \p handle as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle. 
    ///
    bool Untouch(const DataHandle handle) {
        const bool wasTouched = _outputData[handle].touched;
        _outputData[handle].touched = false;
        return wasTouched;
    }

    /// Returns the size of the container.
    ///
    size_t GetSize() const {
        return _locations.size();
    }

    /// Returns the number of outputs that have data associated with them.
    ///
    size_t GetNumData() const {
        return _bufferData.size();
    }

    /// Resets the output data at the given data \p handle to a newly
    /// constructed state. The output with \p outputId is the new owner
    /// of the output data.
    ///
    inline void Reset(const DataHandle handle, const VdfId outputId);

    /// Clears all the data in the container.
    ///
    VDF_API
    void Clear();

private:

    // The auxiliary output data stored for each output in the vector.
    struct _OutputData {
        _OutputData(const VdfId oid) :
            outputId(oid),
            invalidationTimestamp(
                VdfExecutorInvalidationData::InitialInvalidationTimestamp),
            touched(false)
        {}

        void Reset(const VdfId oid) {
            outputId = oid;
            invalidationTimestamp =
                VdfExecutorInvalidationData::InitialInvalidationTimestamp;
            touched = false;
        }

        VdfId outputId;
        VdfInvalidationTimestamp invalidationTimestamp;
        bool touched;
    };

    // Type of each segment in the locations array. An array of integers.
    using _LocationsSegment = uint32_t *;

    // Create a new segment at the provided index out of line, and return a
    // pointer to the new segment.
    VDF_API
    _LocationsSegment _CreateSegment(size_t segmentIndex);

    // Pushes a new data entry into the internal vectors for the output with
    // the given outputId.
    inline void _CreateData(const VdfId outputId);

    // Reserve this many executor data entries
    constexpr static size_t _InitialExecutorDataNum = 1000;

    // The size of a segment in the segmented locations array. Must be a power
    // of two.
    constexpr static size_t _SegmentSize = 4096;

    // The locations array, indexing the data vector.
    std::vector<_LocationsSegment> _locations;

    // The data vector. Note we use a tbb::concurrent_vector here not for its
    // thread-safe properties, but for the fact that it allocates chunks of
    // contiguous memory, and does not copy/move any existing elements when
    // the container grows.
    tbb::concurrent_vector<_OutputData> _outputData;
    tbb::concurrent_vector<VdfExecutorBufferData> _bufferData;
    tbb::concurrent_vector<VdfExecutorInvalidationData> _invalidationData;
    tbb::concurrent_vector<std::unique_ptr<VdfSMBLData>> _smblData;

};

///////////////////////////////////////////////////////////////////////////////

Vdf_ExecutorDataVector::DataHandle
Vdf_ExecutorDataVector::GetOrCreateDataHandle(const VdfId outputId)
{
    // Get the output index.
    const VdfIndex outputIndex = VdfOutput::GetIndexFromId(outputId);

    // Compute the index of the segment.
    const size_t segmentIndex = outputIndex / _SegmentSize;
    TF_DEV_AXIOM(segmentIndex < _locations.size());

    // Retrieve the corresponding segment, or create a new one of necessary.
    _LocationsSegment segment = _locations[segmentIndex];
    if (!segment) {
        segment = _CreateSegment(segmentIndex);
    }

    // Using the segment offset, look up the location in the segment.
    const size_t segmentOffset = outputIndex & (_SegmentSize - 1);
    uint32_t * const location = &segment[segmentOffset];

    // The location may be uninitialized, so let's do a bounds check against
    // the data vector. We further have to check whether the index at the
    // proposed location in the data vector points back to the same output.
    // If either of this is false, we will insert a new entry into the data
    // vector and update the location.
    const size_t numData = GetNumData();
    if (*location >= numData ||
        outputIndex != VdfOutput::GetIndexFromId(
            _outputData[*location].outputId)) {
        *location = numData;
        _CreateData(outputId);
    }

    // If the location points to the right place in the data vector, we must
    // also verify the output version. Note, that we do not have to extract
    // the version from the id. At this point, the entire id better match!
    // If not, the version has changed an the output data must be reset.
    else if (outputId != _outputData[*location].outputId) {
        Reset(*location, outputId);
    }

    // Return the newly inserted or existing data entry.
    return *location;
}

Vdf_ExecutorDataVector::DataHandle
Vdf_ExecutorDataVector::GetDataHandle(const VdfId outputId) const
{
    // Get the output index.
    const VdfIndex outputIndex = VdfOutput::GetIndexFromId(outputId);

    // Compute the index to the segment.
    const size_t segmentIndex = outputIndex / _SegmentSize;

    // If the segment index is out of bounds or the segment has not been
    // allocated, we can bail out right away.
    if (segmentIndex >= _locations.size() || !_locations[segmentIndex]) {
        return InvalidHandle;
    }

    // If the location points to a valid entry in the data vector, and the
    // data at that index matches the output id, we can return the data.
    // Otherwise, the location may either be garbage, or the output version
    // may have changed.
    const size_t segmentOffset = outputIndex & (_SegmentSize - 1);
    const uint32_t location = _locations[segmentIndex][segmentOffset];
    return
        location < GetNumData() && outputId == _outputData[location].outputId
            ? location
            : InvalidHandle;
}

void
Vdf_ExecutorDataVector::Reset(const DataHandle handle, const VdfId outputId)
{
    _outputData[handle].Reset(outputId);
    _bufferData[handle].Reset();
    _invalidationData[handle].Reset();
    _smblData[handle].reset();
}

void
Vdf_ExecutorDataVector::_CreateData(const VdfId outputId)
{
    _outputData.emplace_back(outputId);
    _bufferData.emplace_back();
    _invalidationData.emplace_back();
    _smblData.emplace_back();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
