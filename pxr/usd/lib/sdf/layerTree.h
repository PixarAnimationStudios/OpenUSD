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
#ifndef SDF_LAYER_TREE_H
#define SDF_LAYER_TREE_H

/// \file sdf/layerTree.h

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layerOffset.h"
#include <boost/noncopyable.hpp>
#include <vector>

// Layer tree forward declarations.
class SdfLayerTree;
typedef TfRefPtr<SdfLayerTree> SdfLayerTreeHandle;
typedef std::vector<SdfLayerTreeHandle> SdfLayerTreeHandleVector;

SDF_DECLARE_HANDLES(SdfLayer);

/// \class SdfLayerTree
///
/// A SdfLayerTree is an immutable tree structure representing a sublayer
/// stack and its recursive structure.
///
/// Layers can have sublayers, which can in turn have sublayers of their
/// own.  Clients that want to represent that hierarchical structure in
/// memory can build a SdfLayerTree for that purpose.
///
/// We use TfRefPtr<SdfLayerTree> as handles to LayerTrees, as a simple way
/// to pass them around as immutable trees without worrying about lifetime.
///
class SdfLayerTree : public TfRefBase, public TfWeakBase, boost::noncopyable {
public:
    /// Create a new layer tree node.
    static SdfLayerTreeHandle
    New( const SdfLayerHandle & layer,
         const SdfLayerTreeHandleVector & childTrees,
         const SdfLayerOffset & cumulativeOffset = SdfLayerOffset() );

    /// Returns the layer handle this tree node represents.
    const SdfLayerHandle & GetLayer() const;

    /// Returns the cumulative layer offset from the root of the tree.
    const SdfLayerOffset & GetOffset() const;

    /// Returns the children of this tree node.
    const SdfLayerTreeHandleVector & GetChildTrees() const;

private:
    SdfLayerTree( const SdfLayerHandle & layer,
                  const SdfLayerTreeHandleVector & childTrees,
                  const SdfLayerOffset & cumulativeOffset );

private:
    const SdfLayerHandle _layer;
    const SdfLayerOffset _offset;
    const SdfLayerTreeHandleVector _childTrees;
};

#endif
