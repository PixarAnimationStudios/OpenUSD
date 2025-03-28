//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_GL_VERSION_H
#define PXR_USD_IMAGING_USD_IMAGING_GL_VERSION_H

// 0 -> 1: added IDRenderColor decode and direct Rprim path fetching.
// 1 -> 2: added RenderParams::enableUsdDrawModes
// 2 -> 3: refactor picking API.
// 3 -> 4: Add "instancerContext" to new picking API.
// 4 -> 5: Use UsdImagingGLEngine::_GetSceneDelegate() instead of _delegate.
// 5 -> 6: Use UsdImagingGLEngine::_GetHdEngine() instead of _engine.
// 6 -> 7: Added UsdImagingGLEngine::_GetTaskController() and _IsUsingLegacyImpl()
// 7 -> 8: Added outHitNormal parameter to UsdImagingGLEngine::TestIntersection()
// 8 -> 9: Removed the "HydraDisabled" renderer (i.e. LegacyEngine).
// 9 -> 10: Added new UsdImagingGLEngine::TestIntersection() method with resolve mode
// 10 -> 11: Removed UsdImagingGLRenderParams::enableIdRender.
#define USDIMAGINGGL_API_VERSION 11

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_VERSION_H

