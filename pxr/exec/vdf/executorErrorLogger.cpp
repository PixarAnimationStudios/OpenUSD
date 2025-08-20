//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorErrorLogger.h"

#include "pxr/exec/vdf/node.h"

#include "pxr/base/arch/functionLite.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExecutorErrorLogger::VdfExecutorErrorLogger()
:   _warnings(nullptr)
{
}

VdfExecutorErrorLogger::~VdfExecutorErrorLogger()
{
    // Delete any node -> warnings map, if any.
    delete _warnings.load(std::memory_order_acquire);
}

const VdfExecutorErrorLogger::NodeToStringMap &
VdfExecutorErrorLogger::GetWarnings() const
{
    if (const NodeToStringMap *warnings =
        _warnings.load(std::memory_order_acquire)) {
        return *warnings;
    }

    static TfStaticData<const NodeToStringMap> empty;
    return *empty;
}

void 
VdfExecutorErrorLogger::ReportWarnings() const
{
    TF_FOR_ALL(w, GetWarnings())
        IssueDefaultWarning(*(w->first), w->second);
}

void 
VdfExecutorErrorLogger::IssueDefaultWarning(
    const VdfNode &node,
    const std::string &warning)
{
    TF_WARN("Node: '%s' Exec Warning: %s\n", 
        node.GetDebugName().c_str(), warning.c_str());
}

void
VdfExecutorErrorLogger::LogWarning(
    const VdfNode &node,
    const std::string &warning) const
{
    TfAutoMallocTag2 tag2("Vdf", __ARCH_PRETTY_FUNCTION__);

    NodeToStringMap *warnings = _warnings.load(std::memory_order_acquire);

    if (!warnings) {
        NodeToStringMap *newWarnings = new NodeToStringMap();

        if (!_warnings.compare_exchange_strong(warnings, newWarnings)) {
            delete newWarnings;
        } else {
            warnings = newWarnings;
        }
    }

    std::pair<NodeToStringMap::iterator, bool> res = 
        warnings->insert(std::make_pair(&node, warning));

    if (!res.second) { 
        // Don't concatenate if the warning text is the repeated.
        if (res.first->second != warning) {
            res.first->second += " " + warning;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

