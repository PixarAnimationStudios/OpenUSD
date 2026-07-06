//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ID_ASSIGNING_SCENE_INDEX_PLUGIN_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ID_ASSIGNING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"

// XXX what version are we now?
#if PXR_VERSION >= 2605

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPrman_IdSceneIndexPlugin
///
/// This plugin pulls a few bits of instancer schema data into primvars,
/// to assist with assigning ID's for instances of prototypes.  In
/// particular, this includes primOrigin/scenePath and
/// instancerTopology/instanceLocations.
///
/// \see HdPrman_IdMap
///
class HdPrman_IdSceneIndexPlugin
  : public HdSceneIndexPlugin
{
protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr& inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2605

#endif
