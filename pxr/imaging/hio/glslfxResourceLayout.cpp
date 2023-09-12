//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hio/glslfxResourceLayout.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(HioGlslfxResourceLayoutTokens,
                        HIO_GLSLFX_RESOURCE_LAYOUT_TOKENS);

HioGlslfxResourceLayout::HioGlslfxResourceLayout() = default;
HioGlslfxResourceLayout::~HioGlslfxResourceLayout() = default;

namespace {

using InOut = HioGlslfxResourceLayout::InOut;
using Kind = HioGlslfxResourceLayout::Kind;
using Member = HioGlslfxResourceLayout::Member;
using MemberVector = HioGlslfxResourceLayout::MemberVector;
using Element = HioGlslfxResourceLayout::Element;
using ElementVector = HioGlslfxResourceLayout::ElementVector;

using InputValue = VtValue;
using InputValueVector = std::vector<VtValue>;

TfToken
_Token(InputValue const & input)
{
    return TfToken(input.GetWithDefault(
        HioGlslfxResourceLayoutTokens->unknown.GetString()));
}

InputValueVector
_GetInputValueVector(InputValue const & input)
{
    return input.GetWithDefault(InputValueVector());
}

// e.g. ["vec4", "Peye"]
MemberVector
_ParseMembers(InputValueVector const & input, int fromElement)
{
    MemberVector result;
    for (auto const & inputValue : input) {
        InputValueVector memberInput = _GetInputValueVector(inputValue);
        if (memberInput.size() != 2 && memberInput.size() != 3) continue;
        result.emplace_back(/*dataType=*/_Token(memberInput[0]),
                            /*name=*/_Token(memberInput[1]));
        if (input.size() == 3) {
            result.back().arraySize = _Token(input[2]);
        }
    }
    return result;
}

// e.g. ["in", "vec3", "color"]
// e.g. ["in", "int", "pointId", "flat"]
bool
_ParseValue(InputValueVector const & input, Element *element)
{
    if (input.size() != 3 && input.size() != 4) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->inValue) {
        *element = Element(InOut::STAGE_IN, Kind::VALUE,
                           /*dataType=*/_Token(input[1]),
                           /*name=*/_Token(input[2]));
        if (input.size() == 4) {
            element->qualifiers = _Token(input[3]);
        }
        return true;
    } else if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->outValue) {
        *element = Element(InOut::STAGE_OUT, Kind::VALUE,
                           /*dataType=*/_Token(input[1]),
                           /*name=*/_Token(input[2]));
        if (input.size() == 4) {
            element->qualifiers = _Token(input[3]);
        }
        return true;
    }
    return false;
}

// e.g. ["in array", "vec3", "color", "NUM_VERTS"]
bool
_ParseValueArray(InputValueVector const & input, Element *element)
{
    if (input.size() != 4) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->inValueArray) {
        *element = Element(InOut::STAGE_IN, Kind::VALUE,
                           /*dataType=*/_Token(input[1]),
                           /*name=*/_Token(input[2]),
                           /*arraySize=*/_Token(input[3]));
        return true;
    } else if (
        _Token(input[0]) == HioGlslfxResourceLayoutTokens->outValueArray) {
        *element = Element(InOut::STAGE_OUT, Kind::VALUE,
                           /*dataType=*/_Token(input[1]),
                           /*name=*/_Token(input[2]),
                           /*arraySize=*/_Token(input[3]));
        return true;
    }
    return false;
}

// e.g. ["in block", "VertexData", "inData",
//          ["vec3", "Peye"],
//          ["vec3", "Neye"]
//      ]
bool
_ParseBlock(InputValueVector const & input, Element *element)
{
    if (input.size() < 4) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->inBlock) {
        *element = Element(InOut::STAGE_IN, Kind::BLOCK,
                           /*dataType=*/HioGlslfxResourceLayoutTokens->block,
                           /*name=*/_Token(input[2]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/3);
        return true;
    } else if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->outBlock) {
        *element = Element(InOut::STAGE_OUT, Kind::BLOCK,
                           /*dataType=*/HioGlslfxResourceLayoutTokens->block,
                           /*name=*/_Token(input[2]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/3);
        return true;
    }
    return false;
}

// e.g. ["in block array", "VertexData", "inData", "NUM_VERTS",
//          ["vec3", "Peye"],
//          ["vec3", "Neye"]
//      ]
bool
_ParseBlockArray(InputValueVector const & input, Element *element)
{
    if (input.size() < 5) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->inBlockArray) {
        *element = Element(InOut::STAGE_IN, Kind::BLOCK,
                           /*dataType=*/HioGlslfxResourceLayoutTokens->block,
                           /*name=*/_Token(input[2]),
                           /*arraySize=*/_Token(input[3]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/4);
        return true;
    } else if (_Token(input[0]) ==
               HioGlslfxResourceLayoutTokens->outBlockArray) {
        *element = Element(InOut::STAGE_OUT, Kind::BLOCK,
                           /*dataType=*/HioGlslfxResourceLayoutTokens->block,
                           /*name=*/_Token(input[2]),
                           /*arraySize=*/_Token(input[3]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/4);
        return true;
    }
    return false;
}

// e.g. ["in", "early_fragment_tests"]
bool
_ParseQualifier(InputValueVector const & input, Element *element)
{
    if (input.size() != 2) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->inValue) {
        *element = Element(InOut::STAGE_IN, Kind::QUALIFIER);
        element->qualifiers = _Token(input[1]);
        return true;
    } else if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->outValue) {
        *element = Element(InOut::STAGE_OUT, Kind::QUALIFIER);
        element->qualifiers = _Token(input[1]);
        return true;
    }

    return true;
}

// e.g. ["uniform block", "Uniforms", "cullParams",
//          ["mat4", "cullMatrix"],
//          ["vec2", "drawRangeNDC"],
//          ["uint", "drawCommandNumUints"],
//          ["int",  "resetPass"]
//      ]
bool
_ParseUniformBlock(InputValueVector const & input, Element *element)
{
    if (input.size() < 4) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->uniformBlock) {
        *element = Element(
            InOut::NONE, Kind::UNIFORM_BLOCK_CONSTANT_PARAMS,
            /*dataType=*/HioGlslfxResourceLayoutTokens->uniformBlock,
            /*name=*/_Token(input[2]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/3);
        return true;
    }
    return false;
}

// e.g. ["buffer readWrite", "DispatchBuffer", "dispatchBuffer",
//          ["uint", "drawCommands", "[]"]
//      ]
bool
_ParseBuffer(InputValueVector const & input, Element *element)
{
    if (input.size() < 4) return false;
    if (_Token(input[0]) == HioGlslfxResourceLayoutTokens->bufferReadOnly) {
        *element = Element(
            InOut::NONE, Kind::BUFFER_READ_ONLY,
            /*dataType=*/HioGlslfxResourceLayoutTokens->bufferReadOnly,
            /*name=*/_Token(input[2]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/3);
        return true;
    } else if (_Token(input[0]) ==
               HioGlslfxResourceLayoutTokens->bufferReadWrite) {
        *element = Element(
            InOut::NONE, Kind::BUFFER_READ_WRITE,
            /*dataType=*/HioGlslfxResourceLayoutTokens->bufferReadWrite,
            /*name=*/_Token(input[2]));
        element->aggregateName = _Token(input[1]);
        element->members = _ParseMembers(input, /*fromElement=*/3);
        return true;
    }
    return false;
}

void
_ParsePerStageLayout(ElementVector *result, VtValue const &perStageLayout)
{
    InputValueVector perSnippetVector = _GetInputValueVector(perStageLayout);
    for (VtValue const &perSnippet: perSnippetVector) {

        InputValueVector perDeclVector = _GetInputValueVector(perSnippet);
        for (VtValue const &perDecl : perDeclVector) {

            InputValueVector input = _GetInputValueVector(perDecl);
            Element element;

            if (_ParseValue(input, &element) ||
                _ParseValueArray(input, &element) ||
                _ParseBlock(input, &element) ||
                _ParseBlockArray(input, &element) ||
                _ParseQualifier(input, &element) ||
                _ParseUniformBlock(input, &element) ||
                _ParseBuffer(input, &element)) {
                result->push_back(element);
            } else {
                TF_CODING_ERROR("Error parsing PerStageLayout");
            }
        }
    }
}

};

/* static */
void
HioGlslfxResourceLayout::ParseLayout(
    ElementVector *result,
    TfToken const &shaderStage,
    VtDictionary const &layoutDict)
{
    VtValue perStageLayout;
    if (TfMapLookup(layoutDict, shaderStage, &perStageLayout)) {
        _ParsePerStageLayout(result, perStageLayout);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
