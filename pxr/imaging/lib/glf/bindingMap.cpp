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
/// \file bindingMap.cpp

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

int
GlfBindingMap::GetSamplerUnit(std::string const & name)
{
    return GetSamplerUnit(TfToken(name));
}

int
GlfBindingMap::GetSamplerUnit(TfToken const & name)
{
    int samplerUnit = -1;
    if (not TfMapLookup(_samplerBindings, name, &samplerUnit)) {
        // XXX error check < MAX_TEXTURE_IMAGE_UNITS
        samplerUnit = static_cast<int>(_samplerBindings.size());
        _samplerBindings[name] = samplerUnit;
    }
    TF_VERIFY(samplerUnit >= 0);
    return samplerUnit;
}

int
GlfBindingMap::GetAttributeIndex(std::string const & name)
{
    return GetAttributeIndex(TfToken(name));
}

int
GlfBindingMap::GetAttributeIndex(TfToken const & name)
{
    int attribIndex = -1;
    if (not TfMapLookup(_attribBindings, name, &attribIndex)) {
        return -1;
    }
    return attribIndex;
}

void
GlfBindingMap::AssignSamplerUnitsToProgram(GLuint program)
{
    TF_FOR_ALL(it, _samplerBindings) {
        GLint loc = glGetUniformLocation(program, it->first.GetText());
        if (loc != -1) {
            glProgramUniform1i(program, loc, it->second);
        }
    }
}

int
GlfBindingMap::GetUniformBinding(std::string const & name)
{
    return GetUniformBinding(TfToken(name));
}

int
GlfBindingMap::GetUniformBinding(TfToken const & name)
{
    int binding = -1;
    if (not TfMapLookup(_uniformBindings, name, &binding)) {
        binding = (int)_uniformBindings.size();
        _uniformBindings[name] = binding;
    }
    TF_VERIFY(binding >= 0);
    return binding;
}

bool
GlfBindingMap::HasUniformBinding(std::string const & name) const
{
    return HasUniformBinding(TfToken(name));
}

bool
GlfBindingMap::HasUniformBinding(TfToken const & name) const
{
    return (_uniformBindings.find(name) != _uniformBindings.end());
}

void
GlfBindingMap::AssignUniformBindingsToProgram(GLuint program)
{
    TF_FOR_ALL(it, _uniformBindings) {
        GLuint uboIndex = glGetUniformBlockIndex(program, it->first.GetText());
        if (uboIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(program, uboIndex, it->second);
        }
    }
}

void
GlfBindingMap::AddCustomBindings(GLuint program)
{
    _AddActiveAttributeBindings(program);
    _AddActiveUniformBindings(program);
    _AddActiveUniformBlockBindings(program);

    // assign uniform bindings / texture samplers
    AssignUniformBindingsToProgram(program);
    AssignSamplerUnitsToProgram(program);
}

void
GlfBindingMap::_AddActiveAttributeBindings(GLuint program)
{
    GLint numAttributes = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
    if (numAttributes == 0) return;

    GLint maxNameLength = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength);
    maxNameLength = std::max(maxNameLength, 100);
    GLint size;
    GLenum type;
    char * name = new char[maxNameLength];

    for (int i = 0; i < numAttributes; ++i) {
        glGetActiveAttrib(program, i, maxNameLength, NULL, &size, &type, name);
        GLint location = glGetAttribLocation(program, name);
        TfToken token(name);

        BindingMap::iterator it = _attribBindings.find(token);
        if (it == _attribBindings.end()) {
            _attribBindings[token] = location;
        } else if (it->second != location) {
            TF_RUNTIME_ERROR("Inconsistent attribute binding detected.");
        }
    }

    delete[] name;
}

void
GlfBindingMap::_AddActiveUniformBindings(GLuint program)
{
    GLint numUniforms = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
    if (numUniforms == 0) return;

    GLint maxNameLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
    GLint size;
    GLenum type;
    char * name = new char[maxNameLength];

    for (int i = 0; i < numUniforms; ++i) {
        glGetActiveUniform(program, i, maxNameLength, NULL, &size, &type, name);
        switch(type) {
        case GL_SAMPLER_1D:
        case GL_SAMPLER_2D:
        case GL_SAMPLER_3D:
        case GL_SAMPLER_CUBE:
        case GL_SAMPLER_1D_SHADOW:
        case GL_SAMPLER_2D_SHADOW:
        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_SAMPLER_CUBE_SHADOW:
        case GL_SAMPLER_BUFFER:
        case GL_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_RECT_SHADOW:
        case GL_INT_SAMPLER_1D:
        case GL_INT_SAMPLER_2D:
        case GL_INT_SAMPLER_3D:
        case GL_INT_SAMPLER_CUBE:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            GetSamplerUnit(name);
            break;
        }
    }
    delete[] name;
}

void
GlfBindingMap::_AddActiveUniformBlockBindings(GLuint program)
{
    GLint numUniformBlocks = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
    if (numUniformBlocks == 0) return;

    GLint maxNameLength = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxNameLength);
    char *name = new char[maxNameLength];

    for (int i = 0; i < numUniformBlocks; ++i) {
        glGetActiveUniformBlockName(program, i, maxNameLength, NULL, name);
        GetUniformBinding(name);
    }
    delete[] name;
}

void
GlfBindingMap::Debug() const
{
    printf("GlfBindingMap\n");

    // sort for comparing baseline in testGlfBindingMap
    std::map<TfToken, int> attribBindings, samplerBindings, uniformBindings;
    TF_FOR_ALL (it, _attribBindings) { attribBindings.insert(*it); }
    TF_FOR_ALL (it, _samplerBindings) { samplerBindings.insert(*it); }
    TF_FOR_ALL (it, _uniformBindings) { uniformBindings.insert(*it); }
    
    printf(" Attribute bindings\n");
    TF_FOR_ALL (it, attribBindings) {
        printf("  %s : %d\n", it->first.GetText(), it->second);
    }
    printf(" Sampler bindings\n");
    TF_FOR_ALL (it, samplerBindings) {
        printf("  %s : %d\n", it->first.GetText(), it->second);
    }
    printf(" Uniform bindings\n");
    TF_FOR_ALL (it, uniformBindings) {
        printf("  %s : %d\n", it->first.GetText(), it->second);
    }
}

