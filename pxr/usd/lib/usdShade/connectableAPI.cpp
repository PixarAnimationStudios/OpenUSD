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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/usdShade/tokens.h"

#include "debugCodes.h"

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
UsdShadeConnectableAPI::IsSubgraph() const
{
    return GetPrim().IsA<UsdShadeSubgraph>();
}

/* virtual */
bool 
UsdShadeConnectableAPI::_IsCompatible(const UsdPrim &prim) const
{
    return IsShader() or IsSubgraph();
}

static TfToken 
_GetConnectionRelName(const TfToken &attrName)
{
    return TfToken(UsdShadeTokens->connectedSourceFor.GetString() + 
                   attrName.GetString());
}

UsdRelationship 
UsdShadeConnectableAPI::GetConnectionRel(
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

/* static */
bool  
UsdShadeConnectableAPI::MakeConnection(
    UsdProperty const &shadingProp,
    UsdShadeConnectableAPI const &source, 
    TfToken const &sourceName, 
    SdfValueTypeName typeName,
    UsdShadeAttributeType const sourceType)
{
    UsdPrim sourcePrim = source.GetPrim();
    bool  success = true;
    
    UsdRelationship rel = UsdShadeConnectableAPI::GetConnectionRel(shadingProp, 
        /* create */ true);

    // XXX it WBN to be able to validate source itself, guaranteeing
    // that the source is, in fact connectable (i.e., a shader or subgraph).
    // However, it remains useful to be able to target a pure-over.
    if (rel && sourcePrim) {
        std::string prefix = UsdShadeUtilsGetPrefixForAttributeType(
            sourceType);
        TfToken sourceAttrName(prefix + sourceName.GetString());

        UsdAttribute  sourceAttr = sourcePrim.GetAttribute(sourceAttrName);

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
UsdShadeConnectableAPI::EvaluateConnection(
    UsdProperty const &shadingProp,
    UsdShadeConnectableAPI *source, 
    TfToken *sourceName,
    UsdShadeAttributeType *sourceType)
{
    UsdRelationship connection = GetConnectionRel(shadingProp, false);

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
                UsdShadeUtilsGetBaseNameAndType(attrName);
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

UsdShadeOutput 
UsdShadeConnectableAPI::CreateOutput(const TfToken& name,
                                     const SdfValueTypeName& typeName) const
{
    return UsdShadeOutput(GetPrim(), name, typeName);
}

UsdShadeOutput 
UsdShadeConnectableAPI::GetOutput(const TfToken &name) const
{
    return UsdShadeOutput(GetPrim().GetAttribute(TfToken(
        UsdShadeTokens->outputs.GetString() + name.GetString())));
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
