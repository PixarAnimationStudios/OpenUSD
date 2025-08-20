//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_H
#define PXR_EXEC_EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/computationBuilders.h"
#include "pxr/exec/exec/types.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// Generates a schema computation registration function name.
//
/// \cond
#define _EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_NAME(SchemaType) Exec_RegisterSchema_##SchemaType
/// \endcond

/// \ingroup group_Exec_ComputationDefinitionLanguage
/// \hideinitializer
///
/// Initiates registration of exec computations for the schema \p SchemaType.
/// 
/// \param SchemaType
/// The schema type for which exec computations can be registered in the code
/// block that follows an invocation of this macro.
///
/// > **Note:**  
/// > For the full reference on the domain-specific language that is used to
/// > register exec computations refer to the [Computation Definition
/// > Language](#group_Exec_ComputationDefinitionLanguage) reference page.
///
#define EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA(SchemaType)                       \
                                                                                \
    PXR_NAMESPACE_OPEN_SCOPE                                                    \
                                                                                \
    /* Putting the registration fuction in the same namespace as the input   */ \
    /* classes/functions makes it so we can use unadorned names for the      */ \
    /* arguments to .Inputs(), etc.                                          */ \
    namespace exec_registration {                                               \
        /* Forward declaration of the static registration function.          */ \
        static void _EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_NAME(SchemaType) (                    \
            PXR_NS::Exec_ComputationBuilder &self);                             \
    }                                                                           \
                                                                                \
    /* The registry function calls the schema computation registration       */ \
    /* function.                                                             */ \
    TF_REGISTRY_FUNCTION(ExecDefinitionRegistryTag) {                           \
        Exec_ComputationBuilder self =                                          \
            Exec_ComputationBuilder::ConstructionAccess::Construct(             \
                TfType::FindByName(#SchemaType));                               \
        PXR_NS::exec_registration::_EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_NAME(SchemaType)(      \
            self);                                                              \
    }                                                                           \
                                                                                \
    PXR_NAMESPACE_CLOSE_SCOPE                                                   \
                                                                                \
    /* Define the schema computation registration function. The body of the  */ \
    /* function is provided by the client code that calls this macro.        */ \
    static void                                                                 \
    PXR_NS::exec_registration::_EXEC_REGISTER_COMPUTATIONS_FOR_SCHEMA_NAME(SchemaType) (         \
        PXR_NS::Exec_ComputationBuilder &self)

PXR_NAMESPACE_CLOSE_SCOPE

#endif
