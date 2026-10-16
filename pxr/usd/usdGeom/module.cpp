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
    TF_WRAP_NB(UsdGeomBBoxCache); 
    TF_WRAP_NB(UsdGeomConstraintTarget);
    TF_WRAP_NB(UsdGeomPrimvar);
    TF_WRAP_NB(UsdGeomXformOp);
    TF_WRAP_NB(UsdGeomXformCache);
    TF_WRAP_NB(Metrics);
    
    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
