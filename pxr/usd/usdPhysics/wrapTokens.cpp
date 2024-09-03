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

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdPhysicsTokens->name.GetString(); });

void wrapUsdPhysicsTokens()
{
    boost::python::class_<UsdPhysicsTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _ADD_TOKEN(cls, acceleration);
    _ADD_TOKEN(cls, angular);
    _ADD_TOKEN(cls, boundingCube);
    _ADD_TOKEN(cls, boundingSphere);
    _ADD_TOKEN(cls, colliders);
    _ADD_TOKEN(cls, convexDecomposition);
    _ADD_TOKEN(cls, convexHull);
    _ADD_TOKEN(cls, distance);
    _ADD_TOKEN(cls, drive);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsDamping);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsMaxForce);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsStiffness);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsTargetPosition);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsTargetVelocity);
    _ADD_TOKEN(cls, drive_MultipleApplyTemplate_PhysicsType);
    _ADD_TOKEN(cls, force);
    _ADD_TOKEN(cls, kilogramsPerUnit);
    _ADD_TOKEN(cls, limit);
    _ADD_TOKEN(cls, limit_MultipleApplyTemplate_PhysicsHigh);
    _ADD_TOKEN(cls, limit_MultipleApplyTemplate_PhysicsLow);
    _ADD_TOKEN(cls, linear);
    _ADD_TOKEN(cls, meshSimplification);
    _ADD_TOKEN(cls, none);
    _ADD_TOKEN(cls, physicsAngularVelocity);
    _ADD_TOKEN(cls, physicsApproximation);
    _ADD_TOKEN(cls, physicsAxis);
    _ADD_TOKEN(cls, physicsBody0);
    _ADD_TOKEN(cls, physicsBody1);
    _ADD_TOKEN(cls, physicsBreakForce);
    _ADD_TOKEN(cls, physicsBreakTorque);
    _ADD_TOKEN(cls, physicsCenterOfMass);
    _ADD_TOKEN(cls, physicsCollisionEnabled);
    _ADD_TOKEN(cls, physicsConeAngle0Limit);
    _ADD_TOKEN(cls, physicsConeAngle1Limit);
    _ADD_TOKEN(cls, physicsDensity);
    _ADD_TOKEN(cls, physicsDiagonalInertia);
    _ADD_TOKEN(cls, physicsDynamicFriction);
    _ADD_TOKEN(cls, physicsExcludeFromArticulation);
    _ADD_TOKEN(cls, physicsFilteredGroups);
    _ADD_TOKEN(cls, physicsFilteredPairs);
    _ADD_TOKEN(cls, physicsGravityDirection);
    _ADD_TOKEN(cls, physicsGravityMagnitude);
    _ADD_TOKEN(cls, physicsInvertFilteredGroups);
    _ADD_TOKEN(cls, physicsJointEnabled);
    _ADD_TOKEN(cls, physicsKinematicEnabled);
    _ADD_TOKEN(cls, physicsLocalPos0);
    _ADD_TOKEN(cls, physicsLocalPos1);
    _ADD_TOKEN(cls, physicsLocalRot0);
    _ADD_TOKEN(cls, physicsLocalRot1);
    _ADD_TOKEN(cls, physicsLowerLimit);
    _ADD_TOKEN(cls, physicsMass);
    _ADD_TOKEN(cls, physicsMaxDistance);
    _ADD_TOKEN(cls, physicsMergeGroup);
    _ADD_TOKEN(cls, physicsMinDistance);
    _ADD_TOKEN(cls, physicsPrincipalAxes);
    _ADD_TOKEN(cls, physicsRestitution);
    _ADD_TOKEN(cls, physicsRigidBodyEnabled);
    _ADD_TOKEN(cls, physicsSimulationOwner);
    _ADD_TOKEN(cls, physicsStartsAsleep);
    _ADD_TOKEN(cls, physicsStaticFriction);
    _ADD_TOKEN(cls, physicsUpperLimit);
    _ADD_TOKEN(cls, physicsVelocity);
    _ADD_TOKEN(cls, rotX);
    _ADD_TOKEN(cls, rotY);
    _ADD_TOKEN(cls, rotZ);
    _ADD_TOKEN(cls, transX);
    _ADD_TOKEN(cls, transY);
    _ADD_TOKEN(cls, transZ);
    _ADD_TOKEN(cls, x);
    _ADD_TOKEN(cls, y);
    _ADD_TOKEN(cls, z);
    _ADD_TOKEN(cls, PhysicsArticulationRootAPI);
    _ADD_TOKEN(cls, PhysicsCollisionAPI);
    _ADD_TOKEN(cls, PhysicsCollisionGroup);
    _ADD_TOKEN(cls, PhysicsDistanceJoint);
    _ADD_TOKEN(cls, PhysicsDriveAPI);
    _ADD_TOKEN(cls, PhysicsFilteredPairsAPI);
    _ADD_TOKEN(cls, PhysicsFixedJoint);
    _ADD_TOKEN(cls, PhysicsJoint);
    _ADD_TOKEN(cls, PhysicsLimitAPI);
    _ADD_TOKEN(cls, PhysicsMassAPI);
    _ADD_TOKEN(cls, PhysicsMaterialAPI);
    _ADD_TOKEN(cls, PhysicsMeshCollisionAPI);
    _ADD_TOKEN(cls, PhysicsPrismaticJoint);
    _ADD_TOKEN(cls, PhysicsRevoluteJoint);
    _ADD_TOKEN(cls, PhysicsRigidBodyAPI);
    _ADD_TOKEN(cls, PhysicsScene);
    _ADD_TOKEN(cls, PhysicsSphericalJoint);
}
