//
// Copyright 2021 Pixar
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

#include "pxr/pxr.h"

#include "pxr/base/gf/transform.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usdGeom/plane.h"
#include "pxr/usd/usdGeom/points.h"

#include "pxr/usd/usdGeom/metrics.h"

#include "pxr/usd/usdShade/materialBindingAPI.h"

#include "parseUtils.h"
#include "parseDesc.h"

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

PXR_NAMESPACE_OPEN_SCOPE

void ParseFilteredPairs(const pxr::UsdPrim& usdPrim, pxr::SdfPathVector& filteredPairs)
{
    UsdPhysicsFilteredPairsAPI filteredPairsAPI = UsdPhysicsFilteredPairsAPI::Get(usdPrim.GetStage(), usdPrim.GetPrimPath());
    if (filteredPairsAPI && filteredPairsAPI.GetFilteredPairsRel())
    {
        filteredPairsAPI.GetFilteredPairsRel().GetTargets(&filteredPairs);
    }
}

bool ParseArticulationDesc(const UsdPhysicsArticulationRootAPI& articulationAPI, ArticulationDesc* articulationDesc)
{
    if (articulationDesc && articulationAPI)
    {       
        ParseFilteredPairs(articulationAPI.GetPrim(), articulationDesc->filteredCollisions);

        articulationDesc->primPath = articulationAPI.GetPrim().GetPrimPath();
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsArticulationRootAPI or ArticulationDesc is not valid.");
        return false;

    }
    return true;
}

PhysicsObjectType::Enum GetCollisionType(const UsdPrim& prim, const std::vector<TfToken>* customTokens, TfToken* customeGeometryToken)
{
    PhysicsObjectType::Enum retVal = PhysicsObjectType::eUndefined;

    // custom shape handling
    bool customShape = false;
    if (customTokens)
    {
        const TfTokenVector& apis = prim.GetPrimTypeInfo().GetAppliedAPISchemas();

        const TfToken& primType = prim.GetTypeName();
        for (size_t i = 0; i < customTokens->size(); i++)
        {
            for (size_t j = 0; j < apis.size(); j++)
            {
                if (apis[j] == (*customTokens)[i])
                {
                    retVal = PhysicsObjectType::eCustomShape;
                    if (customeGeometryToken)
                        *customeGeometryToken = apis[j];
                    break;
                }
            }
            if(retVal == PhysicsObjectType::eCustomShape)
            {
                break;
            }
            if (primType == (*customTokens)[i])
            {
                    retVal = PhysicsObjectType::eCustomShape;
                    if (customeGeometryToken)
                        *customeGeometryToken = primType;                
                break;
            }
        }
    }

    if (retVal == PhysicsObjectType::eCustomShape)
        return retVal;

    // geomgprim that belongs to that collision
    if (prim.IsA<UsdGeomGprim>())
    {
        // If the primitive is a UsdGeomPoints *and* it has a widths attribute
        // corresponding to the positions attribute, then we treat it as an
        // array of spheres corresponding to the 'SpherePointsShapeDesc'
        if (prim.IsA<UsdGeomMesh>())
        {
            retVal = PhysicsObjectType::eMeshShape;
        }
        else if (prim.IsA<UsdGeomCube>())
        {
            retVal = PhysicsObjectType::eCubeShape;
        }
        else if (prim.IsA<UsdGeomSphere>())
        {
            retVal = PhysicsObjectType::eSphereShape;
        }
        else if (prim.IsA<UsdGeomCapsule>())
        {
            retVal = PhysicsObjectType::eCapsuleShape;
        }
        else if (prim.IsA<UsdGeomCylinder>())
        {
            retVal = PhysicsObjectType::eCylinderShape;
        }
        else if (prim.IsA<UsdGeomCone>())
        {
            retVal = PhysicsObjectType::eConeShape;
        }
        else if (prim.IsA<UsdGeomPlane>())
        {
            retVal = PhysicsObjectType::ePlaneShape;
        }
        else if (prim.IsA<UsdGeomPoints>())
        {
            retVal = PhysicsObjectType::eSpherePointsShape;
        }
    }
    
    return retVal;
}

const double tolerance = 1e-4;

void CheckNonUniformScale(const GfVec3d& scale, const SdfPath& primPath)
{
    if (abs(scale[0] - scale[1]) > tolerance || abs(scale[0] - scale[2]) > tolerance || abs(scale[2] - scale[1]) > tolerance)
    {
        TF_DIAGNOSTIC_WARNING("Non-uniform scale may result in a non matching collision representation on prim: %s", primPath.GetText());
    }
}


pxr::SdfPath GetMaterialBinding(const pxr::UsdPrim& usdPrim)
{
    SdfPath materialPath = SdfPath();

    const static TfToken physicsPurpose("physics");
    UsdShadeMaterialBindingAPI materialBindingAPI = UsdShadeMaterialBindingAPI(usdPrim);
    if (materialBindingAPI)
    {
        UsdShadeMaterial material = materialBindingAPI.ComputeBoundMaterial(physicsPurpose);
        if (material)
        {
            materialPath = material.GetPrim().GetPrimPath();
        }
    }
    else
    {
        // handle material through a direct binding rel search
        std::vector<UsdPrim> prims;
        prims.push_back(usdPrim);
        std::vector<UsdShadeMaterial> materials =
            UsdShadeMaterialBindingAPI::ComputeBoundMaterials(prims, physicsPurpose);
        if (!materials.empty() && materials[0])
        {
            materialPath = materials[0].GetPrim().GetPrimPath();
        }
    }

    return materialPath;
}

static void ParseColFilteredPairs(const pxr::UsdPrim& usdPrim, pxr::SdfPathVector& filteredPairs)
{    
    UsdPhysicsFilteredPairsAPI filteredPairsAPI = UsdPhysicsFilteredPairsAPI::Get(usdPrim.GetStage(), usdPrim.GetPrimPath());
    if (filteredPairsAPI && filteredPairsAPI.GetFilteredPairsRel())
    {
        filteredPairsAPI.GetFilteredPairsRel().GetTargets(&filteredPairs);
    }
}

void FinalizeDesc(const UsdPhysicsCollisionAPI& colAPI, ShapeDesc& desc)
{
    // set the collider material as last
    // set SdfPath() anyway, this would indicate default material should be used, this is required for trimesh subset materials
    // as not alway all faces are covered with a subset material
    const SdfPath& materialPath = GetMaterialBinding(colAPI.GetPrim());
    if (materialPath != SdfPath())
    {
        const UsdPrim materialPrim = colAPI.GetPrim().GetStage()->GetPrimAtPath(materialPath);
        if (materialPrim && materialPrim.HasAPI<UsdPhysicsMaterialAPI>())
            desc.materials.push_back(materialPath);
        else
            desc.materials.push_back(SdfPath());
    }
    else
    {
        desc.materials.push_back(SdfPath());
    }

    ParseColFilteredPairs(colAPI.GetPrim(), desc.filteredCollisions);
    colAPI.GetCollisionEnabledAttr().Get(&desc.collisionEnabled);
    const UsdRelationship ownerRel = colAPI.GetSimulationOwnerRel();
    if (ownerRel)
    {
        ownerRel.GetTargets(&desc.simulationOwners);
    }
}


bool ParseSphereShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, SphereShapeDesc* sphereShapeDesc)
{
    if (sphereShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomSphere shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;

            // Check scale, its part of the collision size
            {
                const pxr::GfVec3d sc = tr.GetScale();
                // as we dont support scale in physics and scale can be non uniform
                // we pick the largest scale value as the sphere radius base
                CheckNonUniformScale(sc, usdPrim.GetPrimPath());
                radius = fmaxf(fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0]))), fabsf(float(sc[2])));
            }

            // Get shape parameters
            {

                double radiusAttr;
                shape.GetRadiusAttr().Get(&radiusAttr);
                radius *= (float)radiusAttr;
            }

            sphereShapeDesc->radius = fabsf(radius);
            sphereShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *sphereShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomSphere.");
            return false;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or SphereShapeDesc is not valid.");
        return false;
    }
    return true;
}

bool ParseCubeShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, CubeShapeDesc* cubeShapeDesc)
{
    if (cubeShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCube shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            GfVec3f halfExtents;

            // Add scale
            {
                const pxr::GfVec3d sc = tr.GetScale();
                // scale is taken, its a part of the cube size, as the physics does not support scale
                halfExtents = GfVec3f(sc);
            }

            // Get shape parameters
            {
                UsdGeomCube shape(usdPrim);
                double sizeAttr;
                shape.GetSizeAttr().Get(&sizeAttr);
                sizeAttr = abs(sizeAttr) * 0.5f; // convert cube edge length to half extend
                halfExtents *= (float)sizeAttr;
            }

            cubeShapeDesc->halfExtents = halfExtents;
            cubeShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *cubeShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomCube.");
            return false;
        }        
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or CubeShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseCylinderShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, CylinderShapeDesc* cylinderShapeDesc)
{
    if (cylinderShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCylinder shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            PhysicsAxis::Enum axis = PhysicsAxis::eX;

            // Get shape parameters
            {
                double radiusAttr;
                shape.GetRadiusAttr().Get(&radiusAttr);
                double heightAttr;
                shape.GetHeightAttr().Get(&heightAttr);
                radius = (float)radiusAttr;
                halfHeight = (float)heightAttr * 0.5f;

                TfToken capAxis;
                if (shape.GetAxisAttr())
                {
                    shape.GetAxisAttr().Get(&capAxis);
                    if (capAxis == UsdPhysicsTokens.Get()->y)
                        axis = PhysicsAxis::eY;
                    else if (capAxis == UsdPhysicsTokens.Get()->z)
                        axis = PhysicsAxis::eZ;
                }
            }

            {
                // scale the radius and height based on the given axis token
                const pxr::GfVec3d sc = tr.GetScale();
                CheckNonUniformScale(sc, usdPrim.GetPrimPath());
                if (axis == PhysicsAxis::eX)
                {
                    halfHeight *= float(sc[0]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
                }
                else if (axis == PhysicsAxis::eY)
                {
                    halfHeight *= float(sc[1]);
                    radius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
                }
                else
                {
                    halfHeight *= float(sc[2]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
                }
            }
            cylinderShapeDesc->radius = fabsf(radius);
            cylinderShapeDesc->axis = axis;
            cylinderShapeDesc->halfHeight = fabsf(halfHeight);
            cylinderShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *cylinderShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomCylinder.");
            return false;
        }        
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or CylinderShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseCapsuleShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, CapsuleShapeDesc* capsuleShapeDesc)
{
    if (capsuleShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCapsule shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            PhysicsAxis::Enum axis = PhysicsAxis::eX;

            // Get shape parameters
            {
                double radiusAttr;
                shape.GetRadiusAttr().Get(&radiusAttr);
                double heightAttr;
                shape.GetHeightAttr().Get(&heightAttr);
                radius = (float)radiusAttr;
                halfHeight = (float)heightAttr * 0.5f;

                TfToken capAxis;
                if (shape.GetAxisAttr())
                {
                    shape.GetAxisAttr().Get(&capAxis);
                    if (capAxis == UsdPhysicsTokens.Get()->y)
                        axis = PhysicsAxis::eY;
                    else if (capAxis == UsdPhysicsTokens.Get()->z)
                        axis = PhysicsAxis::eZ;
                }
            }

            {
                // scale the radius and height based on the given axis token
                const pxr::GfVec3d sc = tr.GetScale();
                CheckNonUniformScale(sc, usdPrim.GetPrimPath());
                if (axis == PhysicsAxis::eX)
                {
                    halfHeight *= float(sc[0]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
                }
                else if (axis == PhysicsAxis::eY)
                {
                    halfHeight *= float(sc[1]);
                    radius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
                }
                else
                {
                    halfHeight *= float(sc[2]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
                }
            }
            capsuleShapeDesc->radius = fabsf(radius);
            capsuleShapeDesc->axis = axis;
            capsuleShapeDesc->halfHeight = fabsf(halfHeight);
            capsuleShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *capsuleShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomCapsule.");
            return false;
        }        
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or CapsuleShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseConeShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, ConeShapeDesc* coneShapeDesc)
{
    if (coneShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomCone shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            float radius = 1.0f;
            float halfHeight = 1.0f;
            PhysicsAxis::Enum axis = PhysicsAxis::eX;

            // Get shape parameters
            {
                double radiusAttr;
                shape.GetRadiusAttr().Get(&radiusAttr);
                double heightAttr;
                shape.GetHeightAttr().Get(&heightAttr);
                radius = (float)radiusAttr;
                halfHeight = (float)heightAttr * 0.5f;

                TfToken capAxis;
                if (shape.GetAxisAttr())
                {
                    shape.GetAxisAttr().Get(&capAxis);
                    if (capAxis == UsdPhysicsTokens.Get()->y)
                        axis = PhysicsAxis::eY;
                    else if (capAxis == UsdPhysicsTokens.Get()->z)
                        axis = PhysicsAxis::eZ;
                }
            }

            {
                // scale the radius and height based on the given axis token
                const pxr::GfVec3d sc = tr.GetScale();
                CheckNonUniformScale(sc, usdPrim.GetPrimPath());
                if (axis == PhysicsAxis::eX)
                {
                    halfHeight *= float(sc[0]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[2])));
                }
                else if (axis == PhysicsAxis::eY)
                {
                    halfHeight *= float(sc[1]);
                    radius *= fmaxf(fabsf(float(sc[0])), fabsf(float(sc[2])));
                }
                else
                {
                    halfHeight *= float(sc[2]);
                    radius *= fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0])));
                }
            }
            coneShapeDesc->radius = fabsf(radius);
            coneShapeDesc->axis = axis;
            coneShapeDesc->halfHeight = fabsf(halfHeight);
            coneShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *coneShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomCone.");
            return false;
        }          
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or ConeShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseMeshShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, MeshShapeDesc* meshShapeDesc)
{
    if (meshShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomMesh shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

            const pxr::GfVec3d sc = tr.GetScale();
            meshShapeDesc->meshScale = GfVec3f(sc);

            // Get approximation type
            meshShapeDesc->approximation = UsdPhysicsTokens.Get()->none;
            UsdPhysicsMeshCollisionAPI physicsColMeshAPI(usdPrim);
            if (physicsColMeshAPI)
            {
                physicsColMeshAPI.GetApproximationAttr().Get(&meshShapeDesc->approximation);
            }

            shape.GetDoubleSidedAttr().Get(&meshShapeDesc->doubleSided);

            // Gather materials through subsets
            const std::vector<pxr::UsdGeomSubset> subsets = pxr::UsdGeomSubset::GetGeomSubsets(shape, pxr::UsdGeomTokens->face);
            if (!subsets.empty())
            {
                for (const pxr::UsdGeomSubset& subset : subsets)
                {
                    const pxr::SdfPath material = GetMaterialBinding(subset.GetPrim());
                    if (material != SdfPath())
                    {
                        const UsdPrim materialPrim = usdPrim.GetStage()->GetPrimAtPath(material);
                        if (materialPrim && materialPrim.HasAPI<UsdPhysicsMaterialAPI>())
                        {
                            meshShapeDesc->materials.push_back(material);
                        }
                    }
                }
            }

            meshShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *meshShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomMesh.");
            return false;
        }         
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or MeshShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParsePlaneShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, PlaneShapeDesc* planeShapeDesc)
{
    if (planeShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomPlane shape(usdPrim);
        if (shape)
        {
            PhysicsAxis::Enum axis = PhysicsAxis::eX;     

            TfToken tfAxis;            
            shape.GetAxisAttr().Get(&tfAxis);
            if (tfAxis == UsdPhysicsTokens.Get()->y)
            {
                axis = PhysicsAxis::eY;
            }
            else if (tfAxis == UsdPhysicsTokens.Get()->z)
            {
                axis = PhysicsAxis::eZ;
            }

            planeShapeDesc->axis = axis;            
            planeShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *planeShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomPlane.");
            return false;
        }           
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or PlaneShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseSpherePointsShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, SpherePointsShapeDesc* spherePointsShapeDesc)
{
    if (spherePointsShapeDesc && collisionAPI)
    {       
        const UsdPrim usdPrim = collisionAPI.GetPrim();
        const UsdGeomPoints shape(usdPrim);
        if (shape)
        {
            const pxr::GfTransform tr(shape.ComputeLocalToWorldTransform(UsdTimeCode::Default()));

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
                        const pxr::GfVec3d sc = tr.GetScale();

                        // as we don't support scale in physics and scale can be non uniform
                        // we pick the largest scale value as the sphere radius base
                        CheckNonUniformScale(sc, usdPrim.GetPrimPath());
                        sphereScale = fmaxf(fmaxf(fabsf(float(sc[1])), fabsf(float(sc[0]))), fabsf(float(sc[2])));
                    }
                    
                    const size_t scount = positions.size();
                    spherePointsShapeDesc->spherePoints.resize(scount);
                    for (size_t i=0; i<scount; i++)
                    {
                        spherePointsShapeDesc->spherePoints[i].radius = sphereScale * widths[i] * 0.5f;
                        spherePointsShapeDesc->spherePoints[i].center = positions[i];
                    }                                        
                }
                else
                {
                    TF_DIAGNOSTIC_WARNING("UsdGeomPoints width array size does not match position array size: %s", usdPrim.GetPrimPath().GetText());
                    spherePointsShapeDesc->isValid = false;
                }
            }
            else
            {
                TF_DIAGNOSTIC_WARNING("UsdGeomPoints width array not filled: %s", usdPrim.GetPrimPath().GetText());
                spherePointsShapeDesc->isValid = false;
            }
            
            spherePointsShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

            FinalizeDesc(collisionAPI, *spherePointsShapeDesc);
        }
        else
        {
            TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI is not applied to a UsdGeomPoints.");
            return false;
        }        
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or SpherePointsShapeDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseCustomShapeDesc(const UsdPhysicsCollisionAPI& collisionAPI, CustomShapeDesc* customShapeDesc)
{
    if (customShapeDesc && collisionAPI)
    {       

        customShapeDesc->primPath = collisionAPI.GetPrim().GetPrimPath();

        FinalizeDesc(collisionAPI, *customShapeDesc);
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionAPI or CustomShapeDesc is not valid.");
        return false;

    }
    return true;
}


bool ParseCollisionGroupDesc(const UsdPhysicsCollisionGroup& collisionGroup, CollisionGroupDesc* collisionGroupDesc)
{
    if (collisionGroup && collisionGroupDesc)
    {        
        const UsdRelationship rel = collisionGroup.GetFilteredGroupsRel();
        if (rel)
        {
            rel.GetTargets(&collisionGroupDesc->filteredGroups);
        }

        collisionGroup.GetInvertFilteredGroupsAttr().Get(&collisionGroupDesc->invertFilteredGroups);
        collisionGroup.GetMergeGroupNameAttr().Get(&collisionGroupDesc->mergeGroupName);

        collisionGroupDesc->primPath = collisionGroup.GetPrim().GetPrimPath();
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsCollisionGroup or CollisionGroupDesc is not valid.");
        return false;
    }

    return true;
}

SdfPath GetRel(const pxr::UsdRelationship& ref, const pxr::UsdPrim& jointPrim)
{
    pxr::SdfPathVector targets;
    ref.GetTargets(&targets);

    if (targets.size() == 0)
    {
        return SdfPath();
    }
    if (targets.size() > 1)
    {
        TF_DIAGNOSTIC_WARNING("Joint prim does have relationship to multiple bodies this is not supported, jointPrim %s", jointPrim.GetPrimPath().GetText());
        return targets.at(0);
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
        TF_RUNTIME_ERROR("Joint (%s) body relationship %s points to a non existent prim, joint will not be parsed.", jointPrim.GetPrimPath().GetText(), relPath.GetText());
        return false;
    }
    return true;
}

pxr::UsdPrim GetBodyPrim(UsdStageWeakPtr stage, const SdfPath& relPath, UsdPrim& relPrim)
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

SdfPath GetLocalPose(UsdStageWeakPtr stage, const SdfPath& relPath, GfVec3f& t, GfQuatf& q)
{
    UsdPrim relPrim;    
    const UsdPrim body = GetBodyPrim(stage, relPath, relPrim);    

    // get scale and apply it into localPositions vectors
    const UsdGeomXformable xform(relPrim);
    const GfMatrix4d worldRel = relPrim ? xform.ComputeLocalToWorldTransform(UsdTimeCode::Default()) : GfMatrix4d(1.0);

    // we need to apply scale to the localPose, the scale comes from the rigid body
    GfVec3f sc;
    // if we had a rel not to rigid body, we need to recompute the localPose
    if (relPrim != body)
    {
        GfMatrix4d localAnchor;
        localAnchor.SetIdentity();
        localAnchor.SetTranslate(GfVec3d(t));
        localAnchor.SetRotateOnly(GfQuatd(q));

        GfMatrix4d bodyMat;
        if (body)
            bodyMat = UsdGeomXformable(body).ComputeLocalToWorldTransform(UsdTimeCode::Default());
        else
            bodyMat.SetIdentity();
        
        const GfMatrix4d worldAnchor = localAnchor * worldRel;
        GfMatrix4d bodyLocalAnchor = worldAnchor * bodyMat.GetInverse();
        bodyLocalAnchor = bodyLocalAnchor.RemoveScaleShear();

        t = GfVec3f(bodyLocalAnchor.ExtractTranslation());
        q = GfQuatf(bodyLocalAnchor.ExtractRotationQuat());
        q.Normalize();

        const GfTransform tr(bodyMat);
        sc = GfVec3f(tr.GetScale());
    }
    else
    {
        const GfTransform tr(worldRel);
        sc = GfVec3f(tr.GetScale());
    }

    // apply the scale, this is not obvious, but in physics there is no scale, so we need to
    // apply it before its send to physics
    for (int i = 0; i < 3; i++)
    {
        t[i] *= sc[i];
    }

    return body ? body.GetPrimPath() : SdfPath();
}

void FinalizeJoint(const UsdPhysicsJoint& jointPrim, JointDesc* jointDesc)
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
    if (jointDesc->rel0 != SdfPath())
    {
        jointDesc->body0 = GetLocalPose(stage, jointDesc->rel0, t0, q0);
    }

    if (jointDesc->rel1 != SdfPath())
    {
        jointDesc->body1 = GetLocalPose(stage, jointDesc->rel1, t1, q1);
    }

    jointDesc->localPose0Position = t0;
    jointDesc->localPose0Orientation = q0;
    jointDesc->localPose1Position = t1;
    jointDesc->localPose1Orientation = q1;
}

bool ParseCommonJointDesc(const UsdPhysicsJoint& jointPrim, JointDesc* jointDesc)
{
    const UsdPrim prim = jointPrim.GetPrim();

    jointDesc->primPath = prim.GetPrimPath();

    // parse the joint common parameters
    jointPrim.GetJointEnabledAttr().Get(&jointDesc->jointEnabled);
    jointPrim.GetCollisionEnabledAttr().Get(&jointDesc->collisionEnabled);
    jointPrim.GetBreakForceAttr().Get(&jointDesc->breakForce);            
    jointPrim.GetBreakTorqueAttr().Get(&jointDesc->breakTorque);
    jointPrim.GetExcludeFromArticulationAttr().Get(&jointDesc->excludeFromArticulation);

    jointDesc->rel0 = GetRel(jointPrim.GetBody0Rel(), prim);
    jointDesc->rel1 = GetRel(jointPrim.GetBody1Rel(), prim);

    // check rel validity
    if (!CheckJointRel(jointDesc->rel0, prim) || !CheckJointRel(jointDesc->rel1, prim))
    {
        return false;
    }

    FinalizeJoint(jointPrim, jointDesc);

    return true;
}

bool ParseDistanceJointDesc(const UsdPhysicsDistanceJoint& distanceJoint, DistanceJointDesc* distanceJointDesc)
{
    if (distanceJointDesc && distanceJoint)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(distanceJoint, distanceJointDesc))
        {
            return false;
        }

        distanceJointDesc->maxEnabled = false;
        distanceJointDesc->minEnabled = false;
        distanceJoint.GetMinDistanceAttr().Get(&distanceJointDesc->limit.minDist);
        distanceJoint.GetMaxDistanceAttr().Get(&distanceJointDesc->limit.maxDist);
            
        if (distanceJointDesc->limit.minDist >= 0.0f)
        {
            distanceJointDesc->minEnabled = true;
                
        }
        if (distanceJointDesc->limit.maxDist >= 0.0f)
        {
            distanceJointDesc->maxEnabled = true;
                
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsDistanceJoint or DistanceJointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseDrive(const UsdPhysicsDriveAPI& drive, JointDrive* jointDrive)
{
    if (drive && jointDrive)
    {
        drive.GetTargetPositionAttr().Get(&jointDrive->targetPosition);
        drive.GetTargetVelocityAttr().Get(&jointDrive->targetVelocity);
        drive.GetMaxForceAttr().Get(&jointDrive->forceLimit);    

        drive.GetDampingAttr().Get(&jointDrive->damping);
        drive.GetStiffnessAttr().Get(&jointDrive->stiffness);    

        TfToken typeToken;
        drive.GetTypeAttr().Get(&typeToken);
        if (typeToken == UsdPhysicsTokens->acceleration)
            jointDrive->acceleration = true;
        jointDrive->enabled = true;
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsDriveAPI or JointDrive is not valid.");
        return false;
    }

    return true;
}

bool ParseFixedJointDesc(const UsdPhysicsFixedJoint& fixedJoint, FixedJointDesc* fixedJointDesc)
{
    if (fixedJointDesc && fixedJoint)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(fixedJoint, fixedJointDesc))
        {
            return false;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsFixedJoint or FixedJointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseLimit(const UsdPhysicsLimitAPI& limit, JointLimit* jointLimit)
{
    if (limit && jointLimit)
    {
        limit.GetLowAttr().Get(&jointLimit->lower);
        limit.GetHighAttr().Get(&jointLimit->upper);
        if ((isfinite(jointLimit->lower) && jointLimit->lower > -physicsSentinelLimit) ||
            (isfinite(jointLimit->upper) && jointLimit->upper < physicsSentinelLimit))
                jointLimit->enabled = true;
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsLimitAPI or JointLimit is not valid.");
        return false;
    }

    return true;
}

bool ParseD6JointDesc(const UsdPhysicsJoint& jointPrim, D6JointDesc* jointDesc)
{    
    if (jointDesc && jointPrim)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(jointPrim, jointDesc))
        {
            return false;
        }

        // D6 joint        
        const std::array<std::pair<PhysicsJointDOF::Enum, TfToken>, 7> axisVector = {
            std::make_pair(PhysicsJointDOF::eDistance, UsdPhysicsTokens->distance), std::make_pair(PhysicsJointDOF::eTransX, UsdPhysicsTokens->transX),
            std::make_pair(PhysicsJointDOF::eTransY, UsdPhysicsTokens->transY),     std::make_pair(PhysicsJointDOF::eTransZ, UsdPhysicsTokens->transZ),
            std::make_pair(PhysicsJointDOF::eRotX, UsdPhysicsTokens->rotX),         std::make_pair(PhysicsJointDOF::eRotY, UsdPhysicsTokens->rotY),
            std::make_pair(PhysicsJointDOF::eRotZ, UsdPhysicsTokens->rotZ)
        };

        for (size_t i = 0; i < axisVector.size(); i++)
        {
            const TfToken& axisToken = axisVector[i].second;

            const UsdPhysicsLimitAPI limitAPI = UsdPhysicsLimitAPI::Get(jointPrim.GetPrim(), axisToken);
            if (limitAPI)
            {
                JointLimit limit;
                if (ParseLimit(limitAPI, &limit))
                {
                    jointDesc->jointLimits.push_back(std::make_pair(axisVector[i].first, limit));
                }                
            }

            const UsdPhysicsDriveAPI driveAPI = UsdPhysicsDriveAPI::Get(jointPrim.GetPrim(), axisToken);
            if (driveAPI)
            {
                JointDrive drive;
                if (ParseDrive(driveAPI, &drive))
                {
                    jointDesc->jointDrives.push_back(std::make_pair(axisVector[i].first, drive));
                }
            }
        }    
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsJoint or JointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseCustomJointDesc(const UsdPhysicsJoint& jointPrim, CustomJointDesc* customJointDesc)
{
    if (customJointDesc && jointPrim)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(jointPrim, customJointDesc))
        {
            return false;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsJoint or JointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseRigidBodyMaterialDesc(const UsdPhysicsMaterialAPI& usdMaterial, RigidBodyMaterialDesc* rbMaterialDesc)
{
    if (rbMaterialDesc && usdMaterial)
    {       
        usdMaterial.GetDynamicFrictionAttr().Get(&rbMaterialDesc->dynamicFriction);
        usdMaterial.GetStaticFrictionAttr().Get(&rbMaterialDesc->staticFriction);

        usdMaterial.GetRestitutionAttr().Get(&rbMaterialDesc->restitution);

        usdMaterial.GetDensityAttr().Get(&rbMaterialDesc->density);            

        rbMaterialDesc->primPath = usdMaterial.GetPrim().GetPrimPath();
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsMaterialAPI or RigidBodyMaterialDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseLinearDrive(JointDrive* dst, const UsdPrim& usdPrim)
{
    dst->enabled = false;
    const UsdPhysicsDriveAPI driveAPI = UsdPhysicsDriveAPI::Get(usdPrim, UsdPhysicsTokens->linear);
    if (driveAPI)
    {
        return ParseDrive(driveAPI, dst);
    }

    return true;
}


bool ParsePrismaticJointDesc(const UsdPhysicsPrismaticJoint& prismaticJoint, PrismaticJointDesc* prismaticJointDesc)
{
    if (prismaticJointDesc && prismaticJoint)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(prismaticJoint, prismaticJointDesc))
        {
            return false;
        }

        PhysicsAxis::Enum jointAxis = PhysicsAxis::eX;
        TfToken axis = UsdPhysicsTokens->x;            
        prismaticJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = PhysicsAxis::eY;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = PhysicsAxis::eZ;
        prismaticJointDesc->axis = jointAxis;

        prismaticJointDesc->limit.enabled = false;
        prismaticJoint.GetLowerLimitAttr().Get(&prismaticJointDesc->limit.lower);
        prismaticJoint.GetUpperLimitAttr().Get(&prismaticJointDesc->limit.upper);
        if ((isfinite(prismaticJointDesc->limit.lower) && (prismaticJointDesc->limit.lower > -physicsSentinelLimit)) || 
            (isfinite(prismaticJointDesc->limit.upper) && (prismaticJointDesc->limit.upper < physicsSentinelLimit)))
        {
            prismaticJointDesc->limit.enabled = true;
        }

        if (!ParseLinearDrive(&prismaticJointDesc->drive, prismaticJoint.GetPrim()))
        {
            return false;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsPrismaticJoint or PrismaticJointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseAngularDrive(JointDrive* dst, const UsdPrim& usdPrim)
{
    dst->enabled = false;
    const UsdPhysicsDriveAPI driveAPI = UsdPhysicsDriveAPI::Get(usdPrim, UsdPhysicsTokens->angular);
    if (driveAPI)
    {
        return ParseDrive(driveAPI, dst);
    }

    return true;
}


bool ParseRevoluteJointDesc(const UsdPhysicsRevoluteJoint& revoluteJoint, RevoluteJointDesc* revoluteJointDesc)
{
    if (revoluteJointDesc && revoluteJoint)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(revoluteJoint, revoluteJointDesc))
        {
            return false;
        }

        PhysicsAxis::Enum jointAxis = PhysicsAxis::eX;
        TfToken axis = UsdPhysicsTokens->x;
        revoluteJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = PhysicsAxis::eY;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = PhysicsAxis::eZ;
        revoluteJointDesc->axis = jointAxis;

        revoluteJointDesc->limit.enabled = false;

        
        revoluteJoint.GetLowerLimitAttr().Get(&revoluteJointDesc->limit.lower);
        revoluteJoint.GetUpperLimitAttr().Get(&revoluteJointDesc->limit.upper);
        if (isfinite(revoluteJointDesc->limit.lower) && isfinite(revoluteJointDesc->limit.upper)
            && revoluteJointDesc->limit.lower > -physicsSentinelLimit && revoluteJointDesc->limit.upper < physicsSentinelLimit)
        {
            revoluteJointDesc->limit.enabled = true;
        }

        if (!ParseAngularDrive(&revoluteJointDesc->drive, revoluteJoint.GetPrim()))
        {
            return false;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsJoint or JointDesc is not valid.");
        return false;
    }

    return true;
}

template<typename T>
inline bool ScaleIsUniform(T scaleX, T scaleY, T scaleZ, T eps = T(1.0e-5))
{
    // Find min and max scale values
    T lo, hi;

    if (scaleX < scaleY)
    {
        lo = scaleX;
        hi = scaleY;
    }
    else
    {
        lo = scaleY;
        hi = scaleX;
    }
    
    if (scaleZ < lo)
    {
        lo = scaleZ;
    }
    else if (scaleZ > hi)
    {
        hi = scaleZ;
    }
    
    if (lo*hi < 0.0)
    {
        return false;   // opposite signs
    }

    return hi > 0.0 ? hi - lo <= eps*lo : lo - hi >= eps*hi;
}


void GetRigidBodyTransformation(const UsdPrim& bodyPrim, RigidBodyDesc& desc)
{
    const GfMatrix4d mat = UsdGeomXformable(bodyPrim).ComputeLocalToWorldTransform(UsdTimeCode::Default());
    const GfTransform tr(mat);
    const GfVec3d pos = tr.GetTranslation();
    const GfQuatd rot = tr.GetRotation().GetQuat();
    const GfVec3d sc = tr.GetScale();

    if (!ScaleIsUniform(sc[0], sc[1], sc[2]) && tr.GetScaleOrientation().GetQuaternion() != GfQuaternion::GetIdentity())
    {
        TF_DIAGNOSTIC_WARNING("ScaleOrientation is not supported for rigid bodies, prim path: %s. You may ignore this if the scale is close to uniform.", bodyPrim.GetPrimPath().GetText());
    }

    desc.position = GfVec3f(pos);
    desc.rotation = GfQuatf(rot);
    desc.scale = GfVec3f(sc);
}

bool ParseRigidBodyDesc(const UsdPhysicsRigidBodyAPI& rigidBodyAPI, RigidBodyDesc* rigidBodyDesc)
{
    if (rigidBodyDesc && rigidBodyAPI)
    {       
        if (!rigidBodyAPI.GetPrim().IsA<UsdGeomXformable>())
        {
            TF_DIAGNOSTIC_WARNING("RigidBodyAPI applied to a non-xformable primitive. (%s)", rigidBodyAPI.GetPrim().GetPrimPath().GetText());
            return false;
        }

        // check instancing
        {
            bool reportInstanceError = false;
            if (rigidBodyAPI.GetPrim().IsInstanceProxy())
            {
                reportInstanceError = true;

                bool kinematic = false;
                rigidBodyAPI.GetKinematicEnabledAttr().Get(&kinematic);
                if (kinematic)
                    reportInstanceError = false;

                bool enabled = false;
                rigidBodyAPI.GetRigidBodyEnabledAttr().Get(&enabled);
                if (!enabled)
                    reportInstanceError = false;

                if (reportInstanceError)
                {
                    TF_DIAGNOSTIC_WARNING("RigidBodyAPI on an instance proxy not supported. %s", rigidBodyAPI.GetPrim().GetPrimPath().GetText());
                    return false;
                }
            }
        }

        // transformation
        GetRigidBodyTransformation(rigidBodyAPI.GetPrim(), *rigidBodyDesc);

        // filteredPairs
        ParseFilteredPairs(rigidBodyAPI.GetPrim(), rigidBodyDesc->filteredCollisions);

        // velocity
        rigidBodyAPI.GetVelocityAttr().Get(&rigidBodyDesc->linearVelocity);
        rigidBodyAPI.GetAngularVelocityAttr().Get(&rigidBodyDesc->angularVelocity);

        // rigid body flags
        rigidBodyAPI.GetRigidBodyEnabledAttr().Get(&rigidBodyDesc->rigidBodyEnabled);
        rigidBodyAPI.GetKinematicEnabledAttr().Get(&rigidBodyDesc->kinematicBody);
        rigidBodyAPI.GetStartsAsleepAttr().Get(&rigidBodyDesc->startsAsleep);

        // simulation owner
        const UsdRelationship ownerRel = rigidBodyAPI.GetSimulationOwnerRel();
        if (ownerRel)
        {
            SdfPathVector owners;
            ownerRel.GetTargets(&owners);
            if (!owners.empty())
            {
                rigidBodyDesc->simulationOwners = owners;
            }
        }
        rigidBodyDesc->primPath = rigidBodyAPI.GetPrim().GetPrimPath();
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsRigidBodyAPI or RigidBodyDesc is not valid.");
        return false;

    }
    return true;
}

bool ParseSphericalJointDesc(const UsdPhysicsSphericalJoint& sphericalJoint, SphericalJointDesc* sphericalJointDesc)
{
    if (sphericalJointDesc && sphericalJoint)
    {
        // parse the joint common parameters
        if (!ParseCommonJointDesc(sphericalJoint, sphericalJointDesc))
        {
            return false;
        }

        PhysicsAxis::Enum jointAxis = PhysicsAxis::eX;
        TfToken axis = UsdPhysicsTokens->x;
        sphericalJoint.GetAxisAttr().Get(&axis);

        if (axis == UsdPhysicsTokens->y)
            jointAxis = PhysicsAxis::eY;
        else if (axis == UsdPhysicsTokens->z)
            jointAxis = PhysicsAxis::eZ;
        sphericalJointDesc->axis = jointAxis;

        sphericalJointDesc->limit.enabled = false;
        sphericalJoint.GetConeAngle0LimitAttr().Get(&sphericalJointDesc->limit.angle0);
        sphericalJoint.GetConeAngle1LimitAttr().Get(&sphericalJointDesc->limit.angle1);

        if (isfinite(sphericalJointDesc->limit.angle0) && isfinite(sphericalJointDesc->limit.angle1)
            && sphericalJointDesc->limit.angle0 >= 0.0 && sphericalJointDesc->limit.angle1 >= 0.0)
        {
            sphericalJointDesc->limit.enabled = true;
        }
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsSphericalJoint or SphericalJointDesc is not valid.");
        return false;
    }

    return true;
}

bool ParseSceneDesc(const UsdPhysicsScene& scene, SceneDesc* sceneDesc)
{
    if (sceneDesc && scene)
    {
        UsdStageWeakPtr stage = scene.GetPrim().GetStage();

        GfVec3f gravityDirection;
        scene.GetGravityDirectionAttr().Get(&gravityDirection);
        if (gravityDirection == GfVec3f(0.0f))
        {
            TfToken upAxis = pxr::UsdGeomGetStageUpAxis(stage);
            if (upAxis == pxr::UsdGeomTokens.Get()->x)
                gravityDirection = GfVec3f(-1.0f, 0.0f, 0.0f);
            else if (upAxis == pxr::UsdGeomTokens.Get()->y)
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
        if (gravityMagnitude < -0.5e38f)
        {
            float metersPerUnit = (float)pxr::UsdGeomGetStageMetersPerUnit(stage);
            gravityMagnitude = 9.81f / metersPerUnit;
        }

        sceneDesc->gravityMagnitude = gravityMagnitude;
        sceneDesc->gravityDirection = gravityDirection;
        sceneDesc->primPath = scene.GetPrim().GetPrimPath();
    }
    else
    {
        TF_RUNTIME_ERROR("Provided UsdPhysicsScene or SceneDesc is not valid.");
        return false;
    }
    return true;
}

struct SchemaAPIFlag
{
    enum Enum
    {
        eArticulationRootAPI = 1 << 0,
        eCollisionAPI   = 1 << 1,
        eRigidBodyAPI   = 1 << 2,
        eMaterialAPI    = 1 << 3
    };
};

bool CheckNestedArticulationRoot(const pxr::UsdPrim& usdPrim, const std::unordered_set<SdfPath, SdfPath::Hash>& articulationSet)
{
    UsdPrim parent = usdPrim.GetParent();
    while (parent && parent != usdPrim.GetStage()->GetPseudoRoot())
    {
        if (articulationSet.find(parent.GetPrimPath()) != articulationSet.end())
            return true;
        parent = parent.GetParent();
    }

    return false;
}

using RigidBodyMap = std::map<pxr::SdfPath, RigidBodyDesc*>;

bool IsDynamicBody(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap, bool& physicsAPIFound)
{
    RigidBodyMap::const_iterator it = bodyMap.find(usdPrim.GetPrimPath());
    if (it != bodyMap.end())
    {
        {
            bool isAPISchemaEnabled = it->second->rigidBodyEnabled;

            // Prim is dynamic body off PhysicsAPI is present and enabled
            physicsAPIFound = true;
            return isAPISchemaEnabled;
        }
    }

    physicsAPIFound = false;
    return false;
}


bool HasDynamicBodyParent(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap, UsdPrim& bodyPrimPath)
{
    bool physicsAPIFound = false;    
    UsdPrim parent = usdPrim;
    while (parent != usdPrim.GetStage()->GetPseudoRoot())
    {
        if (IsDynamicBody(parent, bodyMap, physicsAPIFound))
        {
            bodyPrimPath = parent;
            return true;
        }

        if (physicsAPIFound)
        {
            bodyPrimPath = parent;
            return false;
        }

        parent = parent.GetParent();
    }
    return false;
}

template <typename DescType, typename UsdType> 
void ProcessPhysicsPrims(const std::vector<UsdPrim>& physicsPrims, std::vector<DescType>& physicsDesc, 
    std::function<bool(const UsdType& prim, DescType* desc)> processDescFn)
{
    if (!physicsPrims.empty())
    {
        physicsDesc.resize(physicsPrims.size());
        for (size_t i = 0; i < physicsPrims.size(); i++)
        {
            const UsdType prim(physicsPrims[i]);
            const bool ret = processDescFn(prim, &physicsDesc[i]);
            if (!ret)
            {
                physicsDesc[i].isValid = false;
            }
        }
    }
}

template <typename DescType> 
void CallReportFn(PhysicsObjectType::Enum descType, const std::vector<UsdPrim>& physicsPrims, const std::vector<DescType>& physicsDesc, 
    UsdPhysicsReportFn reportFn, SdfPathVector& primPathsVector, void* userData)
{
    primPathsVector.resize(physicsPrims.size());
    for (size_t i = 0; i < physicsPrims.size(); i++)
    {
        primPathsVector[i] = physicsPrims[i].GetPrimPath();
    }
    reportFn(descType, primPathsVector.size(), primPathsVector.data(), physicsDesc.data(), userData);
}

void CheckRigidBodySimulationOwner(std::vector<UsdPrim>& rigidBodyPrims, std::vector<RigidBodyDesc>& rigidBodyDescs, 
    bool defaultSimulationOwner, std::unordered_set<SdfPath, SdfPath::Hash>& reportedBodies,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = rigidBodyDescs.size(); i--;)
    {
        bool ownerFound = false;
        const RigidBodyDesc& desc = rigidBodyDescs[i];
        if (desc.isValid)
        {
            if (desc.simulationOwners.empty() && defaultSimulationOwner)
            {                
                reportedBodies.insert(desc.primPath);
                ownerFound = true;
            }
            else
            {
                for (const SdfPath& owner : desc.simulationOwners)
                {
                    if (simulationOwnersSet.find(owner) != simulationOwnersSet.end())
                    {
                        reportedBodies.insert(desc.primPath);
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
void CheckCollisionSimulationOwner(std::vector<UsdPrim>& collisionPrims, std::vector<DescType>& shapeDesc, 
    bool defaultSimulationOwner, const std::unordered_set<SdfPath, SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = shapeDesc.size(); i--;)
    {
        bool ownerFound = false;
        const ShapeDesc& desc = shapeDesc[i];
        if (desc.isValid)
        {
            if (desc.rigidBody != SdfPath() && rigidBodiesSet.find(desc.rigidBody) != rigidBodiesSet.end())
            {        if (desc.rigidBody != SdfPath() && rigidBodiesSet.find(desc.rigidBody) != rigidBodiesSet.end())
        {
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
                        if (simulationOwnersSet.find(owner) != simulationOwnersSet.end())
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
                            if (simulationOwnersSet.find(owner) != simulationOwnersSet.end())
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
void CheckJointSimulationOwner(std::vector<UsdPrim>& jointPrims, std::vector<DescType>& jointDesc, 
    bool defaultSimulationOwner, const std::unordered_set<SdfPath, SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = jointDesc.size(); i--;)
    {
        const JointDesc& desc = jointDesc[i];

        bool ownersValid = false;
        if (desc.isValid)
        {
            if ((desc.body0 == SdfPath() || rigidBodiesSet.find(desc.body0) != rigidBodiesSet.end()) &&
                (desc.body1 == SdfPath() || rigidBodiesSet.find(desc.body1) != rigidBodiesSet.end()))
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
void CheckArticulationSimulationOwner(std::vector<UsdPrim>& articulationPrims, std::vector<ArticulationDesc>& articulationDescs, 
    bool defaultSimulationOwner, const std::unordered_set<SdfPath, SdfPath::Hash>& rigidBodiesSet,
    const std::unordered_set<SdfPath, SdfPath::Hash>& simulationOwnersSet)
{
    for (size_t i = articulationDescs.size(); i--;)
    {
        const ArticulationDesc& desc = articulationDescs[i];

        bool ownersValid = true;
        if (desc.isValid)
        {
            for (const SdfPath& body : desc.articulatedBodies)
            {
                if (body != SdfPath() && rigidBodiesSet.find(body) == rigidBodiesSet.end())
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

SdfPath GetRigidBody(const UsdPrim& usdPrim, const RigidBodyMap& bodyMap)
{
    UsdPrim bodyPrim = UsdPrim();
    RigidBodyDesc* body = nullptr;
    if (HasDynamicBodyParent(usdPrim, bodyMap, bodyPrim))
    {
       return bodyPrim.GetPrimPath();
    }
    else
    {
        // collision does not have a dynamic body parent, it is considered a static collision        
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

void GetCollisionShapeLocalTransfrom(UsdGeomXformCache& xfCache, const UsdPrim& collisionPrim, const UsdPrim& bodyPrim,
                                            GfVec3f& localPosOut,
                                            GfQuatf& localRotOut,
                                            GfVec3f& localScaleOut)
{
    // compute the shape rel transform to a body and store it.
    pxr::GfVec3f localPos(0.0f);
    if (collisionPrim != bodyPrim)
    {
        bool resetXformStack;
        const pxr::GfMatrix4d mat = xfCache.ComputeRelativeTransform(collisionPrim, bodyPrim, &resetXformStack);
        GfTransform colLocalTransform(mat);

        localPos = pxr::GfVec3f(colLocalTransform.GetTranslation());
        localRotOut = pxr::GfQuatf(colLocalTransform.GetRotation().GetQuat());
        localScaleOut = pxr::GfVec3f(colLocalTransform.GetScale());
    }
    else
    {
        const pxr::GfMatrix4d mat(1.0);

        localRotOut = pxr::GfQuatf(1.0f);
        localScaleOut = pxr::GfVec3f(1.0f);
    }

    // now apply the body scale to localPos
    // physics does not support scales, so a rigid body scale has to be baked into the localPos
    const pxr::GfTransform tr(xfCache.GetLocalToWorldTransform(bodyPrim));
    const pxr::GfVec3d sc = tr.GetScale();

    for (int i = 0; i < 3; i++)
    {
        localPos[i] *= (float)sc[i];
    }

    localPosOut = localPos;
}

void FinalizeCollision(UsdStageWeakPtr stage, UsdGeomXformCache& xfCache, const RigidBodyDesc* bodyDesc, ShapeDesc* shapeDesc)
{
    // get shape local pose
    const UsdPrim shapePrim = stage->GetPrimAtPath(shapeDesc->primPath);
    GetCollisionShapeLocalTransfrom(xfCache, shapePrim, bodyDesc ? stage->GetPrimAtPath(bodyDesc->primPath) : stage->GetPseudoRoot(),
        shapeDesc->localPos, shapeDesc->localRot, shapeDesc->localScale);

    if (bodyDesc)
    {
        shapeDesc->rigidBody = bodyDesc->primPath;
    }
}



template <typename DescType> 
void FinalizeCollisionDescs(UsdGeomXformCache& xfCache, const std::vector<UsdPrim>& physicsPrims, std::vector<DescType>& physicsDesc, 
    RigidBodyMap& bodyMap, const std::map<SdfPath, std::unordered_set<SdfPath, SdfPath::Hash>>& collisionGroups)
{
    for (size_t i = 0; i < physicsDesc.size(); i++)
    {
        DescType& colDesc = physicsDesc[i];
        if (colDesc.isValid)
        {
            const UsdPrim prim = physicsPrims[i];
            // get the body
            SdfPath bodyPath = GetRigidBody(prim, bodyMap);
            // body was found, add collision to the body
            RigidBodyDesc* bodyDesc = nullptr;
            if (bodyPath != SdfPath())
            {
                RigidBodyMap::iterator bodyIt = bodyMap.find(bodyPath);
                if (bodyIt != bodyMap.end())
                {
                    bodyDesc = bodyIt->second;
                    bodyDesc->collisions.push_back(colDesc.primPath);
                }
            }

            // check if collision belongs to collision groups
            for (std::map<SdfPath, std::unordered_set<SdfPath, SdfPath::Hash>>::const_iterator it = collisionGroups.begin();
             it != collisionGroups.end(); ++it)
            {
                if (it->second.find(colDesc.primPath) != it->second.end())
                {
                    colDesc.collisionGroups.push_back(it->first);
                }
            }

            // finalize the collision, fill up the local transform etc
            FinalizeCollision(prim.GetStage(), xfCache, bodyDesc, &colDesc);
        }
    }
}

struct ArticulationLink
{ 
    SdfPathVector   childs;
    SdfPath         rootJoint;
    uint32_t        weight;
    uint32_t        index;
    bool            hasFixedJoint;
    SdfPathVector   joints;
};

using ArticulationLinkMap = std::map<pxr::SdfPath, ArticulationLink>;
using BodyJointMap = pxr::TfHashMap<pxr::SdfPath, std::vector<const JointDesc*>, pxr::SdfPath::Hash>;
using JointMap = std::map<pxr::SdfPath, JointDesc*>;
using ArticulationMap = std::map<pxr::SdfPath, ArticulationDesc*>;

bool IsInLinkMap(const SdfPath& path, const std::vector<ArticulationLinkMap>& linkMaps)
{
    for (size_t i = 0; i < linkMaps.size(); i++)
    {
        ArticulationLinkMap::const_iterator it = linkMaps[i].find(path);
        if (it != linkMaps[i].end())
            return true;
    }

    return false;
}

void TraverseHierarchy(const pxr::UsdStageWeakPtr stage, const SdfPath& linkPath, ArticulationLinkMap& articulationLinkMap, const BodyJointMap& bodyJointMap, uint32_t& index, SdfPathVector& linkOrderVector)
{
    // check if we already parsed this link
    ArticulationLinkMap::const_iterator artIt = articulationLinkMap.find(linkPath);
    if (artIt != articulationLinkMap.end())
        return;

    linkOrderVector.push_back(linkPath);

    BodyJointMap::const_iterator bjIt = bodyJointMap.find(linkPath);
    if (bjIt != bodyJointMap.end())
    {
        ArticulationLink& link = articulationLinkMap[linkPath];
        link.weight = 0;
        link.index = index++;
        link.hasFixedJoint = false;
        const std::vector<const JointDesc*>& joints = bjIt->second;
        for (size_t i = 0; i < joints.size(); i++)
        {            
            const JointDesc* desc = joints[i];
            link.joints.push_back(desc->primPath);
            if (desc->body0 == SdfPath() || (bodyJointMap.find(desc->body0) == bodyJointMap.end()) ||
                desc->body1 == SdfPath() || (bodyJointMap.find(desc->body1) == bodyJointMap.end()))
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
                link.childs.push_back(SdfPath());                                
            }
            else
            {
                if (desc->excludeFromArticulation)
                {
                    link.childs.push_back(desc->body0 == linkPath ? desc->body1 : desc->body0);
                    link.weight += 1000;
                }
                else
                {
                    link.childs.push_back(desc->body0 == linkPath ? desc->body1 : desc->body0);
                    link.weight += 100;
                    TraverseHierarchy(stage, link.childs.back(), articulationLinkMap, bodyJointMap, index, linkOrderVector);
                }
            }
        }
    }
}

void TraverseChilds(const ArticulationLink& link, const ArticulationLinkMap& map, uint32_t startIndex, uint32_t distance, int32_t* pathMatrix)
{
    const size_t mapSize = map.size();
    const uint32_t currentIndex = link.index;
    pathMatrix[startIndex + currentIndex * mapSize] = distance;

    for (size_t i = 0; i < link.childs.size(); i++)
    {
        ArticulationLinkMap::const_iterator it = map.find(link.childs[i]);
        if (it != map.end())
        {
            const uint32_t childIndex = it->second.index;
            if (pathMatrix[startIndex + childIndex * mapSize] < 0)
            {
                TraverseChilds(it->second, map, startIndex, distance + 1, pathMatrix);
            }
        }        
    }
}

pxr::SdfPath GetCenterOfGraph(const ArticulationLinkMap& map, const SdfPathVector& linkOrderVector)
{
    const size_t size = map.size();
    int32_t* pathMatrix = new int32_t[size * size];
    for (size_t i = 0; i < size; i ++)
    {
        for (size_t j = 0; j < size; j++)
        {
            pathMatrix [i + j * size] = -1;
        }
    }

    for (ArticulationLinkMap::const_reference& ref : map)
    {
        const uint32_t startIndex = ref.second.index;
        uint32_t distance = 0;
        TraverseChilds(ref.second, map, startIndex, distance, pathMatrix);
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

        // this needs to be deterministic, get the shortest path
        // if there are more paths with same lenght, pick the one with more childs
        // if there are more with same path and same amount of childs, pick the one with lowest hash
        // The lowest hash is not right, this is wrong, it has to be the first link ordered by the traversal
        if (longestPath < shortestDistance)
        {
            shortestDistance = longestPath;
            numChilds = ref.second.childs.size();
            primpath = ref.first;
        }
        else if (longestPath == shortestDistance)
        {
            if (numChilds < ref.second.childs.size())
            {
                numChilds = ref.second.childs.size();
                primpath = ref.first;
            }
            else if (numChilds == ref.second.childs.size())
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

    delete [] pathMatrix;

    return primpath;
}

void FinalizeArticulations(const pxr::UsdStageWeakPtr stage, ArticulationMap& articulationMap, const RigidBodyMap& rigidBodyMap, const JointMap& jointMap)
{
    BodyJointMap bodyJointMap;
    if (!articulationMap.empty())
    {
        // construct the BodyJointMap
        bodyJointMap.reserve(rigidBodyMap.size());
        for (JointMap::const_reference& jointIt : jointMap)
        {
            const JointDesc* desc = jointIt.second;
            if (desc->jointEnabled)
            {
                if (desc->body0 != SdfPath())
                {
                    RigidBodyMap::const_iterator fit = rigidBodyMap.find(desc->body0);
                    if (fit != rigidBodyMap.end() && fit->second->rigidBodyEnabled && !fit->second->kinematicBody)
                    {
                        bodyJointMap[desc->body0].push_back(desc);
                    }                    
                }
                if (desc->body1 != SdfPath())
                {
                    RigidBodyMap::const_iterator fit = rigidBodyMap.find(desc->body1);
                    if (fit != rigidBodyMap.end() && fit->second->rigidBodyEnabled && !fit->second->kinematicBody)
                    {
                        bodyJointMap[desc->body1].push_back(desc);
                    }
                }
            }
        }
    }

    SdfPathVector articulationLinkOrderVector;

    // first get user defined articulation roots
    // then search for the best root in the articulation hierarchy
    for (ArticulationMap::const_reference& it : articulationMap)
    {
        const SdfPath& articulationPath = it.first;
        SdfPath articulationBaseLinkPath = articulationPath;

        std::set<SdfPath> articulatedJoints;
        std::set<SdfPath> articulatedBodies;

        // check if its a floating articulation
        {
            RigidBodyMap::const_iterator bodyIt = rigidBodyMap.find(articulationPath);
            if (bodyIt != rigidBodyMap.end())
            {
                if (!bodyIt->second->rigidBodyEnabled)
                {
                    TF_DIAGNOSTIC_WARNING(
                        "ArticulationRootAPI definition on a static rigid body is not allowed, articulation root will be ignored. Prim: %s",
                        articulationPath.GetText());
                    continue;
                }
                if (bodyIt->second->kinematicBody)
                {
                    TF_DIAGNOSTIC_WARNING(
                        "ArticulationRootAPI definition on a kinematic rigid body is not allowed, articulation root will be ignored. Prim: %s",
                        articulationPath.GetText());
                    continue;
                }
                it.second->rootPrims.push_back(bodyIt->first);
            }
            else
            {
                JointMap::const_iterator jointIt = jointMap.find(articulationPath);
                if (jointIt != jointMap.end())
                {
                    const SdfPath& jointPath = jointIt->first;
                    const JointDesc* jointDesc = jointIt->second;
                    if (jointDesc->body0 == SdfPath() || jointDesc->body1 == SdfPath())
                    {                        
                        it.second->rootPrims.push_back(jointPath);
                        articulationBaseLinkPath = jointDesc->body0 == SdfPath() ? jointDesc->body1 : jointDesc->body0;
                    }
                }
            }
        }

        // search through the hierarchy for the best root        
        const UsdPrim articulationPrim = stage->GetPrimAtPath(articulationBaseLinkPath);        
        if (!articulationPrim)
            continue;        
        UsdPrimRange range(articulationPrim, UsdTraverseInstanceProxies());
        std::vector<ArticulationLinkMap> articulationLinkMaps;
        articulationLinkOrderVector.clear();

        for (pxr::UsdPrimRange::const_iterator iter = range.begin(); iter != range.end(); ++iter)
        {
            const pxr::UsdPrim& prim = *iter;
            if (!prim)
                continue;
            const SdfPath primPath = prim.GetPrimPath();            
            if (IsInLinkMap(primPath, articulationLinkMaps))
            {
                iter.PruneChildren(); // Skip the subtree rooted at this prim
                continue;
            }

            RigidBodyMap::const_iterator bodyIt = rigidBodyMap.find(primPath);
            if (bodyIt != rigidBodyMap.end())
            {
                articulationLinkMaps.push_back(ArticulationLinkMap());
                uint32_t index = 0;
                TraverseHierarchy(stage, primPath, articulationLinkMaps.back(), bodyJointMap, index, articulationLinkOrderVector);
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
                        linkPath = (linkIt.second.rootJoint != SdfPath()) ? linkIt.second.rootJoint : linkIt.first;
                        largestWeight = linkIt.second.weight;
                    }
                    else if (linkIt.second.weight == largestWeight)
                    {
                        const SdfPath optionalLinkPath = (linkIt.second.rootJoint != SdfPath()) ? linkIt.second.rootJoint : linkIt.first;
                        for (const SdfPath& orderPath : articulationLinkOrderVector)
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

                // for floating articulation lets find the body with the shortest paths (center of graph)
                if (!hasFixedJoint)
                {
                    linkPath = GetCenterOfGraph(map, articulationLinkOrderVector);
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
                SdfPath linkPath = SdfPath();
                uint32_t largestWeight = 0;
                bool hasFixedOrLoopJoint = false;
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
                articulatedBodies.insert(linkIt.second.childs.begin(), linkIt.second.childs.end());
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
    }
    
}



bool LoadUsdPhysicsFromRange(const UsdStageWeakPtr stage,
        ParsePrimIteratorBase& primIterator,
        UsdPhysicsReportFn reportFn,
        void* userData,
        const CustomUsdPhysicsTokens* customPhysicsTokens,
        const std::vector<SdfPath>* simulationOwners)
{
    bool retVal = true;

    if (!stage)
    {
        TF_RUNTIME_ERROR("Provided stage not valid.");
        return false;
    }

    if (!reportFn)
    {
        TF_RUNTIME_ERROR("Provided report callback is not valid.");
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

    // parse for scene first, get the descriptors, report all prims
    // the descriptors are not complete yet
    primIterator.reset();

    static const TfToken gRigidBodyAPIToken("PhysicsRigidBodyAPI");
    static const TfToken gCollisionAPIToken("PhysicsCollisionAPI");
    static const TfToken gArticulationRootAPIToken("PhysicsArticulationRootAPI");
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

    while (!primIterator.atEnd())
    {
        const pxr::UsdPrim& prim = *primIterator.getCurrent();
        if (!prim)
        {
            primIterator.pruneChildren();
            primIterator.next();
            continue;
        }

        const SdfPath primPath = prim.GetPrimPath();
        const UsdPrimTypeInfo& typeInfo = prim.GetPrimTypeInfo();
        PhysicsObjectDesc* reportDesc = nullptr;

        uint64_t apiFlags = 0;
        const TfTokenVector& apis = prim.GetPrimTypeInfo().GetAppliedAPISchemas();
        for (const TfToken& token : apis)
        {
            if (token == gArticulationRootAPIToken)
            {
                apiFlags |= SchemaAPIFlag::eArticulationRootAPI;
            }
            if (token == gCollisionAPIToken)
            {
                apiFlags |= SchemaAPIFlag::eCollisionAPI;
            }
            if (token == gRigidBodyAPIToken)
            {
                apiFlags |= SchemaAPIFlag::eRigidBodyAPI;
            }
            if (!apiFlags && token == gMaterialAPIToken)
            {
                apiFlags |= SchemaAPIFlag::eMaterialAPI;
            }
        }

        if (typeInfo.GetSchemaType().IsA<UsdGeomPointInstancer>())
        {
            primIterator.pruneChildren(); // Skip the subtree for point instancers, those have to be traversed per prototype
        }
        else if (customPhysicsTokens && !customPhysicsTokens->instancerTokens.empty())
        {
            for (const TfToken& instToken : customPhysicsTokens->instancerTokens)
            {
                if (instToken == typeInfo.GetTypeName())
                {
                    primIterator.pruneChildren(); // Skip the subtree for custom instancers, those have to be traversed per prototype
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
        else if (apiFlags & SchemaAPIFlag::eMaterialAPI)
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
                    for (size_t i = 0; i < customPhysicsTokens->jointTokens.size(); i++)
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
            if (apiFlags & SchemaAPIFlag::eArticulationRootAPI)
            {
                articulationPrims.push_back(prim);
                articulationPathsSet.insert(prim.GetPrimPath());
            }
        }
        else
        {
            if (apiFlags & SchemaAPIFlag::eCollisionAPI)
            {
                collisionPrims.push_back(prim);
            }
            if (apiFlags & SchemaAPIFlag::eRigidBodyAPI)
            {
                rigidBodyPrims.push_back(prim);
            }
            if (apiFlags & SchemaAPIFlag::eArticulationRootAPI)
            {
                articulationPrims.push_back(prim);
                articulationPathsSet.insert(prim.GetPrimPath());
            }
        }

        primIterator.next();
    }

    // process parsing
    // 
    // Scenes
    std::vector<SceneDesc> sceneDescs;

    // is simulation owners provided, restrict scenes to just the one specified
    if (simulationOwners)
    {
        for (size_t i = scenePrims.size(); i--;)
        {
            const SdfPath& primPath = scenePrims[i].GetPrimPath();
            std::unordered_set<SdfPath, SdfPath::Hash>::const_iterator fit = simulationOwnersSet.find(primPath);
            if (fit == simulationOwnersSet.end())
            {
                scenePrims[i] = scenePrims.back();
                scenePrims.pop_back();
            }            
        }
    }
    ProcessPhysicsPrims<SceneDesc, UsdPhysicsScene>(scenePrims, sceneDescs, ParseSceneDesc);

    // Collision Groups
    std::vector<CollisionGroupDesc> collisionGroupsDescs;
    ProcessPhysicsPrims<CollisionGroupDesc, UsdPhysicsCollisionGroup>(collisionGroupPrims, collisionGroupsDescs, ParseCollisionGroupDesc);
    // Run groups merging
    std::unordered_map<std::string, size_t> mergeGroupNameToIndex;
    for (size_t i = 0; i < collisionGroupsDescs.size(); i++)
    {
        const CollisionGroupDesc& desc = collisionGroupsDescs[i];

        if (!desc.mergeGroupName.empty())
        {
            std::unordered_map<std::string, size_t>::const_iterator fit = mergeGroupNameToIndex.find(desc.mergeGroupName);
            if (fit != mergeGroupNameToIndex.end())
            {                
                CollisionGroupDesc& mergeDesc = collisionGroupsDescs[fit->second];
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

    // Populate the sets to check collisions, this needs to run in parallel!!!
    std::map<SdfPath, std::unordered_set<SdfPath, SdfPath::Hash>> collisionGroupSets; 
    for (size_t i = 0; i < collisionGroupsDescs.size(); i++)
    {
        const UsdPrim groupPrim = collisionGroupPrims[i];
        UsdStageWeakPtr stage = groupPrim.GetStage();
        const CollisionGroupDesc& desc = collisionGroupsDescs[i];
        std::unordered_set<SdfPath, SdfPath::Hash>& hashSet = collisionGroupSets[desc.primPath];

        if (desc.mergedGroups.empty())
        {
            const UsdPhysicsCollisionGroup cg(stage->GetPrimAtPath(desc.primPath));
            if (cg)
            {
                const UsdCollectionAPI collectionAPI = cg.GetCollidersCollectionAPI();
                UsdCollectionMembershipQuery query = collectionAPI.ComputeMembershipQuery();
                const SdfPathSet includedPaths = UsdCollectionAPI::ComputeIncludedPaths(query, stage, UsdTraverseInstanceProxies());
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
                const UsdPhysicsCollisionGroup cg(stage->GetPrimAtPath(groupPath));
                if (cg)
                {
                    const UsdCollectionAPI collectionAPI = cg.GetCollidersCollectionAPI();
                    UsdCollectionMembershipQuery query = collectionAPI.ComputeMembershipQuery();
                    const SdfPathSet includedPaths = UsdCollectionAPI::ComputeIncludedPaths(query, stage, UsdTraverseInstanceProxies());
                    for (const SdfPath& path : includedPaths)
                    {
                        hashSet.insert(path);
                    }
                }
            }
        }
    }

    // Rigid body physics material
    std::vector<RigidBodyMaterialDesc> materialDescs;
    ProcessPhysicsPrims<RigidBodyMaterialDesc, UsdPhysicsMaterialAPI>(materialPrims, materialDescs, ParseRigidBodyMaterialDesc);

    // Joints
    std::vector<D6JointDesc> jointDescs;
    ProcessPhysicsPrims<D6JointDesc, UsdPhysicsJoint>(physicsD6JointPrims, jointDescs, ParseD6JointDesc);

    std::vector<RevoluteJointDesc> revoluteJointDescs;
    ProcessPhysicsPrims<RevoluteJointDesc, UsdPhysicsRevoluteJoint>(physicsRevoluteJointPrims, revoluteJointDescs, ParseRevoluteJointDesc);

    std::vector<PrismaticJointDesc> prismaticJointDescs;
    ProcessPhysicsPrims<PrismaticJointDesc, UsdPhysicsPrismaticJoint>(physicsPrismaticJointPrims, prismaticJointDescs, ParsePrismaticJointDesc);

    std::vector<SphericalJointDesc> sphericalJointDescs;
    ProcessPhysicsPrims<SphericalJointDesc, UsdPhysicsSphericalJoint>(physicsSphericalJointPrims, sphericalJointDescs, ParseSphericalJointDesc);

    std::vector<FixedJointDesc> fixedJointDescs;
    ProcessPhysicsPrims<FixedJointDesc, UsdPhysicsFixedJoint>(physicsFixedJointPrims, fixedJointDescs, ParseFixedJointDesc);

    std::vector<DistanceJointDesc> distanceJointDescs;
    ProcessPhysicsPrims<DistanceJointDesc, UsdPhysicsDistanceJoint>(physicsDistanceJointPrims, distanceJointDescs, ParseDistanceJointDesc);

    std::vector<CustomJointDesc> customJointDescs;
    ProcessPhysicsPrims<CustomJointDesc, UsdPhysicsJoint>(physicsCustomJointPrims, customJointDescs, ParseCustomJointDesc);

    // A.B. contruct joint map revisit    
    JointMap jointMap;
    for (D6JointDesc& desc : jointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (RevoluteJointDesc& desc : revoluteJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (PrismaticJointDesc& desc : prismaticJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (SphericalJointDesc& desc : sphericalJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (FixedJointDesc& desc : fixedJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (DistanceJointDesc& desc : distanceJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }
    for (CustomJointDesc& desc : customJointDescs)
    {
        jointMap[desc.primPath] = &desc;
    }


    // collisions
    // first get the type
    std::vector<PhysicsObjectType::Enum> collisionTypes;
    collisionTypes.resize(collisionPrims.size());
    std::vector<TfToken> customTokens;
    for (size_t i = 0; i < collisionPrims.size(); i++)
    {
        if (customPhysicsTokens)
        {
            TfToken shapeToken;
            const PhysicsObjectType::Enum shapeType = GetCollisionType(collisionPrims[i], &customPhysicsTokens->shapeTokens, &shapeToken);
            collisionTypes[i] = shapeType;
            if (shapeType == PhysicsObjectType::eCustomShape)
            {
                customTokens.push_back(shapeToken);
            }
        }
        else
        {
            collisionTypes[i] = GetCollisionType(collisionPrims[i], nullptr, nullptr);
        }
    }

    std::vector<UsdPrim> sphereShapePrims;
    std::vector<UsdPrim> cubeShapePrims;
    std::vector<UsdPrim> cylinderShapePrims;
    std::vector<UsdPrim> capsuleShapePrims;
    std::vector<UsdPrim> coneShapePrims;
    std::vector<UsdPrim> planeShapePrims;
    std::vector<UsdPrim> meshShapePrims;
    std::vector<UsdPrim> spherePointsShapePrims;
    std::vector<UsdPrim> customShapePrims;
    for (size_t i = 0; i < collisionTypes.size(); i++)
    {
        PhysicsObjectType::Enum type = collisionTypes[i];
        switch (type)
        {
            case PhysicsObjectType::eSphereShape:
            {
                sphereShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eCubeShape:
            {
                cubeShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eCapsuleShape:
            {
                capsuleShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eCylinderShape:
            {
                cylinderShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eConeShape:
            {
                coneShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eMeshShape:
            {
                meshShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::ePlaneShape:
            {
                planeShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eCustomShape:
            {
                customShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eSpherePointsShape:
            {
                spherePointsShapePrims.push_back(collisionPrims[i]);
            }
            break;
            case PhysicsObjectType::eUndefined:
            default:            
            {
                TF_DIAGNOSTIC_WARNING("CollisionAPI applied to an unknown UsdGeomGPrim type, prim %s.",
                    collisionPrims[i].GetPrimPath().GetString().c_str());
            }
            break;
        }
    }
    std::vector<SphereShapeDesc> sphereShapeDescs;
    ProcessPhysicsPrims<SphereShapeDesc, UsdPhysicsCollisionAPI>(sphereShapePrims, sphereShapeDescs, ParseSphereShapeDesc);

    std::vector<CubeShapeDesc> cubeShapeDescs;
    ProcessPhysicsPrims<CubeShapeDesc, UsdPhysicsCollisionAPI>(cubeShapePrims, cubeShapeDescs, ParseCubeShapeDesc);

    std::vector<CylinderShapeDesc> cylinderShapeDescs;
    ProcessPhysicsPrims<CylinderShapeDesc, UsdPhysicsCollisionAPI>(cylinderShapePrims, cylinderShapeDescs, ParseCylinderShapeDesc);

    std::vector<CapsuleShapeDesc> capsuleShapeDescs;
    ProcessPhysicsPrims<CapsuleShapeDesc, UsdPhysicsCollisionAPI>(capsuleShapePrims, capsuleShapeDescs, ParseCapsuleShapeDesc);

    std::vector<ConeShapeDesc> coneShapeDescs;
    ProcessPhysicsPrims<ConeShapeDesc, UsdPhysicsCollisionAPI>(coneShapePrims, coneShapeDescs, ParseConeShapeDesc);

    std::vector<PlaneShapeDesc> planeShapeDescs;
    ProcessPhysicsPrims<PlaneShapeDesc, UsdPhysicsCollisionAPI>(planeShapePrims, planeShapeDescs, ParsePlaneShapeDesc);

    std::vector<MeshShapeDesc> meshShapeDescs;
    ProcessPhysicsPrims<MeshShapeDesc, UsdPhysicsCollisionAPI>(meshShapePrims, meshShapeDescs, ParseMeshShapeDesc);

    std::vector<SpherePointsShapeDesc> spherePointsShapeDescs;
    ProcessPhysicsPrims<SpherePointsShapeDesc, UsdPhysicsCollisionAPI>(spherePointsShapePrims, spherePointsShapeDescs, ParseSpherePointsShapeDesc);

    std::vector<CustomShapeDesc> customShapeDescs;
    ProcessPhysicsPrims<CustomShapeDesc, UsdPhysicsCollisionAPI>(customShapePrims, customShapeDescs, ParseCustomShapeDesc);
    if (customShapeDescs.size() == customTokens.size())
    {
        for (size_t i = 0; i < customShapeDescs.size(); i++)
        {
            customShapeDescs[i].customGeometryToken = customTokens[i];
        }
    }

    // rigid bodies
    std::vector<RigidBodyDesc> rigidBodyDescs;
    ProcessPhysicsPrims<RigidBodyDesc, UsdPhysicsRigidBodyAPI>(rigidBodyPrims, rigidBodyDescs, ParseRigidBodyDesc);
    // Ensure if we have a hierarchical parent that has a dynamic parent,
    // that we also have a reset xform stack, otherwise we should log an error.
    // check for nested articulation roots, these are not supported
    RigidBodyMap bodyMap;
    for (size_t i = rigidBodyPrims.size(); i--;)
    {
        bodyMap[rigidBodyPrims[i].GetPrimPath()] = &rigidBodyDescs[i];
    }

    for (size_t i = rigidBodyPrims.size(); i--;)
    {
        const UsdPrim bodyPrim = rigidBodyPrims[i];
        UsdPrim bodyParent = UsdPrim();
        if (HasDynamicBodyParent(bodyPrim.GetParent(), bodyMap, bodyParent))
        {
            bool hasResetXformStack = false;
            UsdPrim parent = bodyPrim;
            while (parent != stage->GetPseudoRoot() && parent != bodyParent)
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

                TF_DIAGNOSTIC_WARNING("Rigid Body of (%s) missing xformstack reset when child of rigid body (%s) in hierarchy. "
                    "Simulation of multiple RigidBodyAPI's in a hierarchy will cause unpredicted results. "
                    "Please fix the hierarchy or use XformStack reset.",
                    bodyPrim.GetPrimPath().GetText(),
                    bodyParent.GetPrimPath().GetText());

                rigidBodyPrims[i] = rigidBodyPrims.back();
                rigidBodyPrims.pop_back();
            }
        }
    }

    // articulations
    // check for nested articulation roots, these are not supported    
    for (size_t i = articulationPrims.size(); i--;)
    {
        if (CheckNestedArticulationRoot(articulationPrims[i], articulationPathsSet))
        {
            TF_DIAGNOSTIC_WARNING("Nested ArticulationRootAPI not supported, API ignored, prim %s.",
                    articulationPrims[i].GetPrimPath().GetString().c_str());
            articulationPrims[i] = articulationPrims.back();
            articulationPrims.pop_back();
        }
    }
    std::vector<ArticulationDesc> articulationDescs;
    ProcessPhysicsPrims<ArticulationDesc, UsdPhysicsArticulationRootAPI>(articulationPrims, articulationDescs, ParseArticulationDesc);

    ArticulationMap articulationMap; // A.B. TODO probably not needed
    for (size_t i = articulationPrims.size(); i--;)
    {
        articulationMap[articulationPrims[i].GetPrimPath()] = &articulationDescs[i];
    }

    // Finalize collisions
    {
        UsdGeomXformCache xfCache;

        FinalizeCollisionDescs<SphereShapeDesc>(xfCache, sphereShapePrims, sphereShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<CubeShapeDesc>(xfCache, cubeShapePrims, cubeShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<CapsuleShapeDesc>(xfCache, capsuleShapePrims, capsuleShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<CylinderShapeDesc>(xfCache, cylinderShapePrims, cylinderShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<ConeShapeDesc>(xfCache, coneShapePrims, coneShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<PlaneShapeDesc>(xfCache, planeShapePrims, planeShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<MeshShapeDesc>(xfCache, meshShapePrims, meshShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<SpherePointsShapeDesc>(xfCache, spherePointsShapePrims, spherePointsShapeDescs, bodyMap, collisionGroupSets);
        FinalizeCollisionDescs<CustomShapeDesc>(xfCache, customShapePrims, customShapeDescs, bodyMap, collisionGroupSets);
    }

    // Finalize articulations
    {   
        // A.B. walk through the finalize code refactor
        FinalizeArticulations(stage, articulationMap, bodyMap, jointMap);
    }

    // if simulationOwners are in play lets shrink down the reported descriptors    
    if (simulationOwners && !simulationOwners->empty())
    {
        std::unordered_set<SdfPath, SdfPath::Hash> reportedBodies;
        // first check bodies
        CheckRigidBodySimulationOwner(rigidBodyPrims, rigidBodyDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);

        // check collisions
        // if collision belongs to a body that we care about include it
        // if collision does not belong to a body we care about its not included
        // if collision does not have a body set, we check its own simulationOwners
        CheckCollisionSimulationOwner(sphereShapePrims, sphereShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(cubeShapePrims, cubeShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(capsuleShapePrims, capsuleShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(cylinderShapePrims, cylinderShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(coneShapePrims, coneShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(planeShapePrims, planeShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(meshShapePrims, meshShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(spherePointsShapePrims, spherePointsShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckCollisionSimulationOwner(customShapePrims, customShapeDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);

        // Both bodies need to have simulation owners valid
        CheckJointSimulationOwner(physicsFixedJointPrims, fixedJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsRevoluteJointPrims, revoluteJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsPrismaticJointPrims, prismaticJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsSphericalJointPrims, sphericalJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsDistanceJointPrims, distanceJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsD6JointPrims, jointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
        CheckJointSimulationOwner(physicsCustomJointPrims, customJointDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);

        // All bodies need to have simulation owners valid
        CheckArticulationSimulationOwner(articulationPrims, articulationDescs, defaultSimulationOwner, reportedBodies, simulationOwnersSet);
    }

    SdfPathVector primPathsVector;
    // get the descriptors, finalize them and send them out in an order
    // 1. send out the scenes
    {
        CallReportFn(PhysicsObjectType::eScene, scenePrims, sceneDescs, reportFn, primPathsVector, userData);
    }

    // 2. send out the CollisionGroups
    {
        CallReportFn(PhysicsObjectType::eCollisionGroup, collisionGroupPrims, collisionGroupsDescs, reportFn, primPathsVector, userData);
    }

    // 3. send out the materials
    {
        CallReportFn(PhysicsObjectType::eRigidBodyMaterial, materialPrims, materialDescs, reportFn, primPathsVector, userData);
    }

    // 4. finish out and send out shapes
    {   
        CallReportFn(PhysicsObjectType::eSphereShape, sphereShapePrims, sphereShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eCubeShape, cubeShapePrims, cubeShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eCapsuleShape, capsuleShapePrims, capsuleShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eCylinderShape, cylinderShapePrims, cylinderShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eConeShape, coneShapePrims, coneShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::ePlaneShape, planeShapePrims, planeShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eMeshShape, meshShapePrims, meshShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eSpherePointsShape, spherePointsShapePrims, spherePointsShapeDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eCustomShape, customShapePrims, customShapeDescs, reportFn, primPathsVector, userData);        
    }

    // 5. send out articulations
    {
        CallReportFn(PhysicsObjectType::eArticulation, articulationPrims, articulationDescs, reportFn, primPathsVector, userData);        
    }

    // 6. send out bodies
    {
        CallReportFn(PhysicsObjectType::eRigidBody, rigidBodyPrims, rigidBodyDescs, reportFn, primPathsVector, userData);                        
    }

    // 7. send out joints    
    {
        CallReportFn(PhysicsObjectType::eFixedJoint, physicsFixedJointPrims, fixedJointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eRevoluteJoint, physicsRevoluteJointPrims, revoluteJointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::ePrismaticJoint, physicsPrismaticJointPrims, prismaticJointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eSphericalJoint, physicsSphericalJointPrims, sphericalJointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eDistanceJoint, physicsDistanceJointPrims, distanceJointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eD6Joint, physicsD6JointPrims, jointDescs, reportFn, primPathsVector, userData);        
        CallReportFn(PhysicsObjectType::eCustomJoint, physicsCustomJointPrims, customJointDescs, reportFn, primPathsVector, userData);        
    }

    return retVal;
}


PXR_NAMESPACE_CLOSE_SCOPE
