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

template <typename T>
void registerVectorConverter(const char* name)
{
    pxr_boost::python::class_<std::vector<T>>(name).def(vector_indexing_suite<std::vector<T>>());
}

PXR_NAMESPACE_USING_DIRECTIVE

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
    std::vector<SceneDesc>          sceneDescs;

    SdfPathVector                   rigidBodyPrimPaths;
    std::vector<RigidBodyDesc>      rigidBodyDescs;

    SdfPathVector                   sphereShapePrimPaths;
    std::vector<SphereShapeDesc>    sphereShapeDescs;

    SdfPathVector                   cubeShapePrimPaths;
    std::vector<CubeShapeDesc>    cubeShapeDescs;

    SdfPathVector                   capsuleShapePrimPaths;
    std::vector<CapsuleShapeDesc>    capsuleShapeDescs;

    SdfPathVector                   cylinderShapePrimPaths;
    std::vector<CylinderShapeDesc>    cylinderShapeDescs;

    SdfPathVector            coneShapePrimPaths;
    std::vector<ConeShapeDesc>    coneShapeDescs;

    SdfPathVector            meshShapePrimPaths;
    std::vector<MeshShapeDesc>    meshShapeDescs;

    SdfPathVector            planeShapePrimPaths;
    std::vector<PlaneShapeDesc>    planeShapeDescs;

    SdfPathVector            customShapePrimPaths;
    std::vector<CustomShapeDesc>    customShapeDescs;

    SdfPathVector            spherePointShapePrimPaths;
    std::vector<SpherePointsShapeDesc>    spherePointsShapeDescs;

    SdfPathVector            fixedJointPrimPaths;
    std::vector<FixedJointDesc>    fixedJointDescs;

    SdfPathVector            revoluteJointPrimPaths;
    std::vector<RevoluteJointDesc>    revoluteJointDescs;

    SdfPathVector            prismaticJointPrimPaths;
    std::vector<PrismaticJointDesc>    prismaticJointDescs;

    SdfPathVector            sphericalJointPrimPaths;
    std::vector<SphericalJointDesc>    sphericalJointDescs;

    SdfPathVector            distanceJointPrimPaths;
    std::vector<DistanceJointDesc>    distanceJointDescs;

    SdfPathVector            d6JointPrimPaths;
    std::vector<D6JointDesc>    d6JointDescs;

    SdfPathVector            customJointPrimPaths;
    std::vector<CustomJointDesc>    customJointDescs;

    SdfPathVector            rigidBodyMaterialPrimPaths;
    std::vector<RigidBodyMaterialDesc>    rigidBodyMaterialDescs;

    SdfPathVector            articulationPrimPaths;
    std::vector<ArticulationDesc>    articulationDescs;

    SdfPathVector            collisionGroupPrimPaths;
    std::vector<CollisionGroupDesc>    collisionGroupDescs;

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
void copyDescs(size_t numDesc, const SdfPath* primsSource, SdfPathVector& primsDest, const PhysicsObjectDesc* objectDescsSource, std::vector<DescType>& objectDescsDest)
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

void ReportPhysicsObjectsFn(PhysicsObjectType::Enum type, size_t numDesc, const SdfPath* primPaths,
    const PhysicsObjectDesc* objectDescs, void* userData)
{
    MarshalCallback* cb = (MarshalCallback*)userData;
        
    switch (type)
    {
        case PhysicsObjectType::eScene:
        {       
            copyDescs(numDesc, primPaths, cb->scenePrimPaths, objectDescs, cb->sceneDescs);
        }        
        break;
        case PhysicsObjectType::eRigidBody:
        {       
            copyDescs(numDesc, primPaths, cb->rigidBodyPrimPaths, objectDescs, cb->rigidBodyDescs);
        }        
        break;        
        case PhysicsObjectType::eSphereShape:
        {       
            copyDescs(numDesc, primPaths, cb->sphereShapePrimPaths, objectDescs, cb->sphereShapeDescs);
        }  
        break;      
        case PhysicsObjectType::eCubeShape:
        {       
            copyDescs(numDesc, primPaths, cb->cubeShapePrimPaths, objectDescs, cb->cubeShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eCapsuleShape:
        {       
            copyDescs(numDesc, primPaths, cb->capsuleShapePrimPaths, objectDescs, cb->capsuleShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eCylinderShape:
        {       
            copyDescs(numDesc, primPaths, cb->cylinderShapePrimPaths, objectDescs, cb->cylinderShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eConeShape:
        {       
            copyDescs(numDesc, primPaths, cb->coneShapePrimPaths, objectDescs, cb->coneShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eMeshShape:
        {       
            copyDescs(numDesc, primPaths, cb->meshShapePrimPaths, objectDescs, cb->meshShapeDescs);
        }        
        break;        
        case PhysicsObjectType::ePlaneShape:
        {       
            copyDescs(numDesc, primPaths, cb->planeShapePrimPaths, objectDescs, cb->planeShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eCustomShape:
        {       
            copyDescs(numDesc, primPaths, cb->customShapePrimPaths, objectDescs, cb->customShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eSpherePointsShape:
        {       
            copyDescs(numDesc, primPaths, cb->spherePointShapePrimPaths, objectDescs, cb->spherePointsShapeDescs);
        }        
        break;        
        case PhysicsObjectType::eFixedJoint:
        {       
            copyDescs(numDesc, primPaths, cb->fixedJointPrimPaths, objectDescs, cb->fixedJointDescs);
        }        
        break;
        case PhysicsObjectType::eRevoluteJoint:
        {       
            copyDescs(numDesc, primPaths, cb->revoluteJointPrimPaths, objectDescs, cb->revoluteJointDescs);
        }     
        break;   
        case PhysicsObjectType::ePrismaticJoint:
        {       
            copyDescs(numDesc, primPaths, cb->prismaticJointPrimPaths, objectDescs, cb->prismaticJointDescs);
        }     
        break;   
        case PhysicsObjectType::eSphericalJoint:
        {       
            copyDescs(numDesc, primPaths, cb->sphericalJointPrimPaths, objectDescs, cb->sphericalJointDescs);
        }     
        break;   
        case PhysicsObjectType::eDistanceJoint:
        {       
            copyDescs(numDesc, primPaths, cb->distanceJointPrimPaths, objectDescs, cb->distanceJointDescs);
        }     
        break;   
        case PhysicsObjectType::eD6Joint:
        {       
            copyDescs(numDesc, primPaths, cb->d6JointPrimPaths, objectDescs, cb->d6JointDescs);
        }     
        break;   
        case PhysicsObjectType::eCustomJoint:
        {       
            copyDescs(numDesc, primPaths, cb->customJointPrimPaths, objectDescs, cb->customJointDescs);
        }     
        break;   
        case PhysicsObjectType::eRigidBodyMaterial:
        {       
            copyDescs(numDesc, primPaths, cb->rigidBodyMaterialPrimPaths, objectDescs, cb->rigidBodyMaterialDescs);
        }     
        break;   
        case PhysicsObjectType::eArticulation:
        {       
            copyDescs(numDesc, primPaths, cb->articulationPrimPaths, objectDescs, cb->articulationDescs);
        }     
        break;   
        case PhysicsObjectType::eCollisionGroup:
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
            retDict[PhysicsObjectType::eScene] = pxr_boost::python::make_tuple(gMarshalCallback.scenePrimPaths, gMarshalCallback.sceneDescs);
        }
        if (!gMarshalCallback.rigidBodyDescs.empty())
        {
            retDict[PhysicsObjectType::eRigidBody] = pxr_boost::python::make_tuple(gMarshalCallback.rigidBodyPrimPaths, gMarshalCallback.rigidBodyDescs);
        }
        if (!gMarshalCallback.sphereShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eSphereShape] = pxr_boost::python::make_tuple(gMarshalCallback.sphereShapePrimPaths, gMarshalCallback.sphereShapeDescs);
        }
        if (!gMarshalCallback.cubeShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eCubeShape] = pxr_boost::python::make_tuple(gMarshalCallback.cubeShapePrimPaths, gMarshalCallback.cubeShapeDescs);
        }
        if (!gMarshalCallback.capsuleShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eCapsuleShape] = pxr_boost::python::make_tuple(gMarshalCallback.capsuleShapePrimPaths, gMarshalCallback.capsuleShapeDescs);
        }
        if (!gMarshalCallback.cylinderShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eCylinderShape] = pxr_boost::python::make_tuple(gMarshalCallback.cylinderShapePrimPaths, gMarshalCallback.cylinderShapeDescs);
        }
        if (!gMarshalCallback.coneShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eConeShape] = pxr_boost::python::make_tuple(gMarshalCallback.coneShapePrimPaths, gMarshalCallback.coneShapeDescs);
        }
        if (!gMarshalCallback.meshShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eMeshShape] = pxr_boost::python::make_tuple(gMarshalCallback.meshShapePrimPaths, gMarshalCallback.meshShapeDescs);
        }
        if (!gMarshalCallback.planeShapeDescs.empty())
        {
            retDict[PhysicsObjectType::ePlaneShape] = pxr_boost::python::make_tuple(gMarshalCallback.planeShapePrimPaths, gMarshalCallback.planeShapeDescs);
        }
        if (!gMarshalCallback.customShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eCustomShape] = pxr_boost::python::make_tuple(gMarshalCallback.customShapePrimPaths, gMarshalCallback.customShapeDescs);
        }
        if (!gMarshalCallback.spherePointsShapeDescs.empty())
        {
            retDict[PhysicsObjectType::eSpherePointsShape] = pxr_boost::python::make_tuple(gMarshalCallback.spherePointShapePrimPaths, gMarshalCallback.spherePointsShapeDescs);
        }
        if (!gMarshalCallback.fixedJointDescs.empty())
        {
            retDict[PhysicsObjectType::eFixedJoint] = pxr_boost::python::make_tuple(gMarshalCallback.fixedJointPrimPaths, gMarshalCallback.fixedJointDescs);
        }
        if (!gMarshalCallback.revoluteJointDescs.empty())
        {
            retDict[PhysicsObjectType::eRevoluteJoint] = pxr_boost::python::make_tuple(gMarshalCallback.revoluteJointPrimPaths, gMarshalCallback.revoluteJointDescs);
        }
        if (!gMarshalCallback.prismaticJointDescs.empty())
        {
            retDict[PhysicsObjectType::ePrismaticJoint] = pxr_boost::python::make_tuple(gMarshalCallback.prismaticJointPrimPaths, gMarshalCallback.prismaticJointDescs);
        }
        if (!gMarshalCallback.sphericalJointDescs.empty())
        {
            retDict[PhysicsObjectType::eSphericalJoint] = pxr_boost::python::make_tuple(gMarshalCallback.sphericalJointPrimPaths, gMarshalCallback.sphericalJointDescs);
        }
        if (!gMarshalCallback.distanceJointDescs.empty())
        {
            retDict[PhysicsObjectType::eDistanceJoint] = pxr_boost::python::make_tuple(gMarshalCallback.distanceJointPrimPaths, gMarshalCallback.distanceJointDescs);
        }
        if (!gMarshalCallback.d6JointDescs.empty())
        {
            retDict[PhysicsObjectType::eD6Joint] = pxr_boost::python::make_tuple(gMarshalCallback.d6JointPrimPaths, gMarshalCallback.d6JointDescs);
        }
        if (!gMarshalCallback.customJointDescs.empty())
        {
            retDict[PhysicsObjectType::eCustomJoint] = pxr_boost::python::make_tuple(gMarshalCallback.customJointPrimPaths, gMarshalCallback.customJointDescs);
        }
        if (!gMarshalCallback.rigidBodyMaterialDescs.empty())
        {
            retDict[PhysicsObjectType::eRigidBodyMaterial] = pxr_boost::python::make_tuple(gMarshalCallback.rigidBodyMaterialPrimPaths, gMarshalCallback.rigidBodyMaterialDescs);
        }
        if (!gMarshalCallback.articulationDescs.empty())
        {
            retDict[PhysicsObjectType::eArticulation] = pxr_boost::python::make_tuple(gMarshalCallback.articulationPrimPaths, gMarshalCallback.articulationDescs);
        }
        if (!gMarshalCallback.collisionGroupDescs.empty())
        {
            retDict[PhysicsObjectType::eCollisionGroup] = pxr_boost::python::make_tuple(gMarshalCallback.collisionGroupPrimPaths, gMarshalCallback.collisionGroupDescs);
        }
    }    
    return retDict;
}

static std::string
_CustomUsdPhysicsTokens_Repr(const CustomUsdPhysicsTokens& self)
{
    return TfStringPrintf("%sCustomUsdPhysicsTokens(jointTokens=%s, shapeTokens=%s, instancerTokens=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.jointTokens).c_str(),
        TfPyRepr(self.shapeTokens).c_str(),
        TfPyRepr(self.instancerTokens).c_str());
}

static std::string
_PhysicsObjectDesc_Repr(const PhysicsObjectDesc& self)
{
    return TfStringPrintf("%sPhysicsObjectDesc(type=%s, primPath=%s, isValid=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.type).c_str(),
        TfPyRepr(self.primPath).c_str(),
        TfPyRepr(self.isValid).c_str());
}

static std::string
_SceneDesc_Repr(const SceneDesc& self)
{
    return TfStringPrintf("%sSceneDesc(gravityDirection=%s, gravityMagnitude=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.gravityDirection).c_str(),
        TfPyRepr(self.gravityMagnitude).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_CollisionGroupDesc_Repr(const CollisionGroupDesc& self)
{
    return TfStringPrintf("%sCollisionGroupDesc(invertFilteredGroups=%s, mergeGroupName=%s, mergedGroups=%s, filteredGroups=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.invertFilteredGroups).c_str(),
        TfPyRepr(self.mergeGroupName).c_str(),
        TfPyRepr(self.mergedGroups).c_str(),
        TfPyRepr(self.filteredGroups).c_str(),        
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_RigidBodyMaterialDesc_Repr(const RigidBodyMaterialDesc& self)
{
    return TfStringPrintf("%sRigidBodyMaterialDesc(staticFriction=%s, dynamicFriction=%s, restitution=%s, density=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.staticFriction).c_str(),
        TfPyRepr(self.dynamicFriction).c_str(),
        TfPyRepr(self.restitution).c_str(),
        TfPyRepr(self.density).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_ShapeDesc_Repr(const ShapeDesc& self)
{
    return TfStringPrintf("%sShapeDesc(rigidBody=%s, localPos=%s, localRot=%s, localScale=%s, materials=%s, simulationOwners=%s, filteredCollisions=%s, collisionGroups=%s, collisionEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX,
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
_SphereShapeDesc_Repr(const SphereShapeDesc& self)
{
    return TfStringPrintf("%sSphereShapeDesc(radius=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.radius).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CapsuleShapeDesc_Repr(const CapsuleShapeDesc& self)
{
    return TfStringPrintf("%sCapsuleShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CylinderShapeDesc_Repr(const CylinderShapeDesc& self)
{
    return TfStringPrintf("%sCylinderShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_ConeShapeDesc_Repr(const ConeShapeDesc& self)
{
    return TfStringPrintf("%sConeShapeDesc(radius=%s, halfHeight=%s, axis=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.radius).c_str(),
        TfPyRepr(self.halfHeight).c_str(),
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_PlaneShapeDesc_Repr(const PlaneShapeDesc& self)
{
    return TfStringPrintf("%sPlaneShapeDesc(axis=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.axis).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CustomShapeDesc_Repr(const CustomShapeDesc& self)
{
    return TfStringPrintf("%sCustomShapeDesc(customGeometryToken=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.customGeometryToken).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_CubeShapeDesc_Repr(const CubeShapeDesc& self)
{
    return TfStringPrintf("%sCubeShapeDesc(halfExtents=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.halfExtents).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_MeshShapeDesc_Repr(const MeshShapeDesc& self)
{
    return TfStringPrintf("%sMeshShapeDesc(approximation=%s, meshScale=%s, doubleSided=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.approximation).c_str(),
        TfPyRepr(self.meshScale).c_str(),
        TfPyRepr(self.doubleSided).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_SpherePoint_Repr(const SpherePoint& self)
{
    return TfStringPrintf("%sSpherePoint(center=%s, radius=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.center).c_str(),
        TfPyRepr(self.radius).c_str());
}

static std::string
_SpherePointsShapeDesc_Repr(const SpherePointsShapeDesc& self)
{
    return TfStringPrintf("%sSpherePointsShapeDesc(spherePoints=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.spherePoints).c_str(),
        _ShapeDesc_Repr(self).c_str());
}

static std::string
_RigidBodyDesc_Repr(const RigidBodyDesc& self)
{
    return TfStringPrintf("%sRigidBodyDesc(collisions=%s, filteredCollisions=%s, simulationOwners=%s, position=%s, rotation=%s, scale=%s, rigidBodyEnabled=%s, kinematicBody=%s, startsAsleep=%s, linearVelocity=%s, angularVelocity=%s), parent %s",
        TF_PY_REPR_PREFIX,
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
_JointLimit_Repr(const JointLimit& self)
{
    return TfStringPrintf("%sJointLimit(enabled=%s, lower=%s, upper=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.enabled).c_str(),
        TfPyRepr(self.lower).c_str(),
        TfPyRepr(self.upper).c_str());
}

static std::string
_JointDrive_Repr(const JointDrive& self)
{
    return TfStringPrintf("%sJointDrive(enabled=%s, targetPosition=%s, targetVelocity=%s, forceLimit=%s, stiffness=%s, damping=%s, acceleration=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.enabled).c_str(),
        TfPyRepr(self.targetPosition).c_str(),
        TfPyRepr(self.targetVelocity).c_str(),
        TfPyRepr(self.forceLimit).c_str(),
        TfPyRepr(self.stiffness).c_str(),
        TfPyRepr(self.damping).c_str(),
        TfPyRepr(self.acceleration).c_str());
}

static std::string
_ArticulationDesc_Repr(const ArticulationDesc& self)
{
    return TfStringPrintf("%sArticulationDesc(rootPrims=%s, filteredCollisions=%s, articulatedJoints=%s, articulatedBodies=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.rootPrims).c_str(),
        TfPyRepr(self.filteredCollisions).c_str(),
        TfPyRepr(self.articulatedJoints).c_str(),
        TfPyRepr(self.articulatedBodies).c_str(),
        _PhysicsObjectDesc_Repr(self).c_str());
}

static std::string
_JointDesc_Repr(const JointDesc& self)
{
    return TfStringPrintf("%sJointDesc(rel0=%s, rel1=%s, body0=%s, body1=%s, localPose0Position=%s, localPose0Orientation=%s, localPose1Position=%s, localPose1Orientation=%s, jointEnabled=%s, breakForce=%s, breakTorque=%s, excludeFromArticulation=%s, collisionEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX,
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
_JointLimitDOFPair_Repr(const std::pair<PhysicsJointDOF::Enum, JointLimit>& self)
{
    return TfStringPrintf("%sJointLimitDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_JointDriveDOFPair_Repr(const std::pair<PhysicsJointDOF::Enum, JointDrive>& self)
{
    return TfStringPrintf("%sJointDriveDOFPair(first=%s, second=%s)",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.first).c_str(),
        TfPyRepr(self.second).c_str());
}

static std::string
_D6JointDesc_Repr(const D6JointDesc& self)
{
    return TfStringPrintf("%sD6JointDesc(jointLimits=%s, jointDrives=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.jointLimits).c_str(),
        TfPyRepr(self.jointDrives).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_PrismaticJointDesc_Repr(const PrismaticJointDesc& self)
{
    return TfStringPrintf("%sPrismaticJointDesc(axis=%s, limit=%s, drive=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.drive).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_SphericalJointDesc_Repr(const SphericalJointDesc& self)
{
    return TfStringPrintf("%sSphericalJointDesc(axis=%s, limit=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_RevoluteJointDesc_Repr(const RevoluteJointDesc& self)
{
    return TfStringPrintf("%sRevoluteJointDesc(axis=%s, limit=%s, drive=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.axis).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.drive).c_str(),
        _JointDesc_Repr(self).c_str());
}

static std::string
_DistanceJointDesc_Repr(const DistanceJointDesc& self)
{
    return TfStringPrintf("%sDistanceJointDesc(minEnabled=%s, limit=%s, maxEnabled=%s), parent %s",
        TF_PY_REPR_PREFIX,
        TfPyRepr(self.minEnabled).c_str(),
        TfPyRepr(self.limit).c_str(),
        TfPyRepr(self.maxEnabled).c_str(),
        _JointDesc_Repr(self).c_str());
}

void wrapParseUtils()
{
    pxr_boost::python::enum_<PhysicsObjectType::Enum>("PhysicsObjectType")
    .value("Undefined", PhysicsObjectType::eUndefined)
    .value("Scene", PhysicsObjectType::eScene)
    .value("RigidBody", PhysicsObjectType::eRigidBody)
    .value("SphereShape", PhysicsObjectType::eSphereShape)
    .value("CubeShape", PhysicsObjectType::eCubeShape)
    .value("CapsuleShape", PhysicsObjectType::eCapsuleShape)
    .value("CylinderShape", PhysicsObjectType::eCylinderShape)
    .value("ConeShape", PhysicsObjectType::eConeShape)
    .value("MeshShape", PhysicsObjectType::eMeshShape)
    .value("PlaneShape", PhysicsObjectType::ePlaneShape)
    .value("CustomShape", PhysicsObjectType::eCustomShape)
    .value("SpherePointsShape", PhysicsObjectType::eSpherePointsShape)
    .value("FixedJoint", PhysicsObjectType::eFixedJoint)
    .value("RevoluteJoint", PhysicsObjectType::eRevoluteJoint)
    .value("PrismaticJoint", PhysicsObjectType::ePrismaticJoint)
    .value("SphericalJoint", PhysicsObjectType::eSphericalJoint)
    .value("DistanceJoint", PhysicsObjectType::eDistanceJoint)
    .value("D6Joint", PhysicsObjectType::eD6Joint)
    .value("CustomJoint", PhysicsObjectType::eCustomJoint)
    .value("RigidBodyMaterial", PhysicsObjectType::eRigidBodyMaterial)
    .value("Articulation", PhysicsObjectType::eArticulation)
    .value("CollisionGroup", PhysicsObjectType::eCollisionGroup)
    ;

    pxr_boost::python::enum_<PhysicsAxis::Enum>("PhysicsAxis")
    .value("X", PhysicsAxis::eX)
    .value("Y", PhysicsAxis::eY)
    .value("Z", PhysicsAxis::eZ)
    ;

    pxr_boost::python::enum_<PhysicsJointDOF::Enum>("PhysicsJointDOF")
    .value("Distance", PhysicsJointDOF::eDistance)
    .value("TransX", PhysicsJointDOF::eTransX)
    .value("TransY", PhysicsJointDOF::eTransY)
    .value("TransZ", PhysicsJointDOF::eTransZ)
    .value("RotX", PhysicsJointDOF::eRotX)
    .value("RotY", PhysicsJointDOF::eRotY)
    .value("RotZ", PhysicsJointDOF::eRotZ)
    ;
    
    pxr_boost::python::class_<_CustomUsdPhysicsTokens>
        cupt("CustomUsdPhysicsTokens");
    cupt
        .def_readwrite("jointTokens", &_CustomUsdPhysicsTokens::jointTokens)
        .def_readwrite("shapeTokens", &_CustomUsdPhysicsTokens::shapeTokens)
        .def_readwrite("instancerTokens", &_CustomUsdPhysicsTokens::instancerTokens)
        .def("__repr__", _CustomUsdPhysicsTokens_Repr);

    pxr_boost::python::class_<PhysicsObjectDesc>
        podcls("PhysicsObjectDesc", pxr_boost::python::no_init);
    podcls
        .def_readonly("type", &PhysicsObjectDesc::type)
        .def_readonly("primPath", &PhysicsObjectDesc::primPath)
        .def_readonly("isValid", &PhysicsObjectDesc::isValid)
        .def("__repr__", _PhysicsObjectDesc_Repr);

    pxr_boost::python::class_<SceneDesc, bases<PhysicsObjectDesc>>
        sdcls("SceneDesc", pxr_boost::python::no_init);
    sdcls
        .def_readonly("gravityDirection", &SceneDesc::gravityDirection)        
        .def_readonly("gravityMagnitude", &SceneDesc::gravityMagnitude)
        .def("__repr__", _SceneDesc_Repr);

    pxr_boost::python::class_<CollisionGroupDesc, bases<PhysicsObjectDesc>>
        cgcls("CollisionGroupDesc", pxr_boost::python::no_init);
    cgcls
        .def_readonly("invertFilteredGroups", &CollisionGroupDesc::invertFilteredGroups)
        .add_property("mergedGroups", make_function(&CollisionGroupDesc::getMergedGroups,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredGroups", make_function(&CollisionGroupDesc::getFilteredGroups,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("mergeGroupName", &CollisionGroupDesc::mergeGroupName)
        .def("__repr__", _CollisionGroupDesc_Repr);

    pxr_boost::python::class_<RigidBodyMaterialDesc, bases<PhysicsObjectDesc>>
        rbmcls("RigidBodyMaterialDesc", pxr_boost::python::no_init);
    rbmcls
        .def_readonly("staticFriction", &RigidBodyMaterialDesc::staticFriction)
        .def_readonly("dynamicFriction", &RigidBodyMaterialDesc::dynamicFriction)
        .def_readonly("restitution", &RigidBodyMaterialDesc::restitution)
        .def_readonly("density", &RigidBodyMaterialDesc::density)
        .def("__repr__", _RigidBodyMaterialDesc_Repr);

    pxr_boost::python::class_<ShapeDesc, bases<PhysicsObjectDesc>>
        shdcls("ShapeDesc", pxr_boost::python::no_init);
    shdcls
        .def_readonly("rigidBody", &ShapeDesc::rigidBody)
        .def_readonly("localPos", &ShapeDesc::localPos)
        .def_readonly("localRot", &ShapeDesc::localRot)
        .def_readonly("localScale", &ShapeDesc::localScale)
        .add_property("materials", make_function(&ShapeDesc::getMaterials,
            return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners", make_function(&ShapeDesc::getSimulationOwners,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&ShapeDesc::getFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("collisionGroups", make_function(&ShapeDesc::getCollisionGroups,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("collisionEnabled", &ShapeDesc::collisionEnabled)
        .def("__repr__", _ShapeDesc_Repr);

    pxr_boost::python::class_<SphereShapeDesc, bases<ShapeDesc>>
        ssdcls("SphereShapeDesc", pxr_boost::python::no_init);
    ssdcls
        .def_readonly("radius", &SphereShapeDesc::radius)
        .def("__repr__", _SphereShapeDesc_Repr);

    pxr_boost::python::class_<CapsuleShapeDesc, bases<ShapeDesc>>
        csdcls("CapsuleShapeDesc", pxr_boost::python::no_init);
    csdcls
        .def_readonly("radius", &CapsuleShapeDesc::radius)
        .def_readonly("halfHeight", &CapsuleShapeDesc::halfHeight)
        .def_readonly("axis", &CapsuleShapeDesc::axis)
        .def("__repr__", _CapsuleShapeDesc_Repr);

    pxr_boost::python::class_<CylinderShapeDesc, bases<ShapeDesc>>
        cysdcls("CylinderShapeDesc", pxr_boost::python::no_init);
    cysdcls
        .def_readonly("radius", &CylinderShapeDesc::radius)
        .def_readonly("halfHeight", &CylinderShapeDesc::halfHeight)
        .def_readonly("axis", &CylinderShapeDesc::axis)
        .def("__repr__", _CylinderShapeDesc_Repr);

    pxr_boost::python::class_<ConeShapeDesc, bases<ShapeDesc>>
        cosdcls("ConeShapeDesc", pxr_boost::python::no_init);
    cosdcls
        .def_readonly("radius", &ConeShapeDesc::radius)
        .def_readonly("halfHeight", &ConeShapeDesc::halfHeight)
        .def_readonly("axis", &ConeShapeDesc::axis)
        .def("__repr__", _ConeShapeDesc_Repr);

    pxr_boost::python::class_<PlaneShapeDesc, bases<ShapeDesc>>
        pscls("PlaneShapeDesc", pxr_boost::python::no_init);
    pscls
        .def_readonly("axis", &PlaneShapeDesc::axis)
        .def("__repr__", _PlaneShapeDesc_Repr);

    pxr_boost::python::class_<CustomShapeDesc, bases<ShapeDesc>>
        cuscls("CustomShapeDesc", pxr_boost::python::no_init);
    cuscls
        .def_readonly("customGeometryToken", &CustomShapeDesc::customGeometryToken)
        .def("__repr__", _CustomShapeDesc_Repr);

    pxr_boost::python::class_<CubeShapeDesc, bases<ShapeDesc>>
        cubescls("CubeShapeDesc", pxr_boost::python::no_init);
    cubescls
        .def_readonly("halfExtents", &CubeShapeDesc::halfExtents)
        .def("__repr__", _CubeShapeDesc_Repr);

    pxr_boost::python::class_<MeshShapeDesc, bases<ShapeDesc>>
        mscls("MeshShapeDesc", pxr_boost::python::no_init);
    mscls
        .add_property("approximation", &MeshShapeDesc::GetApproximation)
        .def_readonly("meshScale", &MeshShapeDesc::meshScale)
        .def_readonly("doubleSided", &MeshShapeDesc::doubleSided)
        .def("__repr__", _MeshShapeDesc_Repr);

    pxr_boost::python::class_<SpherePoint>
        sppb("SpherePoint", pxr_boost::python::no_init);
    sppb
        .def_readonly("center", &SpherePoint::center)
        .def_readonly("radius", &SpherePoint::radius)
        .def("__repr__", _SpherePoint_Repr);

    pxr_boost::python::class_<SpherePointsShapeDesc, bases<ShapeDesc>>
        spmscls("SpherePointsShapeDesc", pxr_boost::python::no_init);
    spmscls
        .add_property("spherePoints", &SpherePointsShapeDesc::spherePoints)
        .def("__repr__", _SpherePointsShapeDesc_Repr);

    pxr_boost::python::class_<RigidBodyDesc, bases<PhysicsObjectDesc>>
        rbcls("RigidBodyDesc", pxr_boost::python::no_init);
    rbcls
        .add_property("collisions", make_function(&RigidBodyDesc::getCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&RigidBodyDesc::getFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("simulationOwners", make_function(&RigidBodyDesc::getSimulationOwners,
            return_value_policy<TfPySequenceToList>()))
        .def_readonly("position", &RigidBodyDesc::position)
        .def_readonly("rotation", &RigidBodyDesc::rotation)
        .def_readonly("scale", &RigidBodyDesc::scale)
        .def_readonly("rigidBodyEnabled", &RigidBodyDesc::rigidBodyEnabled)
        .def_readonly("kinematicBody", &RigidBodyDesc::kinematicBody)
        .def_readonly("startsAsleep", &RigidBodyDesc::startsAsleep)
        .def_readonly("linearVelocity", &RigidBodyDesc::linearVelocity)
        .def_readonly("angularVelocity", &RigidBodyDesc::angularVelocity)
        .def("__repr__", _RigidBodyDesc_Repr);

    pxr_boost::python::class_<JointLimit>
        jlcls("JointLimit", pxr_boost::python::no_init);
    jlcls
        .def_readonly("enabled", &JointLimit::enabled)
        .def_readonly("lower", &JointLimit::lower)
        .def_readonly("upper", &JointLimit::upper)
        .def("__repr__", _JointLimit_Repr);

    pxr_boost::python::class_<JointDrive>
        jdcls("JointDrive", pxr_boost::python::no_init);
    jdcls
        .def_readonly("enabled", &JointDrive::enabled)
        .def_readonly("targetPosition", &JointDrive::targetPosition)
        .def_readonly("targetVelocity", &JointDrive::targetVelocity)
        .def_readonly("forceLimit", &JointDrive::forceLimit)
        .def_readonly("stiffness", &JointDrive::stiffness)
        .def_readonly("damping", &JointDrive::damping)
        .def_readonly("acceleration", &JointDrive::acceleration)
        .def("__repr__", _JointDrive_Repr);

    pxr_boost::python::class_<ArticulationDesc, bases<PhysicsObjectDesc>>
        adcls("ArticulationDesc", pxr_boost::python::no_init);
    adcls
        .add_property("rootPrims", make_function(&ArticulationDesc::GetRootPrims,
            return_value_policy<TfPySequenceToList>()))
        .add_property("filteredCollisions", make_function(&ArticulationDesc::GetFilteredCollisions,
            return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedJoints", make_function(&ArticulationDesc::GetArticulatedJoints,
            return_value_policy<TfPySequenceToList>()))
        .add_property("articulatedBodies", make_function(&ArticulationDesc::GetArticulatedBodies,
            return_value_policy<TfPySequenceToList>()))
        .def("__repr__", _ArticulationDesc_Repr);

    pxr_boost::python::class_<JointDesc, bases<PhysicsObjectDesc>>
        jdscls("JointDesc", pxr_boost::python::no_init);
    jdscls
        .def_readonly("rel0", &JointDesc::rel0)
        .def_readonly("rel1", &JointDesc::rel1)
        .def_readonly("body0", &JointDesc::body0)
        .def_readonly("body1", &JointDesc::body1)
        .def_readonly("localPose0Position", &JointDesc::localPose0Position)
        .def_readonly("localPose0Orientation", &JointDesc::localPose0Orientation)
        .def_readonly("localPose1Position", &JointDesc::localPose1Position)
        .def_readonly("localPose1Orientation", &JointDesc::localPose1Orientation)
        .def_readonly("jointEnabled", &JointDesc::jointEnabled)
        .def_readonly("breakForce", &JointDesc::breakForce)
        .def_readonly("breakTorque", &JointDesc::breakTorque)
        .def_readonly("excludeFromArticulation", &JointDesc::excludeFromArticulation)
        .def_readonly("collisionEnabled", &JointDesc::collisionEnabled)
        .def("__repr__", _JointDesc_Repr);

    pxr_boost::python::class_<CustomJointDesc, bases<JointDesc>>
        cjdscls("CustomJointDesc", pxr_boost::python::no_init);

    pxr_boost::python::class_<FixedJointDesc, bases<JointDesc>>
        fjdscls("FixedJointDesc", pxr_boost::python::no_init);

    class_<std::pair<PhysicsJointDOF::Enum, JointLimit> >("JointLimitDOFPair")
        .def_readwrite("first", &std::pair<PhysicsJointDOF::Enum, JointLimit>::first)
        .def_readwrite("second", &std::pair<PhysicsJointDOF::Enum, JointLimit>::second)
        .def("__repr__", _JointLimitDOFPair_Repr);

    class_<std::pair<PhysicsJointDOF::Enum, JointDrive> >("JointDriveDOFPair")
        .def_readwrite("first", &std::pair<PhysicsJointDOF::Enum, JointDrive>::first)
        .def_readwrite("second", &std::pair<PhysicsJointDOF::Enum, JointDrive>::second)
        .def("__repr__", _JointDriveDOFPair_Repr);

    pxr_boost::python::class_<D6JointDesc, bases<JointDesc>>
        d6jdscls("D6JointDesc", pxr_boost::python::no_init);
    d6jdscls
        .def_readonly("jointLimits", &D6JointDesc::jointLimits)
        .def_readonly("jointDrives", &D6JointDesc::jointDrives)
        .def("__repr__", _D6JointDesc_Repr);

    pxr_boost::python::class_<PrismaticJointDesc, bases<JointDesc>>
        pjdscls("PrismaticJointDesc", pxr_boost::python::no_init);
    pjdscls
        .def_readonly("axis", &PrismaticJointDesc::axis)
        .def_readonly("limit", &PrismaticJointDesc::limit)
        .def_readonly("drive", &PrismaticJointDesc::drive)
        .def("__repr__", _PrismaticJointDesc_Repr);

    pxr_boost::python::class_<SphericalJointDesc, bases<JointDesc>>
        sjdscls("SphericalJointDesc", pxr_boost::python::no_init);
    sjdscls
        .def_readonly("axis", &SphericalJointDesc::axis)
        .def_readonly("limit", &SphericalJointDesc::limit)
        .def("__repr__", _SphericalJointDesc_Repr);

    pxr_boost::python::class_<RevoluteJointDesc, bases<JointDesc>>
        rjdscls("RevoluteJointDesc", pxr_boost::python::no_init);
    rjdscls
        .def_readonly("axis", &RevoluteJointDesc::axis)
        .def_readonly("limit", &RevoluteJointDesc::limit)
        .def_readonly("drive", &RevoluteJointDesc::drive)
        .def("__repr__", _RevoluteJointDesc_Repr);

    pxr_boost::python::class_<DistanceJointDesc, bases<JointDesc>>
        djdscls("DistanceJointDesc", pxr_boost::python::no_init);
    djdscls
        .def_readonly("minEnabled", &DistanceJointDesc::minEnabled)
        .def_readonly("limit", &DistanceJointDesc::limit)
        .def_readonly("maxEnabled", &DistanceJointDesc::maxEnabled)
        .def("__repr__", _DistanceJointDesc_Repr);

    registerVectorConverter<UsdCollectionMembershipQuery>("PhysicsCollectionMembershipQueryVector");

    registerVectorConverter<std::pair<PhysicsJointDOF::Enum, JointLimit>>("PhysicsJointLimitDOFVector");

    registerVectorConverter<std::pair<PhysicsJointDOF::Enum, JointDrive>>("PhysicsJointDriveDOFVector");

    registerVectorConverter<SpherePoint>("PhysicsSpherePointVector");

    registerVectorConverter<SceneDesc>("SceneDescVector");

    registerVectorConverter<RigidBodyDesc>("RigidBodyDescVector");

    registerVectorConverter<SphereShapeDesc>("SphereShapeDescVector");

    registerVectorConverter<CapsuleShapeDesc>("CapsuleShapeDescVector");

    registerVectorConverter<CylinderShapeDesc>("CylinderShapeDescVector");

    registerVectorConverter<ConeShapeDesc>("ConeShapeDescVector");

    registerVectorConverter<CubeShapeDesc>("CubeShapeDescVector");

    registerVectorConverter<MeshShapeDesc>("MeshShapeDescVector");

    registerVectorConverter<PlaneShapeDesc>("PlaneShapeDescVector");

    registerVectorConverter<CustomShapeDesc>("CustomShapeDescVector");

    registerVectorConverter<SpherePointsShapeDesc>("SpherePointsShapeDescVector");

    registerVectorConverter<JointDesc>("JointDescVector");

    registerVectorConverter<FixedJointDesc>("FixedJointDescVector");

    registerVectorConverter<DistanceJointDesc>("DistanceJointDescVector");

    registerVectorConverter<RevoluteJointDesc>("RevoluteJointDescVector");

    registerVectorConverter<PrismaticJointDesc>("PrismaticJointDescVector");

    registerVectorConverter<SphericalJointDesc>("SphericalJointDescVector");

    registerVectorConverter<D6JointDesc>("D6JointDescVector");

    registerVectorConverter<CustomJointDesc>("CustomJointDescVector");

    registerVectorConverter<RigidBodyMaterialDesc>("RigidBodyMaterialDescVector");

    registerVectorConverter<ArticulationDesc>("ArticulationDescVector");

    registerVectorConverter<CollisionGroupDesc>("CollisionGroupDescVector");

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
