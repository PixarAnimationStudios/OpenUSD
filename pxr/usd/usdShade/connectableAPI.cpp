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
UsdSchemaKind UsdShadeConnectableAPI::_GetSchemaKind() const {
    return UsdShadeConnectableAPI::schemaKind;
}

/* virtual */
UsdSchemaKind UsdShadeConnectableAPI::_GetSchemaType() const {
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
#include "pxr/base/tf/envSetting.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (outputName)
    (outputs)
);

static UsdAttribute
_GetOrCreateSourceAttr(UsdShadeConnectionSourceInfo const &sourceInfo,
                       SdfValueTypeName fallbackTypeName)
{
    // Note, the validity of sourceInfo has been checked in ConnectToSource and
    // SetConnectedSources, which includes a check of source, sourceType and
    // sourceName
    UsdPrim sourcePrim = sourceInfo.source.GetPrim();

    std::string prefix = UsdShadeUtils::GetPrefixForAttributeType(
            sourceInfo.sourceType);
    TfToken sourceAttrName(prefix + sourceInfo.sourceName.GetString());

    UsdAttribute sourceAttr = sourcePrim.GetAttribute(sourceAttrName);

    // If a source attribute doesn't exist on the sourcePrim we create one with
    // the proper type
    if (!sourceAttr) {
        sourceAttr = sourcePrim.CreateAttribute(sourceAttrName,
                                                // If typeName isn't valid use
                                                // the fallback
                                                sourceInfo.typeName
                                                ? sourceInfo.typeName
                                                : fallbackTypeName,
                                                /* custom = */ false);
    }

    return sourceAttr;
}

/* static */
bool
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    UsdShadeConnectionSourceInfo const &source,
    ConnectionModification const mod)
{
    if (!source) {
        TF_CODING_ERROR("Failed connecting shading attribute <%s> to "
                        "attribute %s%s on prim %s. The given source "
                        "information is not valid",
                        shadingAttr.GetPath().GetText(),
                        UsdShadeUtils::GetPrefixForAttributeType(
                                            source.sourceType).c_str(),
                        source.sourceName.GetText(),
                        source.source.GetPath().GetText());
        return false;
    }

    UsdAttribute sourceAttr =
                _GetOrCreateSourceAttr(source, shadingAttr.GetTypeName());
    if (!sourceAttr) {
        // _GetOrCreateSourceAttr can only fail if CreateAttribute fails, which
        // will issue an appropriate error
        return false;
    }

    if (mod == ConnectionModification::Replace) {
        return shadingAttr.SetConnections(
            SdfPathVector{sourceAttr.GetPath()});
    } else if (mod == ConnectionModification::Prepend) {
        return shadingAttr.AddConnection(sourceAttr.GetPath(),
                                    UsdListPositionFrontOfPrependList);
    } else if (mod == ConnectionModification::Append) {
        return shadingAttr.AddConnection(sourceAttr.GetPath(),
                                    UsdListPositionBackOfAppendList);
    }

    return false;
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
    return ConnectToSource(shadingAttr,
        UsdShadeConnectionSourceInfo(source, sourceName, sourceType, typeName));
}

/* static */
bool
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    SdfPath const &sourcePath)
{
    return ConnectToSource(shadingAttr,
        UsdShadeConnectionSourceInfo(shadingAttr.GetStage(), sourcePath));
}

/* static */
bool
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    UsdShadeInput const &sourceInput)
{
    return ConnectToSource(
        shadingAttr,
        UsdShadeConnectableAPI(sourceInput.GetPrim()),
        sourceInput.GetBaseName(),
        UsdShadeAttributeType::Input,
        sourceInput.GetTypeName());
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdAttribute const &shadingAttr,
    UsdShadeOutput const &sourceOutput)
{
    return ConnectToSource(
        shadingAttr,
        UsdShadeConnectableAPI(sourceOutput.GetPrim()),
        sourceOutput.GetBaseName(),
        UsdShadeAttributeType::Output,
        sourceOutput.GetTypeName());
}

/* static */
bool
UsdShadeConnectableAPI::SetConnectedSources(
    UsdAttribute const &shadingAttr,
    std::vector<UsdShadeConnectionSourceInfo> const &sourceInfos)
{
    SdfPathVector sourcePaths;
    sourcePaths.reserve(sourceInfos.size());

    for (UsdShadeConnectionSourceInfo const& sourceInfo : sourceInfos) {
        if (!sourceInfo) {
            TF_CODING_ERROR("Failed connecting shading attribute <%s> to "
                            "attribute %s%s on prim %s. The given information "
                            "in `sourceInfos` in is not valid",
                            shadingAttr.GetPath().GetText(),
                            UsdShadeUtils::GetPrefixForAttributeType(
                                                sourceInfo.sourceType).c_str(),
                            sourceInfo.sourceName.GetText(),
                            sourceInfo.source.GetPath().GetText());
            return false;
        }

        UsdAttribute sourceAttr =
                _GetOrCreateSourceAttr(sourceInfo, shadingAttr.GetTypeName());
        if (!sourceAttr) {
            // _GetOrCreateSourceAttr can only fail if CreateAttribute fails,
            // which will issue an appropriate error
            return false;
        }

        sourcePaths.push_back(sourceAttr.GetPath());
    }

    return shadingAttr.SetConnections(sourcePaths);
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

    UsdShadeSourceInfoVector sourceInfos = GetConnectedSources(shadingAttr);
    if (sourceInfos.empty()) {
        return false;
    }

    if (sourceInfos.size() > 1u) {
        TF_WARN("More than one connection for shading attribute %s. "
                "GetConnectedSource will only report the first one. "
                "Please use GetConnectedSources to retrieve all.",
                shadingAttr.GetPath().GetText());
    }

    UsdShadeConnectionSourceInfo const &sourceInfo = sourceInfos[0];

    *source = sourceInfo.source;
    *sourceName = sourceInfo.sourceName;
    *sourceType = sourceInfo.sourceType;

    return true;
}

/* static */
UsdShadeSourceInfoVector
UsdShadeConnectableAPI::GetConnectedSources(UsdAttribute const &shadingAttr,
                                            SdfPathVector *invalidSourcePaths)
{
    TRACE_SCOPE("UsdShadeConnectableAPI::GetConnectedSources");

    SdfPathVector sourcePaths;
    shadingAttr.GetConnections(&sourcePaths);

    UsdShadeSourceInfoVector sourceInfos;
    if (sourcePaths.empty()) {
        return sourceInfos;
    }

    UsdStagePtr stage = shadingAttr.GetStage();

    sourceInfos.reserve(sourcePaths.size());
    for (SdfPath const &sourcePath : sourcePaths) {

        // Make sure the source attribute exists
        UsdAttribute sourceAttr = stage->GetAttributeAtPath(sourcePath);
        if (!sourceAttr) {
            if (invalidSourcePaths) {
                invalidSourcePaths->push_back(sourcePath);
            }
            continue;
        }

        // Check that the attribute has a legal prefix
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        std::tie(sourceName, sourceType) =
            UsdShadeUtils::GetBaseNameAndType(sourcePath.GetNameToken());
        if (sourceType == UsdShadeAttributeType::Invalid) {
            if (invalidSourcePaths) {
                invalidSourcePaths->push_back(sourcePath);
            }
            continue;
        }

        // We do not check whether the UsdShadeConnectableAPI is valid. We
        // implicitly know the prim is valid, since we got a valid attribute.
        // That is the only requirement.
        UsdShadeConnectableAPI source(sourceAttr.GetPrim());

        sourceInfos.emplace_back(source, sourceName, sourceType,
                                 sourceAttr.GetTypeName());
    }

    return sourceInfos;
}

// N.B. The implementation of these static methods is in the cpp file, since the
// UsdShadeSourceInfoVector type is not fully defined at the corresponding point
// in the header.

/* static */
UsdShadeSourceInfoVector
UsdShadeConnectableAPI::GetConnectedSources(
    UsdShadeInput const &input,
    SdfPathVector *invalidSourcePaths)
{
    return GetConnectedSources(input.GetAttr(), invalidSourcePaths);
}

/* static */
UsdShadeSourceInfoVector
UsdShadeConnectableAPI::GetConnectedSources(
    UsdShadeOutput const &output,
    SdfPathVector *invalidSourcePaths)
{
    return GetConnectedSources(output.GetAttr(), invalidSourcePaths);
}

/* static  */
bool
UsdShadeConnectableAPI::GetRawConnectedSourcePaths(
    UsdAttribute const &shadingAttr,
    SdfPathVector *sourcePaths)
{
    return shadingAttr.GetConnections(sourcePaths);
}

/* static */
bool 
UsdShadeConnectableAPI::HasConnectedSource(const UsdAttribute &shadingAttr)
{
    // This MUST have the same semantics as GetConnectedSources().
    // XXX someday we might make this more efficient through careful
    // refactoring, but safest to just call the exact same code.
    return !GetConnectedSources(shadingAttr).empty();
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
UsdShadeConnectableAPI::DisconnectSource(UsdAttribute const &shadingAttr,
                                         UsdAttribute const &sourceAttr)
{
    if (sourceAttr) {
        return shadingAttr.RemoveConnection(sourceAttr.GetPath());
    } else {
        return shadingAttr.SetConnections({});
    }
}

/* static */
bool 
UsdShadeConnectableAPI::ClearSources(UsdAttribute const &shadingAttr)
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
UsdShadeConnectableAPI::GetOutputs(bool onlyAuthored) const
{
    std::vector<UsdProperty> props;
    if (onlyAuthored) {
        props = GetPrim().GetAuthoredPropertiesInNamespace(
            UsdShadeTokens->outputs);
    } else {
        props = GetPrim().GetPropertiesInNamespace(UsdShadeTokens->outputs);
    }

    // Filter for attributes and convert them to ouputs
    std::vector<UsdShadeOutput> outputs;
    outputs.reserve(props.size());
    for (UsdProperty const& prop: props) {
        if (UsdAttribute attr = prop.As<UsdAttribute>()) {
            outputs.push_back(UsdShadeOutput(attr));
        }
    }
    return outputs;
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
UsdShadeConnectableAPI::GetInputs(bool onlyAuthored) const
{
    std::vector<UsdProperty> props;
    if (onlyAuthored) {
        props = GetPrim().GetAuthoredPropertiesInNamespace(
            UsdShadeTokens->inputs);
    } else {
        props = GetPrim().GetPropertiesInNamespace(UsdShadeTokens->inputs);
    }

    // Filter for attributes and convert them to inputs
    std::vector<UsdShadeInput> inputs;
    inputs.reserve(props.size());
    for (UsdProperty const& prop: props) {
        if (UsdAttribute attr = prop.As<UsdAttribute>()) {
            inputs.push_back(UsdShadeInput(attr));
        }
    }
    return inputs;
}

UsdShadeConnectionSourceInfo::UsdShadeConnectionSourceInfo(
    UsdStagePtr const& stage,
    SdfPath const& sourcePath)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return;
    }

    if (!sourcePath.IsPropertyPath()) {
        return;
    }

    std::tie(sourceName, sourceType) =
        UsdShadeUtils::GetBaseNameAndType(sourcePath.GetNameToken());

    // Check if the prim can be found on the stage and is a
    // UsdShadeConnectableAPI compatible prim
    source = UsdShadeConnectableAPI::Get(stage, sourcePath.GetPrimPath());

    // Note, initialization of typeName is optional, since the target attribute
    // might not exist (yet)
    // XXX try to get attribute from source.GetPrim()?
    UsdAttribute sourceAttr = stage->GetAttributeAtPath(sourcePath);
    if (sourceAttr) {
        typeName = sourceAttr.GetTypeName();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
