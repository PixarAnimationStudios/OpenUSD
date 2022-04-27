//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/tokens.h"
#include "pxr/usd/usdExec/execUtils.h"
#include "pxr/usd/usdExec/execInput.h"
#include "pxr/usd/usdExec/execOutput.h"
#include "pxr/usd/usdExec/execConnectableAPI.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/stringUtils.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using std::vector;
using std::string;

/* static */
string 
UsdExecUtils::GetPrefixForAttributeType(UsdExecAttributeType sourceType)
{
    switch (sourceType) {
        case UsdExecAttributeType::Input:
            return UsdExecTokens->inputs.GetString();
        case UsdExecAttributeType::Output:
            return UsdExecTokens->outputs.GetString();
        default:
            return string();
    }
}

/* static */
std::pair<TfToken, UsdExecAttributeType> 
UsdExecUtils::GetBaseNameAndType(const TfToken &fullName)
{
    std::pair<std::string, bool> res = 
        SdfPath::StripPrefixNamespace(fullName, UsdExecTokens->inputs);
    if (res.second) {
        return std::make_pair(TfToken(res.first), UsdExecAttributeType::Input);
    }

    res = SdfPath::StripPrefixNamespace(fullName, UsdExecTokens->outputs);
    if (res.second) {
        return std::make_pair(TfToken(res.first),UsdExecAttributeType::Output);
    }

    return std::make_pair(fullName, UsdExecAttributeType::Invalid);
}

/* static */
UsdExecAttributeType
UsdExecUtils::GetType(const TfToken &fullName)
{
    std::pair<std::string, bool> res = 
        SdfPath::StripPrefixNamespace(fullName, UsdExecTokens->inputs);
    if (res.second) {
        return UsdExecAttributeType::Input;
    }

    res = SdfPath::StripPrefixNamespace(fullName, UsdExecTokens->outputs);
    if (res.second) {
        return UsdExecAttributeType::Output;
    }

    return UsdExecAttributeType::Invalid;
}

/* static */
TfToken 
UsdExecUtils::GetFullName(const TfToken &baseName, 
                           const UsdExecAttributeType type)
{
    return TfToken(UsdExecUtils::GetPrefixForAttributeType(type) + 
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

template <typename UsdExecInOutput>
bool
_GetValueProducingAttributesRecursive(
    UsdExecInOutput const & inoutput,
    _SmallSdfPathVector* foundAttributes,
    UsdExecAttributeVector & attrs,
    bool outputsOnly);

bool
_FollowConnectionSourceRecursive(
    UsdExecConnectionSourceInfo const & sourceInfo,
    _SmallSdfPathVector* foundAttributes,
    UsdExecAttributeVector & attrs,
    bool outputsOnly)
{
    if (sourceInfo.sourceType == UsdExecAttributeType::Output) {
        UsdExecOutput connectedOutput =
                sourceInfo.source.GetOutput(sourceInfo.sourceName);
        if (!sourceInfo.source.IsContainer()) {
            attrs.push_back(connectedOutput.GetAttr());
            return true;
        } else {
            return _GetValueProducingAttributesRecursive(connectedOutput,
                                                         foundAttributes,
                                                         attrs,
                                                         outputsOnly);
        }
    } else { // sourceType == UsdExecAttributeType::Input
        UsdExecInput connectedInput =
                sourceInfo.source.GetInput(sourceInfo.sourceName);
        if (!sourceInfo.source.IsContainer()) {
            // Note, this is an invalid situation for a connected
            // chain. Since we started on an input to either a
            // Node or a container we cannot legally connect to an
            // input on a non-container.
        } else {
            return _GetValueProducingAttributesRecursive(connectedInput,
                                                         foundAttributes,
                                                         attrs,
                                                         outputsOnly);
        }
    }

    return false;
}

template <typename UsdExecInOutput>
bool
_GetValueProducingAttributesRecursive(
    UsdExecInOutput const & inoutput,
    _SmallSdfPathVector* foundAttributes,
    UsdExecAttributeVector & attrs,
    bool outputsOnly)
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
    UsdExecSourceInfoVector sourceInfos =
        UsdExecConnectableAPI::GetConnectedSources(inoutput);

    if (!sourceInfos.empty()) {
        // Remember the path of this attribute, so that we do not visit it again
        // Since this a cycle protection we only need to do this if we have
        // valid connections
        foundAttributes->push_back(thisAttrPath);
    }

    bool foundValidAttr = false;

    if (sourceInfos.size() > 1) {
        // Follow each connection until we reach an output attribute on an
        // actual node or an input attribute with a value
        for (const UsdExecConnectionSourceInfo& sourceInfo : sourceInfos) {
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
                                                 outputsOnly);
        }
    } else if (!sourceInfos.empty()) {
        // Follow the one connection it until we reach an output attribute on an
        // actual node or an input attribute with a value
        foundValidAttr =
                _FollowConnectionSourceRecursive(sourceInfos[0],
                                                 foundAttributes,
                                                 attrs,
                                                 outputsOnly);
    }

    // If our trace should accept attributes with authored values, check if this
    // input or output doesn't have any valid attributes from connections, but
    // has an authored value. Return this attribute.
    if (!outputsOnly && !foundValidAttr) {
        // N.B. Checking whether an attribute has an authored value is a
        // non-trivial operation and should not be done unless required
        if (inoutput.GetAttr().HasAuthoredValue()) {
            attrs.push_back(inoutput.GetAttr());
            foundValidAttr = true;
        }
    }

    return foundValidAttr;
}

/* static */
UsdExecAttributeVector
UsdExecUtils::GetValueProducingAttributes(UsdExecInput const &input,
                                           bool outputsOnly)
{
    TRACE_FUNCTION_SCOPE("INPUT");

    // We track which attributes we've visited so far to avoid getting caught
    // in an infinite loop, if the network contains a cycle.
    _SmallSdfPathVector foundAttributes;

    UsdExecAttributeVector valueAttributes;
    _GetValueProducingAttributesRecursive(input,
                                          &foundAttributes,
                                          valueAttributes,
                                          outputsOnly);

    return valueAttributes;
}

/* static */
UsdExecAttributeVector
UsdExecUtils::GetValueProducingAttributes(UsdExecOutput const &output,
                                           bool outputsOnly)
{
    TRACE_FUNCTION_SCOPE("OUTPUT");

    // We track which attributes we've visited so far to avoid getting caught
    // in an infinite loop, if the network contains a cycle.
    _SmallSdfPathVector foundAttributes;

    UsdExecAttributeVector valueAttributes;
    _GetValueProducingAttributesRecursive(output,
                                          &foundAttributes,
                                          valueAttributes,
                                          outputsOnly);

    return valueAttributes;
}

PXR_NAMESPACE_CLOSE_SCOPE

