//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SCENE_INDEX_OBSERVER_API_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SCENE_INDEX_OBSERVER_API_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"

// There was no hdsi/version.h before this HD_API_VERSION.
#if HD_API_VERSION >= 58 

#include "pxr/imaging/hdsi/version.h"

// HDPRMAN_USE_SCENE_INDEX_OBSERVER controls whether hdPrman uses the new
// HdsiPrimManagingSceneIndexObserver and other new API for the implementation
// as a scene index observer.
//
// We can only use for late enough versions of USD (that is 24.03 or later).
//
#if HDSI_API_VERSION >= 12
#define HDPRMAN_USE_SCENE_INDEX_OBSERVER
#endif

#endif // #if HD_API_VERSION >= 58

// Using HdsiPrimManagingSceneIndexObserver is controlled by env var
// HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER.

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/api.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

extern HDPRMAN_API TfEnvSetting<bool> HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_SCENE_INDEX_OBSERVER_API_H
