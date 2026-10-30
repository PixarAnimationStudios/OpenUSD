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
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

NB_MAKE_OPAQUE(std::pair<PXR_NS::UsdPhysicsJointDOF, PXR_NS::UsdPhysicsJointLimit>);
NB_MAKE_OPAQUE(std::pair<PXR_NS::UsdPhysicsJointDOF, PXR_NS::UsdPhysicsJointDrive>);

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

template <typename DescType>
void moveDescsToDict(UsdPhysicsObjectType objectType,
    TfSpan<const SdfPath> primsSource,
    TfSpan<const UsdPhysicsObjectDesc> objectDescsSource,
    nb::dict* retDict)
{
    const SdfPathVector primPaths(primsSource.begin(), primsSource.end());
    const TfSpan<const DescType> objDescsSpan((const DescType*)objectDescsSource.data(), objectDescsSource.size());
    const std::vector<DescType> objDescs(objDescsSpan.begin(), objDescsSpan.end());

    (*retDict)[nb::cast(objectType)] =
        nb::make_tuple(
            primPaths, 
            objDescs);
}

void ReportPhysicsObjectsFn(UsdPhysicsObjectType type,
                            TfSpan<const SdfPath> primPaths,
                            TfSpan<const UsdPhysicsObjectDesc> objectDescs,
                            const VtValue& userData)
{
    nb::dict* retDict = userData.GetWithDefault<nb::dict*>(nullptr);
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

struct _UsdPhysicsCustomTokens
{
    _UsdPhysicsCustomTokens()
    {

    }

    /// Custom joints to be reported by parsing
    nb::list jointTokens;
    /// Custom shapes to be reported by parsing
    nb::list shapeTokens;
    /// Custom physics instancers to be reported by parsing
    nb::list instancerTokens;
};

struct _DeprecatedCustomUsdPhysicsTokens : public _UsdPhysicsCustomTokens {};

std::string GetString(nb::handle po)
{
    return nb::cast<std::string>(nb::str(po));
}

nb::dict _UsdPhysicsLoadStageFromPrimRange(UsdStageWeakPtr stage,
    const std::vector<SdfPath>& includePaths, 
    const std::vector<SdfPath>& excludePaths,
    const _UsdPhysicsCustomTokens& customTokens,
    const std::vector<SdfPath>& simulationOwners)
{
    UsdPhysicsCustomTokens parsingCustomTokens;
    bool customTokensValid = false;
    const size_t jointTokesSize = nb::len(customTokens.jointTokens);
    const size_t shapeTokesSize = nb::len(customTokens.shapeTokens);
    const size_t instancerTokesSize = nb::len(customTokens.instancerTokens);
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

    nb::dict retDict;    
    if (!UsdPhysicsLoadStageFromPrimRange(stage, includePaths,
        ReportPhysicsObjectsFn, VtValue(&retDict),
        !excludePaths.empty() ? &excludePaths : nullptr,
        customTokensValid ? &parsingCustomTokens : nullptr,
        !simulationOwners.empty() ? &simulationOwners : nullptr)) {

        TF_WARN("Unable to perform physics parsing on stage.");
    }
    return retDict;
}

nb::dict _DeprecatedLoadUsdPhysicsFromRange(UsdStageWeakPtr stage,
    const std::vector<SdfPath>& includePaths, 
    const std::vector<SdfPath>& excludePaths,
    const _UsdPhysicsCustomTokens& customTokens,
    const std::vector<SdfPath>& simulationOwners)
{
    PyErr_WarnEx(PyExc_DeprecationWarning,
                 "LoadUsdPhysicsFromRange is deprecated, use "
                 "UsdPhysicsLoadStageFromPrimRange instead.", 1);
    return _UsdPhysicsLoadStageFromPrimRange(
        stage, includePaths, excludePaths, customTokens, simulationOwners);
}

static std::string
_UsdPhysicsCustomTokens_Repr(const UsdPhysicsCustomTokens& self)
{
    return TfStringPrintf(
        "%sUsdPhysicsCustomTokens(jointTokens=%s, shapeTokens=%s, "
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
_CustomShapeDesc_GetCustomGeometryToken(const UsdPhysicsCustomShapeDesc& self)
{
    return self.customGeometryToken.GetString();
}

static std::string
_CustomShapeDesc_Repr(const UsdPhysicsCustomShapeDesc& self)
{
    return TfStringPrintf("%sCustomShapeDesc(customGeometryToken=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.customGeometryToken.GetString()).c_str(),
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

void wrapParseUtils(nanobind::module_ &m)
{
    nb::enum_<UsdPhysicsObjectType>(m, "ObjectType")
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

    nb::enum_<UsdPhysicsAxis>(m, "Axis")
        .value("X", UsdPhysicsAxis::X)
        .value("Y", UsdPhysicsAxis::Y)
        .value("Z", UsdPhysicsAxis::Z)
        ;

    nb::enum_<UsdPhysicsJointDOF>(m, "JointDOF")
        .value("Distance", UsdPhysicsJointDOF::Distance)
        .value("TransX", UsdPhysicsJointDOF::TransX)
        .value("TransY", UsdPhysicsJointDOF::TransY)
        .value("TransZ", UsdPhysicsJointDOF::TransZ)
        .value("RotX", UsdPhysicsJointDOF::RotX)
        .value("RotY", UsdPhysicsJointDOF::RotY)
        .value("RotZ", UsdPhysicsJointDOF::RotZ)
        ;

    nb::class_<_UsdPhysicsCustomTokens>(m, "UsdPhysicsCustomTokens")
        .def(nb::init<>())
        .def_rw("jointTokens", &_UsdPhysicsCustomTokens::jointTokens)
        .def_rw("shapeTokens", &_UsdPhysicsCustomTokens::shapeTokens)
        .def_rw("instancerTokens", &_UsdPhysicsCustomTokens::instancerTokens)
        .def("__repr__", _UsdPhysicsCustomTokens_Repr);

    nb::class_<_DeprecatedCustomUsdPhysicsTokens, _UsdPhysicsCustomTokens>(
        m, "CustomUsdPhysicsTokens")
        .def("__init__", [](_DeprecatedCustomUsdPhysicsTokens *self) {
            PyErr_WarnEx(PyExc_DeprecationWarning,
                         "CustomUsdPhysicsTokens is deprecated, use "
                         "UsdPhysicsCustomTokens instead.", 1);
            new (self) _DeprecatedCustomUsdPhysicsTokens();
        })
        .def_rw("jointTokens", &_UsdPhysicsCustomTokens::jointTokens)
        .def_rw("shapeTokens", &_UsdPhysicsCustomTokens::shapeTokens)
        .def_rw("instancerTokens", &_UsdPhysicsCustomTokens::instancerTokens)
        .def("__repr__", _UsdPhysicsCustomTokens_Repr);

    nb::class_<UsdPhysicsObjectDesc>(m, "ObjectDesc")
        .def_ro("type", &UsdPhysicsObjectDesc::type)
        .def_ro("primPath", &UsdPhysicsObjectDesc::primPath)
        .def_ro("isValid", &UsdPhysicsObjectDesc::isValid)
        .def("__repr__", _PhysicsObjectDesc_Repr);

    nb::class_<UsdPhysicsSceneDesc, UsdPhysicsObjectDesc>(m, "SceneDesc")
        .def_ro("gravityDirection", &UsdPhysicsSceneDesc::gravityDirection)
        .def_ro("gravityMagnitude", &UsdPhysicsSceneDesc::gravityMagnitude)
        .def("__repr__", _SceneDesc_Repr);

    nb::class_<UsdPhysicsCollisionGroupDesc, UsdPhysicsObjectDesc>(
        m, "CollisionGroupDesc")
        .def_ro("invertFilteredGroups", 
                &UsdPhysicsCollisionGroupDesc::invertFilteredGroups)
        .def_prop_ro("mergedGroups", &UsdPhysicsCollisionGroupDesc::GetMergedGroups)
        .def_prop_ro("filteredGroups", &UsdPhysicsCollisionGroupDesc::GetFilteredGroups)
        .def_ro("mergeGroupName", &UsdPhysicsCollisionGroupDesc::mergeGroupName)
        .def("__repr__", _CollisionGroupDesc_Repr);

    nb::class_<UsdPhysicsRigidBodyMaterialDesc, UsdPhysicsObjectDesc>(
        m, "RigidBodyMaterialDesc")
        .def_ro("staticFriction", &UsdPhysicsRigidBodyMaterialDesc::staticFriction)
        .def_ro("dynamicFriction", &UsdPhysicsRigidBodyMaterialDesc::dynamicFriction)
        .def_ro("restitution", &UsdPhysicsRigidBodyMaterialDesc::restitution)
        .def_ro("density", &UsdPhysicsRigidBodyMaterialDesc::density)
        .def("__repr__", _RigidBodyMaterialDesc_Repr);

    nb::class_<UsdPhysicsShapeDesc, UsdPhysicsObjectDesc>(m, "ShapeDesc")
        .def_ro("rigidBody", &UsdPhysicsShapeDesc::rigidBody)
        .def_ro("localPos", &UsdPhysicsShapeDesc::localPos)
        .def_ro("localRot", &UsdPhysicsShapeDesc::localRot)
        .def_ro("localScale", &UsdPhysicsShapeDesc::localScale)
        .def_prop_ro("materials", &UsdPhysicsShapeDesc::GetMaterials)
        .def_prop_ro("simulationOwners", &UsdPhysicsShapeDesc::GetSimulationOwners)
        .def_prop_ro("filteredCollisions", &UsdPhysicsShapeDesc::GetFilteredCollisions)
        .def_prop_ro("collisionGroups", &UsdPhysicsShapeDesc::GetCollisionGroups)
        .def_ro("collisionEnabled", &UsdPhysicsShapeDesc::collisionEnabled)
        .def("__repr__", _ShapeDesc_Repr);

    nb::class_<UsdPhysicsSphereShapeDesc, UsdPhysicsShapeDesc>(
        m, "SphereShapeDesc")
        .def_ro("radius", &UsdPhysicsSphereShapeDesc::radius)
        .def("__repr__", _SphereShapeDesc_Repr);

    nb::class_<UsdPhysicsCapsuleShapeDesc, UsdPhysicsShapeDesc>(
        m, "CapsuleShapeDesc")
        .def_ro("radius", &UsdPhysicsCapsuleShapeDesc::radius)
        .def_ro("halfHeight", &UsdPhysicsCapsuleShapeDesc::halfHeight)
        .def_ro("axis", &UsdPhysicsCapsuleShapeDesc::axis)
        .def("__repr__", _CapsuleShapeDesc_Repr);

    nb::class_<UsdPhysicsCapsule1ShapeDesc, UsdPhysicsShapeDesc>(
        m, "Capsule1ShapeDesc")
        .def_ro("topRadius", &UsdPhysicsCapsule1ShapeDesc::topRadius)
        .def_ro("bottomRadius", &UsdPhysicsCapsule1ShapeDesc::bottomRadius)
        .def_ro("halfHeight", &UsdPhysicsCapsule1ShapeDesc::halfHeight)
        .def_ro("axis", &UsdPhysicsCapsule1ShapeDesc::axis)
        .def("__repr__", _Capsule1ShapeDesc_Repr);

    nb::class_<UsdPhysicsCylinderShapeDesc, UsdPhysicsShapeDesc>(
        m, "CylinderShapeDesc")
        .def_ro("radius", &UsdPhysicsCylinderShapeDesc::radius)
        .def_ro("halfHeight", &UsdPhysicsCylinderShapeDesc::halfHeight)
        .def_ro("axis", &UsdPhysicsCylinderShapeDesc::axis)
        .def("__repr__", _CylinderShapeDesc_Repr);

    nb::class_<UsdPhysicsCylinder1ShapeDesc, UsdPhysicsShapeDesc>(
        m, "Cylinder1ShapeDesc")
        .def_ro("topRadius", &UsdPhysicsCylinder1ShapeDesc::topRadius)
        .def_ro("bottomRadius", &UsdPhysicsCylinder1ShapeDesc::bottomRadius)
        .def_ro("halfHeight", &UsdPhysicsCylinder1ShapeDesc::halfHeight)
        .def_ro("axis", &UsdPhysicsCylinder1ShapeDesc::axis)
        .def("__repr__", _Cylinder1ShapeDesc_Repr);

    nb::class_<UsdPhysicsConeShapeDesc, UsdPhysicsShapeDesc>(
        m, "ConeShapeDesc")
        .def_ro("radius", &UsdPhysicsConeShapeDesc::radius)
        .def_ro("halfHeight", &UsdPhysicsConeShapeDesc::halfHeight)
        .def_ro("axis", &UsdPhysicsConeShapeDesc::axis)
        .def("__repr__", _ConeShapeDesc_Repr);

    nb::class_<UsdPhysicsPlaneShapeDesc, UsdPhysicsShapeDesc>(
        m, "PlaneShapeDesc")
        .def_ro("axis", &UsdPhysicsPlaneShapeDesc::axis)
        .def("__repr__", _PlaneShapeDesc_Repr);

    nb::class_<UsdPhysicsCustomShapeDesc, UsdPhysicsShapeDesc>(
        m, "CustomShapeDesc")
        .def_prop_ro("customGeometryToken", _CustomShapeDesc_GetCustomGeometryToken)
        .def("__repr__", _CustomShapeDesc_Repr);

    nb::class_<UsdPhysicsCubeShapeDesc, UsdPhysicsShapeDesc>(
        m, "CubeShapeDesc")
        .def_ro("halfExtents", &UsdPhysicsCubeShapeDesc::halfExtents)
        .def("__repr__", _CubeShapeDesc_Repr);

    nb::class_<UsdPhysicsMeshShapeDesc, UsdPhysicsShapeDesc>(
        m, "MeshShapeDesc")
        .def_prop_ro("approximation", &UsdPhysicsMeshShapeDesc::GetApproximation)
        .def_ro("meshScale", &UsdPhysicsMeshShapeDesc::meshScale)
        .def_ro("doubleSided", &UsdPhysicsMeshShapeDesc::doubleSided)
        .def("__repr__", _MeshShapeDesc_Repr);

    nb::class_<UsdPhysicsSpherePoint>(m, "SpherePoint")
        .def_ro("center", &UsdPhysicsSpherePoint::center)
        .def_ro("radius", &UsdPhysicsSpherePoint::radius)
        .def("__repr__", _SpherePoint_Repr);

    nb::class_<UsdPhysicsSpherePointsShapeDesc, UsdPhysicsShapeDesc>(
        m, "SpherePointsShapeDesc")
        .def_ro("spherePoints", &UsdPhysicsSpherePointsShapeDesc::spherePoints)
        .def("__repr__", _SpherePointsShapeDesc_Repr);

    nb::class_<UsdPhysicsRigidBodyDesc, UsdPhysicsObjectDesc>(
        m, "RigidBodyDesc")
        .def_prop_ro("collisions", &UsdPhysicsRigidBodyDesc::GetCollisions)
        .def_prop_ro("filteredCollisions", &UsdPhysicsRigidBodyDesc::GetFilteredCollisions)
        .def_prop_ro("simulationOwners", &UsdPhysicsRigidBodyDesc::GetSimulationOwners)
        .def_ro("position", &UsdPhysicsRigidBodyDesc::position)
        .def_ro("rotation", &UsdPhysicsRigidBodyDesc::rotation)
        .def_ro("scale", &UsdPhysicsRigidBodyDesc::scale)
        .def_ro("rigidBodyEnabled", &UsdPhysicsRigidBodyDesc::rigidBodyEnabled)
        .def_ro("kinematicBody", &UsdPhysicsRigidBodyDesc::kinematicBody)
        .def_ro("startsAsleep", &UsdPhysicsRigidBodyDesc::startsAsleep)
        .def_ro("linearVelocity", &UsdPhysicsRigidBodyDesc::linearVelocity)
        .def_ro("angularVelocity", &UsdPhysicsRigidBodyDesc::angularVelocity)
        .def("__repr__", _RigidBodyDesc_Repr);

    nb::class_<UsdPhysicsJointLimit>(m, "JointLimit")
        .def_ro("enabled", &UsdPhysicsJointLimit::enabled)
        .def_ro("lower", &UsdPhysicsJointLimit::lower)
        .def_ro("upper", &UsdPhysicsJointLimit::upper)
        .def("__repr__", _JointLimit_Repr);

    nb::class_<UsdPhysicsJointDrive>(m, "JointDrive")
        .def_ro("enabled", &UsdPhysicsJointDrive::enabled)
        .def_ro("targetPosition", &UsdPhysicsJointDrive::targetPosition)
        .def_ro("targetVelocity", &UsdPhysicsJointDrive::targetVelocity)
        .def_ro("forceLimit", &UsdPhysicsJointDrive::forceLimit)
        .def_ro("stiffness", &UsdPhysicsJointDrive::stiffness)
        .def_ro("damping", &UsdPhysicsJointDrive::damping)
        .def_ro("acceleration", &UsdPhysicsJointDrive::acceleration)
        .def("__repr__", _JointDrive_Repr);

    nb::class_<UsdPhysicsArticulationDesc, UsdPhysicsObjectDesc>(
        m, "ArticulationDesc")
        .def_prop_ro("rootPrims", &UsdPhysicsArticulationDesc::GetRootPrims)
        .def_prop_ro("filteredCollisions", &UsdPhysicsArticulationDesc::GetFilteredCollisions)
        .def_prop_ro("articulatedJoints", &UsdPhysicsArticulationDesc::GetArticulatedJoints)
        .def_prop_ro("articulatedBodies", &UsdPhysicsArticulationDesc::GetArticulatedBodies)
        .def("__repr__", _ArticulationDesc_Repr);

    nb::class_<UsdPhysicsJointDesc, UsdPhysicsObjectDesc>(m, "JointDesc")
        .def_ro("rel0", &UsdPhysicsJointDesc::rel0)
        .def_ro("rel1", &UsdPhysicsJointDesc::rel1)
        .def_ro("body0", &UsdPhysicsJointDesc::body0)
        .def_ro("body1", &UsdPhysicsJointDesc::body1)
        .def_ro("localPose0Position", &UsdPhysicsJointDesc::localPose0Position)
        .def_ro("localPose0Orientation", &UsdPhysicsJointDesc::localPose0Orientation)
        .def_ro("localPose1Position", &UsdPhysicsJointDesc::localPose1Position)
        .def_ro("localPose1Orientation", &UsdPhysicsJointDesc::localPose1Orientation)
        .def_ro("jointEnabled", &UsdPhysicsJointDesc::jointEnabled)
        .def_ro("breakForce", &UsdPhysicsJointDesc::breakForce)
        .def_ro("breakTorque", &UsdPhysicsJointDesc::breakTorque)
        .def_ro("excludeFromArticulation", &UsdPhysicsJointDesc::excludeFromArticulation)
        .def_ro("collisionEnabled", &UsdPhysicsJointDesc::collisionEnabled)
        .def("__repr__", _JointDesc_Repr);

    nb::class_<UsdPhysicsCustomJointDesc, UsdPhysicsJointDesc>(
        m, "CustomJointDesc");

    nb::class_<UsdPhysicsFixedJointDesc, UsdPhysicsJointDesc>(
        m, "FixedJointDesc");

    using JointLimitDOFPair = std::pair<UsdPhysicsJointDOF, UsdPhysicsJointLimit>;
    nb::class_<JointLimitDOFPair>(m, "JointLimitDOFPair")
        .def(nb::init<>())
        .def_rw("first", &JointLimitDOFPair::first)
        .def_rw("second", &JointLimitDOFPair::second)
        .def("__repr__", _JointLimitDOFPair_Repr);

    using JointDriveDOFPair = std::pair<UsdPhysicsJointDOF, UsdPhysicsJointDrive>;
    nb::class_<JointDriveDOFPair>(m, "JointDriveDOFPair")
        .def(nb::init<>())
        .def_rw("first", &JointDriveDOFPair::first)
        .def_rw("second", &JointDriveDOFPair::second)
        .def("__repr__", _JointDriveDOFPair_Repr);

    nb::class_<UsdPhysicsD6JointDesc, UsdPhysicsJointDesc>(m, "D6JointDesc")
        .def_ro("jointLimits", &UsdPhysicsD6JointDesc::jointLimits)
        .def_ro("jointDrives", &UsdPhysicsD6JointDesc::jointDrives)
        .def("__repr__", _D6JointDesc_Repr);

    nb::class_<UsdPhysicsPrismaticJointDesc, UsdPhysicsJointDesc>(
        m, "PrismaticJointDesc")
        .def_ro("axis", &UsdPhysicsPrismaticJointDesc::axis)
        .def_ro("limit", &UsdPhysicsPrismaticJointDesc::limit)
        .def_ro("drive", &UsdPhysicsPrismaticJointDesc::drive)
        .def("__repr__", _PrismaticJointDesc_Repr);

    nb::class_<UsdPhysicsSphericalJointDesc, UsdPhysicsJointDesc>(
        m, "SphericalJointDesc")
        .def_ro("axis", &UsdPhysicsSphericalJointDesc::axis)
        .def_ro("limit", &UsdPhysicsSphericalJointDesc::limit)
        .def("__repr__", _SphericalJointDesc_Repr);

    nb::class_<UsdPhysicsRevoluteJointDesc, UsdPhysicsJointDesc>(
        m, "RevoluteJointDesc")
        .def_ro("axis", &UsdPhysicsRevoluteJointDesc::axis)
        .def_ro("limit", &UsdPhysicsRevoluteJointDesc::limit)
        .def_ro("drive", &UsdPhysicsRevoluteJointDesc::drive)
        .def("__repr__", _RevoluteJointDesc_Repr);

    nb::class_<UsdPhysicsDistanceJointDesc, UsdPhysicsJointDesc>(
        m, "DistanceJointDesc")
        .def_ro("minEnabled", &UsdPhysicsDistanceJointDesc::minEnabled)
        .def_ro("limit", &UsdPhysicsDistanceJointDesc::limit)
        .def_ro("maxEnabled", &UsdPhysicsDistanceJointDesc::maxEnabled)
        .def("__repr__", _DistanceJointDesc_Repr);

    m.def("LoadUsdPhysicsFromRange", _DeprecatedLoadUsdPhysicsFromRange,
          nb::arg("stage"), nb::arg("includePaths"),
          nb::arg("excludePaths") = std::vector<SdfPath>(),
          nb::arg("customTokens") = _UsdPhysicsCustomTokens(),
          nb::arg("simulationOwners") = std::vector<SdfPath>());

    m.def("UsdPhysicsLoadStageFromPrimRange", _UsdPhysicsLoadStageFromPrimRange,
          nb::arg("stage"), nb::arg("includePaths"),
          nb::arg("excludePaths") = std::vector<SdfPath>(),
          nb::arg("customTokens") = _UsdPhysicsCustomTokens(),
          nb::arg("simulationOwners") = std::vector<SdfPath>());
}
