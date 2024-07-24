//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file SubLayerListEditor.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/subLayerListEditor.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

Sdf_SubLayerListEditor::Sdf_SubLayerListEditor(
    const SdfLayerHandle& owner)
    : Parent(owner->GetPseudoRoot(), 
             SdfFieldKeys->SubLayers, SdfListOpTypeOrdered)
{
}

Sdf_SubLayerListEditor::~Sdf_SubLayerListEditor() = default;

void 
Sdf_SubLayerListEditor::_OnEdit(
    SdfListOpType op,
    const std::vector<std::string>& oldValues,
    const std::vector<std::string>& newValues) const
{
    // When sublayer paths are added or removed, we need to keep the
    // sublayer offsets vector (stored in a separate field) in sync.
    const SdfLayerOffsetVector oldLayerOffsets = _GetOwner()->
        GetFieldAs<SdfLayerOffsetVector>(SdfFieldKeys->SubLayerOffsets);

    // If this is ever the case, bad things will probably happen as code
    // in SdfLayer assumes the two vectors are in sync.
    if (!TF_VERIFY(oldValues.size() == oldLayerOffsets.size(),
                  "Sublayer offsets do not match sublayer paths")) {
        return;
    }

    // Rebuild the layer offsets vector, retaining offsets.
    SdfLayerOffsetVector newLayerOffsets(newValues.size());
    for (size_t i = 0; i < newValues.size(); ++i) {
        const std::string& newLayer = newValues[i];

        std::vector<std::string>::const_iterator oldValuesIt = 
            std::find(oldValues.begin(), oldValues.end(), newLayer);
        if (oldValuesIt == oldValues.end()) {
            continue;
        }
        
        const size_t oldLayerOffsetIndex = 
            std::distance(oldValues.begin(), oldValuesIt);
        newLayerOffsets[i] = oldLayerOffsets[oldLayerOffsetIndex];
    }
    
    _GetOwner()->SetField(SdfFieldKeys->SubLayerOffsets, newLayerOffsets);
}

PXR_NAMESPACE_CLOSE_SCOPE
