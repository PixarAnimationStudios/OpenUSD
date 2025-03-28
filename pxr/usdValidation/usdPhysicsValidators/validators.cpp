//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usdValidation/usdPhysicsValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/capsule_1.h"
#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/cylinder_1.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usdPhysics/collisionAPI.h"
#include "pxr/usd/usdPhysics/articulationRootAPI.h"
#include "pxr/usd/usdPhysics/joint.h"
#include "pxr/base/gf/transform.h"

#include <algorithm>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

inline bool ScaleIsUniform(const GfVec3d& scale)
{
    const double eps = 1.0e-5;
    // Find min and max scale values
    double lo, hi;

    if (scale[0] < scale[1])
    {
        lo = scale[0];
        hi = scale[1];
    }
    else
    {
        lo = scale[1];
        hi = scale[0];
    }

    if (scale[2] < lo)
    {
        lo = scale[2];
    }
    else if (scale[2] > hi)
    {
        hi = scale[2];
    }

    if (lo* hi < 0.0)
    {
        return false;   // opposite signs
    }

    return hi > 0.0 ? hi - lo <= eps * lo : lo - hi >= eps * hi;
}

bool IsDynamicBody(const UsdPrim& usdPrim, bool* outPhysicsAPIFound)
{
    const UsdPhysicsRigidBodyAPI rboAPI(usdPrim);
    if (rboAPI)
    {
        {
            bool isAPISchemaEnabled = false;
            rboAPI.GetRigidBodyEnabledAttr().Get(&isAPISchemaEnabled);

            // Prim is dynamic body off PhysicsAPI is present and enabled
            *outPhysicsAPIFound = true;
            return isAPISchemaEnabled;
        }
    }

    *outPhysicsAPIFound = false;
    return false;
}

bool HasDynamicBodyParent(const UsdPrim& usdPrim, UsdPrim* outBodyPrimPath)
{
    bool physicsAPIFound = false;
    UsdPrim parent = usdPrim;
    while (parent != usdPrim.GetStage()->GetPseudoRoot())
    {
        if (IsDynamicBody(parent, &physicsAPIFound))
        {
            *outBodyPrimPath = parent;
            return true;
        }

        if (physicsAPIFound)
        {
            *outBodyPrimPath = parent;
            return false;
        }

        parent = parent.GetParent();
    }
    return false;
}

bool CheckNestedArticulationRoot(const UsdPrim& usdPrim)
{
    UsdPrim parent = usdPrim.GetParent();
    while (parent && parent != usdPrim.GetStage()->GetPseudoRoot())
    {
        const UsdPhysicsArticulationRootAPI artAPI = UsdPhysicsArticulationRootAPI(parent);
        if (artAPI)
        {
            return true;
        }
        parent = parent.GetParent();
    }
    return false;
}


static
UsdValidationErrorVector
_GetRigidBodyErrors(const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{   
    UsdValidationErrorVector errors;

    const UsdPhysicsRigidBodyAPI rbAPI = UsdPhysicsRigidBodyAPI(usdPrim);

    if (rbAPI)
    {
        const UsdValidationErrorSites primErrorSites = {
            UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
        };

        
        // rigid body applied to xformable
        {
            if (!usdPrim.IsA<UsdGeomXformable>())
            {
                errors.emplace_back(
                    UsdPhysicsValidationErrorNameTokens->rigidBodyNonXformable,
                    UsdValidationErrorType::Error,
                    primErrorSites,
                    TfStringPrintf(
                        "Rigid body API has to be applied to a xformable prim, prim path: %s",
                        usdPrim.GetPath().GetText())
                );
            }
        }

        // check instancing
        {
            bool reportInstanceError = false;
            if (usdPrim.IsInstanceProxy())
            {
                reportInstanceError = true;

                bool kinematic = false;
                rbAPI.GetKinematicEnabledAttr().Get(&kinematic);
                if (kinematic)
                    reportInstanceError = false;

                bool enabled = false;
                rbAPI.GetRigidBodyEnabledAttr().Get(&enabled);
                if (!enabled)
                    reportInstanceError = false;

                if (reportInstanceError)
                {
                    errors.emplace_back(
                        UsdPhysicsValidationErrorNameTokens->rigidBodyNonInstanceable,
                        UsdValidationErrorType::Error,
                        primErrorSites,
                        TfStringPrintf(
                            "RigidBodyAPI on an instance proxy is not supported, prim path: %s",
                            usdPrim.GetPath().GetText())
                    );
                }
            }
        }

        // scale orientation check
        {
            const GfMatrix4d mat =
                UsdGeomXformable(usdPrim).ComputeLocalToWorldTransform(
                    UsdTimeCode::Default());
            const GfTransform tr(mat);
            const GfVec3d sc = tr.GetScale();

            if (!ScaleIsUniform(sc) &&
                tr.GetScaleOrientation().GetQuaternion() != GfQuaternion::GetIdentity())
            {
                errors.emplace_back(
                    UsdPhysicsValidationErrorNameTokens->rigidBodyOrientationScale,
                    UsdValidationErrorType::Error,
                    primErrorSites,
                    TfStringPrintf(
                        "ScaleOrientation is not supported for rigid bodies, prim path: %s",                        
                        usdPrim.GetPath().GetText())                        
                );
            }
        }

        // nested rigid body check
        {
            UsdPrim bodyParent = UsdPrim();
            if (HasDynamicBodyParent(usdPrim.GetParent(), &bodyParent))
            {
                bool hasResetXformStack = false;
                UsdPrim parent = usdPrim;
                while (parent != usdPrim.GetStage()->GetPseudoRoot() && parent != bodyParent)
                {
                    const UsdGeomXformable xform(parent);
                    if (xform && xform.GetResetXformStack())
                    {
                        hasResetXformStack = true;
                        break;
                    }
                    parent = parent.GetParent();
                }
                if (!hasResetXformStack)
                {
                    errors.emplace_back(
                        UsdPhysicsValidationErrorNameTokens->nestedRigidBody,
                        UsdValidationErrorType::Error,
                        primErrorSites,
                        TfStringPrintf(
                            "Rigid Body (%s) is missing xformstack reset, when child of "
                            "rigid body (%s) in hierarchy. Simulation of multiple "
                            "RigidBodyAPI's in a hierarchy will cause unpredicted "
                            "results. Please fix the hierarchy or use XformStack reset.",
                            usdPrim.GetPrimPath().GetText(),
                            bodyParent.GetPrimPath().GetText())
                        );
                }
            }
        }
    }

    return errors;
}

bool CheckNonUniformScale(const UsdPrim& usdPrim)
{
    const UsdGeomXformable xform(usdPrim);
    const GfTransform tr(
        xform.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

    const GfVec3d sc = tr.GetScale();
    return ScaleIsUniform(sc);
}


static
UsdValidationErrorVector
_GetColliderErrors(const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;

    const UsdPhysicsCollisionAPI collisionAPI = UsdPhysicsCollisionAPI(usdPrim);

    if (collisionAPI && usdPrim.IsA<UsdGeomGprim>())
    {        
        const UsdValidationErrorSites primErrorSites = {
            UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
        };

    if (usdPrim.IsA<UsdGeomSphere>() ||
        usdPrim.IsA<UsdGeomCapsule>() ||
        usdPrim.IsA<UsdGeomCapsule_1>() ||
        usdPrim.IsA<UsdGeomCylinder>() ||
        usdPrim.IsA<UsdGeomCylinder_1>() ||
        usdPrim.IsA<UsdGeomCone>() ||
        usdPrim.IsA<UsdGeomPoints>() )
    {
        // non uniform scale check
        if (!CheckNonUniformScale(usdPrim))
        {
            errors.emplace_back(
                UsdPhysicsValidationErrorNameTokens->colliderNonUniformScale,
                UsdValidationErrorType::Error,
                primErrorSites,
                TfStringPrintf(
                    "Non-uniform scale is not supported for %s geometry, prim path: %s",
                    usdPrim.GetTypeName().GetText(), usdPrim.GetPath().GetText())
            );
        }            
    }
        if (usdPrim.IsA<UsdGeomPoints>())
        {
            {
                const UsdGeomPoints shape(usdPrim);

                VtArray<float> widths;
                VtArray<GfVec3f> positions;
                shape.GetWidthsAttr().Get(&widths);
                shape.GetPointsAttr().Get(&positions);

                if (widths.empty() || positions.empty() || widths.size() != positions.size())
                {
                    errors.emplace_back(
                        UsdPhysicsValidationErrorNameTokens->colliderSpherePointsDataMissing,
                        UsdValidationErrorType::Error,
                        primErrorSites,
                        TfStringPrintf(
                            "UsdGeomPoints width or position array not filled or sizes do not match, prim path: %s",
                            usdPrim.GetPath().GetText())
                    );
                }
            }
        }

    }

    return errors;
}

static
UsdValidationErrorVector
_GetArticulationErrors(const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;

    const UsdPhysicsArticulationRootAPI artAPI = UsdPhysicsArticulationRootAPI(usdPrim);

    if (artAPI)
    {
        const UsdValidationErrorSites primErrorSites = {
            UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
        };


        // nested articulation check
        {
            // articulations
            // check for nested articulation roots, these are not supported    
            if (CheckNestedArticulationRoot(usdPrim))
            {
                errors.emplace_back(
                    UsdPhysicsValidationErrorNameTokens->nestedArticulation,
                    UsdValidationErrorType::Error,
                    primErrorSites,
                    TfStringPrintf(
                        "Nested ArticulationRootAPI not supported, "
                        "prim %s.",
                        usdPrim.GetPrimPath().GetText())
                );
            }
        }

        // rigid body static or kinematic error
        {
            const UsdPhysicsRigidBodyAPI rboAPI = UsdPhysicsRigidBodyAPI(usdPrim);

            if (rboAPI)
            {
                bool bodyEnabled = false;
                rboAPI.GetRigidBodyEnabledAttr().Get(&bodyEnabled);
                if (!bodyEnabled)
                {
                    errors.emplace_back(
                        UsdPhysicsValidationErrorNameTokens->articulationOnStaticBody,
                        UsdValidationErrorType::Error,
                        primErrorSites,
                        TfStringPrintf(
                            "ArticulationRootAPI definition on a "
                            "static rigid body is not allowed. "                            
                            "Prim: %s",
                            usdPrim.GetPrimPath().GetText())
                    );
                }

                bool kinematicEnabled = false;
                rboAPI.GetKinematicEnabledAttr().Get(&kinematicEnabled);
                if (kinematicEnabled)
                {
                    errors.emplace_back(
                        UsdPhysicsValidationErrorNameTokens->articulationOnKinematicBody,
                        UsdValidationErrorType::Error,
                        primErrorSites,
                        TfStringPrintf(
                            "ArticulationRootAPI definition on a "
                            "kinematic rigid body is not allowed. "
                            "Prim: %s",
                            usdPrim.GetPrimPath().GetText())
                    );
                }
            }
        }

    }

    return errors;
}

SdfPath GetRel(const UsdRelationship& ref)
{
    SdfPathVector targets;
    ref.GetTargets(&targets);

    if (targets.size() == 0)
    {
        return SdfPath();
    }

    return targets.at(0);
}

bool CheckJointRel(const SdfPath& relPath, const UsdPrim& jointPrim)
{
    if (relPath == SdfPath())
        return true;

    const UsdPrim relPrim = jointPrim.GetStage()->GetPrimAtPath(relPath);
    if (!relPrim)
    {
        return false;
    }
    return true;
}


static
UsdValidationErrorVector
_GetPhysicsJointErrors(const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;

    const UsdPhysicsJoint physicsJoint = UsdPhysicsJoint(usdPrim);

    if (physicsJoint)
    {
        const UsdValidationErrorSites primErrorSites = {
            UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath())
        };


        // valid rel prims
        {
            const SdfPath rel0 = GetRel(physicsJoint.GetBody0Rel());
            const SdfPath rel1 = GetRel(physicsJoint.GetBody1Rel());

            // check rel validity
            if (!CheckJointRel(rel0, usdPrim) || !CheckJointRel(
                rel1, usdPrim))
            {
                errors.emplace_back(
                    UsdPhysicsValidationErrorNameTokens->jointInvalidPrimRel,
                    UsdValidationErrorType::Error,
                    primErrorSites,
                    TfStringPrintf(
                        "Joint (%s) body relationship points to a non "
                        "existent prim, joint will not be parsed.",
                        usdPrim.GetPrimPath().GetText())
                );
            }
        }

        // multiple rel prims
        {
            SdfPathVector targets0;
            SdfPathVector targets1;
            
            physicsJoint.GetBody0Rel().GetTargets(&targets0);
            physicsJoint.GetBody1Rel().GetTargets(&targets1);

            // check rel validity
            if (targets0.size() > 1 || targets1.size() > 1)
            {
                errors.emplace_back(
                    UsdPhysicsValidationErrorNameTokens->jointMultiplePrimsRel,
                    UsdValidationErrorType::Error,
                    primErrorSites,
                    TfStringPrintf(
                        "Joint prim does have relationship to multiple "
                        "bodies this is not supported, jointPrim %s",
                        usdPrim.GetPrimPath().GetText())
                    );
            }
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdPhysicsValidatorNameTokens->rigidBodyChecker, 
        _GetRigidBodyErrors);
    
    registry.RegisterPluginValidator(
        UsdPhysicsValidatorNameTokens->colliderChecker,
        _GetColliderErrors);

    registry.RegisterPluginValidator(
        UsdPhysicsValidatorNameTokens->articulationChecker,
        _GetArticulationErrors);

    registry.RegisterPluginValidator(
        UsdPhysicsValidatorNameTokens->physicsJointChecker,
        _GetPhysicsJointErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
