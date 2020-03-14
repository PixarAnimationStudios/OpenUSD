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
    explicit Usd_Resolver(const PcpPrimIndex* index, bool skipEmptyNodes = true);

    /// Returns true when there is a current Node and Layer.
    bool IsValid() const {
        return _curNode != _lastNode;
    }

    /// Advances the resolver to the next weaker Layer in the layer
    /// stack, if the current LayerStack has no more layers, the resolver will
    /// be advanced to the next weaker PcpNode. If no layers are available, the
    /// resolver will be marked as invalid.  Returns \c true iff the resolver
    /// advanced to another node or was or became invalid.
    USD_API
    bool NextLayer();

    /// Skips all pending layers in the current LayerStack and jumps to
    /// the next weaker PcpNode. When no more nodes are available, the resolver
    /// will be marked as invalid.
    USD_API
    void NextNode();

    /// Returns the current PCP node. 
    ///
    /// This is useful for coarse grained resolution tasks, however
    /// individual layers must be inspected in the common case.
    USD_API
    PcpNodeRef GetNode() const;

    /// Returns the current layer for the current PcpNode.
    ///
    /// PERFORMANCE: This returns a const-ref to avoid ref-count bumps during
    /// resolution. This is safe under the assumption that no changes will occur
    /// during resolution and that the lifetime of this object will be short.
    USD_API
    const SdfLayerRefPtr& GetLayer() const;

    /// Returns a translated path for the current PcpNode and Layer.
    USD_API
    const SdfPath& GetLocalPath() const;

    SdfPath GetLocalPath(TfToken const &propName) const {
        return propName.IsEmpty() ? GetLocalPath() :
            GetLocalPath().AppendProperty(propName);
    }

    /// Returns the PcpPrimIndex. 
    ///
    /// This value is initialized when the resolver is constructed and does not
    /// change as a result of calling NextLayer() or NextNode().
    USD_API
    const PcpPrimIndex* GetPrimIndex() const;

    /// \struct Position
    /// Represents a position in the prim index for value resolution.
    /// For performance, this object stores pointers and iterators to avoid
    /// unnecessary copies and ref-count bumps.
    struct Position
    {
        Position() { }

        PcpNodeRef GetNode() const { return *_curNode; }
        const SdfLayerRefPtr& GetLayer() const { return *_curLayer; }
        const SdfPath& GetLocalPath() const { return _curNode->GetPath(); }
        SdfPath GetLocalPath(TfToken const &propName) const {
            return propName.IsEmpty() ? GetLocalPath() :
                GetLocalPath().AppendProperty(propName);
        }

    private:
        friend class Usd_Resolver;

        Position(const PcpNodeIterator& curNode, 
                 const SdfLayerRefPtrVector::const_iterator& curLayer)
            : _curNode(curNode), _curLayer(curLayer) { }

        PcpNodeIterator _curNode;
        SdfLayerRefPtrVector::const_iterator _curLayer;
    };

    /// Returns a Position object representing the current node and layer in
    /// the prim index.
    USD_API
    Position GetPosition() const;

private:
    void _Init();
    void _SkipEmptyNodes();

    const PcpPrimIndex* _index;
    bool _skipEmptyNodes;

    PcpNodeIterator _curNode;
    PcpNodeIterator _lastNode;
    SdfLayerRefPtrVector::const_iterator _curLayer;
    SdfLayerRefPtrVector::const_iterator _lastLayer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVER_H
