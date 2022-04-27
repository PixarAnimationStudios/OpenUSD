//
// Unlicensed 2022 benmalartre
//
#ifndef PXR_USD_EXEC_DECLARE_H
#define PXR_USD_EXEC_DECLARE_H

/// \file exec/declare.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/declare.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class ExecNode;
class ExecProperty;

/// Common typedefs that are used throughout the exec library.

// ExecNode
typedef ExecNode* ExecNodePtr;
typedef ExecNode const* ExecNodeConstPtr;
typedef std::unique_ptr<ExecNode> ExecNodeUniquePtr;
typedef std::vector<ExecNodeConstPtr> ExecNodePtrVec;

// ExecProperty
typedef ExecProperty* ExecPropertyPtr;
typedef ExecProperty const* ExecPropertyConstPtr;
typedef std::unique_ptr<ExecProperty> ExecPropertyUniquePtr;
typedef std::unordered_map<TfToken, ExecPropertyConstPtr,
                           TfToken::HashFunctor> ExecPropertyMap;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // JVR_EXEC_DECLARE_H
