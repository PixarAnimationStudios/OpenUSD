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
#include "pxr/imaging/hd/shaderKey.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include <boost/functional/hash.hpp>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


static
std::string
_JoinTokens(const char *stage, TfToken const *tokens, bool *firstStage)
{
    if ((!tokens) || tokens->IsEmpty()) return std::string();

    std::stringstream ss;

    if (*firstStage == false) ss << ", ";
    *firstStage = false;

    ss << "\"" << stage << "\" : { "
       << "\"source\" : [";

    bool first = true;
    while (!tokens->IsEmpty()) {
        if (first == false) ss << ", ";
        ss << "\"" << tokens->GetText() << "\"";
        ++tokens;
        first = false;
    }

    ss << "] }\n";
    return ss.str();
}

/*static*/
HdShaderKey::ID
HdShaderKey::ComputeHash(TfToken const &glslfxFile,
                         TfToken const *VS,
                         TfToken const *TCS,
                         TfToken const *TES,
                         TfToken const *GS,
                         TfToken const *FS,
                         int16_t primType,                         
                         HdCullStyle cullStyle,
                         HdPolygonMode polygonMode,
                         bool cullingPass,
                         bool faceVarying)
{
    ID hash = glslfxFile.Hash();

    while (VS && (!VS->IsEmpty())) {
        boost::hash_combine(hash, VS->Hash());
        ++VS;
    }
    while (TCS && (!TCS->IsEmpty())) {
        boost::hash_combine(hash, TCS->Hash());
        ++TCS;
    }
    while (TES && (!TES->IsEmpty())) {
        boost::hash_combine(hash, TES->Hash());
        ++TES;
    }
    while (GS && (!GS->IsEmpty())) {
        boost::hash_combine(hash, GS->Hash());
        ++GS;
    }
    while (FS && (!FS->IsEmpty())) {
        boost::hash_combine(hash, FS->Hash());
        ++FS;
    }
    boost::hash_combine(hash, primType);    
    boost::hash_combine(hash, cullStyle);
    boost::hash_combine(hash, polygonMode);
    boost::hash_combine(hash, cullingPass);
    boost::hash_combine(hash, faceVarying);
    return hash;
}

/*static*/
std::string
HdShaderKey::GetGLSLFXString(TfToken const &glslfxFile,
                             TfToken const *VS,
                             TfToken const *TCS,
                             TfToken const *TES,
                             TfToken const *GS,
                             TfToken const *FS)
{
    std::stringstream ss;

    ss << "-- glslfx version 0.1\n";

    if (!glslfxFile.IsEmpty())
        ss << "#import $TOOLS/hd/shaders/" << glslfxFile.GetText() << "\n";

    ss << "-- configuration\n"
       << "{\"techniques\": {\"default\": {\n";

    bool firstStage = true;
    ss << _JoinTokens("vertexShader",      VS,  &firstStage);
    ss << _JoinTokens("tessControlShader", TCS, &firstStage);
    ss << _JoinTokens("tessEvalShader",    TES, &firstStage);
    ss << _JoinTokens("geometryShader",    GS,  &firstStage);
    ss << _JoinTokens("fragmentShader",    FS,  &firstStage);
    ss << "}}}\n";

    return ss.str();
}

PXR_NAMESPACE_CLOSE_SCOPE

