//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdPrman_RenderParam;

/// \class HdPrmanMaterial
///
/// A representation for materials (including displacement) in prman.
///
class HdPrmanMaterial final : public HdMaterial 
{
public:
    HdPrmanMaterial(SdfPath const& id);
    ~HdPrmanMaterial() override;

    /// Synchronizes state from the delegate to this object.
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    riley::MaterialId GetMaterialId() const { return _materialId; }
    riley::DisplacementId GetDisplacementId() const { return _displacementId; }

    /// Return true if this material is valid.
    bool IsValid() const;

    void Finalize(HdRenderParam *renderParam) override;

    /// Return the static list of tokens supported.
    static TfTokenVector const& GetShaderSourceTypes();

    /// Return the material network after filtering.
    HdMaterialNetwork2 const& GetMaterialNetwork() const;

private:
    void _ResetMaterial(HdPrman_RenderParam *renderParam);

    riley::MaterialId _materialId;
    riley::DisplacementId _displacementId;

    HdMaterialNetwork2 _materialNetwork;
};

/// Helper function for converting an HdMaterialNetwork into Riley shading
/// nodes. Lights and light filters, in addition to materials, need to be able
/// to perform this conversion.
bool
HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
    HdMaterialNetwork2 const& network,
    SdfPath const& nodePath,
    std::vector<riley::ShadingNode> *result);

/// Return the fallback surface material network description.  This network
/// is meant to resemble Storm's fallback material.  It uses displayColor,
/// displayRoughness, displayOpacity, and displayMetallic.
HdMaterialNetwork2
HdPrmanMaterial_GetFallbackSurfaceMaterialNetwork();

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H
