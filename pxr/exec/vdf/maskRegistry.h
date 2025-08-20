//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASK_REGISTRY_H
#define PXR_EXEC_VDF_MASK_REGISTRY_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include <tbb/spin_rw_mutex.h>

#include <algorithm>
#include <memory>
#include <new>

PXR_NAMESPACE_OPEN_SCOPE

/// A flyweighting table customized for VdfMask.
///
/// Vdf_MaskRegistry implements a hash table customized specifically for
/// flyweighting performance.  Unlike the more general std & boost associative
/// containers, Vdf_MaskRegistry supports only find+insertion and deletion of
/// values.
class Vdf_MaskRegistry
{
    typedef VdfMask::_BitsImpl _BitsImpl;

public:
    Vdf_MaskRegistry();
    ~Vdf_MaskRegistry();

    // Non-copyable
    Vdf_MaskRegistry(const Vdf_MaskRegistry &) = delete;
    Vdf_MaskRegistry& operator=(const Vdf_MaskRegistry &) = delete;

    // Non-movable
    Vdf_MaskRegistry(Vdf_MaskRegistry &&) = delete;
    Vdf_MaskRegistry& operator=(Vdf_MaskRegistry &&) = delete;

    /// Discard this number of least significant bits when computing the bucket
    /// index. These bits will instead be used to distribute entries between a
    /// fixed number of registries.
    static constexpr size_t DiscardBucketBits = 6;

    /// If \p *bits is found, increment its refcount and return a pointer to
    /// the existing entry.
    ///
    /// Otherwise, move \p *bits into a new entry and return a pointer to
    /// the new entry (with its ref count initialized to 1.)
    inline _BitsImpl *FindOrEmplace(VdfMask::Bits &&bits, size_t hash);

    /// If \p bits is found, increment its refcount and return a pointer to
    /// the existing entry.
    ///
    /// Otherwise, copy \p bits into a new entry and return a pointer to
    /// the new entry (with its ref count initialized to 1.)
    inline _BitsImpl *FindOrInsert(const VdfMask::Bits &bits, size_t hash);

    /// Delete the entry pointed to by \p target.
    ///
    /// \warning Invokes undefined behavior if \p target does not exist in
    ///          this registry.
    inline void Erase(_BitsImpl *target, size_t hash);

    /// Return the current number of entries in the registry.
    ///
    /// For test use only.
    ///
    size_t GetSize();

private:

    // Return a pointer to an existing entry for \p bits whose hash is \p hash.
    // If no entry exists, return NULL.
    inline _BitsImpl *_Find(const VdfMask::Bits &bits, size_t hash);

    // Construct a new hash table node, moving \p *bits into the newly
    // constructed node using \p hash to determine the target bucket.
    inline _BitsImpl *_Emplace(VdfMask::Bits &&bits, size_t hash);

    // Unlinks the entry pointed to by \p target from the bucket. Returns a
    // pointer to the unlinked entry, or nullptr if no entry has been unlinked.
    inline _BitsImpl *_Unlink(_BitsImpl *target, size_t hash);

    // Return the bucket index for the entry corresponding to \p hash.
    inline size_t _ComputeBucketIndex(size_t hash) const;

    // Return the number of buckets in the bucket array.
    inline size_t _GetBucketCount() const;

    // Grow the bucket array to a larger size and redistribute nodes into
    // the new array.
    void _Rehash();

private:
    // Masking is used for fast modulo len(_buckets) when computing bucket
    // indices for a value.  _bucketMask must be len(_buckets) - 1 and
    // the len(_buckets) must be a power of two.
    size_t _bucketMask;
    std::unique_ptr<_BitsImpl*[]> _buckets;

    // Number of entries in the hash table.
    size_t _nodeCount;

    // Guards the hash table buckets.
    // Public methods must acquire the mutex; all private methods assume that
    // it is already held.
    tbb::spin_rw_mutex _mutex;
};


Vdf_MaskRegistry::_BitsImpl *
Vdf_MaskRegistry::FindOrEmplace(VdfMask::Bits &&bits, size_t hash)
{
    // Acquire the reader lock.
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);

    // See if we can find the bits in the table.
    if (Vdf_MaskRegistry::_BitsImpl *n = _Find(bits, hash)) {
        return n;
    }

    // If we did not find the bits in the table, we need to upgrade the lock
    // to a writer lock. If doing so ended up releasing and re-acquiring the 
    // lock we need to check the table again, as someone else could have
    // inserted the entry in the meantime.
    if (!lock.upgrade_to_writer()) {
        if (Vdf_MaskRegistry::_BitsImpl *n = _Find(bits, hash)) {
            return n;
        }
    }

    // Move the bits into the table.
    return _Emplace(std::move(bits), hash);
}

Vdf_MaskRegistry::_BitsImpl *
Vdf_MaskRegistry::FindOrInsert(const VdfMask::Bits &bits, size_t hash)
{
    // Acquire the reader lock.
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);

    // See if we can find the bits in the table.
    if (Vdf_MaskRegistry::_BitsImpl *n = _Find(bits, hash)) {
        return n;
    }

    // If we did not find the bits in the table, we need to upgrade the lock
    // to a writer lock. If doing so ended up releasing and re-acquiring the 
    // lock we need to check the table again, as someone else could have
    // inserted the entry in the meantime.
    if (!lock.upgrade_to_writer()) {
        if (Vdf_MaskRegistry::_BitsImpl *n = _Find(bits, hash)) {
            return n;
        }
    }

    // Copy the bits here and move the copy into the new node.
    return _Emplace(VdfMask::Bits(bits), hash);
}

Vdf_MaskRegistry::_BitsImpl *
Vdf_MaskRegistry::_Find(const VdfMask::Bits &bits, size_t hash)
{
    // Find the bucket that will contain bits (whether or not bits is actually
    // present.)
    size_t idx = _ComputeBucketIndex(hash);
    _BitsImpl *n = _buckets[idx];

    // Search the bucket
    while (n) {
        if (n->Get() == bits) {
            // We found an entry, so increment its ref count.
            //
            // Relaxed memory ordering is sufficient here because _mutex
            // serializes accesses via the registry.  Even in the case where
            // we find an about-to-be-deleted node, the deleter must also
            // first acquire the registry mutex.
            if (ARCH_UNLIKELY(n->_refCount.fetch_add(
                                  1, std::memory_order_relaxed) == 0)) {
                // If we observe an entry about to be deleted, increment the
                // resurrection counter so that the corresponding erase
                // doesn't delete it out from under us.
                //
                // Note that with multiple racing find/erases, each _Find call
                // that revives n should increment the counter by one.
                n->_resurrectionCount.fetch_add(1, std::memory_order_relaxed);                
            }

            return n;
        }
        n = n->_next;
    }

    return NULL;
}

inline
VdfMask::_BitsImpl::_BitsImpl(
    _BitsImpl *next, size_t hash, VdfMask::Bits &&bits)
    : _next(next)
    , _hash(hash)
    , _bits(std::move(bits))
    , _refCount(1)
    , _resurrectionCount(0)
    , _isImmortal(_bits.GetSize() <= 8)
{
}

inline
size_t
VdfMask::_BitsImpl::GetHash() const
{
    return _hash;
}

Vdf_MaskRegistry::_BitsImpl *
Vdf_MaskRegistry::_Emplace(VdfMask::Bits &&bits, size_t hash)
{
    // Rehash when load factor exceeds 1.0.
    if (_nodeCount >= _GetBucketCount()) {
        _Rehash();
    }

    size_t idx = _ComputeBucketIndex(hash);

    // Insert a new node as one doesn't exist already.  New entries are
    // inserted as the first element in the bucket based on the hypothesis
    // that newly inserted entries are more likely to be looked up again than
    // older entries in the same bucket.
    _BitsImpl **bucketHead = &(_buckets[idx]);
    _BitsImpl *newNode = new _BitsImpl(*bucketHead, hash, std::move(bits));
    *bucketHead = newNode;
    ++_nodeCount;

    return newNode;
}

void
Vdf_MaskRegistry::Erase(_BitsImpl *target, size_t hash)
{
    // Will point to the unlinked node.
    _BitsImpl *node = nullptr;

    {
        // Acquire the writer lock only to unlink the node.
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ true);

        // Unlink the node.
        node = _Unlink(target, hash);
    }

    // Delete the unlinked node.
    if (node) {
        delete node;
    }
}

Vdf_MaskRegistry::_BitsImpl *
Vdf_MaskRegistry::_Unlink(_BitsImpl *target, size_t hash)
{
    // In the highly unlikely event that we revived an entry during a
    // erase/find race, we decrement the resurrection counter instead of
    // actually deleting the node. This ensures that we don't
    // double-erase due to an ABA problem with the node's reference count.
    if (target->_resurrectionCount.load(std::memory_order_relaxed)) {
        target->_resurrectionCount.fetch_sub(1, std::memory_order_relaxed);
        return nullptr;
    }

    size_t idx = _ComputeBucketIndex(hash);

    // Pointer to the "next" field of the predecessor of the current node.
    _BitsImpl **pred = &(_buckets[idx]);
    _BitsImpl *n = _buckets[idx];

    // No need to actually compare the keys, pointer value is enough because
    // target point to an entry in the bucket.
    while (n != target) {
        pred = &(n->_next);
        n = n->_next;
    }

    // Relink the list to omit n.
    *pred = n->_next;

    --_nodeCount;

    // Return the unlinked entry.
    return n;
}

size_t
Vdf_MaskRegistry::_ComputeBucketIndex(size_t hash) const
{
    // Use masking for a fast modulo power-of-2 of the hash value.
    return (hash >> DiscardBucketBits) & _bucketMask;
}

size_t
Vdf_MaskRegistry::_GetBucketCount() const
{
    // The mask implies the bucket count.  We keep the mask instead of an
    // explicit count because bucket indexing is the more common operation.
    return _bucketMask + 1;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
