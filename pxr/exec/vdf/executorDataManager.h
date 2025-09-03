//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_DATA_MANAGER_H
#define PXR_EXEC_VDF_EXECUTOR_DATA_MANAGER_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/executorBufferData.h"
#include "pxr/exec/vdf/executorDataManagerInterface.h"
#include "pxr/exec/vdf/executorInvalidationData.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfMask;
class VdfNetwork;
class VdfVector;

/// Forward definition of the traits class, which will be specialized by the
/// derived data manager implementation, and contain at least a type definition
/// for the DataHandle.
template<typename DerivedClass> struct Vdf_ExecutorDataManagerTraits;

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorDataManager
///
/// \brief This class provides functionality to manage the executor specific
/// data associated with each output in the network.
///
/// The data manager implementations use static polymorphism to dispatch the
/// API on VdfExecutorDataManager. See Vdf_ExecutorDataManagerInterface for
/// which methods are expected to be implemented by the derived classes.
///

template<typename DerivedClass>
class VdfExecutorDataManager :
    public Vdf_ExecutorDataManagerInterface<
        DerivedClass,
        typename Vdf_ExecutorDataManagerTraits<DerivedClass>::DataHandle> 
{
public:

    /// The data handle type defined via the specialized traits class.
    ///
    typedef
        typename Vdf_ExecutorDataManagerTraits<DerivedClass>::DataHandle
        DataHandle;

    /// The base class type.
    ///
    typedef
        Vdf_ExecutorDataManagerInterface<DerivedClass, DataHandle>
        Base;

    /// \name Cache Management
    /// @{

    /// Returns the input value flowing across the given \p connection with the
    /// given \p mask.  If the the cache is not valid, or if the cache does not
    /// contain all the elements in \p mask, returns NULL.  If no output data
    /// exists for \p output, it will not be created.
    ///
    const VdfVector *GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const;

    /// Returns the cached value for a given \p output and \p mask.  If the the
    /// cache is not valid, or if the cache does not contain all the elements in
    /// \p mask, returns NULL.  If no output data exists for \p output, it will
    /// not be created.
    ///
    VdfVector *GetOutputValueForReading(
        const DataHandle handle,
        const VdfMask &mask) const;

    /// Returns a new or existing output value for writing data into.
    ///
    VdfVector *GetOrCreateOutputValueForWriting(
        const VdfOutput &output,
        const DataHandle handle) const;

    /// Sets the cached value for a given \p output, creating the output cache
    /// if necessary.
    ///
    /// If the output already contains data, it will be merged with the new
    /// data as indicated by \p value and \p mask.
    ///
    void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask);

    /// Transfers ownership of \p value to the given \p output, returning
    /// \c true if the transfer of ownership succeeds. If successful, the
    /// data manager assumes responsibility for the lifetime of \p value.
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
    VdfVector *CreateOutputCache(
        const VdfOutput &output,
        VdfExecutorBufferData *bufferData) const;

    /// Duplicates the output data associated with \p sourceOutput and copies
    /// it to \p destOutput.
    ///
    void DuplicateOutputData(
        const VdfOutput &sourceOutput,
        const VdfOutput &destOutput);

    /// Marks the output whose data is \p bufferData as computed for the 
    /// entries in \p mask.
    ///
    void SetComputedOutputMask(
        VdfExecutorBufferData *bufferData, 
        const VdfMask &mask) {
        bufferData->SetExecutorCacheMask(mask);
    }

    /// @}


    /// \name Invalidation
    /// @{
    
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
    bool InvalidateOutput(const VdfOutput &output,
                          const VdfMask &invalidationMask);

    /// Marks the data at the given \p output as having been touched by
    /// evaluation.
    ///
    void Touch(const VdfOutput &output) const;

    /// Increments the the current invalidation timestamp on this executor.
    ///
    void UpdateInvalidationTimestamp(VdfInvalidationTimestamp timestamp) {
        _invalidationTimestamp = timestamp;
    }

    /// Returns the current invalidation timestamp on this executor.
    ///
    VdfInvalidationTimestamp GetInvalidationTimestamp() const {
        return _invalidationTimestamp;
    }

    /// Returns \c true, if the invalidation timestamps between \p sourceData
    /// and \p destData do not match, i.e. the source output should be
    /// mung buffer locked.
    bool HasInvalidationTimestampMismatch(
        const DataHandle sourceHandle,
        const DataHandle destHandle) const;

    /// @}


    /// \name Buffer Passing
    /// @{

    /// This method is called to pass a buffer from \p fromOutput to \p
    /// toOutput.  The \p keepMask is the mask of elements that \p fromOutput
    /// should keep after the pass.
    ///
    /// Returns the cache data that ends up in \p toOutput.
    ///
    VdfVector *PassBuffer(
        const VdfOutput &fromOutput,
        VdfExecutorBufferData *fromBuffer,
        const VdfOutput &toOutput,
        VdfExecutorBufferData *toBuffer,
        const VdfMask &keepMask);

    /// @}

protected:

    /// Constructor. Note that the invalidation timestamp is initialized to be
    /// ahead of the initial timestamp in the VdfExecutorData. This is to
    /// allow the executor to correctly identify when data has never been
    /// invalidated before.
    ///
    VdfExecutorDataManager() :
        _invalidationTimestamp(
            VdfExecutorInvalidationData::InitialInvalidationTimestamp + 1)
    {}

    /// Prevent destruction via base class pointers (static polymorphism only).
    ///
    ~VdfExecutorDataManager() = default;

private:

    // The current invalidation timestamp, recording the timestamp that was
    // applied to the last (if any) round of outputs traversed during 
    // invalidation.  This record and the timestamps on individual ExecutorData
    // objects are the keys to activating mung buffer locking.
    VdfInvalidationTimestamp _invalidationTimestamp;
};

///////////////////////////////////////////////////////////////////////////////

template<typename DerivedClass>
const VdfVector *
VdfExecutorDataManager<DerivedClass>::GetInputValue(
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
    const VdfOutput &readOutput = ao && input.GetNumConnections() == 1
        ? *ao
        : connection.GetSourceOutput();
    return GetOutputValueForReading(
        Base::_GetDataHandle(readOutput.GetId()), mask);
}

template<typename DerivedClass>
VdfVector *
VdfExecutorDataManager<DerivedClass>::GetOutputValueForReading(
    const DataHandle handle, 
    const VdfMask &mask) const
{
    // We have the output value if
    //  o The output data exists
    //  o The cache is not empty
    //  o One of the following is true:
    //     o The request mask has no bits set because we ask for an
    //       attribute with shape of length zero (e.g., points attribute
    //       with zero points in it).
    //    or
    //     o The output is not dirty and
    //     o The computed mask covers what is requested.

    const bool hasValue =
        Base::_IsValidDataHandle(handle) && 
        Base::_GetBufferData(handle)->GetExecutorCache() &&
        (mask.IsAllZeros() ||
            (Base::_GetBufferData(handle)
                ->GetExecutorCacheMask().IsAnySet() &&
             Base::_GetBufferData(handle)
                ->GetExecutorCacheMask().Contains(mask)));

    return hasValue
        ? Base::_GetBufferData(handle)->GetExecutorCache()
        : NULL;
}

template<typename DerivedClass>
VdfVector *
VdfExecutorDataManager<DerivedClass>::GetOrCreateOutputValueForWriting(
    const VdfOutput &output,
    const DataHandle handle) const
{
    // If the specified handle is not valid, return a null pointer.
    if (!Base::_IsValidDataHandle(handle)) {
        return nullptr;
    }

    // Return the output value, if available.
    VdfExecutorBufferData *bufferData = Base::_GetBufferData(handle);
    if (VdfVector *value = bufferData->GetExecutorCache()) {
        return value;
    }

    // Create a new output value, if there isn't one already available.
    return CreateOutputCache(output, bufferData);
}

template<typename DerivedClass>
void
VdfExecutorDataManager<DerivedClass>::SetOutputValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    // Make sure the data manager is appropriately sized.
    Base::_Resize(output.GetNode().GetNetwork());

    // Retrieve the vector at the output
    const DataHandle handle = Base::_GetOrCreateDataHandle(output.GetId());
    VdfExecutorBufferData *bufferData = Base::_GetBufferData(handle);

    // If there is no output value available, create a new one
    VdfVector *outputValue = bufferData->GetExecutorCache();
    if (!outputValue) {
        // This also touches the new output data
        outputValue = CreateOutputCache(output, bufferData);
    }

    // Mark the output as having been touched by evaluation, in order
    // for it to be considered by invalidation.
    Base::_Touch(handle);

    // Merge with existing data or replace?
    const VdfMask &cacheMask = bufferData->GetExecutorCacheMask();
    const bool mergeData =
        !outputValue->IsEmpty() &&
        !cacheMask.IsEmpty() &&
        cacheMask != mask;

    // Set the new output value, by either merging into the existing vector,
    // or simply replacing it all together
    if (mergeData) {
        outputValue->Merge(value, mask);
    } else {
        outputValue->Copy(value, mask);
    }

    // Set the new executor cache mask
    SetComputedOutputMask(bufferData, mergeData ? cacheMask | mask : mask);
}

template<typename DerivedClass>
bool
VdfExecutorDataManager<DerivedClass>::TakeOutputValue(
    const VdfOutput &output,
    VdfVector *value,
    const VdfMask &mask)
{
    // Make sure the data manager is appropriately sized.
    Base::_Resize(output.GetNode().GetNetwork());

    // Retrieve the vector at the output
    const DataHandle handle = Base::_GetOrCreateDataHandle(output.GetId());
    VdfExecutorBufferData *bufferData = Base::_GetBufferData(handle);

    // Return if there is already a cache associated with the output.
    if (bufferData->GetExecutorCache()) {
        return false;
    }

    // Otherwise, transfer ownership of the value into the buffer.
    bufferData->TakeOwnership(value);
    bufferData->SetExecutorCacheMask(mask);

    // Mark the output as having been touched by evaluation, in order
    // for it to be considered by invalidation.
    Base::_Touch(handle);

    // Successfully transferred the value.
    return true;
}

template<typename DerivedClass>
void
VdfExecutorDataManager<DerivedClass>::SetReferenceOutputValue(
    const VdfVector *sourceValue,
    const VdfId destOutputId) const
{
    const DataHandle handle = Base::_GetDataHandle(destOutputId);
    TF_DEV_AXIOM(Base::_IsValidDataHandle(handle));

    VdfExecutorBufferData *bufferData = Base::_GetBufferData(handle);

    // XXX:cleanup This const_cast is real trouble.
    bufferData->YieldOwnership(const_cast<VdfVector *>(sourceValue));
}

template<typename DerivedClass>
VdfVector *
VdfExecutorDataManager<DerivedClass>::CreateOutputCache(
    const VdfOutput &output,
    VdfExecutorBufferData *bufferData) const
{
    // If the executor is providing its own cache-reuse mechanism, then
    // we assert that the cache is NULL before we get here.  Otherwise,
    // we will try to reuse whatever cache is there already.
    TF_DEV_AXIOM(!bufferData->GetExecutorCache());
    
    // This storage is freed when bufferData is destructed.
    return bufferData->CreateExecutorCache(output.GetSpec());
}

template<typename DerivedClass>
void
VdfExecutorDataManager<DerivedClass>::DuplicateOutputData(
    const VdfOutput &sourceOutput,
    const VdfOutput &destOutput)
{
    // If the source output data exists, clone it to the destination
    // output data.
    const DataHandle sourceHandle = Base::_GetDataHandle(sourceOutput.GetId());
    if (!Base::_IsValidDataHandle(sourceHandle)) {
        return;
    }

    // Make sure the data manager is appropriately sized for us to copy
    // the source value to the destination output.
    Base::_Resize(destOutput.GetNode().GetNetwork());

    // Get the destination data handle.
    const DataHandle destHandle =
        Base::_GetOrCreateDataHandle(destOutput.GetId());

    // Clone the buffer data.
    Base::_GetBufferData(sourceHandle)->Clone(Base::_GetBufferData(destHandle));

    // Clone the invalidation data.
    Base::_GetInvalidationData(sourceHandle)->Clone(
        Base::_GetInvalidationData(destHandle));

    // Copy the invalidation timestamp.
    Base::_SetInvalidationTimestamp(
        destHandle, Base::_GetInvalidationTimestamp(sourceHandle));

    // Untouch the destination data, unless the source data has been touched.
    Base::_Untouch(destHandle);
    if (Base::_Untouch(sourceHandle)) {
        Base::_Touch(sourceHandle);
        Base::_Touch(destHandle);
    }

    // Clear the SMBL data, if any.
    if (VdfSMBLData *smblData = Base::_GetSMBLData(destHandle)) {
        smblData->Clear();
    }
}

template<typename DerivedClass>
bool
VdfExecutorDataManager<DerivedClass>::IsOutputInvalid(
    const VdfId outputId,
    const VdfMask &invalidationMask) const
{
    // If there is no data handle for the given output, it cannot possibly have
    // been computed, therefore it is still invalid.
    const DataHandle handle = Base::_GetDataHandle(outputId);
    if (!Base::_IsValidDataHandle(handle)) {
        return true;
    }

    // If the output has been touched by evaluation, it is valid. If the output
    // has not been touched, check if the given mask is marked as invalid in the
    // invalidation data.
    const bool wasTouched = Base::_IsTouched(handle);
    return
        !wasTouched &&
        Base::_GetInvalidationData(handle)->IsInvalid(
            invalidationMask, wasTouched);    
}

template<typename DerivedClass>
bool 
VdfExecutorDataManager<DerivedClass>::InvalidateOutput(
    const VdfOutput &output,
    const VdfMask &invalidationMask)
{
    // Retrieve the data handle for the output.
    const DataHandle handle = Base::_GetDataHandle(output.GetId());

    // Thou shalt not invalidate what has not been evaluated!
    if (!Base::_IsValidDataHandle(handle)) {
        return false;
    }
    
    // Invalidate the output via the VdfExecutorInvalidationData. Make sure
    // to also untouch the output, if it has previously been touched by
    // evaluation.
    const bool didInvalidate = Base::_GetInvalidationData(handle)->Invalidate(
        invalidationMask, Base::_Untouch(handle));

    // If the output had previously been touched, and it has now been
    // invalidated. Make sure to invalidate the VdfExecutorBufferData.
    if (didInvalidate) {

        // Update the invalidation timestamp, by applying the timestamp from
        // the data manager to the timestamp stored at the output.
        Base::_SetInvalidationTimestamp(handle, GetInvalidationTimestamp());

        // Retrieve the buffer data stored at the output.
        VdfExecutorBufferData *bufferData = Base::_GetBufferData(handle);

        // If this is an output in the pool, let's apply sparse invalidation.
        if (Vdf_IsPoolOutput(output)) {

            // Sparsely invalidate the executor cache mask, using the bits in
            // the invalidation mask. During a steady-state mung, the cache and
            // invalidation masks will likely always be the same across
            // iterations, so we memoize this operation.
            //
            // XXX: Don't always create SMBL data for this. Store the memoized
            //      result in VdfExecutorInvalidationData?
            VdfMask newCacheMask =
                Base::_GetOrCreateSMBLData(handle)->InvalidateCacheMask(
                    bufferData->GetExecutorCacheMask(),
                    invalidationMask);

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

        // We did some invalidation.
        return true;
    }
    
    // Nothing to invalidate.
    return false;
}

template<typename DerivedClass>
void
VdfExecutorDataManager<DerivedClass>::Touch(const VdfOutput &output) const
{
    Base::_Touch(Base::_GetOrCreateDataHandle(output.GetId()));
}

template<typename DerivedClass>
VdfVector *
VdfExecutorDataManager<DerivedClass>::PassBuffer(
    const VdfOutput &fromOutput,
    VdfExecutorBufferData *fromBuffer,
    const VdfOutput &toOutput,
    VdfExecutorBufferData *toBuffer,
    const VdfMask &keepMask)
{
    // If we don't have a cache here, there is nothing to pass.
    // It is up to the user to handle this case correctly.  We wouldn't
    // normally expect to get into this case unless a speculating executor is
    // trying to read from its parent executor.
    if (!fromBuffer->GetExecutorCache()) {
        return nullptr;
    }

    // Swap the from- and toBuffers.
    VdfVector *result = toBuffer->SwapExecutorCache(fromBuffer);

    if (keepMask.IsEmpty()) {

        // If we don't need to keep anything, then it's a straight pass through.
        // Simply make sure that there is no cache stored at the fromBuffer.
        fromBuffer->ResetExecutorCache();

    } else {
        TfAutoMallocTag2 tag(
            "Vdf", "VdfExecutorDataManager<DerivedClass>::PassBuffer (keep)");

        // Create a cache in the fromOutput. This is where we store the
        // kept value.
        VdfVector *keptValue = CreateOutputCache(fromOutput, fromBuffer);

        // Copy the subset that we want fromOutput to keep. Note, that it is
        // okay to keep an empty buffer. That just means that the schedule has
        // determined that a buffer must reside at this output, in order for
        // all-zero mask cache lookups to return a valid vector.
        if (keepMask.IsAnySet()) {
            keptValue->Copy(*result, keepMask);
        }

        // What's set in the keepMask is what remains cached at fromBuffer.
        fromBuffer->SetExecutorCacheMask(keepMask);

    }

    // Return the value now stored at toOutput.
    return result;
}

template<typename DerivedClass>
bool
VdfExecutorDataManager<DerivedClass>::HasInvalidationTimestampMismatch(
    const DataHandle sourceHandle,
    const DataHandle destHandle) const 
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
        Base::_GetInvalidationTimestamp(destHandle) == 
            _invalidationTimestamp &&
        Base::_GetInvalidationTimestamp(sourceHandle) !=
            _invalidationTimestamp;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
