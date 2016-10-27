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
    (arrayConnectionSize)
);

static TfToken 
_GetConnectionRelName(const TfToken& name, int element=-1)
{
    if (element == -1){
        return TfToken(_tokens->connectedSourceFor.GetString() + name.GetString());
    } else {
        string fullName = TfStringPrintf("%s%s:_%0d",
                                         _tokens->connectedSourceFor.GetText(),
                                         name.GetText(),
                                         element);
        return TfToken(fullName);
    }
}

static size_t
_ElementIndexFromConnection(UsdRelationship const &connection)
{
    string  eltName = connection.GetName();
    return size_t(atoi(&eltName[eltName.rfind('_')+1]));
}

static UsdRelationship
_GetCompleteParameterConnection(
        const UsdAttribute& attr,
        bool create)
{
    const UsdPrim& prim = attr.GetPrim();
    const TfToken& relName = _GetConnectionRelName(attr.GetName());
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

static UsdRelationship
_GetElementParameterConnection(
        const UsdAttribute& attr,
        size_t element,
        bool create)
{
    const UsdPrim& prim = attr.GetPrim();
    const TfToken& relName = _GetConnectionRelName(attr.GetName(), static_cast<int>(element));
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

static bool
_IsArrayConnection(UsdProperty const &prop)
{
    // array connection rel names will always look like
    // connectedSourceFor:someParameter:_##
    // ...where ## is one or more digits
    if (not prop.Is<UsdRelationship>())
        return false;
    string name = prop.GetName();
    size_t  lastSep = name.rfind(':');
    if (ARCH_UNLIKELY(lastSep == string::npos))
        return false;
    if (name[++lastSep] != '_')
        return false;
    while (++lastSep < name.size()){
        if (not isdigit(name[lastSep]))
            return false;
    }
    return true;
}

// Returns connections in element order... may be sparse if not all 
// elements have been connected or disconnected
static vector<UsdRelationship>
_GetElementConnections(UsdAttribute const &attr)
{
    string arrayPrefix = TfStringPrintf("%s%s", 
                                        _tokens->connectedSourceFor.GetText(),
                                        attr.GetName().GetText());
    vector<UsdProperty> props = 
        attr.GetPrim().GetPropertiesInNamespace(arrayPrefix);
    vector<UsdRelationship> rels;
    if (props.empty())
        return rels;
    rels.reserve(props.size());

    TF_FOR_ALL(p, props){
        if (_IsArrayConnection(*p)){
            rels.push_back(p->As<UsdRelationship>());
        }
    }
    // Unfortunate as they will probably 99.99% of the time already be
    // sorted, but the data model cannot guarantee it.
    std::sort(rels.begin(), rels.end(), _PropertyLessThan());
    return rels;
}


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
    if (not _attr) {
        _attr = prim.CreateAttribute(name, typeName, /* custom = */ false);
    }
}

bool
UsdShadeParameter::IsArray() const
{
    return _attr.GetTypeName().IsArray();
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
    if (rel and sourcePrim) {
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
    else if (not sourceShader){
        TF_CODING_ERROR("Failed connecting parameter <%s>. "
                        "The given source shader prim <%s> is not defined", 
                        _attr.GetPath().GetText(),
                        sourcePrim ? sourcePrim.GetPath().GetText() :
                        "invalid-prim");
        return false;
    }
    else if (not rel){
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
    UsdRelationship rel = _GetCompleteParameterConnection(_attr, true);

    return _Connect(rel, source, outputName, outputIsParameter);
}

bool 
UsdShadeParameter::ConnectElementToSource(
        size_t elementIndex, 
        UsdShadeShader const &source, 
        TfToken const &outputName,
        bool outputIsParameter) const
{
    if (not IsArray())
        return false;
    
    UsdRelationship rel = _GetElementParameterConnection(_attr, 
                                                         elementIndex,
                                                         true);
    return _Connect(rel, source, outputName, outputIsParameter);
}

bool 
UsdShadeParameter::DisconnectElement(size_t elementIndex) const
{
    if (not IsArray())
        return false;
    
    UsdRelationship rel = _GetElementParameterConnection(_attr, 
                                                         elementIndex,
                                                         true);
    if (not rel){
        return false;
    }
    
    return rel.SetMetadata(_tokens->outputName, TfToken()) and
        rel.BlockTargets();
}
    
bool 
UsdShadeParameter::DisconnectSources() const
{
    bool success = true;
    if (UsdRelationship rel = _GetCompleteParameterConnection(_attr, false)) {
        success = rel.BlockTargets();
    }

    // For an array that is connected as a whole, we will wind up making
    // a redundant, ignored connectedSourceFor:attr:_0 connection here,
    // shouldn't be an issue.
    size_t numElements = GetConnectedArraySize();
    for (size_t i=0; i<numElements; ++i){
        UsdRelationship elt = _GetElementParameterConnection(_attr, i, true);
        if (elt){
            success = elt.BlockTargets() and success;
        }
        else {
            success = false;
        }
    }
    return success;

}

bool
UsdShadeParameter::ClearSources() const
{
    bool success = true;
    if (UsdRelationship rel = _GetCompleteParameterConnection(_attr, false)) {
        success = rel.ClearTargets(/* removeSpec = */ true);
    }

    for (auto elt : _GetElementConnections(_attr)) {
        success = elt.ClearTargets(/* removeSpec = */ true) and success;
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
                    and *source;
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
    if (not (source and outputName)){
        TF_CODING_ERROR("GetConnectedSource() requires non-NULL "
                        "output parameters");
        return false;
    }
    if (UsdRelationship rel = _GetCompleteParameterConnection(_attr, false)) {
        return _EvaluateConnection(rel, source, outputName);
    }
    else {
        *source = UsdShadeShader();
        return false;
    }
}

bool 
UsdShadeParameter::GetConnectedSources(
    vector<UsdShadeShader> *sources, 
    vector<TfToken> *outputNames) const
{
    if (not (sources and outputNames)){
        TF_CODING_ERROR("GetConnectedSources() requires non-NULL "
                        "output parameters");
        return false;
    }
    sources->clear();
    outputNames->clear();
    
    // Single connection always wins
    UsdShadeShader source;
    TfToken outputName;
    if (GetConnectedSource(&source, &outputName)){
        sources->push_back(source);
        outputNames->push_back(outputName);
        return true;
    }
    else if (not IsArray()){
        return false;
    }

    bool connected = false;
    vector<UsdRelationship> connections = _GetElementConnections(_attr);
    int numElts = -1;
    if (((not _attr.GetMetadata(_tokens->arrayConnectionSize, &numElts)) or
         (numElts < 0)) and
        not connections.empty()){
        numElts = 1 + static_cast<int>(_ElementIndexFromConnection(connections.back()));
    }
    if (numElts <= 0){
        return false;
    }

    *sources = vector<UsdShadeShader>(numElts, UsdShadeShader());
    *outputNames = vector<TfToken>(numElts, TfToken());
    
    TF_FOR_ALL(connection, connections){
        size_t  eltIndex = _ElementIndexFromConnection(*connection);
        // numElts may have been explicitly authored smaller than the
        // number of authored connections
        if (eltIndex >= static_cast<size_t>(numElts))
            break;
        
        if (_EvaluateConnection(*connection, 
                                &(*sources)[eltIndex],
                                &(*outputNames)[eltIndex])){
            connected = true;
        }
    }
        
    return connected;
}

bool 
UsdShadeParameter::IsConnected() const
{
    /// This MUST have the same semantics as GetConnectedSource(s).
    /// XXX someday we might make this more efficient through careful
    /// refactoring, but safest to just call the exact same code.
    if (IsArray()){
        std::vector<UsdShadeShader> sources;
        std::vector<TfToken> outputNames;
        return GetConnectedSources(&sources, &outputNames);
    }
    else {
        UsdShadeShader source;
        TfToken        outputName;
        return GetConnectedSource(&source, &outputName);
    }
}

bool 
UsdShadeParameter::SetConnectedArraySize(size_t numElts) const
{
    return _attr.SetMetadata(_tokens->arrayConnectionSize, int(numElts));
}
    
size_t
UsdShadeParameter::GetConnectedArraySize() const
{
    if (not IsArray())
        return 0;
    
    int  explicitNumElts = -1;
    if (_attr.GetMetadata(_tokens->arrayConnectionSize, &explicitNumElts) and
        explicitNumElts > -1){
        return size_t(explicitNumElts);
    }

    // connected-as-unit-to-another-array case
    UsdShadeShader  source;
    TfToken  outputName;
    if (GetConnectedSource(&source, &outputName) and source){
        return 1;
    }
    
    vector<UsdRelationship> connections = _GetElementConnections(_attr);
    if (connections.empty())
        return 0;
    return 1 + _ElementIndexFromConnection(connections.back());
}

