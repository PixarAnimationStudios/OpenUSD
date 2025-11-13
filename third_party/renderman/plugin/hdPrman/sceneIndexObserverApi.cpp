//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/sceneIndexObserverApi.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

TF_DEFINE_ENV_SETTING(HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER, false,
                      "Enables the incomplete pure Hydra 2.0 implementation of "
                      "hdPrman as a scene index observer. "
                      "When this env var is enabled, filtering scene indices "
                      "will convert supported geometry to riley:FOO prims "
                      "that are picked up by the riley scene index observer "
                      "rather than resulting in legacy Hd[RBS]Prim's. "
                      "See HdPrmanRenderDelegate::_RileySceneIndices for more.");

#endif

PXR_NAMESPACE_CLOSE_SCOPE
