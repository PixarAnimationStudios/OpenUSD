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
#include "pxr/usd/usdShade/parameter.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdShade/shader.h"

#include <stdlib.h>
#include <algorithm>

#include "pxr/base/tf/envSetting.h"

#include "debugCodes.h"

TF_DEFINE_ENV_SETTING(
    USD_SHADE_BACK_COMPAT, true,
    "Set to false to terminate support for older encodings of the UsdShading model.");


using std::vector;
using std::string;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderType)
    ((connectedSourceFor, "connectedSourceFor:"))
    (outputName)
    ((outputs, "outputs:"))
);

static UsdRelationship
_GetParameterConnection(
        const UsdShadeParameter &param,
        bool create)
{
    const UsdAttribute& attr = param.GetAttr();
    const UsdPrim& prim = attr.GetPrim();
    const TfToken& relName = param.GetConnectionRelName();
    if (UsdRelationship rel = prim.GetRelationship(relName)) {
        return rel;
    }

    if (create) {
        return prim.CreateRelationship(relName, /* custom = */ false);
    }
    else {
        return UsdRelationship();
    }
}

struct _PropertyLessThan {
    bool operator()(UsdProperty const &p1, UsdProperty const &p2){
        return TfDictionaryLessThan()(p1.GetName(), p2.GetName());
    }
};

UsdShadeParameter::UsdShadeParameter(
        const UsdAttribute &attr)
    :
        _attr(attr)
{
}

UsdShadeParameter::UsdShadeParameter(
        UsdPrim prim,
        TfToken const &name,
        SdfValueTypeName const &typeName)
{
    // XXX what do we do if the type name doesn't match and it exists already?

    _attr = prim.GetAttribute(name);
    if (!_attr) {
        _attr = prim.CreateAttribute(name, typeName, /* custom = */ false);
    }
}

bool
UsdShadeParameter::Set(
        const VtValue& value,
        UsdTimeCode time) const
{
    return _attr.Set(value, time);
}

bool 
UsdShadeParameter::SetRenderType(
        TfToken const& renderType) const
{
    return _attr.SetMetadata(_tokens->renderType, renderType);
}

TfToken 
UsdShadeParameter::GetRenderType() const
{
    TfToken renderType;
    _attr.GetMetadata(_tokens->renderType, &renderType);
    return renderType;
}

bool 
UsdShadeParameter::HasRenderType() const
{
    return _attr.HasMetadata(_tokens->renderType);
}

bool 
UsdShadeParameter::_Connect(
        UsdRelationship const &rel,
        UsdShadeShader const &sourceShader, 
        TfToken const &outputName, 
        bool outputIsParameter) const
{

    UsdPrim sourcePrim = sourceShader.GetPrim();
    bool  success = true;
    
    // XXX it WBN to be able to validate source itself, guaranteeing
    // that the source is, in fact, a shader.  However, it remains useful to
    // be able to target a pure-over.
    if (rel && sourcePrim) {
        TfToken sourceName = outputIsParameter ? outputName :
            TfToken(_tokens->outputs.GetString() + outputName.GetString());
        UsdAttribute  sourceAttr = sourcePrim.GetAttribute(sourceName);

        // First make sure there is a source attribute of the proper type
        // on the sourcePrim.
        if (sourceAttr){
            const SdfValueTypeName sourceType = sourceAttr.GetTypeName();
            const SdfValueTypeName sinkType   = _attr.GetTypeName();
            // Comparing the TfType allows us to connect parameters with 
            // different "roles" of the same underlying type, 
            // e.g. float3 and color3f
            if (sourceType.GetType() != sinkType.GetType()) {
                TF_DEBUG(KATANA_USDBAKE_CONNECTIONS).Msg(
                        "Connecting parameter <%s> of type %s to source <%s>, "
                        "of potentially incompatible type %s. \n",
                        _attr.GetPath().GetText(),
                        sinkType.GetAsToken().GetText(),
                        sourceAttr.GetPath().GetText(),
                        sourceType.GetAsToken().GetText());
            }
        } else {
            sourceAttr =
                sourcePrim.CreateAttribute(sourceName, _attr.GetTypeName(),
                                           /* custom = */ false);
        }
        SdfPathVector  target(1, sourceAttr.GetPath());
        success = rel.SetTargets(target);
    }
    else if (!sourceShader){
        TF_CODING_ERROR("Failed connecting parameter <%s>. "
                        "The given source shader prim <%s> is not defined", 
                        _attr.GetPath().GetText(),
                        sourcePrim ? sourcePrim.GetPath().GetText() :
                        "invalid-prim");
        return false;
    }
    else if (!rel){
        TF_CODING_ERROR("Failed connecting parameter <%s>. "
                        "Unable to make the connection to source <%s>.", 
                        _attr.GetPath().GetText(),
                        sourcePrim.GetPath().GetText());
        return false;
    }

    return success;
}

bool 
UsdShadeParameter::ConnectToSource(
        UsdShadeShader const &source, 
        TfToken const &outputName,
        bool outputIsParameter) const
{
    UsdRelationship rel = _GetParameterConnection(*this, true);

    return _Connect(rel, source, outputName, outputIsParameter);
}
    
bool 
UsdShadeParameter::DisconnectSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetParameterConnection(*this, false)) {
        success = rel.BlockTargets();
    }
    return success;
}

bool
UsdShadeParameter::ClearSource() const
{
    bool success = true;
    if (UsdRelationship rel = _GetParameterConnection(*this, false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }
    return success;
}

static
bool
_EvaluateConnection(
    UsdRelationship const &connection,
    UsdShadeShader *source, 
    TfToken *outputName)
{
    *source = UsdShadeShader();
    SdfPathVector targets;
    // There should be no possibility of forwarding, here, since the API
    // only allows targetting prims
    connection.GetTargets(&targets);
    // XXX(validation)  targets.size() <= 1, also outputName
    if (targets.size() == 1) {
        SdfPath const & path = targets[0];
        *source = UsdShadeShader::Get(connection.GetStage(), 
                                      path.GetPrimPath());
        if (path.IsPropertyPath()){
            const size_t prefixLen = _tokens->outputs.GetString().size();
            TfToken const &attrName(path.GetNameToken());
            if (TfStringStartsWith(attrName, _tokens->outputs)){
                *outputName = TfToken(attrName.GetString().substr(prefixLen));
            }
            else {
                *outputName = attrName;
            }
        } 
        else {
            // XXX validation error
            if ( TfGetEnvSetting(USD_SHADE_BACK_COMPAT) ) {
                return connection.GetMetadata(_tokens->outputName, outputName)
                    && *source;
            }
        }
    }
    return *source;
}

bool 
UsdShadeParameter::GetConnectedSource(
        UsdShadeShader *source, 
        TfToken *outputName) const
{
    if (!(source && outputName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }
    if (UsdRelationship rel = _GetParameterConnection(*this, false)) {
        return _EvaluateConnection(rel, source, outputName);
    }
    else {
        *source = UsdShadeShader();
        return false;
    }
}

bool 
UsdShadeParameter::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    UsdShadeShader source;
    TfToken        outputName;
    return GetConnectedSource(&source, &outputName);
}

TfToken 
UsdShadeParameter::GetConnectionRelName() const
{
    return TfToken(_tokens->connectedSourceFor.GetString() + 
           _attr.GetName().GetString());
}
