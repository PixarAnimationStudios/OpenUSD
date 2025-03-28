//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RESOLVER_H
#define PXR_USD_USD_RESOLVER_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/iterator.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/declareHandles.h"

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;
class UsdResolveTarget;

/// \class Usd_Resolver
///
/// Given a PcpPrimIndex, this class facilitates value resolution by providing
/// a mechanism for walking the composition structure in strong-to-weak order.
///
class Usd_Resolver {
public:

    /// Constructs a resolver with the given \p index. The index is 
    /// held for the duration of the resolver's lifetime. If \p skipEmptyNodes
    /// is \c true, the resolver will skip over nodes that provide no opinions
    /// about the prim represented by \p index. Otherwise, the resolver will
    /// visit all non-inert nodes in the index.
    USD_API
    explicit Usd_Resolver(
        const PcpPrimIndex* index, 
        bool skipEmptyNodes = true);

    /// Constructs a resolver with the given \p resolveTarget. The resolve 
    /// target provides the prim index as well as the range of nodes and layers
    /// this resolver will iterate over. If \p skipEmptyNodes is \c true, the
    /// resolver will skip over nodes that provide no opinions about the prim
    /// represented by \p index. Otherwise, the resolver will visit all
    /// non-inert nodes in the index.
    USD_API
    explicit Usd_Resolver(
        const UsdResolveTarget *resolveTarget, 
        bool skipEmptyNodes = true);

    /// Returns true when there is a current Node and Layer.
    /// 
    /// The resolver must be known to be valid before calling any of the
    /// accessor or iteration functions, otherwise the behavior will be
    /// undefined.
    bool IsValid() const {
        return _curNode != _endNode;
    }

    /// Advances the resolver to the next weaker Layer in the layer
    /// stack, if the current LayerStack has no more layers, the resolver will
    /// be advanced to the next weaker PcpNode. If no layers are available, the
    /// resolver will be marked as invalid.  Returns \c true iff the resolver
    /// advanced to another node or became invalid.
    ///
    /// If the resolver is already invalid, the behavior of this function is 
    /// undefined.
    USD_API
    bool NextLayer();

    /// Skips all pending layers in the current LayerStack and jumps to
    /// the next weaker PcpNode. When no more nodes are available, the resolver
    /// will be marked as invalid.
    ///
    /// If the resolver is already invalid, the behavior of this function is 
    /// undefined.
    USD_API
    void NextNode();

    /// Returns the current PCP node for a valid resolver. 
    /// 
    /// This is useful for coarse grained resolution tasks, however
    /// individual layers must be inspected in the common case.
    ///
    /// The behavior is undefined if the resolver is not valid.
    PcpNodeRef GetNode() const {
        return *_curNode;
    }

    /// Returns the current layer for the current PcpNode for a valid resolver.
    ///
    /// The behavior is undefined if the resolver is not valid.
    ///
    /// PERFORMANCE: This returns a const-ref to avoid ref-count bumps during
    /// resolution. This is safe under the assumption that no changes will occur
    /// during resolution and that the lifetime of this object will be short.
    const SdfLayerRefPtr& GetLayer() const {
        return *_curLayer;
    }

    /// Returns a translated path for the current PcpNode and Layer for a valid
    /// resolver.
    ///
    /// The behavior is undefined if the resolver is not valid.
    const SdfPath& GetLocalPath() const {
        return _curNode->GetPath(); 
    }

    /// Returns a translated path of the property with the given \p propName for
    /// the current PcpNode and Layer for a valid resolver.
    ///
    /// The behavior is undefined if the resolver is not valid.
    SdfPath GetLocalPath(TfToken const &propName) const {
        return propName.IsEmpty() ? GetLocalPath() :
            GetLocalPath().AppendProperty(propName);
    }

    /// Returns the PcpPrimIndex. 
    ///
    /// This value is initialized when the resolver is constructed and does not
    /// change as a result of calling NextLayer() or NextNode().
    const PcpPrimIndex* GetPrimIndex() const {
        return _index; 
    }

private:
    void _SkipEmptyNodes();

    const PcpPrimIndex* _index;
    bool _skipEmptyNodes;

    PcpNodeIterator _curNode;
    PcpNodeIterator _endNode;
    SdfLayerRefPtrVector::const_iterator _curLayer;
    SdfLayerRefPtrVector::const_iterator _endLayer;
    const UsdResolveTarget *_resolveTarget;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVER_H
