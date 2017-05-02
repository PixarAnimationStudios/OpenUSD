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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/resourceBinder.h"

#include "pxr/imaging/hd/drawBatch.h" // XXX: temp
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/shaderCode.h"

#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_int, "int"))
    (vec2)
    (vec3)
    (vec4)
    (dvec3)
    (ivec2)
    (ivec3)
    (ivec4)
    (primitiveParam)
);

namespace {
    struct BindingLocator {
        BindingLocator() :
            uniformLocation(0), uboLocation(0),
            ssboLocation(0), attribLocation(0),
            textureUnit(0) {}

        HdBinding GetBinding(HdBinding::Type type, TfToken const &debugName) {
            switch(type) {
            case HdBinding::UNIFORM:
                return HdBinding(HdBinding::UNIFORM, uniformLocation++);
                break;
            case HdBinding::UBO:
                return HdBinding(HdBinding::UBO, uboLocation++);
                break;
            case HdBinding::SSBO:
                return HdBinding(HdBinding::SSBO, ssboLocation++);
                break;
            case HdBinding::TBO:
                return HdBinding(HdBinding::TBO, uniformLocation++, textureUnit++);
                break;
            case HdBinding::BINDLESS_UNIFORM:
                return HdBinding(HdBinding::BINDLESS_UNIFORM, uniformLocation++);
                break;
            case HdBinding::VERTEX_ATTR:
                return HdBinding(HdBinding::VERTEX_ATTR, attribLocation++);
                break;
            case HdBinding::DRAW_INDEX:
                return HdBinding(HdBinding::DRAW_INDEX, attribLocation++);
                break;
            case HdBinding::DRAW_INDEX_INSTANCE:
                return HdBinding(HdBinding::DRAW_INDEX_INSTANCE, attribLocation++);
                break;
            default:
                TF_CODING_ERROR("Unknown binding type %d for %s",
                                type, debugName.GetText());
                return HdBinding();
                break;
            }
        }

        int uniformLocation;
        int uboLocation;
        int ssboLocation;
        int attribLocation;
        int textureUnit;
    };

    static inline GLboolean _ShouldBeNormalized(int GLdataType)
    {
        if (GLdataType == GL_INT_2_10_10_10_REV ||
            GLdataType == GL_UNSIGNED_INT_2_10_10_10_REV) {
            return GL_TRUE;
        }
        return GL_FALSE;
    }
    static inline int _GetNumComponents(int numComponents, int GLdataType)
    {
        if (GLdataType == GL_INT_2_10_10_10_REV ||
            GLdataType == GL_UNSIGNED_INT_2_10_10_10_REV) {
            return 4;
        } else {
            return numComponents;
        }
    }
}

Hd_ResourceBinder::Hd_ResourceBinder()
    : _numReservedTextureUnits(0)
{
}

void
Hd_ResourceBinder::ResolveBindings(HdDrawItem const *drawItem,
                                   HdShaderCodeSharedPtrVector const &shaders,
                                   Hd_ResourceBinder::MetaData *metaDataOut,
                                   bool indirect,
                                   bool instanceDraw,
                                   HdBindingRequestVector const &customBindings)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(metaDataOut)) return;

    // GL context caps
    const bool ssboEnabled
        = HdRenderContextCaps::GetInstance().shaderStorageBufferEnabled;
    const bool bindlessUniformEnabled
        = HdRenderContextCaps::GetInstance().bindlessBufferEnabled;
    const bool bindlessTextureEnabled
        = HdRenderContextCaps::GetInstance().bindlessTextureEnabled;

    HdBinding::Type arrayBufferBindingType = HdBinding::TBO;  // 3.0
    if (bindlessUniformEnabled) {
        arrayBufferBindingType = HdBinding::BINDLESS_UNIFORM; // EXT
    } else if (ssboEnabled) {
        arrayBufferBindingType = HdBinding::SSBO;             // 4.3
    }

    HdBinding::Type structBufferBindingType = HdBinding::UBO;  // 3.1
    if (bindlessUniformEnabled) {
        structBufferBindingType = HdBinding::BINDLESS_UNIFORM; // EXT
    } else if (ssboEnabled) {
        structBufferBindingType = HdBinding::SSBO;             // 4.3
    }

    HdBinding::Type drawingCoordBindingType = HdBinding::UNIFORM;
    if (indirect) {
        if (instanceDraw) {
            drawingCoordBindingType = HdBinding::DRAW_INDEX_INSTANCE;
        } else {
            drawingCoordBindingType = HdBinding::DRAW_INDEX;
        }
    }

    bool useBindlessForTexture = bindlessTextureEnabled;

    // binding assignments
    BindingLocator locator;
    locator.textureUnit = 5; // XXX: skip glop's texture --- need fix.

    int bindlessTextureLocation = 0;
    // Note that these locations are used for hash keys only and
    // are never used for actual resource binding.
    int shaderFallbackLocation = 0;
    int shaderRedirectLocation = 0;

    // clear all
    _bindingMap.clear();

    // constant primvar (per-object)
    HdBinding constantPrimVarBinding =
                locator.GetBinding(structBufferBindingType,
                                   HdTokens->constantPrimVars);

    if (HdBufferArrayRangeSharedPtr constantBar =
        drawItem->GetConstantPrimVarRange()) {
        MetaData::StructBlock sblock(HdTokens->constantPrimVars);
        TF_FOR_ALL (it, constantBar->GetResources()) {
            sblock.entries.push_back(
                MetaData::StructEntry(/*name=*/it->first,
                                      /*type=*/it->second->GetGLTypeName(),
                                      /*offset=*/it->second->GetOffset(),
                                      /*arraySize=*/it->second->GetArraySize()));
        }
        // sort by offset
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.
        std::sort(sblock.entries.begin(), sblock.entries.end());

        metaDataOut->constantData.insert(
            std::make_pair(constantPrimVarBinding, sblock));
    }

     // constant primvars are interleaved into single struct.
    _bindingMap[HdTokens->constantPrimVars] = constantPrimVarBinding;

    // instance primvar (per-instance)
    int instancerNumLevels = drawItem->GetInstancePrimVarNumLevels();
    metaDataOut->instancerNumLevels = instancerNumLevels;
    for (int i = 0; i < instancerNumLevels; ++i) {
        if (HdBufferArrayRangeSharedPtr instanceBar =
            drawItem->GetInstancePrimVarRange(i)) {
            TF_FOR_ALL (it, instanceBar->GetResources()) {
                // non-interleaved, always create new binding.
                HdBinding instancePrimVarBinding =
                    locator.GetBinding(arrayBufferBindingType, it->first);
                _bindingMap[NameAndLevel(it->first, i)] = instancePrimVarBinding;

                metaDataOut->instanceData[instancePrimVarBinding] =
                    MetaData::NestedPrimVar(/*name=*/it->first,
                                            /*type=*/it->second->GetGLTypeName(),
                                            /*level=*/i);
            }
        }
    }

    // vertex primvar (per-vertex)
    // always assigned to VertexAttribute.
    if (HdBufferArrayRangeSharedPtr vertexBar =
        drawItem->GetVertexPrimVarRange()) {
        TF_FOR_ALL (it, vertexBar->GetResources()) {
            HdBinding vertexPrimVarBinding =
                locator.GetBinding(HdBinding::VERTEX_ATTR, it->first);
            _bindingMap[it->first] = vertexPrimVarBinding;

            metaDataOut->vertexData[vertexPrimVarBinding] =
                MetaData::PrimVar(/*name=*/it->first,
                                  /*type=*/it->second->GetGLTypeName());
        }
    }

    // index buffer
    if (HdBufferArrayRangeSharedPtr topologyBar =
        drawItem->GetTopologyRange()) {
        TF_FOR_ALL (it, topologyBar->GetResources()) {
            if (it->first == HdTokens->indices) {
                // IBO. no need for codegen
                _bindingMap[HdTokens->indices] = HdBinding(HdBinding::INDEX_ATTR, 0);
            } else {
                // primitive parameter (for all tris, quads and patches)
                HdBinding primitiveParamBinding =
                    locator.GetBinding(arrayBufferBindingType, it->first);
                _bindingMap[it->first] = primitiveParamBinding;

                metaDataOut->primitiveParamBinding =
                    MetaData::BindingDeclaration(/*name=*/it->first,
                                                 /*type=*/it->second->GetGLTypeName(),
                                                 /*binding=*/primitiveParamBinding);
            }
        }
    }

    // element primvar (per-face, per-line)
    if (HdBufferArrayRangeSharedPtr elementBar =
        drawItem->GetElementPrimVarRange()) {
        TF_FOR_ALL (it, elementBar->GetResources()) {
            HdBinding elementPrimVarBinding =
                locator.GetBinding(arrayBufferBindingType, it->first);
            _bindingMap[it->first] = elementPrimVarBinding;
            metaDataOut->elementData[elementPrimVarBinding] =
                MetaData::PrimVar(/*name=*/it->first,
                                  /*type=*/it->second->GetGLTypeName());
        }
    }

    // facevarying primvar (per-face-vertex)
    if (HdBufferArrayRangeSharedPtr fvarBar =
        drawItem->GetFaceVaryingPrimVarRange()) {
        TF_FOR_ALL (it, fvarBar->GetResources()) {
            HdBinding fvarPrimVarBinding =
                locator.GetBinding(arrayBufferBindingType, it->first);
            _bindingMap[it->first] = fvarPrimVarBinding;
            metaDataOut->fvarData[fvarPrimVarBinding] =
                MetaData::PrimVar(/*name=*/it->first,
                                  /*type=*/it->second->GetGLTypeName());
        }
    }

    // draw parameter
    // assigned to draw index (vertex attributeI w/divisor) (indiect)
    // assigned to uniform          (immediate)
    //
    // note that instanceDraw may be true even for non-instance drawing,
    // because there's only instanced version of glMultiDrawElementsIndirect.
    HdBinding drawingCoord0Binding = locator.GetBinding(
        drawingCoordBindingType, HdTokens->drawingCoord0);
    _bindingMap[HdTokens->drawingCoord0] = drawingCoord0Binding;
    metaDataOut->drawingCoord0Binding =
        MetaData::BindingDeclaration(/*name=*/HdTokens->drawingCoord0,
                                     /*type=*/_tokens->ivec4,
                                     /*binding=*/drawingCoord0Binding);

    HdBinding drawingCoord1Binding = locator.GetBinding(
        drawingCoordBindingType, HdTokens->drawingCoord1);
    _bindingMap[HdTokens->drawingCoord1] = drawingCoord1Binding;
    metaDataOut->drawingCoord1Binding =
        MetaData::BindingDeclaration(/*name=*/HdTokens->drawingCoord1,
                                     /*type=*/_tokens->ivec3,
                                     /*binding=*/drawingCoord1Binding);

    if (instancerNumLevels > 0) {
        HdBinding drawingCoordIBinding = indirect
            ? HdBinding(HdBinding::DRAW_INDEX_INSTANCE_ARRAY, locator.attribLocation)
            : HdBinding(HdBinding::UNIFORM_ARRAY, locator.uniformLocation);
        if (indirect) {
            // each vertex attribute takes 1 location
            locator.attribLocation += instancerNumLevels;
        } else {
            // int[N] may consume more than 1 location
            locator.uniformLocation += instancerNumLevels;
        }
        _bindingMap[HdTokens->drawingCoordI] = drawingCoordIBinding;
        metaDataOut->drawingCoordIBinding =
            MetaData::BindingDeclaration(/*name=*/HdTokens->drawingCoordI,
                                         /*type=*/_tokens->_int,
                                         /*binding=*/drawingCoordIBinding);
    }

    // instance index indirection buffer
    if (HdBufferArrayRangeSharedPtr instanceIndexBar =
        drawItem->GetInstanceIndexRange()) {
        HdBufferResourceSharedPtr instanceIndices
            = instanceIndexBar->GetResource(HdTokens->instanceIndices);
        HdBufferResourceSharedPtr culledInstanceIndices
            = instanceIndexBar->GetResource(HdTokens->culledInstanceIndices);

        if (instanceIndices) {
            HdBinding instanceIndexArrayBinding =
                locator.GetBinding(arrayBufferBindingType, HdTokens->instanceIndices);
            _bindingMap[HdTokens->instanceIndices] = instanceIndexArrayBinding;
            metaDataOut->instanceIndexArrayBinding =
                MetaData::BindingDeclaration(
                    /*name=*/HdTokens->instanceIndices,
                    /*type=*/instanceIndices->GetGLTypeName(),
                    /*binding=*/instanceIndexArrayBinding);
        }
        if (culledInstanceIndices) {
            HdBinding culledInstanceIndexArrayBinding =
                locator.GetBinding(arrayBufferBindingType, HdTokens->culledInstanceIndices);
            _bindingMap[HdTokens->culledInstanceIndices] =
                culledInstanceIndexArrayBinding;
            metaDataOut->culledInstanceIndexArrayBinding =
                MetaData::BindingDeclaration(
                    /*name=*/HdTokens->culledInstanceIndices,
                    /*type=*/culledInstanceIndices->GetGLTypeName(),
                    /*binding=*/culledInstanceIndexArrayBinding);
        }
    }

    // indirect dispatch
    if (indirect) {
        HdBinding dispatchBinding(HdBinding::DISPATCH, /*location=(not used)*/0);
        _bindingMap[HdTokens->drawDispatch] = dispatchBinding;
    }

    // shader parameter bindings

    TF_FOR_ALL(shader, shaders) {
        HdShaderParamVector params = (*shader)->GetParams();

        // uniform block
        HdBufferArrayRangeSharedPtr const &shaderBar = (*shader)->GetShaderData();
        if (shaderBar) {
            HdBinding shaderParamBinding =
                locator.GetBinding(structBufferBindingType, HdTokens->surfaceShaderParams);

            // for fallback values and bindless textures
            // XXX: name of sblock must be unique for each shaders.
            MetaData::StructBlock sblock(HdTokens->surfaceShaderParams);
            TF_FOR_ALL(it, shaderBar->GetResources()) {
                sblock.entries.push_back(
                    MetaData::StructEntry(/*name=*/it->first,
                                          /*type=*/it->second->GetGLTypeName(),
                                          /*offset=*/it->second->GetOffset(),
                                          /*arraySize=*/it->second->GetArraySize()));
            }
            // sort by offset
            std::sort(sblock.entries.begin(), sblock.entries.end());
            metaDataOut->shaderData.insert(
                std::make_pair(shaderParamBinding, sblock));

            //XXX:hack  we want to generalize surfaceShaderParams to other shaders.
            if ((*shader) == drawItem->GetSurfaceShader()) {
                // shader parameters are interleaved into single struct.
                _bindingMap[HdTokens->surfaceShaderParams] = shaderParamBinding;
            }
        }

        // for primvar and texture accessors
        TF_FOR_ALL(it, params) {
            // renderpass texture should be bindfull (for now)
            bool bindless = useBindlessForTexture && ((*shader) == drawItem->GetSurfaceShader());
            if (it->IsFallback()) {
                metaDataOut->shaderParameterBinding[HdBinding(HdBinding::FALLBACK, shaderFallbackLocation++)]
                    = MetaData::ShaderParameterAccessor(it->GetName(),
                                                        it->GetGLTypeName());
            } else if (it->IsTexture()) {
                if (it->IsPtex()) {
                    // ptex texture
                    HdBinding texelBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL, bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_PTEX_TEXEL, locator.uniformLocation++);

                    metaDataOut->shaderParameterBinding[texelBinding] =
                        MetaData::ShaderParameterAccessor(/*name=*/it->GetName(),
                                                          /*type=*/it->GetGLTypeName());
                    _bindingMap[it->GetName()] = texelBinding; // used for non-bindless

                    TfToken layoutName = TfToken(std::string(it->GetName().GetText()) + "_layout");
                    HdBinding layoutBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT, bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_PTEX_LAYOUT, locator.uniformLocation++);

                    metaDataOut->shaderParameterBinding[layoutBinding] =
                        MetaData::ShaderParameterAccessor(/*name=*/layoutName,
                                                          /*type=*/TfToken("isamplerBuffer"));

                    // XXX: same name ?
                    _bindingMap[layoutName] = layoutBinding; // used for non-bindless
                } else {
                    // 2d texture
                    HdBinding textureBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_2D, bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_2D, locator.uniformLocation++);

                    metaDataOut->shaderParameterBinding[textureBinding] =
                        MetaData::ShaderParameterAccessor(/*name=*/it->GetName(),
                                                          /*type=*/it->GetGLTypeName(),
                                                          /*inPrimVars=*/it->GetSamplerCoordinates());
                    _bindingMap[it->GetName()] = textureBinding; // used for non-bindless
                }
            } else if (it->IsPrimvar()) {
                metaDataOut->shaderParameterBinding[HdBinding(HdBinding::PRIMVAR_REDIRECT, shaderRedirectLocation++)]
                    = MetaData::ShaderParameterAccessor(/*name=*/it->GetName(),
                                                        /*type=*/it->GetGLTypeName(),
                                                        /*inPrimVars=*/it->GetSamplerCoordinates());
            } else {
                TF_CODING_ERROR("Can't resolve %s", it->GetName().GetText());
            }
        }
    }

    // Add custom bindings
    TF_FOR_ALL (it, customBindings) {
        if (it->IsInterleavedBufferArray()) {
            // Interleaved resource, only need a single binding point
            HdBinding binding = locator.GetBinding(it->GetType(), it->GetName());
            MetaData::StructBlock sblock(it->GetName());
            for (auto const& nameRes : it->GetBar()->GetResources()) {
                sblock.entries.push_back(MetaData::StructEntry(
                                             nameRes.first,
                                             nameRes.second->GetGLTypeName(),
                                             nameRes.second->GetOffset(),
                                             nameRes.second->GetArraySize()));
            }
            metaDataOut->customInterleavedBindings.insert(
                std::make_pair(binding, sblock));
            _bindingMap[it->GetName()] = binding;
        } else {
            // Non interleaved resource
            typedef MetaData::BindingDeclaration BindingDeclaration;
            if (it->IsBufferArray()) {
                // The BAR was provided, so we will record the name, dataType,
                // binding type and binding location.
                for (auto const& nameRes : it->GetBar()->GetResources()) {
                    HdBinding binding = locator.GetBinding(it->GetType(), nameRes.first);
                    BindingDeclaration b(nameRes.first, nameRes.second->GetGLTypeName(), binding);
                    metaDataOut->customBindings.push_back(b);
                    _bindingMap[nameRes.first] = binding;
                }
            } else {
                HdBinding binding = locator.GetBinding(it->GetType(), it->GetName());
                BindingDeclaration b(it->GetName(), it->GetGLTypeName(), binding);

                // note that GetGLTypeName() may return empty, in case it's a
                // typeless binding. CodeGen generates declarations and
                // accessors only for BindingDeclaration with a valid type.
                metaDataOut->customBindings.push_back(b);
                _bindingMap[it->GetName()] = binding;
            }
        }
    }
    _numReservedTextureUnits = locator.textureUnit;
}

void
Hd_ResourceBinder::BindBuffer(TfToken const &name,
                              HdBufferResourceSharedPtr const &buffer) const
{
    BindBuffer(name, buffer, buffer->GetOffset(), /*level=*/-1);
}

void
Hd_ResourceBinder::BindBuffer(TfToken const &name,
                              HdBufferResourceSharedPtr const &buffer,
                              int offset,
                              int level) const
{
    HD_TRACE_FUNCTION();

    // it is possible that the buffer has not been initialized when
    // the instanceIndex is empty (e.g. FX points. see bug 120354)
    if (buffer->GetId() == 0) return;

    HdBinding binding = GetBinding(name, level);
    HdBinding::Type type = binding.GetType();
    int loc              = binding.GetLocation();
    int textureUnit      = binding.GetTextureUnit();

    void const* offsetPtr =
        reinterpret_cast<const void*>(
            static_cast<intptr_t>(offset));
    switch(type) {
    case HdBinding::VERTEX_ATTR:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetId());
        glVertexAttribPointer(loc,
                              _GetNumComponents(buffer->GetNumComponents(),
                                                buffer->GetGLDataType()),
                              buffer->GetGLDataType(),
                              _ShouldBeNormalized(buffer->GetGLDataType()),
                              buffer->GetStride(),
                              offsetPtr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetId());
        glVertexAttribIPointer(loc,
                               buffer->GetNumComponents(),
                               GL_INT,
                               buffer->GetStride(),
                               offsetPtr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX_INSTANCE:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetId());
        glVertexAttribIPointer(loc,
                               buffer->GetNumComponents(),
                               GL_INT,
                               buffer->GetStride(),
                               offsetPtr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // set the divisor to uint-max so that the same base value is used
        // for all instances.
        glVertexAttribDivisor(loc,
                              std::numeric_limits<GLint>::max());
        glEnableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX_INSTANCE_ARRAY:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetId());
        // a trick : we store instancerNumLevels in numComponents.
        // it could be more than 4. we unroll it to an array of int[1] attributes.
        //
        for (int i = 0; i < buffer->GetNumComponents(); ++i) {
            offsetPtr = reinterpret_cast<const void*>(offset + i*sizeof(int));
            glVertexAttribIPointer(loc, 1, GL_INT, buffer->GetStride(),
                                   offsetPtr);

            // set the divisor to uint-max so that the same base value is used
            // for all instances.
            glVertexAttribDivisor(loc, std::numeric_limits<GLint>::max());
            glEnableVertexAttribArray(loc);
            ++loc;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        break;
    case HdBinding::INDEX_ATTR:
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->GetId());
        break;
    case HdBinding::BINDLESS_UNIFORM:
        // at least in nvidia driver 346.59, this query call doesn't show
        // any pipeline stall.
        if (!glIsNamedBufferResidentNV(buffer->GetId())) {
            glMakeNamedBufferResidentNV(buffer->GetId(), GL_READ_WRITE);
        }
        glUniformui64NV(loc, buffer->GetGPUAddress());
        break;
    case HdBinding::SSBO:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc,
                         buffer->GetId());
        break;
    case HdBinding::DISPATCH:
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer->GetId());
        break;
    case HdBinding::UBO:
    case HdBinding::UNIFORM:
        glBindBufferRange(GL_UNIFORM_BUFFER, loc,
                          buffer->GetId(),
                          offset,
                          buffer->GetStride());
        break;
    case HdBinding::TBO:
        if (loc != HdBinding::NOT_EXIST) {
            glUniform1i(loc, textureUnit);
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindSampler(textureUnit, 0);
            glBindTexture(GL_TEXTURE_BUFFER, buffer->GetTextureBuffer());
        }
        break;
    case HdBinding::TEXTURE_2D:
        // nothing
        break;
    default:
        TF_CODING_ERROR("binding type %d not found for %s",
                        type, name.GetText());
        break;
    }
}

void
Hd_ResourceBinder::UnbindBuffer(TfToken const &name,
                                HdBufferResourceSharedPtr const &buffer,
                                int level) const
{
    HD_TRACE_FUNCTION();

    // it is possible that the buffer has not been initialized when
    // the instanceIndex is empty (e.g. FX points)
    if (buffer->GetId() == 0) return;

    HdBinding binding = GetBinding(name, level);
    HdBinding::Type type = binding.GetType();
    int loc = binding.GetLocation();

    switch(type) {
    case HdBinding::VERTEX_ATTR:
        glDisableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX:
        glDisableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX_INSTANCE:
        glDisableVertexAttribArray(loc);
        glVertexAttribDivisor(loc, 0);
        break;
    case HdBinding::DRAW_INDEX_INSTANCE_ARRAY:
        for (int i = 0; i < buffer->GetNumComponents(); ++i) {
            glDisableVertexAttribArray(loc);
            glVertexAttribDivisor(loc, 0);
            ++loc;
        }
        break;
    case HdBinding::INDEX_ATTR:
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        break;
    case HdBinding::BINDLESS_UNIFORM:
        if (glIsNamedBufferResidentNV(buffer->GetId())) {
            glMakeNamedBufferNonResidentNV(buffer->GetId());
        }
        break;
    case HdBinding::SSBO:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc, 0);
        break;
    case HdBinding::DISPATCH:
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        break;
    case HdBinding::UBO:
    case HdBinding::UNIFORM:
        glBindBufferBase(GL_UNIFORM_BUFFER, loc, 0);
        break;
    case HdBinding::TBO:
        if (loc != HdBinding::NOT_EXIST) {
            glActiveTexture(GL_TEXTURE0 + binding.GetTextureUnit());
            glBindTexture(GL_TEXTURE_BUFFER, 0);
        }
        break;
    case HdBinding::TEXTURE_2D:
        // nothing
        break;
    default:
        TF_CODING_ERROR("binding type %d not found for %s",
                        type, name.GetText());
        break;
    }
}

void
Hd_ResourceBinder::BindConstantBuffer(
    HdBufferArrayRangeSharedPtr const &constantBar) const
{
    if (!constantBar) return;

    // constant buffer is interleaved. we just need to bind a buffer.
    BindBuffer(HdTokens->constantPrimVars, constantBar->GetResource());
}

void
Hd_ResourceBinder::UnbindConstantBuffer(
    HdBufferArrayRangeSharedPtr const &constantBar) const
{
    if (!constantBar) return;

    UnbindBuffer(HdTokens->constantPrimVars, constantBar->GetResource());
}

void
Hd_ResourceBinder::BindInstanceBufferArray(
    HdBufferArrayRangeSharedPtr const &bar, int level) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        BindBuffer(it->first, it->second, it->second->GetOffset(), level);
    }
}

void
Hd_ResourceBinder::UnbindInstanceBufferArray(
    HdBufferArrayRangeSharedPtr const &bar, int level) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        UnbindBuffer(it->first, it->second, level);
    }
}

void
Hd_ResourceBinder::BindShaderResources(HdShaderCode const *shader) const
{
    // bind fallback values and sampler uniforms (unit#? or bindless address)

    // this is bound in batches.
    //BindBufferArray(shader->GetShaderData());

    // bind textures
    HdShaderCode::TextureDescriptorVector textures = shader->GetTextures();
    TF_FOR_ALL(it, textures) {
        HdBinding binding = GetBinding(it->name);
        HdBinding::Type type = binding.GetType();

        if (type == HdBinding::TEXTURE_2D) {
        } else if (type == HdBinding::BINDLESS_TEXTURE_2D
                || type == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL
                || type == HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT) {
            // nothing? or make it resident?? but it only binds the first one.
            // XXX: it looks like this function should take all textures in the batch.

//            if (!glIsTextureHandleResidentNV(it->handle)) {
//                glMakeTextureHandleResidentNV(it->handle);
//            }
        }
    }
}

void
Hd_ResourceBinder::UnbindShaderResources(HdShaderCode const *shader) const
{
//    UnbindBufferArray(shader->GetShaderData());

    HdShaderCode::TextureDescriptorVector textures = shader->GetTextures();
    TF_FOR_ALL(it, textures) {
        HdBinding binding = GetBinding(it->name);
        HdBinding::Type type = binding.GetType();

        if (type == HdBinding::TEXTURE_2D) {
        } else if (type == HdBinding::BINDLESS_TEXTURE_2D
                || type == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL
                   || type == HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT) {
//            if (glIsTextureHandleResidentNV(it->handle)) {
//                glMakeTextureHandleNonResidentNV(it->handle);
//            }
        }
        // XXX: unbind
    }
}

void
Hd_ResourceBinder::BindBufferArray(HdBufferArrayRangeSharedPtr const &bar) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        BindBuffer(it->first, it->second);
    }
}

void
Hd_ResourceBinder::Bind(HdBindingRequest const& req) const
{
    if (req.IsTypeless()) {
        return;
    } else if (req.IsResource()) {
        BindBuffer(req.GetName(), req.GetResource(), req.GetOffset());
    } else if (req.IsInterleavedBufferArray()) {
        // note: interleaved buffer needs only 1 binding
        BindBuffer(req.GetName(), req.GetBar()->GetResource(), req.GetOffset());
    } else if (req.IsBufferArray()) {
        BindBufferArray(req.GetBar());
    }
}

void
Hd_ResourceBinder::Unbind(HdBindingRequest const& req) const
{
    if (req.IsTypeless()) {
        return;
    } else if (req.IsResource()) {
        UnbindBuffer(req.GetName(), req.GetResource());
    } else if (req.IsInterleavedBufferArray()) {
        // note: interleaved buffer needs only 1 binding
        UnbindBuffer(req.GetName(), req.GetBar()->GetResource());
    } else if (req.IsBufferArray()) {
        UnbindBufferArray(req.GetBar());
    }
}

void
Hd_ResourceBinder::UnbindBufferArray(
    HdBufferArrayRangeSharedPtr const &bar) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        UnbindBuffer(it->first, it->second);
    }
}

void
Hd_ResourceBinder::BindUniformi(TfToken const &name,
                                int count, const int *value) const
{
    HdBinding uniformLocation = GetBinding(name);
    if (uniformLocation.GetLocation() == HdBinding::NOT_EXIST) return;

    TF_VERIFY(uniformLocation.IsValid());
    TF_VERIFY(uniformLocation.GetType() == HdBinding::UNIFORM);

    if (count == 1) {
        glUniform1iv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 2) {
        glUniform2iv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 3) {
        glUniform3iv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 4) {
        glUniform4iv(uniformLocation.GetLocation(), 1, value);
    } else {
        TF_CODING_ERROR("Invalid count %d.\n", count);
    }
}

void
Hd_ResourceBinder::BindUniformArrayi(TfToken const &name,
                                 int count, const int *value) const
{
    HdBinding uniformLocation = GetBinding(name);
    if (uniformLocation.GetLocation() == HdBinding::NOT_EXIST) return;

    TF_VERIFY(uniformLocation.IsValid());
    TF_VERIFY(uniformLocation.GetType() == HdBinding::UNIFORM_ARRAY);

    glUniform1iv(uniformLocation.GetLocation(), count, value);
}

void
Hd_ResourceBinder::BindUniformui(TfToken const &name,
                                int count, const unsigned int *value) const
{
    HdBinding uniformLocation = GetBinding(name);
    if (uniformLocation.GetLocation() == HdBinding::NOT_EXIST) return;

    TF_VERIFY(uniformLocation.IsValid());
    TF_VERIFY(uniformLocation.GetType() == HdBinding::UNIFORM);

    if (count == 1) {
        glUniform1uiv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 2) {
        glUniform2uiv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 3) {
        glUniform3uiv(uniformLocation.GetLocation(), 1, value);
    } else if (count == 4) {
        glUniform4uiv(uniformLocation.GetLocation(), 1, value);
    } else {
        TF_CODING_ERROR("Invalid count %d.", count);
    }
}

void
Hd_ResourceBinder::BindUniformf(TfToken const &name,
                                int count, const float *value) const
{
    HdBinding uniformLocation = GetBinding(name);
    if (uniformLocation.GetLocation() == HdBinding::NOT_EXIST) return;

    if (!TF_VERIFY(uniformLocation.IsValid())) return;
    if (!TF_VERIFY(uniformLocation.GetType() == HdBinding::UNIFORM)) return;
    GLint location = uniformLocation.GetLocation();

    if (count == 1) {
        glUniform1fv(location, 1, value);
    } else if (count == 2) {
        glUniform2fv(location, 1, value);
    } else if (count == 3) {
        glUniform3fv(location, 1, value);
    } else if (count == 4) {
        glUniform4fv(location, 1, value);
    } else if (count == 16) {
        glUniformMatrix4fv(location, 1, /*transpose=*/false, value);
    } else {
        TF_CODING_ERROR("Invalid count %d.", count);
    }
}

void
Hd_ResourceBinder::IntrospectBindings(GLuint program)
{
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    if (ARCH_UNLIKELY(!caps.shadingLanguage420pack)) {
        GLint numUBO = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &numUBO);

        const int MAX_NAME = 256;
        int length = 0;
        char name[MAX_NAME+1];
        for (int i = 0; i < numUBO; ++i) {
            glGetActiveUniformBlockName(program, i, MAX_NAME, &length, name);
            // note: ubo_ prefix is added in HdCodeGen::_EmitDeclaration()
            if (strstr(name, "ubo_") == name) {
                HdBinding binding;
                if (TfMapLookup(_bindingMap, NameAndLevel(TfToken(name+4)), &binding)) {
                    // set uniform block binding.
                    glUniformBlockBinding(program, i, binding.GetLocation());
                }
            }
        }
    }

    if (ARCH_UNLIKELY(!caps.explicitUniformLocation)) {
        TF_FOR_ALL(it, _bindingMap) {
            HdBinding binding = it->second;
            HdBinding::Type type = binding.GetType();
            std::string name = it->first.name;
            int level = it->first.level;
            if (level >=0) {
                // follow nested instancing naming convention.
                std::stringstream n;
                n << name << "_" << level;
                name = n.str();
            }
            if (type == HdBinding::UNIFORM       ||
                type == HdBinding::UNIFORM_ARRAY ||
                type == HdBinding::TBO) {
                GLint loc = glGetUniformLocation(program, name.c_str());
                // update location in resource binder.
                // some uniforms may be optimized out.
                if (loc < 0) loc = HdBinding::NOT_EXIST;
                it->second.Set(type, loc, binding.GetTextureUnit());
            } else if (type == HdBinding::TEXTURE_2D) {
                // note: sampler2d_ prefix is added in
                // HdCodeGen::_GenerateShaderParameters()
                name = "sampler2d_" + name;
                GLint loc = glGetUniformLocation(program, name.c_str());
                if (loc < 0) loc = HdBinding::NOT_EXIST;
                it->second.Set(type, loc, binding.GetTextureUnit());
            }
        }
    }
}

Hd_ResourceBinder::MetaData::ID
Hd_ResourceBinder::MetaData::ComputeHash() const
{
    ID hash = 0;
    
    boost::hash_combine(hash, drawingCoord0Binding.binding.GetValue());
    boost::hash_combine(hash, drawingCoord0Binding.dataType.Hash());
    boost::hash_combine(hash, drawingCoord1Binding.binding.GetValue());
    boost::hash_combine(hash, drawingCoord1Binding.dataType.Hash());
    boost::hash_combine(hash, drawingCoordIBinding.binding.GetValue());
    boost::hash_combine(hash, drawingCoordIBinding.dataType.Hash());
    boost::hash_combine(hash, instanceIndexArrayBinding.binding.GetValue());
    boost::hash_combine(hash, instanceIndexArrayBinding.dataType.Hash());
    boost::hash_combine(hash, instanceIndexBaseBinding.binding.GetValue());
    boost::hash_combine(hash, instanceIndexBaseBinding.dataType.Hash());
    boost::hash_combine(hash, primitiveParamBinding.binding.GetValue());
    boost::hash_combine(hash, primitiveParamBinding.dataType.Hash());

    // separators are inserted to distinguish primvars have a same layout
    // but different interpolation.
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL(binDecl, customBindings) {
        boost::hash_combine(hash, binDecl->name.Hash());
        boost::hash_combine(hash, binDecl->dataType.Hash());
        boost::hash_combine(hash, binDecl->binding.GetType());
        boost::hash_combine(hash, binDecl->binding.GetLocation());
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL(blockIt, customInterleavedBindings) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType.Hash());
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (blockIt, constantData) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType.Hash());
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, instanceData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        NestedPrimVar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType.Hash());
        boost::hash_combine(hash, primvar.level);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, vertexData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        PrimVar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType.Hash());
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, elementData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        PrimVar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType.Hash());
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, fvarData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        PrimVar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType.Hash());
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (blockIt, shaderData) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType.Hash());
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, shaderParameterBinding) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        ShaderParameterAccessor const &entry = it->second;
        boost::hash_combine(hash, entry.name.Hash());
        boost::hash_combine(hash, entry.dataType.Hash());
    }

    return hash;
}

PXR_NAMESPACE_CLOSE_SCOPE

