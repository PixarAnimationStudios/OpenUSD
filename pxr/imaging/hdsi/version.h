//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_VERSION_H
#define PXR_IMAGING_HDSI_VERSION_H

// 10 -> 11: Adding HdsiPrimManagingSceneIndexObserver and
//           HdsiPrimTypeNoticeBatchingSceneIndex
// 11 -> 12: Adding HdsiPrimManagingSceneIndexObserver::GetTypedPrim.
// 12 -> 13: Adding HdsiLightLinkingSceneIndex.
// 13 -> 14: Add utilities for evaluating expressions on pruning collections.
// 14 -> 15: Fix VelocityMotionResolvingSceneIndex's handling of instance
//           scales; fixes for correct behavior in a motion blur context.
// 15 -> 16: Introducing HdsiDomeLightCameraVisibilitySceneIndex.

#define HDSI_API_VERSION 16

#endif // PXR_IMAGING_HDSI_VERSION_H
