//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_FALLBACK_MATERIAL_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_FALLBACK_MATERIAL_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneIndexBase;
class SdfPath;

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

/// \namespace HdPrman_RileyFallbackMaterial
///
/// Utility functions to provide a retained scene index with the hard-coded 
/// fallback material and the scene index prim path.
///
/// This is part of the HdPrman 2.0 pathway and instantiated by
/// HdPrman_Renderer2SceneIndexPlugin.
///
/// Only active if HDPRMAN_USE_SCENE_INDEX_OBSERVER is set to
/// true.
///
namespace HdPrman_RileyFallbackMaterial
{
    /// Returns the path to the fallback material prim.
    HDPRMAN_API
    const SdfPath &GetPrimPath();

    /// Creates a retained scene index containing the fallback material prim.
    HDPRMAN_API
    HdSceneIndexBaseRefPtr
    CreateRetainedSceneIndex();

    /// Appends the fallback material scene index to the input scene index.
    /// XXX
    /// This function currently returns a merging scene index combining the
    /// input scene and the fallback material scene index.
    /// This could result in inefficiences due to the notice processing
    /// overhead of the merging scene index.
    /// Since its use is guarded by HDPRMAN_USE_SCENE_INDEX_OBSERVER, it is not
    /// an immediate concern.
    ///
    HDPRMAN_API
    HdSceneIndexBaseRefPtr
    AppendSceneIndex(const HdSceneIndexBaseRefPtr &inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_FALLBACK_MATERIAL_H

