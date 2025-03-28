//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(Tokens);
    TF_WRAP(MeshTopologyValidation);
    TF_WRAP(MeshTopology);
    TF_WRAP(SubdivTags);
}
