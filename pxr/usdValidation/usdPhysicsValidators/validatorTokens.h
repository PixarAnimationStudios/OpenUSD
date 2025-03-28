//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_GEOM_VALIDATORS_TOKENS_H
#define PXR_USD_VALIDATION_USD_GEOM_VALIDATORS_TOKENS_H

/// \file

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usdValidation/usdPhysicsValidators/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USD_PHYSICS_VALIDATOR_NAME_TOKENS                               \
    ((rigidBodyChecker, "usdPhysicsValidators:RigidBodyChecker"))       \
    ((colliderChecker, "usdPhysicsValidators:ColliderChecker"))         \
    ((physicsJointChecker, "usdPhysicsValidators:PhysicsJointChecker")) \
    ((articulationChecker, "usdPhysicsValidators:ArticulationChecker"))

#define USD_PHYSICS_VALIDATOR_KEYWORD_TOKENS  (UsdPhysicsValidators) 

#define USD_PHYSICS_VALIDATION_ERROR_NAME_TOKENS                        \
    ((nestedRigidBody, "NestedRigidBody"))                              \
    ((nestedArticulation, "NestedArticulation"))                        \
    ((articulationOnStaticBody, "ArticulationOnStaticBody"))            \
    ((articulationOnKinematicBody, "ArticulationOnKinematicBody"))      \
    ((rigidBodyOrientationScale,  "RigidBodyOrientationScale"))         \
    ((rigidBodyNonXformable,  "RigidBodyNonXformable"))                 \
    ((rigidBodyNonInstanceable,  "RigidBodyNonInstanceable"))           \
    ((jointInvalidPrimRel,  "JointInvalidPrimRel"))                     \
    ((jointMultiplePrimsRel,  "JointMultiplePrimsRel"))                 \
    ((colliderNonUniformScale, "ColliderNonUniformScale"))              \
    ((colliderSpherePointsDataMissing, "ColliderSpherePointsDataMissing"))

/// \def USD_PHYSICS_VALIDATOR_NAME_TOKENS
/// Tokens representing validator names. Note that for plugin provided
/// validators, the names must be prefixed by usdPhysics:, which is the name of
/// the usdPhysics plugin.
TF_DECLARE_PUBLIC_TOKENS(UsdPhysicsValidatorNameTokens, USDPHYSICSVALIDATORS_API,
                             USD_PHYSICS_VALIDATOR_NAME_TOKENS);

/// \def USD_PHYSICS_VALIDATOR_KEYWORD_TOKENS
/// Tokens representing keywords associated with any validator in the usdPhysics
/// plugin. Clients can use this to inspect validators contained within a
/// specific keywords, or use these to be added as keywords to any new
/// validator.
TF_DECLARE_PUBLIC_TOKENS(UsdPhysicsValidatorKeywordTokens, USDPHYSICSVALIDATORS_API,
                             USD_PHYSICS_VALIDATOR_KEYWORD_TOKENS);

/// \def USD_PHYSICS_VALIDATION_ERROR_NAME_TOKENS
/// Tokens representing validation error identifier.
TF_DECLARE_PUBLIC_TOKENS(UsdPhysicsValidationErrorNameTokens, USDPHYSICSVALIDATORS_API, 
                         USD_PHYSICS_VALIDATION_ERROR_NAME_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
