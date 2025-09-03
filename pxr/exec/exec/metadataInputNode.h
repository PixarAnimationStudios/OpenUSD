//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_METADATA_INPUT_NODE_H
#define PXR_EXEC_EXEC_METADATA_INPUT_NODE_H

#include "pxr/pxr.h"

#include "pxr/exec/esf/object.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A node that computes the value of scene object metadata.
class Exec_MetadataInputNode final : public VdfNode
{
public:
    /// Create a node that provides the value of the metadata indicated by
    /// \p metadataKey for \p object.
    ///
    Exec_MetadataInputNode(
        VdfNetwork *network,
        EsfObject &&object,
        const TfToken &metadataKey,
        TfType valueType);

    ~Exec_MetadataInputNode() override;

    // VdfNode override
    void Compute(VdfContext const& ctx) const override;

    /// Returns the scene path to the object that the metadata value is sourced
    /// from.
    /// 
    SdfPath GetObjectPath() const {
        return _object->GetPath(/* journal */ nullptr);
    }

    /// Returns the key for the metadata field whose value is provided by this
    /// node.
    /// 
    const TfToken &GetMetadataKey() const {
        return _metadataKey;
    }

private:
    // TODO: Once we stop treating namespace edits as resyncs, we will need to
    // update _object in response to edits like rename and reparent.
    EsfObject _object;
    TfToken _metadataKey;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
