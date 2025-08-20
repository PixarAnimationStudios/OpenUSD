//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/maskRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

// Initial bucket array size; must be a power of 2.
//
// As of Dec 2014, a typical character produces a few thousand masks but, in
// 2018, we're striping across 64 tables so start each table with 16 entries.
//
static const size_t _InitialBucketCount = 1 << 4;

Vdf_MaskRegistry::Vdf_MaskRegistry()
    : _bucketMask(_InitialBucketCount-1)
    , _buckets(new _BitsImpl*[_InitialBucketCount]())
    , _nodeCount(0)
{
}

Vdf_MaskRegistry::~Vdf_MaskRegistry()
{
    size_t bucketCount = _GetBucketCount();
    for (size_t i=0; i<bucketCount; ++i) {
        _BitsImpl *n = _buckets[i];
        while (n) {
            _BitsImpl *next = n->_next;
            n->~_BitsImpl();
            n = next;
        }
    }
}

size_t
Vdf_MaskRegistry::GetSize()
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);
    return _nodeCount;
}

void
Vdf_MaskRegistry::_Rehash()
{
    size_t oldBucketsCount = _GetBucketCount();
    std::unique_ptr<_BitsImpl*[]> oldBuckets;
    oldBuckets.swap(_buckets);

    // Increase bucket table size to the next power of 2.
    _bucketMask = (_bucketMask << 1) + 1;
    _buckets.reset(new _BitsImpl*[_GetBucketCount()]());

    // Redistribute nodes into the new bucket array.
    for (size_t i=0; i<oldBucketsCount; ++i) {
        _BitsImpl *n = oldBuckets[i];

        while (n) {
            const size_t hash = n->GetHash();
            size_t idx = _ComputeBucketIndex(hash);

            _BitsImpl ** bucketHead = &(_buckets[idx]);
            _BitsImpl *next = n->_next;

            // In _Emplace, we made the claim that newer entries should appear
            // earlier in the bucket.  Inserting into the bucket head here
            // will implicitly reverse the ordering of entries that share a
            // bucket both before and after rehashing.  This situation should
            // be unlikely in practice, because we expect rehashing to usually
            // result in one entry per bucket.
            n->_next = *bucketHead;
            *bucketHead = n;
            n = next;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
