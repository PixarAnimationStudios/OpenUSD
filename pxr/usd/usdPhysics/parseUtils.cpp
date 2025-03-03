//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/gf/transform.h"

#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/capsule_1.h"
#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/cylinder_1.h"
#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usdGeom/plane.h"
#include "pxr/usd/usdGeom/points.h"

#include "pxr/usd/usdGeom/metrics.h"

#include "pxr/usd/usdShade/materialBindingAPI.h"

#include "pxr/usd/usdPhysics/parseUtils.h"
#include "pxr/usd/usdPhysics/parseDesc.h"

#include "pxr/usd/usdPhysics/scene.h"
#include "pxr/usd/usdPhysics/collisionGroup.h"
#include "pxr/usd/usdPhysics/materialAPI.h"
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usdPhysics/collisionAPI.h"
#include "pxr/usd/usdPhysics/articulationRootAPI.h"

#include "pxr/usd/usdPhysics/joint.h"
#include "pxr/usd/usdPhysics/fixedJoint.h"
#include "pxr/usd/usdPhysics/revoluteJoint.h"
#include "pxr/usd/usdPhysics/prismaticJoint.h"
#include "pxr/usd/usdPhysics/sphericalJoint.h"
#include "pxr/usd/usdPhysics/distanceJoint.h"

#include "pxr/usd/usdPhysics/limitAPI.h"
#include "pxr/usd/usdPhysics/driveAPI.h"
#include "pxr/usd/usdPhysics/filteredPairsAPI.h"
#include "pxr/usd/usdPhysics/meshCollisionAPI.h"


#include "pxr/usd/usdGeom/pointInstancer.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"


PXR_NAMESPACE_OPEN_SCOPE


static constexpr float infSentinel = 0.5e38f;
static constexpr float defaultGravity = 9.81f;

// Gather the filtered pairs from UsdPhysicsFilteredPairsAPI if applied to a prim
void _ParseFilteredPairs(const UsdPrim& usdPrim, SdfPathVector* outFilteredPairs)
{
    UsdPhysicsFilteredPairsAPI filteredPairsAPI =
        UsdPhysicsFilteredPairsAPI::Get(usdPrim.GetStage(), 
                                        usdPrim.GetPrimPath());

    if (outFilteredPairs && filteredPairsAPI && 
        filteredPairsAPI.GetFilteredPairsRel())
    {
        filteredPairsAPI.GetFilteredPairsRel().GetTargets(outFilteredPairs);
    }
}

// Parse base descriptor for given UsdPhysicsArticulationRootAPI
bool _ParseArticulationDesc(const UsdPhysicsArticulationRootAPI &articulationAPI,
                            UsdPhysicsArticulationDesc *outArticulationDesc)
{
    if (outArticulationDesc && articulationAPI)
    {
        _ParseFilteredPairs(articulationAPI.GetPrim(),
                            &outArticulationDesc->filteredCollisions);

        outArticulationDesc->primPath = articulationAPI.GetPrim().GetPrimPath();
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsArticulationRootAPI or "
                        "UsdPhysicsArticulationDesc is not valid.");
        return false;
    }
    return true;
}

// Get collision type for given prim, collision type is determined based on
// the UsdGeom type.
UsdPhysicsObjectType _GetCollisionType(const UsdPrim& prim,
    const std::vector<TfToken>* customTokens, TfToken* customGeometryToken)
{
    UsdPhysicsObjectType retVal = UsdPhysicsObjectType::Undefined;

    // Custom shape handling, custom shape can be defined by the user
    // we need to check whether a custom collisionAPI or type is on a prim
    if (customTokens)
    {
        const TfTokenVector& apis = 
            prim.GetPrimTypeInfo().GetAppliedAPISchemas();

        const TfToken& primType = prim.GetTypeName();
        for (size_t i = 0; i < customTokens->size(); i++)
        {
            for (size_t j = 0; j < apis.size(); j++)
            {
                if (apis[j] == (*customTokens)[i])
                {
                    retVal = UsdPhysicsObjectType::CustomShape;
                    if (customGeometryToken) 
                    {
                        *customGeometryToken = apis[j];
                    }
                    break;
                }
            }
            if (retVal == UsdPhysicsObjectType::CustomShape)
            {
                break;
            }
            if (primType == (*customTokens)[i])
            {
                retVal = UsdPhysicsObjectType::CustomShape;
                if (customGeometryToken) 
                {
                    *customGeometryToken = primType;
                }
                break;
            }
        }
    }

    if (retVal == UsdPhysicsObjectType::CustomShape) 
    {
        return retVal;
    }

    // geomgprim that belongs to that collisionm define type based on that
    if (prim.IsA<UsdGeomGprim>())
    {
        if (prim.IsA<UsdGeomMesh>())
        {
            retVal = UsdPhysicsObjectType::MeshShape;
        }
        else if (prim.IsA<UsdGeomCube>())
        {
            retVal = UsdPhysicsObjectType::CubeShape;
        }
        else if (prim.IsA<UsdGeomSphere>())
        {
            retVal = UsdPhysicsObjectType::SphereShape;
        }
        else if (prim.IsA<UsdGeomCapsule>())
        {
            retVal = UsdPhysicsObjectType::CapsuleShape;
        }
        else if (prim.IsA<UsdGeomCapsule_1>())
        {
            retVal = UsdPhysicsObjectType::Capsule1Shape;
        }
        else if (prim.IsA<UsdGeomCylinder>())
        {
            retVal = UsdPhysicsObjectType::CylinderShape;
        }
        else if (prim.IsA<UsdGeomCylinder_1>())
        {
            retVal = UsdPhysicsObjectType::Cylinder1Shape;
        }
        else if (prim.IsA<UsdGeomCone>())
        {
            retVal = UsdPhysicsObjectType::ConeShape;
        }
        else if (prim.IsA<UsdGeomPlane>())
        {
            retVal = UsdPhysicsObjectType::PlaneShape;
        }
        else if (prim.IsA<UsdGeomPoints>())
        {
            retVal = UsdPhysicsObjectType::SpherePointsShape;
        }
    }

    return retVal;
}

// Gather material binding, where the expected puporse token is "physics"
SdfPath _GetMaterialBinding(const UsdPrim& usdPrim)
{
    SdfPath materialPath = SdfPath();

    const static TfToken physicsPurpose("physics");
    UsdShadeMaterialBindingAPI materialBindingAPI = 
        UsdShadeMaterialBindingAPI(usdPrim);
    if (materialBindingAPI)
    {
        UsdShadeMaterial material = 
            materialBindingAPI.ComputeBoundMaterial(physicsPurpose);
        if (material)
        {
            materialPath = material.GetPrim().GetPrimPath();
        }
    }

    return materialPath;
}

// Finalize collision descriptor
void _FinalizeCollisionDesc(const UsdPhysicsCollisionAPI& colAPI, 
    UsdPhysicsShapeDesc* outDesc)
{
    // Get material information for the collider
    const SdfPath& materialPath = _GetMaterialBinding(colAPI.GetPrim());
    if (materialPath != SdfPath())
    {
        const UsdPrim materialPrim = 
            colAPI.GetPrim().GetStage()->GetPrimAtPath(materialPath);
        if (materialPrim && materialPrim.HasAPI<UsdPhysicsMaterialAPI>())
        {
            outDesc->materials.push_back(materialPath);
        }
    }

    _ParseFilteredPairs(colAPI.GetPrim(), &outDesc->filteredCollisions);
    colAPI.GetCollisionEnabledAttr().Get(&outDesc->collisionEnabled);
    const UsdRelationship ownerRel = colAPI.GetSimulationOwnerRel();
    if (ownerRel)
    {
        ownerRel.GetTargets(&outDesc->simulationOwners);
    }
}

// Parse sphere shape desc
bool _ParseSphereShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsSphereShapeDesc* outSphereShapeDesc)
{
    if (outSphereShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomSphere shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;

            // Check scale, its part of the collision size
            {
                const GfVec3d sc = tr.GetScale();
                radius = fmaxf(fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0]))),
                    fabsf(float(sc[2])));
            }

            // Get shape parameters
            {

                double radiusAttr;
                shape.GetRadiusAttr().Get(&radiusAttr);
                radius *= (float)radiusAttr;
            }

            outSphereShapeDesc->radius = fabsf(radius);
            outSphereShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outSphereShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomSphere.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsSphereShapeDesc is not valid.");
        return false;
    }
    return true;
}

// Parse cube shape desc
bool _ParseCubeShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCubeShapeDesc* outCubeShapeDesc)
{
    if (outCubeShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCube shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            GfVec3f halfExtents;

            // Add scale
            {
                const GfVec3d sc = tr.GetScale();
                // scale is taken, its a part of the cube size, as the physics 
                // does not support scale
                halfExtents = GfVec3f(sc);
            }

            // Get shape parameters
            {
                UsdGeomCube shape(usdPrim);
                double sizeAttr;
                shape.GetSizeAttr().Get(&sizeAttr);
                // convert cube edge length to half extend
                sizeAttr = abs(sizeAttr) * 0.5f;
                halfExtents *= (float)sizeAttr;
            }

            outCubeShapeDesc->halfExtents = halfExtents;
            outCubeShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outCubeShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCube.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCubeShapeDesc is not valid.");
        return false;

    }
    return true;
}

template<typename T>
void _GetAxisRadiusHalfHeight(const T& shape, const GfTransform& tr, 
    const SdfPath& primPath, UsdPhysicsAxis* outAxis, float* outRadius, 
    float* outHalfHeight)
{
    // Get shape parameters
    {
        double radiusAttr;
        shape.GetRadiusAttr().Get(&radiusAttr);
        double heightAttr;
        shape.GetHeightAttr().Get(&heightAttr);
        *outRadius = (float)radiusAttr;
        *outHalfHeight = (float)heightAttr * 0.5f;

        TfToken capAxis;
        if (shape.GetAxisAttr())
        {
            shape.GetAxisAttr().Get(&capAxis);
            if (capAxis == UsdPhysicsTokens.Get()->y)
            {
                *outAxis = UsdPhysicsAxis::Y;
            }
            else if (capAxis == UsdPhysicsTokens.Get()->z)
            {
                *outAxis = UsdPhysicsAxis::Z;
            }
        }
    }

    {
        // scale the radius and height based on the given axis token
        const GfVec3d sc = tr.GetScale();        
        if (*outAxis == UsdPhysicsAxis::X)
        {
            *outHalfHeight *= float(sc[0]);
            *outRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
        }
        else if (*outAxis == UsdPhysicsAxis::Y)
        {
            *outHalfHeight *= float(sc[1]);
            *outRadius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
        }
        else
        {
            *outHalfHeight *= float(sc[2]);
            *outRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
        }
    }
}

template<typename T>
void _GetAxisTopBottomRadiusHalfHeight(const T& shape, const GfTransform& tr, 
    const SdfPath& primPath, UsdPhysicsAxis* outAxis, float* outTopRadius, 
    float* outBottomRadius, float* outHalfHeight)
{
    // Get shape parameters
    {
        double topRadiusAttr;
        shape.GetRadiusTopAttr().Get(&topRadiusAttr);
        double bottomRadiusAttr;
        shape.GetRadiusBottomAttr().Get(&bottomRadiusAttr);
        double heightAttr;
        shape.GetHeightAttr().Get(&heightAttr);
        *outTopRadius = (float)topRadiusAttr;
        *outBottomRadius = (float)bottomRadiusAttr;
        *outHalfHeight = (float)heightAttr * 0.5f;

        TfToken capAxis;
        if (shape.GetAxisAttr())
        {
            shape.GetAxisAttr().Get(&capAxis);
            if (capAxis == UsdPhysicsTokens.Get()->y)
            {
                *outAxis = UsdPhysicsAxis::Y;
            }
            else if (capAxis == UsdPhysicsTokens.Get()->z)
            {
                *outAxis = UsdPhysicsAxis::Z;
            }
        }
    }

    {
        // scale the radius and height based on the given axis token
        const GfVec3d sc = tr.GetScale();        
        if (*outAxis == UsdPhysicsAxis::X)
        {
            *outHalfHeight *= float(sc[0]);
            *outTopRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
            *outBottomRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
        }
        else if (*outAxis == UsdPhysicsAxis::Y)
        {
            *outHalfHeight *= float(sc[1]);
            *outTopRadius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
            *outBottomRadius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
        }
        else
        {
            *outHalfHeight *= float(sc[2]);
            *outTopRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
            *outBottomRadius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
        }
    }
}

// Parse cylinder shape desc
bool _ParseCylinderShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCylinderShapeDesc* outCylinderShapeDesc)
{
    if (outCylinderShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCylinder shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            _GetAxisRadiusHalfHeight(shape, tr, usdPrim.GetPrimPath(), &axis,
                &radius, &halfHeight);

            outCylinderShapeDesc->radius = fabsf(radius);
            outCylinderShapeDesc->axis = axis;
            outCylinderShapeDesc->halfHeight = fabsf(halfHeight);
            outCylinderShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outCylinderShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCylinder.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCylinderShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse capsule shape desc
bool _ParseCapsuleShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCapsuleShapeDesc* outCapsuleShapeDesc)
{
    if (outCapsuleShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCapsule shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            _GetAxisRadiusHalfHeight(shape, tr, usdPrim.GetPrimPath(), &axis, 
                &radius, &halfHeight);

            outCapsuleShapeDesc->radius = fabsf(radius);
            outCapsuleShapeDesc->axis = axis;
            outCapsuleShapeDesc->halfHeight = fabsf(halfHeight);
            outCapsuleShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outCapsuleShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCapsule.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCapsuleShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse capsule1 shape desc
bool _ParseCapsule1ShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCapsule1ShapeDesc* outCapsule1ShapeDesc)
{
    if (outCapsule1ShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCapsule_1 shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float topRadius = 1.0f;
            float bottomRadius = 1.0f;
            float halfHeight = 1.0f;
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            _GetAxisTopBottomRadiusHalfHeight(shape, tr, usdPrim.GetPrimPath(), 
                &axis, &topRadius, &bottomRadius, &halfHeight);

            outCapsule1ShapeDesc->topRadius = fabsf(topRadius);
            outCapsule1ShapeDesc->bottomRadius = fabsf(bottomRadius);
            outCapsule1ShapeDesc->axis = axis;
            outCapsule1ShapeDesc->halfHeight = fabsf(halfHeight);
            outCapsule1ShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outCapsule1ShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCapsule_1.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCapsule1ShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse cylinder1 shape desc
bool _ParseCylinder1ShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCylinder1ShapeDesc* outCylinder1ShapeDesc)
{
    if (outCylinder1ShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCylinder_1 shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float topRadius = 1.0f;
            float bottomRadius = 1.0f;
            float halfHeight = 1.0f;
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            _GetAxisTopBottomRadiusHalfHeight(shape, tr, usdPrim.GetPrimPath(), 
                &axis, &topRadius, &bottomRadius, &halfHeight);

            outCylinder1ShapeDesc->topRadius = fabsf(topRadius);
            outCylinder1ShapeDesc->bottomRadius = fabsf(bottomRadius);
            outCylinder1ShapeDesc->axis = axis;
            outCylinder1ShapeDesc->halfHeight = fabsf(halfHeight);
            outCylinder1ShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outCylinder1ShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCylinder_1.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCylinder1ShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse cone shape desc
bool _ParseConeShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsConeShapeDesc* outConeShapeDesc)
{
    if (outConeShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCone shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            _GetAxisRadiusHalfHeight(shape, tr, usdPrim.GetPrimPath(), &axis, 
                &radius, &halfHeight);

            outConeShapeDesc->radius = fabsf(radius);
            outConeShapeDesc->axis = axis;
            outConeShapeDesc->halfHeight = fabsf(halfHeight);
            outConeShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outConeShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomCone.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsConeShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse mesh shape desc
bool _ParseMeshShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsMeshShapeDesc* outMeshShapeDesc)
{
    if (outMeshShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomMesh shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            const GfVec3d sc = tr.GetScale();
            outMeshShapeDesc->meshScale = GfVec3f(sc);

            // Get approximation type
            outMeshShapeDesc->approximation = UsdPhysicsTokens.Get()->none;
            UsdPhysicsMeshCollisionAPI physicsColMeshAPI(usdPrim);
            if (physicsColMeshAPI)
            {
                physicsColMeshAPI.GetApproximationAttr().Get(
                    &outMeshShapeDesc->approximation);
            }

            shape.GetDoubleSidedAttr().Get(&outMeshShapeDesc->doubleSided);

            // Gather materials through subsets
            const std::vector<UsdGeomSubset> subsets =
                UsdGeomSubset::GetGeomSubsets(shape, UsdGeomTokens->face);
            if (!subsets.empty())
            {
                for (const UsdGeomSubset& subset : subsets)
                {
                    const SdfPath material = _GetMaterialBinding(
                        subset.GetPrim());
                    if (material != SdfPath())
                    {
                        const UsdPrim materialPrim = 
                            usdPrim.GetStage()->GetPrimAtPath(material);
                        if (materialPrim && 
                            materialPrim.HasAPI<UsdPhysicsMaterialAPI>())
                        {
                            outMeshShapeDesc->materials.push_back(material);
                        }
                    }
                }
            }

            outMeshShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outMeshShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomMesh.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsMeshShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse plane shape desc
bool _ParsePlaneShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsPlaneShapeDesc* outPlaneShapeDesc)
{
    if (outPlaneShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomPlane shape(usdPrim);
        if (shape)
        {
            UsdPhysicsAxis axis = UsdPhysicsAxis::X;

            TfToken tfAxis;
            shape.GetAxisAttr().Get(&tfAxis);
            if (tfAxis == UsdPhysicsTokens.Get()->y)
            {
                axis = UsdPhysicsAxis::Y;
            }
            else if (tfAxis == UsdPhysicsTokens.Get()->z)
            {
                axis = UsdPhysicsAxis::Z;
            }

            outPlaneShapeDesc->axis = axis;
            outPlaneShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outPlaneShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomPlane.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsPlaneShapeDesc is not valid.");
        return false;

    }
    return true;
}

// Parse sphere points shape desc
bool _ParseSpherePointsShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsSpherePointsShapeDesc* outSpherePointsShapeDesc)
{
    if (outSpherePointsShapeDesc && collisionAPI)
    {
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomPoints shape(usdPrim);
        if (shape)
        {
            const GfTransform tr(
                shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            VtArray<float> widths;
            VtArray<GfVec3f> positions;
            shape.GetWidthsAttr().Get(&widths);
            if (widths.size())
            {
                shape.GetPointsAttr().Get(&positions);
                if (positions.size() == widths.size())
                {
                    float sphereScale = 1.0f;
                    {
                        const GfVec3d sc = tr.GetScale();

                        sphereScale = fmaxf(fmaxf(fabsf(float(sc[1])), 
                                                  fabsf(float(sc[0]))),
                                            fabsf(float(sc[2])));
                    }

                    const size_t scount = positions.size();
                    outSpherePointsShapeDesc->spherePoints.resize(scount);
                    for (size_t i = 0; i < scount; i++)
                    {
                        outSpherePointsShapeDesc->spherePoints[i].radius =
                            sphereScale * widths[i] * 0.5f;
                        outSpherePointsShapeDesc->spherePoints[i].center =
                            positions[i];
                    }
                }
                else
                {
                    outSpherePointsShapeDesc->isValid = false;
                }
            }
            else
            {
                outSpherePointsShapeDesc->isValid = false;
            }

            outSpherePointsShapeDesc->primPath =
                collisionAPI.GetPrim().GetPrimPath();

            _FinalizeCollisionDesc(collisionAPI, outSpherePointsShapeDesc);
        }
        else
        {
            TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI is not applied "
                             "to a UsdGeomPoints.");
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsSpherePointsShapeDesc is not valid.");
        return false;
    }
    return true;
}

// Parse custom shape desc
bool _ParseCustomShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI,
    UsdPhysicsCustomShapeDesc* outCustomShapeDesc)
{
    if (outCustomShapeDesc && collisionAPI)
    {

        outCustomShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

        _FinalizeCollisionDesc(collisionAPI, outCustomShapeDesc);
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionAPI or "
                         "UsdPhysicsCustomShapeDesc is not valid.");
        return false;
    }
    return true;
}

// Parse collision group desc
bool _ParseCollisionGroupDesc(const UsdPhysicsCollisionGroup& collisionGroup,
    UsdPhysicsCollisionGroupDesc* outCollisionGroupDesc)
{
    if (collisionGroup && outCollisionGroupDesc)
    {
        const UsdRelationship rel = collisionGroup.GetFilteredGroupsRel();
        if (rel)
        {
            rel.GetTargets(&outCollisionGroupDesc->filteredGroups);
        }

        collisionGroup.GetInvertFilteredGroupsAttr().Get(
            &outCollisionGroupDesc->invertFilteredGroups);
        collisionGroup.GetMergeGroupNameAttr().Get(
            &outCollisionGroupDesc->mergeGroupName);

        outCollisionGroupDesc->primPath = collisionGroup.GetPrim().GetPrimPath();
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsCollisionGroup or "
                         "UsdPhysicsCollisionGroupDesc is not valid.");
        return false;
    }

    return true;
}

// Get joint rel target
SdfPath _GetRel(const UsdRelationship& ref, const UsdPrim& jointPrim)
{
    SdfPathVector targets;
    ref.GetTargets(&targets);

    if (targets.size() == 0)
    {
        return SdfPath();
    }
    return targets.at(0);
}

// Get body for a given path, the body can be on a parent prim
UsdPrim _GetBodyPrim(UsdStageWeakPtr stage, const SdfPath& relPath, 
                    UsdPrim& relPrim)
{
    UsdPrim parent = stage->GetPrimAtPath(relPath);
    relPrim = parent;
    UsdPrim collisionPrim = UsdPrim();
    while (parent && parent != stage->GetPseudoRoot())
    {
        if (parent.HasAPI<UsdPhysicsRigidBodyAPI>())
        {
            return parent;
        }
        if (parent.HasAPI<UsdPhysicsCollisionAPI>())
        {
            collisionPrim = parent;
        }
        parent = parent.GetParent();
    }

    return collisionPrim;
}

// Get joint local pose base on provided body rel path
SdfPath _GetLocalPose(UsdStageWeakPtr stage, const SdfPath& relPath, GfVec3f* outT,
    GfQuatf* outQ)
{
    UsdPrim relPrim;
    const UsdPrim body = _GetBodyPrim(stage, relPath, relPrim);

    // get scale and apply it into localPositions vectors
    const UsdGeomXformable xform(relPrim);
    const GfMatrix4d worldRel = relPrim ? xform.ComputeLocalToWorldTransform(
        UsdTimeCode::Default()) : GfMatrix4d(1.0);

    // we need to apply scale to the localPose, the scale comes from the rigid 
    // body
    GfVec3f sc;
    // if we had a rel not to rigid body, we need to recompute the localPose
    if (relPrim != body)
    {
        GfMatrix4d localAnchor;
        localAnchor.SetIdentity();
        localAnchor.SetTranslate(GfVec3d(*outT));
        localAnchor.SetRotateOnly(GfQuatd(*outQ));

        GfMatrix4d bodyMat;
        if (body)
        {
            bodyMat = UsdGeomXformable(body).ComputeLocalToWorldTransform(
                UsdTimeCode::Default());
        }
        else
        {
            bodyMat.SetIdentity();
        }

        const GfMatrix4d worldAnchor = localAnchor * worldRel;
        GfMatrix4d bodyLocalAnchor = worldAnchor * bodyMat.GetInverse();
        bodyLocalAnchor = bodyLocalAnchor.RemoveScaleShear();

        *outT = GfVec3f(bodyLocalAnchor.ExtractTranslation());
        *outQ = GfQuatf(bodyLocalAnchor.ExtractRotationQuat());
        outQ->Normalize();

        const GfTransform tr(bodyMat);
        sc = GfVec3f(tr.GetScale());
    }
    else
    {
        const GfTransform tr(worldRel);
        sc = GfVec3f(tr.GetScale());
    }

    // apply the scale, this is not obvious, but in physics there is no scale, 
    // so we need to apply it before its send to physics
    for (int i = 0; i < 3; i++)
    {
        (*outT)[i] *= sc[i];
    }

    return body ? body.GetPrimPath() : SdfPath();
}

// Finalize joint desc
void _FinalizeJoint(const UsdPhysicsJoint& jointPrim, 
                   UsdPhysicsJointDesc* outJointDesc)
{
    // joint bodies anchor point local transforms    
    GfVec3f t0(0.f);
    GfVec3f t1(0.f);
    GfQuatf q0(1.f);
    GfQuatf q1(1.f);
    jointPrim.GetLocalPos0Attr().Get(&t0);
    jointPrim.GetLocalRot0Attr().Get(&q0);
    jointPrim.GetLocalPos1Attr().Get(&t1);
    jointPrim.GetLocalRot1Attr().Get(&q1);

    q0.Normalize();
    q1.Normalize();

    UsdStageWeakPtr stage = jointPrim.GetPrim().GetStage();

    // get scale and apply it into localPositions vectors
    if (outJointDesc->rel0 != SdfPath())
    {
        outJointDesc->body0 = _GetLocalPose(stage, outJointDesc->rel0, &t0, &q0);
    }

    if (outJointDesc->rel1 != SdfPath())
    {
        outJointDesc->body1 = _GetLocalPose(stage, outJointDesc->rel1, &t1, &q1);
    }

    outJointDesc->localPose0Position = t0;
    outJointDesc->localPose0Orientation = q0;
    outJointDesc->localPose1Position = t1;
    outJointDesc->localPose1Orientation = q1;
}

// Parse common joint parameters
bool _ParseCommonJointDesc(const UsdPhysicsJoint& jointPrim, 
                          UsdPhysicsJointDesc* outJointDesc)
{
    const UsdPrim prim = jointPrim.GetPrim();

    outJointDesc->primPath = prim.GetPrimPath();

    // parse the joint common parameters
    jointPrim.GetJointEnabledAttr().Get(&outJointDesc->jointEnabled);
    jointPrim.GetCollisionEnabledAttr().Get(&outJointDesc->collisionEnabled);
    jointPrim.GetBreakForceAttr().Get(&outJointDesc->breakForce);
    jointPrim.GetBreakTorqueAttr().Get(&outJointDesc->breakTorque);
    jointPrim.GetExcludeFromArticulationAttr().Get(
        &outJointDesc->excludeFromArticulation);

    outJointDesc->rel0 = _GetRel(jointPrim.GetBody0Rel(), prim);
    outJointDesc->rel1 = _GetRel(jointPrim.GetBody1Rel(), prim);

    _FinalizeJoint(jointPrim, outJointDesc);

    return true;
}

// Parse distance joint desc
bool _ParseDistanceJointDesc(const UsdPhysicsDistanceJoint& distanceJoint,
    UsdPhysicsDistanceJointDesc* outDistanceJointDesc)
{
    if (outDistanceJointDesc && distanceJoint)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(distanceJoint, outDistanceJointDesc))
        {
            return false;
        }

        outDistanceJointDesc->maxEnabled = false;
        outDistanceJointDesc->minEnabled = false;
        distanceJoint.GetMinDistanceAttr().Get(
            &outDistanceJointDesc->limit.minDist);
        distanceJoint.GetMaxDistanceAttr().Get(
            &outDistanceJointDesc->limit.maxDist);

        if (outDistanceJointDesc->limit.minDist >= 0.0f)
        {
            outDistanceJointDesc->minEnabled = true;
        }
        if (outDistanceJointDesc->limit.maxDist >= 0.0f)
        {
            outDistanceJointDesc->maxEnabled = true;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsDistanceJoint or "
                         "UsdPhysicsDistanceJointDesc is not valid.");
        return false;
    }

    return true;
}

// Parse joint drive
bool _ParseDrive(const UsdPhysicsDriveAPI& drive, 
                UsdPhysicsJointDrive* outJointDrive)
{
    if (drive && outJointDrive)
    {
        drive.GetTargetPositionAttr().Get(&outJointDrive->targetPosition);
        drive.GetTargetVelocityAttr().Get(&outJointDrive->targetVelocity);
        drive.GetMaxForceAttr().Get(&outJointDrive->forceLimit);

        drive.GetDampingAttr().Get(&outJointDrive->damping);
        drive.GetStiffnessAttr().Get(&outJointDrive->stiffness);

        TfToken typeToken;
        drive.GetTypeAttr().Get(&typeToken);
        if (typeToken == UsdPhysicsTokens->acceleration)
        {
            outJointDrive->acceleration = true;
        }
        outJointDrive->enabled = true;
    }
    else
    {
        TF_CODING_ERROR(
            "Provided UsdPhysicsDriveAPI or UsdPhysicsJointDrive is not valid.");
        return false;
    }

    return true;
}

// Parse fixed joint desc
bool _ParseFixedJointDesc(const UsdPhysicsFixedJoint& fixedJoint,
    UsdPhysicsFixedJointDesc* outFixedJointDesc)
{
    if (outFixedJointDesc && fixedJoint)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(fixedJoint, outFixedJointDesc))
        {
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsFixedJoint or "
                         "UsdPhysicsFixedJointDesc is not valid.");
        return false;
    }

    return true;
}

// Parse joint limit
bool _ParseLimit(const UsdPhysicsLimitAPI& limit, 
                UsdPhysicsJointLimit* outJointLimit)
{
    if (limit && outJointLimit)
    {
        limit.GetLowAttr().Get(&outJointLimit->lower);
        limit.GetHighAttr().Get(&outJointLimit->upper);
        if ((isfinite(outJointLimit->lower) &&
            outJointLimit->lower > -usdPhysicsSentinelLimit) ||
            (isfinite(outJointLimit->upper) &&
                outJointLimit->upper < usdPhysicsSentinelLimit))
        {
                outJointLimit->enabled = true;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsLimitAPI or "
                         "UsdPhysicsJointLimit is not valid.");
        return false;
    }

    return true;
}

// Parse generic D6 joint desc
bool _ParseD6JointDesc(const UsdPhysicsJoint& jointPrim, 
                      UsdPhysicsD6JointDesc* outJointDesc)
{
    if (outJointDesc && jointPrim)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(jointPrim, outJointDesc))
        {
            return false;
        }

        // D6 joint        
        const std::array<
            std::pair<UsdPhysicsJointDOF, TfToken>, 7> axisVector =
        {
            std::make_pair(UsdPhysicsJointDOF::Distance, 
                           UsdPhysicsTokens->distance),
            std::make_pair(UsdPhysicsJointDOF::TransX, 
                           UsdPhysicsTokens->transX),
            std::make_pair(UsdPhysicsJointDOF::TransY, 
                           UsdPhysicsTokens->transY),
            std::make_pair(UsdPhysicsJointDOF::TransZ, 
                           UsdPhysicsTokens->transZ),
            std::make_pair(UsdPhysicsJointDOF::RotX, UsdPhysicsTokens->rotX),
            std::make_pair(UsdPhysicsJointDOF::RotY, UsdPhysicsTokens->rotY),
            std::make_pair(UsdPhysicsJointDOF::RotZ, UsdPhysicsTokens->rotZ)
        };

        for (size_t i = 0; i < axisVector.size(); i++)
        {
            const TfToken& axisToken = axisVector[i].second;

            const UsdPhysicsLimitAPI limitAPI = 
                UsdPhysicsLimitAPI::Get(jointPrim.GetPrim(), axisToken);
            if (limitAPI)
            {
                UsdPhysicsJointLimit limit;
                if (_ParseLimit(limitAPI, &limit))
                {
                    outJointDesc->jointLimits.push_back(
                        std::make_pair(axisVector[i].first, limit));
                }
            }

            const UsdPhysicsDriveAPI driveAPI = 
                UsdPhysicsDriveAPI::Get(jointPrim.GetPrim(), axisToken);
            if (driveAPI)
            {
                UsdPhysicsJointDrive drive;
                if (_ParseDrive(driveAPI, &drive))
                {
                    outJointDesc->jointDrives.push_back(
                        std::make_pair(axisVector[i].first, drive));
                }
            }
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsJoint or UsdPhysicsJointDesc is "
                         "not valid.");
        return false;
    }

    return true;
}

// Parse custom joint desc
bool _ParseCustomJointDesc(const UsdPhysicsJoint& jointPrim,
    UsdPhysicsCustomJointDesc* outCustomJointDesc)
{
    if (outCustomJointDesc && jointPrim)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(jointPrim, outCustomJointDesc))
        {
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsJoint or UsdPhysicsJointDesc is "
                         "not valid.");
        return false;
    }

    return true;
}

// Parse rigid body material desc
bool _ParseRigidBodyMaterialDesc(const UsdPhysicsMaterialAPI& usdMaterial,
    UsdPhysicsRigidBodyMaterialDesc* outRbMaterialDesc)
{
    if (outRbMaterialDesc && usdMaterial)
    {
        usdMaterial.GetDynamicFrictionAttr().Get(
            &outRbMaterialDesc->dynamicFriction);
        usdMaterial.GetStaticFrictionAttr().Get(
            &outRbMaterialDesc->staticFriction);

        usdMaterial.GetRestitutionAttr().Get(&outRbMaterialDesc->restitution);

        usdMaterial.GetDensityAttr().Get(&outRbMaterialDesc->density);

        outRbMaterialDesc->primPath = usdMaterial.GetPrim().GetPrimPath();
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsMaterialAPI or "
                         "UsdPhysicsRigidBodyMaterialDesc is not valid.");
        return false;

    }
    return true;
}

// Parse linear drive
bool _ParseLinearDrive(const UsdPrim& usdPrim, UsdPhysicsJointDrive* outDst)
{
    outDst->enabled = false;
    const UsdPhysicsDriveAPI driveAPI = 
        UsdPhysicsDriveAPI::Get(usdPrim, UsdPhysicsTokens->linear);
    if (driveAPI)
    {
        return _ParseDrive(driveAPI, outDst);
    }

    return true;
}

// Parse prismatic joint desc
bool _ParsePrismaticJointDesc(const UsdPhysicsPrismaticJoint& prismaticJoint,
    UsdPhysicsPrismaticJointDesc* outPrismaticJointDesc)
{
    if (outPrismaticJointDesc && prismaticJoint)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(prismaticJoint, outPrismaticJointDesc))
        {
            return false;
        }

        UsdPhysicsAxis jointAxis = UsdPhysicsAxis::X;
        TfToken axis = UsdPhysicsTokens->x;
        prismaticJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = UsdPhysicsAxis::Y;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = UsdPhysicsAxis::Z;
        outPrismaticJointDesc->axis = jointAxis;

        outPrismaticJointDesc->limit.enabled = false;
        prismaticJoint.GetLowerLimitAttr().Get(
            &outPrismaticJointDesc->limit.lower);
        prismaticJoint.GetUpperLimitAttr().Get(
            &outPrismaticJointDesc->limit.upper);
        if ((isfinite(outPrismaticJointDesc->limit.lower) &&
            (outPrismaticJointDesc->limit.lower > -usdPhysicsSentinelLimit)) ||
            (isfinite(outPrismaticJointDesc->limit.upper) &&
                (outPrismaticJointDesc->limit.upper < usdPhysicsSentinelLimit)))
        {
            outPrismaticJointDesc->limit.enabled = true;
        }

        if (!_ParseLinearDrive(prismaticJoint.GetPrim(), &outPrismaticJointDesc->drive))
        {
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsPrismaticJoint or "
                         "UsdPhysicsPrismaticJointDesc is not valid.");
        return false;
    }

    return true;
}

// Parse angular drive
bool _ParseAngularDrive(const UsdPrim& usdPrim, UsdPhysicsJointDrive* outDst)
{
    outDst->enabled = false;
    const UsdPhysicsDriveAPI driveAPI = UsdPhysicsDriveAPI::Get(usdPrim,
        UsdPhysicsTokens->angular);
    if (driveAPI)
    {
        return _ParseDrive(driveAPI, outDst);
    }

    return true;
}


// Parse revolute joint desc
bool _ParseRevoluteJointDesc(const UsdPhysicsRevoluteJoint& revoluteJoint,
    UsdPhysicsRevoluteJointDesc* outRevoluteJointDesc)
{
    if (outRevoluteJointDesc && revoluteJoint)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(revoluteJoint, outRevoluteJointDesc))
        {
            return false;
        }

        UsdPhysicsAxis jointAxis = UsdPhysicsAxis::X;
        TfToken axis = UsdPhysicsTokens->x;
        revoluteJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = UsdPhysicsAxis::Y;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = UsdPhysicsAxis::Z;
        outRevoluteJointDesc->axis = jointAxis;

        outRevoluteJointDesc->limit.enabled = false;


        revoluteJoint.GetLowerLimitAttr().Get(&outRevoluteJointDesc->limit.lower);
        revoluteJoint.GetUpperLimitAttr().Get(&outRevoluteJointDesc->limit.upper);
        if (isfinite(outRevoluteJointDesc->limit.lower) &&
            isfinite(outRevoluteJointDesc->limit.upper)
            && outRevoluteJointDesc->limit.lower > -usdPhysicsSentinelLimit &&
            outRevoluteJointDesc->limit.upper < usdPhysicsSentinelLimit)
        {
            outRevoluteJointDesc->limit.enabled = true;
        }

        if (!_ParseAngularDrive(revoluteJoint.GetPrim(), &outRevoluteJointDesc->drive))
        {
            return false;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsJoint or UsdPhysicsJointDesc is "
                         "not valid.");
        return false;
    }

    return true;
}

// Compute the rigid body transformation and store to the desc
void _GetRigidBodyTransformation(const UsdPrim& bodyPrim, 
                                UsdPhysicsRigidBodyDesc* outDesc)
{
    const GfMatrix4d mat =
        UsdGeomXformable(bodyPrim).ComputeLocalToWorldTransform(
            UsdTimeCode::Default());
    const GfTransform tr(mat);
    const GfVec3d pos = tr.GetTranslation();
    const GfQuatd rot = tr.GetRotation().GetQuat();
    const GfVec3d sc = tr.GetScale();

    outDesc->position = GfVec3f(pos);
    outDesc->rotation = GfQuatf(rot);
    outDesc->scale = GfVec3f(sc);
}

// Parse rigid body desc
bool _ParseRigidBodyDesc(const UsdPhysicsRigidBodyAPI& rigidBodyAPI,
    UsdPhysicsRigidBodyDesc* outRigidBodyDesc)
{
    if (outRigidBodyDesc && rigidBodyAPI)
    {
        // transformation
        _GetRigidBodyTransformation(rigidBodyAPI.GetPrim(), outRigidBodyDesc);

        // filteredPairs
        _ParseFilteredPairs(rigidBodyAPI.GetPrim(), 
                           &outRigidBodyDesc->filteredCollisions);

        // velocity
        rigidBodyAPI.GetVelocityAttr().Get(&outRigidBodyDesc->linearVelocity);
        rigidBodyAPI.GetAngularVelocityAttr().Get(
            &outRigidBodyDesc->angularVelocity);

        // rigid body flags
        rigidBodyAPI.GetRigidBodyEnabledAttr().Get(
            &outRigidBodyDesc->rigidBodyEnabled);
        rigidBodyAPI.GetKinematicEnabledAttr().Get(
            &outRigidBodyDesc->kinematicBody);
        rigidBodyAPI.GetStartsAsleepAttr().Get(&outRigidBodyDesc->startsAsleep);

        // simulation owner
        const UsdRelationship ownerRel = rigidBodyAPI.GetSimulationOwnerRel();
        if (ownerRel)
        {
            SdfPathVector owners;
            ownerRel.GetTargets(&owners);
            if (!owners.empty())
            {
                outRigidBodyDesc->simulationOwners = owners;
            }
        }
        outRigidBodyDesc->primPath = rigidBodyAPI.GetPrim().GetPrimPath();
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsRigidBodyAPI or "
                         "UsdPhysicsRigidBodyDesc is not valid.");
        return false;
    }
    return true;
}

// Parse spherical joint desc
bool _ParseSphericalJointDesc(const UsdPhysicsSphericalJoint& sphericalJoint,
    UsdPhysicsSphericalJointDesc* outSphericalJointDesc)
{
    if (outSphericalJointDesc && sphericalJoint)
    {
        // parse the joint common parameters
        if (!_ParseCommonJointDesc(sphericalJoint, outSphericalJointDesc))
        {
            return false;
        }

        UsdPhysicsAxis jointAxis = UsdPhysicsAxis::X;
        TfToken axis = UsdPhysicsTokens->x;
        sphericalJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = UsdPhysicsAxis::Y;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = UsdPhysicsAxis::Z;
        outSphericalJointDesc->axis = jointAxis;

        outSphericalJointDesc->limit.enabled = false;
        sphericalJoint.GetConeAngle0LimitAttr().Get(
            &outSphericalJointDesc->limit.angle0);
        sphericalJoint.GetConeAngle1LimitAttr().Get(
            &outSphericalJointDesc->limit.angle1);

        if (isfinite(outSphericalJointDesc->limit.angle0) &&
            isfinite(outSphericalJointDesc->limit.angle1) &&
            outSphericalJointDesc->limit.angle0 >= 0.0 &&
            outSphericalJointDesc->limit.angle1 >= 0.0)
        {
            outSphericalJointDesc->limit.enabled = true;
        }
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsSphericalJoint or "
                         "UsdPhysicsSphericalJointDesc is not valid.");
        return false;
    }

    return true;
}

// Parse scene desc
bool _ParseSceneDesc(const UsdPhysicsScene& scene, 
                    UsdPhysicsSceneDesc* outSceneDesc)
{
    if (outSceneDesc && scene)
    {
        UsdStageWeakPtr stage = scene.GetPrim().GetStage();

        GfVec3f gravityDirection;
        scene.GetGravityDirectionAttr().Get(&gravityDirection);
        if (gravityDirection == GfVec3f(0.0f))
        {
            TfToken upAxis = UsdGeomGetStageUpAxis(stage);
            if (upAxis == UsdGeomTokens.Get()->x)
                gravityDirection = GfVec3f(-1.0f, 0.0f, 0.0f);
            else if (upAxis == UsdGeomTokens.Get()->y)
                gravityDirection = GfVec3f(0.0f, -1.0f, 0.0f);
            else
                gravityDirection = GfVec3f(0.0f, 0.0f, -1.0f);
        }
        else
        {
            gravityDirection.Normalize();
        }

        float gravityMagnitude;
        scene.GetGravityMagnitudeAttr().Get(&gravityMagnitude);
        if (gravityMagnitude < -infSentinel)
        {
            float metersPerUnit = (float)UsdGeomGetStageMetersPerUnit(stage);
            gravityMagnitude = defaultGravity / metersPerUnit;
        }

        outSceneDesc->gravityMagnitude = gravityMagnitude;
        outSceneDesc->gravityDirection = gravityDirection;
        outSceneDesc->primPath = scene.GetPrim().GetPrimPath();
    }
    else
    {
        TF_CODING_ERROR("Provided UsdPhysicsScene or UsdPhysicsSceneDesc is "
                         "not valid.");
        return false;
    }
    return true;
}

// Helper flags to store APIs
enum class _SchemaAPIFlag
{
    ArticulationRootAPI = 1 << 0,
    CollisionAPI = 1 << 1,
    RigidBodyAPI = 1 << 2,
    MaterialAPI = 1 << 3
};

using RigidBodyMap = std::map<SdfPath, UsdPhysicsRigidBodyDesc*>;

// Check if body is dynamic accessing parsed data
bool _IsDynamicBody(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap, 
                   bool* outPhysicsAPIFound)
{
    RigidBodyMap::const_iterator it = bodyMap.find(usdPrim.GetPrimPath());
    if (it != bodyMap.end())
    {
        {
            bool isAPISchemaEnabled = it->second->rigidBodyEnabled;

            // Prim is dynamic body off PhysicsAPI is present and enabled
            *outPhysicsAPIFound = true;
            return isAPISchemaEnabled;
        }
    }

    *outPhysicsAPIFound = false;
    return false;
}

// Check if prim has a dynamic body as a parent
bool _HasDynamicBodyParent(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap,
    UsdPrim* outBodyPrimPath)
{
    bool physicsAPIFound = false;
    UsdPrim parent = usdPrim;
    while (parent != usdPrim.GetStage()->GetPseudoRoot())
    {
        if (_IsDynamicBody(parent, bodyMap, &physicsAPIFound))
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

// Helper function to process descriptors in parallel 
template <typename DescType, typename UsdType>
void _ProcessPhysicsPrims(const std::vector<UsdPrim>& physicsPrims,
    std::vector<DescType>& physicsDesc,
    std::function<bool(const UsdType& prim, DescType* desc)> processDescFn)
{
    if (!physicsPrims.empty())
    {
        const size_t numPrims = physicsPrims.size();
        physicsDesc.resize(numPrims);

        const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
        {
            for (size_t i = beginIdx; i < endIdx; i++)
            {
                const UsdType prim(physicsPrims[i]);
                const bool ret = processDescFn(prim, &physicsDesc[i]);
                if (!ret)
                {
                    physicsDesc[i].isValid = false;
                }
            }
        };

        const size_t numPrimPerBatch = 10;
        WorkParallelForN(numPrims, workLambda, numPrimPerBatch);
    }
}

// Helper function to call report function
template <typename DescType>
void _CallReportFn(
    UsdPhysicsObjectType descType, 
    const std::vector<UsdPrim>& physicsPrims, 
    const std::vector<DescType>& physicsDesc, UsdPhysicsReportFn reportFn, 
    SdfPathVector& primPathsVector, const VtValue& userData)
{
    if (!physicsPrims.empty() && physicsPrims.size() == physicsDesc.size())
    {
        primPathsVector.resize(physicsPrims.size());
        for (size_t i = 0; i < physicsPrims.size(); i++)
        {
            primPathsVector[i] = physicsPrims[i].GetPrimPath();
        }
        reportFn(descType, TfMakeConstSpan(primPathsVector),
                TfSpan<const UsdPhysicsObjectDesc>(
                    physicsDesc.data(), physicsDesc.size()), userData);
    }
}


void _CheckRigidBodySimulationOwner(
    std::vector<UsdPrim>& rigidBodyPrims, 
    std::vector<UsdPhysicsRigidBodyDesc>& rigidBodyDescs, 
    bool defaultSimulationOwner,     
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet,
    std::unordered_set<SdfPath, SdfPath::Hash>* outReportedBodies)
{
    for (size_t i = rigidBodyDescs.size(); i--;)
    {
        bool ownerFound = false;
        const UsdPhysicsRigidBodyDesc& desc = rigidBodyDescs[i];
        if (desc.isValid)
        {
            if (desc.simulationOwners.empty() && defaultSimulationOwner)
            {
                outReportedBodies->insert(desc.primPath);
                ownerFound = true;
            }
            else
            {
                for (const SdfPath& owner : desc.simulationOwners)
                {
                    if (simulationOwnersSet.find(owner) != 
                        simulationOwnersSet.end())
                    {
                        outReportedBodies->insert(desc.primPath);
                        ownerFound = true;
                        break;
                    }
                }
            }
        }
        if (!ownerFound)
        {
            rigidBodyDescs[i] = rigidBodyDescs.back();
            rigidBodyDescs.pop_back();
            rigidBodyPrims[i] = rigidBodyPrims.back();
            rigidBodyPrims.pop_back();
        }
    }
}

// if collision belongs to a body that we care about include it
// if collision does not belong to a body we care about its not included
// if collision does not have a body set, we check its own simulationOwners
template <typename DescType>
void _CheckCollisionSimulationOwner(std::vector<UsdPrim>& collisionPrims,
    std::vector<DescType>& shapeDesc,
    bool defaultSimulationOwner,
    const std::unordered_set<SdfPath, SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = shapeDesc.size(); i--;)
    {
        bool ownerFound = false;
        const UsdPhysicsShapeDesc& desc = shapeDesc[i];
        if (desc.isValid)
        {
            if (desc.rigidBody != SdfPath() &&
                rigidBodiesSet.find(desc.rigidBody) != rigidBodiesSet.end())
            {
                if (desc.rigidBody != SdfPath() &&
                    rigidBodiesSet.find(desc.rigidBody) != rigidBodiesSet.end())
                {
                    ownerFound = true;
                }
                else
                {
                    if (desc.rigidBody == SdfPath())
                    {
                        if (desc.simulationOwners.empty() && 
                            defaultSimulationOwner)
                        {
                            ownerFound = true;
                        }
                        else
                        {
                            for (const SdfPath& owner : desc.simulationOwners)
                            {
                                if (simulationOwnersSet.find(owner) != 
                                    simulationOwnersSet.end())
                                {
                                    ownerFound = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                ownerFound = true;
            }
            else
            {
                if (desc.rigidBody == SdfPath())
                {
                    if (desc.simulationOwners.empty() && defaultSimulationOwner)
                    {
                        ownerFound = true;
                    }
                    else
                    {
                        for (const SdfPath& owner : desc.simulationOwners)
                        {
                            if (simulationOwnersSet.find(owner) != 
                                simulationOwnersSet.end())
                            {
                                ownerFound = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!ownerFound)
        {
            shapeDesc[i] = shapeDesc.back();
            shapeDesc.pop_back();
            collisionPrims[i] = collisionPrims.back();
            collisionPrims.pop_back();
        }
    }
}

// Both bodies need to have simulation owners valid
template <typename DescType>
void _CheckJointSimulationOwner(std::vector<UsdPrim>& jointPrims,
    std::vector<DescType>& jointDesc,
    bool defaultSimulationOwner, const std::unordered_set<SdfPath,
    SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = jointDesc.size(); i--;)
    {
        const UsdPhysicsJointDesc& desc = jointDesc[i];

        bool ownersValid = false;
        if (desc.isValid)
        {
            if ((desc.body0 == SdfPath() ||
                rigidBodiesSet.find(desc.body0) != rigidBodiesSet.end()) &&
                (desc.body1 == SdfPath() ||
                    rigidBodiesSet.find(desc.body1) != rigidBodiesSet.end()))
            {
                ownersValid = true;
            }
        }

        if (!ownersValid)
        {
            jointDesc[i] = jointDesc.back();
            jointDesc.pop_back();
            jointPrims[i] = jointPrims.back();
            jointPrims.pop_back();
        }
    }
}

// all bodies must have valid owner
void _CheckArticulationSimulationOwner(std::vector<UsdPrim>& articulationPrims,
    std::vector<UsdPhysicsArticulationDesc>& articulationDescs,
    bool defaultSimulationOwner,
    const std::unordered_set<SdfPath, SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = articulationDescs.size(); i--;)
    {
        const UsdPhysicsArticulationDesc& desc = articulationDescs[i];

        bool ownersValid = true;
        if (desc.isValid)
        {
            for (const SdfPath& body : desc.articulatedBodies)
            {
                if (body != SdfPath() && 
                    rigidBodiesSet.find(body) == rigidBodiesSet.end())
                {
                    ownersValid = false;
                    break;
                }
            }
        }

        if (!ownersValid)
        {
            articulationDescs[i] = articulationDescs.back();
            articulationDescs.pop_back();
            articulationPrims[i] = articulationPrims.back();
            articulationPrims.pop_back();
        }
    }
}

// Get body for the usdPrim can be a parent
SdfPath _GetRigidBody(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap)
{
    UsdPrim bodyPrim = UsdPrim();
    if (_HasDynamicBodyParent(usdPrim, bodyMap, &bodyPrim))
    {
        return bodyPrim.GetPrimPath();
    }
    else
    {
        // collision does not have a dynamic body parent, it is considered a 
        // static collision        
        if (bodyPrim == UsdPrim())
        {
            return SdfPath();
        }
        else
        {
            return bodyPrim.GetPrimPath();
        }
    }
}

// Compute the relative pose between the collision and the rigid body
void _GetCollisionShapeLocalTransfrom(const UsdPrim& collisionPrim,
    const UsdPrim& bodyPrim,
    GfVec3f* outLocalPos,
    GfQuatf* outLocalRot,
    GfVec3f* outLocalScale)
{
    // body transform
    const GfMatrix4d bodyLocalToWorldMatrix =
        UsdGeomXformable(bodyPrim).ComputeLocalToWorldTransform(
            UsdTimeCode::Default());

    // compute the shape rel transform to a body and store it.
    GfVec3f localPos(0.0f);
    if (collisionPrim != bodyPrim)
    {
        const GfMatrix4d collisionLocalToWorldMatrix =
            UsdGeomXformable(collisionPrim).ComputeLocalToWorldTransform(
                UsdTimeCode::Default());

        const GfMatrix4d mat =
            collisionLocalToWorldMatrix * bodyLocalToWorldMatrix.GetInverse();
        GfTransform colLocalTransform(mat);

        localPos = GfVec3f(colLocalTransform.GetTranslation());
        *outLocalRot = GfQuatf(colLocalTransform.GetRotation().GetQuat());
        *outLocalScale = GfVec3f(colLocalTransform.GetScale());
    }
    else
    {
        const GfMatrix4d mat(1.0);

        *outLocalRot = GfQuatf(1.0f);
        *outLocalScale = GfVec3f(1.0f);
    }

    // now apply the body scale to localPos
    // physics does not support scales, so a rigid body scale has to be baked 
    // into the localPos
    const GfTransform tr(bodyLocalToWorldMatrix);
    const GfVec3d sc = tr.GetScale();

    for (int i = 0; i < 3; i++)
    {
        localPos[i] *= (float)sc[i];
    }

    *outLocalPos = localPos;
}

// Finalize the collision, requires the bodies
void _FinalizeCollision(UsdStageWeakPtr stage, 
                       const UsdPhysicsRigidBodyDesc* bodyDesc, 
                       UsdPhysicsShapeDesc* outShapeDesc)
{
    // get shape local pose
    const UsdPrim shapePrim = stage->GetPrimAtPath(outShapeDesc->primPath);
    _GetCollisionShapeLocalTransfrom(
        shapePrim, 
        bodyDesc ? 
            stage->GetPrimAtPath(bodyDesc->primPath) : stage->GetPseudoRoot(),
        &outShapeDesc->localPos, &outShapeDesc->localRot, &outShapeDesc->localScale);

    if (bodyDesc)
    {
        outShapeDesc->rigidBody = bodyDesc->primPath;
    }
}

// Finalize the collision desc, run in parallel
template <typename DescType>
void _FinalizeCollisionDescs(
    UsdGeomXformCache& xfCache, const std::vector<UsdPrim>& physicsPrims, 
    std::vector<DescType>& physicsDesc, const RigidBodyMap& bodyMap,
    const std::map<SdfPath, std::unordered_set<SdfPath, 
    SdfPath::Hash>>& collisionGroups)
{
    const auto workLambda = [physicsPrims, &physicsDesc, bodyMap, 
        collisionGroups]
    (const size_t beginIdx, const size_t endIdx)
    {
        for (size_t i = beginIdx; i < endIdx; i++)
        {
            DescType& colDesc = physicsDesc[i];
            if (colDesc.isValid)
            {
                const UsdPrim prim = physicsPrims[i];
                // get the body
                SdfPath bodyPath = _GetRigidBody(prim, bodyMap);
                // body was found, add collision to the body
                UsdPhysicsRigidBodyDesc* bodyDesc = nullptr;
                if (bodyPath != SdfPath())
                {
                    RigidBodyMap::const_iterator bodyIt = 
                            bodyMap.find(bodyPath);
                    if (bodyIt != bodyMap.end())
                    {
                        bodyDesc = bodyIt->second;
                        bodyDesc->collisions.push_back(colDesc.primPath);
                    }
                }

                // check if collision belongs to collision groups
                for (std::map<SdfPath, 
                    std::unordered_set<SdfPath, 
                        SdfPath::Hash>>::const_iterator it = 
                            collisionGroups.begin();
                    it != collisionGroups.end(); ++it)
                {
                    if (it->second.find(colDesc.primPath) != it->second.end())
                    {
                        colDesc.collisionGroups.push_back(it->first);
                    }
                }

                // finalize the collision, fill up the local transform etc
                _FinalizeCollision(prim.GetStage(), bodyDesc, &colDesc);
            }
        }
    };

    const size_t numPrimPerBatch = 10;
    WorkParallelForN(physicsPrims.size(), workLambda, numPrimPerBatch);
}

struct ArticulationLink
{
    SdfPathVector   children;
    SdfPath         rootJoint;
    uint32_t        weight;
    uint32_t        index;
    bool            hasFixedJoint;
    SdfPathVector   joints;
};

using ArticulationLinkMap = std::map<SdfPath, ArticulationLink>;
using BodyJointMap =
TfHashMap<SdfPath, std::vector<const UsdPhysicsJointDesc*>, SdfPath::Hash>;
using JointMap = std::map<SdfPath, UsdPhysicsJointDesc*>;
using ArticulationMap = std::map<SdfPath, UsdPhysicsArticulationDesc*>;

bool _IsInLinkMap(const SdfPath& path, 
                 const std::vector<ArticulationLinkMap>& linkMaps)
{
    for (size_t i = 0; i < linkMaps.size(); i++)
    {
        ArticulationLinkMap::const_iterator it = linkMaps[i].find(path);
        if (it != linkMaps[i].end())
            return true;
    }

    return false;
}

// Recursive traversal of the hierarchy, adding weight for the links based on number
// of childs and if it belongs to a joint to world
// Each child adds 100 weight, while if link belongs to a MC joint it adds 1000 weight
// if link belong to a joint to world it adds 10000 weight. The weight is used
// if an articulation root has to be decided automatically. 
void _TraverseHierarchy(const UsdStageWeakPtr stage, const SdfPath& linkPath,
    ArticulationLinkMap& articulationLinkMap, const BodyJointMap& bodyJointMap,
    uint32_t& index, SdfPathVector* outLinkOrderVector)
{
    // check if we already parsed this link
    ArticulationLinkMap::const_iterator artIt = 
        articulationLinkMap.find(linkPath);
    if (artIt != articulationLinkMap.end())
        return;

    outLinkOrderVector->push_back(linkPath);

    BodyJointMap::const_iterator bjIt = bodyJointMap.find(linkPath);
    if (bjIt != bodyJointMap.end())
    {
        ArticulationLink& link = articulationLinkMap[linkPath];
        link.weight = 0;
        link.index = index++;
        link.hasFixedJoint = false;
        const std::vector<const UsdPhysicsJointDesc*>& joints = bjIt->second;
        for (size_t i = 0; i < joints.size(); i++)
        {
            const UsdPhysicsJointDesc* desc = joints[i];
            link.joints.push_back(desc->primPath);
            if (desc->body0 == SdfPath() || 
                (bodyJointMap.find(desc->body0) == bodyJointMap.end()) ||
                desc->body1 == SdfPath() || 
                (bodyJointMap.find(desc->body1) == bodyJointMap.end()))
            {
                if (desc->excludeFromArticulation)
                {
                    link.weight += 1000;
                }
                else
                {
                    link.weight += 100000;
                    link.rootJoint = desc->primPath;
                    link.hasFixedJoint = true;
                }
                link.children.push_back(SdfPath());
            }
            else
            {
                if (desc->excludeFromArticulation)
                {
                    link.children.push_back(desc->body0 == 
                                          linkPath ? desc->body1 : desc->body0);
                    link.weight += 1000;
                }
                else
                {
                    link.children.push_back(desc->body0 == 
                                          linkPath ? desc->body1 : desc->body0);
                    link.weight += 100;
                    _TraverseHierarchy(stage, link.children.back(), 
                        articulationLinkMap, bodyJointMap, index, 
                        outLinkOrderVector);
                }
            }
        }
    }
}

// Traversal that marks distances, this is used for finding the center of the graph
void _TraverseChilds(const ArticulationLink& link, 
                    const ArticulationLinkMap& map, uint32_t startIndex, 
                    uint32_t distance, int32_t* pathMatrix)
{
    const size_t mapSize = map.size();
    const uint32_t currentIndex = link.index;
    pathMatrix[startIndex + currentIndex * mapSize] = distance;

    for (size_t i = 0; i < link.children.size(); i++)
    {
        ArticulationLinkMap::const_iterator it = map.find(link.children[i]);
        if (it != map.end())
        {
            const uint32_t childIndex = it->second.index;
            if (pathMatrix[startIndex + childIndex * mapSize] < 0)
            {
                _TraverseChilds(it->second, map, startIndex, distance + 1, 
                               pathMatrix);
            }
        }
    }
}

// Get the center of graph
SdfPath _GetCenterOfGraph(const ArticulationLinkMap& map, 
                         const SdfPathVector& linkOrderVector)
{
    const size_t size = map.size();
    int32_t* pathMatrix = new int32_t[size * size];
    for (size_t i = 0; i < size; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            pathMatrix[i + j * size] = -1;
        }
    }

    for (ArticulationLinkMap::const_reference& ref : map)
    {
        const uint32_t startIndex = ref.second.index;
        uint32_t distance = 0;
        _TraverseChilds(ref.second, map, startIndex, distance, pathMatrix);
    }

    int32_t shortestDistance = INT_MAX;
    size_t numChilds = 0;
    SdfPath primpath = SdfPath();
    for (ArticulationLinkMap::const_reference& ref : map)
    {
        const uint32_t startIndex = ref.second.index;
        int32_t longestPath = 0;
        for (size_t i = 0; i < size; i++)
        {
            if (pathMatrix[startIndex + i * size] > longestPath)
            {
                longestPath = pathMatrix[startIndex + i * size];
            }
        }

        // this needs to be deterministic, get the shortest path if there are
        // more paths with same length, pick the one with more children if there
        // are more with same path and same amount of children, pick the one
        // with lowest hash The lowest hash is not right, this is wrong, it has
        // to be the first link ordered by the traversal
        if (longestPath < shortestDistance)
        {
            shortestDistance = longestPath;
            numChilds = ref.second.children.size();
            primpath = ref.first;
        }
        else if (longestPath == shortestDistance)
        {
            if (numChilds < ref.second.children.size())
            {
                numChilds = ref.second.children.size();
                primpath = ref.first;
            }
            else if (numChilds == ref.second.children.size())
            {
                for (const SdfPath& orderPath : linkOrderVector)
                {
                    if (orderPath == primpath)
                    {
                        break;
                    }
                    else if (orderPath == ref.first)
                    {
                        primpath = ref.first;
                    }
                }
            }
        }
    }

    delete[] pathMatrix;

    return primpath;
}

// Finalize articulations, process in parallel
void _FinalizeArticulations(const UsdStageWeakPtr stage,
    ArticulationMap& articulationMap, const RigidBodyMap& rigidBodyMap,
    const JointMap& jointMap)
{
    BodyJointMap bodyJointMap;
    if (!articulationMap.empty())
    {
        // construct the BodyJointMap
        bodyJointMap.reserve(rigidBodyMap.size());
        for (JointMap::const_reference& jointIt : jointMap)
        {
            const UsdPhysicsJointDesc* desc = jointIt.second;
            if (desc->jointEnabled)
            {
                if (desc->body0 != SdfPath())
                {
                    RigidBodyMap::const_iterator fit = 
                        rigidBodyMap.find(desc->body0);
                    if (fit != rigidBodyMap.end() && 
                        fit->second->rigidBodyEnabled &&
                        !fit->second->kinematicBody)
                    {
                        bodyJointMap[desc->body0].push_back(desc);
                    }
                }
                if (desc->body1 != SdfPath())
                {
                    RigidBodyMap::const_iterator fit = 
                        rigidBodyMap.find(desc->body1);
                    if (fit != rigidBodyMap.end() && 
                        fit->second->rigidBodyEnabled &&
                        !fit->second->kinematicBody)
                    {
                        bodyJointMap[desc->body1].push_back(desc);
                    }
                }
            }
        }
    }

    // first get user defined articulation roots
    // then search for the best root in the articulation hierarchy
    const auto workLambda = [rigidBodyMap, jointMap, stage, bodyJointMap]
    (ArticulationMap::const_reference& it)
    {
        SdfPathVector articulationLinkOrderVector;

        const SdfPath& articulationPath = it.first;
        SdfPath articulationBaseLinkPath = articulationPath;

        std::set<SdfPath> articulatedJoints;
        std::set<SdfPath> articulatedBodies;

        // check if its a floating articulation
        {
            RigidBodyMap::const_iterator bodyIt = 
                    rigidBodyMap.find(articulationPath);
            if (bodyIt != rigidBodyMap.end())
            {
                it.second->rootPrims.push_back(bodyIt->first);
            }
            else
            {
                JointMap::const_iterator jointIt = 
                        jointMap.find(articulationPath);
                if (jointIt != jointMap.end())
                {
                    const SdfPath& jointPath = jointIt->first;
                    const UsdPhysicsJointDesc* jointDesc = jointIt->second;
                    if (jointDesc->body0 == SdfPath() || 
                        jointDesc->body1 == SdfPath())
                    {
                        it.second->rootPrims.push_back(jointPath);
                        articulationBaseLinkPath = 
                            jointDesc->body0 == SdfPath() ? 
                                jointDesc->body1 : jointDesc->body0;
                    }
                }
            }
        }

        // search through the hierarchy for the best root        
        const UsdPrim articulationPrim = 
                stage->GetPrimAtPath(articulationBaseLinkPath);
        if (!articulationPrim)
            return;
        UsdPrimRange range(articulationPrim, UsdTraverseInstanceProxies());
        std::vector<ArticulationLinkMap> articulationLinkMaps;
        articulationLinkOrderVector.clear();

        for (UsdPrimRange::const_iterator iter = range.begin(); 
            iter != range.end(); ++iter)
        {
            const UsdPrim& prim = *iter;
            if (!prim)
                continue;
            const SdfPath primPath = prim.GetPrimPath();
            if (_IsInLinkMap(primPath, articulationLinkMaps))
            {
                iter.PruneChildren(); // Skip the subtree rooted at this prim
                continue;
            }

            RigidBodyMap::const_iterator bodyIt = rigidBodyMap.find(primPath);
            if (bodyIt != rigidBodyMap.end())
            {
                articulationLinkMaps.push_back(ArticulationLinkMap());
                uint32_t index = 0;
                _TraverseHierarchy(stage, primPath, articulationLinkMaps.back(),
                    bodyJointMap, index, &articulationLinkOrderVector);
            }
        }

        if (it.second->rootPrims.empty())
        {
            for (size_t i = 0; i < articulationLinkMaps.size(); i++)
            {
                const ArticulationLinkMap& map = articulationLinkMaps[i];
                SdfPath linkPath = SdfPath();
                uint32_t largestWeight = 0;
                bool hasFixedJoint = false;
                for (ArticulationLinkMap::const_reference& linkIt : map)
                {
                    if (linkIt.second.hasFixedJoint)
                    {
                        hasFixedJoint = true;
                    }
                    if (linkIt.second.weight > largestWeight)
                    {
                        linkPath = (linkIt.second.rootJoint != SdfPath()) ?
                            linkIt.second.rootJoint : linkIt.first;
                        largestWeight = linkIt.second.weight;
                    }
                    else if (linkIt.second.weight == largestWeight)
                    {
                        const SdfPath optionalLinkPath =
                            (linkIt.second.rootJoint != SdfPath()) ?
                            linkIt.second.rootJoint : linkIt.first;
                        for (const SdfPath& orderPath : 
                                articulationLinkOrderVector)
                        {
                            if (orderPath == linkPath)
                            {
                                break;
                            }
                            else if (orderPath == optionalLinkPath)
                            {
                                linkPath = optionalLinkPath;
                            }
                        }
                    }

                    for (size_t j = linkIt.second.joints.size(); j--;)
                    {
                        articulatedJoints.insert(linkIt.second.joints[j]);
                    }
                }

                // for floating articulation lets find the body with the
                // shortest paths (center of graph)
                if (!hasFixedJoint)
                {
                    linkPath = _GetCenterOfGraph(map, 
                                                articulationLinkOrderVector);
                }

                if (linkPath != SdfPath())
                {
                    it.second->rootPrims.push_back(linkPath);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < articulationLinkMaps.size(); i++)
            {
                const ArticulationLinkMap& map = articulationLinkMaps[i];
                for (ArticulationLinkMap::const_reference& linkIt : map)
                {
                    for (size_t j = linkIt.second.joints.size(); j--;)
                    {
                        articulatedJoints.insert(linkIt.second.joints[j]);
                    }
                }
            }
        }
        for (size_t i = 0; i < articulationLinkMaps.size(); i++)
        {
            const ArticulationLinkMap& map = articulationLinkMaps[i];
            for (ArticulationLinkMap::const_reference& linkIt : map)
            {
                articulatedBodies.insert(linkIt.second.children.begin(), 
                                         linkIt.second.children.end());
            }
        }

        if (it.second->rootPrims.empty())
        {
            it.second->isValid = false;
        }

        for (const SdfPath& p : articulatedJoints)
        {
            it.second->articulatedJoints.push_back(p);
        }
        for (const SdfPath& p : articulatedBodies)
        {
            it.second->articulatedBodies.push_back(p);
        }
    };

    WorkParallelForEach(articulationMap.begin(), articulationMap.end(), 
                        workLambda);
}

bool LoadUsdPhysicsFromRange(const UsdStageWeakPtr stage,
    const std::vector<SdfPath>& includePaths,
    UsdPhysicsReportFn reportFn,
    const VtValue& userData,
    const std::vector<SdfPath>* excludePaths,
    const CustomUsdPhysicsTokens* customPhysicsTokens,
    const std::vector<SdfPath>* simulationOwners)
{
    bool retVal = true;

    if (!stage)
    {
        TF_CODING_ERROR("Provided stage not valid.");
        return false;
    }

    if (!reportFn)
    {
        TF_CODING_ERROR("Provided report callback is not valid.");
        return false;
    }

    if (includePaths.empty())
    {
        TF_CODING_ERROR("No include path provided, nothing to parse.");
        return false;
    }

    std::vector<UsdPrim> scenePrims;
    std::vector<UsdPrim> collisionGroupPrims;
    std::vector<UsdPrim> materialPrims;
    std::vector<UsdPrim> articulationPrims;
    std::unordered_set<SdfPath, SdfPath::Hash> articulationPathsSet;
    std::vector<UsdPrim> physicsD6JointPrims;
    std::vector<UsdPrim> physicsRevoluteJointPrims;
    std::vector<UsdPrim> physicsFixedJointPrims;
    std::vector<UsdPrim> physicsPrismaticJointPrims;
    std::vector<UsdPrim> physicsSphericalJointPrims;
    std::vector<UsdPrim> physicsDistanceJointPrims;
    std::vector<UsdPrim> physicsCustomJointPrims;
    std::vector<UsdPrim> collisionPrims;
    std::vector<UsdPrim> rigidBodyPrims;

    std::unordered_set<SdfPath, SdfPath::Hash> excludePathsSet;
    if (excludePaths && !excludePaths->empty())
    {
        for (const SdfPath& p : *excludePaths)
        {
            excludePathsSet.insert(p);
        }
    }

    static const TfToken gRigidBodyAPIToken("PhysicsRigidBodyAPI");
    static const TfToken gCollisionAPIToken("PhysicsCollisionAPI");
    static const TfToken gArticulationRootAPIToken(
        "PhysicsArticulationRootAPI");
    static const TfToken gMaterialAPIToken("PhysicsMaterialAPI");

    bool defaultSimulationOwner = false;
    std::unordered_set<SdfPath, SdfPath::Hash> simulationOwnersSet;
    if (simulationOwners)
    {
        for (const SdfPath& p : *simulationOwners)
        {
            if (p == SdfPath())
            {
                defaultSimulationOwner = true;
            }
            else
            {
                simulationOwnersSet.insert(p);
            }
        }
    }

    for (const SdfPath& includePath : includePaths)
    {
        const UsdPrim includePrim = stage->GetPrimAtPath(includePath);
        UsdPrimRange includePrimRange(includePrim, UsdTraverseInstanceProxies());

        for (UsdPrimRange::const_iterator iter = includePrimRange.begin(); 
            iter != includePrimRange.end(); ++iter)
        {
            const UsdPrim& prim = *iter;
            if (!prim)
            {
                iter.PruneChildren();
                continue;
            }

            if (!excludePathsSet.empty() && 
                excludePathsSet.find(prim.GetPrimPath()) != excludePathsSet.end())
            {
                iter.PruneChildren();
                continue;
            }

            const SdfPath primPath = prim.GetPrimPath();
            const UsdPrimTypeInfo& typeInfo = prim.GetPrimTypeInfo();

            uint32_t apiFlags = 0;
            const TfTokenVector& apis =
                prim.GetPrimTypeInfo().GetAppliedAPISchemas();
            for (const TfToken& token : apis)
            {
                if (token == gArticulationRootAPIToken)
                {
                    apiFlags |= uint32_t(_SchemaAPIFlag::ArticulationRootAPI);
                }
                if (token == gCollisionAPIToken)
                {
                    apiFlags |= uint32_t(_SchemaAPIFlag::CollisionAPI);
                }
                if (token == gRigidBodyAPIToken)
                {
                    apiFlags |= uint32_t(_SchemaAPIFlag::RigidBodyAPI);
                }
                if (!apiFlags && token == gMaterialAPIToken)
                {
                    apiFlags |= uint32_t(_SchemaAPIFlag::MaterialAPI);
                }
            }

            if (typeInfo.GetSchemaType().IsA<UsdGeomPointInstancer>())
            {
                // Skip the subtree for point instancers, those have to be 
                // traversed per prototype
                iter.PruneChildren();
            }
            else if (customPhysicsTokens &&
                !customPhysicsTokens->instancerTokens.empty())
            {
                for (const TfToken& instToken :
                    customPhysicsTokens->instancerTokens)
                {
                    if (instToken == typeInfo.GetTypeName())
                    {
                        // Skip the subtree for custom 
                        // instancers, those have to be traversed per prototype
                        iter.PruneChildren();
                        break;
                    }
                }
            }

            if (typeInfo.GetSchemaType().IsA<UsdPhysicsScene>())
            {
                scenePrims.push_back(prim);
            }
            else if (typeInfo.GetSchemaType().IsA<UsdPhysicsCollisionGroup>())
            {
                collisionGroupPrims.push_back(prim);
            }
            else if (apiFlags & uint32_t(_SchemaAPIFlag::MaterialAPI))
            {
                materialPrims.push_back(prim);
            }
            else if (typeInfo.GetSchemaType().IsA<UsdPhysicsJoint>())
            {
                if (typeInfo.GetSchemaType().IsA<UsdPhysicsFixedJoint>())
                {
                    physicsFixedJointPrims.push_back(prim);
                }
                else if (typeInfo.GetSchemaType().IsA<UsdPhysicsRevoluteJoint>())
                {
                    physicsRevoluteJointPrims.push_back(prim);
                }
                else if (typeInfo.GetSchemaType().IsA<UsdPhysicsPrismaticJoint>())
                {
                    physicsPrismaticJointPrims.push_back(prim);
                }
                else if (typeInfo.GetSchemaType().IsA<UsdPhysicsSphericalJoint>())
                {
                    physicsSphericalJointPrims.push_back(prim);
                }
                else if (typeInfo.GetSchemaType().IsA<UsdPhysicsDistanceJoint>())
                {
                    physicsDistanceJointPrims.push_back(prim);
                }
                else
                {
                    bool customJoint = false;
                    if (customPhysicsTokens)
                    {
                        const TfToken& primType = typeInfo.GetTypeName();
                        for (size_t i = 0;
                            i < customPhysicsTokens->jointTokens.size(); i++)
                        {
                            if (primType == customPhysicsTokens->jointTokens[i])
                            {
                                customJoint = true;
                                break;
                            }
                        }
                    }

                    if (customJoint)
                    {
                        physicsCustomJointPrims.push_back(prim);
                    }
                    else
                    {
                        physicsD6JointPrims.push_back(prim);
                    }
                }


                // can be articulation definition
                if (apiFlags & uint32_t(_SchemaAPIFlag::ArticulationRootAPI))
                {
                    articulationPrims.push_back(prim);
                    articulationPathsSet.insert(prim.GetPrimPath());
                }
            }
            else
            {
                if (apiFlags & uint32_t(_SchemaAPIFlag::CollisionAPI))
                {
                    collisionPrims.push_back(prim);
                }
                if (apiFlags & uint32_t(_SchemaAPIFlag::RigidBodyAPI))
                {
                    rigidBodyPrims.push_back(prim);
                }
                if (apiFlags & uint32_t(_SchemaAPIFlag::ArticulationRootAPI))
                {
                    articulationPrims.push_back(prim);
                    articulationPathsSet.insert(prim.GetPrimPath());
                }
            }
        }
    }


    // process parsing
    // 
    // Scenes
    std::vector<UsdPhysicsSceneDesc> sceneDescs;

    // is simulation owners provided, restrict scenes to just the one specified
    if (simulationOwners)
    {
        for (size_t i = scenePrims.size(); i--;)
        {
            const SdfPath& primPath = scenePrims[i].GetPrimPath();
            std::unordered_set<SdfPath, SdfPath::Hash>::const_iterator fit =
                simulationOwnersSet.find(primPath);
            if (fit == simulationOwnersSet.end())
            {
                scenePrims[i] = scenePrims.back();
                scenePrims.pop_back();
            }
        }
    }
    _ProcessPhysicsPrims<UsdPhysicsSceneDesc, UsdPhysicsScene>(
        scenePrims, sceneDescs, _ParseSceneDesc);

    // Collision Groups
    std::vector<UsdPhysicsCollisionGroupDesc> collisionGroupsDescs;
    _ProcessPhysicsPrims<UsdPhysicsCollisionGroupDesc, UsdPhysicsCollisionGroup>
        (collisionGroupPrims, collisionGroupsDescs, _ParseCollisionGroupDesc);
    // Run groups merging
    std::map<SdfPath, 
        std::unordered_set<SdfPath, SdfPath::Hash>> collisionGroupSets;
    std::unordered_map<std::string, size_t> mergeGroupNameToIndex;
    for (size_t i = 0; i < collisionGroupsDescs.size(); i++)
    {
        const UsdPhysicsCollisionGroupDesc& desc = collisionGroupsDescs[i];

        collisionGroupSets[desc.primPath];

        if (!desc.mergeGroupName.empty())
        {
            std::unordered_map<std::string, size_t>::const_iterator fit =
                mergeGroupNameToIndex.find(desc.mergeGroupName);
            if (fit != mergeGroupNameToIndex.end())
            {
                UsdPhysicsCollisionGroupDesc& mergeDesc = 
                    collisionGroupsDescs[fit->second];
                mergeDesc.mergedGroups.push_back(desc.primPath);
                for (const SdfPath& sp : desc.filteredGroups)
                {
                    mergeDesc.filteredGroups.push_back(sp);
                }

                collisionGroupsDescs[i] = collisionGroupsDescs.back();
                collisionGroupPrims[i] = collisionGroupPrims.back();
                collisionGroupsDescs.pop_back();
                collisionGroupPrims.pop_back();
                i--;
            }
            else
            {
                mergeGroupNameToIndex[desc.mergeGroupName] = i;
                collisionGroupsDescs[i].mergedGroups.push_back(desc.primPath);
            }
        }
    }

    // Populate the sets to check collisions
    {
        const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
        {
            for (size_t i = beginIdx; i < endIdx; i++)
            {
                const UsdPrim groupPrim = collisionGroupPrims[i];
                UsdStageWeakPtr stage = groupPrim.GetStage();
                const UsdPhysicsCollisionGroupDesc& desc = 
                        collisionGroupsDescs[i];

                std::unordered_set<SdfPath, SdfPath::Hash>& hashSet =
                    collisionGroupSets[desc.primPath];

                if (desc.mergedGroups.empty())
                {
                    const UsdPhysicsCollisionGroup cg(
                            stage->GetPrimAtPath(desc.primPath));
                    if (cg)
                    {
                        const UsdCollectionAPI collectionAPI = 
                                cg.GetCollidersCollectionAPI();
                        UsdCollectionMembershipQuery query = 
                                collectionAPI.ComputeMembershipQuery();
                        const SdfPathSet includedPaths = 
                                UsdCollectionAPI::ComputeIncludedPaths(
                                    query, stage, UsdTraverseInstanceProxies());
                        for (const SdfPath& path : includedPaths)
                        {
                            hashSet.insert(path);
                        }
                    }
                }
                else
                {
                    for (const SdfPath& groupPath : desc.mergedGroups)
                    {
                        const UsdPhysicsCollisionGroup cg(
                                stage->GetPrimAtPath(groupPath));
                        if (cg)
                        {
                            const UsdCollectionAPI collectionAPI =
                                cg.GetCollidersCollectionAPI();
                            UsdCollectionMembershipQuery query =
                                collectionAPI.ComputeMembershipQuery();
                            const SdfPathSet includedPaths =
                                UsdCollectionAPI::ComputeIncludedPaths(
                                        query, stage, 
                                        UsdTraverseInstanceProxies());
                            for (const SdfPath& path : includedPaths)
                            {
                                hashSet.insert(path);
                            }
                        }
                    }
                }
            }
        };

        const size_t numPrimPerBatch = 10;
        WorkParallelForN(collisionGroupsDescs.size(), workLambda, 
                         numPrimPerBatch);
    }

    // Rigid body physics material
    std::vector<UsdPhysicsRigidBodyMaterialDesc> materialDescs;
    _ProcessPhysicsPrims<UsdPhysicsRigidBodyMaterialDesc, UsdPhysicsMaterialAPI>
        (materialPrims, materialDescs, _ParseRigidBodyMaterialDesc);

    // Joints
    std::vector<UsdPhysicsD6JointDesc> jointDescs;
    _ProcessPhysicsPrims<UsdPhysicsD6JointDesc, UsdPhysicsJoint>
        (physicsD6JointPrims, jointDescs, _ParseD6JointDesc);

    std::vector<UsdPhysicsRevoluteJointDesc> revoluteJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsRevoluteJointDesc, UsdPhysicsRevoluteJoint>
        (physicsRevoluteJointPrims, revoluteJointDescs, _ParseRevoluteJointDesc);

    std::vector<UsdPhysicsPrismaticJointDesc> prismaticJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsPrismaticJointDesc, UsdPhysicsPrismaticJoint>
        (physicsPrismaticJointPrims, prismaticJointDescs, 
         _ParsePrismaticJointDesc);

    std::vector<UsdPhysicsSphericalJointDesc> sphericalJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsSphericalJointDesc, UsdPhysicsSphericalJoint>
        (physicsSphericalJointPrims, sphericalJointDescs, 
         _ParseSphericalJointDesc);

    std::vector<UsdPhysicsFixedJointDesc> fixedJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsFixedJointDesc, UsdPhysicsFixedJoint>
        (physicsFixedJointPrims, fixedJointDescs, _ParseFixedJointDesc);

    std::vector<UsdPhysicsDistanceJointDesc> distanceJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsDistanceJointDesc, UsdPhysicsDistanceJoint>
        (physicsDistanceJointPrims, distanceJointDescs, _ParseDistanceJointDesc);

    std::vector<UsdPhysicsCustomJointDesc> customJointDescs;
    _ProcessPhysicsPrims<UsdPhysicsCustomJointDesc, UsdPhysicsJoint>
        (physicsCustomJointPrims, customJointDescs, _ParseCustomJointDesc);

    // A.B. construct joint map revisit    
    JointMap jointMap;
    for (UsdPhysicsD6JointDesc& desc : jointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsRevoluteJointDesc& desc : revoluteJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsPrismaticJointDesc& desc : prismaticJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsSphericalJointDesc& desc : sphericalJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsFixedJointDesc& desc : fixedJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsDistanceJointDesc& desc : distanceJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (UsdPhysicsCustomJointDesc& desc : customJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }


    // collisions
    // first get the type
    std::vector<UsdPhysicsObjectType> collisionTypes;
    collisionTypes.resize(collisionPrims.size());
    std::vector<TfToken> customTokens;
    {
        const auto workLambda = [&](const size_t beginIdx, const size_t endIdx)
        {
            for (size_t i = beginIdx; i < endIdx; i++)
            {
                if (customPhysicsTokens)
                {
                    TfToken shapeToken;
                    const UsdPhysicsObjectType shapeType =
                        _GetCollisionType(collisionPrims[i], 
                                         &customPhysicsTokens->shapeTokens, 
                                         &shapeToken);
                    collisionTypes[i] = shapeType;
                    if (shapeType == UsdPhysicsObjectType::CustomShape)
                    {
                        customTokens.push_back(shapeToken);
                    }
                }
                else
                {
                    collisionTypes[i] = _GetCollisionType(collisionPrims[i], 
                                                         nullptr, nullptr);
                }
            }
        };

        const size_t numPrimPerBatch = 10;
        WorkParallelForN(collisionPrims.size(), workLambda, numPrimPerBatch);
    }

    std::vector<UsdPrim> sphereShapePrims;
    std::vector<UsdPrim> cubeShapePrims;
    std::vector<UsdPrim> cylinderShapePrims;
    std::vector<UsdPrim> cylinder1ShapePrims;
    std::vector<UsdPrim> capsuleShapePrims;
    std::vector<UsdPrim> capsule1ShapePrims;
    std::vector<UsdPrim> coneShapePrims;
    std::vector<UsdPrim> planeShapePrims;
    std::vector<UsdPrim> meshShapePrims;
    std::vector<UsdPrim> spherePointsShapePrims;
    std::vector<UsdPrim> customShapePrims;
    for (size_t i = 0; i < collisionTypes.size(); i++)
    {
        UsdPhysicsObjectType type = collisionTypes[i];
        switch (type)
        {
        case UsdPhysicsObjectType::SphereShape:
        {
            sphereShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::CubeShape:
        {
            cubeShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::CapsuleShape:
        {
            capsuleShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::Capsule1Shape:
        {
            capsule1ShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::CylinderShape:
        {
            cylinderShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::Cylinder1Shape:
        {
            cylinder1ShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::ConeShape:
        {
            coneShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::MeshShape:
        {
            meshShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::PlaneShape:
        {
            planeShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::CustomShape:
        {
            customShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::SpherePointsShape:
        {
            spherePointsShapePrims.push_back(collisionPrims[i]);
        }
        break;
        case UsdPhysicsObjectType::Undefined:
        default:
        {
            TF_DIAGNOSTIC_WARNING("CollisionAPI applied to an unknown "
                                  "UsdGeomGPrim type, prim %s.",
                collisionPrims[i].GetPrimPath().GetString().c_str());
        }
        break;
        }
    }
    std::vector<UsdPhysicsSphereShapeDesc> sphereShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsSphereShapeDesc, UsdPhysicsCollisionAPI>
        (sphereShapePrims, sphereShapeDescs, _ParseSphereShapeDesc);

    std::vector<UsdPhysicsCubeShapeDesc> cubeShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCubeShapeDesc, UsdPhysicsCollisionAPI>
        (cubeShapePrims, cubeShapeDescs, _ParseCubeShapeDesc);

    std::vector<UsdPhysicsCylinderShapeDesc> cylinderShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCylinderShapeDesc, UsdPhysicsCollisionAPI>
        (cylinderShapePrims, cylinderShapeDescs, _ParseCylinderShapeDesc);

    std::vector<UsdPhysicsCylinder1ShapeDesc> cylinder1ShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCylinder1ShapeDesc, UsdPhysicsCollisionAPI>
        (cylinder1ShapePrims, cylinder1ShapeDescs, _ParseCylinder1ShapeDesc);

    std::vector<UsdPhysicsCapsuleShapeDesc> capsuleShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCapsuleShapeDesc, UsdPhysicsCollisionAPI>
        (capsuleShapePrims, capsuleShapeDescs, _ParseCapsuleShapeDesc);

    std::vector<UsdPhysicsCapsule1ShapeDesc> capsule1ShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCapsule1ShapeDesc, UsdPhysicsCollisionAPI>
        (capsule1ShapePrims, capsule1ShapeDescs, _ParseCapsule1ShapeDesc);
    
    std::vector<UsdPhysicsConeShapeDesc> coneShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsConeShapeDesc, UsdPhysicsCollisionAPI>
        (coneShapePrims, coneShapeDescs, _ParseConeShapeDesc);

    std::vector<UsdPhysicsPlaneShapeDesc> planeShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsPlaneShapeDesc, UsdPhysicsCollisionAPI>
        (planeShapePrims, planeShapeDescs, _ParsePlaneShapeDesc);

    std::vector<UsdPhysicsMeshShapeDesc> meshShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsMeshShapeDesc, UsdPhysicsCollisionAPI>
        (meshShapePrims, meshShapeDescs, _ParseMeshShapeDesc);

    std::vector<UsdPhysicsSpherePointsShapeDesc> spherePointsShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsSpherePointsShapeDesc, UsdPhysicsCollisionAPI>
        (spherePointsShapePrims, spherePointsShapeDescs, 
         _ParseSpherePointsShapeDesc);

    std::vector<UsdPhysicsCustomShapeDesc> customShapeDescs;
    _ProcessPhysicsPrims<UsdPhysicsCustomShapeDesc, UsdPhysicsCollisionAPI>
        (customShapePrims, customShapeDescs, _ParseCustomShapeDesc);
    if (customShapeDescs.size() == customTokens.size())
    {
        for (size_t i = 0; i < customShapeDescs.size(); i++)
        {
            customShapeDescs[i].customGeometryToken = customTokens[i];
        }
    }

    // rigid bodies
    std::vector<UsdPhysicsRigidBodyDesc> rigidBodyDescs;
    _ProcessPhysicsPrims<UsdPhysicsRigidBodyDesc, UsdPhysicsRigidBodyAPI>
        (rigidBodyPrims, rigidBodyDescs, _ParseRigidBodyDesc);

    RigidBodyMap bodyMap;
    for (size_t i = rigidBodyPrims.size(); i--;)
    {
        bodyMap[rigidBodyPrims[i].GetPrimPath()] = &rigidBodyDescs[i];
    }


    std::vector<UsdPhysicsArticulationDesc> articulationDescs;
    _ProcessPhysicsPrims<UsdPhysicsArticulationDesc, 
        UsdPhysicsArticulationRootAPI>
        (articulationPrims, articulationDescs, _ParseArticulationDesc);

    ArticulationMap articulationMap; // A.B. TODO probably not needed
    for (size_t i = articulationPrims.size(); i--;)
    {
        articulationMap[articulationPrims[i].GetPrimPath()] = 
            &articulationDescs[i];
    }

    // Finalize collisions
    {
        UsdGeomXformCache xfCache;

        _FinalizeCollisionDescs<UsdPhysicsSphereShapeDesc>
            (xfCache, sphereShapePrims, sphereShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsCubeShapeDesc>
            (xfCache, cubeShapePrims, cubeShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsCapsuleShapeDesc>
            (xfCache, capsuleShapePrims, capsuleShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsCapsule1ShapeDesc>
            (xfCache, capsule1ShapePrims, capsule1ShapeDescs, bodyMap, 
            collisionGroupSets);
         _FinalizeCollisionDescs<UsdPhysicsCylinderShapeDesc>
            (xfCache, cylinderShapePrims, cylinderShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsCylinder1ShapeDesc>
            (xfCache, cylinder1ShapePrims, cylinder1ShapeDescs, bodyMap, 
            collisionGroupSets);
         _FinalizeCollisionDescs<UsdPhysicsConeShapeDesc>
            (xfCache, coneShapePrims, coneShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsPlaneShapeDesc>
            (xfCache, planeShapePrims, planeShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsMeshShapeDesc>
            (xfCache, meshShapePrims, meshShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsSpherePointsShapeDesc>
            (xfCache, spherePointsShapePrims, spherePointsShapeDescs, bodyMap, 
             collisionGroupSets);
        _FinalizeCollisionDescs<UsdPhysicsCustomShapeDesc>
            (xfCache, customShapePrims, customShapeDescs, bodyMap, 
             collisionGroupSets);
    }

    // Finalize articulations
    {
        // A.B. walk through the finalize code refactor
        _FinalizeArticulations(stage, articulationMap, bodyMap, jointMap);
    }

    // if simulationOwners are in play lets shrink down the reported descriptors    
    if (simulationOwners && !simulationOwners->empty())
    {
        std::unordered_set<SdfPath, SdfPath::Hash> reportedBodies;
        // first check bodies
        _CheckRigidBodySimulationOwner(rigidBodyPrims, rigidBodyDescs,
            defaultSimulationOwner, simulationOwnersSet, &reportedBodies);

        // check collisions
        // if collision belongs to a body that we care about include it
        // if collision does not belong to a body we care about its not included
        // if collision does not have a body set, we check its own 
        // simulationOwners
        _CheckCollisionSimulationOwner(sphereShapePrims, sphereShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(cubeShapePrims, cubeShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(capsuleShapePrims, capsuleShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(capsule1ShapePrims, capsule1ShapeDescs,
                defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(cylinderShapePrims, cylinderShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(cylinder1ShapePrims, cylinder1ShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);    
        _CheckCollisionSimulationOwner(coneShapePrims, coneShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(planeShapePrims, planeShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(meshShapePrims, meshShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(spherePointsShapePrims, 
                                      spherePointsShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckCollisionSimulationOwner(customShapePrims, customShapeDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);

        // Both bodies need to have simulation owners valid
        _CheckJointSimulationOwner(physicsFixedJointPrims, fixedJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsRevoluteJointPrims, revoluteJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsPrismaticJointPrims, 
                                  prismaticJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsSphericalJointPrims, 
                                  sphericalJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsDistanceJointPrims, distanceJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsD6JointPrims, jointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        _CheckJointSimulationOwner(physicsCustomJointPrims, customJointDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);

        // All bodies need to have simulation owners valid
        _CheckArticulationSimulationOwner(articulationPrims, articulationDescs,
            defaultSimulationOwner, reportedBodies, simulationOwnersSet);
    }

    SdfPathVector primPathsVector;
    // get the descriptors, finalize them and send them out in an order
    // 1. send out the scenes
    {
        _CallReportFn(UsdPhysicsObjectType::Scene, scenePrims, sceneDescs,
            reportFn, primPathsVector, userData);
    }

    // 2. send out the CollisionGroups
    {
        _CallReportFn(UsdPhysicsObjectType::CollisionGroup, collisionGroupPrims,
            collisionGroupsDescs, reportFn, primPathsVector, userData);
    }

    // 3. send out the materials
    {
        _CallReportFn(UsdPhysicsObjectType::RigidBodyMaterial, materialPrims,
            materialDescs, reportFn, primPathsVector, userData);
    }

    // 4. finish out and send out shapes
    {
        _CallReportFn(UsdPhysicsObjectType::SphereShape, sphereShapePrims, 
                     sphereShapeDescs,
            reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::CubeShape, cubeShapePrims, 
                     cubeShapeDescs,
            reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::CapsuleShape, capsuleShapePrims,
            capsuleShapeDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::Capsule1Shape, capsule1ShapePrims,
                capsule1ShapeDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::CylinderShape, cylinderShapePrims,
            cylinderShapeDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::Cylinder1Shape, cylinder1ShapePrims,
            cylinder1ShapeDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::ConeShape, coneShapePrims, 
                    coneShapeDescs,
            reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::PlaneShape, planeShapePrims, 
                     planeShapeDescs,
            reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::MeshShape, meshShapePrims, 
                     meshShapeDescs,
            reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::SpherePointsShape, 
                     spherePointsShapePrims,
            spherePointsShapeDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::CustomShape, customShapePrims, 
                     customShapeDescs,
            reportFn, primPathsVector, userData);
    }

    // 5. send out articulations
    {
        _CallReportFn(UsdPhysicsObjectType::Articulation, articulationPrims,
            articulationDescs, reportFn, primPathsVector, userData);
    }

    // 6. send out bodies
    {
        _CallReportFn(UsdPhysicsObjectType::RigidBody, rigidBodyPrims, 
                     rigidBodyDescs, reportFn, primPathsVector, userData);
    }

    // 7. send out joints    
    {
        _CallReportFn(UsdPhysicsObjectType::FixedJoint, physicsFixedJointPrims,
            fixedJointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::RevoluteJoint, 
                     physicsRevoluteJointPrims,
            revoluteJointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::PrismaticJoint, 
                     physicsPrismaticJointPrims,
            prismaticJointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::SphericalJoint, 
                     physicsSphericalJointPrims,
            sphericalJointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::DistanceJoint, 
                     physicsDistanceJointPrims,
            distanceJointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::D6Joint, physicsD6JointPrims, 
                     jointDescs, reportFn, primPathsVector, userData);
        _CallReportFn(UsdPhysicsObjectType::CustomJoint, 
                     physicsCustomJointPrims, customJointDescs, reportFn, 
                     primPathsVector, userData);
    }

    return retVal;
}


PXR_NAMESPACE_CLOSE_SCOPE
