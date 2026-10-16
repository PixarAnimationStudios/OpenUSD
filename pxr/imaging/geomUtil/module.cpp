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
    TF_WRAP_NB(Tokens);
    TF_WRAP_NB(CapsuleMeshGenerator);
    TF_WRAP_NB(ConeMeshGenerator);
    TF_WRAP_NB(CuboidMeshGenerator);
    TF_WRAP_NB(CylinderMeshGenerator);
    TF_WRAP_NB(DiskMeshGenerator);
    TF_WRAP_NB(PlaneMeshGenerator);
    TF_WRAP_NB(SphereMeshGenerator);
}
