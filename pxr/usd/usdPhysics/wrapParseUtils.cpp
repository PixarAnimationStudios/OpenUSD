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

#include "pxr/usd/usd/pyConversions.h"

#include "pxr/external/boost/python/suite/indexing/vector_indexing_suite.hpp"

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/str.hpp"


PXR_NAMESPACE_USING_DIRECTIVE

template <typename T>
void registerVectorConverter(const char* name)
{
    pxr_boost::python::class_<std::vector<T>>(name).def(pxr_boost::python::vector_indexing_suite<std::vector<T>>());
}

using namespace pxr_boost::python;


// Wrapper class
class ParsePrimIteratorBaseWrap : public ParsePrimIteratorBase, public pxr_boost::python::wrapper<ParsePrimIteratorBase> {
public:
    void reset() {
        this->get_override("reset")();
    }

    bool atEnd() const {
        return this->get_override("atEnd")();
    }

    UsdPrimRange::const_iterator getCurrent() {
        return this->get_override("getCurrent")();
    }

    void next() {
        this->get_override("next")();
    }

    void pruneChildren() {
        this->get_override("pruneChildren")();
    }
};

class MarshalCallback
{
public:    

    SdfPathVector                   scenePrimPaths;
    std::vector<UsdPhysicsSceneDesc>          sceneDescs;

    SdfPathVector                   rigidBodyPrimPaths;
    std::vector<UsdPhysicsRigidBodyDesc>      rigidBodyDescs;

    SdfPathVector                   sphereShapePrimPaths;
    std::vector<UsdPhysicsSphereShapeDesc>    sphereShapeDescs;

    SdfPathVector                   cubeShapePrimPaths;
    std::vector<UsdPhysicsCubeShapeDesc>    cubeShapeDescs;

    SdfPathVector                   capsuleShapePrimPaths;
    std::vector<UsdPhysicsCapsuleShapeDesc>    capsuleShapeDescs;

    SdfPathVector                   cylinderShapePrimPaths;
    std::vector<UsdPhysicsCylinderShapeDesc>    cylinderShapeDescs;

    SdfPathVector            coneShapePrimPaths;
    std::vector<UsdPhysicsConeShapeDesc>    coneShapeDescs;

    SdfPathVector            meshShapePrimPaths;
    std::vector<UsdPhysicsMeshShapeDesc>    meshShapeDescs;

    SdfPathVector            planeShapePrimPaths;
    std::vector<UsdPhysicsPlaneShapeDesc>    planeShapeDescs;

    SdfPathVector            customShapePrimPaths;
    std::vector<UsdPhysicsCustomShapeDesc>    customShapeDescs;

    SdfPathVector            spherePointShapePrimPaths;
    std::vector<UsdPhysicsSpherePointsShapeDesc>    spherePointsShapeDescs;

    SdfPathVector            fixedJointPrimPaths;
    std::vector<UsdPhysicsFixedJointDesc>    fixedJointDescs;

    SdfPathVector            revoluteJointPrimPaths;
    std::vector<UsdPhysicsRevoluteJointDesc>    revoluteJointDescs;

    SdfPathVector            prismaticJointPrimPaths;
    std::vector<UsdPhysicsPrismaticJointDesc>    prismaticJointDescs;

    SdfPathVector            sphericalJointPrimPaths;
    std::vector<UsdPhysicsSphericalJointDesc>    sphericalJointDescs;

    SdfPathVector            distanceJointPrimPaths;
    std::vector<UsdPhysicsDistanceJointDesc>    distanceJointDescs;

    SdfPathVector            d6JointPrimPaths;
    std::vector<UsdPhysicsD6JointDesc>    d6JointDescs;

    SdfPathVector            customJointPrimPaths;
    std::vector<UsdPhysicsCustomJointDesc>    customJointDescs;

    SdfPathVector            rigidBodyMaterialPrimPaths;
    std::vector<UsdPhysicsRigidBodyMaterialDesc>    rigidBodyMaterialDescs;

    SdfPathVector            articulationPrimPaths;
    std::vector<UsdPhysicsArticulationDesc>    articulationDescs;

    SdfPathVector            collisionGroupPrimPaths;
    std::vector<UsdPhysicsCollisionGroupDesc>    collisionGroupDescs;

    void clear()
    {
        scenePrimPaths.clear();
        sceneDescs.clear();

        rigidBodyPrimPaths.clear();
        rigidBodyDescs.clear();

        sphereShapePrimPaths.clear();
        sphereShapeDescs.clear();

        cubeShapePrimPaths.clear();
        cubeShapeDescs.clear();

        capsuleShapePrimPaths.clear();
        capsuleShapeDescs.clear();

        cylinderShapePrimPaths.clear();
        cylinderShapeDescs.clear();

        coneShapePrimPaths.clear();
        coneShapeDescs.clear();

        meshShapePrimPaths.clear();
        meshShapeDescs.clear();

        planeShapePrimPaths.clear();
        planeShapeDescs.clear();

        customShapePrimPaths.clear();
        customShapeDescs.clear();

        spherePointShapePrimPaths.clear();
        spherePointsShapeDescs.clear();

        fixedJointPrimPaths.clear();
        fixedJointDescs.clear();

        revoluteJointPrimPaths.clear();
        revoluteJointDescs.clear();

        prismaticJointPrimPaths.clear();
        prismaticJointDescs.clear();

        sphericalJointPrimPaths.clear();
        sphericalJointDescs.clear();

        distanceJointPrimPaths.clear();
        distanceJointDescs.clear();

        d6JointPrimPaths.clear();
        d6JointDescs.clear();

        customJointPrimPaths.clear();
        customJointDescs.clear();

        rigidBodyMaterialPrimPaths.clear();
        rigidBodyMaterialDescs.clear();

        articulationPrimPaths.clear();
        articulationDescs.clear();

        collisionGroupPrimPaths.clear();
        collisionGroupDescs.clear();
    }

} gMarshalCallback;

template <typename DescType>
void copyDescs(size_t numDesc, const SdfPath* primsSource, SdfPathVector& primsDest, const UsdPhysicsObjectDesc* objectDescsSource, std::vector<DescType>& objectDescsDest)
{
    primsDest.resize(numDesc);
    objectDescsDest.resize(numDesc);

    if (numDesc)
    {
        const DescType* sourceDesc = reinterpret_cast<const DescType*>(objectDescsSource);

        for (size_t i = 0; i < numDesc; i++)
        {
            primsDest[i] = primsSource[i];
            objectDescsDest[i] = sourceDesc[i];
        }
    }
}

void ReportPhysicsObjectsFn(UsdPhysicsObjectType::Enum type, size_t numDesc, const SdfPath* primPaths,
    const UsdPhysicsObjectDesc* objectDescs, void* userData)
{
    MarshalCallback* cb = (MarshalCallback*)userData;
        
    switch (type)
    {
        case UsdPhysicsObjectType::eScene:
        {       
            copyDescs(numDesc, primPaths, cb->scenePrimPaths, objectDescs, cb->sceneDescs);
        }        
        break;
        case UsdPhysicsObjectType::eRigidBody:
        {       
            copyDescs(numDesc, primPaths, cb->rigidBodyPrimPaths, objectDescs, cb->rigidBodyDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eSphereShape:
        {       
            copyDescs(numDesc, primPaths, cb->sphereShapePrimPaths, objectDescs, cb->sphereShapeDescs);
        }  
        break;      
        case UsdPhysicsObjectType::eCubeShape:
        {       
            copyDescs(numDesc, primPaths, cb->cubeShapePrimPaths, objectDescs, cb->cubeShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eCapsuleShape:
        {       
            copyDescs(numDesc, primPaths, cb->capsuleShapePrimPaths, objectDescs, cb->capsuleShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eCylinderShape:
        {       
            copyDescs(numDesc, primPaths, cb->cylinderShapePrimPaths, objectDescs, cb->cylinderShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eConeShape:
        {       
            copyDescs(numDesc, primPaths, cb->coneShapePrimPaths, objectDescs, cb->coneShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eMeshShape:
        {       
            copyDescs(numDesc, primPaths, cb->meshShapePrimPaths, objectDescs, cb->meshShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::ePlaneShape:
        {       
            copyDescs(numDesc, primPaths, cb->planeShapePrimPaths, objectDescs, cb->planeShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eCustomShape:
        {       
            copyDescs(numDesc, primPaths, cb->customShapePrimPaths, objectDescs, cb->customShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eSpherePointsShape:
        {       
            copyDescs(numDesc, primPaths, cb->spherePointShapePrimPaths, objectDescs, cb->spherePointsShapeDescs);
        }        
        break;        
        case UsdPhysicsObjectType::eFixedJoint:
        {       
            copyDescs(numDesc, primPaths, cb->fixedJointPrimPaths, objectDescs, cb->fixedJointDescs);
        }        
        break;
        case UsdPhysicsObjectType::eRevoluteJoint:
        {       
            copyDescs(numDesc, primPaths, cb->revoluteJointPrimPaths, objectDescs, cb->revoluteJointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::ePrismaticJoint:
        {       
            copyDescs(numDesc, primPaths, cb->prismaticJointPrimPaths, objectDescs, cb->prismaticJointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eSphericalJoint:
        {       
            copyDescs(numDesc, primPaths, cb->sphericalJointPrimPaths, objectDescs, cb->sphericalJointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eDistanceJoint:
        {       
            copyDescs(numDesc, primPaths, cb->distanceJointPrimPaths, objectDescs, cb->distanceJointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eD6Joint:
        {       
            copyDescs(numDesc, primPaths, cb->d6JointPrimPaths, objectDescs, cb->d6JointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eCustomJoint:
        {       
            copyDescs(numDesc, primPaths, cb->customJointPrimPaths, objectDescs, cb->customJointDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eRigidBodyMaterial:
        {       
            copyDescs(numDesc, primPaths, cb->rigidBodyMaterialPrimPaths, objectDescs, cb->rigidBodyMaterialDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eArticulation:
        {       
            copyDescs(numDesc, primPaths, cb->articulationPrimPaths, objectDescs, cb->articulationDescs);
        }     
        break;   
        case UsdPhysicsObjectType::eCollisionGroup:
        {       
            copyDescs(numDesc, primPaths, cb->collisionGroupPrimPaths, objectDescs, cb->collisionGroupDescs);
        }     
        break;   
    }
}

struct _CustomUsdPhysicsTokens
{
    _CustomUsdPhysicsTokens()
    {

    }

    pxr_boost::python::list jointTokens;       ///< Custom joints to be reported by parsing
    pxr_boost::python::list shapeTokens;       ///< Custom shapes to be reported by parsing
    pxr_boost::python::list instancerTokens;   ///< Custom physics instancers to be skipped by parsing
};

std::string getString(const pxr_boost::python::object& po)
{
    if (pxr_boost::python::extract<std::string>(po).check()) {
        std::string str = pxr_boost::python::extract<std::string>(po);
        return str;
    }
    else 
    {
        // Handle non-string items, e.g. convert to string
        std::string str = pxr_boost::python::extract<std::string>(pxr_boost::python::str(po));
        return str;
    }
}

pxr_boost::python::dict _LoadUsdPhysicsFromRange(UsdStageWeakPtr stage, ParsePrimIteratorBase& range, const _CustomUsdPhysicsTokens& customTokens, const std::vector<SdfPath>& simulationOwners)
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
            parsingCustomTokens.jointTokens.push_back(TfToken(getString(customTokens.jointTokens[i])));
        }
        for (size_t i = 0; i < shapeTokesSize; i++)
        {
            parsingCustomTokens.shapeTokens.push_back(TfToken(getString(customTokens.shapeTokens[i])));
        }
        for (size_t i = 0; i < instancerTokesSize; i++)
        {
            parsingCustomTokens.instancerTokens.push_back(TfToken(getString(customTokens.instancerTokens[i])));
        }
        customTokensValid = true;
    }

    gMarshalCallback.clear();
    const bool ret_val = LoadUsdPhysicsFromRange(stage, range, ReportPhysicsObjectsFn, &gMarshalCallback, 
        customTokensValid ? &parsingCustomTokens : nullptr,
        !simulationOwners.empty() ? &simulationOwners : nullptr);
    pxr_boost::python::dict retDict;
    if (ret_val)
    {
        if (!gMarshalCallback.sceneDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eScene] = pxr_boost::python::make_tuple(gMarshalCallback.scenePrimPaths, gMarshalCallback.sceneDescs);
        }
        if (!gMarshalCallback.rigidBodyDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eRigidBody] = pxr_boost::python::make_tuple(gMarshalCallback.rigidBodyPrimPaths, gMarshalCallback.rigidBodyDescs);
        }
        if (!gMarshalCallback.sphereShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eSphereShape] = pxr_boost::python::make_tuple(gMarshalCallback.sphereShapePrimPaths, gMarshalCallback.sphereShapeDescs);
        }
        if (!gMarshalCallback.cubeShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCubeShape] = pxr_boost::python::make_tuple(gMarshalCallback.cubeShapePrimPaths, gMarshalCallback.cubeShapeDescs);
        }
        if (!gMarshalCallback.capsuleShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCapsuleShape] = pxr_boost::python::make_tuple(gMarshalCallback.capsuleShapePrimPaths, gMarshalCallback.capsuleShapeDescs);
        }
        if (!gMarshalCallback.cylinderShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCylinderShape] = pxr_boost::python::make_tuple(gMarshalCallback.cylinderShapePrimPaths, gMarshalCallback.cylinderShapeDescs);
        }
        if (!gMarshalCallback.coneShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eConeShape] = pxr_boost::python::make_tuple(gMarshalCallback.coneShapePrimPaths, gMarshalCallback.coneShapeDescs);
        }
        if (!gMarshalCallback.meshShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eMeshShape] = pxr_boost::python::make_tuple(gMarshalCallback.meshShapePrimPaths, gMarshalCallback.meshShapeDescs);
        }
        if (!gMarshalCallback.planeShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::ePlaneShape] = pxr_boost::python::make_tuple(gMarshalCallback.planeShapePrimPaths, gMarshalCallback.planeShapeDescs);
        }
        if (!gMarshalCallback.customShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCustomShape] = pxr_boost::python::make_tuple(gMarshalCallback.customShapePrimPaths, gMarshalCallback.customShapeDescs);
        }
        if (!gMarshalCallback.spherePointsShapeDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eSpherePointsShape] = pxr_boost::python::make_tuple(gMarshalCallback.spherePointShapePrimPaths, gMarshalCallback.spherePointsShapeDescs);
        }
        if (!gMarshalCallback.fixedJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eFixedJoint] = pxr_boost::python::make_tuple(gMarshalCallback.fixedJointPrimPaths, gMarshalCallback.fixedJointDescs);
        }
        if (!gMarshalCallback.revoluteJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eRevoluteJoint] = pxr_boost::python::make_tuple(gMarshalCallback.revoluteJointPrimPaths, gMarshalCallback.revoluteJointDescs);
        }
        if (!gMarshalCallback.prismaticJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::ePrismaticJoint] = pxr_boost::python::make_tuple(gMarshalCallback.prismaticJointPrimPaths, gMarshalCallback.prismaticJointDescs);
        }
        if (!gMarshalCallback.sphericalJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eSphericalJoint] = pxr_boost::python::make_tuple(gMarshalCallback.sphericalJointPrimPaths, gMarshalCallback.sphericalJointDescs);
        }
        if (!gMarshalCallback.distanceJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eDistanceJoint] = pxr_boost::python::make_tuple(gMarshalCallback.distanceJointPrimPaths, gMarshalCallback.distanceJointDescs);
        }
        if (!gMarshalCallback.d6JointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eD6Joint] = pxr_boost::python::make_tuple(gMarshalCallback.d6JointPrimPaths, gMarshalCallback.d6JointDescs);
        }
        if (!gMarshalCallback.customJointDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCustomJoint] = pxr_boost::python::make_tuple(gMarshalCallback.customJointPrimPaths, gMarshalCallback.customJointDescs);
        }
        if (!gMarshalCallback.rigidBodyMaterialDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eRigidBodyMaterial] = pxr_boost::python::make_tuple(gMarshalCallback.rigidBodyMaterialPrimPaths, gMarshalCallback.rigidBodyMaterialDescs);
        }
        if (!gMarshalCallback.articulationDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eArticulation] = pxr_boost::python::make_tuple(gMarshalCallback.articulationPrimPaths, gMarshalCallback.articulationDescs);
        }
        if (!gMarshalCallback.collisionGroupDescs.empty())
        {
            retDict[UsdPhysicsObjectType::eCollisionGroup] = pxr_boost::python::make_tuple(gMarshalCallback.collisionGroupPrimPaths, gMarshalCallback.collisionGroupDescs);
        }
    }    
    return retDict;
}

static std::string
_CustomUsdPhysicsTokens_Repr(const CustomUsdPhysicsTokens& self)
{
    return TfStringPrintf("%sCustomUsdPhysicsTokens(jointTokens=%s, shapeTokens=%s, instancerTokens=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.jointTokens).c_str(),
        TfPyRepr(self.shapeTokens).c_str(),
        TfPyRepr(self.instancerTokens).c_str());
}

static std::string
_PhysicsObjectDesc_Repr(const UsdPhysicsObjectDesc& self)
{
    return TfStringPrintf("%sPhysicsObjectDesc(type=%s, primPath=%s, isValid=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.type).c_str(),
        TfPyRepr(self.primPath).c_str(),
        TfPyRepr(self.isValid).c_str());
}

static std::string
_SceneDesc_Repr(const UsdPhysicsSceneDesc& self)
{
    return TfStringPrintf("%sSceneDesc(gravityDirection=%s, gravityMagnitude=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.gravityDirection).c_str(),
        TfPyRepr(self.gravityMagnitude).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_CollisionGroupDesc_Repr(const UsdPhysicsCollisionGroupDesc& self)
{
    return TfStringPrintf("%sCollisionGroupDesc(invertFilteredGroups=%s, mergeGroupName=%s, mergedGroups=%s, filteredGroups=%s), parent %s",
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
    return TfStringPrintf("%sRigidBodyMaterialDesc(staticFriction=%s, dynamicFriction=%s, restitution=%s, density=%s), parent %s",
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
    return TfStringPrintf("%sShapeDesc(rigidBody=%s, localPos=%s, localRot=%s, localScale=%s, materials=%s, simulationOwners=%s, filteredCollisions=%s, collisionGroups=%s, collisionEnabled=%s), parent %s",
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
    return TfStringPrintf("%sSphereShapeDesc(radius=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CapsuleShapeDesc_Repr(const UsdPhysicsCapsuleShapeDesc& self)
{
    return TfStringPrintf("%sCapsuleShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CylinderShapeDesc_Repr(const UsdPhysicsCylinderShapeDesc& self)
{
    return TfStringPrintf("%sCylinderShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_ConeShapeDesc_Repr(const UsdPhysicsConeShapeDesc& self)
{
    return TfStringPrintf("%sConeShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
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
    return TfStringPrintf("%sMeshShapeDesc(approximation=%s, meshScale=%s, doubleSided=%s), parent %s",
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
    return TfStringPrintf("%sRigidBodyDesc(collisions=%s, filteredCollisions=%s, simulationOwners=%s, position=%s, rotation=%s, scale=%s, rigidBodyEnabled=%s, kinematicBody=%s, startsAsleep=%s, linearVelocity=%s, angularVelocity=%s), parent %s",
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
    return TfStringPrintf("%sJointDrive(enabled=%s, targetPosition=%s, targetVelocity=%s, forceLimit=%s, stiffness=%s, damping=%s, acceleration=%s)",
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
    return TfStringPrintf("%sArticulationDesc(rootPrims=%s, filteredCollisions=%s, articulatedJoints=%s, articulatedBodies=%s), parent %s",
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
    return TfStringPrintf("%sJointDesc(rel0=%s, rel1=%s, body0=%s, body1=%s, localPose0Position=%s, localPose0Orientation=%s, localPose1Position=%s, localPose1Orientation=%s, jointEnabled=%s, breakForce=%s, breakTorque=%s, excludeFromArticulation=%s, collisionEnabled=%s), parent %s",
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
_JointLimitDOFPair_Repr(const std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointLimit>& self)
{
    return TfStringPrintf("%sJointLimitDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_JointDriveDOFPair_Repr(const std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointDrive>& self)
{
    return TfStringPrintf("%sJointDriveDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_D6JointDesc_Repr(const UsdPhysicsD6JointDesc& self)
{
    return TfStringPrintf("%sD6JointDesc(jointLimits=%s, jointDrives=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.jointLimits).c_str(),
        TfPyRepr(self.jointDrives).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_PrismaticJointDesc_Repr(const UsdPhysicsPrismaticJointDesc& self)
{
    return TfStringPrintf("%sPrismaticJointDesc(axis=%s, limit=%s, drive=%s), parent %s",
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
    return TfStringPrintf("%sRevoluteJointDesc(axis=%s, limit=%s, drive=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.drive).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_DistanceJointDesc_Repr(const UsdPhysicsDistanceJointDesc& self)
{
    return TfStringPrintf("%sDistanceJointDesc(minEnabled=%s, limit=%s, maxEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(self.minEnabled).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.maxEnabled).c_str(),
        _JointDesc_Repr(self).c_str());
}

void wrapParseUtils()
{
    pxr_boost::python::enum_<UsdPhysicsObjectType::Enum>("ObjectType")
    .value("Undefined", UsdPhysicsObjectType::eUndefined)
    .value("Scene", UsdPhysicsObjectType::eScene)
    .value("RigidBody", UsdPhysicsObjectType::eRigidBody)
    .value("SphereShape", UsdPhysicsObjectType::eSphereShape)
    .value("CubeShape", UsdPhysicsObjectType::eCubeShape)
    .value("CapsuleShape", UsdPhysicsObjectType::eCapsuleShape)
    .value("CylinderShape", UsdPhysicsObjectType::eCylinderShape)
    .value("ConeShape", UsdPhysicsObjectType::eConeShape)
    .value("MeshShape", UsdPhysicsObjectType::eMeshShape)
    .value("PlaneShape", UsdPhysicsObjectType::ePlaneShape)
    .value("CustomShape", UsdPhysicsObjectType::eCustomShape)
    .value("SpherePointsShape", UsdPhysicsObjectType::eSpherePointsShape)
    .value("FixedJoint", UsdPhysicsObjectType::eFixedJoint)
    .value("RevoluteJoint", UsdPhysicsObjectType::eRevoluteJoint)
    .value("PrismaticJoint", UsdPhysicsObjectType::ePrismaticJoint)
    .value("SphericalJoint", UsdPhysicsObjectType::eSphericalJoint)
    .value("DistanceJoint", UsdPhysicsObjectType::eDistanceJoint)
    .value("D6Joint", UsdPhysicsObjectType::eD6Joint)
    .value("CustomJoint", UsdPhysicsObjectType::eCustomJoint)
    .value("RigidBodyMaterial", UsdPhysicsObjectType::eRigidBodyMaterial)
    .value("Articulation", UsdPhysicsObjectType::eArticulation)
    .value("CollisionGroup", UsdPhysicsObjectType::eCollisionGroup)
    ;

    pxr_boost::python::enum_<UsdPhysicsAxis::Enum>("Axis")
    .value("X", UsdPhysicsAxis::eX)
    .value("Y", UsdPhysicsAxis::eY)
    .value("Z", UsdPhysicsAxis::eZ)
    ;

    pxr_boost::python::enum_<UsdPhysicsJointDOF::Enum>("JointDOF")
    .value("Distance", UsdPhysicsJointDOF::eDistance)
    .value("TransX", UsdPhysicsJointDOF::eTransX)
    .value("TransY", UsdPhysicsJointDOF::eTransY)
    .value("TransZ", UsdPhysicsJointDOF::eTransZ)
    .value("RotX", UsdPhysicsJointDOF::eRotX)
    .value("RotY", UsdPhysicsJointDOF::eRotY)
    .value("RotZ", UsdPhysicsJointDOF::eRotZ)
    ;
    
    pxr_boost::python::class_<_CustomUsdPhysicsTokens>
        cupt("CustomUsdPhysicsTokens");
    cupt
        .def_readwrite("jointTokens", &_CustomUsdPhysicsTokens::jointTokens)
        .def_readwrite("shapeTokens", &_CustomUsdPhysicsTokens::shapeTokens)
        .def_readwrite("instancerTokens", &_CustomUsdPhysicsTokens::instancerTokens)
        .def("__repr__", _CustomUsdPhysicsTokens_Repr);

    pxr_boost::python::class_<UsdPhysicsObjectDesc>
        podcls("ObjectDesc", pxr_boost::python::no_init);
    podcls
        .def_readonly("type", &UsdPhysicsObjectDesc::type)
        .def_readonly("primPath", &UsdPhysicsObjectDesc::primPath)
        .def_readonly("isValid", &UsdPhysicsObjectDesc::isValid)
        .def("__repr__", _PhysicsObjectDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsSceneDesc, bases<UsdPhysicsObjectDesc>>
        sdcls("SceneDesc", pxr_boost::python::no_init);
    sdcls
        .def_readonly("gravityDirection", &UsdPhysicsSceneDesc::gravityDirection)        
        .def_readonly("gravityMagnitude", &UsdPhysicsSceneDesc::gravityMagnitude)
        .def("__repr__", _SceneDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCollisionGroupDesc, bases<UsdPhysicsObjectDesc>>
        cgcls("CollisionGroupDesc", pxr_boost::python::no_init);
    cgcls
        .def_readonly("invertFilteredGroups", &UsdPhysicsCollisionGroupDesc::invertFilteredGroups)
        .add_property("mergedGroups", make_function(&UsdPhysicsCollisionGroupDesc::getMergedGroups,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredGroups", make_function(&UsdPhysicsCollisionGroupDesc::getFilteredGroups,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("mergeGroupName", &UsdPhysicsCollisionGroupDesc::mergeGroupName)
        .def("__repr__", _CollisionGroupDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsRigidBodyMaterialDesc, bases<UsdPhysicsObjectDesc>>
        rbmcls("RigidBodyMaterialDesc", pxr_boost::python::no_init);
    rbmcls
        .def_readonly("staticFriction", &UsdPhysicsRigidBodyMaterialDesc::staticFriction)
        .def_readonly("dynamicFriction", &UsdPhysicsRigidBodyMaterialDesc::dynamicFriction)
        .def_readonly("restitution", &UsdPhysicsRigidBodyMaterialDesc::restitution)
        .def_readonly("density", &UsdPhysicsRigidBodyMaterialDesc::density)
        .def("__repr__", _RigidBodyMaterialDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsShapeDesc, bases<UsdPhysicsObjectDesc>>
        shdcls("ShapeDesc", pxr_boost::python::no_init);
    shdcls
        .def_readonly("rigidBody", &UsdPhysicsShapeDesc::rigidBody)
        .def_readonly("localPos", &UsdPhysicsShapeDesc::localPos)
        .def_readonly("localRot", &UsdPhysicsShapeDesc::localRot)
        .def_readonly("localScale", &UsdPhysicsShapeDesc::localScale)
        .add_property("materials", make_function(&UsdPhysicsShapeDesc::getMaterials,
            return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners", make_function(&UsdPhysicsShapeDesc::getSimulationOwners,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&UsdPhysicsShapeDesc::getFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("collisionGroups", make_function(&UsdPhysicsShapeDesc::getCollisionGroups,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("collisionEnabled", &UsdPhysicsShapeDesc::collisionEnabled)
        .def("__repr__", _ShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsSphereShapeDesc, bases<UsdPhysicsShapeDesc>>
        ssdcls("SphereShapeDesc", pxr_boost::python::no_init);
    ssdcls
        .def_readonly("radius", &UsdPhysicsSphereShapeDesc::radius)
        .def("__repr__", _SphereShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCapsuleShapeDesc, bases<UsdPhysicsShapeDesc>>
        csdcls("CapsuleShapeDesc", pxr_boost::python::no_init);
    csdcls
        .def_readonly("radius", &UsdPhysicsCapsuleShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsCapsuleShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCapsuleShapeDesc::axis)
        .def("__repr__", _CapsuleShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCylinderShapeDesc, bases<UsdPhysicsShapeDesc>>
        cysdcls("CylinderShapeDesc", pxr_boost::python::no_init);
    cysdcls
        .def_readonly("radius", &UsdPhysicsCylinderShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsCylinderShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsCylinderShapeDesc::axis)
        .def("__repr__", _CylinderShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsConeShapeDesc, bases<UsdPhysicsShapeDesc>>
        cosdcls("ConeShapeDesc", pxr_boost::python::no_init);
    cosdcls
        .def_readonly("radius", &UsdPhysicsConeShapeDesc::radius)
        .def_readonly("halfHeight", &UsdPhysicsConeShapeDesc::halfHeight)
        .def_readonly("axis", &UsdPhysicsConeShapeDesc::axis)
        .def("__repr__", _ConeShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsPlaneShapeDesc, bases<UsdPhysicsShapeDesc>>
        pscls("PlaneShapeDesc", pxr_boost::python::no_init);
    pscls
        .def_readonly("axis", &UsdPhysicsPlaneShapeDesc::axis)
        .def("__repr__", _PlaneShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCustomShapeDesc, bases<UsdPhysicsShapeDesc>>
        cuscls("CustomShapeDesc", pxr_boost::python::no_init);
    cuscls
        .def_readonly("customGeometryToken", &UsdPhysicsCustomShapeDesc::customGeometryToken)
        .def("__repr__", _CustomShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCubeShapeDesc, bases<UsdPhysicsShapeDesc>>
        cubescls("CubeShapeDesc", pxr_boost::python::no_init);
    cubescls
        .def_readonly("halfExtents", &UsdPhysicsCubeShapeDesc::halfExtents)
        .def("__repr__", _CubeShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsMeshShapeDesc, bases<UsdPhysicsShapeDesc>>
        mscls("MeshShapeDesc", pxr_boost::python::no_init);
    mscls
        .add_property("approximation", &UsdPhysicsMeshShapeDesc::GetApproximation)
        .def_readonly("meshScale", &UsdPhysicsMeshShapeDesc::meshScale)
        .def_readonly("doubleSided", &UsdPhysicsMeshShapeDesc::doubleSided)
        .def("__repr__", _MeshShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsSpherePoint>
        sppb("SpherePoint", pxr_boost::python::no_init);
    sppb
        .def_readonly("center", &UsdPhysicsSpherePoint::center)
        .def_readonly("radius", &UsdPhysicsSpherePoint::radius)
        .def("__repr__", _SpherePoint_Repr);

    pxr_boost::python::class_<UsdPhysicsSpherePointsShapeDesc, bases<UsdPhysicsShapeDesc>>
        spmscls("SpherePointsShapeDesc", pxr_boost::python::no_init);
    spmscls
        .add_property("spherePoints", &UsdPhysicsSpherePointsShapeDesc::spherePoints)
        .def("__repr__", _SpherePointsShapeDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsRigidBodyDesc, bases<UsdPhysicsObjectDesc>>
        rbcls("RigidBodyDesc", pxr_boost::python::no_init);
    rbcls
        .add_property("collisions", make_function(&UsdPhysicsRigidBodyDesc::getCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&UsdPhysicsRigidBodyDesc::getFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners", make_function(&UsdPhysicsRigidBodyDesc::getSimulationOwners,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("position", &UsdPhysicsRigidBodyDesc::position)
        .def_readonly("rotation", &UsdPhysicsRigidBodyDesc::rotation)
        .def_readonly("scale", &UsdPhysicsRigidBodyDesc::scale)
        .def_readonly("rigidBodyEnabled", &UsdPhysicsRigidBodyDesc::rigidBodyEnabled)
        .def_readonly("kinematicBody", &UsdPhysicsRigidBodyDesc::kinematicBody)
        .def_readonly("startsAsleep", &UsdPhysicsRigidBodyDesc::startsAsleep)
        .def_readonly("linearVelocity", &UsdPhysicsRigidBodyDesc::linearVelocity)
        .def_readonly("angularVelocity", &UsdPhysicsRigidBodyDesc::angularVelocity)
        .def("__repr__", _RigidBodyDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsJointLimit>
        jlcls("JointLimit", pxr_boost::python::no_init);
    jlcls
        .def_readonly("enabled", &UsdPhysicsJointLimit::enabled)
        .def_readonly("lower", &UsdPhysicsJointLimit::lower)
        .def_readonly("upper", &UsdPhysicsJointLimit::upper)
        .def("__repr__", _JointLimit_Repr);

    pxr_boost::python::class_<UsdPhysicsJointDrive>
        jdcls("JointDrive", pxr_boost::python::no_init);
    jdcls
        .def_readonly("enabled", &UsdPhysicsJointDrive::enabled)
        .def_readonly("targetPosition", &UsdPhysicsJointDrive::targetPosition)
        .def_readonly("targetVelocity", &UsdPhysicsJointDrive::targetVelocity)
        .def_readonly("forceLimit", &UsdPhysicsJointDrive::forceLimit)
        .def_readonly("stiffness", &UsdPhysicsJointDrive::stiffness)
        .def_readonly("damping", &UsdPhysicsJointDrive::damping)
        .def_readonly("acceleration", &UsdPhysicsJointDrive::acceleration)
        .def("__repr__", _JointDrive_Repr);

    pxr_boost::python::class_<UsdPhysicsArticulationDesc, bases<UsdPhysicsObjectDesc>>
        adcls("ArticulationDesc", pxr_boost::python::no_init);
    adcls
        .add_property("rootPrims", make_function(&UsdPhysicsArticulationDesc::GetRootPrims,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&UsdPhysicsArticulationDesc::GetFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedJoints", make_function(&UsdPhysicsArticulationDesc::GetArticulatedJoints,
            return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedBodies", make_function(&UsdPhysicsArticulationDesc::GetArticulatedBodies,
            return_value_policy<TfPySequenceToList>()))
        .def("__repr__", _ArticulationDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsJointDesc, bases<UsdPhysicsObjectDesc>>
        jdscls("JointDesc", pxr_boost::python::no_init);
    jdscls
        .def_readonly("rel0", &UsdPhysicsJointDesc::rel0)
        .def_readonly("rel1", &UsdPhysicsJointDesc::rel1)
        .def_readonly("body0", &UsdPhysicsJointDesc::body0)
        .def_readonly("body1", &UsdPhysicsJointDesc::body1)
        .def_readonly("localPose0Position", &UsdPhysicsJointDesc::localPose0Position)
        .def_readonly("localPose0Orientation", &UsdPhysicsJointDesc::localPose0Orientation)
        .def_readonly("localPose1Position", &UsdPhysicsJointDesc::localPose1Position)
        .def_readonly("localPose1Orientation", &UsdPhysicsJointDesc::localPose1Orientation)
        .def_readonly("jointEnabled", &UsdPhysicsJointDesc::jointEnabled)
        .def_readonly("breakForce", &UsdPhysicsJointDesc::breakForce)
        .def_readonly("breakTorque", &UsdPhysicsJointDesc::breakTorque)
        .def_readonly("excludeFromArticulation", &UsdPhysicsJointDesc::excludeFromArticulation)
        .def_readonly("collisionEnabled", &UsdPhysicsJointDesc::collisionEnabled)
        .def("__repr__", _JointDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsCustomJointDesc, bases<UsdPhysicsJointDesc>>
        cjdscls("CustomJointDesc", pxr_boost::python::no_init);

    pxr_boost::python::class_<UsdPhysicsFixedJointDesc, bases<UsdPhysicsJointDesc>>
        fjdscls("FixedJointDesc", pxr_boost::python::no_init);

    class_<std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointLimit> >("JointLimitDOFPair")
        .def_readwrite("first", &std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointLimit>::first)
        .def_readwrite("second", &std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointLimit>::second)
        .def("__repr__", _JointLimitDOFPair_Repr);

    class_<std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointDrive> >("JointDriveDOFPair")
        .def_readwrite("first", &std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointDrive>::first)
        .def_readwrite("second", &std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointDrive>::second)
        .def("__repr__", _JointDriveDOFPair_Repr);

    pxr_boost::python::class_<UsdPhysicsD6JointDesc, bases<UsdPhysicsJointDesc>>
        d6jdscls("D6JointDesc", pxr_boost::python::no_init);
    d6jdscls
        .def_readonly("jointLimits", &UsdPhysicsD6JointDesc::jointLimits)
        .def_readonly("jointDrives", &UsdPhysicsD6JointDesc::jointDrives)
        .def("__repr__", _D6JointDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsPrismaticJointDesc, bases<UsdPhysicsJointDesc>>
        pjdscls("PrismaticJointDesc", pxr_boost::python::no_init);
    pjdscls
        .def_readonly("axis", &UsdPhysicsPrismaticJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsPrismaticJointDesc::limit)
        .def_readonly("drive", &UsdPhysicsPrismaticJointDesc::drive)
        .def("__repr__", _PrismaticJointDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsSphericalJointDesc, bases<UsdPhysicsJointDesc>>
        sjdscls("SphericalJointDesc", pxr_boost::python::no_init);
    sjdscls
        .def_readonly("axis", &UsdPhysicsSphericalJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsSphericalJointDesc::limit)
        .def("__repr__", _SphericalJointDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsRevoluteJointDesc, bases<UsdPhysicsJointDesc>>
        rjdscls("RevoluteJointDesc", pxr_boost::python::no_init);
    rjdscls
        .def_readonly("axis", &UsdPhysicsRevoluteJointDesc::axis)
        .def_readonly("limit", &UsdPhysicsRevoluteJointDesc::limit)
        .def_readonly("drive", &UsdPhysicsRevoluteJointDesc::drive)
        .def("__repr__", _RevoluteJointDesc_Repr);

    pxr_boost::python::class_<UsdPhysicsDistanceJointDesc, bases<UsdPhysicsJointDesc>>
        djdscls("DistanceJointDesc", pxr_boost::python::no_init);
    djdscls
        .def_readonly("minEnabled", &UsdPhysicsDistanceJointDesc::minEnabled)
        .def_readonly("limit", &UsdPhysicsDistanceJointDesc::limit)
        .def_readonly("maxEnabled", &UsdPhysicsDistanceJointDesc::maxEnabled)
        .def("__repr__", _DistanceJointDesc_Repr);

    registerVectorConverter<UsdCollectionMembershipQuery>("PhysicsCollectionMembershipQueryVector");

    registerVectorConverter<std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointLimit>>("PhysicsJointLimitDOFVector");

    registerVectorConverter<std::pair<UsdPhysicsJointDOF::Enum, UsdPhysicsJointDrive>>("PhysicsJointDriveDOFVector");

    registerVectorConverter<UsdPhysicsSpherePoint>("PhysicsSpherePointVector");

    registerVectorConverter<UsdPhysicsSceneDesc>("SceneDescVector");

    registerVectorConverter<UsdPhysicsRigidBodyDesc>("RigidBodyDescVector");

    registerVectorConverter<UsdPhysicsSphereShapeDesc>("SphereShapeDescVector");

    registerVectorConverter<UsdPhysicsCapsuleShapeDesc>("CapsuleShapeDescVector");

    registerVectorConverter<UsdPhysicsCylinderShapeDesc>("CylinderShapeDescVector");

    registerVectorConverter<UsdPhysicsConeShapeDesc>("ConeShapeDescVector");

    registerVectorConverter<UsdPhysicsCubeShapeDesc>("CubeShapeDescVector");

    registerVectorConverter<UsdPhysicsMeshShapeDesc>("MeshShapeDescVector");

    registerVectorConverter<UsdPhysicsPlaneShapeDesc>("PlaneShapeDescVector");

    registerVectorConverter<UsdPhysicsCustomShapeDesc>("CustomShapeDescVector");

    registerVectorConverter<UsdPhysicsSpherePointsShapeDesc>("SpherePointsShapeDescVector");

    registerVectorConverter<UsdPhysicsJointDesc>("JointDescVector");

    registerVectorConverter<UsdPhysicsFixedJointDesc>("FixedJointDescVector");

    registerVectorConverter<UsdPhysicsDistanceJointDesc>("DistanceJointDescVector");

    registerVectorConverter<UsdPhysicsRevoluteJointDesc>("RevoluteJointDescVector");

    registerVectorConverter<UsdPhysicsPrismaticJointDesc>("PrismaticJointDescVector");

    registerVectorConverter<UsdPhysicsSphericalJointDesc>("SphericalJointDescVector");

    registerVectorConverter<UsdPhysicsD6JointDesc>("D6JointDescVector");

    registerVectorConverter<UsdPhysicsCustomJointDesc>("CustomJointDescVector");

    registerVectorConverter<UsdPhysicsRigidBodyMaterialDesc>("RigidBodyMaterialDescVector");

    registerVectorConverter<UsdPhysicsArticulationDesc>("ArticulationDescVector");

    registerVectorConverter<UsdPhysicsCollisionGroupDesc>("CollisionGroupDescVector");

    pxr_boost::python::class_<ParsePrimIteratorBaseWrap, noncopyable>
        vparseitclsvv("ParsePrimIteratorBaseWrap", no_init);
      vparseitclsvv.def(init<>());

    pxr_boost::python::class_<ParsePrimIteratorRange, bases<ParsePrimIteratorBase>>
        parseitcls("ParsePrimIteratorRange", no_init);
    parseitcls
        .def(init<UsdPrimRange>(arg("primRange")));        

    pxr_boost::python::class_<ExcludeListPrimIteratorRange, bases<ParsePrimIteratorBase>>
        parseitExccls("ExcludeListPrimIteratorRange", no_init);
    parseitExccls
        .def(init<UsdPrimRange,SdfPathVector>());

    def("LoadUsdPhysicsFromRange", _LoadUsdPhysicsFromRange, 
            (args("stage"), args("range"), args("customTokens") = _CustomUsdPhysicsTokens(), args("simulationOwners") = std::vector<SdfPath>()));
}
