//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PCP_LAYER_STACK_H
#define PCP_LAYER_STACK_H

/// \file pcp/layerStack.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/sdf/layerTree.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/noncopyable.hpp>
#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(SdfLayer);
TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(Pcp_LayerStackRegistry);

class ArResolverContext;
class Pcp_LayerStackRegistry;
class Pcp_MutedLayers;
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
class PcpLayerStack : public TfRefBase, public TfWeakBase, boost::noncopyable {
public:
    // See Pcp_LayerStackRegistry for creating layer stacks.

    virtual ~PcpLayerStack();

    /// Returns the identifier for this layer stack.
    const PcpLayerStackIdentifier& GetIdentifier() const;

    /// Returns the layers in this layer stack in strong-to-weak order.
    /// Note that this is only the *local* layer stack -- it does not
    /// include any layers brought in by references inside prims.
    const SdfLayerRefPtrVector& GetLayers() const;

    /// Returns only the session layers in the layer stack in strong-to-weak 
    /// order.
    SdfLayerHandleVector GetSessionLayers() const;

    /// Returns the layer tree representing the structure of this layer
    /// stack.
    const SdfLayerTreeHandle& GetLayerTree() const;

    /// Returns the layer offset for the given layer, or NULL if the layer
    /// can't be found or is the identity.
    const SdfLayerOffset* GetLayerOffsetForLayer(const SdfLayerHandle&) const;

    /// Returns the layer offset for the layer at the given index in this
    /// layer stack. Returns NULL if the offset is the identity.
    const SdfLayerOffset* GetLayerOffsetForLayer(size_t layerIdx) const;

    /// Returns the set of asset paths resolved while building the
    /// layer stack.
    const std::set<std::string>& GetResolvedAssetPaths() const;

    /// Returns the set of layers that were muted in this layer
    /// stack.
    const std::set<std::string>& GetMutedLayers() const;

    /// Return the list of errors local to this layer stack.
    PcpErrorVector GetLocalErrors() const {
        return _localErrors ? *_localErrors.get() : PcpErrorVector();
    }

    /// Returns true if this layer stack contains the given layer, false
    /// otherwise.
    bool HasLayer(const SdfLayerHandle& layer) const;
    bool HasLayer(const SdfLayerRefPtr& layer) const;

    /// Returns relocation source-to-target mapping for this layer stack.
    const SdfRelocatesMap& GetRelocatesSourceToTarget() const;

    /// Returns relocation target-to-source mapping for this layer stack.
    const SdfRelocatesMap& GetRelocatesTargetToSource() const;

    /// Returns a list of paths to all prims across all layers in this 
    /// layer stack that contained relocates.
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
    void Apply(const PcpLayerStackChanges& changes, PcpLifeboat* lifeboat);

    /// Return a PcpMapExpression representing the relocations that affect
    /// namespace at and below the given path.  The value of this
    /// expression will continue to track the effective relocations if
    /// they are changed later.
    PcpMapExpression GetExpressionForRelocatesAtPath(const SdfPath &path);

private:
    // Only a registry can create a layer stack.
    friend class Pcp_LayerStackRegistry;
    // PcpCache needs access to check the _registry.
    friend class PcpCache;
    // Needs access to _sublayerSourceInfo
    friend bool Pcp_NeedToRecomputeDueToAssetPathChange(const PcpLayerStackPtr&);

    // It's a coding error to construct a layer stack with a NULL root layer.
    PcpLayerStack(const PcpLayerStackIdentifier &identifier,
                  const std::string &targetSchema,
                  const Pcp_MutedLayers &mutedLayers,
                  bool isUsd);

    void _BlowLayers();
    void _BlowRelocations();
    void _Compute(const std::string &targetSchema,
                  const Pcp_MutedLayers &mutedLayers);

    SdfLayerTreeHandle _BuildLayerStack(
        const SdfLayerHandle & layer,
        const SdfLayerOffset & offset,
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

    /// The tree structure of the layer stack.
    /// Stored separately because this is needed only ocassionally.
    SdfLayerTreeHandle _layerTree;

    /// Tracks information used to compute sublayer asset paths.
    struct _SublayerSourceInfo {
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

    /// Set of asset paths resolved while building the layer stack.
    /// This is used to handle updates.
    std::set<std::string> _assetPaths;

    /// Set of asset paths that were muted in this layer stack.
    std::set<std::string> _mutedAssetPaths;

    /// The errors, if any, discovered while computing this layer stack.
    /// NULL if no errors were found (the expected common case).
    boost::scoped_ptr<PcpErrorVector> _localErrors;

    /// Pre-computed table of local relocates.
    SdfRelocatesMap _relocatesSourceToTarget;
    SdfRelocatesMap _relocatesTargetToSource;

    /// A map of PcpMapExpressions::Variable instances used to represent
    /// the current value of relocations given out by
    /// GetExpressionForRelocatesAtPath().  This map is used to update
    /// those values when relocations change.
    typedef std::map<SdfPath, PcpMapExpression::VariableRefPtr,
            SdfPath::FastLessThan> _RelocatesVarMap;
    _RelocatesVarMap _relocatesVariables;

    /// List of all prim spec paths where relocations were found.
    SdfPathVector _relocatesPrimPaths;

    bool _isUsd;
};

std::ostream& operator<<(std::ostream&, const PcpLayerStackPtr&);
std::ostream& operator<<(std::ostream&, const PcpLayerStackRefPtr&);

/// Compose the relocation arcs in the given stack of layers,
/// putting the results into the given sourceToTarget and targetToSource
/// maps.
void
Pcp_ComputeRelocationsForLayerStack( const SdfLayerRefPtrVector & layers,
                                     SdfRelocatesMap *relocatesSourceToTarget,
                                     SdfRelocatesMap *relocatesTargetToSource,
                                     SdfPathVector *relocatesPrimPaths);

// Returns true if \p layerStack should be recomputed due to changes to
// any computed asset paths that were used to find or open layers
// when originally composing \p layerStack. This may be due to scene
// description changes or external changes to asset resolution that
// may affect the computation of those asset paths.
bool
Pcp_NeedToRecomputeDueToAssetPathChange(const PcpLayerStackPtr& layerStack);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_LAYER_STACK_H
