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
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeConnectableAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ConnectableAPI)
);

/* virtual */
UsdShadeConnectableAPI::~UsdShadeConnectableAPI()
{
}

/* static */
UsdShadeConnectableAPI
UsdShadeConnectableAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeConnectableAPI();
    }
    return UsdShadeConnectableAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaType UsdShadeConnectableAPI::_GetSchemaType() const {
    return UsdShadeConnectableAPI::schemaType;
}

/* static */
const TfType &
UsdShadeConnectableAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeConnectableAPI>();
    return tfType;
}

/* static */
bool 
UsdShadeConnectableAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeConnectableAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeConnectableAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/usd/usdShade/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (outputName)
    (outputs)
);

bool 
UsdShadeConnectableAPI::IsShader() const
{
    return GetPrim().IsA<UsdShadeShader>();
}

bool 
UsdShadeConnectableAPI::IsNodeGraph() const
{
    return GetPrim().IsA<UsdShadeNodeGraph>();
}

/* virtual */
bool 
UsdShadeConnectableAPI::_IsCompatible() const
{
    if (!UsdAPISchemaBase::_IsCompatible() )
        return false;

    // Shaders and node-graphs are compatible with this API schema. 
    // XXX: What if the typeName isn't known (eg, pure over)?
    return IsShader() || IsNodeGraph();
}

static bool 
_CanConnectOutputToSource(const UsdShadeOutput &output, 
                          const UsdAttribute &source,
                          std::string *reason)
{
    if (!output.IsDefined()) {
        if (reason) {
            *reason = TfStringPrintf("Invalid output");
        }
        return false;
    }

    // Only outputs on node-graphs are connectable.
    if (!UsdShadeConnectableAPI(output.GetPrim()).IsNodeGraph()) {
        if (reason) {
            *reason = "Output does not belong to a node-graph.";
        }
        return false;
    }

    if (source) {
        // Ensure that the source prim is a descendent of the node-graph owning 
        // the output.
        const SdfPath sourcePrimPath = source.GetPrim().GetPath();
        const SdfPath outputPrimPath = output.GetPrim().GetPath();

        if (!sourcePrimPath.HasPrefix(outputPrimPath)) {
            if (reason) {
                *reason = TfStringPrintf("Source of output '%s' on node-graph "
                    "at path <%s> is outside the node-graph: <%s>",
                    source.GetName().GetText(), outputPrimPath.GetText(),
                    sourcePrimPath.GetText());
            }
            return false;
        }
        
    }

    return true;
}

bool
UsdShadeConnectableAPI::CanConnect(
    const UsdShadeOutput &output, 
    const UsdAttribute &source)
{
    std::string reason;
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    return _CanConnectOutputToSource(output, source, &reason);
}

bool
_CanConnectInputToSource(const UsdShadeInput &input, 
                         const UsdAttribute &source,
                         std::string *reason)
{
    if (!input.IsDefined()) {
        if (reason) {
            *reason = TfStringPrintf("Invalid input: %s",  
                input.GetAttr().GetPath().GetText());
        }
        return false;
    }

    if (!source) {
        if (reason) {
            *reason = TfStringPrintf("Invalid source: %s", 
                source.GetPath().GetText());
        }
        return false;
    }

    TfToken inputConnectability = input.GetConnectability();
    if (inputConnectability == UsdShadeTokens->full) {
        return true;
    } else if (inputConnectability == UsdShadeTokens->interfaceOnly) {
        if (UsdShadeInput::IsInput(source)) {
            TfToken sourceConnectability = 
                UsdShadeInput(source).GetConnectability();
            if (sourceConnectability == UsdShadeTokens->interfaceOnly) {
                return true;
            }
        }
    }

    if (reason) {
        *reason = TfStringPrintf("Input connectability is 'interfaceOnly' and "
            "source does not have 'interfaceOnly' connectability.");
    }

    return false;
}

bool
UsdShadeConnectableAPI::CanConnect(
    const UsdShadeInput &input, 
    const UsdAttribute &source)
{
    std::string reason;
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    return _CanConnectInputToSource(input, source, &reason);
}

/* static */
bool  
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    UsdShadeConnectableAPI const &source, 
    TfToken const &sourceName,
    UsdShadeAttributeType const sourceType,
    SdfValueTypeName typeName)
{
    UsdPrim sourcePrim = source.GetPrim();
    bool  success = true;

    // XXX it WBN to be able to validate source itself, guaranteeing
    // that the source is, in fact connectable (i.e., a shader or node-graph).
    // However, it remains useful to be able to target a pure-over.
    if (sourcePrim) {
        std::string prefix = UsdShadeUtils::GetPrefixForAttributeType(
            sourceType);
        TfToken sourceAttrName(prefix + sourceName.GetString());

        UsdAttribute sourceAttr = sourcePrim.GetAttribute(sourceAttrName);

        // First make sure there is a source attribute of the proper type
        // on the sourcePrim.
        if (!sourceAttr) {
            // If a typeName isn't specified, 
            if (!typeName) {
                // If sourceAttr does not exist, get typeName from shading
                // attribute
                typeName = shadingAttr.GetTypeName();
            }
            sourceAttr = sourcePrim.CreateAttribute(sourceAttrName, typeName,
                /* custom = */ false);
        }

        success = shadingAttr.SetConnections(
            SdfPathVector{sourceAttr.GetPath()});

    } else if (!source) {
        TF_CODING_ERROR("Failed connecting shading attribute <%s>. "
                        "The given source shader prim <%s> is not defined", 
                        shadingAttr.GetPath().GetText(),
                        source.GetPrim() ? source.GetPath().GetText() :
                        "invalid-prim");
        return false;
    }

    return success;
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    SdfPath const &sourcePath)
{
    // sourcePath needs to be a property path for us to make a connection.
    if (!sourcePath.IsPropertyPath())
        return false;

    UsdPrim sourcePrim = shadingAttr.GetStage()->GetPrimAtPath(
        sourcePath.GetPrimPath());
    UsdShadeConnectableAPI source(sourcePrim);
    // We don't validate UsdShadeConnectableAPI, as the type of the source prim 
    // may be unknown. (i.e. it could be a pure over or a typeless def).

    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtils::GetBaseNameAndType(
        sourcePath.GetNameToken());

    // In case the sourceAttr does not exist, use typeName from shading
    // attribute
    SdfValueTypeName typeName = shadingAttr.GetTypeName();
    return ConnectToSource(shadingAttr, source, sourceName, sourceType, 
        typeName);
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr, 
    UsdShadeInput const &sourceInput)
{
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtils::GetBaseNameAndType(
        sourceInput.GetFullName());

    return ConnectToSource(
        shadingAttr, 
        UsdShadeConnectableAPI(sourceInput.GetPrim()),
        sourceName,
        sourceType, 
        sourceInput.GetTypeName());
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr, 
    UsdShadeOutput const &sourceOutput)
{
    return UsdShadeConnectableAPI::ConnectToSource(shadingAttr, 
        UsdShadeConnectableAPI(sourceOutput.GetPrim()),
        sourceOutput.GetBaseName(), UsdShadeAttributeType::Output,
        sourceOutput.GetTypeName());
}

/* static */
bool
UsdShadeConnectableAPI::GetConnectedSource(
    UsdAttribute const &shadingAttr,
    UsdShadeConnectableAPI *source, 
    TfToken *sourceName,
    UsdShadeAttributeType *sourceType)
{
    TRACE_SCOPE("UsdShadeConnectableAPI::GetConnectedSource");

    if (!(source && sourceName && sourceType)) {
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output-parameters.");
        return false;
    }
    
    *source = UsdShadeConnectableAPI();
    SdfPathVector sources;
    shadingAttr.GetConnections(&sources);

    // XXX(validation)  sources.size() <= 1, also sourceName,
    //                  target Material == source Material ?
    if (sources.size() == 1) {
        SdfPath const & path = sources[0];
        UsdObject target = shadingAttr.GetStage()->GetObjectAtPath(path);
        *source = UsdShadeConnectableAPI(target.GetPrim());

       if (path.IsPropertyPath()){
            TfToken const &attrName(path.GetNameToken());

            std::tie(*sourceName, *sourceType) = 
                UsdShadeUtils::GetBaseNameAndType(attrName);
            return target.Is<UsdAttribute>();
        }
    }

    return false;
}

/* static  */
bool 
UsdShadeConnectableAPI::GetRawConnectedSourcePaths(
    UsdAttribute const &shadingAttr, 
    SdfPathVector *sourcePaths)
{
    if (!shadingAttr.GetConnections(sourcePaths)) {
        TF_WARN("Unable to get connections for shading attribute <%s>", 
                shadingAttr.GetPath().GetText());
        return false;
    }
    
    return true;
}

/* static */
bool 
UsdShadeConnectableAPI::HasConnectedSource(const UsdAttribute &shadingAttr)
{
    // This MUST have the same semantics as GetConnectedSource().
    // XXX someday we might make this more efficient through careful
    // refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken        sourceName;
    UsdShadeAttributeType sourceType;
    return UsdShadeConnectableAPI::GetConnectedSource(shadingAttr, 
        &source, &sourceName, &sourceType);
}

// This tests if a given node represents a "live" base material,
// i.e. once that hasn't been "flattened out" due to being
// pulled across a reference to a library.
static bool
_NodeRepresentsLiveBaseMaterial(const PcpNodeRef &node)
{
    bool isLiveBaseMaterial = false;
    for (PcpNodeRef n = node; 
            n; // 0, or false, means we are at the root node
            n = n.GetOriginNode()) {
        switch(n.GetArcType()) {
        case PcpArcTypeSpecialize:
            isLiveBaseMaterial = true;
            break;
        // dakrunch: specializes across references are actually still valid.
        // 
        // case PcpArcTypeReference:
        //     if (isLiveBaseMaterial) {
        //         // Node is within a base material, but that is in turn
        //         // across a reference. That means this is a library
        //         // material, so it is not live and we should flatten it
        //         // out.  Continue iterating, however, since this
        //         // might be referenced into some other live base material
        //         // downstream.
        //         isLiveBaseMaterial = false;
        //     }
        //     break;
        default:
            break;
        }
    }
    return isLiveBaseMaterial;
}

/* static */
bool 
UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial(
    const UsdAttribute &shadingAttr)
{
    // USD core doesn't provide a UsdResolveInfo style API for asking where
    // connections are authored, so we do it here ourselves.
    // Find the strongest opinion about connections.
    SdfAttributeSpecHandle strongestAttrSpecWithConnections;
    SdfPropertySpecHandleVector propStack = shadingAttr.GetPropertyStack();
    for (const SdfPropertySpecHandle &prop: propStack) {
        if (SdfAttributeSpecHandle attrSpec =
            TfDynamic_cast<SdfAttributeSpecHandle>(prop)) {
            if (attrSpec->HasConnectionPaths()) {
                strongestAttrSpecWithConnections = attrSpec;
                break;
            }
        }
    }

    // Find which prim node introduced that opinion.
    if (strongestAttrSpecWithConnections) {
        for(const PcpNodeRef &node:
            shadingAttr.GetPrim().GetPrimIndex().GetNodeRange()) {
            if (node.GetPath() == strongestAttrSpecWithConnections->
                    GetPath().GetPrimPath() 
                &&
                node.GetLayerStack()->HasLayer(
                    strongestAttrSpecWithConnections->GetLayer())) 
            {
                return _NodeRepresentsLiveBaseMaterial(node);
            }
        }
    }

    return false;
}

/* static */
bool 
UsdShadeConnectableAPI::DisconnectSource(UsdAttribute const &shadingAttr)
{
    return shadingAttr.BlockConnections();
}

/* static */
bool 
UsdShadeConnectableAPI::ClearSource(UsdAttribute const &shadingAttr)
{
    return shadingAttr.ClearConnections();
}

UsdShadeOutput 
UsdShadeConnectableAPI::CreateOutput(const TfToken& name,
                                     const SdfValueTypeName& typeName) const
{
    return UsdShadeOutput(GetPrim(), name, typeName);
}

UsdShadeOutput 
UsdShadeConnectableAPI::GetOutput(const TfToken &name) const
{
    TfToken outputAttrName(UsdShadeTokens->outputs.GetString() + 
                           name.GetString());
    if (GetPrim().HasAttribute(outputAttrName)) {
        return UsdShadeOutput(GetPrim().GetAttribute(outputAttrName));
    } 
 
    return UsdShadeOutput();
}

std::vector<UsdShadeOutput> 
UsdShadeConnectableAPI::GetOutputs() const
{
    std::vector<UsdShadeOutput> ret;

    std::vector<UsdAttribute> attrs = GetPrim().GetAttributes();
    TF_FOR_ALL(attrIter, attrs) { 
        const UsdAttribute& attr = *attrIter;
        // If the attribute is in the "outputs:" namespace, then
        // it must be a valid UsdShadeOutput.
        if (TfStringStartsWith(attr.GetName().GetString(), 
                               UsdShadeTokens->outputs)) {
            ret.push_back(UsdShadeOutput(attr));
        }
    }

    return ret;
}

UsdShadeInput 
UsdShadeConnectableAPI::CreateInput(const TfToken& name,
                                    const SdfValueTypeName& typeName) const
{
    return UsdShadeInput(GetPrim(), name, typeName);
}

UsdShadeInput 
UsdShadeConnectableAPI::GetInput(const TfToken &name) const
{
    TfToken inputAttrName(UsdShadeTokens->inputs.GetString() + 
                          name.GetString());

    if (GetPrim().HasAttribute(inputAttrName)) {
        return UsdShadeInput(GetPrim().GetAttribute(inputAttrName));
    }

    return UsdShadeInput();
}

std::vector<UsdShadeInput> 
UsdShadeConnectableAPI::GetInputs() const
{
    std::vector<UsdShadeInput> ret;

    std::vector<UsdAttribute> attrs = GetPrim().GetAttributes();
    TF_FOR_ALL(attrIter, attrs) { 
        const UsdAttribute& attr = *attrIter;
        // If the attribute is in the "inputs:" namespace, then
        // it must be a valid UsdShadeInput.
        if (TfStringStartsWith(attr.GetName().GetString(), 
                               UsdShadeTokens->inputs)) {
            ret.push_back(UsdShadeInput(attr));
            continue;
        }
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
