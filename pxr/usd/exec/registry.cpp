//
// Unlicensed 2022 benmalartre
//

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/usd/exec/registry.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(ExecRegistry);


static ExecNodeConstPtr NdrNodeToExecNode(NdrNodeConstPtr node)
{
    return dynamic_cast<ExecNodeConstPtr>(node);
}

static ExecNodePtrVec NdrNodeVecToExecNodeVec(NdrNodeConstPtrVec nodeVec)
{
    ExecNodePtrVec execNodes;

    std::transform(nodeVec.begin(), nodeVec.end(),
        std::back_inserter(execNodes),
        [](NdrNodeConstPtr baseNode) {
            return ExecNodeConstPtr(baseNode);
        }
    );

    return execNodes;
}

ExecRegistry::ExecRegistry()
: NdrRegistry()
{
    // Track plugin discovery cost of base class
    TRACE_FUNCTION();
}

ExecRegistry::~ExecRegistry()
{
}

ExecRegistry&
ExecRegistry::GetInstance()
{
    return TfSingleton<ExecRegistry>::GetInstance();
}

ExecNodeConstPtr
ExecRegistry::GetExecNodeByIdentifier(
    const NdrIdentifier& identifier, const NdrTokenVec& typePriority)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(
        GetInstance().GetNodeByIdentifier(identifier, typePriority)
    );
}

ExecNodeConstPtr
ExecRegistry::GetExecNodeByIdentifierAndType(
    const NdrIdentifier& identifier, const TfToken& nodeType)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(
        GetInstance().GetNodeByIdentifierAndType(identifier, nodeType)
    );
}

ExecNodeConstPtr 
ExecRegistry::GetExecNodeFromAsset(
    const SdfAssetPath &execAsset,
    const NdrTokenMap &metadata,
    const TfToken &subIdentifier,
    const TfToken &sourceType)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(
        GetInstance().GetNodeFromAsset(execAsset,
                                       metadata,
                                       subIdentifier,
                                       sourceType));
}

ExecNodeConstPtr 
ExecRegistry::GetExecNodeFromSourceCode(
    const std::string &sourceCode,
    const TfToken &sourceType,
    const NdrTokenMap &metadata)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(GetInstance().GetNodeFromSourceCode(
        sourceCode, sourceType, metadata));
}

ExecNodeConstPtr
ExecRegistry::GetExecNodeByName(
    const std::string& name, const NdrTokenVec& typePriority,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(
        GetInstance().GetNodeByName(name, typePriority, filter)
    );
}

ExecNodeConstPtr
ExecRegistry::GetExecNodeByNameAndType(
    const std::string& name, const TfToken& nodeType,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeToExecNode(
        GetInstance().GetNodeByNameAndType(name, nodeType, filter)
    );
}

ExecNodePtrVec
ExecRegistry::GetExecNodesByIdentifier(const NdrIdentifier& identifier)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToExecNodeVec(
        GetInstance().GetNodesByIdentifier(identifier)
    );
}

ExecNodePtrVec
ExecRegistry::GetExecNodesByName(
    const std::string& name,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToExecNodeVec(
        GetInstance().GetNodesByName(name, filter)
    );
}

ExecNodePtrVec
ExecRegistry::GetExecNodesByFamily(
    const TfToken& family,
    NdrVersionFilter filter)
{
    // XXX Remove trace function when function performance has improved
    TRACE_FUNCTION();

    return NdrNodeVecToExecNodeVec(
        GetInstance().GetNodesByFamily(family, filter)
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
