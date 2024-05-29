//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_REGISTRY_H
#define PXR_USD_SDR_REGISTRY_H

/// \file sdr/registry.h

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdrRegistry
///
/// The shading-specialized version of `NdrRegistry`.
///
class SdrRegistry : public NdrRegistry
{
public:
    /// Get the single `SdrRegistry` instance.
    SDR_API
    static SdrRegistry& GetInstance();

    /// Exactly like `NdrRegistry::GetNodeByIdentifier()`, but returns a
    /// `SdrShaderNode` pointer instead of a `NdrNode` pointer.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByIdentifier(
        const NdrIdentifier& identifier,
        const NdrTokenVec& typePriority = NdrTokenVec());

    /// Exactly like `NdrRegistry::GetNodeByIdentifierAndType()`, but returns
    /// a `SdrShaderNode` pointer instead of a `NdrNode` pointer.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByIdentifierAndType(
        const NdrIdentifier& identifier,
        const TfToken& nodeType);

    /// Exactly like `NdrRegistry::GetNodeByName()`, but returns a
    /// `SdrShaderNode` pointer instead of a `NdrNode` pointer.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByName(
        const std::string& name,
        const NdrTokenVec& typePriority = NdrTokenVec(),
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Exactly like `NdrRegistry::GetNodeByNameAndType()`, but returns a
    /// `SdrShaderNode` pointer instead of a `NdrNode` pointer.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByNameAndType(
        const std::string& name,
        const TfToken& nodeType,
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Wrapper method for NdrRegistry::GetNodeFromAsset(). 
    /// Returns a valid SdrShaderNode pointer upon success.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeFromAsset(
        const SdfAssetPath &shaderAsset,
        const NdrTokenMap &metadata=NdrTokenMap(),
        const TfToken &subIdentifier=TfToken(),
        const TfToken &sourceType=TfToken());

    /// Wrapper method for NdrRegistry::GetNodeFromSourceCode(). 
    /// Returns a valid SdrShaderNode pointer upon success.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeFromSourceCode(
        const std::string &sourceCode,
        const TfToken &sourceType,
        const NdrTokenMap &metadata=NdrTokenMap());

    /// Exactly like `NdrRegistry::GetNodesByIdentifier()`, but returns a vector
    /// of `SdrShaderNode` pointers instead of a vector of `NdrNode` pointers.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByIdentifier(const NdrIdentifier& identifier);

    /// Exactly like `NdrRegistry::GetNodesByName()`, but returns a vector of
    /// `SdrShaderNode` pointers instead of a vector of `NdrNode` pointers.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByName(
        const std::string& name,
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Exactly like `NdrRegistry::GetNodesByFamily()`, but returns a vector of
    /// `SdrShaderNode` pointers instead of a vector of `NdrNode` pointers.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByFamily(
        const TfToken& family = TfToken(),
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

protected:
    // Allow TF to construct the class
    friend class TfSingleton<SdrRegistry>;

    SdrRegistry();
    ~SdrRegistry();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_REGISTRY_H
