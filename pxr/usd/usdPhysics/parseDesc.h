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
#ifndef USDPHYSICS_PARSE_DESC_H
#define USDPHYSICS_PARSE_DESC_H

/// \file usdPhysics/parseDesc.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/collectionMembershipQuery.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/quatf.h"

PXR_NAMESPACE_OPEN_SCOPE

// -------------------------------------------------------------------------- //
// PHYSICSPARSEDESC                                                           //
// -------------------------------------------------------------------------- //

/// Sentinel value for flt max compare
const float physicsSentinelLimit = 0.5e38f;

/// \struct PhysicsObjectType
///
/// Physics object type structure for type enumeration
///
struct PhysicsObjectType
{
    enum Enum
    {
        eUndefined,

        eScene,

        eRigidBody,

        eSphereShape,
        eCubeShape,
        eCapsuleShape,
        eCylinderShape,
        eConeShape,
        eMeshShape,
        ePlaneShape,
        eCustomShape,
        eSpherePointsShape,

        eFixedJoint,
        eRevoluteJoint,
        ePrismaticJoint,
        eSphericalJoint,
        eDistanceJoint,
        eD6Joint,
        eCustomJoint,

        eRigidBodyMaterial,

        eArticulation,

        eCollisionGroup,

        eLast,
    };
};

/// \struct PhysicsAxis
///
/// Physics axis structure for type enumeration
///
struct PhysicsAxis
{
    enum Enum
    {
        eX,
        eY,
        eZ
    };
};

/// \struct PhysicsJointDOF
///
/// Physics joint degree of freedom structure for type enumeration
///
struct PhysicsJointDOF
{
    enum Enum
    {
        eDistance,
        eTransX,
        eTransY,
        eTransZ,
        eRotX,
        eRotY,
        eRotZ
    };
};

/// \struct PhysicsObjectDesc
///
/// Base physics object descriptor
///
struct PhysicsObjectDesc
{
    PhysicsObjectDesc() : type(PhysicsObjectType::eUndefined), isValid(true)
    {
    }

    virtual ~PhysicsObjectDesc()
    {
    }

    PhysicsObjectType::Enum type;   ///< Descriptor type
    SdfPath primPath;                ///< SdfPath for the prim from which the descriptor was parsed
    bool isValid;                   ///< Validity of a descriptor, the parsing may succeed, but the descriptor might be not valid
};

/// \struct RigidBodyMaterialDesc
///
/// Rigid body material descriptor
///
struct RigidBodyMaterialDesc : PhysicsObjectDesc
{
    RigidBodyMaterialDesc() : staticFriction(0.0f), dynamicFriction(0.0f), restitution(0.0f), density(-1.0f)
    {
        type = PhysicsObjectType::eRigidBodyMaterial;
    }

    bool operator == (const RigidBodyMaterialDesc&) const
    {
        return false;
    }


    float staticFriction;       ///< Static friction
    float dynamicFriction;      ///< Dynamic friction
    float restitution;          ///< Restitution
    float density;              ///< Density
};

/// \struct SceneDesc
///
/// Scene descriptor
///
struct SceneDesc : PhysicsObjectDesc
{
    SceneDesc() : gravityDirection(0.0f, 0.0f, 0.0f), gravityMagnitude(-INFINITY)
    {
        type = PhysicsObjectType::eScene;
    }

    bool operator == (const SceneDesc&) const
    {
        return false;
    }

    GfVec3f gravityDirection;   ///< Gravity direction, if default 0,0,0 was used negative upAxis direction will be returned
    float gravityMagnitude;     ///< Gravity magnitude, -inf means Earth gravity adjusted by metersPerUnit will be returned
};

/// \struct CollisionGroupDesc
///
/// Collision group descriptor
///
struct CollisionGroupDesc : PhysicsObjectDesc
{
    CollisionGroupDesc()
    {
        type = PhysicsObjectType::eCollisionGroup;
    }

    bool operator == (const CollisionGroupDesc&) const
    {
        return false;
    }

    const SdfPathVector& getFilteredGroups() const
    {
        return filteredGroups;
    }

    const SdfPathVector& getMergedGroups() const
    {
        return mergedGroups;
    }

    bool invertFilteredGroups;                      ///< If filtering is inverted or not (default does not collide with)
    SdfPathVector filteredGroups;                   ///< Filtered groups SdfPath vector
    std::string mergeGroupName;                     ///< Merge group name
    SdfPathVector mergedGroups;                     ///< List of merged collision groups
};

/// \struct ShapeDesc
///
/// Shape descriptor, base class should not be reported
///
/// Note as scale is not supported in most physics engines,
/// the collision shape sizes already contain the scale.
/// The exception are mesh collisions which do have geometry scale reported.
///
struct ShapeDesc : PhysicsObjectDesc
{
    ShapeDesc()
        : localPos(0.0f, 0.0f, 0.0f),
          localRot(1.0f, 0.0f, 0.0f, 0.0f),
          localScale(1.0f, 1.0f, 1.0f),
          collisionEnabled(true)
    {
    }

    const SdfPathVector& getMaterials() const
    {
        return materials;
    }

    const SdfPathVector& getSimulationOwners() const
    {
        return simulationOwners;
    }

    const SdfPathVector& getFilteredCollisions() const
    {
        return filteredCollisions;
    }

    const SdfPathVector& getCollisionGroups() const
    {
        return collisionGroups;
    }


    SdfPath rigidBody;                      ///< Rigid body the collision shape belongs to, if not set its a static collider
    GfVec3f localPos;                       ///< Local position of the shape relative to the body world pose
    GfQuatf localRot;                       ///< Local orientation of the shape relative to the body world pose
    GfVec3f localScale;                     ///< Local scale of the shape relative to the body world pose
    SdfPathVector materials;                ///< Materials assigned to the collision geometry, can be multiple materials used on UsdGeomSubset
    SdfPathVector simulationOwners;         ///< Simulation owners list
    SdfPathVector filteredCollisions;       ///< Filtered collisions list
    SdfPathVector collisionGroups;          ///< List of collision groups this collision belongs to, note that only collision groups that are part of the current range are checked.
    bool collisionEnabled;                  ///< Collision enabled/disabled bool
};

/// \struct SphereShapeDesc
///
/// Sphere shape collision descriptor
///
struct SphereShapeDesc : ShapeDesc
{
    SphereShapeDesc(float inRadius = 0.0f) : radius(inRadius)
    {
        type = PhysicsObjectType::eSphereShape;
    }

    bool operator == (const SphereShapeDesc&) const
    {
        return false;
    }

    float radius;               ///< Sphere radius
};

/// \struct CapsuleShapeDesc
///
/// Capsule shape collision descriptor
///
struct CapsuleShapeDesc : ShapeDesc
{
    CapsuleShapeDesc(float inRadius = 0.0f, float half_height = 0.0f, PhysicsAxis::Enum cap_axis = PhysicsAxis::eX)
        : radius(inRadius), halfHeight(half_height), axis(cap_axis)
    {
        type = PhysicsObjectType::eCapsuleShape;
    }

    bool operator == (const CapsuleShapeDesc&) const
    {
        return false;
    }

    float radius;               ///< Capsule radius
    float halfHeight;           ///< Capsule half height
    PhysicsAxis::Enum axis;     ///< Capsule axis
};

/// \struct CylinderShapeDesc
///
/// Cylinder shape collision descriptor
///
struct CylinderShapeDesc : ShapeDesc
{
    CylinderShapeDesc(float inRadius = 0.0f, float half_height = 0.0f, PhysicsAxis::Enum cap_axis = PhysicsAxis::eX)
        : radius(inRadius), halfHeight(half_height), axis(cap_axis)
    {
        type = PhysicsObjectType::eCylinderShape;
    }

    bool operator == (const CylinderShapeDesc&) const
    {
        return false;
    }

    float radius;               ///< Cylinder radius
    float halfHeight;           ///< Cylinder half height
    PhysicsAxis::Enum axis;     ///< Cylinder axis
};

/// \struct ConeShapeDesc
///
/// Cone shape collision descriptor
///
struct ConeShapeDesc : ShapeDesc
{
    ConeShapeDesc(float inRadius = 0.0f, float half_height = 0.0f, PhysicsAxis::Enum cap_axis = PhysicsAxis::eX)
        : radius(inRadius), halfHeight(half_height), axis(cap_axis)
    {
        type = PhysicsObjectType::eConeShape;
    }

    bool operator == (const ConeShapeDesc&) const
    {
        return false;
    }


    float radius;               ///< Cone radius
    float halfHeight;           ///< Cone half height
    PhysicsAxis::Enum axis;     ///< Cone axis
};

/// \struct PlaneShapeDesc
///
/// Plane shape collision descriptor
///
struct PlaneShapeDesc : ShapeDesc
{
    PlaneShapeDesc(PhysicsAxis::Enum up_axis = PhysicsAxis::eX)
        : axis(up_axis)
    {
        type = PhysicsObjectType::ePlaneShape;
    }
    bool operator == (const PlaneShapeDesc&) const
    {
        return false;
    }


    PhysicsAxis::Enum axis;     ///< Plane axis
};


/// \struct CustomShapeDesc
///
/// Custom shape collision descriptor
///
struct CustomShapeDesc : ShapeDesc
{
    CustomShapeDesc()
    {
        type = PhysicsObjectType::eCustomShape;
    }
    bool operator == (const CustomShapeDesc&) const
    {
        return false;
    }


    TfToken customGeometryToken;    ///< Custom geometry token for this collision
};

/// \struct CubeShapeDesc
///
/// Cube shape collision descriptor
///
struct CubeShapeDesc : ShapeDesc
{
    CubeShapeDesc(const GfVec3f& inHalfExtents = GfVec3f(1.0f)) : halfExtents(inHalfExtents)
    {
        type = PhysicsObjectType::eCubeShape;
    }
    bool operator == (const CubeShapeDesc&) const
    {
        return false;
    }

    GfVec3f halfExtents;        ///< Half extents of the cube
};

/// \struct MeshShapeDesc
///
/// Mesh shape collision descriptor
///
struct MeshShapeDesc : ShapeDesc
{
    MeshShapeDesc() : meshScale(1.0f, 1.0f, 1.0f), doubleSided(false)
    {
        type = PhysicsObjectType::eMeshShape;
    }
    bool operator == (const MeshShapeDesc&) const
    {
        return false;
    }


    ~MeshShapeDesc()
    {
    }

    const TfToken GetApproximation()
    {
        return approximation;
    }

    TfToken approximation;      ///< Desired approximation for the mesh collision
    GfVec3f meshScale;          ///< Mesh scale
    bool doubleSided;           ///< Bool to define whether mesh is double sided or not
};

/// \struct SpherePoint
///
/// This struct represents a single sphere-point
/// which is a position and a radius
///
struct SpherePoint
{
    bool operator == (const SpherePoint&) const
    {
        return false;
    }

    GfVec3f center;
    float radius;
};

/// \struct SpherePointsShapeDesc
///
/// This struct represents a collection of
/// sphere points. Basically just an array of
/// spheres which has been populated from a
/// UsdGeomPoints primitive
///
struct SpherePointsShapeDesc : ShapeDesc
{
    SpherePointsShapeDesc(void)
    {
        type = PhysicsObjectType::eSpherePointsShape;
    }
    bool operator == (const SpherePointsShapeDesc&) const
    {
        return false;
    }


    ~SpherePointsShapeDesc(void)
    {
    }

    std::vector<SpherePoint> spherePoints;  ///< Lit of sphere points
};

/// \struct RigidBodyDesc
///
/// Rigid body descriptor
///
struct RigidBodyDesc : PhysicsObjectDesc
{
    RigidBodyDesc()
        : position(0.0f, 0.0f, 0.0f),
          rotation(1.0f, 0.0f, 0.0f, 0.0f),
          scale(1.0f, 1.0f, 1.0f),
          rigidBodyEnabled(true),
          kinematicBody(false),
          startsAsleep(false),
          linearVelocity(0.0f, 0.0f, 0.0f),
          angularVelocity(0.0f, 0.0f, 0.0f)
    {
        type = PhysicsObjectType::eRigidBody;
    }
    bool operator == (const RigidBodyDesc&) const
    {
        return false;
    }

    const SdfPathVector& getCollisions() const
    {
        return collisions;
    }

    const SdfPathVector& getFilteredCollisions() const
    {
        return filteredCollisions;
    }

    const SdfPathVector& getSimulationOwners() const
    {
        return simulationOwners;
    }

    SdfPathVector collisions;               ///< List of collision shapes that belong to this rigid body
    SdfPathVector filteredCollisions;       ///< Filtered collisions
    SdfPathVector simulationOwners;         ///< Simulation owners list
    GfVec3f position;                       ///< Rigid body position in world space
    GfQuatf rotation;                       ///< Rigid body orientation in world space
    GfVec3f scale;                          ///< Rigid body scale

    bool rigidBodyEnabled;                  ///< Defines whether body is enabled or not, if not enabled its a static body
    bool kinematicBody;                     ///< Defines if the body is kinematic or not
    bool startsAsleep;                      ///< Defines if body starts asleep or awake
    GfVec3f linearVelocity;                 ///< Rigid body initial linear velocity
    GfVec3f angularVelocity;                ///< Rigid body initial angular velocity
};

/// \struct JointLimit
///
/// Joint limit descriptor
///
struct JointLimit
{
    JointLimit() : enabled(false), angle0(90.0), angle1(-90.0)
    {
    }

    bool operator == (const JointLimit&) const
    {
        return false;
    }


    bool enabled;           ///< Defines whether limit is enabled or not
    union                   ///< Min, lower, initial angle
    {
        float angle0;
        float lower;
        float minDist;
    };
    union                   ///< Max, upper, final angle
    {
        float angle1;
        float upper;
        float maxDist;
    };
};

/// \struct JointDrive
///
/// Joint drive descriptor
/// The expected drive formula:
/// force = spring * (target position - position) + damping * (targetVelocity - velocity)
///
struct JointDrive
{
    JointDrive()
        : enabled(false),
          targetPosition(0.0f),
          targetVelocity(0.0f),
          forceLimit(FLT_MAX),
          stiffness(0.0f),
          damping(0.0f),
          acceleration(false)
    {
    }

    bool operator == (const JointDrive&) const
    {
        return false;
    }

    bool enabled;           ///< Defines whether limit is enabled or not
    float targetPosition;   ///< Drive target position
    float targetVelocity;   ///< Drive target velocity
    float forceLimit;       ///< Force limit
    float stiffness;        ///< Drive stiffness
    float damping;          ///< Drive damping
    bool acceleration;      ///< Drive mode is acceleration or force
};


/// \struct ArticulationDesc
///
/// Articulation description
///
struct ArticulationDesc : PhysicsObjectDesc
{
    ArticulationDesc()
    {
        type = PhysicsObjectType::eArticulation;
    }
    bool operator == (const ArticulationDesc&) const
    {
        return false;
    }

    const SdfPathVector& GetRootPrims() const
    {
        return rootPrims;
    }

    const SdfPathVector& GetFilteredCollisions() const
    {
        return filteredCollisions;
    }

    const SdfPathVector& GetArticulatedJoints() const
    {
        return articulatedJoints;
    }

    const SdfPathVector& GetArticulatedBodies() const
    {
        return articulatedBodies;
    }

    SdfPathVector rootPrims;            ///< List of articulation roots, this defines where the articulation topology starts
    SdfPathVector filteredCollisions;   ///< Filtered collisions
    SdfPathVector articulatedJoints;    ///< List of joints that can be part of this articulation
    SdfPathVector articulatedBodies;    ///< List of bodies that can be part of this articulation
};

using JointLimits = std::vector<std::pair<PhysicsJointDOF::Enum, JointLimit>>;
using JointDrives = std::vector<std::pair<PhysicsJointDOF::Enum, JointDrive>>;

/// \struct JointDesc
///
/// Base UsdPhysics joint descriptor
///
struct JointDesc : public PhysicsObjectDesc
{
    JointDesc()
        : localPose0Position(0.0f, 0.0f, 0.0f),
          localPose0Orientation(1.0f, 0.0f, 0.0f, 0.0f),
          localPose1Position(0.0f, 0.0f, 0.0f),
          localPose1Orientation(1.0f, 0.0f, 0.0f, 0.0f),
          jointEnabled(true),
          breakForce(FLT_MAX), // USD default is none, which is not a float...
          breakTorque(FLT_MAX),
          excludeFromArticulation(false)
    {
    }

    bool operator == (const JointDesc&) const
    {
        return false;
    }

    SdfPath rel0;                   ///< UsdPrim relationship 0 for the joint
    SdfPath rel1;                   ///< UsdPrim relationship 1 for the joint
    SdfPath body0;                  ///< Rigid body 0 that the joint is connected, does not have to match the rel0
    SdfPath body1;                  ///< Rigid body 1 that the joint is connected, does not have to match the rel1
    GfVec3f localPose0Position;     ///< Relative local position against the body0 world frame
    GfQuatf localPose0Orientation;  ///< Relative local orientation against the body0 world frame
    GfVec3f localPose1Position;     ///< Relative local position against the body1 world frame
    GfQuatf localPose1Orientation;  ///< Relative local orientation against the body1 world frame
    bool jointEnabled;              ///< Defines if joint is enabled or disabled
    float breakForce;               ///< Joint break force
    float breakTorque;              ///< Joint break torque
    bool excludeFromArticulation;   ///< Defines if joints belongs to an articulation or if its a maximum coordinate joint
    bool collisionEnabled;          ///< Defines if collision is enabled or disabled between the jointed bodies
};

/// \struct CustomJointDesc
///
/// Custom joint descriptor
///
struct CustomJointDesc : public JointDesc
{
    CustomJointDesc()
    {
        type = PhysicsObjectType::eCustomJoint;
    }
    bool operator == (const CustomJointDesc&) const
    {
        return false;
    }

};

/// \struct FixedJointDesc
///
/// Fixed joint descriptor
///
struct FixedJointDesc : public JointDesc
{
    FixedJointDesc()
    {
        type = PhysicsObjectType::eFixedJoint;
    }
    bool operator == (const FixedJointDesc&) const
    {
        return false;
    }
};

/// \struct D6JointDesc
///
/// Generic D6 joint descriptor
///
struct D6JointDesc : public JointDesc
{
    D6JointDesc()
    {
        type = PhysicsObjectType::eD6Joint;
    }
    bool operator == (const D6JointDesc&) const
    {
        return false;
    }

    JointLimits jointLimits;    ///< List of joint limits
    JointDrives jointDrives;    ///< List of joint drives
};

/// \struct PrismaticJointDesc
///
/// Prismatic joint descriptor
///
struct PrismaticJointDesc : public JointDesc
{
    PrismaticJointDesc() : axis(PhysicsAxis::eX)
    {
        type = PhysicsObjectType::ePrismaticJoint;
    }
    bool operator == (const PrismaticJointDesc&) const
    {
        return false;
    }


    PhysicsAxis::Enum axis; ///< The joints axis
    JointLimit limit;       ///< Joint linear limit
    JointDrive drive;       ///< Joint linear drive
};

/// \struct SphericalJointDesc
///
/// Spherical joint descriptor
///
struct SphericalJointDesc : public JointDesc
{
    SphericalJointDesc() : axis(PhysicsAxis::eX)
    {
        type = PhysicsObjectType::eSphericalJoint;
    }
    bool operator == (const SphericalJointDesc&) const
    {
        return false;
    }

    PhysicsAxis::Enum axis; ///< The joints axis
    JointLimit limit;       ///< The joint spherical limit
};

/// \struct RevoluteJointDesc
///
/// Revolute joint descriptor
///
struct RevoluteJointDesc : public JointDesc
{
    RevoluteJointDesc() : axis(PhysicsAxis::eX)
    {
        type = PhysicsObjectType::eRevoluteJoint;
    }
    bool operator == (const RevoluteJointDesc&) const
    {
        return false;
    }

    PhysicsAxis::Enum axis; ///< The joints axis
    JointLimit limit;       ///< The angular limit
    JointDrive drive;       ///< The angular drive
};

/// \struct DistanceJointDesc
///
/// Distance joint descriptor
///
struct DistanceJointDesc : public JointDesc
{
    DistanceJointDesc() : minEnabled(false), maxEnabled(false)
    {
        type = PhysicsObjectType::eDistanceJoint;
    }
    bool operator == (const DistanceJointDesc&) const
    {
        return false;
    }
    bool minEnabled;    ///< Defines if minimum limit is enabled
    bool maxEnabled;    ///< Defines if maximum limit is enabled
    JointLimit limit;   ///< The distance limit
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
