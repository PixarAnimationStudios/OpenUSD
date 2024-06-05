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
    TF_WRAP(UsdGeomModelAPI);
    TF_WRAP(UsdGeomPrimvar);
    TF_WRAP(UsdGeomPrimvarsAPI);
    TF_WRAP(UsdGeomTokens);
    TF_WRAP(UsdGeomXformOp);
    TF_WRAP(UsdGeomXformCommonAPI);
    TF_WRAP(UsdGeomXformCache);
    TF_WRAP(Metrics);
    TF_WRAP(UsdGeomMotionAPI);
    TF_WRAP(UsdGeomVisibilityAPI);
    
    // Generated schema.  Base classes must precede derived classes.
    // Indentation shows class hierarchy.
    TF_WRAP(UsdGeomImageable);
        TF_WRAP(UsdGeomScope);
        TF_WRAP(UsdGeomXformable);
            TF_WRAP(UsdGeomXform);
            TF_WRAP(UsdGeomCamera);
            TF_WRAP(UsdGeomBoundable);
                TF_WRAP(UsdGeomGprim);
                    TF_WRAP(UsdGeomCapsule);
                    TF_WRAP(UsdGeomCapsule_1);
                    TF_WRAP(UsdGeomCone);
                    TF_WRAP(UsdGeomCube);
                    TF_WRAP(UsdGeomCylinder);
                    TF_WRAP(UsdGeomCylinder_1);
                    TF_WRAP(UsdGeomSphere);
                    TF_WRAP(UsdGeomPlane);
                    TF_WRAP(UsdGeomPointBased);
                        TF_WRAP(UsdGeomMesh);
                        TF_WRAP(UsdGeomTetMesh);
                        TF_WRAP(UsdGeomNurbsPatch);
                        TF_WRAP(UsdGeomPoints);
                        TF_WRAP(UsdGeomCurves);
                            TF_WRAP(UsdGeomBasisCurves);
                            TF_WRAP(UsdGeomHermiteCurves);
                            TF_WRAP(UsdGeomNurbsCurves);
                TF_WRAP(UsdGeomPointInstancer);
    TF_WRAP(UsdGeomSubset);
}
