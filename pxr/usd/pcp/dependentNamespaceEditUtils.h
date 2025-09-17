//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_DEPENDENT_NAMESPACE_EDIT_UTILS_H
#define PXR_USD_PCP_DEPENDENT_NAMESPACE_EDIT_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
TF_DECLARE_REF_PTRS(PcpLayerStack);

/// Structure for bundling all the edits that need to be performed in order to 
/// perform a namespace edit and fix up downstream composition dependencies on
/// dependent prim indexes in dependent PcpCaches. This is the return value of
/// PcpGatherDependentNamespaceEdits.
class PcpDependentNamespaceEdits {
public:
    /// Description of an edit to a prim spec composition field, such as 
    /// references, inherits, or relocates.
    struct CompositionFieldEdit {
        /// Layer containing the prim spec to edit.
        SdfLayerHandle layer;

        /// Path of the prim spec to edit.
        SdfPath path;

        /// Name of the composition field.
        TfToken fieldName;

        /// New value of the composition field to set.
        VtValue newFieldValue;
    };

    /// List of all composition fields edits to perform.
    std::vector<CompositionFieldEdit> compositionFieldEdits;

    /// Description of move edit which consists of the old (source) path and the
    /// new (destination) path.
    struct MoveEditDescription {
        SdfPath oldPath;
        SdfPath newPath;
    };
    using MoveEditDescriptionVector = std::vector<MoveEditDescription>;

    /// Map of layer to the spec moves edits to perform on the layer.
    using LayerSpecMoveEdits = std::unordered_map<
        SdfLayerHandle, MoveEditDescriptionVector, TfHash>;
    LayerSpecMoveEdits layerSpecMoves;

    /// Map of layer to relocates value to set in the layer metadata relocates
    /// field.
    using LayerRelocatesEdits = 
        std::unordered_map<SdfLayerHandle, SdfRelocates, TfHash>;
    LayerRelocatesEdits dependentRelocatesEdits;

    /// Errors encountered during the processing of the dependent namespace 
    /// edits.
    std::vector<std::string> errors;

    /// Warnings encountered during the processing of the dependent namespace 
    /// edits.
    std::vector<std::string> warnings;

    /// Lists of composed prim paths in each affected cache whose prim indexes
    /// will need to be recomputed after the changes in this object are applied.
    /// This information can be useful during change processing and notification
    /// to help report the intended effects of all the layer spec edits that are
    /// performed during a namespace edit.
    std::unordered_map<const PcpCache *, MoveEditDescriptionVector> 
        dependentCachePathChanges;
};

/// Given a prim spec move edit from \p oldPrimPath to \p newPrimPath and the 
/// \p affectedLayers on which this spec move will be performed, this function
/// finds all prim indexes already cached in each PcpCache in \p dependentCaches
/// that would be affected by these edits and computes a full set of edits that
/// would be required to maintain these dependent prim indexes' composed prim
/// stacks, possibly moving the prim index to a new prim path if necessary.
/// If \p addRelocatesToLayerStack is provided, this will also add a new 
/// relocates edit to the necessary layers in the layer stack that moves
/// oldPrimPath to newPrimPath. The layer \p addRelocatesToLayerStackEditLayer
/// provided is only relevant when the relocates layer stack is also provided as
/// it determines which specific layer in the layer stack will have a new
/// relocates entry added to it (see PcpLayerRelocatesEditBuilder).
PCP_API
PcpDependentNamespaceEdits
PcpGatherDependentNamespaceEdits(
    const SdfPath &oldPrimPath,
    const SdfPath &newPrimPath,
    const SdfLayerHandleVector &affectedLayers,
    const PcpLayerStackRefPtr &addRelocatesToLayerStack,
    const SdfLayerHandle &addRelocatesToLayerStackEditLayer,
    const std::vector<const PcpCache *> &dependentCaches);

/// Gathers the list of layers that need to be edited to perform the spec move
/// from \p oldSpecPath to \p newSpecPath on the given \p layerStack. 
///
/// If any errors are encountered where the spec would not be able to performed
/// on a layer that needs to be edited, those errors will be added to \p errors.
/// Layers with errors are still included in the returned result regardless.
PCP_API
SdfLayerHandleVector
PcpGatherLayersToEditForSpecMove(
    const PcpLayerStackRefPtr &layerStack,
    const SdfPath &oldSpecPath,
    const SdfPath &newSpecPath,
    std::vector<std::string> *errors);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_NAMESPACE_EDITS_H
