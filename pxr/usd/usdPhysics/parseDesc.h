//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
const float usdPhysicsSentinelLimit = 0.5e38f;

/// \class UsdPhysicsObjectType
///
/// Physics object type structure for type enumeration
///
enum class UsdPhysicsObjectType
{
    Undefined,

    Scene,

    RigidBody,

    SphereShape,
    CubeShape,
    CapsuleShape,
    Capsule1Shape,
    CylinderShape,
    Cylinder1Shape,
    ConeShape,
    MeshShape,
    PlaneShape,
    CustomShape,
    SpherePointsShape,

    FixedJoint,
    RevoluteJoint,
    PrismaticJoint,
    SphericalJoint,
    DistanceJoint,
    D6Joint,
    CustomJoint,

    RigidBodyMaterial,

    Articulation,

    CollisionGroup,

    Last,
};

/// \class UsdPhysicsAxis
///
/// Physics axis structure for type enumeration
///
enum class UsdPhysicsAxis
{
    X,
    Y,
    Z
};

/// \class UsdPhysicsJointDOF
///
/// Physics joint degree of freedom structure for type enumeration
///
enum class UsdPhysicsJointDOF
{
    Distance,
    TransX,
    TransY,
    TransZ,
    RotX,
    RotY,
    RotZ
};

/// \struct UsdPhysicsObjectDesc
///
/// Base physics object descriptor
///
struct UsdPhysicsObjectDesc
{
    UsdPhysicsObjectDesc(UsdPhysicsObjectType inType) : 
        type(inType), isValid(true)
    {
    }

    virtual ~UsdPhysicsObjectDesc() = default;

    /// Descriptor type
    UsdPhysicsObjectType type;
    /// SdfPath for the prim from which the descriptor was parsed
    SdfPath primPath;
    /// Validity of a descriptor, the parsing may succeed, but the descriptor 
    /// might be not valid
    bool isValid;
};

/// \struct UsdPhysicsRigidBodyMaterialDesc
///
/// Rigid body material descriptor
///
struct UsdPhysicsRigidBodyMaterialDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsRigidBodyMaterialDesc() : 
        UsdPhysicsObjectDesc(UsdPhysicsObjectType::RigidBodyMaterial), 
        staticFriction(0.0f), dynamicFriction(0.0f), restitution(0.0f), 
        density(-1.0f)
    {
    }

    bool operator == (const UsdPhysicsRigidBodyMaterialDesc& /*desc*/) const
    {
        return false;
    }

    /// Static friction
    float staticFriction;
    /// Dynamic friction
    float dynamicFriction;
    /// Restitution
    float restitution;
    /// Density
    float density;
};

/// \struct UsdPhysicsSceneDesc
///
/// Scene descriptor
///
struct UsdPhysicsSceneDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsSceneDesc() : 
        UsdPhysicsObjectDesc(UsdPhysicsObjectType::Scene), 
        gravityDirection(0.0f, 0.0f, 0.0f), gravityMagnitude(-INFINITY)
    {        
    }

    bool operator == (const UsdPhysicsSceneDesc& /*desc*/) const
    {
        return false;
    }

    /// Gravity direction, if default 0,0,0 was used negative upAxis direction
    /// will be returned
    GfVec3f gravityDirection;
    /// Gravity magnitude, -inf means Earth gravity adjusted by metersPerUnit
    /// will be returned
    float gravityMagnitude;
};

/// \struct UsdPhysicsCollisionGroupDesc
///
/// Collision group descriptor
///
struct UsdPhysicsCollisionGroupDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsCollisionGroupDesc():
        UsdPhysicsObjectDesc(UsdPhysicsObjectType::CollisionGroup)
    {        
    }

    bool operator == (const UsdPhysicsCollisionGroupDesc& /*desc*/) const
    {
        return false;
    }

    const SdfPathVector& GetFilteredGroups() const
    {
        return filteredGroups;
    }

    const SdfPathVector& GetMergedGroups() const
    {
        return mergedGroups;
    }

    /// If filtering is inverted or not (default does not collide with)
    bool invertFilteredGroups;
    /// Filtered groups SdfPath vector
    SdfPathVector filteredGroups;
    /// Merge group name
    std::string mergeGroupName;
    /// List of merged collision groups
    SdfPathVector mergedGroups;
};

/// \struct UsdPhysicsShapeDesc
///
/// Shape descriptor, base class should not be reported
///
/// Note as scale is not supported in most physics engines,
/// the collision shape sizes already contain the scale.
/// The exception are mesh collisions which do have geometry scale reported.
///
struct UsdPhysicsShapeDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsShapeDesc(UsdPhysicsObjectType inType)
        : UsdPhysicsObjectDesc(inType), localPos(0.0f, 0.0f, 0.0f), 
        localRot(1.0f, 0.0f, 0.0f, 0.0f), 
        localScale(1.0f, 1.0f, 1.0f), collisionEnabled(true)
    {
    }

    const SdfPathVector& GetMaterials() const
    {
        return materials;
    }

    const SdfPathVector& GetSimulationOwners() const
    {
        return simulationOwners;
    }

    const SdfPathVector& GetFilteredCollisions() const
    {
        return filteredCollisions;
    }

    const SdfPathVector& GetCollisionGroups() const
    {
        return collisionGroups;
    }


    /// Rigid body the collision shape belongs to, if not set it's a static 
    /// collider
    SdfPath rigidBody;
    /// Local position of the shape relative to the body world pose
    GfVec3f localPos;
    /// Local orientation of the shape relative to the body world pose
    GfQuatf localRot;
    /// Local scale of the shape relative to the body world pose
    GfVec3f localScale;
    /// Materials assigned to the collision geometry, can be multiple materials
    /// used on UsdGeomSubset
    SdfPathVector materials;
    /// Simulation owners list
    SdfPathVector simulationOwners;
    /// Filtered collisions list
    SdfPathVector filteredCollisions;
    /// List of collision groups this collision belongs to, note that only
    /// collision groups that are part of the current range are checked.
    SdfPathVector collisionGroups;
    /// Collision enabled/disabled bool
    bool collisionEnabled;
};

/// \struct UsdPhysicsSphereShapeDesc
///
/// Sphere shape collision descriptor
///
struct UsdPhysicsSphereShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsSphereShapeDesc(float inRadius = 0.0f) : 
        UsdPhysicsShapeDesc(UsdPhysicsObjectType::SphereShape), radius(inRadius)
    {        
    }

    bool operator == (const UsdPhysicsSphereShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Sphere radius
    float radius;
};

/// \struct UsdPhysicsCapsuleShapeDesc
///
/// Capsule shape collision descriptor
///
struct UsdPhysicsCapsuleShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCapsuleShapeDesc(float inRadius = 0.0f, float half_height = 0.0f,
        UsdPhysicsAxis cap_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::CapsuleShape), 
        radius(inRadius), halfHeight(half_height), axis(cap_axis)
    {        
    }

    bool operator == (const UsdPhysicsCapsuleShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Capsule radius
    float radius;
    /// Capsule half height
    float halfHeight;
    /// Capsule axis
    UsdPhysicsAxis axis;
};

/// \struct UsdPhysicsCapsule1ShapeDesc
///
/// Capsule1 shape collision descriptor
///
struct UsdPhysicsCapsule1ShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCapsule1ShapeDesc(float inTopRadius = 0.0f,
        float inBottomRadius = 0.0f, float half_height = 0.0f,
        UsdPhysicsAxis cap_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::Capsule1Shape), 
        topRadius(inTopRadius), bottomRadius(inBottomRadius),
        halfHeight(half_height), axis(cap_axis)
    {        
    }

    bool operator == (const UsdPhysicsCapsule1ShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Capsule top radius
    float topRadius;
    /// Capsule bottom radius
    float bottomRadius;
    /// Capsule half height
    float halfHeight;
    /// Capsule axis
    UsdPhysicsAxis axis;
};

/// \struct UsdPhysicsCylinderShapeDesc
///
/// Cylinder shape collision descriptor
///
struct UsdPhysicsCylinderShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCylinderShapeDesc(float inRadius = 0.0f, float half_height = 0.0f,
        UsdPhysicsAxis cap_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::CylinderShape), 
        radius(inRadius), halfHeight(half_height), axis(cap_axis)
    {        
    }

    bool operator == (const UsdPhysicsCylinderShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Cylinder radius
    float radius;
    /// Cylinder half height
    float halfHeight;
    /// Cylinder axis
    UsdPhysicsAxis axis;
};

/// \struct UsdPhysicsCylinder1ShapeDesc
///
/// Cylinder1 shape collision descriptor
///
struct UsdPhysicsCylinder1ShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCylinder1ShapeDesc(float inTopRadius = 0.0f, 
        float inBottomRadius = 0.0f, float half_height = 0.0f,
        UsdPhysicsAxis cap_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::Cylinder1Shape), 
        topRadius(inTopRadius), bottomRadius(inBottomRadius),
        halfHeight(half_height), axis(cap_axis)
    {        
    }

    bool operator == (const UsdPhysicsCylinder1ShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Cylinder top radius
    float topRadius;
    /// Cylinder bottom radius
    float bottomRadius;
    /// Cylinder half height
    float halfHeight;
    /// Cylinder axis
    UsdPhysicsAxis axis;
};

/// \struct UsdPhysicsConeShapeDesc
///
/// Cone shape collision descriptor
///
struct UsdPhysicsConeShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsConeShapeDesc(float inRadius = 0.0f, float half_height = 0.0f,
        UsdPhysicsAxis cap_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::ConeShape), radius(inRadius), 
        halfHeight(half_height), axis(cap_axis)
    {        
    }

    bool operator == (const UsdPhysicsConeShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Cone radius
    float radius;
    /// Cone half height
    float halfHeight;
    /// Cone axis
    UsdPhysicsAxis axis;
};

/// \struct UsdPhysicsPlaneShapeDesc
///
/// Plane shape collision descriptor
///
struct UsdPhysicsPlaneShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsPlaneShapeDesc(UsdPhysicsAxis up_axis = UsdPhysicsAxis::X)
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::PlaneShape), axis(up_axis)
    {        
    }

    bool operator == (const UsdPhysicsPlaneShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Plane axis
    UsdPhysicsAxis axis;
};


/// \struct UsdPhysicsCustomShapeDesc
///
/// Custom shape collision descriptor
///
struct UsdPhysicsCustomShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCustomShapeDesc()
    : UsdPhysicsShapeDesc(UsdPhysicsObjectType::CustomShape)
    {        
    }

    bool operator == (const UsdPhysicsCustomShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Custom geometry token for this collision
    TfToken customGeometryToken;
};

/// \struct UsdPhysicsCubeShapeDesc
///
/// Cube shape collision descriptor
///
struct UsdPhysicsCubeShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsCubeShapeDesc(const GfVec3f& inHalfExtents = GfVec3f(1.0f))
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::CubeShape), 
        halfExtents(inHalfExtents)
    {
    }

    bool operator == (const UsdPhysicsCubeShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// Half extents of the cube
    GfVec3f halfExtents;
};

/// \struct UsdPhysicsMeshShapeDesc
///
/// Mesh shape collision descriptor
///
struct UsdPhysicsMeshShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsMeshShapeDesc()
        : UsdPhysicsShapeDesc(UsdPhysicsObjectType::MeshShape), 
        meshScale(1.0f, 1.0f, 1.0f), 
        doubleSided(false)
    {
    }

    bool operator == (const UsdPhysicsMeshShapeDesc& /*desc*/) const
    {
        return false;
    }

    TfToken GetApproximation() const
    {
        return approximation;
    }

    /// Desired approximation for the mesh collision
    TfToken approximation;
    /// Mesh scale
    GfVec3f meshScale;
    /// Defines whether mesh is double sided or not
    bool doubleSided;
};

/// \struct UsdPhysicsSpherePoint
///
/// This struct represents a single sphere-point
/// which is a position and a radius
///
struct UsdPhysicsSpherePoint
{
    bool operator == (const UsdPhysicsSpherePoint& /*desc*/) const
    {
        return false;
    }

    /// Sphere point center
    GfVec3f center; 

    /// Sphere point radius
    float radius;
};

/// \struct UsdPhysicsSpherePointsShapeDesc
///
/// This struct represents a collection of
/// sphere points. Basically just an array of
/// spheres which has been populated from a
/// UsdGeomPoints primitive
///
struct UsdPhysicsSpherePointsShapeDesc : UsdPhysicsShapeDesc
{
    UsdPhysicsSpherePointsShapeDesc()
    : UsdPhysicsShapeDesc(UsdPhysicsObjectType::SpherePointsShape)
    {
    }

    bool operator == (const UsdPhysicsSpherePointsShapeDesc& /*desc*/) const
    {
        return false;
    }

    /// List of sphere points
    std::vector<UsdPhysicsSpherePoint> spherePoints;
};

/// \struct UsdPhysicsRigidBodyDesc
///
/// Rigid body descriptor
///
struct UsdPhysicsRigidBodyDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsRigidBodyDesc()
        : UsdPhysicsObjectDesc(UsdPhysicsObjectType::RigidBody), 
        position(0.0f, 0.0f, 0.0f), 
        rotation(1.0f, 0.0f, 0.0f, 0.0f),
        scale(1.0f, 1.0f, 1.0f), rigidBodyEnabled(true), kinematicBody(false),
        startsAsleep(false), linearVelocity(0.0f, 0.0f, 0.0f), 
        angularVelocity(0.0f, 0.0f, 0.0f)
    {
    }

    bool operator == (const UsdPhysicsRigidBodyDesc& /*desc*/) const
    {
        return false;
    }

    const SdfPathVector& GetCollisions() const
    {
        return collisions;
    }

    const SdfPathVector& GetFilteredCollisions() const
    {
        return filteredCollisions;
    }

    const SdfPathVector& GetSimulationOwners() const
    {
        return simulationOwners;
    }

    /// List of collision shapes that belong to this rigid body
    SdfPathVector collisions;
    /// Filtered collisions
    SdfPathVector filteredCollisions;
    /// Simulation owners list
    SdfPathVector simulationOwners;
    /// Rigid body position in world space
    GfVec3f position;
    /// Rigid body orientation in world space
    GfQuatf rotation;
    /// Rigid body scale
    GfVec3f scale;

    /// Defines whether body is enabled or not, if not enabled it's a static body
    bool rigidBodyEnabled;
    /// Defines if the body is kinematic or not
    bool kinematicBody;
    /// Defines if body starts asleep or awake
    bool startsAsleep;
    /// Rigid body initial linear velocity
    GfVec3f linearVelocity;
    /// Rigid body initial angular velocity
    GfVec3f angularVelocity;
};

/// \struct UsdPhysicsJointLimit
///
/// Joint limit descriptor
///
struct UsdPhysicsJointLimit
{
    UsdPhysicsJointLimit() : enabled(false), angle0(90.0), angle1(-90.0)
    {
    }

    bool operator == (const UsdPhysicsJointLimit& /*desc*/) const
    {
        return false;
    }

    /// Defines whether limit is enabled or not
    bool enabled;

    /// Min, lower, initial angle
    union
    {
        float angle0;
        float lower;
        float minDist;
    };

    /// Max, upper, final angle
    union
    {
        float angle1;
        float upper;
        float maxDist;
    };
};

/// \struct UsdPhysicsJointDrive
///
/// Joint drive descriptor
/// The expected drive formula:
/// force = 
/// spring * (target position - position) + damping * (targetVelocity - velocity)
///
struct UsdPhysicsJointDrive
{
    UsdPhysicsJointDrive()
        : enabled(false), targetPosition(0.0f), targetVelocity(0.0f), 
        forceLimit(FLT_MAX), stiffness(0.0f), damping(0.0f), acceleration(false)
    {
    }

    bool operator == (const UsdPhysicsJointDrive& /*desc*/) const
    {
        return false;
    }

    /// Defines whether drive is enabled or not
    bool enabled;
    /// Drive target position
    float targetPosition;
    /// Drive target velocity
    float targetVelocity;
    /// force limit
    float forceLimit;
    /// Drive stiffness
    float stiffness;
    /// Drive damping
    float damping;
    /// Drive mode is acceleration or force
    bool acceleration;
};


/// \struct UsdPhysicsArticulationDesc
///
/// Articulation description
///
struct UsdPhysicsArticulationDesc : UsdPhysicsObjectDesc
{
    UsdPhysicsArticulationDesc()
    : UsdPhysicsObjectDesc(UsdPhysicsObjectType::Articulation)
    {
    }

    bool operator == (const UsdPhysicsArticulationDesc& /*desc*/) const
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

    /// List of articulation roots, this defines where the articulation 
    /// topology starts
    SdfPathVector rootPrims;
    /// Filtered collisions
    SdfPathVector filteredCollisions;
    /// List of joints that can be part of this articulation
    SdfPathVector articulatedJoints;
    /// List of bodies that can be part of this articulation
    SdfPathVector articulatedBodies;
};

using JointLimits = std::vector<
                        std::pair<
                            UsdPhysicsJointDOF, UsdPhysicsJointLimit>>;
using JointDrives = std::vector<
                        std::pair<
                            UsdPhysicsJointDOF, UsdPhysicsJointDrive>>;

/// \struct UsdPhysicsJointDesc
///
/// Base UsdPhysics joint descriptor
///
struct UsdPhysicsJointDesc : public UsdPhysicsObjectDesc
{
    UsdPhysicsJointDesc(UsdPhysicsObjectType inType)
        : UsdPhysicsObjectDesc(inType), localPose0Position(0.0f, 0.0f, 0.0f), 
        localPose0Orientation(1.0f, 0.0f, 0.0f, 0.0f), 
        localPose1Position(0.0f, 0.0f, 0.0f), 
        localPose1Orientation(1.0f, 0.0f, 0.0f, 0.0f), jointEnabled(true),
        breakForce(FLT_MAX), // USD default is none, which is not a float...
        breakTorque(FLT_MAX), excludeFromArticulation(false)
    {
    }

    bool operator == (const UsdPhysicsJointDesc& /*desc*/) const
    {
        return false;
    }

    /// UsdPrim relationship 0 for the joint
    SdfPath rel0;
    /// UsdPrim relationship 1 for the joint
    SdfPath rel1;
    /// Rigid body 0 that the joint is connected, does not have to match the 
    /// rel0
    SdfPath body0;
    /// Rigid body 1 that the joint is connected, does not have to match the
    /// rel1
    SdfPath body1;
    /// Relative local position against the body0 world frame
    GfVec3f localPose0Position;
    /// Relative local orientation against the body0 world frame
    GfQuatf localPose0Orientation;
    /// Relative local position against the body1 world frame
    GfVec3f localPose1Position;
    /// Relative local orientation against the body1 world frame
    GfQuatf localPose1Orientation;
    /// Defines if joint is enabled or disabled
    bool jointEnabled;
    /// Joint break force
    float breakForce;
    /// Joint break torque
    float breakTorque;
    /// Defines if joint belongs to an articulation or if it's a maximum
    /// coordinate joint
    bool excludeFromArticulation;
    /// Defines if collision is enabled or disabled between the jointed bodies
    bool collisionEnabled;
};

/// \struct UsdPhysicsCustomJointDesc
///
/// Custom joint descriptor
///
struct UsdPhysicsCustomJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsCustomJointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::CustomJoint)
    {
    }

    bool operator == (const UsdPhysicsCustomJointDesc& /*desc*/) const
    {
        return false;
    }

};

/// \struct UsdPhysicsFixedJointDesc
///
/// Fixed joint descriptor
///
struct UsdPhysicsFixedJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsFixedJointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::FixedJoint)
    {
    }

    bool operator == (const UsdPhysicsFixedJointDesc& /*desc*/) const
    {
        return false;
    }
};

/// \struct UsdPhysicsD6JointDesc
///
/// Generic D6 joint descriptor
///
struct UsdPhysicsD6JointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsD6JointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::D6Joint)
    {
    }

    bool operator == (const UsdPhysicsD6JointDesc& /*desc*/) const
    {
        return false;
    }

    /// List of joint limit's
    JointLimits jointLimits;
    /// List of joint drives
    JointDrives jointDrives;
};

/// \struct UsdPhysicsPrismaticJointDesc
///
/// Prismatic joint descriptor
///
struct UsdPhysicsPrismaticJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsPrismaticJointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::PrismaticJoint), 
    axis(UsdPhysicsAxis::X)
    {
    }

    bool operator == (const UsdPhysicsPrismaticJointDesc& /*desc*/) const
    {
        return false;
    }

    /// The joints axis
    UsdPhysicsAxis axis;
    /// Joint linear limit
    UsdPhysicsJointLimit limit;
    /// Joint linear drive
    UsdPhysicsJointDrive drive;
};

/// \struct UsdPhysicsSphericalJointDesc
///
/// Spherical joint descriptor
///
struct UsdPhysicsSphericalJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsSphericalJointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::SphericalJoint), 
    axis(UsdPhysicsAxis::X)
    {
    }

    bool operator == (const UsdPhysicsSphericalJointDesc& /*desc*/) const
    {
        return false;
    }

    /// The joints axis
    UsdPhysicsAxis axis;
    /// The join spherical limit
    UsdPhysicsJointLimit limit;
};

/// \struct UsdPhysicsRevoluteJointDesc
///
/// Revolute joint descriptor
///
struct UsdPhysicsRevoluteJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsRevoluteJointDesc()
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::RevoluteJoint), 
    axis(UsdPhysicsAxis::X)
    {
    }

    bool operator == (const UsdPhysicsRevoluteJointDesc& /*desc*/) const
    {
        return false;
    }

    /// The joints axis
    UsdPhysicsAxis axis;
    /// The angular limit
    UsdPhysicsJointLimit limit;
    /// The angular drive
    UsdPhysicsJointDrive drive;
};

/// \struct UsdPhysicsDistanceJointDesc
///
/// Distance joint descriptor
///
struct UsdPhysicsDistanceJointDesc : public UsdPhysicsJointDesc
{
    UsdPhysicsDistanceJointDesc() 
    : UsdPhysicsJointDesc(UsdPhysicsObjectType::DistanceJoint), 
    minEnabled(false), maxEnabled(false)
    {
    }

    bool operator == (const UsdPhysicsDistanceJointDesc& /*desc*/) const
    {
        return false;
    }

    /// Defines if minimum limit is enabled
    bool minEnabled;
    /// Defines if maximum limit is enabled
    bool maxEnabled;
    /// The distance limit
    UsdPhysicsJointLimit limit;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
