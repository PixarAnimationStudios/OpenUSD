//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLResourceBindings::HgiGLResourceBindings(
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
    , _vao()
{
    glCreateVertexArrays(1, &_vao);
    glObjectLabel(GL_VERTEX_ARRAY, _vao, -1, _descriptor.debugName.c_str());

    // Configure the vertex buffers in the vertex array object.
    for (HgiVertexBufferDesc const& vbo : _descriptor.vertexBuffers) {

        HgiVertexAttributeDescVector const& vas = vbo.vertexAttributes;

        // Describe each vertex attribute in the vertex buffer
        for (size_t loc=0; loc<vas.size(); loc++) {
            HgiVertexAttributeDesc const& va = vas[loc];

            uint32_t idx = va.shaderBindLocation;
            glEnableVertexArrayAttrib(_vao, idx);
            glVertexArrayAttribBinding(_vao, idx, vbo.bindingIndex);
            glVertexArrayAttribFormat(
                _vao, 
                idx,
                HgiGLConversions::GetElementCount(va.format),
                HgiGLConversions::GetFormatType(va.format),
                GL_FALSE,
                va.offset);
        }
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLResourceBindings::~HgiGLResourceBindings()
{
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &_vao);
}

void
HgiGLResourceBindings::BindResources()
{
    glBindVertexArray(_vao);

    std::vector<uint32_t> textures(_descriptor.textures.size(), 0);

    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        // OpenGL does not support arrays-of-textures bound to a unit.
        // (Which is different from texture-arrays. See Vulkan/Metal)
        if (!TF_VERIFY(texDesc.textures.size() == 1)) continue;

        uint32_t unit = texDesc.bindingIndex;
        if (textures.size() <= unit) {
            textures.resize(unit+1, 0);
        }

        HgiTextureHandle const& texHandle = texDesc.textures.front();
        HgiGLTexture* glTexture = static_cast<HgiGLTexture*>(texHandle.Get());

        textures[texDesc.bindingIndex] = glTexture->GetTextureId();
    }

    if (!textures.empty()) {
        glBindTextures(0, textures.size(), textures.data());
    }

    // todo here we must loop through .buffers and bind UBO and SSBO buffers.
    // Note that index and vertex buffers are not bound here.
    // They are bound via the GraphicsEncoder.
    TF_VERIFY(_descriptor.buffers.empty(), "Missing implementation buffers");

    HGIGL_POST_PENDING_GL_ERRORS();
}


PXR_NAMESPACE_CLOSE_SCOPE
