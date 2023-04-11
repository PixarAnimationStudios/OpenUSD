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

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsRigidBodyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

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
#include "pxr/usd/usdPhysics/metrics.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _physicsPurposeTokens,
    ((materialPurposePhysics, "physics"))
);


const float COMPARE_TOLERANCE = 1e-05f;

struct _MassApiData
{
    float       mass = -1.0f;
    float       density = -1.0f;
    bool        hasInertia = false;
    GfVec3f     diagonalInertia = { 1.0f, 1.0f, 1.0f };
    bool        hasPa = false;
    GfQuatf     principalAxes;
};

// gather all the mass information for the gien prim, based on UsdPhysicsMassAPI
_MassApiData _ParseMassApi(const UsdPrim& usdPrim)
{
    _MassApiData result;

    if (usdPrim.HasAPI<UsdPhysicsMassAPI>())
    {        
        // density
        const UsdPhysicsMassAPI massAPI(usdPrim);
        const UsdAttribute densityAttribute = massAPI.GetDensityAttr();
        const UsdAttribute massAttribute = massAPI.GetMassAttr();
        const UsdAttribute diagonalInertia = massAPI.GetDiagonalInertiaAttr();
        const UsdAttribute principalAxes = massAPI.GetPrincipalAxesAttr();

        densityAttribute.Get(&result.density);

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
        
        // 0 0 0 0 is the sentinel value
        GfQuatf pa;
        principalAxes.Get(&pa);
        if (!GfIsClose(pa.GetImaginary(), GfVec3f(0.0f), COMPARE_TOLERANCE) || fabsf(pa.GetReal()) > COMPARE_TOLERANCE)
        {
            result.hasPa = true;
            result.principalAxes = pa;            
        }
    }
    return result;
}

// custom get center of mass, using also transformation to apply scaling
bool _GetCoM(const UsdPrim& usdPrim, GfVec3f* com, UsdGeomXformCache* xfCache)
{
    bool comSet = false;
    if (usdPrim.HasAPI<UsdPhysicsMassAPI>())
    {
        // com
        const UsdPhysicsMassAPI massAPI(usdPrim);
        const UsdAttribute comAttribute = massAPI.GetCenterOfMassAttr();

        GfVec3f v;
        comAttribute.Get(&v);        

        // -inf -inf -inf is the sentinel value, though any inf works
        if (isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]))
        {
            // need to extract scale as physics in general does not support scale, 
            //we need to scale the com
            GfMatrix4d mat = xfCache->GetLocalToWorldTransform(usdPrim);
            const GfTransform tr(mat);
            const GfVec3f sc = GfVec3f(tr.GetScale());

            *com = GfCompMult(v, sc);

            comSet = true;
        }
    }
    return comSet;
}

// get density form a collision prim, it will also check its materials eventually
_MassApiData _GetCollisionShapeMassAPIData(const UsdPhysicsCollisionAPI& collisionAPI, 
    float bodyDensity, float* density, const UsdShadeMaterial& materialPrim)
{    
    
    _MassApiData shapeMassInfo = _ParseMassApi(collisionAPI.GetPrim());
    if (shapeMassInfo.density <= 0.0)
    {
        // use parent density if shape doesn't have one specified
        shapeMassInfo.density = bodyDensity; 
    }

    // handle material
    *density = shapeMassInfo.density;
    if (shapeMassInfo.density <= 0.0f) // density not set, so we take it from the materials
    {
        if (materialPrim)
        {
            const UsdPhysicsMaterialAPI materialAPI = UsdPhysicsMaterialAPI(materialPrim.GetPrim());

            if (materialAPI)
            {
                UsdAttribute densityAttr = materialAPI.GetDensityAttr();                
                densityAttr.Get(density);
            }
        }
    }

    return shapeMassInfo;
}


// gather mass information for given collision shape
UsdPhysicsMassProperties _ParseCollisionShapeForMass(const UsdPrim& prim, 
    const _MassApiData& inShapeMassInfo, float density, GfMatrix4f* transform, 
    UsdGeomXformCache* xformCache, const UsdPhysicsRigidBodyAPI::MassInformationFn& massInfoFn)
{
    _MassApiData shapeMassInfo = inShapeMassInfo;        

    // Get the actuall mass information for the prim
    UsdPhysicsRigidBodyAPI::MassInformation massInfo = massInfoFn(prim);
    if (massInfo.volume < 0.0f)
    {
        TF_DIAGNOSTIC_WARNING(
            "Provided mass information not valid for a prim %s.",
            prim.GetPrimPath().GetString().c_str());

        return UsdPhysicsMassProperties();
    }

    GfMatrix3f inertia = massInfo.inertia;

    // if no density was set, use default based on units
    if (density <= 0.0f)
    {
        // default density 1000.0 kg / meters * meters * meters 
        float metersPerUnit = float(UsdGeomGetStageMetersPerUnit(prim.GetStage()));
        float kgPerUnit = float(UsdPhysicsGetStageKilogramsPerUnit(prim.GetStage()));
        density = 1000.0f * metersPerUnit * metersPerUnit * metersPerUnit / kgPerUnit;
    }

    GfVec3f centerOfMass(0.0f);
    GfQuatf principalAxes(1.0f);
    const bool hasCoM = _GetCoM(prim, &centerOfMass, xformCache);

    // we have a collider mass override
    if (shapeMassInfo.mass > 0.0f)
    {
        inertia = inertia * (shapeMassInfo.mass / massInfo.volume);
    }
    else if (massInfo.volume >= 0.0f)
    {
        // we dont have mass compute it based on the collision volume
        shapeMassInfo.mass = massInfo.volume * density;
        inertia = inertia * density;
    }

    // inertia was provided, update the inertia data
    if (shapeMassInfo.hasInertia)
    {        
        const GfMatrix3f rotMatr(principalAxes);
        GfMatrix3f inMatr(0.0f);
        inMatr[0][0] = shapeMassInfo.diagonalInertia[0];
        inMatr[1][1] = shapeMassInfo.diagonalInertia[1];
        inMatr[2][2] = shapeMassInfo.diagonalInertia[2];

        inertia = inMatr * rotMatr;
    }

    if (shapeMassInfo.hasPa)
    {
        inertia = UsdPhysicsMassProperties::RotateInertia(inertia, shapeMassInfo.principalAxes);
    }

    // center of mass provided, update the inertia
    if (hasCoM)
    {
        if (!shapeMassInfo.hasInertia)
        {
            // update inertia if we override the CoM but use the computed inertia
            UsdPhysicsMassProperties massProps(shapeMassInfo.mass, inertia, massInfo.centerOfMass);            
            massProps.Translate(centerOfMass - massProps.GetCenterOfMass());
            inertia = massProps.GetInertiaTensor();
        }
        massInfo.centerOfMass = centerOfMass;
    }

    // setup collision transformation
    transform->SetTranslate(massInfo.localPos);
    transform->SetRotateOnly(GfQuatd(massInfo.localRot));

    // return final collision mass properties
    return UsdPhysicsMassProperties(shapeMassInfo.mass, inertia, massInfo.centerOfMass);
}

// compute mass properties for given rigid body
float UsdPhysicsRigidBodyAPI::ComputeMassProperties(GfVec3f* _diagonalInertia, 
    GfVec3f* _com, GfQuatf* _principalAxes, const MassInformationFn& massInfoFn) const
{
    UsdPhysicsMassProperties massProps;

    const UsdPrim usdPrim = GetPrim();
    const UsdStagePtr stage = usdPrim.GetStage();

    UsdGeomXformCache xfCache;

    // Parse dynamic body mass data
    const _MassApiData rigidBodyMassInfo = _ParseMassApi(usdPrim);

    // If we dont have mass, we need to compute mass from collisions
    const bool accumulateMass = rigidBodyMassInfo.mass <= 0.0f;

    // Get initial data from parsed rigid body mass information
    GfVec3f outCenterOfMass(0.0f);
    const bool hasPa = rigidBodyMassInfo.hasPa;
    GfQuatf outPrincipalAxes = rigidBodyMassInfo.principalAxes;
    const bool hasCoM = _GetCoM(usdPrim, &outCenterOfMass, &xfCache);    
    float outMass = rigidBodyMassInfo.mass;
    GfVec3f outDiagonalizedInertiaTensor = rigidBodyMassInfo.diagonalInertia;

    // if we dont have enough mass information, we need to traverse collisions
    // to gather final mass information
    if (accumulateMass || !rigidBodyMassInfo.hasInertia || !hasCoM)
    {
        std::vector<UsdPhysicsMassProperties> massProps;
        std::vector<GfMatrix4f> massTransf;
        std::vector<UsdPrim> collisionPrims;        

        // traverse all collisions below this body and get their collision information
        // first gather all collisions
        UsdPrimRange range(usdPrim);
        for (auto it = range.begin(); it != range.end(); ++it)
        {
            UsdPrim collisionPrim = *it;
            if (collisionPrim && collisionPrim.HasAPI<UsdPhysicsCollisionAPI>())
            {
                collisionPrims.push_back(std::move(collisionPrim));
            }
        }

        // get materials for all prims
        std::vector<UsdShadeMaterial> physicsMaterials = 
            UsdShadeMaterialBindingAPI::ComputeBoundMaterials(collisionPrims, _physicsPurposeTokens->materialPurposePhysics);
        for (UsdShadeMaterial& material : physicsMaterials)
        {
            if (material && !material.GetPrim().HasAPI<UsdPhysicsMaterialAPI>())
            {
                material = UsdShadeMaterial();
            }
        }

        for (size_t i = 0; i < collisionPrims.size(); i++)
        {
            const UsdPrim& collisionPrim = collisionPrims[i];
            const UsdPhysicsCollisionAPI collisionAPI(collisionPrim);
            float shapeDensity = 0.0f;

            _MassApiData _MassApiData = 
                _GetCollisionShapeMassAPIData(collisionAPI, rigidBodyMassInfo.density, &shapeDensity, physicsMaterials[i]);

            GfMatrix4f matrix;
            massProps.push_back(_ParseCollisionShapeForMass(collisionPrim, _MassApiData, shapeDensity, &matrix, &xfCache, massInfoFn));
            massTransf.push_back(matrix);
        }

        if (!massProps.empty())
        {
            // compute accumulated mass properties from all gathered collisions
            UsdPhysicsMassProperties accumulatedMassProps =
                UsdPhysicsMassProperties::Sum(massProps.data(), massTransf.data(), uint32_t(massProps.size()));

            // if we had to compute mass, set the new mass
            if (accumulateMass)
            {
                outMass = accumulatedMassProps.GetMass();
            }
            else
            {
                // otherwise scale inertia based on the given body mass
                const float massDiff = outMass / accumulatedMassProps.GetMass();
                accumulatedMassProps.SetMass(outMass);
                accumulatedMassProps.SetInertiaTensor(accumulatedMassProps.GetInertiaTensor() * massDiff);
            }

            if (!hasCoM)
            {
                // get CoM from the accumulated props
                outCenterOfMass = accumulatedMassProps.GetCenterOfMass();
            }
            else
            {               
                // otherwise translae the mass props by given body CoM 
                accumulatedMassProps.Translate(outCenterOfMass - accumulatedMassProps.GetCenterOfMass());                
            }

            GfQuatf accPa;
            const GfVec3f accInertia = UsdPhysicsMassProperties::GetMassSpaceInertia(accumulatedMassProps.GetInertiaTensor(), accPa);

            // check for inertia override
            if (!rigidBodyMassInfo.hasInertia)
            {
                // if no inertia was given to the rigid body get the accumulated inertia
                outDiagonalizedInertiaTensor = accInertia;
            }

            if (!hasPa)
            {
                // if no prinicipal axes were given to the rigid body get the accumulated one
                outPrincipalAxes = accPa;
            }
        }
        else
        {
            // no shape provided check inertia
            if (!rigidBodyMassInfo.hasInertia)
            {
                // In the absence of collision shapes and a specified inertia tensor, approximate
                // the tensor using a sphere. If the mass is not specified
                // throw a warning instead. Equation for spherical intertial tensor is (2/5 or
                // 0.4)*mass*radius^2, where we use 0.1 radius to imitate point.

                if (outMass > 0.0f)
                {
                    const float metersPerUnit = float(UsdGeomGetStageMetersPerUnit(stage));
                    const float radius = 0.1f / metersPerUnit;
                    const float inertiaVal = 0.4f * outMass * radius * radius;
                    outDiagonalizedInertiaTensor[0] = inertiaVal;
                    outDiagonalizedInertiaTensor[1] = inertiaVal;
                    outDiagonalizedInertiaTensor[2] = inertiaVal;
                    TF_DIAGNOSTIC_WARNING(
                        "The rigid body at %s has a possibly invalid inertia tensor of {1.0, 1.0, 1.0}, small sphere approximated inertia was used. %s %s",
                        usdPrim.GetPrimPath().GetString().c_str(),
                        "Either specify correct values in the mass properties, or add collider(s) to any shape(s) that you wish to automatically compute mass properties for.",
                        "If you do not want the objects to collide, add colliders regardless then disable the 'enable collision' property.");
                }
                else
                {
                    TF_DIAGNOSTIC_WARNING(
                        "The rigid body at %s has a possibly invalid inertia tensor of {1.0, 1.0, 1.0}%s. %s %s",
                        usdPrim.GetPrimPath().GetString().c_str(), (outMass < 0.0f) ? " and a negative mass" : "",
                        "Either specify correct values in the mass properties, or add collider(s) to any shape(s) that you wish to automatically compute mass properties for.",
                        "If you do not want the objects to collide, add colliders regardless then disable the 'enable collision' property.");
                }
            }
        }
    }

    if (_com)
    {
        *_com = outCenterOfMass;
    }

    if (_diagonalInertia)
    {
        *_diagonalInertia = outDiagonalizedInertiaTensor;
    }

    if (_principalAxes)
    {
        *_principalAxes = outPrincipalAxes;
    }
    return outMass;
}

PXR_NAMESPACE_CLOSE_SCOPE
