//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_DYNAMIC_FILE_FORMAT_CONTEXT_H
#define PXR_USD_PCP_DYNAMIC_FILE_FORMAT_CONTEXT_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/node.h"

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex_StackFrame;
class PcpCache;

/// \class PcpDynamicFileFormatContext
///
/// Context object for the current state of a prim index that is being built
/// that allows implementations of PcpDynamicFileFormatInterface to compose
/// field values when generating dynamic file format arguments. The context 
/// allows us to iterate over all nodes that have already been composed looking
/// for the strongest opinion for a relevant field. 
class PcpDynamicFileFormatContext
{
public:
    using VtValueVector = std::vector<VtValue>;

    PCP_API
    ~PcpDynamicFileFormatContext() = default;

    /// Compose the \p value of the given \p field and return its current 
    /// strongest opinion. For dictionary valued fields this will be a 
    /// dictionary containing the strongest value for each individual key. 
    /// Returns true if a value for the field was found. 
    PCP_API
    bool ComposeValue(const TfToken &field, VtValue *value) const;

    /// Compose the \p values of the given \p field returning all available 
    /// opinions ordered from strongest to weakest. For dictionary valued 
    /// fields, the dictionaries from each opinion are not composed together 
    /// at each step and are instead returned in the list as is. 
    /// Returns true if a value for the field was found. 
    ///
    /// Note that this is slower than ComposeValue, especially for 
    /// non-dictionary valued fields, and should only be used if knowing more 
    /// than just the strongest value is necessary.
    PCP_API
    bool ComposeValueStack(const TfToken &field, 
                           VtValueVector *values) const;

    /// Compose the \p value of the default field of the attribute with the 
    /// given \p attributeName and return its current strongest opinion.
    /// Returns true if a value for the field was found. 
    PCP_API
    bool ComposeAttributeDefaultValue(
        const TfToken &attributeName, VtValue *value) const;

private:
    /// Constructs a context. 
    /// \p parentNode and \p previousFrame are used to traverse the 
    /// current state of the prim index graph when composing the opinions on 
    /// fields. \p composedFieldNames is the set of field names that is
    /// to be updated with the names of fields that ComposeValue and 
    /// ComposeValueStack are called on for dependency tracking.
    PcpDynamicFileFormatContext(
        const PcpNodeRef &parentNode, 
        const SdfPath &pathInNode,
        int arcNum,
        PcpPrimIndex_StackFrame *previousFrame,
        TfToken::Set *composedFieldNames,
        TfToken::Set *composedAttributeNames);

    /// Access to private constructor. Should only be called by prim indexing.
    friend PcpDynamicFileFormatContext Pcp_CreateDynamicFileFormatContext(
        const PcpNodeRef &, const SdfPath&, int, PcpPrimIndex_StackFrame *, 
        TfToken::Set *, TfToken::Set *);

    /// Returns whether the given \p field is allowed to be used to generate
    /// file format arguments. It can also return whether the value type of 
    /// the field is a dictionary if needed.
    bool _IsAllowedFieldForArguments(
        const TfToken &field, bool *fieldValueIsDictionary = nullptr) const;

private:
    PcpNodeRef _parentNode;
    SdfPath _pathInNode;
    int _arcNum;
    PcpPrimIndex_StackFrame *_previousStackFrame;

    // Cached names of fields that had values composed by this context.
    TfToken::Set *_composedFieldNames;

    // Cached names of attributes that had default values composed by this 
    // context.
    TfToken::Set *_composedAttributeNames;

    // Declare private helper.
    class _ComposeValueHelper;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_DYNAMIC_FILE_FORMAT_CONTEXT_H
