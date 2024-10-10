//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "Riley.h"
#include <mutex>

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

    /// Consult the HD_PRMAN_TEX_EXTS env var to determine which textures
    /// should be passed through without processing by the Rtx plug-in
    static bool IsTexExt(const std::string& ext);

    /// Return the material network after filtering.
    HdMaterialNetwork2 const& GetMaterialNetwork() const;

    /// Make sure this material has been updated in Riley.
    void SyncToRiley(
        HdSceneDelegate *sceneDelegate,
        riley::Riley *riley);

private:
    void _ResetMaterialWithLock(riley::Riley *riley);
    void _SyncToRileyWithLock(
        HdSceneDelegate *sceneDelegate,
        riley::Riley *riley);

    riley::MaterialId _materialId;
    riley::DisplacementId _displacementId;

    // XXX only used to set disp bound for UsdPreviewMaterial cases
    HdMaterialNetwork2 _materialNetwork;

    mutable std::mutex _syncToRileyMutex;
    bool _rileyIsInSync;
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

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATERIAL_H
