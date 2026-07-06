//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_COMPUTATIONS_H
#define PXR_EXEC_EXEC_IR_COMPUTATIONS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execIr/api.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Defines computation names provided by all ExecIr schemas.
///
/// These computations may be used in authoring workflows for invertible rigs.
///
/// > [!note]
/// > The computations defined here will be moved into ExecBuiltinComputations
/// > when OpenExec supports native inversion.
///
struct ExecIrComputationsType
{
    EXECIR_API ExecIrComputationsType();

    /// Computes the desired value of an invertible input attribute registered
    /// by ExecIrControllerBuilder::InvertibleInputAttribute.
    ///
    /// **computeDesiredValue** is the opposite of
    /// [computeValue](#Exec_BuiltinComputationTokens::computeValue). As opposed
    /// to computeValue, which produces an attribute's computed value from
    /// upstream authored values, computeDesiredValue produces a value which
    /// must be authored in order for to acheive desired downstream computed
    /// values.
    ///
    /// This computation is designed to be used with
    /// ExecUsdSystem::ComputeWithOverrides. The request contains one or more
    /// value keys for computeDesiredValue, and the desired downstream computed
    /// values are specified as overrides for
    /// [explicitDesiredValue](#ExecIrComputationsType::explicitDesiredValue) on
    /// the downstream attributes.
    ///
    const TfToken computeDesiredValue;

    /// Produces a desired value for an attribute.
    ///
    /// explicitDesiredValue is defined for all attributes in ExecIr. It has
    /// no dependencies on other computations and always computes an empty
    /// value, unless explicitly overridden during an invocation of
    /// ExecUsdSystem::ComputeWithOverrides.
    ///
    const TfToken explicitDesiredValue;
};

EXECIR_API
extern TfStaticData<ExecIrComputationsType> ExecIrComputations;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
