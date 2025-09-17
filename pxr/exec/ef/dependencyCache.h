//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_DEPENDENCY_CACHE_H
#define PXR_EXEC_EF_DEPENDENCY_CACHE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/base/tf/bits.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

////////////////////////////////////////////////////////////////////////////////
///
/// \class EfDependencyCache
///
/// Caches output traversals by associating an input request with a set of
/// stored output dependencies, as determined by a predicate function.
///
/// The traversals in this cache are invalidated by calling the
/// WillDeleteConnection and DidConnect methods in response to network edits
/// (WillDeleteNode, WillDeleteConnection, DidConnect). Invalidation is
/// optionally sparse, depending on the value of the \p updateIncrementally flag
/// passed to FindOutputs and FindNodes.
///
class EfDependencyCache
{
public:
    /// The predicate function that determines the cached dependencies.
    ///
    /// Takes the node currently being visited, as well as a result map to
    /// insert output and node dependencies into.
    ///
    /// Returns \c false in order to stop the traversal at the current branch,
    /// and \c true to continue.
    ///
    using PredicateFunction = 
        bool (*) (
            const VdfNode &node,
            VdfOutputToMaskMap *,
            std::vector<const VdfNode *> *);

    /// Constructor.
    ///
    EF_API
    explicit EfDependencyCache(PredicateFunction predicate);

    /// Destructor.
    ///
    EF_API
    ~EfDependencyCache();

    /// Find the output dependencies associated with the given request.
    /// 
    /// Set \p updateIncrementally to \c true for cached dependencies that
    /// should be sparsely invalidated and updated incrementally.
    ///
    /// Note that the masks here may sometimes be empty, to signify that an
    /// output mask couldn't be inferred for that output. If an output is
    /// returned with an empty mask, that output is reachable from the provided
    /// outputs but the mask associated with the traversal is unknown.
    ///
    /// \note
    /// This method is *not* thread safe.
    ///
    EF_API
    const VdfOutputToMaskMap &FindOutputs(
        const VdfMaskedOutputVector &outputs,
        bool updateIncrementally) const;

    /// Find the node dependencies associated with the given request.
    /// 
    /// Set \p updateIncrementally to \c true for cached dependencies that
    /// should be sparsely invalidated and updated incrementally.
    ///
    /// \note
    /// This method is *not* thread safe.
    ///
    EF_API
    const std::vector<const VdfNode *> &FindNodes(
        const VdfMaskedOutputVector &outputs,
        bool updateIncrementally) const;

    /// Invalidate all cached dependencies.
    ///
    /// \note
    /// This method is *not* thread safe.
    ///
    EF_API
    void Invalidate();

    /// Invalidate all traversals dependent on this connection.
    ///
    /// \note
    /// Intermixed concurrent calls to this method and to DidConnect are
    /// supported. (Though it's not safe for any given source and target output
    /// pair to be concurrently connected and deleted.)
    ///
    EF_API
    void WillDeleteConnection(const VdfConnection &connection);

    /// Invalidate all traversals dependent on this new connection.
    ///
    /// \note
    /// Intermixed concurrent calls to this method and to WillDeleteConnection
    /// are supported. (Though it's not safe for any given source and target
    /// output pair to be concurrently connected and deleted.)
    ///
    EF_API
    void DidConnect(const VdfConnection &connection);

private:

    // The cache entry stored for each traversal.
    class _Entry {
    public:
        // Constructor.
        _Entry(bool updateIncrementally)
            : updateIncrementally(updateIncrementally)
            , valid(true)
        {}

        // Returns \c true if the traversal contains the specified node.
        bool ContainsNode(const VdfNode &node) const {
            const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
            return
                nodeRefs.GetSize() > nodeIndex &&
                nodeRefs.IsSet(nodeIndex);
        }

        bool IsValid() const {
            return valid.load(std::memory_order_relaxed);
        }

        void Invalidate() {
            valid.store(std::memory_order_relaxed);
        }

        // The resulting output dependencies.
        VdfOutputToMaskMap outputDeps;

        // The resulting node dependencies.
        std::vector<const VdfNode *> nodeDeps;

        // Struct that represents a connection that may or may not exist.
        //
        // We store added connections using this representation beacuse it's
        // possible that a connection may be added and then later removed.
        // Therefore, rather than storing pointers to added connections, we
        // store the information needed to look up the connection from the
        // network.
        struct _Connection {
            _Connection(
                VdfId sourceNodeId_,
                TfToken outputName_,
                VdfId targetNodeId_,
                TfToken inputName_)
                : sourceNodeId(sourceNodeId_)
                , outputName(outputName_)
                , targetNodeId(targetNodeId_)
                , inputName(inputName_)
            {}

            // Returns the pointer to this connection, if it exists in the given
            // network; otherwise returns nullptr.
            EF_API
            VdfConnection *GetConnection(
                const VdfNetwork *network) const;

            VdfId sourceNodeId;
            TfToken outputName;
            VdfId targetNodeId;
            TfToken inputName;
        };

        // Any newly added connections that may affect this traversal.
        //
        // If this vector is non-empty when the entry is queried, the traversal
        // must be incrementally updated.
        tbb::concurrent_vector<_Connection> newConnections;

        // Every output and mask encountered during the traversal.
        //
        // Note that the masks here may sometimes be empty, to signify that an
        // output mask couldn't be inferred for that output.
        VdfOutputToMaskMap outputRefs;

        // Every node encountered during the traversal.
        TfBits nodeRefs;

        // The number of outputs for each node at the time of the traversal
        std::vector<size_t> nodeNumOutputs;

        // Incrementally update this traversal?
        bool updateIncrementally;

        // Set to false when the entry is fully invalid.
        std::atomic<bool> valid;
    };

    // Find an entry in the cache.
    const _Entry & _Find(
        const VdfMaskedOutputVector &outputs,
        bool updateIncrementally) const;

    // Populates the cache with a new entry for the given request.
    const _Entry & _PopulateCache(
        const VdfMaskedOutputVector& outputs,
        bool updateIncrementally) const;

    // Traverse with the specified outputs and extend the traversal entry.
    void _Traverse(
        const VdfMaskedOutputVector &outputs,
        _Entry *entry) const;

    // Update the existing traversal, by building a partial request from
    // the new connections stored in the traversal entry.
    void _TraversePartially(
        const VdfNetwork *network,
        _Entry *entry) const;

    // Gather dependencies for the partial traversal across the new connection.
    // Will return \c true if the source output is included in the existing
    // traversal entry.
    bool _GatherDependenciesForNewConnection(
        _Entry *entry,
        const VdfConnection &connection,
        VdfMaskedOutputVector *dependencies) const;

    // Gather dependencies for the partial traversal on a node that has been
    // extended with additional outputs.
    void _GatherDependenciesForExtendedNode(
        const _Entry &entry,
        const VdfNode &node,
        VdfMaskedOutputVector *dependencies) const;

    // The traversal node callback.
    static bool _NodeCallback(
        const VdfNode &node,
        PredicateFunction predicate,
        _Entry *entry);

    // The traversal output callback.
    static bool _OutputCallback(
        const VdfOutput &output,
        const VdfMask &mask,
        const VdfInput *input,
        _Entry *entry);

    //
    // Data members
    //

    // The cache is a map from VdfMaskedOutputVector to stored entry.
    using _Cache = std::unordered_map<
        VdfMaskedOutputVector, _Entry, VdfMaskedOutputVector_Hash>;

    // Dependency cache.
    mutable _Cache _cache;

    // The predicate function.
    PredicateFunction _predicate;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
