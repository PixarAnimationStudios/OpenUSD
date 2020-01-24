//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

private:
    // Callback function for ComposeValue. This callback function will
    // be passed values for the field given to ComposeValue from
    // strongest to weakest available opinion and is free to copy or
    // swap out the value as desired. 
    using _ComposeFunction = std::function<void(VtValue &&)>;

    /// Constructs a context. 
    /// \p parentNode and \p previousFrame are used to traverse the 
    /// current state of the prim index graph when composing the opinions on 
    /// fields. \p composedFieldNames is the set of field names that is
    /// to be updated with the names of fields that ComposeValue and 
    /// ComposeValueStack are called on for dependency tracking.
    PcpDynamicFileFormatContext(
        const PcpNodeRef &parentNode, 
        PcpPrimIndex_StackFrame *previousFrame,
        TfToken::Set *composedFieldNames);
    /// Access to private constructor. Should only be called by prim indexing.
    friend PcpDynamicFileFormatContext Pcp_CreateDynamicFileFormatContext(
        const PcpNodeRef &, PcpPrimIndex_StackFrame *, TfToken::Set *);

    /// Returns whether the given \p field is allowed to be used to generate
    /// file format arguments. It can also return whether the value type of 
    /// the field is a dictionary if needed.
    bool _IsAllowedFieldForArguments(
        const TfToken &field, bool *fieldValueIsDictionary = nullptr) const;

private:
    PcpNodeRef _parentNode;
    PcpPrimIndex_StackFrame *_previousStackFrame;

    // Cached names of fields that had values composed by this context.
    TfToken::Set *_composedFieldNames;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_DYNAMIC_FILE_FORMAT_CONTEXT_H
