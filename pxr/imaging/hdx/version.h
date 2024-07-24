//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_VERSION_H
#define PXR_IMAGING_HDX_VERSION_H

// 1  -> 2  : split HdxRenderSetupTask out of HdxRenderTask
// 2  -> 3  : move simpleLightingShader to Hdx.
// 3  -> 4  : move camera and light to Hdx.
// 4  -> 5  : move drawTarget to Hdx.
// 5  -> 6  : change HdxShadowMatrixComputation signature.
// 6  -> 7  : make HdxShadowMatrixComputationSharedPtr std::shared_ptr instead of boost::shared_ptr
// 7  -> 8  : added another HdxShadowMatrixComputation signature.
// 8  -> 9  : added render index as argument to HdxSelectionTracker::GetSelectedPointColors.
// 9  -> 10 : replaced enableSelection with enableSelectionHighlight and 
//            enableLocateHighlight on HdxSelectionTask and HdxColorizeSelectionTask params.
// 10 -> 11 : New signature for HdxFullscreenShader::BindTextures.
// 11 -> 12 : change HdxPickHit::worldSpaceHitPoint from GfVec3f to GfVec3d.
// 12 -> 13 : Add HdxPickTask "resolveDeep" mode.
//
#define HDX_API_VERSION 13

#endif // PXR_IMAGING_HDX_VERSION_H
