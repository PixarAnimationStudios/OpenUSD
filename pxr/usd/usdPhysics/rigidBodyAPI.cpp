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
#include "pxr/usd/usdPhysics/rigidBodyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsRigidBodyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (PhysicsRigidBodyAPI)
);

/* virtual */
UsdPhysicsRigidBodyAPI::~UsdPhysicsRigidBodyAPI()
{
}

/* static */
UsdPhysicsRigidBodyAPI
UsdPhysicsRigidBodyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsRigidBodyAPI();
    }
    return UsdPhysicsRigidBodyAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsRigidBodyAPI::_GetSchemaKind() const
{
    return UsdPhysicsRigidBodyAPI::schemaKind;
}

/* static */
bool
UsdPhysicsRigidBodyAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsRigidBodyAPI>(whyNot);
}

/* static */
UsdPhysicsRigidBodyAPI
UsdPhysicsRigidBodyAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsRigidBodyAPI>()) {
        return UsdPhysicsRigidBodyAPI(prim);
    }
    return UsdPhysicsRigidBodyAPI();
}

/* static */
const TfType &
UsdPhysicsRigidBodyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsRigidBodyAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsRigidBodyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsRigidBodyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetRigidBodyEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsRigidBodyEnabled);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateRigidBodyEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsRigidBodyEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetKinematicEnabledAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsKinematicEnabled);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateKinematicEnabledAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsKinematicEnabled,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetStartsAsleepAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsStartsAsleep);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateStartsAsleepAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsStartsAsleep,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetVelocityAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsVelocity);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsVelocity,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::GetAngularVelocityAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsAngularVelocity);
}

UsdAttribute
UsdPhysicsRigidBodyAPI::CreateAngularVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsAngularVelocity,
                       SdfValueTypeNames->Vector3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdPhysicsRigidBodyAPI::GetSimulationOwnerRel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsSimulationOwner);
}

UsdRelationship
UsdPhysicsRigidBodyAPI::CreateSimulationOwnerRel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsSimulationOwner,
                       /* custom = */ false);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdPhysicsRigidBodyAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsRigidBodyEnabled,
        UsdPhysicsTokens->physicsKinematicEnabled,
        UsdPhysicsTokens->physicsStartsAsleep,
        UsdPhysicsTokens->physicsVelocity,
        UsdPhysicsTokens->physicsAngularVelocity,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdPhysics/massProperties.h"
#include "pxr/usd/usdPhysics/massAPI.h"
#include "pxr/usd/usdPhysics/collisionAPI.h"
#include "pxr/usd/usdPhysics/materialAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/base/gf/transform.h"

PXR_NAMESPACE_OPEN_SCOPE

const float COMPARE_TOLERANCE = 1e-05f;

struct MassApiData
{
    float       mass = -1.0f;
    float       density = -1.0f;
    bool        hasInertia = false;
    GfVec3f    diagonalInertia = { 1.0f, 1.0f, 1.0f };
};

struct InternalMassAccumulationData
{
    bool accumulateMass; // if true it indicates we are summing up the mass from density/child mass calculations
    float mass = -1.0f; //-1.0 means it is not set yet
    GfVec3f diagonalizedInertiaTensor = { 0.0f, 0.0f, 0.0f };
    GfVec3f centerOfMass = { 0.0f, 0.0f, 0.0f };
    GfQuatf principalAxes;
    float density = -1.0f;
};

struct MassInformation
{
    float volume;
    GfMatrix3f inertia; 
    GfVec3f centerOfMass;
    GfVec3f localPos;
    GfQuatf localRot;
};


MassApiData ParseMassApi(const UsdPrim& usdPrim)
{
    MassApiData result;

    if (usdPrim.HasAPI<UsdPhysicsMassAPI>())
    {
        float metersPerUnit = float(UsdGeomGetStageMetersPerUnit(usdPrim.GetStage()));

        // density
        const UsdPhysicsMassAPI massAPI(usdPrim);
        const UsdAttribute densityAttribute = massAPI.GetDensityAttr();
        const UsdAttribute massAttribute = massAPI.GetMassAttr();
        const UsdAttribute diagonalInertia = massAPI.GetDiagonalInertiaAttr();

        float d;
        densityAttribute.Get(&d);
        if (d > 0.0f)
        {
            result.density = d / (metersPerUnit * metersPerUnit * metersPerUnit);
        }

        float m;
        massAttribute.Get(&m);
        if (m > 0.0f)
        {
            result.mass = m;
        }

        GfVec3f dg;
        diagonalInertia.Get(&dg);        
        if (!GfIsClose(dg, GfVec3f(0.0f), COMPARE_TOLERANCE))
        {
            result.hasInertia = true;
            result.diagonalInertia = dg;
        }
    }
    return result;
}

bool GetCoM(const UsdPrim& usdPrim, GfVec3f& com, UsdGeomXformCache& xfCache)
{
    bool comSet = false;
    if (usdPrim.HasAPI<UsdPhysicsMassAPI>())
    {
        // com
        const UsdPhysicsMassAPI massAPI(usdPrim);
        const UsdAttribute comAttribute = massAPI.GetCenterOfMassAttr();

        GfVec3f v;
        comAttribute.Get(&v);
        if (!GfIsClose(v, GfVec3f(0.0f), COMPARE_TOLERANCE))
        {            
            // need to extract scale as physics in general does not support scale, we need to scale the com
            GfMatrix4d mat = xfCache.GetLocalToWorldTransform(usdPrim);
            const GfTransform tr(mat);
            const GfVec3f sc = GfVec3f(tr.GetScale());

            com[0] = v[0] * sc[0];
            com[1] = v[1] * sc[1];
            com[2] = v[2] * sc[2];

            comSet = true;
        }
    }
    return comSet;
}

void GetPrincipalAxes(const UsdPrim& usdPrim, GfQuatf& pa)
{    
    pa = GfQuatf(1.0f);
    if (usdPrim.HasAPI<UsdPhysicsMassAPI>())
    {
        // principal exes
        const UsdPhysicsMassAPI massAPI(usdPrim);
        const UsdAttribute paAttribute = massAPI.GetPrincipalAxesAttr();

        paAttribute.Get(&pa);
    }
}

SdfPath GetMaterialBinding(const UsdPrim& usdPrim)
{
    SdfPath materialPath;

    UsdShadeMaterialBindingAPI materialBindingAPI = UsdShadeMaterialBindingAPI(usdPrim);
    if (materialBindingAPI)
    {
        const static TfToken physicsPurpose("physics");
        UsdShadeMaterialBindingAPI::DirectBinding binding = materialBindingAPI.GetDirectBinding(physicsPurpose);
        if (binding.GetMaterialPath() == SdfPath())
        {
            binding = materialBindingAPI.GetDirectBinding();
            return binding.GetMaterialPath();
        }
        else
        {
            return binding.GetMaterialPath();
        }
    }
    else
    {
        // handle material through a direct binding rel search
        UsdRelationship materialRel;
        SdfPathVector materials;
        static TfToken materialPhysicsBinding("material:binding:physics");
        static TfToken materialGeneralBinding("material:binding");
        materialRel = usdPrim.GetRelationship(materialPhysicsBinding);
        if (materialRel)
        {
            materialRel.GetTargets(&materials);
        }
        else
        {
            materialRel = usdPrim.GetRelationship(materialGeneralBinding);
            if (materialRel)
                materialRel.GetTargets(&materials);
        }

        if (materials.size() != 0)
        {
            return materials[0].GetPrimPath();
        }
    }

    return SdfPath();
}

MassApiData GetCollisionShapeMassAPIData(const UsdPrim& prim, float bodyDensity, float& density)
{    
    SdfPath materialPath = GetMaterialBinding(prim);

    MassApiData shapeMassInfo = ParseMassApi(prim);
    if (shapeMassInfo.density <= 0.0)
    {
        shapeMassInfo.density = bodyDensity; // use parent density if shape doesn't have one specified
    }

    // handle material
    density = shapeMassInfo.density;
    if (density <= 0.0f) // density not set, so we take it from the materials
    {
        if (materialPath != SdfPath::EmptyPath())
        {
            const UsdPhysicsMaterialAPI materialAPI = UsdPhysicsMaterialAPI::Get(prim.GetStage(), materialPath);

            if (materialAPI)
            {

                UsdAttribute densityAttr = materialAPI.GetDensityAttr();
                float materialDensity;
                densityAttr.Get(&materialDensity);

                if (materialDensity > 0.0f)
                {
                    float metersPerUnit = float(UsdGeomGetStageMetersPerUnit(prim.GetStage()));
                    density = materialDensity / (metersPerUnit * metersPerUnit * metersPerUnit);
                }
            }
        }
    }

    return shapeMassInfo;
}


MassProperties ParseCollisionShapeForMass(const UsdPrim& prim, const MassApiData& inShapeMassInfo, float density,
                                          GfMatrix4f& transform, UsdGeomXformCache& xformCache)
{
    MassApiData shapeMassInfo = inShapeMassInfo;
    GfMatrix3f inertia;    

    // Get the actuall mass information for the prim
    MassInformation massInfo;
    if (0) 
    {
        // massInfo = UsdLoad::getUsdLoad()->getPhysicsInterface()->getShapeMassInfo(primPath, shapeObjectId);
        // memcpy(inertia.data(), &massInfo.inertia[0], sizeof(float) * 9);
    }

    // if no density was set, use 1000 as default
    if (density <= 0.0f)
        density = 1000.0f;

    GfVec3f centerOfMass(0.0f);
    GfQuatf principalAxes(1.0f);
    const bool hasCoM = GetCoM(prim, centerOfMass, xformCache);

    // A.B. I should get here the principal axis too!

    if (shapeMassInfo.mass > 0.0f)
    {
        inertia = inertia * (shapeMassInfo.mass / massInfo.volume);
    }
    else if (massInfo.volume >= 0.0f)
    {
        shapeMassInfo.mass = massInfo.volume * density;
        inertia = inertia * density;
    }

    if (shapeMassInfo.hasInertia)
    {        
        const GfMatrix3f rotMatr(principalAxes);
        GfMatrix3f inMatr(0.0f);
        inMatr[0][0] = shapeMassInfo.diagonalInertia[0];
        inMatr[1][1] = shapeMassInfo.diagonalInertia[1];
        inMatr[2][2] = shapeMassInfo.diagonalInertia[2];

        inertia = inMatr * rotMatr;
    }

    if (hasCoM)
    {
        if (!shapeMassInfo.hasInertia)
        {
            // update inertia if we override the CoM but use the computed inertia
            MassProperties massProps(shapeMassInfo.mass, inertia, massInfo.centerOfMass);            
            massProps.Translate(centerOfMass - massProps.GetCenterOfMass());
            inertia = massProps.GetInertiaTensor();
        }
        massInfo.centerOfMass = centerOfMass;
    }

    transform.SetTranslate(massInfo.localPos);
    transform.SetRotateOnly(GfQuatd(massInfo.localRot));

    return MassProperties(shapeMassInfo.mass, inertia, massInfo.centerOfMass);
}


float ComputeMassProperties(const UsdPrim &prim, GfVec3f& diagonalInertia, GfVec3f& com, GfQuatf& principalAxes)
{
    MassProperties massProps;

    UsdGeomXformCache xfCache;

    // We assume that usdPrim is a dynamic body. Callers responsibility to check
    InternalMassAccumulationData massDesc;
    massDesc.principalAxes = GfQuatf(1.0f);

    // Parse dynamic body mass data
    MassApiData massInfo = parseMassApi(stage, usdPrim);
    massDesc.density = massInfo.density;
    massDesc.mass = massInfo.mass;
    massDesc.diagonalizedInertiaTensor = massInfo.diagonalInertia;
    massDesc.accumulateMass = massDesc.mass <= 0.0f;

    // check for CoM
    carb::Float3 centerOfMass = { 0.0f, 0.0f, 0.0f };
    carb::Float4 principalAxes = { 0.0f, 0.0f, 0.0f, 1.0f };
    const bool hasCoM = getCoM(stage, usdPrim, centerOfMass);
    const bool hasPa = getPrincipalAxes(stage, usdPrim, principalAxes);

    if (massDesc.accumulateMass || !massInfo.hasInertia || !hasCoM)
    {
        std::vector<MassProperties> massProps;
        std::vector<GfMatrix4f> massTransf;
        std::unordered_map<usdparser::ObjectId, UsdPrim> shapeIds;

        UsdLoad::getUsdLoad()->getPhysXPhysicsInterface()->getRigidBodyShapes(it->second, shapeIds);
        const size_t numShapes = shapeIds.size();
        massProps.reserve(numShapes);
        massTransf.reserve(numShapes);

        for (const std::pair<usdparser::ObjectId, UsdPrim>& shapePair : shapeIds)
        {
            float shapeDensity = 0.0f;
            const UsdPrim& shapePrim = shapePair.second;

            if (!shapePrim)
                continue;

            MassApiData massAPIdata = getCollisionShapeMassAPIData(stage, shapePrim, massDesc.density, shapeDensity);

            GfMatrix4f matrix;
            massProps.push_back(parseCollisionShapeForMass(stage, shapePrim, shapePair.first, massAPIdata, shapeDensity, matrix));
            massTransf.push_back(matrix);
        }

        if (!massProps.empty())
        {
            MassProperties accumulatedMassProps =
                MassProperties::sum(massProps.data(), massTransf.data(), uint32_t(massProps.size()));

            // if we had to compute mass, set the new mass
            if (massDesc.accumulateMass)
            {
                massDesc.mass = accumulatedMassProps.mass;
            }
            else
            {
                const double massDiff = massDesc.mass / accumulatedMassProps.mass;
                accumulatedMassProps.mass = massDesc.mass;
                accumulatedMassProps.inertiaTensor = accumulatedMassProps.inertiaTensor * massDiff;
            }

            if (!hasCoM)
            {
                centerOfMass.x = accumulatedMassProps.centerOfMass[0];
                centerOfMass.y = accumulatedMassProps.centerOfMass[1];
                centerOfMass.z = accumulatedMassProps.centerOfMass[2];
            }
            else
            {
                const GfVec3f newCenterOfMass(centerOfMass.x, centerOfMass.y, centerOfMass.z);
                accumulatedMassProps.translate(newCenterOfMass - accumulatedMassProps.centerOfMass);
            }

            GfQuatf accPa;
            const GfVec3f accInertia = MassProperties::getMassSpaceInertia(accumulatedMassProps.inertiaTensor, accPa);

            // check for inertia override
            if (!massInfo.hasInertia)
            {
                massDesc.diagonalizedInertiaTensor = accInertia;
            }

            if (!hasPa)
            {
                principalAxes.x = accPa.GetImaginary()[0];
                principalAxes.y = accPa.GetImaginary()[1];
                principalAxes.z = accPa.GetImaginary()[2];
                principalAxes.w = accPa.GetReal();
            }
        }
        else
        {
            // no shape provided check inertia
            if (!massInfo.hasInertia)
            {
                // In the absence of collision shapes and a specified inertia tensor, approximate
                // the tensor using a sphere. If the mass is not specified
                // throw a warning instead. Equation for spherical intertial tensor is (2/5 or
                // 0.4)*mass*radius^2, where we use 0.1 radius to imitate point.

                if (massDesc.mass > 0.0f)
                {
                    const float metersPerUnit = float(UsdGeomGetStageMetersPerUnit(stage));
                    const float radius = 0.1f / metersPerUnit;
                    const float inertiaVal = 0.4f * massDesc.mass * radius * radius;
                    massDesc.diagonalizedInertiaTensor[0] = inertiaVal;
                    massDesc.diagonalizedInertiaTensor[1] = inertiaVal;
                    massDesc.diagonalizedInertiaTensor[2] = inertiaVal;
                    CARB_LOG_INFO(
                        "The rigid body at %s has a possibly invalid inertia tensor of {1.0, 1.0, 1.0}, small sphere approximated inertia was used. %s %s",
                        primPath.GetString().c_str(),
                        "Either specify correct values in the mass properties, or add collider(s) to any shape(s) that you wish to automatically compute mass properties for.",
                        "If you do not want the objects to collide, add colliders regardless then disable the 'enable collision' property.");
                }
                else
                {
                    CARB_LOG_WARN(
                        "The rigid body at %s has a possibly invalid inertia tensor of {1.0, 1.0, 1.0}%s. %s %s",
                        primPath.GetString().c_str(), (massDesc.mass < 0.0f) ? " and a negative mass" : "",
                        "Either specify correct values in the mass properties, or add collider(s) to any shape(s) that you wish to automatically compute mass properties for.",
                        "If you do not want the objects to collide, add colliders regardless then disable the 'enable collision' property.");
                }
            }
        }
    }

    const carb::Float3 diagInertia = { massDesc.diagonalizedInertiaTensor[0], massDesc.diagonalizedInertiaTensor[1],
                                        massDesc.diagonalizedInertiaTensor[2] };

    UsdLoad::getUsdLoad()->getPhysicsInterface()->updateMass(
        primPath, it->second, massDesc.mass, diagInertia, centerOfMass, principalAxes);

    com = massProps.GetCenterOfMass();
    const GfMatrix3f& inertia = massProps.GetInertiaTensor();
    diagonalInertia = MassProperties::GetMassSpaceInertia(inertia, principalAxes);
    return massProps.GetMass();
}

PXR_NAMESPACE_CLOSE_SCOPE