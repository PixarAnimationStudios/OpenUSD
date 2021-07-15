//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
    /// \brief "force"
    /// 
    /// Possible value for UsdPhysicsDriveAPI::GetTypeAttr(), Default value for UsdPhysicsDriveAPI::GetTypeAttr()
    const TfToken force;
    /// \brief "kilogramsPerUnit"
    /// 
    /// Stage-level metadata that encodes a scene's linear unit of measure as kilograms per encoded unit.
    const TfToken kilogramsPerUnit;
    /// \brief "limit"
    /// 
    /// Property namespace prefix for the UsdPhysicsLimitAPI schema.
    const TfToken limit;
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
    /// Possible value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr(), Default value for UsdPhysicsMeshCollisionAPI::GetApproximationAttr()
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
    /// UsdPhysicsSphericalJoint, UsdPhysicsPrismaticJoint, UsdPhysicsRevoluteJoint
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
    /// UsdPhysicsJoint, UsdPhysicsCollisionAPI
    const TfToken physicsCollisionEnabled;
    /// \brief "physics:coneAngle0Limit"
    /// 
    /// UsdPhysicsSphericalJoint
    const TfToken physicsConeAngle0Limit;
    /// \brief "physics:coneAngle1Limit"
    /// 
    /// UsdPhysicsSphericalJoint
    const TfToken physicsConeAngle1Limit;
    /// \brief "physics:damping"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsDamping;
    /// \brief "physics:density"
    /// 
    /// UsdPhysicsMaterialAPI, UsdPhysicsMassAPI
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
    /// \brief "physics:high"
    /// 
    /// UsdPhysicsLimitAPI
    const TfToken physicsHigh;
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
    /// \brief "physics:low"
    /// 
    /// UsdPhysicsLimitAPI
    const TfToken physicsLow;
    /// \brief "physics:lowerLimit"
    /// 
    /// UsdPhysicsPrismaticJoint, UsdPhysicsRevoluteJoint
    const TfToken physicsLowerLimit;
    /// \brief "physics:mass"
    /// 
    /// UsdPhysicsMassAPI
    const TfToken physicsMass;
    /// \brief "physics:maxDistance"
    /// 
    /// UsdPhysicsDistanceJoint
    const TfToken physicsMaxDistance;
    /// \brief "physics:maxForce"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsMaxForce;
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
    /// UsdPhysicsCollisionAPI, UsdPhysicsRigidBodyAPI
    const TfToken physicsSimulationOwner;
    /// \brief "physics:startsAsleep"
    /// 
    /// UsdPhysicsRigidBodyAPI
    const TfToken physicsStartsAsleep;
    /// \brief "physics:staticFriction"
    /// 
    /// UsdPhysicsMaterialAPI
    const TfToken physicsStaticFriction;
    /// \brief "physics:stiffness"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsStiffness;
    /// \brief "physics:targetPosition"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsTargetPosition;
    /// \brief "physics:targetVelocity"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsTargetVelocity;
    /// \brief "physics:type"
    /// 
    /// UsdPhysicsDriveAPI
    const TfToken physicsType;
    /// \brief "physics:upperLimit"
    /// 
    /// UsdPhysicsPrismaticJoint, UsdPhysicsRevoluteJoint
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
    /// Possible value for UsdPhysicsSphericalJoint::GetAxisAttr(), Default value for UsdPhysicsSphericalJoint::GetAxisAttr(), Possible value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Default value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Possible value for UsdPhysicsRevoluteJoint::GetAxisAttr(), Default value for UsdPhysicsRevoluteJoint::GetAxisAttr()
    const TfToken x;
    /// \brief "Y"
    /// 
    /// Possible value for UsdPhysicsSphericalJoint::GetAxisAttr(), Possible value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Possible value for UsdPhysicsRevoluteJoint::GetAxisAttr()
    const TfToken y;
    /// \brief "Z"
    /// 
    /// Possible value for UsdPhysicsSphericalJoint::GetAxisAttr(), Possible value for UsdPhysicsPrismaticJoint::GetAxisAttr(), Possible value for UsdPhysicsRevoluteJoint::GetAxisAttr()
    const TfToken z;
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
