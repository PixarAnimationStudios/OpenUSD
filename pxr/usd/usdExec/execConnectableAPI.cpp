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
#include "pxr/usd/usdExec/execConnectableAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdExecConnectableAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ExecConnectableAPI)
);

/* virtual */
UsdExecConnectableAPI::~UsdExecConnectableAPI()
{
}

/* static */
UsdExecConnectableAPI
UsdExecConnectableAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdExecConnectableAPI();
    }
    return UsdExecConnectableAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdExecConnectableAPI::_GetSchemaKind() const
{
    return UsdExecConnectableAPI::schemaKind;
}

/* static */
const TfType &
UsdExecConnectableAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdExecConnectableAPI>();
    return tfType;
}

/* static */
bool 
UsdExecConnectableAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdExecConnectableAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdExecConnectableAPI::GetSchemaAttributeNames(bool includeInherited)
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
#include "pxr/usd/usdExec/tokens.h"
#include "pxr/usd/usdExec/execUtils.h"
#include "pxr/usd/usdExec/execGraph.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (outputName)
    (outputs)
);

static UsdAttribute
_GetOrCreateSourceAttr(UsdExecConnectionSourceInfo const &sourceInfo,
                       SdfValueTypeName fallbackTypeName)
{
    // Note, the validity of sourceInfo has been checked in ConnectToSource and
    // SetConnectedSources, which includes a check of source, sourceType and
    // sourceName
    UsdPrim sourcePrim = sourceInfo.source.GetPrim();

    std::string prefix = UsdExecUtils::GetPrefixForAttributeType(
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

static bool _CanConnectInputToSource(
    const UsdExecInput &input,
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

    if (inputConnectability == UsdExecTokens->full) {
        if (UsdExecInput::IsInput(source)) {
            return true;
        }
        return false;
    } else if (inputConnectability == UsdExecTokens->interfaceOnly) {
        if (UsdExecInput::IsInput(source)) {
            TfToken sourceConnectability = 
                UsdExecInput(source).GetConnectability();
            if (sourceConnectability == UsdExecTokens->interfaceOnly) {
                return true;
            } else {
                if (reason) {
                    *reason = "Input connectability is 'interfaceOnly' and " \
                        "source does not have 'interfaceOnly' connectability.";
                }
                return false;
            }
        } else {
            if (reason) {
                *reason = "Input connectability is 'interfaceOnly' but " \
                    "source is not an input";
                return false;
            }
        }
    } else {
        if (reason) {
            *reason = "Input connectability is unspecified";
        }
        return false;
    }
    return false;
}

static bool _CanConnectOutputToSource(
    const UsdExecOutput &output,
    const UsdAttribute &source,
    std::string *reason)
{
    // Nodegraphs allow connections to their outputs, but only from
    // internal nodes.
    if (!output.IsDefined()) {
        if (reason) {
            *reason = TfStringPrintf("Invalid output");
        }
        return false;
    }
    if (!source) {
        if (reason) {
            *reason = TfStringPrintf("Invalid source");
        }
        return false;
    }

    const SdfPath sourcePrimPath = source.GetPrim().GetPath();
    const SdfPath outputPrimPath = output.GetPrim().GetPath();

    if (UsdExecInput::IsInput(source)) {
        // output can connect to an input of the same container as a
        // passthrough.
        if (sourcePrimPath != outputPrimPath) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - output "
                        "'%s' and input source '%s' must be encapsulated by "
                        "the same container prim", 
                        output.GetAttr().GetPath().GetText(),
                        source.GetPath().GetText());
            }
            return false;
        }
        return true;
    } else { // Source is an output
        // output can connect to other node's output directly encapsulated by
        // it, unless explicitly marked to ignore encapsulation rule.

        if (sourcePrimPath.GetParentPath() != outputPrimPath) {
            if (reason) {
                *reason = TfStringPrintf("Encapsulation check failed - prim "
                        "owning the output '%s' is not an immediate descendent "
                        " of the prim owning the output source '%s'.",
                        output.GetAttr().GetPath().GetText(),
                        source.GetPath().GetText());
            }
            return false;
        }

        return true;
    }
}

bool
UsdExecConnectableAPI::IsContainer() const
{
    if(GetPrim().IsA<UsdExecGraph>()) return true;
    return false;
}

bool
UsdExecConnectableAPI::CanConnect(
    const UsdExecInput &input, 
    const UsdAttribute &source)
{
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    std::string reason;
    return _CanConnectInputToSource(input, source, &reason);
}

bool
UsdExecConnectableAPI::CanConnect(
    const UsdExecOutput &output, 
    const UsdAttribute &source)
{
    // The reason why a connection can't be made isn't exposed currently.
    // We may want to expose it in the future, especially when we have 
    // validation in USD.
    std::string reason;
    return _CanConnectOutputToSource(output, source, &reason);
}

/* static */
bool
UsdExecConnectableAPI::ConnectToSource(
    UsdAttribute const &attr,
    UsdExecConnectionSourceInfo const &source,
    ConnectionModification const mod)
{
    if (!source) {
        TF_CODING_ERROR("Failed connecting shading attribute <%s> to "
                        "attribute %s%s on prim %s. The given source "
                        "information is not valid",
                        attr.GetPath().GetText(),
                        UsdExecUtils::GetPrefixForAttributeType(
                                            source.sourceType).c_str(),
                        source.sourceName.GetText(),
                        source.source.GetPath().GetText());
        return false;
    }

    UsdAttribute sourceAttr =
                _GetOrCreateSourceAttr(source, attr.GetTypeName());
    if (!sourceAttr) {
        // _GetOrCreateSourceAttr can only fail if CreateAttribute fails, which
        // will issue an appropriate error
        return false;
    }

    if (mod == ConnectionModification::Replace) {
        return attr.SetConnections(
            SdfPathVector{sourceAttr.GetPath()});
    } else if (mod == ConnectionModification::Prepend) {
        return attr.AddConnection(sourceAttr.GetPath(),
                                    UsdListPositionFrontOfPrependList);
    } else if (mod == ConnectionModification::Append) {
        return attr.AddConnection(sourceAttr.GetPath(),
                                    UsdListPositionBackOfAppendList);
    }

    return false;
}

/* static */
bool
UsdExecConnectableAPI::ConnectToSource(
    UsdAttribute const &attr,
    UsdExecConnectableAPI const &source,
    TfToken const &sourceName,
    UsdExecAttributeType const sourceType,
    SdfValueTypeName typeName)
{
    return ConnectToSource(attr,
        UsdExecConnectionSourceInfo(source, sourceName, sourceType, typeName));
}

/* static */
bool
UsdExecConnectableAPI::ConnectToSource(
    UsdAttribute const &execAttr,
    SdfPath const &sourcePath)
{
    return ConnectToSource(execAttr,
        UsdExecConnectionSourceInfo(execAttr.GetStage(), sourcePath));
}

/* static */
bool
UsdExecConnectableAPI::ConnectToSource(
    UsdAttribute const &execAttr,
    UsdExecInput const &sourceInput)
{
    return ConnectToSource(
        execAttr,
        UsdExecConnectableAPI(sourceInput.GetPrim()),
        sourceInput.GetBaseName(),
        UsdExecAttributeType::Input,
        sourceInput.GetTypeName());
}

/* static */
bool 
UsdExecConnectableAPI::ConnectToSource(
    UsdAttribute const &execAttr,
    UsdExecOutput const &sourceOutput)
{
    return ConnectToSource(
        execAttr,
        UsdExecConnectableAPI(sourceOutput.GetPrim()),
        sourceOutput.GetBaseName(),
        UsdExecAttributeType::Output,
        sourceOutput.GetTypeName());
}

/* static */
bool
UsdExecConnectableAPI::SetConnectedSources(
    UsdAttribute const &execAttr,
    std::vector<UsdExecConnectionSourceInfo> const &sourceInfos)
{
    SdfPathVector sourcePaths;
    sourcePaths.reserve(sourceInfos.size());

    for (UsdExecConnectionSourceInfo const& sourceInfo : sourceInfos) {
        if (!sourceInfo) {
            TF_CODING_ERROR("Failed connecting shading attribute <%s> to "
                            "attribute %s%s on prim %s. The given information "
                            "in `sourceInfos` in is not valid",
                            execAttr.GetPath().GetText(),
                            UsdExecUtils::GetPrefixForAttributeType(
                                                sourceInfo.sourceType).c_str(),
                            sourceInfo.sourceName.GetText(),
                            sourceInfo.source.GetPath().GetText());
            return false;
        }

        UsdAttribute sourceAttr =
                _GetOrCreateSourceAttr(sourceInfo, execAttr.GetTypeName());
        if (!sourceAttr) {
            // _GetOrCreateSourceAttr can only fail if CreateAttribute fails,
            // which will issue an appropriate error
            return false;
        }

        sourcePaths.push_back(sourceAttr.GetPath());
    }

    return execAttr.SetConnections(sourcePaths);
}

/* static */
bool
UsdExecConnectableAPI::GetConnectedSource(
    UsdAttribute const &execAttr,
    UsdExecConnectableAPI *source,
    TfToken *sourceName,
    UsdExecAttributeType *sourceType)
{
    TRACE_SCOPE("UsdExecConnectableAPI::GetConnectedSource");

    if (!(source && sourceName && sourceType)) {
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output-parameters.");
        return false;
    }

    UsdExecSourceInfoVector sourceInfos = GetConnectedSources(execAttr);
    if (sourceInfos.empty()) {
        return false;
    }

    if (sourceInfos.size() > 1u) {
        TF_WARN("More than one connection for shading attribute %s. "
                "GetConnectedSource will only report the first one. "
                "Please use GetConnectedSources to retrieve all.",
                execAttr.GetPath().GetText());
    }

    UsdExecConnectionSourceInfo const &sourceInfo = sourceInfos[0];

    *source = sourceInfo.source;
    *sourceName = sourceInfo.sourceName;
    *sourceType = sourceInfo.sourceType;

    return true;
}

/* static */
UsdExecSourceInfoVector
UsdExecConnectableAPI::GetConnectedSources(UsdAttribute const &execAttr,
                                            SdfPathVector *invalidSourcePaths)
{
    TRACE_SCOPE("UsdExecConnectableAPI::GetConnectedSources");

    SdfPathVector sourcePaths;
    execAttr.GetConnections(&sourcePaths);

    UsdExecSourceInfoVector sourceInfos;
    if (sourcePaths.empty()) {
        return sourceInfos;
    }

    UsdStagePtr stage = execAttr.GetStage();

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
        UsdExecAttributeType sourceType;
        std::tie(sourceName, sourceType) =
            UsdExecUtils::GetBaseNameAndType(sourcePath.GetNameToken());
        if (sourceType == UsdExecAttributeType::Invalid) {
            if (invalidSourcePaths) {
                invalidSourcePaths->push_back(sourcePath);
            }
            continue;
        }

        // We do not check whether the UsdExecConnectableAPI is valid. We
        // implicitly know the prim is valid, since we got a valid attribute.
        // That is the only requirement.
        UsdExecConnectableAPI source(sourceAttr.GetPrim());

        sourceInfos.emplace_back(source, sourceName, sourceType,
                                 sourceAttr.GetTypeName());
    }

    return sourceInfos;
}

// N.B. The implementation of these static methods is in the cpp file, since the
// UsdExecSourceInfoVector type is not fully defined at the corresponding point
// in the header.

/* static */
UsdExecSourceInfoVector
UsdExecConnectableAPI::GetConnectedSources(
    UsdExecInput const &input,
    SdfPathVector *invalidSourcePaths)
{
    return GetConnectedSources(input.GetAttr(), invalidSourcePaths);
}

/* static */
UsdExecSourceInfoVector
UsdExecConnectableAPI::GetConnectedSources(
    UsdExecOutput const &output,
    SdfPathVector *invalidSourcePaths)
{
    return GetConnectedSources(output.GetAttr(), invalidSourcePaths);
}

/* static  */
bool
UsdExecConnectableAPI::GetRawConnectedSourcePaths(
    UsdAttribute const &attr,
    SdfPathVector *sourcePaths)
{
    return attr.GetConnections(sourcePaths);
}

/* static */
bool 
UsdExecConnectableAPI::HasConnectedSource(const UsdAttribute &attr)
{
    // This MUST have the same semantics as GetConnectedSources().
    // XXX someday we might make this more efficient through careful
    // refactoring, but safest to just call the exact same code.
    return !GetConnectedSources(attr).empty();
}

/* static */
bool 
UsdExecConnectableAPI::DisconnectSource(UsdAttribute const &attr,
                                         UsdAttribute const &sourceAttr)
{
    if (sourceAttr) {
        return attr.RemoveConnection(sourceAttr.GetPath());
    } else {
        return attr.SetConnections({});
    }
}

/* static */
bool 
UsdExecConnectableAPI::ClearSources(UsdAttribute const &attr)
{
    return attr.ClearConnections();
}

UsdExecOutput 
UsdExecConnectableAPI::CreateOutput(const TfToken& name,
                                     const SdfValueTypeName& typeName) const
{
    return UsdExecOutput(GetPrim(), name, typeName);
}

UsdExecOutput 
UsdExecConnectableAPI::GetOutput(const TfToken &name) const
{
    TfToken outputAttrName(UsdExecTokens->outputs.GetString() + 
                           name.GetString());
    if (GetPrim().HasAttribute(outputAttrName)) {
        return UsdExecOutput(GetPrim().GetAttribute(outputAttrName));
    } 
 
    return UsdExecOutput();
}

std::vector<UsdExecOutput> 
UsdExecConnectableAPI::GetOutputs(bool onlyAuthored) const
{
    std::vector<UsdProperty> props;
    if (onlyAuthored) {
        props = GetPrim().GetAuthoredPropertiesInNamespace(
            UsdExecTokens->outputs);
    } else {
        props = GetPrim().GetPropertiesInNamespace(UsdExecTokens->outputs);
    }

    // Filter for attributes and convert them to ouputs
    std::vector<UsdExecOutput> outputs;
    outputs.reserve(props.size());
    for (UsdProperty const& prop: props) {
        if (UsdAttribute attr = prop.As<UsdAttribute>()) {
            outputs.push_back(UsdExecOutput(attr));
        }
    }
    return outputs;
}

UsdExecInput 
UsdExecConnectableAPI::CreateInput(const TfToken& name,
                                    const SdfValueTypeName& typeName) const
{
    return UsdExecInput(GetPrim(), name, typeName);
}

UsdExecInput 
UsdExecConnectableAPI::GetInput(const TfToken &name) const
{
    TfToken inputAttrName(UsdExecTokens->inputs.GetString() + 
                          name.GetString());

    if (GetPrim().HasAttribute(inputAttrName)) {
        return UsdExecInput(GetPrim().GetAttribute(inputAttrName));
    }

    return UsdExecInput();
}

std::vector<UsdExecInput> 
UsdExecConnectableAPI::GetInputs(bool onlyAuthored) const
{
    std::vector<UsdProperty> props;
    if (onlyAuthored) {
        props = GetPrim().GetAuthoredPropertiesInNamespace(
            UsdExecTokens->inputs);
    } else {
        props = GetPrim().GetPropertiesInNamespace(UsdExecTokens->inputs);
    }

    // Filter for attributes and convert them to inputs
    std::vector<UsdExecInput> inputs;
    inputs.reserve(props.size());
    for (UsdProperty const& prop: props) {
        if (UsdAttribute attr = prop.As<UsdAttribute>()) {
            inputs.push_back(UsdExecInput(attr));
        }
    }
    return inputs;
}

UsdExecConnectionSourceInfo::UsdExecConnectionSourceInfo(
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
        UsdExecUtils::GetBaseNameAndType(sourcePath.GetNameToken());

    // Check if the prim can be found on the stage and is a
    // UsdExecConnectableAPI compatible prim
    source = UsdExecConnectableAPI::Get(stage, sourcePath.GetPrimPath());

    // Note, initialization of typeName is optional, since the target attribute
    // might not exist (yet)
    // XXX try to get attribute from source.GetPrim()?
    UsdAttribute sourceAttr = stage->GetAttributeAtPath(sourcePath);
    if (sourceAttr) {
        typeName = sourceAttr.GetTypeName();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
