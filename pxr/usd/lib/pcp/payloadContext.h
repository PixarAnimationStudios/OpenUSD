//
// Copyright 2016 Pixar
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
#ifndef PCP_PAYLOAD_CONTEXT_H
#define PCP_PAYLOAD_CONTEXT_H

#include "pxr/usd/pcp/types.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/type.h"

#include <boost/function.hpp>

class PcpPrimIndex_StackFrame;
class TfToken;

/// \class PcpPayloadContext
///
/// Context object that allows PcpPayloadDecorator subclasses to
/// examine the prim index being constructed. See PcpPayloadDecorator
/// for more details.
class PcpPayloadContext 
{
public:
    /// Callback function for ComposeValue. This callback function will
    /// be passed values for the field given to ComposeValue from
    /// strongest to weakest available opinion and is free to copy or
    /// swap out the value as desired. 
    /// 
    /// This function should return true if composition is done, 
    /// meaning no more values will be passed to this function, or false
    /// if composition should continue.
    typedef boost::function<bool(VtValue*)> ComposeFunction;

    /// Compose the value of the scene description \p field using the
    /// given composition function \p fn from strongest to weakest
    /// available opinion. 
    bool ComposeValue(const TfToken& field, const ComposeFunction& fn) const;

private:
    PcpPayloadContext(
        const PcpNodeRef& parentNode, 
        PcpPrimIndex_StackFrame* previousFrame);

    friend PcpPayloadContext Pcp_CreatePayloadContext(
        const PcpNodeRef&, PcpPrimIndex_StackFrame*);

private:
    PcpNodeRef _parentNode;
    PcpPrimIndex_StackFrame* _previousStackFrame;
};

#endif // PCP_PAYLOAD_CONTEXT_H
