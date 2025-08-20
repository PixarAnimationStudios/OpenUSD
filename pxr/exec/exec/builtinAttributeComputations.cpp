//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinAttributeComputations.h"

#include "pxr/exec/exec/attributeInputNode.h"
#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/providerResolution.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include <initializer_list>

PXR_NAMESPACE_OPEN_SCOPE

//
// computeValue
//

Exec_ComputeValueComputationDefinition::Exec_ComputeValueComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::GetUnknownType(),
        ExecBuiltinComputations->computeValue)
    , _inputKeys(_MakeInputKeys())
{
}

Exec_ComputeValueComputationDefinition::~Exec_ComputeValueComputationDefinition()
    = default;

TfType
Exec_ComputeValueComputationDefinition::GetResultType(
    const EsfObjectInterface &providerObject,
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
Exec_ComputeValueComputationDefinition::GetExtractionType(
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
Exec_ComputeValueComputationDefinition::GetInputKeys(
    const EsfObjectInterface &providerObject,
    EsfJournal *) const
{
    return _inputKeys;
}

VdfNode *
Exec_ComputeValueComputationDefinition::CompileNode(
    const EsfObjectInterface &providerObject,
    EsfJournal *const nodeJournal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(nodeJournal, "Null nodeJournal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }
    if (!TF_VERIFY(providerObject.IsAttribute())) {
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
Exec_ComputeValueComputationDefinition::_MakeInputKeys()
{
    const Exec_InputKeyVectorRefPtr inputKeys =
        Exec_InputKeyVector::MakeShared(std::initializer_list<Exec_InputKey>{{
                Exec_AttributeInputNodeTokens->time,
                ExecBuiltinComputations->computeTime,
                TfType::Find<EfTime>(),
                ExecProviderResolution{
                    SdfPath::AbsoluteRootPath(),
                    ExecProviderResolution::DynamicTraversal::Local
                },
                /* optional */ false
        }});

    return Exec_InputKeyVectorConstRefPtr(inputKeys);
}

PXR_NAMESPACE_CLOSE_SCOPE
