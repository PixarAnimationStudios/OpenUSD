//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdAovIdentifier::HdAovIdentifier(TfToken const& aovName)
    : isPrimvar(false)
    , isLpe(false)
    , isShader(false)
{
    std::string const& aov = aovName.GetString();
    std::string const& primvars = HdAovTokens->primvars.GetString();
    std::string const& lpe = HdAovTokens->lpe.GetString();
    std::string const& shader = HdAovTokens->shader.GetString();

    if (aov.size() > primvars.size() &&
        aov.compare(0, primvars.size(), primvars) == 0) {
        name = TfToken(aov.substr(primvars.size()));
        isPrimvar = true;
    } else if (aov.size() > lpe.size() &&
               aov.compare(0, lpe.size(), lpe) == 0) {
        name = TfToken(aov.substr(lpe.size()));
        isLpe = true;
    } else if (aov.size() > shader.size() &&
               aov.compare(0, shader.size(), shader) == 0) {
        name = TfToken(aov.substr(shader.size()));
        isShader = true;
    } else {
        name = aovName;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
