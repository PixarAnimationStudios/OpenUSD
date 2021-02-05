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
#include "pxr/pxr.h"
#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/stringUtils.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using std::vector;
using std::string;

/* static */
string 
UsdShadeUtils::GetPrefixForAttributeType(UsdShadeAttributeType sourceType)
{
    switch (sourceType) {
        case UsdShadeAttributeType::Input:
            return UsdShadeTokens->inputs.GetString();
        case UsdShadeAttributeType::Output:
            return UsdShadeTokens->outputs.GetString();
        default:
            return string();
    }
}

/* static */
std::pair<TfToken, UsdShadeAttributeType> 
UsdShadeUtils::GetBaseNameAndType(const TfToken &fullName)
{
    std::pair<std::string, bool> res = 
        SdfPath::StripPrefixNamespace(fullName, UsdShadeTokens->inputs);
    if (res.second) {
        return std::make_pair(TfToken(res.first), UsdShadeAttributeType::Input);
    }

    res = SdfPath::StripPrefixNamespace(fullName, UsdShadeTokens->outputs);
    if (res.second) {
        return std::make_pair(TfToken(res.first),UsdShadeAttributeType::Output);
    }

    return std::make_pair(fullName, UsdShadeAttributeType::Invalid);
}

/* static */
UsdShadeAttributeType
UsdShadeUtils::GetType(const TfToken &fullName)
{
    std::pair<std::string, bool> res = 
        SdfPath::StripPrefixNamespace(fullName, UsdShadeTokens->inputs);
    if (res.second) {
        return UsdShadeAttributeType::Input;
    }

    res = SdfPath::StripPrefixNamespace(fullName, UsdShadeTokens->outputs);
    if (res.second) {
        return UsdShadeAttributeType::Output;
    }

    return UsdShadeAttributeType::Invalid;
}

/* static */
TfToken 
UsdShadeUtils::GetFullName(const TfToken &baseName, 
                           const UsdShadeAttributeType type)
{
    return TfToken(UsdShadeUtils::GetPrefixForAttributeType(type) + 
                   baseName.GetString());
}

// Note: to avoid getting stuck in an infinite loop when following connections,
// we need to check if we've visited an attribute before, so that we can break
// the cycle and return an invalid result.
// We expect most connections chains to be very small with most of them having
// 0 or 1 connection in the chain. Few will include multiple hops. That is why
// we are going with a vector and not a set to check for previous attributes.
// To avoid the cost of allocating memory on the heap at each invocation, we
// use a TfSmallVector to keep the first couple of entries on the stack.
constexpr unsigned int N = 5;
typedef TfSmallVector<SdfPath, N> _SmallSdfPathVector;

template <typename UsdShadeInOutput>
bool
_GetValueProducingAttributesRecursive(
    UsdShadeInOutput const & inoutput,
    _SmallSdfPathVector* foundAttributes,
    UsdShadeAttributeVector & attrs,
    bool shaderOutputsOnly);

bool
_FollowConnectionSourceRecursive(
    UsdShadeConnectionSourceInfo const & sourceInfo,
    _SmallSdfPathVector* foundAttributes,
    UsdShadeAttributeVector & attrs,
    bool shaderOutputsOnly)
{
    if (sourceInfo.sourceType == UsdShadeAttributeType::Output) {
        UsdShadeOutput connectedOutput =
                sourceInfo.source.GetOutput(sourceInfo.sourceName);
        if (!sourceInfo.source.IsContainer()) {
            attrs.push_back(connectedOutput.GetAttr());
            return true;
        } else {
            return _GetValueProducingAttributesRecursive(connectedOutput,
                                                         foundAttributes,
                                                         attrs,
                                                         shaderOutputsOnly);
        }
    } else { // sourceType == UsdShadeAttributeType::Input
        UsdShadeInput connectedInput =
                sourceInfo.source.GetInput(sourceInfo.sourceName);
        if (!sourceInfo.source.IsContainer()) {
            // Note, this is an invalid situation for a connected
            // chain. Since we started on an input to either a
            // Shader or a container we cannot legally connect to an
            // input on a non-container.
        } else {
            return _GetValueProducingAttributesRecursive(connectedInput,
                                                         foundAttributes,
                                                         attrs,
                                                         shaderOutputsOnly);
        }
    }

    return false;
}

template <typename UsdShadeInOutput>
bool
_GetValueProducingAttributesRecursive(
    UsdShadeInOutput const & inoutput,
    _SmallSdfPathVector* foundAttributes,
    UsdShadeAttributeVector & attrs,
    bool shaderOutputsOnly)
{
    if (!inoutput) {
        return false;
    }

    // Check if we've visited this attribute before and if so abort with an
    // error, since this means we have a loop in the chain
    const SdfPath& thisAttrPath = inoutput.GetAttr().GetPath();
    if (!foundAttributes->empty() &&
        std::find(foundAttributes->begin(), foundAttributes->end(),
                  thisAttrPath) != foundAttributes->end()) {
        TF_WARN("GetValueProducingAttributes: Found cycle with attribute %s",
                thisAttrPath.GetText());
        return false;
    }

    // Retrieve all valid connections
    UsdShadeSourceInfoVector sourceInfos =
        UsdShadeConnectableAPI::GetConnectedSources(inoutput);

    if (!sourceInfos.empty()) {
        // Remember the path of this attribute, so that we do not visit it again
        // Since this a cycle protection we only need to do this if we have
        // valid connections
        foundAttributes->push_back(thisAttrPath);
    }

    bool foundValidAttr = false;

    if (sourceInfos.size() > 1) {
        // Follow each connection until we reach an output attribute on an
        // actual shader node or an input attribute with a value
        for (const UsdShadeConnectionSourceInfo& sourceInfo : sourceInfos) {
            // To handle cycle detection in the case of multiple connection we
            // have to copy the found attributes vector (multiple connections
            // leading to the same attribute would trigger the cycle detection).
            // Since we want to avoid that copy we only do it in case of
            // multiple connections.
            _SmallSdfPathVector localFoundAttrs = *foundAttributes;

            foundValidAttr |=
                _FollowConnectionSourceRecursive(sourceInfo,
                                                 &localFoundAttrs,
                                                 attrs,
                                                 shaderOutputsOnly);
        }
    } else if (!sourceInfos.empty()) {
        // Follow the one connection it until we reach an output attribute on an
        // actual shader node or an input attribute with a value
        foundValidAttr =
                _FollowConnectionSourceRecursive(sourceInfos[0],
                                                 foundAttributes,
                                                 attrs,
                                                 shaderOutputsOnly);
    }

    // If our trace should accept attributes with authored values, check if this
    // input or output doesn't have any valid attributes from connections, but
    // has an authored value. Return this attribute.
    if (!shaderOutputsOnly && !foundValidAttr) {
        // N.B. Checking whether an attribute has an authored value is a
        // non-trivial operation and should not be done unless required
        if (inoutput.GetAttr().HasAuthoredValue()) {
            VtValue val;
            inoutput.GetAttr().Get(&val);
            attrs.push_back(inoutput.GetAttr());
            foundValidAttr = true;
        }
    }

    return foundValidAttr;
}

/* static */
UsdShadeAttributeVector
UsdShadeUtils::GetValueProducingAttributes(UsdShadeInput const &input,
                                           bool shaderOutputsOnly)
{
    TRACE_FUNCTION_SCOPE("INPUT");

    // We track which attributes we've visited so far to avoid getting caught
    // in an infinite loop, if the network contains a cycle.
    _SmallSdfPathVector foundAttributes;

    UsdShadeAttributeVector valueAttributes;
    _GetValueProducingAttributesRecursive(input,
                                          &foundAttributes,
                                          valueAttributes,
                                          shaderOutputsOnly);

    return valueAttributes;
}

/* static */
UsdShadeAttributeVector
UsdShadeUtils::GetValueProducingAttributes(UsdShadeOutput const &output,
                                           bool shaderOutputsOnly)
{
    TRACE_FUNCTION_SCOPE("OUTPUT");

    // We track which attributes we've visited so far to avoid getting caught
    // in an infinite loop, if the network contains a cycle.
    _SmallSdfPathVector foundAttributes;

    UsdShadeAttributeVector valueAttributes;
    _GetValueProducingAttributesRecursive(output,
                                          &foundAttributes,
                                          valueAttributes,
                                          shaderOutputsOnly);

    return valueAttributes;
}

PXR_NAMESPACE_CLOSE_SCOPE

