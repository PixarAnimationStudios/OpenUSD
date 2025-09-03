//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_MANAGER_H
#define PXR_EXEC_VDF_PARALLEL_EXECUTOR_DATA_MANAGER_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorInvalidationData.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/parallelExecutorDataManagerInterface.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfMask;
class VdfVector;

/// Forward definition of the traits class, which will be specialized by the
/// derived data manager implementation, and contain at least a type definition
/// for the DataHandle.
template<typename DerivedClass> struct Vdf_ParallelExecutorDataManagerTraits;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfParallelExecutorDataManager
///
/// \brief This class provides functionality to manage executor data
///        stored as VdfExecutorData from multiple threads. The methods in this
///        data manager are thread-safe unless called out to not be thread-safe
///        in the documentation.
///
template < typename DerivedClass >
class VdfParallelExecutorDataManager :
    public Vdf_ParallelExecutorDataManagerInterface<
        DerivedClass,
        typename Vdf_ParallelExecutorDataManagerTraits<DerivedClass>::DataHandle
        >
{
public:

    /// The data handle type defined via the specialized traits class.
    ///
    typedef
        typename Vdf_ParallelExecutorDataManagerTraits<DerivedClass>::DataHandle
        DataHandle;

    /// The base class type.
    ///
    typedef
        Vdf_ParallelExecutorDataManagerInterface<DerivedClass, DataHandle>
        Base;

    /// Returns the input value flowing across the given \p connection with the
    /// given \p mask.  If the the cache is not valid, or if the cache does not
    /// contain all the elements in \p mask, returns NULL. If no output data
    /// exists for \p output, it will not be created.
    ///
    VdfVector *GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const;

    /// Returns the cached value for a given \p output and \p mask.  If the the
    /// cache is not valid, or if the cache does not contain all the elements in
    /// \p mask, returns NULL.  If no output data exists for \p output, it will
    /// not be created.
    ///
    VdfVector *GetOutputValueForReading(
        const DataHandle dataHandle,
        const VdfMask &mask) const;

    /// Returns a new or existing output value.
    ///
    VdfVector *GetOrCreateOutputValueForWriting(
        const VdfOutput &output,
        const DataHandle dataHandle) const;

    /// Sets the cached value for a given \p output, creating the output cache
    /// if necessary.
    ///
    /// If the output already contains data, it will be merged with the new
    /// data as indicated by \p value and \p mask.
    ///
    /// This method provides very limited thread-safety: It is safe to call
    /// this method if the data manager is already sized appropriately, and
    /// during a round of evaluation, data is set no more than once per
    /// individual output. It is not safe to call this method concurrently on
    /// the same output, including an output that may have its value mutated as
    /// part of a concurrent round of evaluation.
    ///
    void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask);

    /// Transfers the ownership of \p value to the given \p output. Returns
    /// \c true if this succeeds. The output will assume responsibility for the
    /// lifetime management of \p value, if successful. Otherwise, the call site
    /// maintains this responsibility.
    /// 
    /// If a value has previously been transferred to this \p output, the
    /// method will fail to transfer the ownership of this \p value and return
    /// \c false.
    ///
    bool TakeOutputValue(
        const VdfOutput &output,
        VdfVector *value,
        const VdfMask &mask);

    /// Called to set destOutput's buffer output to be a reference to the 
    /// \p sourceValue.
    ///
    void SetReferenceOutputValue(
        const VdfVector *sourceValue,
        const VdfId destOutputId) const;

    /// Creates a new cache for an output, given the output data object.
    ///
    static VdfVector *CreateOutputCache(
        const VdfOutput &output,
        VdfExecutorBufferData *bufferData);

    /// Creates a new cache for an output, given the output data object. This
    /// method makes sure that the returned vector is properly resized to
    /// accommodate all the elements set in the specified \p bits.
    ///
    static VdfVector *CreateOutputCache(
        const VdfOutput &output,
        VdfExecutorBufferData *bufferData,
        const VdfMask::Bits &bits);

    /// Duplicates the output data associated with \p sourceOutput and copies
    /// it to \p destOutput.
    ///
    /// This method is not thread-safe.
    ///
    void DuplicateOutputData(
        const VdfOutput &sourceOutput,
        const VdfOutput &destOutput);

    /// Returns \c true if the output is already invalid for the given
    /// \p invalidationMask.
    ///
    bool IsOutputInvalid(
        const VdfId outputId,
        const VdfMask &invalidationMask) const;

    /// Marks \p output as invalid.
    ///
    /// Returns \c true if there was anything to invalidate and false if the
    /// \p output was already invalid.
    ///
    /// This method is not thread-safe.
    ///
    bool InvalidateOutput(
        const VdfOutput &output,
        const VdfMask &invalidationMask);

    /// Marks the data at the given \p output as having been touched by
    /// evaluation.
    ///
    void Touch(const VdfOutput &output) const;

    /// Increments the the current invalidation timestamp on this executor.
    ///
    /// This method is not thread-safe.
    ///
    void UpdateInvalidationTimestamp(
        const VdfInvalidationTimestamp &timestamp) {
        _invalidationTimestamp = timestamp;
    }

    /// Returns the current invalidation timestamp on this executor.
    ///
    const VdfInvalidationTimestamp &GetInvalidationTimestamp() const {
        return _invalidationTimestamp;
    }

    /// Returns \c true, if the invalidation timestamps between
    /// \p sourceExecutorData and \p destExecutorData do not match, i.e. the
    /// source output should be mung buffer locked.
    ///
    bool HasInvalidationTimestampMismatch(
        const DataHandle &sourceHandle,
        const DataHandle &destHandle) const;

protected:

    /// Constructor
    ///
    VdfParallelExecutorDataManager() :
        _invalidationTimestamp(
            VdfExecutorInvalidationData::InitialInvalidationTimestamp + 1)
    { }

    /// Prevent destruction via base class pointers (static polymorphism only).
    ///
    ~VdfParallelExecutorDataManager() {};

private:

    // Returns an output value for reading.
    //
    VdfVector *_GetOutputValueForReading(
        VdfExecutorBufferData *bufferData,
        const VdfMask &mask) const;

    // The current invalidation timestamp, recording the timestamp that was
    // applied to the last (if any) round of outputs traversed during 
    // invalidation.  This record and the timestamps on individual ExecutorData
    // objects are the keys to activating mung buffer locking.
    //
    VdfInvalidationTimestamp _invalidationTimestamp;
};

///////////////////////////////////////////////////////////////////////////////

template<typename DerivedClass>
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::GetInputValue(
    const VdfConnection &connection,
    const VdfMask &mask) const
{
    // For associated inputs, we need to grab the input value from the
    // associated output. This is because read/write buffers have been
    // prepared before the node callback is invoked.
    // Values for read outputs originate from the source output on the
    // input connection.

    const VdfInput &input = connection.GetTargetInput();
    const VdfOutput *ao = input.GetAssociatedOutput();

    if (!ao || input.GetNumConnections() != 1) {
        return GetOutputValueForReading(
            Base::_GetDataHandle(connection.GetSourceOutput().GetId()), mask);
    }

    // Note that read/write output values are always passed via the private
    // buffers. The only time read/writes are published is when they reach
    // an output that no longer passes the value, or when a kept buffer is
    // being published. Therefore, we always read output values for read/writes
    // out of the private buffer, if requested as an input value.

    const DataHandle dataHandle = Base::_GetDataHandle(ao->GetId());
    return Base::_IsValidDataHandle(dataHandle)
        ? _GetOutputValueForReading(
            Base::_GetPrivateBufferData(dataHandle), mask)
        : nullptr;
}

template<typename DerivedClass>
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::GetOutputValueForReading(
    const DataHandle dataHandle,
    const VdfMask &mask) const
{
    // Output values are always read from public buffers. When nodes are
    // evaluated, they mutate the private buffers, which are then published
    // as soon as the node completes. Note, however, that read/write buffers
    // will never be published until they reach on output that no longer passes
    // the value.

    if (!Base::_IsValidDataHandle(dataHandle)) {
        return nullptr;
    }

    // Attempt to read from the public buffer, first.
    VdfExecutorBufferData *publicData = Base::_GetPublicBufferData(dataHandle);
    if (VdfVector *value = _GetOutputValueForReading(publicData, mask)) {
        return value;
    }

    // Then, fall back to reading from the transferred data, if available.
    VdfExecutorBufferData *transferData =
        Base::_GetTransferredBufferData(dataHandle);
    return transferData 
        ? _GetOutputValueForReading(transferData, mask)
        : nullptr;
}

template<typename DerivedClass>
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::GetOrCreateOutputValueForWriting(
    const VdfOutput &output,
    const DataHandle handle) const
{
    // If the handle is not valid, we can't return a value for writing.
    if (!Base::_IsValidDataHandle(handle)) {
        return nullptr;
    }

    // Note that output values are always written to private buffers. These
    // private buffers will then be published once the node has completed
    // evaluation.

    VdfExecutorBufferData *bufferData = Base::_GetPrivateBufferData(handle);
    if (VdfVector *value = bufferData->GetExecutorCache()) {
        return value;
    }

    // Create a new output value, if there isn't one already available.
    return CreateOutputCache(output, bufferData);
}

template < typename DerivedClass >
void
VdfParallelExecutorDataManager<DerivedClass>::SetOutputValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    // Make sure the data manager is appropriately sized.
    Base::_Resize(output.GetNode().GetNetwork());

    // Mark the output as having been touched by evaluation, in order
    // for it to be considered by invalidation.
    const VdfId outputId = output.GetId();
    Base::_Touch(outputId);

    // Retrieve the data at the output
    const DataHandle handle = Base::_GetOrCreateDataHandle(outputId);
    VdfExecutorBufferData *privateBuffer = Base::_GetPrivateBufferData(handle);
    VdfExecutorBufferData *publicBuffer = Base::_GetPublicBufferData(handle);

    // Merge with existing data or replace?
    const VdfMask &publicMask = publicBuffer->GetExecutorCacheMask();
    const VdfVector *publicValue = publicBuffer->GetExecutorCache();
    const bool mergeData =
        publicValue &&
        !publicMask.IsEmpty() &&
        publicMask != mask;

    // Set the new output value, merging in the previously available public
    // data, if any.
    if (mergeData) {
        const VdfMask privateMask = publicMask | mask;
        VdfVector *outputValue = privateBuffer->CreateExecutorCache(
            output.GetSpec(), privateMask.GetBits());
        outputValue->Merge(*publicValue, publicMask - mask);
        outputValue->Merge(value, mask);
        privateBuffer->SetExecutorCacheMask(privateMask);
    } else {
        VdfVector *outputValue =
            privateBuffer->CreateExecutorCache(output.GetSpec());
        outputValue->Copy(value, mask);
        privateBuffer->SetExecutorCacheMask(mask);
    }

    // We just set the private buffer data, now let's publish it. Note, that
    // we can no longer modify the private or public buffer data after this
    // step, since it could lead to race conditions.
    Base::_PublishPrivateBufferData(handle);
}

template < typename DerivedClass >
bool
VdfParallelExecutorDataManager<DerivedClass>::TakeOutputValue(
    const VdfOutput &output,
    VdfVector *value,
    const VdfMask &mask)
{
    // Make sure the data manager is appropriately sized.
    Base::_Resize(output.GetNode().GetNetwork());

    // Mark the output as having been touched by evaluation, in order
    // for it to be considered by invalidation.
    const VdfId outputId = output.GetId();
    Base::_Touch(outputId);

    // Retrieve the data at the output
    const DataHandle handle = Base::_GetOrCreateDataHandle(outputId);

    // Attempt to transfer the value.
    return Base::_TransferBufferData(handle, value, mask);
}

template < typename DerivedClass >
void
VdfParallelExecutorDataManager<DerivedClass>::SetReferenceOutputValue(
    const VdfVector *sourceValue,
    const VdfId destOutputId) const
{
    const DataHandle handle = Base::_GetDataHandle(destOutputId);
    TF_DEV_AXIOM(Base::_IsValidDataHandle(handle));

    VdfExecutorBufferData *bufferData = Base::_GetPrivateBufferData(handle);  

    // XXX:cleanup This const_cast is real trouble.
    bufferData->YieldOwnership(const_cast<VdfVector *>(sourceValue));
}

template < typename DerivedClass >
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::CreateOutputCache(
    const VdfOutput &output,
    VdfExecutorBufferData *bufferData)
{
    TfAutoMallocTag2 tag(
        "Vdf", "VdfParallelExecutorDataManager::CreateOutputCache");

    // If the executor is providing its own cache-reuse mechanism, then
    // we assert that the cache is NULL before we get here.  Otherwise,
    // we will try to reuse whatever cache is there already.
    TF_DEV_AXIOM(!bufferData->GetExecutorCache());
    
    return bufferData->CreateExecutorCache(output.GetSpec());
}

template < typename DerivedClass >
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::CreateOutputCache(
    const VdfOutput &output,
    VdfExecutorBufferData *bufferData,
    const VdfMask::Bits &bits)
{
    TfAutoMallocTag2 tag(
        "Vdf", "VdfParallelExecutorDataManager::CreateOutputCache");

    // If the executor is providing its own cache-reuse mechanism, then
    // we assert that the cache is NULL before we get here.  Otherwise,
    // we will try to reuse whatever cache is there already.
    TF_DEV_AXIOM(!bufferData->GetExecutorCache());

    return bufferData->CreateExecutorCache(output.GetSpec(), bits);
}

template < typename DerivedClass >
void
VdfParallelExecutorDataManager<DerivedClass>::DuplicateOutputData(
    const VdfOutput &sourceOutput,
    const VdfOutput &destOutput)
{
    // Make sure the data manager is appropriately sized for us to copy
    // the source value to the destination output.
    Base::_Resize(destOutput.GetNode().GetNetwork());

    // Untouch the destination data, unless the source data has been touched.
    // This needs to happen even if we don't get a sourceHandle below.
    const VdfId destOutputId = destOutput.GetId();
    const VdfId sourceOutputId = sourceOutput.GetId();
    Base::_Untouch(destOutputId);
    if (Base::_Untouch(sourceOutputId)) {
        Base::_Touch(sourceOutputId);
        Base::_Touch(destOutputId);
    }

    // If the source output data exists, clone it to the destination
    // output data.
    const DataHandle sourceHandle = Base::_GetDataHandle(sourceOutputId);
    if (!Base::_IsValidDataHandle(sourceHandle)) {
        return;
    }

    // Get the destination data handle.
    const DataHandle destHandle = Base::_GetOrCreateDataHandle(destOutputId);

    // Clone the buffer data.
    Base::_GetPublicBufferData(sourceHandle)->Clone(
        Base::_GetPublicBufferData(destHandle));

    // Clone the invalidation data.
    Base::_GetInvalidationData(sourceHandle)->Clone(
        Base::_GetInvalidationData(destHandle));

    // Copy the invalidation timestamp.
    Base::_SetInvalidationTimestamp(
        destHandle, Base::_GetInvalidationTimestamp(sourceHandle));
}

template < typename DerivedClass >
bool
VdfParallelExecutorDataManager<DerivedClass>::IsOutputInvalid(
    const VdfId outputId,
    const VdfMask &invalidationMask) const
{
    // If the output has been touched by evaluation, it is valid.
    const bool wasTouched = Base::_IsTouched(outputId);
    if (wasTouched) {
        return false;
    }

    // If there is no data handle for the output, the output has never been
    // evaluated and therefore is still invalid.
    const DataHandle handle = Base::_GetDataHandle(outputId);
    if (!Base::_IsValidDataHandle(handle)) {
        return true;
    }

    // Let's check if the invalidation data is marked invalid for the given
    // mask.
    return Base::_GetInvalidationData(handle)->IsInvalid(
        invalidationMask, wasTouched);
}

template < typename DerivedClass >
bool 
VdfParallelExecutorDataManager<DerivedClass>::InvalidateOutput(
    const VdfOutput &output,
    const VdfMask &invalidationMask)
{
    // Untouch the output. If it has been touched, we need to creata a data
    // handle for it, even though it may have never been evaluated with this
    // data manager.
    const VdfId outputId = output.GetId();
    const bool wasTouched = Base::_Untouch(outputId);

    // Retrieve the data handle for the output.
    const DataHandle handle = wasTouched
        ? Base::_GetOrCreateDataHandle(outputId)
        : Base::_GetDataHandle(outputId);

    // Thou shalt not invalidate what has not been evaluated (unless it has
    // been touched, of course)!
    if (!Base::_IsValidDataHandle(handle)) {
        return false;
    }
    
    // Invalidate the output via the VdfExecutorInvalidationData. Make sure
    // to also untouch the output, if it has previously been touched by
    // evaluation.
    const bool didInvalidate = Base::_GetInvalidationData(handle)->Invalidate(
        invalidationMask, wasTouched);

    // If the output had previously been touched, and it has now been
    // invalidated. Make sure to invalidate the VdfExecutorBufferData.
    if (didInvalidate) {

        // Update the invalidation timestamp, by applying the timestamp from
        // the data manager to the timestamp stored at the output.
        Base::_SetInvalidationTimestamp(handle, GetInvalidationTimestamp());

        // Retrieve the buffer data stored at the output.
        VdfExecutorBufferData *bufferData = Base::_GetPublicBufferData(handle);

        // We only need to invalidate the executor cache if the buffer has one.
        if (!bufferData->GetExecutorCacheMask().IsEmpty()) {

            // If this is an output in the pool, let's apply sparse
            // invalidation.
            if (Vdf_IsPoolOutput(output)) {

                // Sparsely invalidate the executor cache mask, using the bits
                // in the invalidation mask. During a steady-state mung, the
                // cache and invalidation masks will likely always be the same
                // across iterations, so we memoize this operation.
                //
                // XXX: Memoize!
                //
                VdfMask newCacheMask =
                    bufferData->GetExecutorCacheMask() - invalidationMask;

                // If the new cache mask is now all-zeros, remove the cache
                // entirely, otherwise simply set the new cache mask.
                if (newCacheMask.IsAllZeros()) {
                    bufferData->ResetExecutorCache();
                } else {
                    bufferData->SetExecutorCacheMask(newCacheMask);
                }
            }

            // Otherwise, we simply remove the cache entirely.
            else {
                bufferData->ResetExecutorCache();
            }
        }

        // Also clear any transferred data.
        Base::_ResetTransferredBufferData(handle);

        // We did some invalidation.
        return true;
    }
    
    // Nothing to invalidate.
    return false;
}

template < typename DerivedClass >
void 
VdfParallelExecutorDataManager<DerivedClass>::Touch(
    const VdfOutput &output) const
{
    Base::_Touch(output.GetId());
}

template < typename DerivedClass >
bool
VdfParallelExecutorDataManager<DerivedClass>::HasInvalidationTimestampMismatch(
    const DataHandle &sourceHandle,
    const DataHandle &destHandle) const
{
    // For this method to return true, indicating that the source output should
    // be locked for mung buffer locking, the invalidation timestamp stored in
    // the destination data object must match the current invalidation
    // timestamp. Furthermore, the source data object must have an invalidation
    // timestamp different from our current invalidation timestamp.
    // Essentially, this means that in the latest round of invalidation, the
    // destination output has been invalidated, whereas the source output
    // remained untouched.
    // If this method returns true, the cache at the source output can be
    // mung buffer locked, because it won't receive invalidation, whereas any
    // output below (and including) the destination output will!

    return
        Base::_IsValidDataHandle(sourceHandle) &&
        Base::_IsValidDataHandle(destHandle) &&
        Base::_GetInvalidationTimestamp(sourceHandle) !=
            _invalidationTimestamp &&
        Base::_GetInvalidationTimestamp(destHandle) == 
            _invalidationTimestamp;
}

template < typename DerivedClass >
VdfVector *
VdfParallelExecutorDataManager<DerivedClass>::_GetOutputValueForReading(
    VdfExecutorBufferData *bufferData,
    const VdfMask &mask) const
{
    // We have the output value if
    //  o The output data exists
    //  o The cache is not empty
    //  o One of the following is true:
    //     o The request mask has no bits set because we ask for an
    //       attribute with shape of length zero (e.g., points attribute
    //       with zero points in it.
    //    or
    //     o The output is not dirty and
    //     o The computed mask covers what is requested.

    VdfVector *value = bufferData->GetExecutorCache();
    const bool hasValue =
        value &&
        (mask.IsAllZeros() ||
            (bufferData->GetExecutorCacheMask().IsAnySet() &&
             bufferData->GetExecutorCacheMask().Contains(mask)));

    return hasValue ? value : nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
