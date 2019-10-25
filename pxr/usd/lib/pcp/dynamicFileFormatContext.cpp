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
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Helper class for composing a field value from the context's inputs.
class _ComposeValueHelper
{
public:
    // Templated static function is the only public interface. ComposeFunc is
    // expected to be a function of type void (VtValue &&)
    template <typename ComposeFunc>
    static 
    bool ComposeValue(    
        const PcpNodeRef &parentNode, 
        PcpPrimIndex_StackFrame *previousFrame,
        const TfToken &fieldName, 
        bool strongestOpinionOnly,
        const ComposeFunc &composeFunc)
    {
        // Create the a new composer with the context state.
        _ComposeValueHelper composer(
            parentNode, previousFrame, fieldName, strongestOpinionOnly);
        // Initiate composition using the compose function and return if
        // a value was found.
        composer._ComposeOpinionFromAncestors(composeFunc);
        return composer._foundValue;
    }

private:
    _ComposeValueHelper(
        const PcpNodeRef &parentNode,
        PcpPrimIndex_StackFrame *&previousStackFrame,
        const TfToken &fieldName, 
        bool strongestOpinionOnly)
        : _iterator(parentNode, previousStackFrame)
        , _fieldName(fieldName)
        , _strongestOpinionOnly(strongestOpinionOnly)
    {
    }

    // Composes the values from the node and its subtree. Return true if 
    // composition should stop.
    template <typename ComposeFunc>
    bool _ComposeOpinionInSubtree(const PcpNodeRef &node, 
                                  const ComposeFunc &composeFunc)
    {
        // Search the node's layer stack in strength order for the field on
        // the spec.
        for (const SdfLayerHandle &layer : node.GetLayerStack()->GetLayers()) {
            VtValue value;
            if (layer->HasField(node.GetPath(), _fieldName, &value)) {
                // Process the value and mark found
                composeFunc(std::move(value));
                _foundValue = true;
                // Stop if we only need the strongest opinion.
                if (_strongestOpinionOnly) {
                    return true;
                }
            }
        }

        TF_FOR_ALL(childNode, Pcp_GetChildrenRange(node)) {
            if (_ComposeOpinionInSubtree(*childNode, composeFunc)) {
                return true;
            }
        }

        return false;
    };

    // Recursively composes opinions from ancestors of the parent node and 
    // their subtrees in strength order. Returns true if composition should 
    // stop.
    template <typename ComposeFunc>
    bool _ComposeOpinionFromAncestors(const ComposeFunc &composeFunc)
    {
        PcpNodeRef currentNode = _iterator.node;

        // Try parent node.
        _iterator.Next();
        if (_iterator.node) {
            // Recurse on parent node's ancestors.
            if (_ComposeOpinionFromAncestors(composeFunc)) {
                return true;
            }
        }

        // Otherwise compose from the current node and it subtrees.
        if (_ComposeOpinionInSubtree(currentNode, composeFunc)) {
            return true;
        }
        return false;
    };

    // State during value composition.
    PcpPrimIndex_StackFrameIterator _iterator;
    const TfToken &_fieldName;
    bool _strongestOpinionOnly;
    bool _foundValue {false};
};

} // anonymous namespace

PcpDynamicFileFormatContext::PcpDynamicFileFormatContext(
    const PcpNodeRef &parentNode, 
    PcpPrimIndex_StackFrame *previousStackFrame,
    TfToken::Set *composedFieldNames)
    : _parentNode(parentNode)
    , _previousStackFrame(previousStackFrame)
    , _composedFieldNames(composedFieldNames)
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
        if (_ComposeValueHelper::ComposeValue(_parentNode, _previousStackFrame, 
                field, /*findStrongestOnly = */ false,
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
        return _ComposeValueHelper::ComposeValue(_parentNode, 
            _previousStackFrame, field, /*findStrongestOnly = */ true,
            [&value](VtValue &&val){
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
    return _ComposeValueHelper::ComposeValue(_parentNode, _previousStackFrame, 
        field, /*findStrongestOnly = */ false,
        [&values](VtValue &&val){
             // Take advantage of VtValue's move assignment
             // operator.
             values->emplace_back(std::move(val));
        });
}

// "Private" function for creating a PcpDynamicFileFormatContext; should only
// be used by prim indexing.
PcpDynamicFileFormatContext
Pcp_CreateDynamicFileFormatContext(const PcpNodeRef &parentNode, 
                                   PcpPrimIndex_StackFrame *previousFrame,
                                   TfToken::Set *composedFieldNames)
{
    return PcpDynamicFileFormatContext(
        parentNode, previousFrame, composedFieldNames);
}

PXR_NAMESPACE_CLOSE_SCOPE
