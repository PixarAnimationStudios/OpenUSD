//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "parseUtils.h"
#include "parseDesc.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python/suite/indexing/vector_indexing_suite.hpp"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/str.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

template <typename T>
void registerVectorConverter(const char* name)
{
    class_<std::vector<T>>(name).def(vector_indexing_suite<std::vector<T>>());
}

template <typename DescType>
void moveDescsToDict(UsdPhysicsObjectType objectType,
    TfSpan<const SdfPath> primsSource,
    TfSpan<const UsdPhysicsObjectDesc> objectDescsSource,
    pxr_boost::python::dict* retDict)
{
    const SdfPathVector primPaths(primsSource.begin(), primsSource.end());
    const TfSpan<const DescType> objDescsSpan((const DescType*)objectDescsSource.data(), objectDescsSource.size());
    const std::vector<DescType> objDescs(objDescsSpan.begin(), objDescsSpan.end());

    (*retDict)[objectType] =
        pxr_boost::python::make_tuple(
            primPaths, 
            objDescs);
}

void ReportPhysicsObjectsFn(UsdPhysicsObjectType type,
                            TfSpan<const SdfPath> primPaths,
                            TfSpan<const UsdPhysicsObjectDesc> objectDescs,
                            const VtValue& userData)
{
    pxr_boost::python::dict* retDict = userData.GetWithDefault<pxr_boost::python::dict*>(nullptr);
    if (!retDict) {
        return;
    }
    if (primPaths.size() != objectDescs.size()) {
        return;
    }

    switch (type)
    {
    case UsdPhysicsObjectType::Scene:
    {
        moveDescsToDict<UsdPhysicsSceneDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::RigidBody:
    {
        moveDescsToDict<UsdPhysicsRigidBodyDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::SphereShape:
    {
        moveDescsToDict<UsdPhysicsSphereShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CubeShape:
    {
        moveDescsToDict<UsdPhysicsCubeShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CapsuleShape:
    {
        moveDescsToDict<UsdPhysicsCapsuleShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::Capsule1Shape:
    {
        moveDescsToDict<UsdPhysicsCapsule1ShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CylinderShape:
    {
        moveDescsToDict<UsdPhysicsCylinderShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::Cylinder1Shape:
    {
        moveDescsToDict<UsdPhysicsCylinder1ShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::ConeShape:
    {
        moveDescsToDict<UsdPhysicsConeShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::MeshShape:
    {
        moveDescsToDict<UsdPhysicsMeshShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::PlaneShape:
    {
        moveDescsToDict<UsdPhysicsPlaneShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CustomShape:
    {
        moveDescsToDict<UsdPhysicsCustomShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::SpherePointsShape:
    {
        moveDescsToDict<UsdPhysicsSpherePointsShapeDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::FixedJoint:
    {
        moveDescsToDict<UsdPhysicsFixedJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::RevoluteJoint:
    {
        moveDescsToDict<UsdPhysicsRevoluteJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::PrismaticJoint:
    {
        moveDescsToDict<UsdPhysicsPrismaticJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::SphericalJoint:
    {
        moveDescsToDict<UsdPhysicsSphericalJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::DistanceJoint:
    {
        moveDescsToDict<UsdPhysicsDistanceJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::D6Joint:
    {
        moveDescsToDict<UsdPhysicsD6JointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CustomJoint:
    {
        moveDescsToDict<UsdPhysicsCustomJointDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::RigidBodyMaterial:
    {
        moveDescsToDict<UsdPhysicsRigidBodyMaterialDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::Articulation:
    {
        moveDescsToDict<UsdPhysicsArticulationDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::CollisionGroup:
    {
        moveDescsToDict<UsdPhysicsCollisionGroupDesc>(type, 
            primPaths, objectDescs, retDict);
    }
    break;
    case UsdPhysicsObjectType::Undefined:
    {
        TF_WARN("UsdPhysicsObject type unknown for python "
                              "wrapping.");
    }
    break;
    default:
        TF_VERIFY(false);
    break;
    }
}

struct _CustomUsdPhysicsTokens
{
    _CustomUsdPhysicsTokens()
    {

    }

    /// Custom joints to be reported by parsing
    list jointTokens;
    /// Custom shapes to be reported by parsing
    list shapeTokens;
    /// Custom physics instancers to be reported by parsing
    list instancerTokens;
};

std::string GetString(const object& po)
{
    if (extract<std::string>(po).check()) {
        std::string str = extract<std::string>(po);
        return str;
    }
    else
    {
        // Handle non-string items, e.g. convert to string
        std::string str = extract<std::string>(
            pxr_boost::python::str(po));
        return str;
    }
}

dict _LoadUsdPhysicsFromRange(UsdStageWeakPtr stage,
    const std::vector<SdfPath>& includePaths, 
    const std::vector<SdfPath>& excludePaths,
    const _CustomUsdPhysicsTokens& customTokens,
    const std::vector<SdfPath>& simulationOwners)
{
    CustomUsdPhysicsTokens parsingCustomTokens;
    bool customTokensValid = false;
    const size_t jointTokesSize = len(customTokens.jointTokens);
    const size_t shapeTokesSize = len(customTokens.shapeTokens);
    const size_t instancerTokesSize = len(customTokens.instancerTokens);
    if (jointTokesSize || shapeTokesSize || instancerTokesSize)
    {
        for (size_t i = 0; i < jointTokesSize; i++)
        {
            parsingCustomTokens.jointTokens.push_back(
                TfToken(GetString(customTokens.jointTokens[i])));
        }
        for (size_t i = 0; i < shapeTokesSize; i++)
        {
            parsingCustomTokens.shapeTokens.push_back(
                TfToken(GetString(customTokens.shapeTokens[i])));
        }
        for (size_t i = 0; i < instancerTokesSize; i++)
        {
            parsingCustomTokens.instancerTokens.push_back(
                TfToken(GetString(customTokens.instancerTokens[i])));
        }
        customTokensValid = true;
    }

    dict retDict;    
    if (!LoadUsdPhysicsFromRange(stage, includePaths,
        ReportPhysicsObjectsFn, VtValue(&retDict),
        !excludePaths.empty() ? &excludePaths : nullptr,
        customTokensValid ? &parsingCustomTokens : nullptr,
        !simulationOwners.empty() ? &simulationOwners : nullptr)) {

        TF_WARN("Unable to perform physics parsing on stage.");
    }
    return retDict;
}

static std::string
_CustomUsdPhysicsTokens_Repr(const CustomUsdPhysicsTokens& self)
{
    return TfStringPrintf(
        "%sCustomUsdPhysicsTokens(jointTokens=%s, shapeTokens=%s, "
        "instancerTokens=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.jointTokens).c_str(),
        TfPyRepr(self.shapeTokens).c_str(),
        TfPyRepr(self.instancerTokens).c_str());
}

static std::string
_PhysicsObjectDesc_Repr(const UsdPhysicsObjectDesc& self)
{
    return TfStringPrintf("%sPhysicsObjectDesc(type=%s, primPath=%s, "
                          "isValid=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.type).c_str(),
        TfPyRepr(self.primPath).c_str(),
        TfPyRepr(self.isValid).c_str());
}

static std::string
_SceneDesc_Repr(const UsdPhysicsSceneDesc& self)
{
    return TfStringPrintf(
        "%sSceneDesc(gravityDirection=%s, gravityMagnitude=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.gravityDirection).c_str(),
        TfPyRepr(self.gravityMagnitude).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_CollisionGroupDesc_Repr(const UsdPhysicsCollisionGroupDesc& self)
{
    return TfStringPrintf(
        "%sCollisionGroupDesc(invertFilteredGroups=%s, mergeGroupName=%s, "
        "mergedGroups=%s, filteredGroups=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.invertFilteredGroups).c_str(),
        TfPyRepr(self.mergeGroupName).c_str(),
        TfPyRepr(self.mergedGroups).c_str(),
        TfPyRepr(self.filteredGroups).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_RigidBodyMaterialDesc_Repr(const UsdPhysicsRigidBodyMaterialDesc& self)
{
    return TfStringPrintf(
        "%sRigidBodyMaterialDesc(staticFriction=%s, dynamicFriction=%s, "
        "restitution=%s, density=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.staticFriction).c_str(),
        TfPyRepr(self.dynamicFriction).c_str(),
        TfPyRepr(self.restitution).c_str(),
        TfPyRepr(self.density).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_ShapeDesc_Repr(const UsdPhysicsShapeDesc& self)
{
    return TfStringPrintf(
        "%sShapeDesc(rigidBody=%s, localPos=%s, localRot=%s, localScale=%s, "
        "materials=%s, simulationOwners=%s, filteredCollisions=%s, "
        "collisionGroups=%s, collisionEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.rigidBody).c_str(),
        TfPyRepr(self.localPos).c_str(),
        TfPyRepr(self.localRot).c_str(),
        TfPyRepr(self.localScale).c_str(),
        TfPyRepr(self.materials).c_str(),
        TfPyRepr(self.simulationOwners).c_str(),
        TfPyRepr(self.filteredCollisions).c_str(),
        TfPyRepr(self.collisionGroups).c_str(),
        TfPyRepr(self.collisionEnabled).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_SphereShapeDesc_Repr(const UsdPhysicsSphereShapeDesc& self)
{
    return TfStringPrintf(
        "%sSphereShapeDesc(radius=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CapsuleShapeDesc_Repr(const UsdPhysicsCapsuleShapeDesc& self)
{
    return TfStringPrintf(
        "%sCapsuleShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_Capsule1ShapeDesc_Repr(const UsdPhysicsCapsule1ShapeDesc& self)
{
    return TfStringPrintf(
        "%sCapsule1ShapeDesc(topRadius=%s, bottomRadius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.topRadius).c_str(),
        TfPyRepr(self.bottomRadius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CylinderShapeDesc_Repr(const UsdPhysicsCylinderShapeDesc& self)
{
    return TfStringPrintf(
        "%sCylinderShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_Cylinder1ShapeDesc_Repr(const UsdPhysicsCylinder1ShapeDesc& self)
{
    return TfStringPrintf(
        "%sCylinder1ShapeDesc(topRadius=%s, bottomRadius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.topRadius).c_str(),
        TfPyRepr(self.bottomRadius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_ConeShapeDesc_Repr(const UsdPhysicsConeShapeDesc& self)
{
    return TfStringPrintf(
        "%sConeShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_PlaneShapeDesc_Repr(const UsdPhysicsPlaneShapeDesc& self)
{
    return TfStringPrintf("%sPlaneShapeDesc(axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CustomShapeDesc_Repr(const UsdPhysicsCustomShapeDesc& self)
{
    return TfStringPrintf("%sCustomShapeDesc(customGeometryToken=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.customGeometryToken).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CubeShapeDesc_Repr(const UsdPhysicsCubeShapeDesc& self)
{
    return TfStringPrintf("%sCubeShapeDesc(halfExtents=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.halfExtents).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_MeshShapeDesc_Repr(const UsdPhysicsMeshShapeDesc& self)
{
    return TfStringPrintf(
        "%sMeshShapeDesc(approximation=%s, meshScale=%s, doubleSided=%s), "
        "parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.approximation).c_str(),
        TfPyRepr(self.meshScale).c_str(),
        TfPyRepr(self.doubleSided).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_SpherePoint_Repr(const UsdPhysicsSpherePoint& self)
{
    return TfStringPrintf("%sSpherePoint(center=%s, radius=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.center).c_str(),
        TfPyRepr(self.radius).c_str());
}

static std::string
_SpherePointsShapeDesc_Repr(const UsdPhysicsSpherePointsShapeDesc& self)
{
    return TfStringPrintf("%sSpherePointsShapeDesc(spherePoints=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.spherePoints).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_RigidBodyDesc_Repr(const UsdPhysicsRigidBodyDesc& self)
{
    return TfStringPrintf(
        "%sRigidBodyDesc(collisions=%s, filteredCollisions=%s, "
        "simulationOwners=%s, position=%s, rotation=%s, scale=%s, "
        "rigidBodyEnabled=%s, kinematicBody=%s, startsAsleep=%s, "
        "linearVelocity=%s, angularVelocity=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.collisions).c_str(),
        TfPyRepr(self.filteredCollisions).c_str(),
        TfPyRepr(self.simulationOwners).c_str(),
        TfPyRepr(self.position).c_str(),
        TfPyRepr(self.rotation).c_str(),
        TfPyRepr(self.scale).c_str(),
        TfPyRepr(self.rigidBodyEnabled).c_str(),
        TfPyRepr(self.kinematicBody).c_str(),
        TfPyRepr(self.startsAsleep).c_str(),
        TfPyRepr(self.linearVelocity).c_str(),
        TfPyRepr(self.angularVelocity).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_JointLimit_Repr(const UsdPhysicsJointLimit& self)
{
    return TfStringPrintf("%sJointLimit(enabled=%s, lower=%s, upper=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.enabled).c_str(),
        TfPyRepr(self.lower).c_str(),
        TfPyRepr(self.upper).c_str());
}

static std::string
_JointDrive_Repr(const UsdPhysicsJointDrive& self)
{
    return TfStringPrintf(
        "%sJointDrive(enabled=%s, targetPosition=%s, targetVelocity=%s, "
        "forceLimit=%s, stiffness=%s, damping=%s, acceleration=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.enabled).c_str(),
        TfPyRepr(self.targetPosition).c_str(),
        TfPyRepr(self.targetVelocity).c_str(),
        TfPyRepr(self.forceLimit).c_str(),
        TfPyRepr(self.stiffness).c_str(),
        TfPyRepr(self.damping).c_str(),
        TfPyRepr(self.acceleration).c_str());
}

static std::string
_ArticulationDesc_Repr(const UsdPhysicsArticulationDesc& self)
{
    return TfStringPrintf(
        "%sArticulationDesc(rootPrims=%s, filteredCollisions=%s, "
        "articulatedJoints=%s, articulatedBodies=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.rootPrims).c_str(),
        TfPyRepr(self.filteredCollisions).c_str(),
        TfPyRepr(self.articulatedJoints).c_str(),
        TfPyRepr(self.articulatedBodies).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_JointDesc_Repr(const UsdPhysicsJointDesc& self)
{
    return TfStringPrintf(
        "%sJointDesc(rel0=%s, rel1=%s, body0=%s, body1=%s, "
        "localPose0Position=%s, localPose0Orientation=%s, "
        "localPose1Position=%s, localPose1Orientation=%s, "
        "jointEnabled=%s, breakForce=%s, breakTorque=%s, "
        "excludeFromArticulation=%s, collisionEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.rel0).c_str(),
        TfPyRepr(self.rel1).c_str(),
        TfPyRepr(self.body0).c_str(),
        TfPyRepr(self.body1).c_str(),
        TfPyRepr(self.localPose0Position).c_str(),
        TfPyRepr(self.localPose0Orientation).c_str(),
        TfPyRepr(self.localPose1Position).c_str(),
        TfPyRepr(self.localPose1Orientation).c_str(),
        TfPyRepr(self.jointEnabled).c_str(),
        TfPyRepr(self.breakForce).c_str(),
        TfPyRepr(self.breakTorque).c_str(),
        TfPyRepr(self.excludeFromArticulation).c_str(),
        TfPyRepr(self.collisionEnabled).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_JointLimitDOFPair_Repr(const std::pair<UsdPhysicsJointDOF, 
                        UsdPhysicsJointLimit>& self)
{
    return TfStringPrintf("%sJointLimitDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_JointDriveDOFPair_Repr(const std::pair<UsdPhysicsJointDOF, 
                        UsdPhysicsJointDrive>& self)
{
    return TfStringPrintf("%sJointDriveDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_D6JointDesc_Repr(const UsdPhysicsD6JointDesc& self)
{
    return TfStringPrintf("%sD6JointDesc(jointLimits=%s, jointDrives=%s), "
                          "parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.jointLimits).c_str(),
        TfPyRepr(self.jointDrives).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_PrismaticJointDesc_Repr(const UsdPhysicsPrismaticJointDesc& self)
{
    return TfStringPrintf("%sPrismaticJointDesc(axis=%s, limit=%s, drive=%s), "
                          "parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.drive).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_SphericalJointDesc_Repr(const UsdPhysicsSphericalJointDesc& self)
{
    return TfStringPrintf("%sSphericalJointDesc(axis=%s, limit=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_RevoluteJointDesc_Repr(const UsdPhysicsRevoluteJointDesc& self)
{
    return TfStringPrintf("%sRevoluteJointDesc(axis=%s, limit=%s, drive=%s), "
                          "parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.drive).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_DistanceJointDesc_Repr(const UsdPhysicsDistanceJointDesc& self)
{
    return TfStringPrintf(
        "%sDistanceJointDesc(minEnabled=%s, limit=%s, maxEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.minEnabled).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.maxEnabled).c_str(),
        _JointDesc_Repr(self).c_str());
}

void wrapParseUtils()
{
    enum_<UsdPhysicsObjectType>("ObjectType")
        .value("Undefined", UsdPhysicsObjectType::Undefined)
        .value("Scene", UsdPhysicsObjectType::Scene)
        .value("RigidBody", UsdPhysicsObjectType::RigidBody)
        .value("SphereShape", UsdPhysicsObjectType::SphereShape)
        .value("CubeShape", UsdPhysicsObjectType::CubeShape)
        .value("CapsuleShape", UsdPhysicsObjectType::CapsuleShape)
        .value("Capsule1Shape", UsdPhysicsObjectType::Capsule1Shape)
        .value("CylinderShape", UsdPhysicsObjectType::CylinderShape)
        .value("Cylinder1Shape", UsdPhysicsObjectType::Cylinder1Shape)
        .value("ConeShape", UsdPhysicsObjectType::ConeShape)
        .value("MeshShape", UsdPhysicsObjectType::MeshShape)
        .value("PlaneShape", UsdPhysicsObjectType::PlaneShape)
        .value("CustomShape", UsdPhysicsObjectType::CustomShape)
        .value("SpherePointsShape", UsdPhysicsObjectType::SpherePointsShape)
        .value("FixedJoint", UsdPhysicsObjectType::FixedJoint)
        .value("RevoluteJoint", UsdPhysicsObjectType::RevoluteJoint)
        .value("PrismaticJoint", UsdPhysicsObjectType::PrismaticJoint)
        .value("SphericalJoint", UsdPhysicsObjectType::SphericalJoint)
        .value("DistanceJoint", UsdPhysicsObjectType::DistanceJoint)
        .value("D6Joint", UsdPhysicsObjectType::D6Joint)
        .value("CustomJoint", UsdPhysicsObjectType::CustomJoint)
        .value("RigidBodyMaterial", UsdPhysicsObjectType::RigidBodyMaterial)
        .value("Articulation", UsdPhysicsObjectType::Articulation)
        .value("CollisionGroup", UsdPhysicsObjectType::CollisionGroup)
        ;

    enum_<UsdPhysicsAxis>("Axis")
        .value("X", UsdPhysicsAxis::X)
        .value("Y", UsdPhysicsAxis::Y)
        .value("Z", UsdPhysicsAxis::Z)
        ;

    enum_<UsdPhysicsJointDOF>("JointDOF")
        .value("Distance", UsdPhysicsJointDOF::Distance)
        .value("TransX", UsdPhysicsJointDOF::TransX)
        .value("TransY", UsdPhysicsJointDOF::TransY)
        .value("TransZ", UsdPhysicsJointDOF::TransZ)
        .value("RotX", UsdPhysicsJointDOF::RotX)
        .value("RotY", UsdPhysicsJointDOF::RotY)
        .value("RotZ", UsdPhysicsJointDOF::RotZ)
        ;

    class_<_CustomUsdPhysicsTokens>
        cupt("CustomUsdPhysicsTokens");
    cupt
        .def_readwrite("jointTokens", &_CustomUsdPhysicsTokens::jointTokens)
        .def_readwrite("shapeTokens", &_CustomUsdPhysicsTokens::shapeTokens)
        .def_readwrite("instancerTokens", 
                       &_CustomUsdPhysicsTokens::instancerTokens)
        .def("__repr__", _CustomUsdPhysicsTokens_Repr);

    class_<UsdPhysicsObjectDesc>
        podcls("ObjectDesc", no_init);
    podcls
        .def_readonly("type", &UsdPhysicsObjectDesc::type)
        .def_readonly("primPath", &UsdPhysicsObjectDesc::primPath)
        .def_readonly("isValid", &UsdPhysicsObjectDesc::isValid)
        .def("__repr__", _PhysicsObjectDesc_Repr);

    class_<UsdPhysicsSceneDesc, bases<UsdPhysicsObjectDesc>>
        sdcls("SceneDesc", no_init);
    sdcls
        .def_readonly("gravityDirection", &UsdPhysicsSceneDesc::gravityDirection)
        .def_readonly("gravityMagnitude", &UsdPhysicsSceneDesc::gravityMagnitude)
        .def("__repr__", _SceneDesc_Repr);

    class_<UsdPhysicsCollisionGroupDesc, 
        bases<UsdPhysicsObjectDesc>>
            cgcls("CollisionGroupDesc", no_init);
    cgcls
        .def_readonly("invertFilteredGroups", 
                      &UsdPhysicsCollisionGroupDesc::invertFilteredGroups)
        .add_property("mergedGroups",
            make_function(&UsdPhysicsCollisionGroupDesc::GetMergedGroups,
                return_value_policy<TfPySequenceToList>()))
        .add_property("filteredGroups",
            make_function(&UsdPhysicsCollisionGroupDesc::GetFilteredGroups,
                return_value_policy<TfPySequenceToList>()))
        .def_readonly("mergeGroupName", 
                      &UsdPhysicsCollisionGroupDesc::mergeGroupName)
        .def("__repr__", _CollisionGroupDesc_Repr);

    class_<UsdPhysicsRigidBodyMaterialDesc, 
        bases<UsdPhysicsObjectDesc>>
        rbmcls("RigidBodyMaterialDesc", no_init);
    rbmcls
        .def_readonly("staticFriction", 
                      &UsdPhysicsRigidBodyMaterialDesc::staticFriction)
        .def_readonly("dynamicFriction", 
                      &UsdPhysicsRigidBodyMaterialDesc::dynamicFriction)
        .def_readonly("restitution", 
                      &UsdPhysicsRigidBodyMaterialDesc::restitution)
        .def_readonly("density", &UsdPhysicsRigidBodyMaterialDesc::density)
        .def("__repr__", _RigidBodyMaterialDesc_Repr);

    class_<UsdPhysicsShapeDesc, bases<UsdPhysicsObjectDesc>>
        shdcls("ShapeDesc", no_init);
    shdcls
        .def_readonly("rigidBody", &UsdPhysicsShapeDesc::rigidBody)
        .def_readonly("localPos", &UsdPhysicsShapeDesc::localPos)
        .def_readonly("localRot", &UsdPhysicsShapeDesc::localRot)
        .def_readonly("localScale", &UsdPhysicsShapeDesc::localScale)
        .add_property("materials", 
                      make_function(&UsdPhysicsShapeDesc::GetMaterials,
            return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners",
            make_function(&UsdPhysicsShapeDesc::GetSimulationOwners,
                return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions",
            make_function(&UsdPhysicsShapeDesc::GetFilteredCollisions,
                return_value_policy<TfPySequenceToList>()))
        .add_property("collisionGroups",
            make_function(&UsdPhysicsShapeDesc::GetCollisionGroups,
                return_value_policy<TfPySequenceToList>()))
        .def_readonly("collisionEnabled", &UsdPhysicsShapeDesc::collisionEnabled)
        .def("__repr__", _ShapeDesc_Repr);

    class_<UsdPhysicsSphereShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            ssdcls("SphereShapeDesc", no_init);
    ssdcls
        .def_readonly("radius", &UsdPhysicsSphereShapeDesc::radius)
        .def("__repr__", _SphereShapeDesc_Repr);

    class_<UsdPhysicsCapsuleShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            csdcls("CapsuleShapeDesc", no_init);
    csdcls
        .def_readonly("radius", &UsdPhysicsCapsuleShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsCapsuleShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCapsuleShapeDesc::axis)
        .def("__repr__", _CapsuleShapeDesc_Repr);

    class_<UsdPhysicsCapsule1ShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cs1dcls("Capsule1ShapeDesc", no_init);
    cs1dcls
        .def_readonly("topRadius", &UsdPhysicsCapsule1ShapeDesc::topRadius)
        .def_readonly("bottomRadius", &UsdPhysicsCapsule1ShapeDesc::bottomRadius)
        .def_readonly("halfHeight", &UsdPhysicsCapsule1ShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCapsule1ShapeDesc::axis)
        .def("__repr__", _Capsule1ShapeDesc_Repr);

    class_<UsdPhysicsCylinderShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cysdcls("CylinderShapeDesc", no_init);
    cysdcls
        .def_readonly("radius", &UsdPhysicsCylinderShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsCylinderShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCylinderShapeDesc::axis)
        .def("__repr__", _CylinderShapeDesc_Repr);

    class_<UsdPhysicsCylinder1ShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cys1dcls("Cylinder1ShapeDesc", no_init);
    cys1dcls
        .def_readonly("topRadius", &UsdPhysicsCylinder1ShapeDesc::topRadius)
        .def_readonly("bottomRadius", &UsdPhysicsCylinder1ShapeDesc::bottomRadius)
        .def_readonly("halfHeight", &UsdPhysicsCylinder1ShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCylinder1ShapeDesc::axis)
        .def("__repr__", _Cylinder1ShapeDesc_Repr);

    class_<UsdPhysicsConeShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cosdcls("ConeShapeDesc", no_init);
    cosdcls
        .def_readonly("radius", &UsdPhysicsConeShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsConeShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsConeShapeDesc::axis)
        .def("__repr__", _ConeShapeDesc_Repr);

    class_<UsdPhysicsPlaneShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            pscls("PlaneShapeDesc", no_init);
    pscls
        .def_readonly("axis", &UsdPhysicsPlaneShapeDesc::axis)
        .def("__repr__", _PlaneShapeDesc_Repr);

    class_<UsdPhysicsCustomShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cuscls("CustomShapeDesc", no_init);
    cuscls
        .def_readonly("customGeometryToken", 
                      &UsdPhysicsCustomShapeDesc::customGeometryToken)
        .def("__repr__", _CustomShapeDesc_Repr);

    class_<UsdPhysicsCubeShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            cubescls("CubeShapeDesc", no_init);
    cubescls
        .def_readonly("halfExtents", &UsdPhysicsCubeShapeDesc::halfExtents)
        .def("__repr__", _CubeShapeDesc_Repr);

    class_<UsdPhysicsMeshShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            mscls("MeshShapeDesc", no_init);
    mscls
        .add_property("approximation", 
                      &UsdPhysicsMeshShapeDesc::GetApproximation)
        .def_readonly("meshScale", &UsdPhysicsMeshShapeDesc::meshScale)
        .def_readonly("doubleSided", &UsdPhysicsMeshShapeDesc::doubleSided)
        .def("__repr__", _MeshShapeDesc_Repr);

    class_<UsdPhysicsSpherePoint>
        sppb("SpherePoint", no_init);
    sppb
        .def_readonly("center", &UsdPhysicsSpherePoint::center)
        .def_readonly("radius", &UsdPhysicsSpherePoint::radius)
        .def("__repr__", _SpherePoint_Repr);

    class_<UsdPhysicsSpherePointsShapeDesc, 
        bases<UsdPhysicsShapeDesc>>
            spmscls("SpherePointsShapeDesc", no_init);
    spmscls
        .add_property("spherePoints", 
                      &UsdPhysicsSpherePointsShapeDesc::spherePoints)
        .def("__repr__", _SpherePointsShapeDesc_Repr);

    class_<UsdPhysicsRigidBodyDesc, 
        bases<UsdPhysicsObjectDesc>>
            rbcls("RigidBodyDesc", no_init);
    rbcls
        .add_property("collisions", 
                      make_function(&UsdPhysicsRigidBodyDesc::GetCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions",
            make_function(&UsdPhysicsRigidBodyDesc::GetFilteredCollisions,
                return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners",
            make_function(&UsdPhysicsRigidBodyDesc::GetSimulationOwners,
                return_value_policy<TfPySequenceToList>()))
        .def_readonly("position", &UsdPhysicsRigidBodyDesc::position)
        .def_readonly("rotation", &UsdPhysicsRigidBodyDesc::rotation)
        .def_readonly("scale", &UsdPhysicsRigidBodyDesc::scale)
        .def_readonly("rigidBodyEnabled", 
                      &UsdPhysicsRigidBodyDesc::rigidBodyEnabled)
        .def_readonly("kinematicBody", &UsdPhysicsRigidBodyDesc::kinematicBody)
        .def_readonly("startsAsleep", &UsdPhysicsRigidBodyDesc::startsAsleep)
        .def_readonly("linearVelocity", &UsdPhysicsRigidBodyDesc::linearVelocity)
        .def_readonly("angularVelocity", 
                      &UsdPhysicsRigidBodyDesc::angularVelocity)
        .def("__repr__", _RigidBodyDesc_Repr);

    class_<UsdPhysicsJointLimit>
        jlcls("JointLimit", no_init);
    jlcls
        .def_readonly("enabled", &UsdPhysicsJointLimit::enabled)
        .def_readonly("lower", &UsdPhysicsJointLimit::lower)
        .def_readonly("upper", &UsdPhysicsJointLimit::upper)
        .def("__repr__", _JointLimit_Repr);

    class_<UsdPhysicsJointDrive>
        jdcls("JointDrive", no_init);
    jdcls
        .def_readonly("enabled", &UsdPhysicsJointDrive::enabled)
        .def_readonly("targetPosition", &UsdPhysicsJointDrive::targetPosition)
        .def_readonly("targetVelocity", &UsdPhysicsJointDrive::targetVelocity)
        .def_readonly("forceLimit", &UsdPhysicsJointDrive::forceLimit)
        .def_readonly("stiffness", &UsdPhysicsJointDrive::stiffness)
        .def_readonly("damping", &UsdPhysicsJointDrive::damping)
        .def_readonly("acceleration", &UsdPhysicsJointDrive::acceleration)
        .def("__repr__", _JointDrive_Repr);

    class_<UsdPhysicsArticulationDesc, 
        bases<UsdPhysicsObjectDesc>>
            adcls("ArticulationDesc", no_init);
    adcls
        .add_property("rootPrims", 
                      make_function(&UsdPhysicsArticulationDesc::GetRootPrims,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions",
            make_function(&UsdPhysicsArticulationDesc::GetFilteredCollisions,
                return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedJoints",
            make_function(&UsdPhysicsArticulationDesc::GetArticulatedJoints,
                return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedBodies",
            make_function(&UsdPhysicsArticulationDesc::GetArticulatedBodies,
                return_value_policy<TfPySequenceToList>()))
        .def("__repr__", _ArticulationDesc_Repr);

    class_<UsdPhysicsJointDesc, bases<UsdPhysicsObjectDesc>>
        jdscls("JointDesc", no_init);
    jdscls
        .def_readonly("rel0", &UsdPhysicsJointDesc::rel0)
        .def_readonly("rel1", &UsdPhysicsJointDesc::rel1)
        .def_readonly("body0", &UsdPhysicsJointDesc::body0)
        .def_readonly("body1", &UsdPhysicsJointDesc::body1)
        .def_readonly("localPose0Position", 
                      &UsdPhysicsJointDesc::localPose0Position)
        .def_readonly("localPose0Orientation", 
                      &UsdPhysicsJointDesc::localPose0Orientation)
        .def_readonly("localPose1Position", 
                      &UsdPhysicsJointDesc::localPose1Position)
        .def_readonly("localPose1Orientation", 
                      &UsdPhysicsJointDesc::localPose1Orientation)
        .def_readonly("jointEnabled", &UsdPhysicsJointDesc::jointEnabled)
        .def_readonly("breakForce", &UsdPhysicsJointDesc::breakForce)
        .def_readonly("breakTorque", &UsdPhysicsJointDesc::breakTorque)
        .def_readonly("excludeFromArticulation", 
                      &UsdPhysicsJointDesc::excludeFromArticulation)
        .def_readonly("collisionEnabled", &UsdPhysicsJointDesc::collisionEnabled)
        .def("__repr__", _JointDesc_Repr);

    class_<UsdPhysicsCustomJointDesc, 
        bases<UsdPhysicsJointDesc>>
            cjdscls("CustomJointDesc", no_init);

    class_<UsdPhysicsFixedJointDesc, 
        bases<UsdPhysicsJointDesc>>
            fjdscls("FixedJointDesc", no_init);

    class_<std::pair<UsdPhysicsJointDOF, UsdPhysicsJointLimit> >(
        "JointLimitDOFPair")
        .def_readwrite("first", &std::pair<UsdPhysicsJointDOF, 
                       UsdPhysicsJointLimit>::first)
        .def_readwrite("second", &std::pair<UsdPhysicsJointDOF, 
                       UsdPhysicsJointLimit>::second)
        .def("__repr__", _JointLimitDOFPair_Repr);

    class_<std::pair<UsdPhysicsJointDOF, UsdPhysicsJointDrive> >(
        "JointDriveDOFPair")
        .def_readwrite("first", &std::pair<UsdPhysicsJointDOF, 
                       UsdPhysicsJointDrive>::first)
        .def_readwrite("second", &std::pair<UsdPhysicsJointDOF, 
                       UsdPhysicsJointDrive>::second)
        .def("__repr__", _JointDriveDOFPair_Repr);

    class_<UsdPhysicsD6JointDesc, bases<UsdPhysicsJointDesc>>
        d6jdscls("D6JointDesc", no_init);
    d6jdscls
        .def_readonly("jointLimits", &UsdPhysicsD6JointDesc::jointLimits)
        .def_readonly("jointDrives", &UsdPhysicsD6JointDesc::jointDrives)
        .def("__repr__", _D6JointDesc_Repr);

    class_<UsdPhysicsPrismaticJointDesc, bases<
        UsdPhysicsJointDesc>>
            pjdscls("PrismaticJointDesc", no_init);
    pjdscls
        .def_readonly("axis", &UsdPhysicsPrismaticJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsPrismaticJointDesc::limit)
        .def_readonly("drive", &UsdPhysicsPrismaticJointDesc::drive)
        .def("__repr__", _PrismaticJointDesc_Repr);

    class_<UsdPhysicsSphericalJointDesc, bases<
        UsdPhysicsJointDesc>>
            sjdscls("SphericalJointDesc", no_init);
    sjdscls
        .def_readonly("axis", &UsdPhysicsSphericalJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsSphericalJointDesc::limit)
        .def("__repr__", _SphericalJointDesc_Repr);

    class_<UsdPhysicsRevoluteJointDesc, bases<
        UsdPhysicsJointDesc>>
            rjdscls("RevoluteJointDesc", no_init);
    rjdscls
        .def_readonly("axis", &UsdPhysicsRevoluteJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsRevoluteJointDesc::limit)
        .def_readonly("drive", &UsdPhysicsRevoluteJointDesc::drive)
        .def("__repr__", _RevoluteJointDesc_Repr);

    class_<UsdPhysicsDistanceJointDesc, bases<   
        UsdPhysicsJointDesc>>
            djdscls("DistanceJointDesc", no_init);
    djdscls
        .def_readonly("minEnabled", &UsdPhysicsDistanceJointDesc::minEnabled)
        .def_readonly("limit", &UsdPhysicsDistanceJointDesc::limit)
        .def_readonly("maxEnabled", &UsdPhysicsDistanceJointDesc::maxEnabled)
        .def("__repr__", _DistanceJointDesc_Repr);

    registerVectorConverter<UsdCollectionMembershipQuery>
        ("PhysicsCollectionMembershipQueryVector");

    registerVectorConverter<std::pair<UsdPhysicsJointDOF, 
        UsdPhysicsJointLimit>>
            ("PhysicsJointLimitDOFVector");

    registerVectorConverter<std::pair<UsdPhysicsJointDOF, 
        UsdPhysicsJointDrive>>
            ("PhysicsJointDriveDOFVector");

    registerVectorConverter<UsdPhysicsSpherePoint>("PhysicsSpherePointVector");

    registerVectorConverter<UsdPhysicsSceneDesc>("SceneDescVector");

    registerVectorConverter<UsdPhysicsRigidBodyDesc>("RigidBodyDescVector");

    registerVectorConverter<UsdPhysicsSphereShapeDesc>("SphereShapeDescVector");

    registerVectorConverter<UsdPhysicsCapsuleShapeDesc>("CapsuleShapeDescVector");

    registerVectorConverter<UsdPhysicsCapsule1ShapeDesc>("Capsule1ShapeDescVector");

    registerVectorConverter<UsdPhysicsCylinderShapeDesc>(
        "CylinderShapeDescVector");

    registerVectorConverter<UsdPhysicsCylinder1ShapeDesc>(
        "Cylinder1ShapeDescVector");

    registerVectorConverter<UsdPhysicsConeShapeDesc>("ConeShapeDescVector");

    registerVectorConverter<UsdPhysicsCubeShapeDesc>("CubeShapeDescVector");

    registerVectorConverter<UsdPhysicsMeshShapeDesc>("MeshShapeDescVector");

    registerVectorConverter<UsdPhysicsPlaneShapeDesc>("PlaneShapeDescVector");

    registerVectorConverter<UsdPhysicsCustomShapeDesc>("CustomShapeDescVector");

    registerVectorConverter<UsdPhysicsSpherePointsShapeDesc>(
        "SpherePointsShapeDescVector");

    registerVectorConverter<UsdPhysicsJointDesc>("JointDescVector");

    registerVectorConverter<UsdPhysicsFixedJointDesc>("FixedJointDescVector");

    registerVectorConverter<UsdPhysicsDistanceJointDesc>(
        "DistanceJointDescVector");

    registerVectorConverter<UsdPhysicsRevoluteJointDesc>(
        "RevoluteJointDescVector");

    registerVectorConverter<UsdPhysicsPrismaticJointDesc>(
        "PrismaticJointDescVector");

    registerVectorConverter<UsdPhysicsSphericalJointDesc>(
        "SphericalJointDescVector");

    registerVectorConverter<UsdPhysicsD6JointDesc>("D6JointDescVector");

    registerVectorConverter<UsdPhysicsCustomJointDesc>("CustomJointDescVector");

    registerVectorConverter<UsdPhysicsRigidBodyMaterialDesc>(
        "RigidBodyMaterialDescVector");

    registerVectorConverter<UsdPhysicsArticulationDesc>(
        "ArticulationDescVector");

    registerVectorConverter<UsdPhysicsCollisionGroupDesc>(
        "CollisionGroupDescVector");

    def("LoadUsdPhysicsFromRange", _LoadUsdPhysicsFromRange,
        (args("stage"), args("includePaths"), args("excludePaths") = std::vector<SdfPath>(), args("customTokens") =
            _CustomUsdPhysicsTokens(), args("simulationOwners") = 
        std::vector<SdfPath>()));
}
