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

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeConnectableAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

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
        UsdSchemaBase::GetSchemaAttributeNames(true);

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

#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/usdShade/tokens.h"

#include "debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USD_SHADE_BACK_COMPAT, true,
    "Set to false to terminate support for older encodings of the UsdShading model.");

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
UsdShadeConnectableAPI::_IsCompatible(const UsdPrim &prim) const
{
    return IsShader() or IsNodeGraph();
}

static TfToken 
_GetConnectionRelName(const TfToken &attrName)
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() + 
                   attrName.GetString());
}

static
UsdRelationship 
_GetConnectionRel(
    const UsdProperty &shadingProp, 
    bool create)
{
    // If it's already a relationship, simply return it as-is.
    if (UsdRelationship rel = shadingProp.As<UsdRelationship>())
        return rel;

    const UsdPrim& prim = shadingProp.GetPrim();

    // If it's an attribute, return the associated connectedSourceFor 
    // relationship.
    UsdAttribute shadingAttr = shadingProp.As<UsdAttribute>();
    if (shadingAttr) {    
        const TfToken& relName = _GetConnectionRelName(shadingAttr.GetName());
        if (UsdRelationship rel = prim.GetRelationship(relName)) {
            return rel;
        }
        else if (create) {
            return prim.CreateRelationship(relName, /* custom = */ false);
        }
    }
    
    return UsdRelationship();
}

static 
TfToken
_GetPropertyName(const TfToken &sourceName, 
                 const UsdShadeAttributeType sourceType)
{
    return TfToken(UsdShadeUtils::GetPrefixForAttributeType(sourceType) + 
                   sourceName.GetString());
}

/* static */
bool  
UsdShadeConnectableAPI::ConnectToSource(
    UsdProperty const &shadingProp,
    UsdShadeConnectableAPI const &source, 
    TfToken const &sourceName, 
    UsdShadeAttributeType const sourceType,
    SdfValueTypeName typeName)
{
    UsdShadeAttributeType shadingPropType = 
        UsdShadeUtils::GetBaseNameAndType(shadingProp.GetName()).second;
    if (shadingPropType == UsdShadeAttributeType::Output) {
        // Only outputs belonging to node-graphs are connectable. We don't allow 
        // connecting outputs of shaders as it's not meaningful.
        // 
        // Note: this warning will not be issued if the prim is untyped or 
        // if the type is unknown.
        if (UsdShadeConnectableAPI(shadingProp.GetPrim()).IsShader()) {
            TF_WARN("Attempted to connect an output of a shader <%s> to <%s>.",
                shadingProp.GetPath().GetText(), 
                source.GetPath().AppendProperty(
                    _GetPropertyName(sourceName, sourceType)).GetText());
            return false;
        }

        // Ensure that the source prim is a descendent of the node-graph owning 
        // the output.
        const SdfPath sourcePrimPath = source.GetPrim().GetPath();
        const SdfPath outputOwnerPath = shadingProp.GetPrim().GetPath();
        if (not sourcePrimPath.HasPrefix(outputOwnerPath)) {
            TF_WARN("Source of output '%s' on node-graph at path <%s> is outside "
                "the node-graph: <%s>", sourceName.GetText(), 
                outputOwnerPath.GetText(), sourcePrimPath.GetText());
            // Issue a warning, but allow this connnection for now.
        }
    }

    UsdPrim sourcePrim = source.GetPrim();
    bool  success = true;
    
    UsdRelationship rel = _GetConnectionRel(shadingProp, /* create */ true);

    // XXX it WBN to be able to validate source itself, guaranteeing
    // that the source is, in fact connectable (i.e., a shader or node-graph).
    // However, it remains useful to be able to target a pure-over.
    if (rel && sourcePrim) {
        std::string prefix = UsdShadeUtils::GetPrefixForAttributeType(
            sourceType);
        TfToken sourceAttrName(prefix + sourceName.GetString());

        UsdAttribute sourceAttr = sourcePrim.GetAttribute(sourceAttrName);

        if (not sourceAttr) {
            // If the sourcePrim contains a relationship with the source 
            // name, then it must be a terminal output on a node-graph and 
            // cannot be connected to.
            if (sourcePrim.GetRelationship(sourceAttrName)) {
                TF_CODING_ERROR("Cannot connect shading property <%s>'s source"
                    "to existing relationship '%s' on source prim <%s>..",
                    shadingProp.GetName().GetText(),
                    sourceAttrName.GetText(), 
                    sourcePrim.GetPath().GetText());
                return false;
            }
        }

        // If a typeName isn't specified, 
        if (not typeName) {
            // If shadingProp is not an attribute, it must be a terminal output 
            // on a node-graph. Hence wrapping shadingProp in a UsdShadeOutput 
            // and asking for its typeName should give us the desired answer.
            typeName = UsdShadeOutput(shadingProp).GetTypeName();
        }

        // First make sure there is a source attribute of the proper type
        // on the sourcePrim.
        if (sourceAttr){
            const SdfValueTypeName sourceType = sourceAttr.GetTypeName();
            const SdfValueTypeName sinkType   = typeName;
            // Comparing the TfType allows us to connect parameters with 
            // different "roles" of the same underlying type, 
            // e.g. float3 and color3f
            if (sourceType.GetType() != sinkType.GetType()) {
                TF_DEBUG(KATANA_USDBAKE_CONNECTIONS).Msg(
                        "Connecting parameter <%s> of type %s to source <%s>, "
                        "of potentially incompatible type %s. \n",
                        shadingProp.GetPath().GetText(),
                        sinkType.GetAsToken().GetText(),
                        sourceAttr.GetPath().GetText(),
                        sourceType.GetAsToken().GetText());
            }
        } else {
            sourceAttr = sourcePrim.CreateAttribute(sourceAttrName, typeName,
                /* custom = */ false);
        }
        SdfPathVector  target(1, sourceAttr.GetPath());
        success = rel.SetTargets(target);
    }
    else if (!source){
        TF_CODING_ERROR("Failed connecting shading property <%s>. "
                        "The given source shader prim <%s> is not defined", 
                        shadingProp.GetPath().GetText(),
                        sourcePrim ? sourcePrim.GetPath().GetText() :
                        "invalid-prim");
        return false;
    }
    else if (!rel){
        TF_CODING_ERROR("Failed connecting shading property <%s>. "
                        "Unable to make the connection to source <%s>.", 
                        shadingProp.GetPath().GetText(),
                        sourcePrim.GetPath().GetText());
        return false;
    }

    return success;
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdProperty const &shadingProp,
    SdfPath const &sourcePath)
{
    // sourcePath needs to be a property path for us to make a connection.
    if (not sourcePath.IsPropertyPath())
        return false;

    UsdPrim sourcePrim = shadingProp.GetStage()->GetPrimAtPath(
        sourcePath.GetPrimPath());
    UsdShadeConnectableAPI source(sourcePrim);
    // We don't validate UsdShadeConnectableAPI, as the type of the source prim 
    // may be unknown. (i.e. it could be a pure over or a typeless def).

    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtils::GetBaseNameAndType(
        sourcePath.GetNameToken());

    // If shadingProp is not an attribute, it must be a terminal output on a
    // node-graph. Hence wrapping shadingProp in a UsdShadeOutput and asking for 
    // its typeName should give us the desired answer.
    SdfValueTypeName typeName = UsdShadeOutput(shadingProp).GetTypeName();
    return ConnectToSource(shadingProp, source, sourceName, sourceType, 
        typeName);
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdProperty const &shadingProp, 
    UsdShadeInput const &sourceInput)
{
    // An interface attribute may be wrapped in the UsdShadeInput, hence get the 
    // 
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    std::tie(sourceName, sourceType) = UsdShadeUtils::GetBaseNameAndType(
        sourceInput.GetFullName());

    return UsdShadeConnectableAPI::ConnectToSource(shadingProp, 
        UsdShadeConnectableAPI(sourceInput.GetPrim()),
        sourceName, sourceType, 
        sourceInput.GetTypeName());
}

/* static */
bool 
UsdShadeConnectableAPI::ConnectToSource(
    UsdProperty const &shadingProp, 
    UsdShadeOutput const &sourceOutput)
{
    if (sourceOutput.IsTerminal()) {
        TF_CODING_ERROR("Cannot connect shading property <%s>'s source to "
            "terminal output '%s'.", shadingProp.GetName().GetText(),
            sourceOutput.GetFullName().GetText());
        return false;
    }

    return UsdShadeConnectableAPI::ConnectToSource(shadingProp, 
        UsdShadeConnectableAPI(sourceOutput.GetPrim()),
        sourceOutput.GetBaseName(), UsdShadeAttributeType::Output,
        sourceOutput.GetTypeName());
}

/* static */
bool
UsdShadeConnectableAPI::GetConnectedSource(
    UsdProperty const &shadingProp,
    UsdShadeConnectableAPI *source, 
    TfToken *sourceName,
    UsdShadeAttributeType *sourceType)
{
    if (not (source and sourceName and sourceType)) {
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output-parameters.");
        return false;
    }
    
    UsdRelationship connection = _GetConnectionRel(shadingProp, false);

    *source = UsdShadeConnectableAPI();
    SdfPathVector targets;
    // There should be no possibility of forwarding, here, since the API
    // only allows targetting prims
    if (connection) {
        connection.GetTargets(&targets);
    }

    // XXX(validation)  targets.size() <= 1, also sourceName
    if (targets.size() == 1) {
        SdfPath const & path = targets[0];
        *source = UsdShadeConnectableAPI::Get(connection.GetStage(), 
                                              path.GetPrimPath());
        if (path.IsPropertyPath()){
            TfToken const &attrName(path.GetNameToken());

            std::tie(*sourceName, *sourceType) = 
                UsdShadeUtils::GetBaseNameAndType(attrName);
        } else {
            // XXX validation error
            if ( TfGetEnvSetting(USD_SHADE_BACK_COMPAT) ) {
                return connection.GetMetadata(_tokens->outputName, sourceName)
                    && *source;
            }
        }
    }
    return *source;
}

/* static */
bool 
UsdShadeConnectableAPI::HasConnectedSource(const UsdProperty &shadingProp)
{
    // This MUST have the same semantics as GetConnectedSource().
    // XXX someday we might make this more efficient through careful
    // refactoring, but safest to just call the exact same code.
    UsdShadeConnectableAPI source;
    TfToken        sourceName;
    UsdShadeAttributeType sourceType;
    return UsdShadeConnectableAPI::GetConnectedSource(shadingProp, 
        &source, &sourceName, &sourceType);
}


/* static */
bool 
UsdShadeConnectableAPI::DisconnectSource(UsdProperty const &shadingProp)
{
    bool success = true;
    if (UsdRelationship rel = _GetConnectionRel(shadingProp, false)) {
        success = rel.BlockTargets();
    }
    return success;
}

/* static */
bool 
UsdShadeConnectableAPI::ClearSource(UsdProperty const &shadingProp)
{
    bool success = true;
    if (UsdRelationship rel = _GetConnectionRel(shadingProp, false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
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
 
    if (UsdShadeUtils::ReadOldEncoding()) {
        if (IsNodeGraph()) {
            if (GetPrim().HasRelationship(name)) {
                return UsdShadeOutput(GetPrim().GetRelationship(name));
            }
        }
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

    if (UsdShadeUtils::ReadOldEncoding() and IsNodeGraph()) {
        std::vector<UsdRelationship> rels= GetPrim().GetRelationships();
        TF_FOR_ALL(relIter, rels) { 
            const UsdRelationship& rel= *relIter;
            // Excluded the "connectedSourceFor:" and "interfaceRecipientsOf:" 
            // relationships.
            if (not TfStringStartsWith(rel.GetName(), 
                                       UsdShadeTokens->connectedSourceFor) and
                not TfStringStartsWith(rel.GetName(), 
                                       UsdShadeTokens->interfaceRecipientsOf)) 
            {
                // All non-connection related relationships on node-graphs
                // typically represent terminal outputs, so wrap the 
                // relationship in a UsdShadeOutput object and add to the 
                // resuls.
                ret.push_back(UsdShadeOutput(rel));
            }
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

    if (UsdShadeUtils::ReadOldEncoding()) {
        if (IsNodeGraph()) {
            TfToken interfaceAttrName = TfToken(
                UsdShadeTokens->interface.GetString() + name.GetString());
            if (GetPrim().HasAttribute(interfaceAttrName)) {
                return UsdShadeInput(GetPrim().GetAttribute(interfaceAttrName));
            }
        }

        if (IsShader()) {
            if (GetPrim().HasAttribute(name)) {
                return UsdShadeInput(GetPrim().GetAttribute(name));
            }
        }
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

        // Support for old style encoding containing interface attributes 
        // and parameters.
        if (UsdShadeUtils::ReadOldEncoding()) {
            if (IsNodeGraph() and 
                TfStringStartsWith(attr.GetName().GetString(), 
                                   UsdShadeTokens->interface)) {                    
                // If it's an interface attribute on a node-graph, wrap it in a 
                // UsdShadeInput object and add it to the list of inputs.
                ret.push_back(UsdShadeInput(attr));
            } else if (IsShader() and attr.GetNamespace().IsEmpty()) {
                // If it's an unnamespaced (parameter) attribute on a shader, 
                // wrap it in a UsdShadeInput object and add it to the list of 
                // inputs.
                ret.push_back(UsdShadeInput(attr));
            }
        }
    }

    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
