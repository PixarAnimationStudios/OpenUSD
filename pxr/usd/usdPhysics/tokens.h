//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPHYSICS_TOKENS_H
#define USDPHYSICS_TOKENS_H

/// \file usdPhysics/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdPhysicsTokensType
///
/// \link UsdPhysicsTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdPhysicsTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdPhysicsTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdPhysicsTokens->acceleration);
/// \endcode
struct UsdPhysicsTokensType {
    USDPHYSICS_API UsdPhysicsTokensType();
    /// \brief "acceleration"
    /// 
    /// Possible value for UsdPhysicsDriveAPI::GetTypeAttr()
    const TfToken acceleration;
    /// \brief "angular"
    /// 
    ///  This token represents the angular degree of freedom used in Revolute Joint Drive. 
    const TfToken angular;
    /// \brief "boundingCube"
    /// 
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken boundingCube;
    /// \brief "boundingSphere"
    /// 
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken boundingSphere;
    /// \brief "colliders"
    /// 
    ///  This token represents the collection name to use with UsdCollectionAPI to represent colliders of a CollisionGroup prim. 
    const TfToken colliders;
    /// \brief "convexDecomposition"
    /// 
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken convexDecomposition;
    /// \brief "convexHull"
    /// 
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken convexHull;
    /// \brief "distance"
    /// 
    ///  This token represents the distance limit used for generic D6 joint. 
    const TfToken distance;
    /// \brief "drive"
    /// 
    /// Property namespace prefix for the UsdPhysicsDriveAPI schema.
    const TfToken drive;
    /// \brief "drive:__INSTANCE_NAME__:physics:damping"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsDamping;
    /// \brief "drive:__INSTANCE_NAME__:physics:maxForce"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsMaxForce;
    /// \brief "drive:__INSTANCE_NAME__:physics:stiffness"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsStiffness;
    /// \brief "drive:__INSTANCE_NAME__:physics:targetPosition"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsTargetPosition;
    /// \brief "drive:__INSTANCE_NAME__:physics:targetVelocity"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsTargetVelocity;
    /// \brief "drive:__INSTANCE_NAME__:physics:type"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken drive_MultipleApplyTemplate_PhysicsType;
    /// \brief "force"
    /// 
    /// Fallback value for UsdPhysicsDriveAPI::GetTypeAttr()
    const TfToken force;
    /// \brief "kilogramsPerUnit"
    /// 
    /// Stage-level metadata that encodes a scene's linear unit of measure as kilograms per encoded unit.
    const TfToken kilogramsPerUnit;
    /// \brief "limit"
    /// 
    /// Property namespace prefix for the UsdPhysicsLimitAPI schema.
    const TfToken limit;
    /// \brief "limit:__INSTANCE_NAME__:physics:high"
    /// 
    /// UsdPhysicsLimitAPI
    const TfToken limit_MultipleApplyTemplate_PhysicsHigh;
    /// \brief "limit:__INSTANCE_NAME__:physics:low"
    /// 
    /// UsdPhysicsLimitAPI
    const TfToken limit_MultipleApplyTemplate_PhysicsLow;
    /// \brief "linear"
    /// 
    ///  This token represents the linear degree of freedom used in Prismatic Joint Drive. 
    const TfToken linear;
    /// \brief "meshSimplification"
    /// 
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken meshSimplification;
    /// \brief "none"
    /// 
    /// Fallback value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
    const TfToken none;
    /// \brief "physics:angularVelocity"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsAngularVelocity;
    /// \brief "physics:approximation"
    /// 
    /// UsdPhysicsMeshCollisionAPI
    const TfToken physicsApproximation;
    /// \brief "physics:axis"
    /// 
    /// UsdPhysicsRevoluteJoint, UsdPhysicsPrismaticJoint, UsdPhysicsSphericalJoint
    const TfToken physicsAxis;
    /// \brief "physics:body0"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsBody0;
    /// \brief "physics:body1"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsBody1;
    /// \brief "physics:breakForce"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsBreakForce;
    /// \brief "physics:breakTorque"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsBreakTorque;
    /// \brief "physics:centerOfMass"
    /// 
    /// UsdPhysicsMassAPI
    const TfToken physicsCenterOfMass;
    /// \brief "physics:collisionEnabled"
    /// 
    /// UsdPhysicsCollisionAPI, UsdPhysicsJoint
    const TfToken physicsCollisionEnabled;
    /// \brief "physics:coneAngle0Limit"
    /// 
    /// UsdPhysicsSphericalJoint
    const TfToken physicsConeAngle0Limit;
    /// \brief "physics:coneAngle1Limit"
    /// 
    /// UsdPhysicsSphericalJoint
    const TfToken physicsConeAngle1Limit;
    /// \brief "physics:density"
    /// 
    /// UsdPhysicsMassAPI, UsdPhysicsMaterialAPI
    const TfToken physicsDensity;
    /// \brief "physics:diagonalInertia"
    /// 
    /// UsdPhysicsMassAPI
    const TfToken physicsDiagonalInertia;
    /// \brief "physics:dynamicFriction"
    /// 
    /// UsdPhysicsMaterialAPI
    const TfToken physicsDynamicFriction;
    /// \brief "physics:excludeFromArticulation"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsExcludeFromArticulation;
    /// \brief "physics:filteredGroups"
    /// 
    /// UsdPhysicsCollisionGroup
    const TfToken physicsFilteredGroups;
    /// \brief "physics:filteredPairs"
    /// 
    /// UsdPhysicsFilteredPairsAPI
    const TfToken physicsFilteredPairs;
    /// \brief "physics:gravityDirection"
    /// 
    /// UsdPhysicsScene
    const TfToken physicsGravityDirection;
    /// \brief "physics:gravityMagnitude"
    /// 
    /// UsdPhysicsScene
    const TfToken physicsGravityMagnitude;
    /// \brief "physics:invertFilteredGroups"
    /// 
    /// UsdPhysicsCollisionGroup
    const TfToken physicsInvertFilteredGroups;
    /// \brief "physics:jointEnabled"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsJointEnabled;
    /// \brief "physics:kinematicEnabled"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsKinematicEnabled;
    /// \brief "physics:localPos0"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsLocalPos0;
    /// \brief "physics:localPos1"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsLocalPos1;
    /// \brief "physics:localRot0"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsLocalRot0;
    /// \brief "physics:localRot1"
    /// 
    /// UsdPhysicsJoint
    const TfToken physicsLocalRot1;
    /// \brief "physics:lowerLimit"
    /// 
    /// UsdPhysicsRevoluteJoint, UsdPhysicsPrismaticJoint
    const TfToken physicsLowerLimit;
    /// \brief "physics:mass"
    /// 
    /// UsdPhysicsMassAPI
    const TfToken physicsMass;
    /// \brief "physics:maxDistance"
    /// 
    /// UsdPhysicsDistanceJoint
    const TfToken physicsMaxDistance;
    /// \brief "physics:mergeGroup"
    /// 
    /// UsdPhysicsCollisionGroup
    const TfToken physicsMergeGroup;
    /// \brief "physics:minDistance"
    /// 
    /// UsdPhysicsDistanceJoint
    const TfToken physicsMinDistance;
    /// \brief "physics:principalAxes"
    /// 
    /// UsdPhysicsMassAPI
    const TfToken physicsPrincipalAxes;
    /// \brief "physics:restitution"
    /// 
    /// UsdPhysicsMaterialAPI
    const TfToken physicsRestitution;
    /// \brief "physics:rigidBodyEnabled"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsRigidBodyEnabled;
    /// \brief "physics:simulationOwner"
    /// 
    /// UsdPhysicsRigidBodyAPI, UsdPhysicsCollisionAPI
    const TfToken physicsSimulationOwner;
    /// \brief "physics:startsAsleep"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsStartsAsleep;
    /// \brief "physics:staticFriction"
    /// 
    /// UsdPhysicsMaterialAPI
    const TfToken physicsStaticFriction;
    /// \brief "physics:upperLimit"
    /// 
    /// UsdPhysicsRevoluteJoint, UsdPhysicsPrismaticJoint
    const TfToken physicsUpperLimit;
    /// \brief "physics:velocity"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsVelocity;
    /// \brief "rotX"
    /// 
    ///  This token represents the rotate around X axis degree of freedom used in Joint Limits and Drives. 
    const TfToken rotX;
    /// \brief "rotY"
    /// 
    ///  This token represents the rotate around Y axis degree of freedom used in Joint Limits and Drives. 
    const TfToken rotY;
    /// \brief "rotZ"
    /// 
    ///  This token represents the rotate around Z axis degree of freedom used in Joint Limits and Drives. 
    const TfToken rotZ;
    /// \brief "transX"
    /// 
    ///  This token represents the translate around X axis degree of freedom used in Joint Limits and Drives. 
    const TfToken transX;
    /// \brief "transY"
    /// 
    ///  This token represents the translate around Y axis degree of freedom used in Joint Limits and Drives. 
    const TfToken transY;
    /// \brief "transZ"
    /// 
    ///  This token represents the translate around Z axis degree of freedom used in Joint Limits and Drives. 
    const TfToken transZ;
    /// \brief "X"
    /// 
    /// Fallback value for UsdPhysicsRevoluteJoint::GetAxisAttr(), Fallback value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Fallback value for UsdPhysicsSphericalJoint::GetAxisAttr()
    const TfToken x;
    /// \brief "Y"
    /// 
    /// Possible value for UsdPhysicsRevoluteJoint::GetAxisAttr(), Possible value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Possible value for UsdPhysicsSphericalJoint::GetAxisAttr()
    const TfToken y;
    /// \brief "Z"
    /// 
    /// Possible value for UsdPhysicsRevoluteJoint::GetAxisAttr(), Possible value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Possible value for UsdPhysicsSphericalJoint::GetAxisAttr()
    const TfToken z;
    /// \brief "PhysicsArticulationRootAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsArticulationRootAPI
    const TfToken PhysicsArticulationRootAPI;
    /// \brief "PhysicsCollisionAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsCollisionAPI
    const TfToken PhysicsCollisionAPI;
    /// \brief "PhysicsCollisionGroup"
    /// 
    /// Schema identifer and family for UsdPhysicsCollisionGroup
    const TfToken PhysicsCollisionGroup;
    /// \brief "PhysicsDistanceJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsDistanceJoint
    const TfToken PhysicsDistanceJoint;
    /// \brief "PhysicsDriveAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsDriveAPI
    const TfToken PhysicsDriveAPI;
    /// \brief "PhysicsFilteredPairsAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsFilteredPairsAPI
    const TfToken PhysicsFilteredPairsAPI;
    /// \brief "PhysicsFixedJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsFixedJoint
    const TfToken PhysicsFixedJoint;
    /// \brief "PhysicsJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsJoint
    const TfToken PhysicsJoint;
    /// \brief "PhysicsLimitAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsLimitAPI
    const TfToken PhysicsLimitAPI;
    /// \brief "PhysicsMassAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsMassAPI
    const TfToken PhysicsMassAPI;
    /// \brief "PhysicsMaterialAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsMaterialAPI
    const TfToken PhysicsMaterialAPI;
    /// \brief "PhysicsMeshCollisionAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsMeshCollisionAPI
    const TfToken PhysicsMeshCollisionAPI;
    /// \brief "PhysicsPrismaticJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsPrismaticJoint
    const TfToken PhysicsPrismaticJoint;
    /// \brief "PhysicsRevoluteJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsRevoluteJoint
    const TfToken PhysicsRevoluteJoint;
    /// \brief "PhysicsRigidBodyAPI"
    /// 
    /// Schema identifer and family for UsdPhysicsRigidBodyAPI
    const TfToken PhysicsRigidBodyAPI;
    /// \brief "PhysicsScene"
    /// 
    /// Schema identifer and family for UsdPhysicsScene
    const TfToken PhysicsScene;
    /// \brief "PhysicsSphericalJoint"
    /// 
    /// Schema identifer and family for UsdPhysicsSphericalJoint
    const TfToken PhysicsSphericalJoint;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdPhysicsTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdPhysicsTokensType
extern USDPHYSICS_API TfStaticData<UsdPhysicsTokensType> UsdPhysicsTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
