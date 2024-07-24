//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdPhysics/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdPhysicsTokens()
{
    boost::python::class_<UsdPhysicsTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "acceleration", UsdPhysicsTokens->acceleration);
    _AddToken(cls, "angular", UsdPhysicsTokens->angular);
    _AddToken(cls, "boundingCube", UsdPhysicsTokens->boundingCube);
    _AddToken(cls, "boundingSphere", UsdPhysicsTokens->boundingSphere);
    _AddToken(cls, "colliders", UsdPhysicsTokens->colliders);
    _AddToken(cls, "convexDecomposition", UsdPhysicsTokens->convexDecomposition);
    _AddToken(cls, "convexHull", UsdPhysicsTokens->convexHull);
    _AddToken(cls, "distance", UsdPhysicsTokens->distance);
    _AddToken(cls, "drive", UsdPhysicsTokens->drive);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsDamping", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsDamping);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsMaxForce", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsMaxForce);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsStiffness", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsStiffness);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsTargetPosition", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsTargetPosition);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsTargetVelocity", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsTargetVelocity);
    _AddToken(cls, "drive_MultipleApplyTemplate_PhysicsType", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsType);
    _AddToken(cls, "force", UsdPhysicsTokens->force);
    _AddToken(cls, "kilogramsPerUnit", UsdPhysicsTokens->kilogramsPerUnit);
    _AddToken(cls, "limit", UsdPhysicsTokens->limit);
    _AddToken(cls, "limit_MultipleApplyTemplate_PhysicsHigh", UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh);
    _AddToken(cls, "limit_MultipleApplyTemplate_PhysicsLow", UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow);
    _AddToken(cls, "linear", UsdPhysicsTokens->linear);
    _AddToken(cls, "meshSimplification", UsdPhysicsTokens->meshSimplification);
    _AddToken(cls, "none", UsdPhysicsTokens->none);
    _AddToken(cls, "physicsAngularVelocity", UsdPhysicsTokens->physicsAngularVelocity);
    _AddToken(cls, "physicsApproximation", UsdPhysicsTokens->physicsApproximation);
    _AddToken(cls, "physicsAxis", UsdPhysicsTokens->physicsAxis);
    _AddToken(cls, "physicsBody0", UsdPhysicsTokens->physicsBody0);
    _AddToken(cls, "physicsBody1", UsdPhysicsTokens->physicsBody1);
    _AddToken(cls, "physicsBreakForce", UsdPhysicsTokens->physicsBreakForce);
    _AddToken(cls, "physicsBreakTorque", UsdPhysicsTokens->physicsBreakTorque);
    _AddToken(cls, "physicsCenterOfMass", UsdPhysicsTokens->physicsCenterOfMass);
    _AddToken(cls, "physicsCollisionEnabled", UsdPhysicsTokens->physicsCollisionEnabled);
    _AddToken(cls, "physicsConeAngle0Limit", UsdPhysicsTokens->physicsConeAngle0Limit);
    _AddToken(cls, "physicsConeAngle1Limit", UsdPhysicsTokens->physicsConeAngle1Limit);
    _AddToken(cls, "physicsDensity", UsdPhysicsTokens->physicsDensity);
    _AddToken(cls, "physicsDiagonalInertia", UsdPhysicsTokens->physicsDiagonalInertia);
    _AddToken(cls, "physicsDynamicFriction", UsdPhysicsTokens->physicsDynamicFriction);
    _AddToken(cls, "physicsExcludeFromArticulation", UsdPhysicsTokens->physicsExcludeFromArticulation);
    _AddToken(cls, "physicsFilteredGroups", UsdPhysicsTokens->physicsFilteredGroups);
    _AddToken(cls, "physicsFilteredPairs", UsdPhysicsTokens->physicsFilteredPairs);
    _AddToken(cls, "physicsGravityDirection", UsdPhysicsTokens->physicsGravityDirection);
    _AddToken(cls, "physicsGravityMagnitude", UsdPhysicsTokens->physicsGravityMagnitude);
    _AddToken(cls, "physicsInvertFilteredGroups", UsdPhysicsTokens->physicsInvertFilteredGroups);
    _AddToken(cls, "physicsJointEnabled", UsdPhysicsTokens->physicsJointEnabled);
    _AddToken(cls, "physicsKinematicEnabled", UsdPhysicsTokens->physicsKinematicEnabled);
    _AddToken(cls, "physicsLocalPos0", UsdPhysicsTokens->physicsLocalPos0);
    _AddToken(cls, "physicsLocalPos1", UsdPhysicsTokens->physicsLocalPos1);
    _AddToken(cls, "physicsLocalRot0", UsdPhysicsTokens->physicsLocalRot0);
    _AddToken(cls, "physicsLocalRot1", UsdPhysicsTokens->physicsLocalRot1);
    _AddToken(cls, "physicsLowerLimit", UsdPhysicsTokens->physicsLowerLimit);
    _AddToken(cls, "physicsMass", UsdPhysicsTokens->physicsMass);
    _AddToken(cls, "physicsMaxDistance", UsdPhysicsTokens->physicsMaxDistance);
    _AddToken(cls, "physicsMergeGroup", UsdPhysicsTokens->physicsMergeGroup);
    _AddToken(cls, "physicsMinDistance", UsdPhysicsTokens->physicsMinDistance);
    _AddToken(cls, "physicsPrincipalAxes", UsdPhysicsTokens->physicsPrincipalAxes);
    _AddToken(cls, "physicsRestitution", UsdPhysicsTokens->physicsRestitution);
    _AddToken(cls, "physicsRigidBodyEnabled", UsdPhysicsTokens->physicsRigidBodyEnabled);
    _AddToken(cls, "physicsSimulationOwner", UsdPhysicsTokens->physicsSimulationOwner);
    _AddToken(cls, "physicsStartsAsleep", UsdPhysicsTokens->physicsStartsAsleep);
    _AddToken(cls, "physicsStaticFriction", UsdPhysicsTokens->physicsStaticFriction);
    _AddToken(cls, "physicsUpperLimit", UsdPhysicsTokens->physicsUpperLimit);
    _AddToken(cls, "physicsVelocity", UsdPhysicsTokens->physicsVelocity);
    _AddToken(cls, "rotX", UsdPhysicsTokens->rotX);
    _AddToken(cls, "rotY", UsdPhysicsTokens->rotY);
    _AddToken(cls, "rotZ", UsdPhysicsTokens->rotZ);
    _AddToken(cls, "transX", UsdPhysicsTokens->transX);
    _AddToken(cls, "transY", UsdPhysicsTokens->transY);
    _AddToken(cls, "transZ", UsdPhysicsTokens->transZ);
    _AddToken(cls, "x", UsdPhysicsTokens->x);
    _AddToken(cls, "y", UsdPhysicsTokens->y);
    _AddToken(cls, "z", UsdPhysicsTokens->z);
    _AddToken(cls, "PhysicsArticulationRootAPI", UsdPhysicsTokens->PhysicsArticulationRootAPI);
    _AddToken(cls, "PhysicsCollisionAPI", UsdPhysicsTokens->PhysicsCollisionAPI);
    _AddToken(cls, "PhysicsCollisionGroup", UsdPhysicsTokens->PhysicsCollisionGroup);
    _AddToken(cls, "PhysicsDistanceJoint", UsdPhysicsTokens->PhysicsDistanceJoint);
    _AddToken(cls, "PhysicsDriveAPI", UsdPhysicsTokens->PhysicsDriveAPI);
    _AddToken(cls, "PhysicsFilteredPairsAPI", UsdPhysicsTokens->PhysicsFilteredPairsAPI);
    _AddToken(cls, "PhysicsFixedJoint", UsdPhysicsTokens->PhysicsFixedJoint);
    _AddToken(cls, "PhysicsJoint", UsdPhysicsTokens->PhysicsJoint);
    _AddToken(cls, "PhysicsLimitAPI", UsdPhysicsTokens->PhysicsLimitAPI);
    _AddToken(cls, "PhysicsMassAPI", UsdPhysicsTokens->PhysicsMassAPI);
    _AddToken(cls, "PhysicsMaterialAPI", UsdPhysicsTokens->PhysicsMaterialAPI);
    _AddToken(cls, "PhysicsMeshCollisionAPI", UsdPhysicsTokens->PhysicsMeshCollisionAPI);
    _AddToken(cls, "PhysicsPrismaticJoint", UsdPhysicsTokens->PhysicsPrismaticJoint);
    _AddToken(cls, "PhysicsRevoluteJoint", UsdPhysicsTokens->PhysicsRevoluteJoint);
    _AddToken(cls, "PhysicsRigidBodyAPI", UsdPhysicsTokens->PhysicsRigidBodyAPI);
    _AddToken(cls, "PhysicsScene", UsdPhysicsTokens->PhysicsScene);
    _AddToken(cls, "PhysicsSphericalJoint", UsdPhysicsTokens->PhysicsSphericalJoint);
}
