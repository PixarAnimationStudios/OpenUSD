//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

// Check fpr specific storage and interpolation qualifiers.
bool
_IsMemberQualifier(TfToken const & input)
{
    return (input == HioGlslfxResourceLayoutTokens->centroid ||
            input == HioGlslfxResourceLayoutTokens->sample ||
            input == HioGlslfxResourceLayoutTokens->flat ||
            input == HioGlslfxResourceLayoutTokens->noperspective ||
            input == HioGlslfxResourceLayoutTokens->smooth);
}

// e.g. ["vec4", "Peye"]
// e.g. ["float", "length", "3"] The member is a float array with 3 elements.
// e.g. ["vec3", "color", "flat"] The member type is vec3, and there is no 
// interpolation across the face.
// e.g. ["float", "length", "3", "flat"] The member is a float array with 3 
// elements, and there is no interpolation across the face.
MemberVector
_ParseMembers(InputValueVector const & input, int fromElement)
{
    MemberVector result;
    for (auto const & inputValue : input) {
        InputValueVector memberInput = _GetInputValueVector(inputValue);
        const size_t n = memberInput.size();
        if (!(2 <= n && n <= 4)) {
            continue;
        }
        result.emplace_back(/*dataType=*/_Token(memberInput[0]),
                            /*name=*/_Token(memberInput[1]));
        if (n == 3) {
            TfToken const inputToken = _Token(memberInput[2]);
            // Try to parse qualifier.
            if (_IsMemberQualifier(inputToken)) {
                result.back().qualifiers = inputToken;
            // If fails, parse as array size.
            } else {
                result.back().arraySize = inputToken;
            }
        } else if (n == 4) {
            result.back().arraySize = _Token(memberInput[2]);
            result.back().qualifiers = _Token(memberInput[3]);
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
