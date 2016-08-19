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
#ifndef USD_RESOLVER_H
#define USD_RESOLVER_H

#include "pxr/usd/usd/common.h"

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/iterator.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/declareHandles.h"

#include <boost/scoped_ptr.hpp>

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
    bool NextLayer();

    /// Skips all pending layers in the current LayerStack and jumps to
    /// the next weaker PcpNode. When no more nodes are available, the resolver
    /// will be marked as invalid.
    void NextNode();

    /// Returns the current PCP node. 
    ///
    /// This is useful for coarse grained resolution tasks, however
    /// individual layers must be inspected in the common case.
    PcpNodeRef GetNode() const;

    /// Returns the current layer for the current PcpNode.
    ///
    /// PERFORMANCE: This returns a const-ref to avoid ref-count bumps during
    /// resolution. This is safe under the assumption that no changes will occur
    /// during resolution and that the lifetime of this object will be short.
    const SdfLayerRefPtr& GetLayer() const;

    /// Returns a translated path for the current PcpNode and Layer.
    const SdfPath& GetLocalPath() const;

    /// Returns the PcpPrimIndex. 
    ///
    /// This value is initialized when the resolver is constructed and does not
    /// change as a result of calling NextLayer() or NextNode().
    const PcpPrimIndex* GetPrimIndex() const;

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

#endif // USD_RESOLVER_H
