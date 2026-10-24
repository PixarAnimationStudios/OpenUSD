//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB( Engine );
    TF_WRAP_NB( RenderParams );
    TF_WRAP_NB( RendererCommands );
    TF_WRAP_NB( RendererSettings );
}
