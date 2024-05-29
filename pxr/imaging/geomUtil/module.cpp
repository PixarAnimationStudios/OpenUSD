//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(CapsuleMeshGenerator);
    TF_WRAP(ConeMeshGenerator);
    TF_WRAP(CuboidMeshGenerator);
    TF_WRAP(CylinderMeshGenerator);
    TF_WRAP(SphereMeshGenerator);
}
