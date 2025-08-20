//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinObjectComputations.h"

#include "pxr/exec/exec/metadataInputNode.h"
#include "pxr/exec/exec/privateBuiltinComputations.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/esf/object.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// computeMetadata
//

Exec_ComputeMetadataComputationDefinition::
Exec_ComputeMetadataComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::GetUnknownType(),
        Exec_PrivateBuiltinComputations->computeMetadata)
{
}

Exec_ComputeMetadataComputationDefinition::
~Exec_ComputeMetadataComputationDefinition() = default;

TfType
Exec_ComputeMetadataComputationDefinition::GetResultType(
    const EsfObjectInterface &providerObject,
    const TfToken &metadataKey,
    EsfJournal *const journal) const
{
    return providerObject.GetMetadataValueType(metadataKey);
}

TfType
Exec_ComputeMetadataComputationDefinition::GetExtractionType(
    const EsfObjectInterface &providerObject) const
{
    TF_VERIFY(false, "Extracting metdata values directly is not supported.");
    return TfType();
}

Exec_InputKeyVectorConstRefPtr
Exec_ComputeMetadataComputationDefinition::GetInputKeys(
    const EsfObjectInterface &providerObject,
    EsfJournal *) const
{
    return Exec_InputKeyVector::GetEmptyVector();
}

VdfNode *
Exec_ComputeMetadataComputationDefinition::CompileNode(
    const EsfObjectInterface &providerObject,
    const TfToken &metadataKey,
    EsfJournal *const nodeJournal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(nodeJournal, "Null nodeJournal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }
    if (!providerObject.IsValidMetadataKey(metadataKey)) {
        TF_CODING_ERROR(
            "Skipping compilation of input node for invalid metadata key '%s'",
            metadataKey.GetText());
        return nullptr;
    }

    return program->CreateNode<Exec_MetadataInputNode>(
        *nodeJournal,
        providerObject.AsObject(),
        metadataKey,
        GetResultType(providerObject, metadataKey, nodeJournal));
}

PXR_NAMESPACE_CLOSE_SCOPE
