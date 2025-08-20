//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_REQUIRED_INPUTS_PREDICATE_H
#define PXR_EXEC_VDF_REQUIRED_INPUTS_PREDICATE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/input.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfRequiredInputsPredicate
///
/// \brief This predicate determines whether a given input value is needed to
///        fulfill the input dependencies required by the node.
///
class VdfRequiredInputsPredicate
{
public:
    /// All read inputs on \p node are required.
    ///
    static VdfRequiredInputsPredicate AllReads(const VdfNode &node) {
        return VdfRequiredInputsPredicate(_Selector::AllReads, node, nullptr);
    }

    /// None of the read inputs on \p node are required.
    ///
    static VdfRequiredInputsPredicate NoReads(const VdfNode &node) {
        return VdfRequiredInputsPredicate(_Selector::NoReads, node, nullptr);
    }

    /// One specific read \p input on \p node is required.
    ///
    static VdfRequiredInputsPredicate OneRead(
        const VdfNode &node,
        const VdfInput &input) {
        return VdfRequiredInputsPredicate(_Selector::OneRead, node, &input);
    }

    /// Is this input a required read? Note that read/writes as well as
    /// prerequisite inputs are not required reads.
    ///
    bool IsRequiredRead(const VdfInput &input) const {
        return
            _selector != _Selector::NoReads &&
            !input.GetAssociatedOutput() &&
            !input.GetSpec().IsPrerequisite() &&
            (_selector == _Selector::AllReads ||
                (_oneRead == &input && &input.GetNode() == &_node));
    }

    /// Are any inputs required?
    ///
    bool HasRequiredReads() const {
        return _selector != _Selector::NoReads;
    }

    /// Are all of the inputs required?
    ///
    bool RequiresAllReads() const {
        return _selector == _Selector::AllReads;
    }

private:
    // Denotes how inputs are selected.
    enum class _Selector {
        NoReads,
        AllReads,
        OneRead
    };

    // Constructor. \p oneRead is only relevant if \p selector equals
    // _Selector::OneRead. Otherwise \p oneRead may be \c nullptr.
    VdfRequiredInputsPredicate(
        _Selector selector,
        const VdfNode &node,
        const VdfInput *oneRead) :
        _selector(selector),
        _node(node),
        _oneRead(oneRead)
    {}

    // Select inputs based on this choice of selector.
    _Selector _selector;

    // The owning node.
    const VdfNode &_node;

    // The input, if _selector is OneRead.
    const VdfInput *_oneRead;
};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif