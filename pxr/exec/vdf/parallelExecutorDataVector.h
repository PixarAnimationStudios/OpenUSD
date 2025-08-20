//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_VECTOR_H
#define PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_VECTOR_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorInvalidationData.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/work/zeroAllocator.h"

#include <tbb/concurrent_vector.h>
#include <tbb/spin_mutex.h>

#include <atomic>
#include <climits>
#include <cstdint>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_ParallelExecutorDataVector
///
/// \brief This is a data container for executor data managers that uses data
///        stored in vectors indexed by output ids. Methods on this container
///        are thread-safe unless otherwise called out in their documentation.
///
class Vdf_ParallelExecutorDataVector
{
public:

    /// The data handle type is an index into the internal data vector.
    ///
    using DataHandle = size_t;

    /// This sentinel index denotes an invalid handle.
    ///
    static const size_t InvalidHandle = size_t(-1);

    /// Constructor.
    ///
    Vdf_ParallelExecutorDataVector() :
        _numSegments(0),
        _numTouched(0)
    {}

    /// Destructor.
    ///
    VDF_API
    ~Vdf_ParallelExecutorDataVector();

    /// Resize the data manager to accommodate the given network.
    ///
    /// This method is not thread-safe. It can only be called during quiescent
    /// state.
    ///
    VDF_API
    void Resize(const VdfNetwork &network);

    /// Returns an existing data handle, or creates a new one for the given
    /// \p output.
    ///
    /// This method is guaranteed to return a valid data handle.
    ///
    inline DataHandle GetOrCreateDataHandle(const VdfId outputId) const;

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
    VdfExecutorBufferData *GetPrivateBufferData(const DataHandle handle) const {
        const uint8_t idx = _outputData[handle].bufferIndices.GetPrivateIndex();
        return &_bufferData[idx][handle];
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData *GetScratchBufferData(const DataHandle handle) const {
        const uint8_t idx = _outputData[handle].bufferIndices.GetScratchIndex();
        return &_bufferData[idx][handle];
    }

    /// Returns the VdfExecutorBufferData associated with the given \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorBufferData *GetPublicBufferData(const DataHandle handle) const {
        const uint8_t idx = _outputData[handle].bufferIndices.GetPublicIndex();
        return &_bufferData[idx][handle];
    }

    /// Publishes the private VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData. After this method returns, clients are
    /// no longer allowed to modify the private or the public buffers.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void PublishPrivateBufferData(const DataHandle handle) const {
        _outputData[handle].bufferIndices.PublishPrivateIndex();    
    }

    /// Publishes the scratch VdfExecutorBufferData, and retains the previously
    /// public VdfExecutorBufferData. After this method returns, clients are
    /// no longer allowed to modify the scratch or the public buffers.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    void PublishScratchBufferData(const DataHandle handle) const {
        _outputData[handle].bufferIndices.PublishScratchIndex();
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
        return _inboxes[handle].Get();
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
        return _inboxes[handle].Take(value, mask);
    }

    /// Resets the transferred buffer associated with the given \p handle. If
    /// any value has previously been written back to this output, its storage
    /// will be freed.
    ///
    /// Note it is undefined behavior to call this method with an invalid data
    /// \p handle.
    ///
    void ResetTransferredBufferData(const DataHandle handle) {
        _inboxes[handle].Reset();
    }

    /// Returns the VdfExecutorInvalidationData associated with the given
    /// \p handle.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    VdfExecutorInvalidationData *GetInvalidationData(
        const DataHandle handle) const {
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
    /// This method is not thread-safe.
    ///
    void SetInvalidationTimestamp(
        const DataHandle handle,
        VdfInvalidationTimestamp timestamp) {
        _outputData[handle].invalidationTimestamp = timestamp;
    }

    /// Returns \c true if the data at the given \p output has been touched by
    /// evaluation.
    ///
    bool IsTouched(const VdfId outputId) const {
        const VdfIndex outputIdx = VdfOutput::GetIndexFromId(outputId);
        const uint32_t idx = outputIdx / _TouchedWordBits;
        const uint64_t bit =
            UINT64_C(1) << (outputIdx & (_TouchedWordBits - 1));
        return
            idx < _numTouched.load(std::memory_order_acquire) &&
            (_touched[idx].load(std::memory_order_relaxed) & bit) != 0;
    }

    /// Marks the data at the given \p output as having been touched by
    /// evaluation.
    ///
    void Touch(const VdfId outputId) {
        // Lazily initialize the touched array, if necessary.
        if (ARCH_UNLIKELY(_numTouched.load(std::memory_order_acquire) == 0)) {
            _GrowTouched();
        }

        const VdfIndex outputIdx = VdfOutput::GetIndexFromId(outputId);
        const uint32_t idx = outputIdx / _TouchedWordBits;
        const uint64_t bit =
            UINT64_C(1) << (outputIdx & (_TouchedWordBits - 1));
        if ((_touched[idx].load(std::memory_order_relaxed) & bit) == 0) {
            _touched[idx].fetch_or(bit);
        }
    }

    /// Marks the data at the given \p handle as not having been touched by
    /// evaluation. Returns \c true if the data has previously been touched.
    ///
    /// Note it is undefined behavior to call this method with an invalid
    /// data \p handle.
    ///
    bool Untouch(const VdfId outputId) {
        const VdfIndex outputIdx = VdfOutput::GetIndexFromId(outputId);
        const uint32_t idx = outputIdx / _TouchedWordBits;
        const uint64_t bit =
            UINT64_C(1) << (outputIdx & (_TouchedWordBits - 1));
        if (idx < _numTouched.load(std::memory_order_acquire) &&
            (_touched[idx].load(std::memory_order_relaxed) & bit) != 0) {
            return (_touched[idx].fetch_and(~bit) & bit) != 0;
        }
        return false;
    }

    /// Returns the number of outputs that have data associated with them.
    ///
    /// This method is thread-safe, but elements accounted for may still be
    /// under construction!
    ///
    size_t GetNumData() const {
        return _outputData.size();
    }

    /// Resets the output data at the given data \p handle to a newly
    /// constructed state. The output with \p outputId is the new owner
    /// of the output data.
    ///
    VDF_API
    void Reset(const DataHandle handle, const VdfId outputId) const;

    /// Clears all the data from this manager.
    /// 
    /// This method is not thread-safe. It must be invoked during quiescent
    /// state only.
    ///
    VDF_API
    void Clear();

private:

    // An 8-bit field containing indices into the executor buffer data array.
    //
    class _BufferIndices
    {
    public:

        // Default constructor.
        //
        _BufferIndices() {
            Reset();
        }

        // Reset the buffer indices.
        //
        void Reset() {
            _indices = 
                (0 << _privateOffset) |
                (1 << _scratchOffset) |
                (2 << _publicOffset);
        }

        // Returns the private index.
        //
        const uint8_t GetPrivateIndex() const {
            return (_indices.load(std::memory_order_acquire) & _privateMask) >>
                _privateOffset;
        }

        // Returns the scratch index.
        //
        const uint8_t GetScratchIndex() const {
            return (_indices.load(std::memory_order_acquire) & _scratchMask) >>
                _scratchOffset;
        }

        // Returns the public index.
        //
        const uint8_t GetPublicIndex() const {
            return (_indices.load(std::memory_order_acquire) & _publicMask) >>
                _publicOffset;
        }

        // Swaps the private buffer with the public buffer index.
        //
        void PublishPrivateIndex() {
            uint8_t indices = _indices.load(std::memory_order_relaxed);
            uint8_t newIndices =
                // Scratch index stays.
                (indices & _scratchMask) |
                // Public index replaces the private index.
                ((indices >> _publicOffset) & _privateMask) |
                // Private index replaces the public index.
                ((indices & _privateMask) << _publicOffset);
            _indices.compare_exchange_strong(indices, newIndices);
        }

        // Swaps the scratch buffer with the public buffer index.
        //
        void PublishScratchIndex() {
            uint8_t indices = _indices.load(std::memory_order_relaxed);
            uint8_t newIndices =
                // Private index stays.
                (indices & _privateMask) |
                // Public index replaces the scratch index.
                ((indices >> _scratchOffset) & _scratchMask) |
                // Scratch index replaces the public index.
                ((indices & _scratchMask) << _scratchOffset);
            _indices.compare_exchange_strong(indices, newIndices);
        }

    private:

        // The bit offsets into the indices bitset.
        //
        constexpr static uint8_t _privateOffset = 0;
        constexpr static uint8_t _scratchOffset = 2;
        constexpr static uint8_t _publicOffset  = 4;

        // The bitmasks for each entry in the indices bitset. Two bits per
        // buffer index.
        //
        constexpr static uint8_t _privateMask   = 0x03;
        constexpr static uint8_t _scratchMask   = 0x0C;
        constexpr static uint8_t _publicMask    = 0x30;

        // The buffer indices packed into a bitset.
        //
        std::atomic<uint8_t> _indices;
    };

    // The generic output data stored for each entry in this container. Note,
    // the memory underpinning this structure must be zero-initialized. This
    // is required for the "constructed flags" to work properly.
    //
    struct _OutputData
    {
        // Noncopyable.
        //
        _OutputData(const _OutputData &) = delete;
        _OutputData &operator=(const _OutputData &) = delete;

        // Default constructor.
        //
        explicit _OutputData(const VdfId oid) {
            Reset(oid);
        }

        // Reset the output data to its defaul constructed state.
        //
        void Reset(const VdfId oid) {
            // Set the default buffer indices.
            bufferIndices.Reset();

            // Reset the invalidation timestamp.
            invalidationTimestamp =
                VdfExecutorInvalidationData::InitialInvalidationTimestamp;

            // Enforce release semantics on the outputId to synchronize the
            // non-atomic and dependent atomic write above.
            outputId.store(oid, std::memory_order_release);
        }

        // The output id.
        //
        std::atomic<VdfId> outputId;

        // The buffer indices.
        //
        _BufferIndices bufferIndices;

        // A zero-initialized checksum to synchronize the corresponding data
        // construction. The checksum will be incremented after construction of
        // each piece of data. Once the checksum has reached a specific value,
        // all data is guaranteed to be constructed.
        //
        // The correctness of the checksum implementation requires that this
        // member be initialized to zero prior to becoming visible to other
        // threads.  Due to the design of concurrent_vector, zero_allocator is
        // used to ensure that the storage is zeroed but the lifetime of the
        // _OutputData object is not guaranteed to begin before other threads
        // are able to observe its entry in the vector.  Aside from this
        // undefined behavior, there is another issue.  As of C++20,
        // std::atomic's default constructor does value-initialization.  This
        // zeroes the checksum again *after* other threads may have already
        // incremented its value.  Placing the checksum in a union and
        // omitting any explicit initialization in the _OutputData constructor
        // dodges this specific re-zeroing problem.  However, the workaround
        // cannot solve the fundamental issue of object lifetime outlined
        // above.
        //
        union {
            std::atomic<uint8_t> constructionChecksum;
        };

        // The invalidation timestamp. We store this information in the
        // generic data vector in order to make it available during evaluation
        // (mung buffer locking).
        //
        VdfInvalidationTimestamp invalidationTimestamp;

    };

    // A simple container that contains output values that had their ownership
    // transferred into this data manager. The operations provided on this
    // class are thread-safe.
    //
    class _Inbox
    {
    public:

        // Constructor.
        //
        _Inbox() : _buffer() {}

        // Destructor.
        //
        ~_Inbox() {
            VdfExecutorBufferData *buffer =
                _buffer.load(std::memory_order_acquire);
            if (buffer) {
                delete buffer;
            }
        }

        // Takes ownership of the \p value, and returns \c true if ownership
        // has successfully been transferred. Note that this instance will
        // assume responsibility of lifetime management over \p value if this
        // operation succeeds. Otherwise, the call site will maintain this
        // responsibility.
        //
        VDF_API
        bool Take(VdfVector *value, const VdfMask &mask);

        // Gets the current value.
        //
        VdfExecutorBufferData *Get() const {
            return _buffer.load(std::memory_order_acquire);
        }

        // Clears out the inbox.
        //
        void Reset() {
            if (_buffer.load(std::memory_order_relaxed)) {
                delete _buffer.exchange(nullptr);
            }
        }

    private:

        // The buffer. May be nullptr, if this inbox is empty.
        // 
        std::atomic<VdfExecutorBufferData *> _buffer;

    };

    // Type of each segment in the locations array. An array of atomic integers.
    using _LocationsSegment = std::atomic<int> *;

    // Expected to be 16 bytes in size.
    static_assert(sizeof(_OutputData) == 16, 
        "_OutputData expected to be 16 bytes in size.");

    // Resize the touched bitset to accommodate touching any output currently
    // in the network. It is safe to call this concurrently.
    //
    VDF_API
    void _GrowTouched();

    // Atomically create a new locations segment and return its pointer.
    //
    VDF_API
    _LocationsSegment _CreateSegment(size_t segmentIndex) const;

    // Create a new location index and its corresponding data for the output
    // with the given output id.
    //
    VDF_API
    int _CreateLocation(
        const VdfId outputId,
        int currentLocation,
        std::atomic<int> *newLocation) const;

    // Pushes a new data entry into the internal vectors for the output with
    // the given outputId.
    // 
    VDF_API
    int _CreateData(const VdfId outputId) const;

    // Reset the data stored at the location.
    //
    VDF_API
    void _ResetLocation(
        const VdfId outputId,
        int currentLocation,
        std::atomic<int> *newLocation) const;

    // Wait for the location entry to become available.
    // 
    VDF_API
    int _WaitForLocation(
        int currentLocation,
        std::atomic<int> *newLocation) const;

    // The number of output buffers (public, private, scratch).
    //
    constexpr static size_t _NumBuffers = 3;

    // The initial number of entries reserved in the data vectors.
    //
    constexpr static size_t _InitialDataNum = 1024;

    // Sentinels for invalid (not yet created) and pending (currently being
    // created) location indices.
    //
    constexpr static int _LocationInvalid = -1;
    constexpr static int _LocationPending = -2;

    // The number of bits in a word of the touched array.
    //
    constexpr static uint32_t _TouchedWordBits = sizeof(uint64_t) * CHAR_BIT;

    // The size of a segment in the segmented locations array. Must be a power
    // of two.
    //
    constexpr static size_t _SegmentSize = 4096;

    // The locations array, mapping from output index to output data,
    // evaluation data and invalidation data index. The array is segmented, and
    // segments will be lazily allocated.
    //
    size_t _numSegments;
    std::unique_ptr<std::atomic<_LocationsSegment>[]> _locations;

    // The touched bitset. This data needs to be stored outside of _OutputData
    // to allow us to set bits concurrently from speculation node evaluation.
    //
    std::atomic<size_t> _numTouched;
    std::unique_ptr<std::atomic<uint64_t>[]> _touched;
    tbb::spin_mutex _touchedMutex;

    // The output data.
    //
    using _OutputDataVector =
        tbb::concurrent_vector<_OutputData, WorkZeroAllocator<_OutputData>>;
    mutable _OutputDataVector _outputData;

    // The arrays of buffer data corresponding with the output data.
    //
    using _BufferDataVector = tbb::concurrent_vector<VdfExecutorBufferData>;
    mutable _BufferDataVector _bufferData[_NumBuffers];

    // The array of inboxes corresponding with the output data.
    //
    using _InboxVector = tbb::concurrent_vector<_Inbox>;
    mutable _InboxVector _inboxes;

    // The invalidation specific data corresponding with the output data.
    //
    using _InvalidationDataVector =
        tbb::concurrent_vector<VdfExecutorInvalidationData>;
    mutable _InvalidationDataVector _invalidationData;

};

///////////////////////////////////////////////////////////////////////////////

Vdf_ParallelExecutorDataVector::DataHandle
Vdf_ParallelExecutorDataVector::GetOrCreateDataHandle(
    const VdfId outputId) const
{
    // Get the output index.
    const VdfIndex outputIndex = VdfOutput::GetIndexFromId(outputId);

    // Compute the index to the segment.
    const size_t segmentIndex = outputIndex / _SegmentSize;
    TF_DEV_AXIOM(segmentIndex < _numSegments);

    // Retrieve the segment.
    _LocationsSegment segment =
        _locations[segmentIndex].load(std::memory_order_acquire);

    // Allocate the segment, if required.
    if (!segment) {
        segment = _CreateSegment(segmentIndex);
    }

    // Using the output index, look up the location in the data vector.
    const size_t segmentOffset = outputIndex & (_SegmentSize - 1);
    std::atomic<int> *location = &segment[segmentOffset];
    const int currentLocation = location->load(std::memory_order_acquire);

    // Create a new entry, if the location is still uninitialized.
    if (currentLocation < 0) {
        return _CreateLocation(outputId, currentLocation, location);
    }

    // Make sure the output id matches at the location, and reset the entry
    // if there is a mismatch.
    else if (outputId != _outputData[currentLocation].outputId.load(
        std::memory_order_acquire)) {
        _ResetLocation(outputId, currentLocation, location);
    }

    // Return the existing location.
    return currentLocation;
}

Vdf_ParallelExecutorDataVector::DataHandle
Vdf_ParallelExecutorDataVector::GetDataHandle(const VdfId outputId) const
{
    // Get the output index.
    const VdfIndex outputIndex = VdfOutput::GetIndexFromId(outputId);

    // Compute the index to the segment.
    const size_t segmentIndex = outputIndex / _SegmentSize;

    // If the index is out of bounds, we can bail out right away.
    if (segmentIndex >= _numSegments) {
        return InvalidHandle;
    }

    // Retrieve the segment.
    _LocationsSegment segment =
        _locations[segmentIndex].load(std::memory_order_acquire);

    // If the segment is not allocated, we can bail out right away.
    if (!segment) {
        return InvalidHandle;
    }

    // If the location points to a valid entry in the data vector, and the
    // data at that index matches the output id, we can return the data.
    // Otherwise, the location may either be garbage, or the output version
    // may have changed.
    const size_t segmentOffset = outputIndex & (_SegmentSize - 1);
    std::atomic<int> * const location = &segment[segmentOffset];
    int currentLocation = location->load(std::memory_order_acquire);

    // Because of the ABA problems that can occur when we call
    // _ResetLocation, it's possible to observe _LocationPending
    // as the location for this output. In this case we have to wait
    if (ARCH_UNLIKELY(currentLocation == _LocationPending)) {
        currentLocation = _WaitForLocation(currentLocation, location);
    }

    return
        currentLocation >= 0 &&
        _outputData[currentLocation].outputId.load(
            std::memory_order_acquire) == outputId
            ? currentLocation
            : InvalidHandle;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
