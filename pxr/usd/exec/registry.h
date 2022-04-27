//
// Unlicensed 2022 benmalartre
//

#ifndef EXEC_REGISTRY_H
#define EXEC_REGISTRY_H

/// \file exec/registry.h

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/usd/exec/api.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/exec/declare.h"
#include "pxr/usd/exec/execNode.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class ExecRegistry
///
/// The execution-specialized version of `NdrRegistry`.
///
class ExecRegistry : public NdrRegistry
{
public:
    /// Get the single `ExecRegistry` instance.
    EXEC_API
    static ExecRegistry& GetInstance();

    /// Exactly like `NdrRegistry::GetNodeByIdentifier()`, but returns a
    /// `ExecNode` pointer instead of a `NdrNode` pointer.
    EXEC_API
    ExecNodeConstPtr GetExecNodeByIdentifier(
        const NdrIdentifier& identifier,
        const NdrTokenVec& typePriority = NdrTokenVec());

    /// Exactly like `NdrRegistry::GetNodeByIdentifierAndType()`, but returns
    /// a `ExecNode` pointer instead of a `NdrNode` pointer.
    EXEC_API
    ExecNodeConstPtr GetExecNodeByIdentifierAndType(
        const NdrIdentifier& identifier,
        const TfToken& nodeType);

    /// Exactly like `NdrRegistry::GetNodeByName()`, but returns a
    /// `ExecNode` pointer instead of a `NdrNode` pointer.
    EXEC_API
    ExecNodeConstPtr GetExecNodeByName(
        const std::string& name,
        const NdrTokenVec& typePriority = NdrTokenVec(),
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Exactly like `NdrRegistry::GetNodeByNameAndType()`, but returns a
    /// `ExecNode` pointer instead of a `NdrNode` pointer.
    EXEC_API
    ExecNodeConstPtr GetExecNodeByNameAndType(
        const std::string& name,
        const TfToken& nodeType,
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Wrapper method for NdrRegistry::GetNodeFromAsset(). 
    /// Returns a valid ExecNode pointer upon success.
    EXEC_API
    ExecNodeConstPtr GetExecNodeFromAsset(
        const SdfAssetPath &execAsset,
        const NdrTokenMap &metadata=NdrTokenMap(),
        const TfToken &subIdentifier=TfToken(),
        const TfToken &sourceType=TfToken());

    /// Wrapper method for NdrRegistry::GetNodeFromSourceCode(). 
    /// Returns a valid ExecNode pointer upon success.
    EXEC_API
    ExecNodeConstPtr GetExecNodeFromSourceCode(
        const std::string &sourceCode,
        const TfToken &sourceType,
        const NdrTokenMap &metadata=NdrTokenMap());

    /// Exactly like `NdrRegistry::GetNodesByIdentifier()`, but returns a vector
    /// of `ExecNode` pointers instead of a vector of `NdrNode` pointers.
    EXEC_API
    ExecNodePtrVec GetExecNodesByIdentifier(const NdrIdentifier& identifier);

    /// Exactly like `NdrRegistry::GetNodesByName()`, but returns a vector of
    /// `ExecNode` pointers instead of a vector of `NdrNode` pointers.
    EXEC_API
    ExecNodePtrVec GetExecNodesByName(
        const std::string& name,
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// Exactly like `NdrRegistry::GetNodesByFamily()`, but returns a vector of
    /// `ExecNode` pointers instead of a vector of `NdrNode` pointers.
    EXEC_API
    ExecNodePtrVec GetExecNodesByFamily(
        const TfToken& family = TfToken(),
        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

protected:
    // Allow TF to construct the class
    friend class TfSingleton<ExecRegistry>;

    ExecRegistry();
    ~ExecRegistry();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXEC_REGISTRY_H
