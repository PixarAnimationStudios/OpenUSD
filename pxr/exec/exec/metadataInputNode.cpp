//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/metadataInputNode.h"

#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/exec/vdf/connectorSpecs.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/tokens.h"

#include "pxr/base/vt/value.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

Exec_MetadataInputNode::Exec_MetadataInputNode(
    VdfNetwork *const network,
    EsfObject &&object,
    const TfToken &metadataKey,
    const TfType valueType)
    : VdfNode(
        network,
        VdfInputSpecs(),
        VdfOutputSpecs().Connector(
            valueType,
            TfToken(VdfTokens->out)))
    , _object(std::move(object))
    , _metadataKey(metadataKey)
{
}

Exec_MetadataInputNode::~Exec_MetadataInputNode() = default;

void
Exec_MetadataInputNode::Compute(VdfContext const &context) const
{
    const VtValue value =
        _object->GetMetadata(_metadataKey);

    VdfRawValueAccessor(context).SetOutputVector(
        *GetOutput(VdfTokens->out),
        VdfMask::AllOnes(1),
        ExecTypeRegistry::GetInstance().CreateVector(value));
}

PXR_NAMESPACE_CLOSE_SCOPE
