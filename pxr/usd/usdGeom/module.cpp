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
    TF_WRAP(UsdGeomBBoxCache); 
    TF_WRAP(UsdGeomConstraintTarget);
    TF_WRAP(UsdGeomPrimvar);
    TF_WRAP(UsdGeomXformOp);
    TF_WRAP(UsdGeomXformCache);
    TF_WRAP(Metrics);
    
    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
