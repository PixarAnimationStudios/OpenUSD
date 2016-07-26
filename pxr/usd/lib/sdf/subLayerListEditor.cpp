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
/// \file SubLayerListEditor.cpp


#include "pxr/usd/sdf/subLayerListEditor.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/mallocTag.h"

Sdf_SubLayerListEditor::Sdf_SubLayerListEditor(
    const SdfLayerHandle& owner)
    : Parent(owner->GetPseudoRoot(), 
             SdfFieldKeys->SubLayers, SdfListOpTypeOrdered)
{
}

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
    if (not TF_VERIFY(oldValues.size() == oldLayerOffsets.size(),
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
