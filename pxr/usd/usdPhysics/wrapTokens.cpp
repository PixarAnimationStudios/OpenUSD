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
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdPhysics/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdPhysicsWrapTokens {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdPhysicsTokens()
{
    boost::python::class_<UsdPhysicsTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "acceleration", UsdPhysicsTokens->acceleration);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "angular", UsdPhysicsTokens->angular);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "boundingCube", UsdPhysicsTokens->boundingCube);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "boundingSphere", UsdPhysicsTokens->boundingSphere);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "colliders", UsdPhysicsTokens->colliders);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "convexDecomposition", UsdPhysicsTokens->convexDecomposition);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "convexHull", UsdPhysicsTokens->convexHull);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "distance", UsdPhysicsTokens->distance);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive", UsdPhysicsTokens->drive);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsDamping", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsDamping);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsMaxForce", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsMaxForce);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsStiffness", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsStiffness);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsTargetPosition", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsTargetPosition);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsTargetVelocity", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsTargetVelocity);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "drive_MultipleApplyTemplate_PhysicsType", UsdPhysicsTokens->drive_MultipleApplyTemplate_PhysicsType);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "force", UsdPhysicsTokens->force);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "kilogramsPerUnit", UsdPhysicsTokens->kilogramsPerUnit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "limit", UsdPhysicsTokens->limit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "limit_MultipleApplyTemplate_PhysicsHigh", UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsHigh);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "limit_MultipleApplyTemplate_PhysicsLow", UsdPhysicsTokens->limit_MultipleApplyTemplate_PhysicsLow);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "linear", UsdPhysicsTokens->linear);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "meshSimplification", UsdPhysicsTokens->meshSimplification);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "none", UsdPhysicsTokens->none);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsAngularVelocity", UsdPhysicsTokens->physicsAngularVelocity);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsApproximation", UsdPhysicsTokens->physicsApproximation);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsAxis", UsdPhysicsTokens->physicsAxis);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsBody0", UsdPhysicsTokens->physicsBody0);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsBody1", UsdPhysicsTokens->physicsBody1);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsBreakForce", UsdPhysicsTokens->physicsBreakForce);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsBreakTorque", UsdPhysicsTokens->physicsBreakTorque);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsCenterOfMass", UsdPhysicsTokens->physicsCenterOfMass);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsCollisionEnabled", UsdPhysicsTokens->physicsCollisionEnabled);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsConeAngle0Limit", UsdPhysicsTokens->physicsConeAngle0Limit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsConeAngle1Limit", UsdPhysicsTokens->physicsConeAngle1Limit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsDensity", UsdPhysicsTokens->physicsDensity);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsDiagonalInertia", UsdPhysicsTokens->physicsDiagonalInertia);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsDynamicFriction", UsdPhysicsTokens->physicsDynamicFriction);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsExcludeFromArticulation", UsdPhysicsTokens->physicsExcludeFromArticulation);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsFilteredGroups", UsdPhysicsTokens->physicsFilteredGroups);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsFilteredPairs", UsdPhysicsTokens->physicsFilteredPairs);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsGravityDirection", UsdPhysicsTokens->physicsGravityDirection);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsGravityMagnitude", UsdPhysicsTokens->physicsGravityMagnitude);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsJointEnabled", UsdPhysicsTokens->physicsJointEnabled);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsKinematicEnabled", UsdPhysicsTokens->physicsKinematicEnabled);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsLocalPos0", UsdPhysicsTokens->physicsLocalPos0);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsLocalPos1", UsdPhysicsTokens->physicsLocalPos1);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsLocalRot0", UsdPhysicsTokens->physicsLocalRot0);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsLocalRot1", UsdPhysicsTokens->physicsLocalRot1);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsLowerLimit", UsdPhysicsTokens->physicsLowerLimit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsMass", UsdPhysicsTokens->physicsMass);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsMaxDistance", UsdPhysicsTokens->physicsMaxDistance);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsMinDistance", UsdPhysicsTokens->physicsMinDistance);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsPrincipalAxes", UsdPhysicsTokens->physicsPrincipalAxes);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsRestitution", UsdPhysicsTokens->physicsRestitution);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsRigidBodyEnabled", UsdPhysicsTokens->physicsRigidBodyEnabled);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsSimulationOwner", UsdPhysicsTokens->physicsSimulationOwner);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsStartsAsleep", UsdPhysicsTokens->physicsStartsAsleep);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsStaticFriction", UsdPhysicsTokens->physicsStaticFriction);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsUpperLimit", UsdPhysicsTokens->physicsUpperLimit);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "physicsVelocity", UsdPhysicsTokens->physicsVelocity);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "rotX", UsdPhysicsTokens->rotX);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "rotY", UsdPhysicsTokens->rotY);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "rotZ", UsdPhysicsTokens->rotZ);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "transX", UsdPhysicsTokens->transX);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "transY", UsdPhysicsTokens->transY);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "transZ", UsdPhysicsTokens->transZ);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "x", UsdPhysicsTokens->x);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "y", UsdPhysicsTokens->y);
    pxrUsdUsdPhysicsWrapTokens::_AddToken(cls, "z", UsdPhysicsTokens->z);
}
