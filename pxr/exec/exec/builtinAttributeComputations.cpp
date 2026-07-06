//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinAttributeComputations.h"

#include "pxr/exec/exec/attributeInputNode.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/callbackNode.h"
#include "pxr/exec/exec/privateBuiltinComputations.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/providerResolution.h"

#include "pxr/base/tf/type.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/tokens.h"
#include "pxr/usd/sdf/valueTypeName.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// computeResolvedValue
//

Exec_ComputeResolvedValueComputationDefinition
::Exec_ComputeResolvedValueComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::GetUnknownType(),
        ExecBuiltinComputations->computeResolvedValue)
    , _inputKeys(_MakeInputKeys())
{
}

Exec_ComputeResolvedValueComputationDefinition
::~Exec_ComputeResolvedValueComputationDefinition()
    = default;

TfType
Exec_ComputeResolvedValueComputationDefinition::GetResultType(
    const EsfObjectInterface &providerObject,
    const TfToken &,
    EsfJournal *const journal) const
{
    if (!TF_VERIFY(providerObject.IsAttribute())) {
        return {};
    }
    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const SdfValueTypeName valueTypeName =
        providerAttribute->GetValueTypeName(journal);

    return valueTypeName.GetScalarType().GetType();
}

TfType
Exec_ComputeResolvedValueComputationDefinition::GetExtractionType(
    const EsfObjectInterface &providerObject) const
{
    if (!TF_VERIFY(providerObject.IsAttribute())) {
        return {};
    }
    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const SdfValueTypeName valueTypeName =
        providerAttribute->GetValueTypeName(/* journal */ nullptr);

    return valueTypeName.GetType();
}

Exec_InputKeyVectorConstRefPtr
Exec_ComputeResolvedValueComputationDefinition::GetInputKeys(
    const EsfObjectInterface &,
    EsfJournal *) const
{
    return _inputKeys;
}

VdfNode *
Exec_ComputeResolvedValueComputationDefinition::CompileNode(
    const EsfObjectInterface &providerObject,
    const TfToken &,
    EsfJournal *const nodeJournal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(nodeJournal, "Null nodeJournal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }
    if (!TF_VERIFY(providerObject.IsAttribute(), "Non-attribute provider")) {
        return nullptr;
    }

    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const SdfValueTypeName valueTypeName =
        providerAttribute->GetValueTypeName(nodeJournal);

    return program->CreateNode<Exec_AttributeInputNode>(
        *nodeJournal,
        providerAttribute->GetQuery(),
        valueTypeName.GetScalarType().GetType());
}

Exec_InputKeyVectorConstRefPtr
Exec_ComputeResolvedValueComputationDefinition::_MakeInputKeys()
{
    return Exec_InputKeyVector::MakeConstShared({{
                Exec_AttributeInputNodeTokens->time,
                ExecBuiltinComputations->computeTime,
                /* disambiguatingId */ TfToken(),
                TfType::Find<EfTime>(),
                ExecProviderResolution{
                    SdfPath::AbsoluteRootPath(),
                    ExecProviderResolution::DynamicTraversal::Local
                },
                /* optional */ false
        }});
}

//
// computeConnectedValue
//

Exec_ComputeConnectedValueComputationDefinition
::Exec_ComputeConnectedValueComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::GetUnknownType(),
        Exec_PrivateBuiltinComputations->computeConnectedValue)
{
}

Exec_ComputeConnectedValueComputationDefinition
::~Exec_ComputeConnectedValueComputationDefinition()
    = default;

TfType
Exec_ComputeConnectedValueComputationDefinition::GetResultType(
    const EsfObjectInterface &providerObject,
    const TfToken &,
    EsfJournal *const journal) const
{
    if (!TF_VERIFY(providerObject.IsAttribute())) {
        return {};
    }
    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const SdfValueTypeName valueTypeName =
        providerAttribute->GetValueTypeName(journal);

    return valueTypeName.GetScalarType().GetType();
}

TfType
Exec_ComputeConnectedValueComputationDefinition::GetExtractionType(
    const EsfObjectInterface &providerObject) const
{
    if (!TF_VERIFY(providerObject.IsAttribute())) {
        return {};
    }
    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const SdfValueTypeName valueTypeName =
        providerAttribute->GetValueTypeName(/* journal */ nullptr);

    return valueTypeName.GetType();
}

Exec_InputKeyVectorConstRefPtr
Exec_ComputeConnectedValueComputationDefinition::GetInputKeys(
    const EsfObjectInterface &providerObject,
    EsfJournal *const journal) const
{
    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    
    // Built-in attribute data flow is currently only supported for attribute 
    // providers that own a single connection.  
    if (!TF_VERIFY(
        providerObject.IsAttribute() 
        && providerAttribute->GetConnections(journal).size() == 1)) { 

        return Exec_InputKeyVector::GetEmptyVector();
    }

    return Exec_InputKeyVector::MakeConstShared({{
            VdfTokens->in,
            ExecBuiltinComputations->computeValue,
            /* disambiguatingId */ TfToken(),
            providerAttribute->GetValueTypeName(journal)
                .GetScalarType().GetType(),
            ExecProviderResolution{
                SdfPath::ReflexiveRelativePath(),
                ExecProviderResolution::DynamicTraversal::
                    ConnectionTargetedObjects
            },
            /* optional */ false 
        }});
}

VdfNode *
Exec_ComputeConnectedValueComputationDefinition::CompileNode(
    const EsfObjectInterface &providerObject,
    const TfToken &,
    EsfJournal *const journal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(journal, "Null journal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }
    if (!TF_VERIFY(providerObject.IsAttribute(), "Non-attribute provider")) {
        return nullptr;
    }

    const EsfAttribute providerAttribute = providerObject.AsAttribute();
    const TfType attrType = 
        providerAttribute->GetValueTypeName(journal).GetScalarType().GetType();

    // Default attribute data flow is currently only supported for providers 
    // that own a single valid connection, which allows the callback to 
    // directly set the output to reference the single input. Note that this 
    // will throw a coding error if a single input cannot be resolved and needs  
    // to be revised when multiple connections are supported. 
    return program->CreateNode<Exec_CallbackNode>(
        *journal,
        VdfInputSpecs().ReadConnector(attrType, VdfTokens->in), 
        VdfOutputSpecs().Connector(attrType, VdfTokens->out),
        [](const VdfContext &ctx) {
            if (!ctx.HasInputValue(VdfTokens->in)) {
                ctx.SetEmptyOutput();
                return;
            } 
            ctx.SetOutputToReferenceInput(VdfTokens->in);
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
