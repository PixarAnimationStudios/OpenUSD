//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "pxr/base/tf/pyModule.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE {
  TF_WRAP(Rendering);
  TF_WRAP(LinkGeometry);
  TF_WRAP(Spatial);
  TF_WRAP(TextLayout);
  TF_WRAP(NodeData);
  TF_WRAP(GraphLayout);
  TF_WRAP(VertexCache);
  TF_WRAP(RenderManagers);
  TF_WRAP(GraphRenderer);
  TF_WRAP(Undo);
  TF_WRAP(UsdPrimRegistry);
}
