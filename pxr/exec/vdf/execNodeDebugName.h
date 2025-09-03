//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXEC_NODE_DEBUG_NAME_H
#define PXR_EXEC_VDF_EXEC_NODE_DEBUG_NAME_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <typeinfo>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Stores all necessary information to lazily construct a node debug name.
///
class Vdf_ExecNodeDebugName
{
public:

    Vdf_ExecNodeDebugName(
        const VdfNode &node,
        VdfNodeDebugNameCallback &&callback) 
    : _node(&node)
    , _callback(std::move(callback))
    {
        TF_VERIFY(
            _callback,
            "Null callback for node: %s",
            ArchGetDemangled(typeid(node)).c_str());
    }

private:
    friend class VdfNetwork;

    // Helper to initialize debug name token.
    //
    void _InitializeDebugName() const 
    {
        // The possibility of a null callback is handled silently here since
        // the constructor shows an error.
        _debugName = (_callback) 
            ? TfToken(ArchGetDemangled(typeid(*_node)) + " " + _callback())
            : TfToken(ArchGetDemangled(typeid(*_node)));
    }

    // Returns a debug name for the node. Only VdfNetwork should call this 
    // function.
    //
    const std::string _GetDebugName() const
    {
        if (_debugName.IsEmpty()) {
            _InitializeDebugName();
        }
        return _debugName.GetString();
    }

private:
    // Node that this debug name describes.
    const VdfNode *_node;

    // Callback to construct node debug name.
    const VdfNodeDebugNameCallback _callback;

    // Stored debug name. This is computed on-demand.
    mutable TfToken _debugName;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
