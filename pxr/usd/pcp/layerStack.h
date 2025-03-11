//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_LAYER_STACK_H
#define PXR_USD_PCP_LAYER_STACK_H

/// \file pcp/layerStack.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/sdf/layerTree.h"
#include "pxr/base/tf/declarePtrs.h"

#include <tbb/spin_mutex.h>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(SdfLayer);
TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(Pcp_LayerStackRegistry);

class ArResolverContext;
class Pcp_LayerStackRegistry;
class Pcp_MutedLayers;
class PcpExpressionVariables;
class PcpLayerStackChanges;
class PcpLifeboat;

/// \class PcpLayerStack
///
/// Represents a stack of layers that contribute opinions to composition.
///
/// Each PcpLayerStack is identified by a PcpLayerStackIdentifier. This
/// identifier contains all of the parameters needed to construct a layer stack,
/// such as the root layer, session layer, and path resolver context.
///
/// PcpLayerStacks are constructed and managed by a Pcp_LayerStackRegistry.
///
class PcpLayerStack : public TfRefBase, public TfWeakBase {
    PcpLayerStack(const PcpLayerStack&) = delete;
    PcpLayerStack& operator=(const PcpLayerStack&) = delete;

public:
    // See Pcp_LayerStackRegistry for creating layer stacks.
    PCP_API
    virtual ~PcpLayerStack();

    /// Returns the identifier for this layer stack.
    PCP_API
    const PcpLayerStackIdentifier& GetIdentifier() const;

    /// Return true if this layer stack is in USD mode.
    bool IsUsd() const {
        return _isUsd;
    };

    /// Returns the layers in this layer stack in strong-to-weak order.
    /// Note that this is only the *local* layer stack -- it does not
    /// include any layers brought in by references inside prims.
    PCP_API
    const SdfLayerRefPtrVector& GetLayers() const;

    /// Returns only the session layers in the layer stack in strong-to-weak 
    /// order.
    PCP_API
    SdfLayerHandleVector GetSessionLayers() const;

    /// Returns the layer tree representing the structure of the non-session
    /// layers in the layer stack.
    PCP_API
    const SdfLayerTreeHandle& GetLayerTree() const;

    /// Returns the layer tree representing the structure of the session
    /// layers in the layer stack or null if there are no session layers.
    PCP_API
    const SdfLayerTreeHandle& GetSessionLayerTree() const;

    /// Returns the layer offset for the given layer, or NULL if the layer
    /// can't be found or is the identity.
    PCP_API
    const SdfLayerOffset* GetLayerOffsetForLayer(const SdfLayerHandle&) const;

    /// Return the layer offset for the given layer, or NULL if the layer
    /// can't be found or is the identity.
    PCP_API
    const SdfLayerOffset* GetLayerOffsetForLayer(const SdfLayerRefPtr&) const;
    
    /// Returns the layer offset for the layer at the given index in this
    /// layer stack. Returns NULL if the offset is the identity.
    PCP_API
    const SdfLayerOffset* GetLayerOffsetForLayer(size_t layerIdx) const;

    /// Returns the set of layers that were muted in this layer
    /// stack.
    PCP_API
    const std::set<std::string>& GetMutedLayers() const;

    /// Return the list of errors local to this layer stack.
    PcpErrorVector GetLocalErrors() const {
        return _localErrors ? *_localErrors.get() : PcpErrorVector();
    }

    /// Returns true if this layer stack contains the given layer, false
    /// otherwise.
    PCP_API
    bool HasLayer(const SdfLayerHandle& layer) const;
    PCP_API
    bool HasLayer(const SdfLayerRefPtr& layer) const;

    /// Return the composed expression variables for this layer stack.
    const PcpExpressionVariables& GetExpressionVariables() const
    { return *_expressionVariables; }

    /// Return the set of expression variables used during the computation
    /// of this layer stack. For example, this may include the variables
    /// used in expression variable expressions in sublayer asset paths.
    const std::unordered_set<std::string>&
    GetExpressionVariableDependencies() const 
    { return _expressionVariableDependencies; }

    /// Return the time codes per second value of the layer stack. This is 
    /// usually the same as the computed time codes per second of the root layer
    /// but may be computed from the session layer when its present.
    double GetTimeCodesPerSecond() const { return _timeCodesPerSecond; }

    /// Returns relocation source-to-target mapping for this layer stack.
    ///
    /// This map combines the individual relocation entries found across
    /// all layers in this layer stack; multiple entries that affect a single
    /// prim will be combined into a single entry. For instance, if this
    /// layer stack contains relocations { /A: /B } and { /A/C: /A/D }, this
    /// map will contain { /A: /B } and { /B/C: /B/D }. This allows consumers
    /// to go from unrelocated namespace to relocated namespace in a single
    /// step.
    PCP_API
    const SdfRelocatesMap& GetRelocatesSourceToTarget() const;

    /// Returns relocation target-to-source mapping for this layer stack.
    ///
    /// See GetRelocatesSourceToTarget for more details.
    PCP_API
    const SdfRelocatesMap& GetRelocatesTargetToSource() const;

    /// Returns incremental relocation source-to-target mapping for this layer 
    /// stack.
    ///
    /// This map contains the individual relocation entries found across
    /// all layers in this layer stack; it does not combine ancestral
    /// entries with descendant entries. For instance, if this
    /// layer stack contains relocations { /A: /B } and { /A/C: /A/D }, this
    /// map will contain { /A: /B } and { /A/C: /A/D }.
    PCP_API
    const SdfRelocatesMap& GetIncrementalRelocatesSourceToTarget() const;

    /// Returns incremental relocation target-to-source mapping for this layer 
    /// stack.
    ///
    /// See GetIncrementalRelocatesTargetToSource for more details.
    PCP_API
    const SdfRelocatesMap& GetIncrementalRelocatesTargetToSource() const;

    /// Returns a list of paths to all prims across all layers in this 
    /// layer stack that contained relocates.
    PCP_API
    const SdfPathVector& GetPathsToPrimsWithRelocates() const;

    /// Apply the changes in \p changes.  This blows caches.  It's up to
    /// the client to pull on those caches again as needed.
    ///
    /// Objects that are no longer needed and would be destroyed are
    /// retained in \p lifeboat and won't be destroyed until \p lifeboat is
    /// itself destroyed.  This gives the client control over the timing
    /// of the destruction of those objects.  Clients may choose to pull
    /// on the caches before destroying \p lifeboat.  That may cause the
    /// caches to again retain the objects, meaning they won't be destroyed
    /// when \p lifeboat is destroyed.
    ///
    /// For example, if blowing a cache means an SdfLayer is no longer
    /// needed then \p lifeboat will hold an SdfLayerRefPtr to that layer. 
    /// The client can then pull on that cache, which could cause the
    /// cache to hold an SdfLayerRefPtr to the layer again.  If so then
    /// destroying \p changes will not destroy the layer.  In any case,
    /// we don't destroy the layer and then read it again.  However, if
    /// the client destroys \p lifeboat before pulling on the cache then
    /// we would destroy the layer then read it again.
    PCP_API
    void Apply(const PcpLayerStackChanges& changes, PcpLifeboat* lifeboat);

    /// Return a PcpMapExpression representing the relocations that affect
    /// namespace at and below the given path.  The value of this
    /// expression will continue to track the effective relocations if
    /// they are changed later. In USD mode only, this will return a null 
    /// expression if there are no relocations on this layer stack.
    PCP_API
    PcpMapExpression GetExpressionForRelocatesAtPath(const SdfPath &path);

    /// Return true if there are any relocated prim paths in this layer
    /// stack.
    PCP_API
    bool HasRelocates() const;

private:
    // Only a registry can create a layer stack.
    friend class Pcp_LayerStackRegistry;
    // PcpCache needs access to check the _registry.
    friend class PcpCache;
    // Needs access to _sublayerSourceInfo
    friend bool Pcp_NeedToRecomputeDueToAssetPathChange(const PcpLayerStackPtr&);

    // Construct a layer stack for the given \p identifier that will be
    // installed into \p registry. This installation is managed by
    // \p registry and does not occur within the c'tor. See comments on
    // _registry for more details.
    PcpLayerStack(const PcpLayerStackIdentifier &identifier,
                  const Pcp_LayerStackRegistry &registry);

    void _BlowLayers();
    void _BlowRelocations();
    void _Compute(const std::string &fileFormatTarget,
                  const Pcp_MutedLayers &mutedLayers);

    SdfLayerTreeHandle _BuildLayerStack(
        const SdfLayerHandle & layer,
        const SdfLayerOffset & offset,
        double layerTcps,
        const ArResolverContext & pathResolverContext,
        const SdfLayer::FileFormatArguments & layerArgs,
        const std::string & sessionOwner,
        const Pcp_MutedLayers & mutedLayers,
        SdfLayerHandleSet *seenLayers,
        PcpErrorVector *errors);

private:
    /// The identifier that uniquely identifies this layer stack.
    const PcpLayerStackIdentifier _identifier;

    /// The registry (1:1 with a PcpCache) this layer stack belongs to.  This
    /// may not be set, particularly when a registry is creating a layer stack
    /// but before it's been installed in the registry.
    Pcp_LayerStackRegistryPtr _registry;

    /// Data representing the computed layer stack contents.
    /// 
    /// This is built by examining the session and root layers for
    /// sublayers, resolving their asset paths with the path resolver context,
    /// and recursively building up the layer stack.
    ///
    /// Note that this is only the *local* layer stack -- it does not
    /// include any layers brought in by references inside prims.

    /// Retained references to the layers in the stack,
    /// in strong-to-weak order.
    SdfLayerRefPtrVector _layers;

    /// The corresponding map functions for each entry in 'layers'. 
    /// Each map function contains a time offset that should be applied
    /// to its corresponding layer.
    std::vector<PcpMapFunction> _mapFunctions;

    /// Stores the computed time codes per second value of the layer stack which
    /// has some special logic when a session layer is present. 
    double _timeCodesPerSecond;

    /// The tree structure of the layer stack.
    /// Stored separately because this is needed only occasionally.
    SdfLayerTreeHandle _layerTree;

    /// The tree structure of the session layer stack.
    /// Stored separately because this is needed only occasionally.
    SdfLayerTreeHandle _sessionLayerTree;

    /// Tracks information used to compute sublayer asset paths.
    struct _SublayerSourceInfo {
        _SublayerSourceInfo() = default;
        _SublayerSourceInfo(
            const SdfLayerHandle& layer_,
            const std::string& authoredSublayerPath_,
            const std::string& computedSublayerPath_)
        : layer(layer_)
        , authoredSublayerPath(authoredSublayerPath_)
        , computedSublayerPath(computedSublayerPath_) { }

        SdfLayerHandle layer;
        std::string authoredSublayerPath;
        std::string computedSublayerPath;
    };

    /// List of source info for sublayer asset path computations.
    std::vector<_SublayerSourceInfo> _sublayerSourceInfo;

    /// Set of asset paths that were muted in this layer stack.
    std::set<std::string> _mutedAssetPaths;

    /// The errors, if any, discovered while computing this layer stack.
    /// NULL if no errors were found (the expected common case).
    std::unique_ptr<PcpErrorVector> _localErrors;

    /// Pre-computed table of local relocates.
    SdfRelocatesMap _relocatesSourceToTarget;
    SdfRelocatesMap _relocatesTargetToSource;
    SdfRelocatesMap _incrementalRelocatesSourceToTarget;
    SdfRelocatesMap _incrementalRelocatesTargetToSource;

    /// A map of PcpMapExpressions::Variable instances used to represent
    /// the current value of relocations given out by
    /// GetExpressionForRelocatesAtPath().  This map is used to update
    /// those values when relocations change.
    typedef std::map<SdfPath, PcpMapExpression::VariableUniquePtr,
            SdfPath::FastLessThan> _RelocatesVarMap;
    _RelocatesVarMap _relocatesVariables;
    tbb::spin_mutex _relocatesVariablesMutex;

    /// List of all prim spec paths where relocations were found.
    SdfPathVector _relocatesPrimPaths;

    /// Composed expression variables.
    std::shared_ptr<PcpExpressionVariables> _expressionVariables;

    /// Set of expression variables this layer stack depends on.
    std::unordered_set<std::string> _expressionVariableDependencies;

    bool _isUsd;
};

PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackPtr&);
PCP_API
std::ostream& operator<<(std::ostream&, const PcpLayerStackRefPtr&);

/// Returns true if negative layer offsets and scales are allowed.
///
/// Negative layer offset scales are deprecated and a warning will be issued
/// when commulative scale during composition is negative with 
/// PCP_ALLOW_NEGATIVE_LAYER_OFFSET_SCALE is set to true (default right now).
/// If PCP_ALLOW_NEGATIVE_LAYER_OFFSET_SCALE is set to false, a coding error
/// will be issued when a negative scale is encountered.
PCP_API
bool PcpNegativeLayerOffsetScaleAllowed();

/// Checks if the source and target paths constitute a valid relocates. This
/// validation is not context specific, i.e. if this returns false, the 
/// combination of source and target paths is always invalid for any attempted
/// relocation.
bool
Pcp_IsValidRelocatesEntry(
    const SdfPath &source, const SdfPath &target, std::string *errorMessage);

/// Builds a relocates map from a list of layer and SdfRelocates value pairs.
void
Pcp_BuildRelocateMap(
    const std::vector<std::pair<SdfLayerHandle, SdfRelocates>> &layerRelocates,
    SdfRelocatesMap *relocatesMap,
    PcpErrorVector *errors);

/// Compose the relocation arcs in the given stack of layers,
/// putting the results into the given sourceToTarget and targetToSource
/// maps.
void
Pcp_ComputeRelocationsForLayerStack(
    const PcpLayerStack &layerStack,
    SdfRelocatesMap *relocatesSourceToTarget,
    SdfRelocatesMap *relocatesTargetToSource,
    SdfRelocatesMap *incrementalRelocatesSourceToTarget,
    SdfRelocatesMap *incrementalRelocatesTargetToSource,
    SdfPathVector *relocatesPrimPaths,
    PcpErrorVector *errors);

// Returns true if \p layerStack should be recomputed due to changes to
// any computed asset paths that were used to find or open layers
// when originally composing \p layerStack. This may be due to scene
// description changes or external changes to asset resolution that
// may affect the computation of those asset paths.
bool
Pcp_NeedToRecomputeDueToAssetPathChange(const PcpLayerStackPtr& layerStack);

// Returns true if the \p layerStack should be recomputed because 
// \p changedLayer has had changes that would cause the layer stack to have
// a different computed overall time codes per second value.
bool
Pcp_NeedToRecomputeLayerStackTimeCodesPerSecond(
    const PcpLayerStackPtr& layerStack, const SdfLayerHandle &changedLayer);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_LAYER_STACK_H
