//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_LOBEPRUNER_H
#define PXR_IMAGING_HD_ST_MATERIALX_LOBEPRUNER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/tokens.h"
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderNodeImpl.h>

PXR_NAMESPACE_OPEN_SCOPE

// Initializes the LobePruner with a library
void HdSt_InitializeLobePruner(const MaterialX::DocumentPtr& library);

// Fetch the LobePruner library containing optimized NodeGraphs
const MaterialX::DocumentPtr& HdSt_GetLobePrunerLibrary();

// Checks if a node is optimizable and if this is the case, create the optimized
// NodeDef and NodeGraph in the library and return the optimized node id. An optimized node id
// will be built from the name of the original category followed by a series of characters
// describing which attibutes were optimized:
//   - 'x' that attribute was not optimized (intermediate value or connected)
//   - '0' a zero value was optimized
//   - '1' a one value was optimized
TfToken HdSt_GetLobePrunedNodeId(const HdMaterialNode2& node);

// Returns the implementation name of an optimized dark PBR node used to replace any base PBR
// node that has a weight of zero. It is the responsibility of the shadergen code to provide a
// working implementation.
TfToken HdSt_GetDarkBaseImplementationName();

// Returns the implementation name of an optimized dark PBR node used to replace any base PBR
// node that has a weight of zero. It is the responsibility of the shadergen code to provide a
// working implementation.
TfToken HdSt_GetDarkLayerImplementationName();

/// Closure node that implements a no-op PBR shading node (with weight zero).
class HdStDarkClosureNode : public MaterialX::ShaderNodeImpl
{
public:
    static MaterialX::ShaderNodeImplPtr create();

    void initialize(const MaterialX::InterfaceElement& element, MaterialX::GenContext& context) override;

    void emitFunctionCall(const MaterialX::ShaderNode& node, MaterialX::GenContext& context, MaterialX::ShaderStage& stage)
        const override;

private:
    bool _isBaseNode = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
