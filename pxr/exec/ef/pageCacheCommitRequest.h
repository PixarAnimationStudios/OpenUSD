//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_COMMIT_REQUEST_H
#define PXR_EXEC_EF_PAGE_CACHE_COMMIT_REQUEST_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"
#include "pxr/exec/ef/inputValueBlock.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class Ef_OutputValueCache;
class EfPageCacheStorage;
class VdfExecutorInterface;
class VdfRequest;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfPageCacheCommitRequest
///
/// \brief This object signifies an intent to commit data to a page cache. It
///        ensures that the page exists and that it is ready to accept the
///        data. The object also stores an input value block, which denotes
///        the cache page to be used for the commit operation. 
///
class EfPageCacheCommitRequest
{
public:
    /// Constructor.
    ///
    /// Creates a new commit request with the specified input values, denoting
    /// the cache page (key), as well as the page cache storage to which the
    /// data will be committed.
    ///
    EF_API
    EfPageCacheCommitRequest(
        const EfInputValueBlock &inputs,
        EfPageCacheStorage *storage);

    /// Destructor.
    ///
    EF_API
    ~EfPageCacheCommitRequest();

    /// Returns the input value block containing the key output value.
    ///
    const EfInputValueBlock &GetInputs() const {
        return _inputs;
    }

    /// Returns the output value cache where values will be committed to. 
    ///
    Ef_OutputValueCache *GetCache() const {
        return _cache;
    }

    /// Returns \c true if any output in the specified \c request is
    /// still not cached.
    ///
    EF_API
    bool IsUncached(const VdfRequest &request) const;

    /// Returns the subset of \p request, which is still not cached.
    ///
    EF_API
    VdfRequest GetUncached(const VdfRequest &request) const;

    /// Commits data for the outputs denoted by the \p request to the
    /// cache, reading their values from the specified \p executor.
    ///
    /// Returns \c true if all the data has been cached.
    ///
    /// Sets \p bytesCommitted (which must be non-null) to the size of the
    /// stored data, in bytes.
    ///
    EF_API
    bool Commit(
        const VdfExecutorInterface &executor,
        const VdfRequest &request,
        size_t *bytesCommitted);

private:
    // The input value block containing the key output value.
    EfInputValueBlock _inputs;

    // The output-to-value cache on the page indexed by the key value.
    Ef_OutputValueCache *_cache;

    // A pointer to the page cache storage class.
    EfPageCacheStorage *_storage;

};

/// A vector of page cache commit requests.
///
typedef std::vector<EfPageCacheCommitRequest> EfPageCacheCommitRequestVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
