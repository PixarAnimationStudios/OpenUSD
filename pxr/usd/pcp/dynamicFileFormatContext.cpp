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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dynamicFileFormatContext.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/primIndex_StackFrame.h"
#include "pxr/usd/pcp/strengthOrdering.h"
#include "pxr/usd/pcp/utils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

// Helper class for composing a field value from the context's inputs.
class PcpDynamicFileFormatContext::_ComposeValueHelper
{
public:
    // ComposeFunc is expected to be a function of type void (VtValue &&)
    template <typename ComposeFunc>
    static 
    bool ComposeFieldValue(
        const PcpDynamicFileFormatContext *context,
        const TfToken &fieldName, 
        bool strongestOpinionOnly,
        const ComposeFunc &composeFunc)
    {
        _ComposeValueHelper composer(context, strongestOpinionOnly);
        composer._ComposeOpinionFromAncestors(TfToken(), fieldName, composeFunc);
        return composer._foundValue;
    }

    // ComposeFunc is expected to be a function of type void (VtValue &&)
    template <typename ComposeFunc>
    static 
    bool ComposeAttributeDefaultValue(
        const PcpDynamicFileFormatContext *context,
        const TfToken &propName,
        const ComposeFunc &composeFunc)
    {
        // Unlike metadata fields, attributes cannot have dictionary values 
        // which simplifies this function compared to ComposeFieldValue. We 
        // compose by just grabbing the strongest default value for the 
        // attribute if one exists.
        _ComposeValueHelper composer(context);
        composer._ComposeOpinionFromAncestors(
            propName, SdfFieldKeys->Default, composeFunc);
        return composer._foundValue;
    }

private:
    _ComposeValueHelper(
        const PcpDynamicFileFormatContext *context,
        bool strongestOpinionOnly = true)
        : _iterator(context->_parentNode, context->_previousStackFrame)
        , _strongestOpinionOnly(strongestOpinionOnly)
        , _parent(context->_parentNode)
        , _pathInNode(context->_pathInNode)
        , _arcNum(context->_arcNum)
    {
    }

    // Composes the values from the node and its subtree. Return true if 
    // composition should stop.
    template <typename ComposeFunc>
    bool _ComposeOpinionInSubtree(const PcpNodeRef &node,
                                  const SdfPath &pathInNode,
                                  const TfToken &propName,
                                  const TfToken &fieldName, 
                                  const ComposeFunc &composeFunc)
    {
        // Get the prim or property path within the node's spec.
        const SdfPath &path = propName.IsEmpty() ? 
            pathInNode : pathInNode.AppendProperty(propName);

        // Search the node's layer stack in strength order for the field on
        // the spec.
        for (const auto &layer : node.GetLayerStack()->GetLayers()) {
            VtValue value;
            if (layer->HasField(path, fieldName, &value)) {
                // Process the value and mark found
                composeFunc(std::move(value));
                _foundValue = true;
                // Stop if we only need the strongest opinion.
                if (_strongestOpinionOnly) {
                    return true;
                }
            }
        }

        const bool isParent = node == _parent;
        TF_FOR_ALL(childNode, Pcp_GetChildrenRange(node)) {
            // If this is the parent, check if each of its children
            // is weaker than the future node
            if (isParent) {
                if (PcpCompareSiblingPayloadNodeStrength(
                    _parent, _arcNum, *childNode) == -1) {
                    return true;
                }
            }
            // Map the path in this node to the next child node, also applying
            // any variant selections represented by the child node.
            SdfPath pathInChildNode = 
                childNode->GetMapToParent().MapTargetToSource(
                    pathInNode.StripAllVariantSelections());
            if (pathInChildNode.IsEmpty()) {
                continue;
            }

            if (const SdfPath childNodePathAtIntro =
                    childNode->GetPathAtIntroduction();
                childNodePathAtIntro.ContainsPrimVariantSelection()) {

                pathInChildNode = pathInChildNode.ReplacePrefix(
                    childNodePathAtIntro.StripAllVariantSelections(),
                    childNodePathAtIntro);
            }

            if (_ComposeOpinionInSubtree(
                    *childNode, pathInChildNode, 
                    propName, fieldName, composeFunc)) {
                return true;
            }
        }

        // Do not look for opinions from nodes weaker than the parent.
        return isParent;
    };

    // Recursively composes opinions from ancestors of the parent node and 
    // their subtrees in strength order. Returns true if composition should 
    // stop.
    template <typename ComposeFunc>
    bool _ComposeOpinionFromAncestors(
        const TfToken &propName,
        const TfToken &fieldName, 
        const ComposeFunc &composeFunc)
    {
        return _ComposeOpinionFromAncestors(
            _iterator.node, _pathInNode.IsEmpty() ? _iterator.node.GetPath() : 
            _pathInNode,  propName, fieldName, composeFunc);
    }

    template <typename ComposeFunc>
    bool _ComposeOpinionFromAncestors(
        const PcpNodeRef &node,
        const SdfPath &pathInNode,
        const TfToken &propName,
        const TfToken &fieldName, 
        const ComposeFunc &composeFunc)
    {
        // Translate the path from the given node's namespace to
        // the root of the node's prim index.
        const auto [rootmostPath, rootmostNode] = 
            Pcp_TranslatePathFromNodeToRootOrClosestNode(node, pathInNode);

        // If we were able to translate the path all the way to the root
        // node, and we're in the middle of a recursive prim indexing
        // call, map across the previous frame and recurse.
        if (rootmostNode.IsRootNode() && _iterator.previousFrame) {
            PcpNodeRef parentNode = _iterator.previousFrame->parentNode;
            SdfPath parentNodePath = _iterator.previousFrame->arcToParent->
                mapToParent.MapSourceToTarget(
                    rootmostPath.StripAllVariantSelections());

            _iterator.NextFrame();

            if (_ComposeOpinionFromAncestors(
                    parentNode, parentNodePath, propName, fieldName,
                    composeFunc)) {
                return true;
            }
        }

        // Compose opinions in the subtree.
        if (_ComposeOpinionInSubtree(
                rootmostNode, rootmostPath, propName, fieldName, composeFunc)) {
            return true;
        }
        return false;
    };

    // State during value composition.
    PcpPrimIndex_StackFrameIterator _iterator;
    bool _strongestOpinionOnly;
    bool _foundValue {false};
    PcpNodeRef _parent;
    SdfPath _pathInNode;
    int _arcNum;
};

PcpDynamicFileFormatContext::PcpDynamicFileFormatContext(
    const PcpNodeRef &parentNode, 
    const SdfPath &pathInNode,
    int arcNum,
    PcpPrimIndex_StackFrame *previousStackFrame,
    TfToken::Set *composedFieldNames,
    TfToken::Set *composedAttributeNames)
    : _parentNode(parentNode)
    , _pathInNode(pathInNode)
    , _arcNum(arcNum)
    , _previousStackFrame(previousStackFrame)
    , _composedFieldNames(composedFieldNames)
    , _composedAttributeNames(composedAttributeNames)
{
}

bool
PcpDynamicFileFormatContext::_IsAllowedFieldForArguments(
    const TfToken &field, bool *fieldValueIsDictionary) const
{
    // We're starting off by restricting the allowed fields to be only fields
    // defined by plugins. We may ease this in the future to allow certain 
    // builtin fields as well but there will need to be some updates to 
    // change management to handle these correctly.
    const SdfSchemaBase &schema = 
        _parentNode.GetLayerStack()->GetIdentifier().rootLayer->GetSchema();
    const SdfSchema::FieldDefinition* fieldDef = 
        schema.GetFieldDefinition(field);
    if (!(fieldDef && fieldDef->IsPlugin())) {
        TF_CODING_ERROR("Field %s is not a plugin field and is not supported "
                        "for composing dynamic file format arguments",
                        field.GetText());
        return false;
    }

    if (fieldValueIsDictionary) {
        *fieldValueIsDictionary = 
            fieldDef->GetFallbackValue().IsHolding<VtDictionary>();
    }

    return true;
}

bool 
PcpDynamicFileFormatContext::ComposeValue(
    const TfToken &field, VtValue *value) const
{
    bool fieldIsDictValued = false;
    if (!_IsAllowedFieldForArguments(field, &fieldIsDictValued)) {
        return false;
    }

    // Update the cached field names for dependency tracking.
    if (_composedFieldNames) {
        _composedFieldNames->insert(field);
    }

    // If the field is a dictionary, compose the dictionary's key values from 
    // strongest to weakest opinions.
    if (fieldIsDictValued) {
        VtDictionary composedDict;
        if (_ComposeValueHelper::ComposeFieldValue(
                this, field, /*findStrongestOnly = */ false,
                [&composedDict](VtValue &&val){
                    if (val.IsHolding<VtDictionary>()) {
                        VtDictionaryOverRecursive(
                            &composedDict, val.UncheckedGet<VtDictionary>());
                    } else {
                        TF_CODING_ERROR("Expected value to contain VtDictionary");
                    }
                })
            ) {
            // Output the composed dictionary only if we found a value for the 
            // field.
            value->Swap(composedDict);
            return true;
        }
        return false;
    } else {
        // For all other value type we compose by just grabbing the strongest 
        // opinion if it exists.
        return _ComposeValueHelper::ComposeFieldValue(
            this, field, /*findStrongestOnly = */ true,
            [&value](VtValue &&val) {
                // Take advantage of VtValue's move assignment
                // operator.
                *value = std::move(val); 
            });
    }
}

bool
PcpDynamicFileFormatContext::ComposeValueStack(
    const TfToken &field, VtValueVector *values) const
{
    if (!_IsAllowedFieldForArguments(field)) {
        return false;
    }

    // Update the cached field names for dependency tracking.
    if (_composedFieldNames) {
        _composedFieldNames->insert(field);
    }

    // For the value stack, just add all opinions we can find for the field
    // in strength order.
    return _ComposeValueHelper::ComposeFieldValue(this, 
        field, /*findStrongestOnly = */ false,
        [&values](VtValue &&val){
             // Take advantage of VtValue's move assignment
             // operator.
             values->emplace_back(std::move(val));
        });
}

bool 
PcpDynamicFileFormatContext::ComposeAttributeDefaultValue(
    const TfToken &attributeName, VtValue *value) const
{
    // Update the cached attribute names for dependency tracking.
    if (_composedAttributeNames) {
        _composedAttributeNames->insert(attributeName);
    }

    // Unlike metadata fields, attributes cannot have dictionary values which
    // simplifies this function compared to ComputeValue.
    return _ComposeValueHelper::ComposeAttributeDefaultValue(
        this, attributeName,
        [&value](VtValue &&val) {
            // Take advantage of VtValue's move assignment operator.
            *value = std::move(val); 
        });
}

// "Private" function for creating a PcpDynamicFileFormatContext; should only
// be used by prim indexing.
PcpDynamicFileFormatContext
Pcp_CreateDynamicFileFormatContext(const PcpNodeRef &parentNode, 
                                   const SdfPath &ancestralPath,
                                   int arcNum,
                                   PcpPrimIndex_StackFrame *previousFrame,
                                   TfToken::Set *composedFieldNames,
                                   TfToken::Set *composedAttributeNames)
{
    return PcpDynamicFileFormatContext(
        parentNode, ancestralPath, arcNum, previousFrame, composedFieldNames, 
        composedAttributeNames);
}

PXR_NAMESPACE_CLOSE_SCOPE
