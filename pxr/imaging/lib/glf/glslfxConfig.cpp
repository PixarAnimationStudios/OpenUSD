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
#include "pxr/imaging/glf/glslfxConfig.h"
#include "pxr/imaging/glf/debugCodes.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


using std::string;
using std::vector;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (attributes)
    (techniques)
    (metadata)
    (parameters)
    (parameterOrder)
    (textures)
    (documentation)
    (role)
    (color)
    ((defVal, "default"))
    (source)
);

VtDictionary Glf_GetDictionaryFromInput(const string &input, const string &filename, string *errorStr);

GlfGLSLFXConfig *
GlfGLSLFXConfig::Read(string const & input, string const & filename, string *errorStr)
{
    return new GlfGLSLFXConfig(
        Glf_GetDictionaryFromInput(input, filename, errorStr), errorStr );
}

GlfGLSLFXConfig::GlfGLSLFXConfig(VtDictionary const & dict, string * errors)
{
    _Init(dict, errors);
}

void
GlfGLSLFXConfig::_Init(VtDictionary const & dict, string * errors)
{
    _params = _GetParameters(dict, errors);
    _textures = _GetTextures(dict, errors);
    _attributes = _GetAttributes(dict, errors);
    _metadata = _GetMetadata(dict, errors);
    _sourceKeyMap = _GetSourceKeyMap(dict, errors);
}

GlfGLSLFXConfig::SourceKeys
GlfGLSLFXConfig::GetSourceKeys(TfToken const & shaderStageKey) const
{
    GlfGLSLFXConfig::SourceKeys ret;
    TfMapLookup(_sourceKeyMap, shaderStageKey, &ret);
    return ret;
}
 
GlfGLSLFXConfig::_SourceKeyMap
GlfGLSLFXConfig::_GetSourceKeyMap(VtDictionary const & dict,
                                   string *errorStr) const
{
    // XXX as we implement more public API for this thing, some better structure
    // in the internal API we use to access parts of this graph would
    // be nice. perhaps even our own variant type instead of VtDictionary?
    _SourceKeyMap ret;

    VtValue techniques;

    // verify that techiniques is specified
    if (!TfMapLookup(dict, _tokens->techniques, &techniques)) {
        *errorStr = TfStringPrintf("Configuration does not specify %s",
                                   _tokens->techniques.GetText());
        return ret;
    }

    // verify that it holds a VtDictionary
    if (!techniques.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s declaration expects a dictionary value",
                                   _tokens->techniques.GetText());
        return ret;
    }

    // allow only one technique for now, but we plan on supporting more in
    // the future
    const VtDictionary& techniquesDict =
        techniques.UncheckedGet<VtDictionary>();

    if (techniquesDict.size() == 0) {
        *errorStr = TfStringPrintf("No %s specified",
                                   _tokens->techniques.GetText());
        return ret;
    }

    if (techniquesDict.size() > 1) {
        *errorStr = TfStringPrintf("Expect only one entry for %s",
                                   _tokens->techniques.GetText());
        return ret;
    }

    // get the value of the first technique spec
    VtDictionary::const_iterator entry = techniquesDict.begin();
    VtValue techniqueSpec = entry->second;
    
    // verify that it also holds a VtDictionary
    if (!techniqueSpec.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s spec for %s expects a dictionary value",
                                   _tokens->techniques.GetText(),
                                   entry->first.c_str());
        return ret;
    }

    const VtDictionary& specDict = techniqueSpec.UncheckedGet<VtDictionary>();
    // get all of the shader stages specified in the spec
    TF_FOR_ALL(it, specDict) {
        const string& shaderStageKey = it->first;
        const VtValue& shaderStageSpec = it->second;

        // verify that the shaderStageSpec also holds a VtDictionary
        if (!shaderStageSpec.IsHolding<VtDictionary>()) {
            *errorStr = TfStringPrintf("%s spec for %s expects a dictionary "
                                       "value",
                                       entry->first.c_str(),
                                       shaderStageKey.c_str());
            return ret;
        }

        // get the source value for the shader stage
        const VtDictionary& shaderStageDict =
            shaderStageSpec.UncheckedGet<VtDictionary>();
        VtValue source;
        if (!TfMapLookup(shaderStageDict, _tokens->source, &source)) {
            *errorStr = TfStringPrintf("%s spec doesn't define %s for %s",
                                       entry->first.c_str(),
                                       _tokens->source.GetText(),
                                       shaderStageKey.c_str());
            return ret;
        }

        // verify that source holds a list
        if (!source.IsHolding<vector<VtValue> >()) {
            *errorStr = TfStringPrintf("%s of %s for spec %s expects a list",
                                       _tokens->source.GetText(),
                                       shaderStageKey.c_str(),
                                       entry->first.c_str());
            return ret;
        }

        vector<VtValue>  sourceList = source.UncheckedGet<vector<VtValue> >();
        TF_FOR_ALL(sourceIt, sourceList) {
            const VtValue& val = *sourceIt;
            // verify that this value is a string
            if (!val.IsHolding<string>()) {
                *errorStr = TfStringPrintf("%s of %s for spec %s expects a "
                                           "list of strings",
                                           _tokens->source.GetText(),
                                           shaderStageKey.c_str(),
                                           entry->first.c_str());
                return ret;
            }

            ret[shaderStageKey].push_back(val.UncheckedGet<string>());
        }
    }

    return ret;
}

static GlfGLSLFXConfig::Role
_GetRoleFromString(string const & roleString, string *errorStr)
{
    if (roleString == _tokens->color) {
        return GlfGLSLFXConfig::RoleColor;
    }

    *errorStr = TfStringPrintf("Unknown role specification: %s",
                               roleString.c_str());
    return GlfGLSLFXConfig::RoleNone;
}


GlfGLSLFXConfig::Parameters
GlfGLSLFXConfig::GetParameters() const
{
    return _params;
}

GlfGLSLFXConfig::Parameters
GlfGLSLFXConfig::_GetParameters(VtDictionary const & dict, 
                                 string *errorStr) const
{
    Parameters ret;

    VtValue params;

    // look for the params section
    if (!TfMapLookup(dict, _tokens->parameters, &params)) {
        return ret;
    }

    // verify that it holds a VtDictionary
    if (!params.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s declaration expects a dictionary value",
                                   _tokens->parameters.GetText());
        return ret;
    }

    // look for the parameterOrder section: 
    vector<string> paramOrder;
    VtValue paramOrderAny;
    TfMapLookup(dict, _tokens->parameterOrder, &paramOrderAny);

    if (!paramOrderAny.IsEmpty()) {
        // verify the type
        if (!paramOrderAny.IsHolding<vector<VtValue> >()) {
            *errorStr =
                TfStringPrintf("%s declaration expects a list of strings",
                               _tokens->parameterOrder.GetText());
            return ret;
        }

        const vector<VtValue>& paramOrderList =
            paramOrderAny.UncheckedGet<vector<VtValue> >();
        TF_FOR_ALL(paramIt, paramOrderList) {
            const VtValue& val = *paramIt;
            // verify that this value is a string
            if (!val.IsHolding<string>()) {
                *errorStr = TfStringPrintf("%s declaration expects a list of "
                                           "strings",
                                           _tokens->parameterOrder.GetText());
                return ret;
            }

            const string& paramName = val.UncheckedGet<string>();
            if (std::find(paramOrder.begin(), paramOrder.end(), paramName) ==
                    paramOrder.end()) {
                paramOrder.push_back(paramName);
            }
        }
    }


    const VtDictionary& paramsDict = params.UncheckedGet<VtDictionary>();
    // pre-process the paramsDict in order to get the merged ordering
    TF_FOR_ALL(it, paramsDict) {
        string paramName = it->first;
        if (std::find(paramOrder.begin(), paramOrder.end(), paramName) ==
                paramOrder.end()) {
            paramOrder.push_back(paramName);
        }
    }

    // now go through the params in the specified order
    TF_FOR_ALL(it, paramOrder) {
        string paramName = *it;

        // ignore anything specified in the order that isn't in the actual dict
        VtDictionary::const_iterator dictIt = paramsDict.find(*it);
        if (dictIt == paramsDict.end()) {
            continue;
        }

        const VtValue& paramData = dictIt->second;

        if (!paramData.IsHolding<VtDictionary>()) {
            *errorStr = TfStringPrintf("%s declaration for %s expects a "
                                       "dictionary value",
                                       _tokens->parameters.GetText(),
                                       paramName.c_str());
            return ret;
        }

        // get the default value out
        const VtDictionary& paramDataDict =
            paramData.UncheckedGet<VtDictionary>();
        VtValue defVal;
        if (!TfMapLookup(paramDataDict, _tokens->defVal, &defVal)) {
            *errorStr = TfStringPrintf("%s declaration for %s must specify "
                                       "a default value",
                                       _tokens->parameters.GetText(),
                                       paramName.c_str());
            return ret;
        }

        // optional documentation string
        VtValue docVal;
        string docString;
        if (TfMapLookup(paramDataDict, _tokens->documentation, &docVal)) {
            if (!docVal.IsHolding<string>()) {
                *errorStr = TfStringPrintf("Value for %s for %s is not a "
                                           "string",
                                           _tokens->documentation.GetText(),
                                           paramName.c_str());
                return ret;
            }

            docString = docVal.UncheckedGet<string>();
        }
        // optional role specification
        VtValue roleVal;
        Role role = RoleNone;
        if (TfMapLookup(paramDataDict, _tokens->role, &roleVal)) {
            if (!roleVal.IsHolding<string>()) {
                *errorStr = TfStringPrintf("Value for %s for %s is not a "
                                           "string",
                                           _tokens->role.GetText(),
                                           paramName.c_str());
                return ret;
            }

            const string& roleString = roleVal.UncheckedGet<string>();
            role = _GetRoleFromString(roleString, errorStr);
            if (!errorStr->empty()) {
                return ret;
            }
        }

        TF_DEBUG(GLF_DEBUG_GLSLFX).Msg("        param: %s\n",
            paramName.c_str());

        ret.push_back(Parameter(paramName, defVal, docString, role));
    }

    return ret;
}


GlfGLSLFXConfig::Textures
GlfGLSLFXConfig::GetTextures() const
{
    return _textures;
}

GlfGLSLFXConfig::Textures
GlfGLSLFXConfig::_GetTextures(VtDictionary const & dict, 
                               string *errorStr) const
{
    Textures ret;

    VtValue textures;

    // look for the params section
    if (!TfMapLookup(dict, _tokens->textures, &textures)) {
        return ret;
    }

    // verify that it holds a VtDictionary
    if (!textures.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s declaration expects a dictionary value",
                                   _tokens->textures.GetText());
        return ret;
    }

    const VtDictionary& texturesDict = textures.UncheckedGet<VtDictionary>();
    TF_FOR_ALL(it, texturesDict) {
        const string& textureName = it->first;
        const VtValue& textureData = it->second;
        if (!textureData.IsHolding<VtDictionary>()) {
            *errorStr = TfStringPrintf("%s declaration for %s expects a "
                                       "dictionary value",
                                       _tokens->textures.GetText(),
                                       textureName.c_str());
            return ret;
        }


        const VtDictionary& textureDataDict =
            textureData.UncheckedGet<VtDictionary>();

        // optional default color
        VtValue defVal;
        TfMapLookup(textureDataDict, _tokens->defVal, &defVal);

        // optional documentation string
        VtValue docVal;
        string docString;
        if (TfMapLookup(textureDataDict, _tokens->documentation, &docVal)) {
            if (!docVal.IsHolding<string>()) {
                *errorStr = TfStringPrintf("Value for %s for %s is not a "
                                           "string",
                                           _tokens->documentation.GetText(),
                                           textureName.c_str());
                return ret;
            }

            docString = docVal.UncheckedGet<string>();
        }

        TF_DEBUG(GLF_DEBUG_GLSLFX).Msg("        texture: %s\n",
            textureName.c_str());

        ret.push_back(Texture(textureName, defVal, docString));
    }

    return ret;
}

GlfGLSLFXConfig::Attributes
GlfGLSLFXConfig::GetAttributes() const
{
    return _attributes;
}

GlfGLSLFXConfig::Attributes
GlfGLSLFXConfig::_GetAttributes(VtDictionary const & dict,
                                 string *errorStr) const
{
    Attributes ret;

    VtValue attributes;

    // look for the attribute section
    if (!TfMapLookup(dict, _tokens->attributes, &attributes)) {
        return ret;
    }

    // verify that it holds a VtDictionary
    if (!attributes.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s declaration expects a dictionary value",
                                   _tokens->attributes.GetText());
        return ret;
    }

    const VtDictionary& attributesDict =
        attributes.UncheckedGet<VtDictionary>();
    TF_FOR_ALL(it, attributesDict) {
        const string& attributeName = it->first;
        const VtValue& attributeData = it->second;
        if (!attributeData.IsHolding<VtDictionary>()) {
            *errorStr = TfStringPrintf("%s declaration for %s expects a "
                                       "dictionary value",
                                       _tokens->attributes.GetText(),
                                       attributeName.c_str());
            return ret;
        }

        const VtDictionary& attributeDataDict =
            attributeData.UncheckedGet<VtDictionary>();

        // optional documentation string
        VtValue docVal;
        string docString;
        if (TfMapLookup(attributeDataDict, _tokens->documentation, &docVal)) {
            if (!docVal.IsHolding<string>()) {
                *errorStr = TfStringPrintf("Value for %s for %s is not a "
                                           "string",
                                           _tokens->documentation.GetText(),
                                           attributeName.c_str());
                return ret;
            }

            docString = docVal.UncheckedGet<string>();
        }

        TF_DEBUG(GLF_DEBUG_GLSLFX).Msg("        attribute: %s\n",
            attributeName.c_str());

        ret.push_back(Attribute(attributeName, docString));
    }

    return ret;
}

GlfGLSLFXConfig::MetadataDictionary
GlfGLSLFXConfig::GetMetadata() const
{
    return _metadata;
}

GlfGLSLFXConfig::MetadataDictionary
GlfGLSLFXConfig::_GetMetadata(VtDictionary const & dict,
                              string *errorStr) const
{
    MetadataDictionary ret;

    VtValue metadata;

    // look for the metadata section
    if (!TfMapLookup(dict, _tokens->metadata, &metadata)) {
        return ret;
    }

    // verify that it holds a VtDictionary
    if (!metadata.IsHolding<VtDictionary>()) {
        *errorStr = TfStringPrintf("%s declaration expects a dictionary value",
                                   _tokens->metadata.GetText());
        return ret;
    }

    return metadata.UncheckedGet<VtDictionary>();
}

PXR_NAMESPACE_CLOSE_SCOPE

