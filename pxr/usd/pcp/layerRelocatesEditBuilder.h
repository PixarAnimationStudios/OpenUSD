//
// Copyright 2024 Pixar
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
#ifndef PXR_USD_PCP_LAYER_RELOCATES_EDIT_BUILDER_H
#define PXR_USD_PCP_LAYER_RELOCATES_EDIT_BUILDER_H

/// \file pcp/layerRelocatesEditBuilder.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/declarePtrs.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \class PcpLayerRelocatesEditBuilder
/// 
/// Utility class for building up a map of valid relocates and producing
/// the layer metadata edits that can be performed to set these relocates on a 
/// layer stack.
///
/// This class must be constructed from an existing PcpLayerStack which will
/// initialize the edit builder with the layer stack's current relocates. Then 
/// Relocate can be called any number of times to build a validly formatted map 
/// of edited relocates. This can then be asked for a list of layer metadata
/// edits that need to be performed to update the layer stack to have the edited
/// relocates.
/// 
/// This class is not stateful in regards to the layer stack or its layers. In
/// other words, the provided layer stack is only used to initialize the 
/// existing relocates and to get which layers should be the layers to edit.
/// This does not listen to any change notifications for the layers or the layer
/// stack or the PcpCache that built it. This is class is meant to be 
/// transiently used to build up a set of edits to perform on layers and then 
/// discarded.
class PcpLayerRelocatesEditBuilder {
public:

    /// Constructor that initializes the relocates map from the given 
    /// \p layerStack. 
    ///
    /// If \p addNewRelocatesLayer is provided, it must be a layer in the given 
    /// layer stack and any new relocates map entries created by calls to
    /// Relocate will be added as part of the edit for that layer. If 
    /// \p addNewRelocatesLayer is not provided, then the layer stack's root 
    /// layer will be used as the target edit location for new relocates.
    ///
    PCP_API
    PcpLayerRelocatesEditBuilder(
        const PcpLayerStackPtr &layerStack,
        const SdfLayerHandle &addNewRelocatesLayer = nullptr);

    /// Updates the relocates map and layer edits so that \p sourcePath is moved
    /// to \p targetPath in the edited relocates.
    ///
    /// Returns true if the relocate can be performed given the source and 
    /// target and current relocates map. Returns false and populates \p whyNot,
    /// (if it's not nullptr) with the reason why if the relocate cannot be
    /// performed.
    ///
    /// The edited relocates map will always conform to the relocates format 
    /// that is considered valid by the layer stack population and will 
    /// therefore not produce relocation errors when set as the layer stack's 
    /// authored relocates. Maintaining this format means that calling Relocate 
    /// can a cause a combination of different effects on the relocates map 
    /// depending the existing relocates at the time. These effects can include
    /// adding a new relocate entry, updating paths in existing entries, and 
    /// deleting existing entries. The following examples demonstrate many of 
    /// these behaviors.
    /// 
    /// Example 1:
    /// Existing relocates:
    ///   - </Root/A> -> </Root/B>
    /// Relocate(</Root/C>, </Root/D>) 
    ///   - Just adds a new relocate
    /// Result relocates:
    ///   - </Root/A> -> </Root/B>
    ///   - </Root/C> -> </Root/D>
    ///
    /// Example 2:
    /// Existing relocates:
    ///   - </Root/A> -> </Root/B>
    /// Relocate(</Root/B>, </Root/C>) 
    ///   - Updates existing relocate to point to </Root/C>
    /// Result relocates:
    ///   - </Root/A> -> </Root/C>
    ///
    /// Example 3:
    /// Existing relocates:
    ///   - </Root/A/Y> -> </Root/B/Y>
    ///   - </Root/A/X> -> </Root/B/X>
    /// Relocate(</Root/A>, </Root/B>) 
    ///   - Adds a new relocate but removes the existing relocates which become
    ///     redundant with their parents relocated.
    /// Result relocates:
    ///   - </Root/A> -> </Root/B>
    ///
    /// Example 4:
    /// Existing relocates:
    ///   - </Root/A/B> -> </Root/A/C>
    ///   - </Root/A/D> -> </Root/D>
    ///   - </Root/E> -> </Root/A/E>
    /// Relocate(</Root/A>, </Root/Z>) 
    ///   - Adds a new relocate and updates any existing relocates so that
    ///     their source and target paths are ancestrally relocated by the new
    ///     relocate.
    /// Result relocates:
    ///   - </Root/A> -> </Root/Z>
    ///   - </Root/Z/B> -> </Root/Z/C>
    ///   - </Root/Z/D> -> </Root/D>
    ///   - </Root/E> -> </Root/Z/E>
    ///
    /// Example 5:
    /// Existing relocates:
    ///   - </Root/A> -> </Root/B>
    /// Relocate(</Root/B>, </Root/A>) 
    ///   - Deletes the relocate that has been moved back to its original source
    /// Result relocates:
    ///   - none
    ///
    /// The source path provided to this function will be mapped across any 
    /// existing relocates to convert it to the most relocated source before it
    /// is used to edit the current relocates. This means the source path can be
    /// an original source path, a fully relocated path, or any partially 
    /// relocated path as long as it can be converted to a valid source path 
    /// when existing relocates are applied.
    ///
    /// For example, if we start with relocates map that already has the 
    /// mappings:
    ///   - </Root/A> -> </Root/X>
    ///   - </Root/X/B> -> </Root/X/Y>
    ///
    /// This is the equivalent of taking the prim hierarchy of /Root/A/B and 
    /// renaming/moving it be the hierarchy /Root/X/Y. Now say we want the 
    /// original path of /Root/A/B/C to now be moved to /Root/X/Y/Z. We have the
    /// option of adding this relocate in any of four ways:
    ///   1. Original source path
    ///      - Relocate(</Root/A/B/C>, </Root/X/Y/Z>) 
    ///   2. Fully relocated source path
    ///      - Relocate(</Root/X/Y/C>, </Root/X/Y/Z>)
    ///   3. Partially relocated source path -
    ///      - Relocate(</Root/X/B/C>, </Root/X/Y/Z>)
    ///   4. Subtle partially relocated source path -
    ///      - Relocate(</Root/A/Y/C>, </Root/X/Y/Z>)
    ///
    /// All of the example source paths become </Root/X/Y/C> by applying 
    /// existing relocates and adding the relocate using any of the four source
    /// paths gives us the same resulting relocates map:
    ///   </Root/A> -> </Root/X>
    ///   </Root/X/B> -> </Root/X/Y>
    ///   </Root/X/Y/C> -> </Root/X/Y/Z>
    ///
    /// The provided target path is never adjusted by existing relocations, 
    /// unlike the source path, and therefore must always be expressed as a 
    /// fully relocated final path. Thus, in the prior example, the only valid
    /// option for the target path that would've produced the move to 
    /// </Root/X/Y/Z> was the path </Root/X/Y/Z> itself. 
    PCP_API
    bool Relocate(
        const SdfPath &sourcePath, 
        const SdfPath &targetPath, 
        std::string *whyNot = nullptr);

    /// An edit is a layer and an SdfRelocates value to set in the layer's 
    /// 'layerRelocates' metatdata.
    using LayerRelocatesEdit = std::pair<SdfLayerHandle, SdfRelocates>;

    /// List of relocates edits to perform on all layers.
    using LayerRelocatesEdits = std::vector<LayerRelocatesEdit>;

    /// Returns a list of edits to perform on the layers of the layer stack this
    /// builder was initialized with that will update the layer stack to have
    /// the relocates returned by GetEditedRelocatesMap.
    ///
    /// The format of each edit is a pair consisting of a layer and an 
    /// SdfRelocates value. To perform each edit, set the 'layerRelocates' 
    /// field in the layer's metadata to be the new relocates value.
    /// For example:
    /// \code
    /// for (const auto &[layer, relocates] : relocatesEditBuilder.GetEdits()) {
    ///     layer->SetRelocates(relocates);
    /// }
    /// \endcode
    PCP_API
    LayerRelocatesEdits GetEdits() const;

    /// Returns a map of relocates composed from the edited layer relocates. 
    PCP_API
    const SdfRelocatesMap &GetEditedRelocatesMap() const;

private:
    bool _AddAndUpdateRelocates(
        const SdfPath &newSource, 
        const SdfPath &newTarget, 
        std::string *whyNot);

    void _RemoveRelocatesWithErrors(const PcpErrorVector &errors);

    mutable std::optional<SdfRelocatesMap> _relocatesMap;

    LayerRelocatesEdits _layerRelocatesEdits;
    SdfLayerHandleSet _layersWithRelocatesChanges;
    size_t _editForNewRelocatesIndex = ~0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_LAYER_RELOCATES_EDIT_BUILDER_H
