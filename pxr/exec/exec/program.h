//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PROGRAM_H
#define PXR_EXEC_EXEC_PROGRAM_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/attributeInputNode.h"
#include "pxr/exec/exec/compiledOutputCache.h"
#include "pxr/exec/exec/compiledLeafNodeCache.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/metadataInputNode.h"
#include "pxr/exec/exec/nodeRecompilationInfoTable.h"
#include "pxr/exec/exec/uncompilationTable.h"

#include "pxr/base/ts/spline.h"
#include "pxr/exec/ef/leafNodeCache.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/esf/schemaConfigKey.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/usd/sdf/path.h"

#include <tbb/concurrent_unordered_map.h>

#include <atomic>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class EfTime;
class EfTimeInputNode;
class EsfJournal;
struct Exec_AttributeValueInvalidationResult;
class Exec_CompilationState;
struct Exec_DisconnectedInputsInvalidationResult;
struct Exec_MetadataInvalidationResult;
class Exec_TimeChangeInvalidationResult;
class TfBits;
template <typename> class TfSpan;
class VdfExecutorInterface;
class VdfGrapherOptions;
class VdfInput;
class VdfNode;

/// Owns a VdfNetwork and related data structures to access and modify the
/// network.
///
/// The VdfNetwork describes the topological structure of nodes and connections,
/// but does not prescribe any meaning to the organization of the network. In
/// order to compile, update, and evaluate the network, Exec requires additional
/// metadata to facilitate common access patterns.
///
/// Generally, the data structures contained by this class are those that must
/// have exactly one instance per-network. The responsibilities of these data
/// structures include:
///
///   - Tracking which VdfOutput provides the value of a given Exec_OutputKey.
///   - Tracking the conditions when specific nodes and connections should be
///     deleted from the network.
///   - Tracking the leaf nodes dependent on any particular output in the
///     network.
///   - Tracking which nodes may be isolated due to uncompilation.
///   - Tracking which inputs have been affected by uncompilation and should
///     later be recompiled.
///
/// Some of these data structures must be modified when the network is modified.
/// Therefore, compilation never directly accesses the VdfNetwork, but does so
/// through an Exec_Program.
///
class Exec_Program
{
public:
    Exec_Program();

    // Non-copyable and non-movable.
    Exec_Program(const Exec_Program &) = delete;
    Exec_Program& operator=(const Exec_Program &) = delete;
    
    ~Exec_Program();

    /// Gets the current compilation version.
    ///
    /// This version is incremented with each new round of compilation.
    ///
    size_t GetCompilationVersion() const {
        return _compilationVersion;
    }

    /// Declares that the program is about to begin a new round of compilation.
    void BeginCompilation() {
        // Increments the program's compilation version. We expect the version
        // to never wrap-around.
        TF_VERIFY(++_compilationVersion != 0);
        _wasInterrupted = false;
    }

    /// Called after completing a round of compilation.
    void EndCompilation(Exec_CompilationState &compilationState);

    /// Adds a new node in the VdfNetwork.
    ///
    /// Constructs a node of type \p NodeType. The first argument of the node
    /// constructor is a pointer to the VdfNetwork maintained by this
    /// Exec_Program, and the remaining arguments are forwarded from
    /// \p nodeCtorArgs.
    ///
    /// Uncompilation rules for the new node are added from the \p journal.
    ///
    /// \return a pointer to the newly constructed node. This pointer is owned
    /// by the network.
    ///
    template <class NodeType, class... NodeCtorArgs>
    NodeType *CreateNode(
        const EsfJournal &journal,
        NodeCtorArgs... nodeCtorArgs);

    /// Makes connections between nodes in the VdfNetwork.
    ///
    /// All non-null VdfMaskedOutputs in \p outputs are connected to the input
    /// named \p inputName on \p inputNode. Null outputs are skipped.
    ///
    /// Even if \p outputs is empty or lacks non-null outputs, this method
    /// should still be called in order to properly add uncompilation rules from
    /// the \p journal.
    ///
    void Connect(
        const EsfJournal &journal,
        TfSpan<const VdfMaskedOutput> outputs,
        VdfNode *inputNode,
        const TfToken &inputName);

    /// Gets the VdfMaskedOutput provided by \p outputKeyIdentity.
    ///
    /// \return a mapped value from the Exec_CompiledOutputCache, which contains
    /// a VdfMaskedOutput and the compilation version at the time that output
    /// was added to the cache; or return nullptr if there is no entry for
    /// \p outputKeyIdentity.
    ///
    /// \note
    /// The returned VdfMaskedOutput may contain a null VdfOutput. This
    /// indicates that the given output key is *already known* to not have a
    /// corresponding output.
    ///
    const Exec_CompiledOutputCache::MappedType *
    GetCompiledOutput(
        const Exec_OutputKey::Identity &outputKeyIdentity) const {
        return _compiledOutputCache.Find(outputKeyIdentity);
    }

    /// Establishes that \p outputKeyIdentity is provided by \p maskedOutput.
    ///
    /// If \p outputKeyIdentity has not yet been mapped to a masked output,
    /// insert the new mapping and return true. Otherwise, the existing mapping
    /// is not modified, and this returns false.
    ///
    bool SetCompiledOutput(
        const Exec_OutputKey::Identity &outputKeyIdentity,
        const VdfMaskedOutput &maskedOutput) {
        return _compiledOutputCache.Insert(
            outputKeyIdentity, maskedOutput, _compilationVersion);
    }

    /// Gets the leaf node compiled for the given \p valueKey.
    const EfLeafNode *GetCompiledLeafNode(const ExecValueKey &valueKey) const {
        return _compiledLeafNodeCache.Find(valueKey);
    }

    /// Establishes that \p leafNode has been compiled for \p valueKey.
    ///
    /// If another leaf node has already been compiled for \p valueKey, then
    /// this function has no effect. This is not an error.
    ///
    void SetCompiledLeafNode(
        const ExecValueKey &valueKey,
        const EfLeafNode *const leafNode) {
        _compiledLeafNodeCache.Insert(valueKey, leafNode);
    }

    /// Returns the leaf node cache.
    EfLeafNodeCache &GetLeafNodeCache() {
        return _leafNodeCache;
    }

    /// Returns the current generational counter of the execution network.
    size_t GetNetworkVersion() const {
        return _network.GetVersion();
    }

    /// Gathers the information required to invalidate the system and notify
    /// requests after uncompilation.
    /// 
    Exec_DisconnectedInputsInvalidationResult InvalidateDisconnectedInputs();

    /// Gathers the information required to invalidate the system and notify
    /// requests after attribute authored value invalidation.
    /// 
    Exec_AttributeValueInvalidationResult InvalidateAttributeAuthoredValues(
        TfSpan<const SdfPath> invalidAttributes);

    /// Gathers the information required to invalidate the system and notify
    /// requests after metadata authored value invalidation.
    /// 
    Exec_MetadataInvalidationResult InvalidateMetadataValues(
        TfSpan<const std::pair<SdfPath, TfToken>> invalidFields);

    /// Resets the accumulated set of input nodes that require invalidation.
    /// 
    /// Returns an executor invalidation requests for the call site to perform
    /// executor invalidation.
    /// 
    VdfMaskedOutputVector ResetInputNodesRequiringInvalidation();

    /// Gathers the information required to invalidate the system and notify
    /// requests after time has changed.
    /// 
    Exec_TimeChangeInvalidationResult InvalidateTime(
        const EfTime &oldTime, const EfTime &newTime);

    /// Returns the time input node.
    ///
    /// Unlike most nodes, a program always has exactly one time input node.
    /// Compilation may not create additional time input nodes and
    /// uncompilation may not remove the time input node.
    ///
    EfTimeInputNode &GetTimeInputNode() const {
        return *_timeInputNode;
    }

    /// Returns the node with the given \p nodeId, or nullptr if no such node
    /// exists.
    ///
    VdfNode *GetNodeById(const VdfId nodeId) {
        return _network.GetNodeById(nodeId);
    }

    /// Deletes a \p node from the network.
    ///
    /// All incoming and outgoing connections on \p node are deleted. Downstream
    /// inputs previously connected to \p node are marked as "dirty" and can be
    /// queried by GetDirtyOutputs. Upstream nodes previously feeding into
    /// \p node may be left isolated.
    ///
    /// \note
    /// This method is not thread-safe.
    ///
    void DisconnectAndDeleteNode(VdfNode *node);

    /// Deletes all connections flowing into \p input.
    ///
    /// This input is added to the set of "dirty" inputs. Upstream nodes
    /// previously feeding into this \p input may be left isolated.
    ///
    /// \note
    /// This method is not thread-safe.
    /// 
    void DisconnectInput(VdfInput *input);

    /// Gets the set of inputs that have been affected by uncompilation and need
    /// to be recompiled.
    ///
    const std::unordered_set<VdfInput *> &
    GetInputsRequiringRecompilation() const {
        return _inputsRequiringRecompilation;
    }

    /// Clears the set of inputs that were affected by uncompilation.
    ///
    /// This should be called after all such inputs have been recompiled.
    ///
    /// \note
    /// This method is not thread-safe.
    ///
    void ClearInputsRequiringRecompilation() {
        _inputsRequiringRecompilation.clear();
    }

    /// Returns Exec_UncompilationRuleSet%s for \p resyncedPath and descendants
    /// of \p resyncedPath.
    ///
    std::vector<Exec_UncompilationTable::Entry>
    ExtractUncompilationRuleSetsForResync(const SdfPath &resyncedPath) {
        return _uncompilationTable.UpdateForRecursiveResync(resyncedPath);
    }

    /// Returns the Exec_UncompilationRuleSet for \p changedPath.
    Exec_UncompilationTable::Entry GetUncompilationRuleSetForPath(
        const SdfPath &changedPath) {
        return _uncompilationTable.Find(changedPath);
    }

    /// Sets recompilation info for the given \p node after it has been
    /// compiled.
    ///
    /// This information will be retrieved during recompilation when inputs of
    /// \p node need to be recompiled.
    ///
    void SetNodeRecompilationInfo(
        const VdfNode *node,
        const EsfObject &provider,
        const EsfSchemaConfigKey dispatchingSchemaId,
        Exec_InputKeyVectorConstRefPtr &&inputKeys) {
        _nodeRecompilationInfoTable.SetNodeRecompilationInfo(
            node,
            provider,
            dispatchingSchemaId,
            std::move(inputKeys));
    }

    /// Retrieves the recompilation information stored for \p node.
    const Exec_NodeRecompilationInfo *GetNodeRecompilationInfo(
        const VdfNode *node) const {
        return _nodeRecompilationInfoTable.GetNodeRecompilationInfo(node);
    }

    /// Returns true if the most recent round of compilation was interrupted.
    bool WasInterrupted() const {
        return _wasInterrupted;
    }

    /// Starting from the set of potentially isolated nodes, creates a
    /// subnetwork containing all isolated nodes and connections.
    ///
    /// \note
    /// This method doesn't remove the isolated objects from the network; the
    /// caller can either call
    /// VdfIsolatedSubnetwork::RemoveIsolatedObjectsFromNetwork or the
    /// VdfIsolatedSubnetwork destructor will remove the objects before it
    /// deletes them.
    ///
    std::unique_ptr<VdfIsolatedSubnetwork> CreateIsolatedSubnetwork();

    /// Writes the compiled network to a file at \p filename.
    void GraphNetwork(
        const char *filename,
        const VdfGrapherOptions &grapherOptions) const;

private:
    // Updates data structures for a newly-added node.
    void _AddNode(const EsfJournal &journal, const VdfNode *node);

    // Registers an attribute input node for authored value initialization.
    void _RegisterAttributeInputNode(Exec_AttributeInputNode *inputNode);

    // Unregisters an attribute input node from authored value initialization.
    void _UnregisterAttributeInputNode(const Exec_AttributeInputNode *inputNode);

    // Registers a metadata input node for authored value initialization.
    void _RegisterMetadataInputNode(Exec_MetadataInputNode *inputNode);

    // Unregisters a metadata input node from authored value initialization.
    void _UnregisterMetadataInputNode(const Exec_MetadataInputNode *inputNode);

    // Notifies the program of a new or deleted connection between the time
    // input node and the given target node.
    // 
    void _ChangedTimeConnections(const VdfNode &targetNode);

    // Flags the array of _timeDependentOutputs as invalid.
    void _InvalidateTimeDependentOutputs();

    // Rebuilds the array of _timeDependentOutputs.
    const VdfMaskedOutputVector &_CollectTimeDependentOutputs();

private:
    // The compiled data flow network.
    VdfNetwork _network;

    // An integer value that increases with each round of compilation.
    size_t _compilationVersion;

    // Every network always has a compiled time input node.
    EfTimeInputNode *const _timeInputNode;

    // A cache of compiled outputs keys and corresponding data flow outputs.
    Exec_CompiledOutputCache _compiledOutputCache;

    // A cache of leaf nodes compiled for value keys.
    Exec_CompiledLeafNodeCache _compiledLeafNodeCache;

    // Maps scene paths to data flow network that must be uncompiled in response
    // to edits to those scene paths.
    Exec_UncompilationTable _uncompilationTable;

    // Caches traversals to quickly determine which leaf nodes are downstream of
    // an aribrary node or output in the network.
    EfLeafNodeCache _leafNodeCache;

    // Collection of compiled attribute input nodes.
    struct _AttributeInputNodeEntry {
        Exec_AttributeInputNode *node;
        std::optional<TsSpline> oldSpline;
    };
    using _AttributeInputNodesMap =
        tbb::concurrent_unordered_map<
        SdfPath, _AttributeInputNodeEntry, SdfPath::Hash>;
    _AttributeInputNodesMap _attributeInputNodes;

    // Collection of compiled metadata input nodes.
    using _MetadataInputNodesMap =
        tbb::concurrent_unordered_map<
            std::pair<SdfPath, TfToken>,
            Exec_MetadataInputNode *,
            TfHash>;
    _MetadataInputNodesMap _metadataInputNodes;

    // Array of outputs connected to the time input node.
    VdfMaskedOutputVector _timeDependentOutputs;

    // Flag indicating whether the _timeDependentOutputs array is up-to-date or
    // must be re-computed.
    std::atomic<bool> _timeDependentOutputsValid;

    // Input nodes currently queued for invalidation.
    std::vector<VdfId> _inputNodesRequiringInvalidation;

    // On behalf of the program intercepts and responds to fine-grained network
    // edits.
    class _EditMonitor;
    std::unique_ptr<_EditMonitor> _editMonitor;

    // Nodes that may be isolated due to prior uncompilation.
    std::unordered_set<VdfNode *> _potentiallyIsolatedNodes;

    // Inputs that were disconnected during uncompilation.
    std::unordered_set<VdfInput *> _inputsRequiringRecompilation;

    // Stores recompilation info for every node.
    Exec_NodeRecompilationInfoTable _nodeRecompilationInfoTable;

    // True if the most recent round of compilation was interrupted.
    bool _wasInterrupted;
};


template <class NodeType, class... NodeCtorArgs>
NodeType *Exec_Program::CreateNode(
    const EsfJournal &journal,
    NodeCtorArgs... nodeCtorArgs)
{
    static_assert(std::is_base_of_v<VdfNode, NodeType>);
    static_assert(!std::is_same_v<EfTimeInputNode, NodeType>,
                  "CreateNode may not construct additional EfTimeInputNodes. "
                  "Use GetTimeInputNode() to access the time node.");

    NodeType *const node = new NodeType(
        &_network, std::forward<NodeCtorArgs>(nodeCtorArgs)...);
    _AddNode(journal, node);

    // Input nodes are tracked for authored value initialization.
    if constexpr (std::is_same_v<Exec_AttributeInputNode, NodeType>) {
        _RegisterAttributeInputNode(node);
    }
    else if constexpr (std::is_same_v<Exec_MetadataInputNode, NodeType>) {
        _RegisterMetadataInputNode(node);
    }

    return node;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
